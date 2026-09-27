#include "xvm.h"

int gc_enabled = 1;

typedef struct gc_str {
	struct gc_str *next;
	usz len;
	int mark;
} gc_str;

#define GS_HDR ((usz)sizeof(gc_str))
#define GS_OF(s) ((gc_str *)((char *)(s) - GS_HDR))
#define GS_DATA(g) ((char *)(g) + GS_HDR)

static gc_str *str_head;
static list *list_head;
static usz alloc_bytes;
static usz threshold = 1024 * 1024;
static long collections;

val vint(long i) {
	val v;
	v.k = V_INT;
	v.i = i;
	v.s = 0;
	v.l = 0;
	return v;
}

val vstr_copy(const char *s) {
	if (!s)
		s = "";
	usz n = xstrlen(s);
	gc_str *g = xmalloc(GS_HDR + n + 1);
	g->len = n;
	g->mark = 0;
	char *d = GS_DATA(g);
	for (usz i = 0; i <= n; i++)
		d[i] = s[i];
	xos_mtx_lock(vm_lock);
	g->next = str_head;
	str_head = g;
	alloc_bytes += GS_HDR + n + 1;
	xos_mtx_unlock(vm_lock);
	val v;
	v.k = V_STR;
	v.i = 0;
	v.s = d;
	v.l = 0;
	return v;
}

val vstr(char *owned) {
	val v = vstr_copy(owned ? owned : "");
	xfree(owned);
	return v;
}

val vlist(list *l) {
	val v;
	v.k = V_LIST;
	v.i = 0;
	v.s = 0;
	v.l = l;
	return v;
}

list *list_new(void) {
	list *l = xcalloc(1, sizeof *l);
	l->cap = 8;
	l->it = xmalloc(l->cap * sizeof(val));
	xos_mtx_lock(vm_lock);
	l->gc_next = list_head;
	list_head = l;
	alloc_bytes += sizeof(list) + l->cap * sizeof(val);
	xos_mtx_unlock(vm_lock);
	return l;
}

void list_push(list *l, val v) {
	if (l->len == l->cap) {
		l->cap *= 2;
		l->it = xrealloc(l->it, l->cap * sizeof(val));
		xos_mtx_lock(vm_lock);
		alloc_bytes += (l->cap / 2) * sizeof(val);
		xos_mtx_unlock(vm_lock);
	}
	l->it[l->len++] = v;
}

long truth(val v) {
	if (v.k == V_INT)
		return v.i != 0;
	if (v.k == V_STR)
		return v.s && v.s[0];
	return v.l && v.l->len;
}

char *tostr(val v) {
	if (v.k == V_STR)
		return xstrdup(v.s ? v.s : "");
	if (v.k == V_INT) {
		char b[32];
		xltoa(v.i, b, sizeof b);
		return xstrdup(b);
	}
	xstr o;
	xstr_init(&o);
	xstr_addc(&o, '[');
	for (usz i = 0; v.l && i < v.l->len; i++) {
		if (i)
			xstr_add(&o, ", ");
		char *s = tostr(v.l->it[i]);
		xstr_add(&o, s);
		xfree(s);
	}
	xstr_addc(&o, ']');
	return xstr_take(&o);
}

long toint(val v) {
	if (v.k == V_INT)
		return v.i;
	if (v.k == V_STR)
		return xatol(v.s ? v.s : "");
	return v.l ? (long)v.l->len : 0;
}

static void mark_val(val v);

static void mark_list(list *l) {
	if (!l || l->gc_mark)
		return;
	l->gc_mark = 1;
	for (usz i = 0; i < l->len; i++)
		mark_val(l->it[i]);
}

static void mark_val(val v) {
	if (v.k == V_LIST)
		mark_list(v.l);
	else if (v.k == V_STR && v.s)
		GS_OF(v.s)->mark = 1;
}

static void collect_locked(void) {
	for (list *l = list_head; l; l = l->gc_next)
		l->gc_mark = 0;
	for (gc_str *g = str_head; g; g = g->next)
		g->mark = 0;

	for (u32 i = 0; i < vm_nglobals; i++)
		mark_val(vm_globals[i]);

	for (vm_thread *t = vm_threads; t; t = t->next) {
		for (usz i = 0; i < t->sp; i++)
			mark_val(t->stack[i]);
		for (int d = 0; d <= t->fdepth && d < VM_MAX_FRAMES; d++) {
			frame *fr = &t->frames[d];
			if (fr->locals)
				for (u32 i = 0; i < fr->nlocals; i++)
					mark_val(fr->locals[i]);
		}
		mark_val(t->result);
		if (t->args)
			mark_list(t->args);
	}
	chan_mark_all(mark_val);

	usz live = 0;
	list **lp = &list_head;
	while (*lp) {
		list *l = *lp;
		if (!l->gc_mark) {
			*lp = l->gc_next;
			xfree(l->it);
			xfree(l);
		} else {
			live += sizeof(list) + l->cap * sizeof(val);
			lp = &l->gc_next;
		}
	}
	gc_str **sp = &str_head;
	while (*sp) {
		gc_str *g = *sp;
		if (!g->mark) {
			*sp = g->next;
			xfree(g);
		} else {
			live += GS_HDR + g->len + 1;
			sp = &g->next;
		}
	}
	alloc_bytes = live;
	threshold = live * 2;
	if (threshold < 1024 * 1024)
		threshold = 1024 * 1024;
	collections++;
}

void gc_maybe(void) {
	if (!gc_enabled || alloc_bytes <= threshold)
		return;
	xos_mtx_lock(vm_lock);
	if (alloc_bytes > threshold && vm_threads && !vm_threads->next)
		collect_locked();
	xos_mtx_unlock(vm_lock);
}

void gc_collect(void) {
	if (!gc_enabled)
		return;
	xos_mtx_lock(vm_lock);
	if (vm_threads && !vm_threads->next)
		collect_locked();
	xos_mtx_unlock(vm_lock);
}

long gc_count(void) {
	return collections;
}

long gc_bytes(void) {
	return (long)alloc_bytes;
}
