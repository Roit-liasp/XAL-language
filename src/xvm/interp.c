#include "xvm.h"

xcore_image *vm_image;
val *vm_globals;
u32 vm_nglobals;
int vm_argc;
char **vm_argv;

int xvm_nest_depth;
int xvm_exit_req;
int xvm_exit_code;
const char *xvm_self_path;

void vmdie(const char *m) {
	xfmt(2, "xvm: %s\n", m);
	xos_exit(1);
}

void push(val v) {
	vm_thread *t = vm_cur;
	if (t->sp == t->stack_cap) {
		t->stack_cap *= 2;
		t->stack = xrealloc(t->stack, t->stack_cap * sizeof(val));
	}
	t->stack[t->sp++] = v;
}

val pop(void) {
	vm_thread *t = vm_cur;
	if (!t->sp)
		vmdie("stack underflow");
	return t->stack[--t->sp];
}

static val concat(val x, val y) {
	char *sx = tostr(x), *sy = tostr(y);
	xstr o;
	xstr_init(&o);
	xstr_add(&o, sx);
	xstr_add(&o, sy);
	xfree(sx);
	xfree(sy);
	return vstr(xstr_take(&o));
}

static int str_eq(val x, val y) {
	char *sx = tostr(x), *sy = tostr(y);
	int r = !xstrcmp(sx, sy);
	xfree(sx);
	xfree(sy);
	return r;
}

static int str_cmp(val x, val y) {
	char *sx = tostr(x), *sy = tostr(y);
	int r = xstrcmp(sx, sy);
	xfree(sx);
	xfree(sy);
	return r < 0 ? -1 : r > 0 ? 1 : 0;
}

static int num_or_str_cmp(val x, val y, int op) {
	long c;
	if (x.k == V_STR || y.k == V_STR)
		c = str_cmp(x, y);
	else {
		long a = toint(x), b = toint(y);
		c = a < b ? -1 : a > b ? 1 : 0;
	}
	switch (op) {
	case OP_LT:
		return c < 0;
	case OP_LE:
		return c <= 0;
	case OP_GT:
		return c > 0;
	default:
		return c >= 0;
	}
}

static val *new_frame_locals(u32 n) {
	val *loc = xmalloc((usz)(n ? n : 1) * sizeof(val));
	for (u32 i = 0; i < n; i++)
		loc[i] = vint(0);
	return loc;
}

int vm_run(vm_thread *t, u32 fnidx, list *args) {
	xcore_image *im = vm_image;
	if (fnidx >= im->nfns)
		vmdie("bad function index");
	xcore_fn *f = &im->fns[fnidx];

	t->fdepth = 0;
	t->frames[0].ret = 0xffffffffu;
	t->frames[0].nlocals = f->nlocals;
	t->frames[0].locals = new_frame_locals(f->nlocals);
	if (args)
		for (usz i = 0; i < args->len && i < (usz)f->nparams; i++)
			t->frames[0].locals[i] = args->it[i];

	u32 pc = f->entry;
	for (;;) {
		if (xvm_exit_req) {
			for (int d = 0; d <= t->fdepth && d < VM_MAX_FRAMES; d++) {
				xfree(t->frames[d].locals);
				t->frames[d].locals = 0;
				t->frames[d].nlocals = 0;
			}
			t->fdepth = 0;
			return xvm_exit_code;
		}
		if (pc >= im->ncode)
			vmdie("pc out of range");
		u32 op = im->code[pc].op;
		long a = im->code[pc].a;
		pc++;
		frame *fr = &t->frames[t->fdepth];
		switch (op) {
		case OP_NOP:
			break;
		case OP_PUSHI:
			push(vint(a));
			break;
		case OP_PUSHS:
			if ((u32)a >= im->nstrs)
				vmdie("string index out of range");
			push(vstr_copy(im->strs[a]));
			break;
		case OP_LOAD:
			if ((u32)a >= fr->nlocals)
				vmdie("local slot out of range");
			push(fr->locals[a]);
			break;
		case OP_STORE:
			if ((u32)a >= fr->nlocals)
				vmdie("local slot out of range");
			fr->locals[a] = pop();
			break;
		case OP_GLOAD:
			if ((u32)a >= vm_nglobals)
				vmdie("global slot out of range");
			xos_mtx_lock(vm_lock);
			push(vm_globals[a]);
			xos_mtx_unlock(vm_lock);
			break;
		case OP_GSTORE: {
			if ((u32)a >= vm_nglobals)
				vmdie("global slot out of range");
			val v = pop();
			xos_mtx_lock(vm_lock);
			vm_globals[a] = v;
			xos_mtx_unlock(vm_lock);
			break;
		}
		case OP_ADD: {
			val y = pop(), x = pop();
			if (x.k == V_STR || y.k == V_STR)
				push(concat(x, y));
			else
				push(vint(toint(x) + toint(y)));
			break;
		}
		case OP_CONCAT: {
			val y = pop(), x = pop();
			push(concat(x, y));
			break;
		}
		case OP_SUB: {
			val y = pop(), x = pop();
			push(vint(toint(x) - toint(y)));
			break;
		}
		case OP_MUL: {
			val y = pop(), x = pop();
			push(vint(toint(x) * toint(y)));
			break;
		}
		case OP_DIV: {
			val y = pop(), x = pop();
			long d = toint(y);
			push(vint(d ? toint(x) / d : 0));
			break;
		}
		case OP_MOD: {
			val y = pop(), x = pop();
			long d = toint(y);
			push(vint(d ? toint(x) % d : 0));
			break;
		}
		case OP_NEG:
			push(vint(-toint(pop())));
			break;
		case OP_NOT:
			push(vint(!truth(pop())));
			break;
		case OP_EQ: {
			val y = pop(), x = pop();
			push(vint((x.k == V_STR || y.k == V_STR) ? str_eq(x, y) : toint(x) == toint(y)));
			break;
		}
		case OP_NE: {
			val y = pop(), x = pop();
			push(vint((x.k == V_STR || y.k == V_STR) ? !str_eq(x, y) : toint(x) != toint(y)));
			break;
		}
		case OP_LT:
		case OP_LE:
		case OP_GT:
		case OP_GE: {
			val y = pop(), x = pop();
			push(vint(num_or_str_cmp(x, y, op)));
			break;
		}
		case OP_JMP:
			if ((u32)a <= pc)
				gc_maybe();
			pc = (u32)a;
			break;
		case OP_JZ:
			if (!truth(pop()))
				pc = (u32)a;
			break;
		case OP_JNZ:
			if (truth(pop()))
				pc = (u32)a;
			break;
		case OP_POP:
			pop();
			break;
		case OP_DUP: {
			val v = pop();
			push(v);
			push(v);
			break;
		}
		case OP_NEWLIST:
			push(vlist(list_new()));
			break;
		case OP_APPEND: {
			val item = pop(), lv = pop();
			if (lv.k == V_LIST && lv.l)
				list_push(lv.l, item);
			push(lv);
			break;
		}
		case OP_INDEX: {
			val idx = pop(), base = pop();
			long i = toint(idx);
			if (base.k == V_LIST && base.l) {
				if (i < 0 || i >= (long)base.l->len)
					push(vint(0));
				else
					push(base.l->it[i]);
			} else if (base.k == V_STR) {
				long l = (long)xstrlen(base.s);
				if (i < 0 || i >= l)
					push(vstr_copy(""));
				else {
					char c[2] = { base.s[i], 0 };
					push(vstr_copy(c));
				}
			} else
				push(vint(0));
			break;
		}
		case OP_SETIDX: {
			val v = pop(), idx = pop(), base = pop();
			long i = toint(idx);
			if (base.k == V_LIST && base.l && i >= 0) {
				while ((long)base.l->len <= i)
					list_push(base.l, vint(0));
				base.l->it[i] = v;
			}
			break;
		}
		case OP_LEN: {
			val v = pop();
			if (v.k == V_LIST)
				push(vint(v.l ? (long)v.l->len : 0));
			else {
				char *s = tostr(v);
				push(vint((long)xstrlen(s)));
				xfree(s);
			}
			break;
		}
		case OP_NCALL: {
			u32 id = XCORE_IDX(a);
			u32 argc = XCORE_ARGC(a);
			if (id >= (u32)NAT__COUNT)
				vmdie("unknown native (image newer than this xvm?)");
			if (argc > XCORE_MAX_ARGS)
				vmdie("too many native arguments");
			if (xvm_nat_caps[id] & xvm_caps_missing) {
				for (u32 i = 0; i < argc; i++)
					pop();
				xvm_nat_unsupported((int)id);
				push(vint(-1));
				break;
			}
			val args[XCORE_MAX_ARGS];
			for (u32 i = argc; i-- > 0;)
				args[i] = pop();
			push(native_call((int)id, args, (int)argc));
			break;
		}
		case OP_CALL: {
			u32 fi = XCORE_IDX(a);
			u32 argc = XCORE_ARGC(a);
			if (fi >= im->nfns)
				vmdie("bad function index");
			xcore_fn *g = &im->fns[fi];
			if (t->fdepth + 1 >= VM_MAX_FRAMES)
				vmdie("call depth exceeded");
			val *loc = new_frame_locals(g->nlocals);
			for (u32 i = argc; i-- > 0;) {
				val v = pop();
				if (i < g->nparams)
					loc[i] = v;
			}
			t->fdepth++;
			t->frames[t->fdepth].ret = pc;
			t->frames[t->fdepth].locals = loc;
			t->frames[t->fdepth].nlocals = g->nlocals;
			pc = g->entry;
			break;
		}
		case OP_RET: {
			val rv = pop();
			xfree(t->frames[t->fdepth].locals);
			t->frames[t->fdepth].locals = 0;
			t->frames[t->fdepth].nlocals = 0;
			if (!t->fdepth) {
				push(rv);
				return (int)toint(rv);
			}
			pc = t->frames[t->fdepth].ret;
			t->fdepth--;
			push(rv);
			break;
		}
		case OP_HALT:
			return 0;
		default:
			vmdie("bad opcode");
		}
	}
}

int xvm_run_image(xcore_image *im, int argc, char **argv) {
	if (!vm_lock)
		vm_lock = xos_mtx_new();

	{
		char missing[256];
		if (!xvm_caps_check(im, missing, sizeof missing)) {
			if (xvm_caps_strict) {
				xfmt(2, "xvm: cannot run image: unsupported here: %s\n", missing);
				return 126;
			}
			xfmt(2, "xvm: warning: image needs unsupported features: %s\n", missing);
		}
	}

	vm_image = im;
	vm_argc = argc;
	vm_argv = argv;

	vm_nglobals = im->nglobals;
	vm_globals = xmalloc((usz)(im->nglobals ? im->nglobals : 1) * sizeof(val));
	for (u32 i = 0; i < im->nglobals; i++)
		vm_globals[i] = vint(0);

	vm_thread *mt = thread_new(im->initidx, 0);
	vm_cur = mt;
	thread_register(mt);

	vm_run(mt, im->initidx, 0);
	mt->sp = 0;
	int rc = vm_run(mt, im->mainidx, 0);
	return rc & 255;
}

#ifndef XVM_NEST_MAX
#define XVM_NEST_MAX 8
#endif
int xvm_run_nested(const u8 *buf, usz len, int argc, char **argv) {
	if (xvm_nest_depth >= XVM_NEST_MAX)
		return -1;
	void *saved = vm_state_save();
	if (!saved)
		return -1;
	xvm_nest_depth++;
	int rc = xvm_run_mem(buf, len, argc, argv);
	xvm_nest_depth--;
	xvm_exit_req = 0;
	vm_state_restore(saved);
	return rc;
}

int xvm_run_mem(const u8 *buf, usz len, int argc, char **argv) {
	xcore_image im;
	if (xcore_image_read(buf, len, &im) < 0) {
		xfmt(2, "xvm: bad bytecode image: %s\n", xcore_image_err());
		return 1;
	}
	xcore_image *heap = xmalloc(sizeof im);
	*heap = im;
	return xvm_run_image(heap, argc, argv);
}

struct vm_saved {
	xcore_image *image;
	val         *globals;
	u32          nglobals;
	int          argc;
	char       **argv;
	vm_thread   *cur;
};

void *vm_state_save(void) {
	struct vm_saved *s = xmalloc(sizeof *s);
	if (!s) return 0;
	s->image    = vm_image;
	s->globals  = vm_globals;
	s->nglobals = vm_nglobals;
	s->argc     = vm_argc;
	s->argv     = vm_argv;
	s->cur      = vm_cur;
	return s;
}

void vm_state_restore(void *p) {
	struct vm_saved *s = (struct vm_saved *)p;
	if (!s) return;

	if (vm_globals && vm_globals != s->globals)
		xfree(vm_globals);
	if (vm_cur && vm_cur != s->cur) {
		thread_unregister(vm_cur);
		xfree(vm_cur->stack);
		xfree(vm_cur);
	}

	vm_image    = s->image;
	vm_globals  = s->globals;
	vm_nglobals = s->nglobals;
	vm_argc     = s->argc;
	vm_argv     = s->argv;
	vm_cur      = s->cur;

	xfree(s);
}
