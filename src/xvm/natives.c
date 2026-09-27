#include "xvm.h"

#define S(i) (n > (i) ? tostr(args[i]) : xstrdup(""))
#define SD(i, d) (n > (i) ? tostr(args[i]) : xstrdup(d))
#define I(i, d) (n > (i) ? toint(args[i]) : (long)(d))
#define ISLIST(i) (n > (i) && args[i].k == V_LIST && args[i].l)

static int at_eof;

static const struct {
	u32 bit;
	const char *name;
} cap_names[] = {
	{ XVM_CAP_IO, "io" },
	{ XVM_CAP_FS, "fs" },
	{ XVM_CAP_DIR, "dir" },
	{ XVM_CAP_ENV, "env" },
	{ XVM_CAP_CWD, "cwd" },
	{ XVM_CAP_PROC, "proc" },
	{ XVM_CAP_KILL, "kill" },
	{ XVM_CAP_FORK, "fork" },
	{ XVM_CAP_PIPE, "pipe" },
	{ XVM_CAP_SIGNAL, "signal" },
	{ XVM_CAP_TERM, "term" },
	{ XVM_CAP_THREADS, "threads" },
	{ XVM_CAP_TIME, "time" },
	{ XVM_CAP_XB, "xb" },
	{ XVM_CAP_UNKNOWN, "unknown" },
};

static u32 xvm_cap_find(const char *n) {
	for (usz i = 0; i < sizeof cap_names / sizeof cap_names[0]; i++)
		if (!xstrcmp(cap_names[i].name, n))
			return cap_names[i].bit;
	return 0;
}

const char *xvm_cap_name(u32 bit) {
	for (usz i = 0; i < sizeof cap_names / sizeof cap_names[0]; i++)
		if (cap_names[i].bit == bit)
			return cap_names[i].name;
	return "?";
}

const u32 xvm_nat_caps[NAT__COUNT] = {
	[NAT_PRINT] = XVM_CAP_IO,
	[NAT_PUTS] = XVM_CAP_IO,
	[NAT_READ] = XVM_CAP_IO,
	[NAT_WRITE] = XVM_CAP_IO,
	[NAT_OPEN] = XVM_CAP_IO,
	[NAT_CLOSE] = XVM_CAP_IO,
	[NAT_EXEC] = XVM_CAP_PROC,
	[NAT_SPAWN] = XVM_CAP_PROC,
	[NAT_WAIT] = XVM_CAP_PROC,
	[NAT_EXIT] = XVM_CAP_PROC,
	[NAT_EXECV] = XVM_CAP_PROC,
	[NAT_GETENV] = XVM_CAP_ENV,
	[NAT_SETENV] = XVM_CAP_ENV,
	[NAT_CHDIR] = XVM_CAP_CWD,
	[NAT_GETCWD] = XVM_CAP_CWD,
	[NAT_EXISTS] = XVM_CAP_FS,
	[NAT_ISDIR] = XVM_CAP_FS,
	[NAT_ISEXEC] = XVM_CAP_FS,
	[NAT_MTIME] = XVM_CAP_TIME,
	[NAT_MKDIRP] = XVM_CAP_FS,
	[NAT_UNLINK] = XVM_CAP_FS,
	[NAT_RENAME] = XVM_CAP_FS,
	[NAT_READDIR] = XVM_CAP_DIR,
	[NAT_READFILE] = XVM_CAP_FS,
	[NAT_WRITEFILE] = XVM_CAP_FS,
	[NAT_SYS] = XVM_CAP_PROC,
	[NAT_INPUT] = XVM_CAP_IO,
	[NAT_EOF] = XVM_CAP_IO,
	[NAT_FORK] = XVM_CAP_FORK,
	[NAT_DUP2] = XVM_CAP_PIPE,
	[NAT_PIPE] = XVM_CAP_PIPE,
	[NAT_SIGNAL] = XVM_CAP_SIGNAL,
	[NAT_WHICH] = XVM_CAP_FS,
	[NAT_NOW] = XVM_CAP_TIME,
	[NAT_SLEEP] = XVM_CAP_TIME,
	[NAT_RAWMODE] = XVM_CAP_TERM,
	[NAT_GETKEY] = XVM_CAP_TERM,
	[NAT_TERMSIZE] = XVM_CAP_TERM,
	[NAT_ISATTY] = XVM_CAP_TERM,
	[NAT_SPAWN_THREAD] = XVM_CAP_THREADS,
	[NAT_JOIN_THREAD] = XVM_CAP_THREADS,
	[NAT_THREAD_ID] = XVM_CAP_THREADS,
	[NAT_THREAD_CPU] = XVM_CAP_THREADS,
	[NAT_SET_AFFINITY] = XVM_CAP_THREADS,
	[NAT_CHAN_NEW] = XVM_CAP_THREADS,
	[NAT_CHAN_SEND] = XVM_CAP_THREADS,
	[NAT_CHAN_RECV] = XVM_CAP_THREADS,
	[NAT_CHAN_TRYRECV] = XVM_CAP_THREADS,
	[NAT_CHAN_CLOSE] = XVM_CAP_THREADS,
	[NAT_PARALLEL_MAP] = XVM_CAP_THREADS,
	[NAT_RUN_XB] = XVM_CAP_XB,
	[NAT_GETPID] = XVM_CAP_PROC,
	[NAT_KILL] = XVM_CAP_PROC | XVM_CAP_KILL,
	[NAT_WAITPID] = XVM_CAP_PROC,
};

#ifndef XOS_HAS_FORK
#define XOS_HAS_FORK XOS_HAS_PROC
#endif
#ifndef XOS_HAS_PIPE
#define XOS_HAS_PIPE XOS_HAS_PROC
#endif
#ifndef XOS_HAS_SIGNAL
#define XOS_HAS_SIGNAL XOS_HAS_PROC
#endif

#ifndef XVM_CAPS_DEFAULT
#define XVM_CAPS_DEFAULT (XVM_CAP_IO | XVM_CAP_TIME | XVM_CAP_XB \
	| (XOS_HAS_FS ? XVM_CAP_FS : 0u)                         \
	| (XOS_HAS_DIR ? XVM_CAP_DIR : 0u)                       \
	| (XOS_HAS_ENV ? XVM_CAP_ENV : 0u)                       \
	| (XOS_HAS_CWD ? XVM_CAP_CWD : 0u)                       \
	| (XOS_HAS_PROC ? XVM_CAP_PROC : 0u)                     \
	| (XOS_HAS_PROC ? XVM_CAP_KILL : 0u)                     \
	| (XOS_HAS_FORK ? XVM_CAP_FORK : 0u)                     \
	| (XOS_HAS_PIPE ? XVM_CAP_PIPE : 0u)                     \
	| (XOS_HAS_SIGNAL ? XVM_CAP_SIGNAL : 0u)                 \
	| (XOS_HAS_TERM ? XVM_CAP_TERM : 0u)                     \
	| (XOS_HAS_THREADS ? XVM_CAP_THREADS : 0u))
#endif

static u32 caps_mask = XVM_CAPS_DEFAULT;
static int caps_ready;
int xvm_caps_strict = 1;

u32 xvm_caps_missing = ~(u32)XVM_CAPS_DEFAULT;

static void caps_sync(void) {
	xvm_caps_missing = ~caps_mask;
}

#if XOS_HAS_ENV
static void caps_apply(const char *tok) {
	int add = 1;
	if (*tok == '+' || *tok == '-') {
		add = (*tok == '+');
		tok++;
	}
	u32 bit = xvm_cap_find(tok);
	if (!bit)
		return;
	if (add)
		caps_mask |= bit;
	else
		caps_mask &= ~bit;
	caps_sync();
}
#endif

static void caps_init(void) {
	if (caps_ready)
		return;
	caps_ready = 1;
	caps_sync();
#if XOS_HAS_ENV
	const char *e = xgetenv("XVM_CAPS_STRICT");
	if (e)
		xvm_caps_strict = !(e[0] == '0' && !e[1]);
	e = xgetenv("XVM_CAPS");
	if (e && e[0]) {
		char buf[128];
		xstrlcpy(buf, e, sizeof buf);
		for (char *p = buf; *p;) {
			while (*p == ' ' || *p == ',' || *p == '\t')
				p++;
			if (!*p)
				break;
			char *q = p;
			while (*q && *q != ' ' && *q != ',' && *q != '\t')
				q++;
			char c = *q;
			*q = 0;
			caps_apply(p);
			p = q;
			if (!c)
				break;
			p++;
		}
	}
#endif
}

u32 xvm_caps(void) {
	caps_init();
	return caps_mask;
}

void xvm_caps_set(u32 mask) {
	caps_init();
	caps_mask = mask;
	caps_sync();
}

void xvm_caps_enable(u32 mask) {
	caps_init();
	caps_mask |= mask;
	caps_sync();
}

void xvm_caps_disable(u32 mask) {
	caps_init();
	caps_mask &= ~mask;
	caps_sync();
}

void xvm_nat_unsupported(int id) {
	static u32 warned[(NAT__COUNT + 31) / 32];
	int w = id >> 5, b = id & 31;
	if (id < 0 || id >= NAT__COUNT || (warned[w] & ((u32)1 << b)))
		return;
	warned[w] |= (u32)1 << b;
	xfmt(2, "xvm: %s: not supported by this host\n", xcore_nat_name[id]);
}

void xvm_caps_format(u32 mask, char *buf, usz n) {
	xstr o;
	xstr_init(&o);
	for (usz i = 0; i < sizeof cap_names / sizeof cap_names[0]; i++)
		if (mask & cap_names[i].bit) {
			if (o.len)
				xstr_addc(&o, ' ');
			xstr_add(&o, cap_names[i].name);
		}
	xstrlcpy(buf, o.p ? o.p : "", n);
	xstr_free(&o);
}

u32 xvm_image_caps(const xcore_image *im) {
	u32 need = 0;
	if (!im || !im->code)
		return 0;
	for (u32 i = 0; i < im->ncode; i++) {
		if (im->code[i].op != OP_NCALL)
			continue;
		u32 id = XCORE_IDX(im->code[i].a);
		if (id >= (u32)NAT__COUNT)
			need |= XVM_CAP_UNKNOWN;
		else
			need |= xvm_nat_caps[id];
	}
	return need;
}

int xvm_caps_check(const xcore_image *im, char *buf, usz n) {
	u32 missing = xvm_image_caps(im) & ~xvm_caps();
	xvm_caps_format(missing, buf, n);
	return missing ? 0 : 1;
}

static char **build_argv(val v) {
	if (v.k != V_LIST || !v.l || !v.l->len)
		return 0;
	char **a = xcalloc(v.l->len + 1, sizeof(char *));
	for (usz i = 0; i < v.l->len; i++)
		a[i] = tostr(v.l->it[i]);
	a[v.l->len] = 0;
	return a;
}

static char **argv_of(val v, const char *deflt) {
	if (v.k == V_LIST && v.l && v.l->len)
		return build_argv(v);
	char *one = v.k == V_STR && v.s && v.s[0] ? xstrdup(v.s) : xstrdup(deflt ? deflt : "");
	if (!one[0]) {
		xfree(one);
		return 0;
	}
	char **a = xcalloc(2, sizeof(char *));
	a[0] = one;
	a[1] = 0;
	return a;
}

static char *resolve_prog(const char *name) {
	if (xstrchr(name, '/'))
		return xstrdup(name);
	const char *path = 0;
#if XOS_HAS_ENV
	path = xgetenv("PATH");
#endif
	if (!path || !path[0])
		path = "/bin:/usr/bin";
	char *res = 0;
	while (*path && !res) {
		const char *e = xstrchr(path, ':');
		usz l = e ? (usz)(e - path) : xstrlen(path);
		xstr b;
		xstr_init(&b);
		xstr_addn(&b, path, l);
		xstr_addc(&b, '/');
		xstr_add(&b, name);
		if (xisexec(b.p))
			res = xstr_take(&b);
		else
			xstr_free(&b);
		if (!e)
			break;
		path = e + 1;
	}
	return res;
}

static void free_argv(char **a) {
	if (!a)
		return;
	for (int i = 0; a[i]; i++)
		xfree(a[i]);
	xfree(a);
}

static long find_fn(val v) {
	if (v.k == V_INT)
		return (v.i >= 0 && (u32)v.i < vm_image->nfns) ? v.i : -1;
	char *name = tostr(v);
	long r = -1;
	for (u32 i = 0; i < vm_image->nfns; i++)
		if (!xstrcmp(vm_image->strs[vm_image->fns[i].nameidx], name)) {
			r = (long)i;
			break;
		}
	xfree(name);
	return r;
}

static val list_copy(list *src) {
	list *out = list_new();
	for (usz i = 0; i < src->len; i++)
		list_push(out, src->it[i]);
	return vlist(out);
}

static val nat_split(char *s, char *sep) {
	list *l = list_new();
	usz sl = xstrlen(sep);
	if (!sl) {
		list_push(l, vstr(s));
		xfree(sep);
		return vlist(l);
	}
	char *p = s;
	for (;;) {
		char *q = xstrstr(p, sep);
		if (!q) {
			if (*p || l->len)
				list_push(l, vstr_copy(p));
			break;
		}
		list_push(l, vstr(xstrndup(p, (usz)(q - p))));
		p = q + sl;
	}
	xfree(s);
	xfree(sep);
	return vlist(l);
}

static val nat_which(char *nm) {
	if (xstrchr(nm, '/')) {
		int ok = xisexec(nm);
		return ok ? vstr(nm) : (xfree(nm), vstr_copy(""));
	}
	const char *path = xgetenv("PATH");
	if (!path)
		path = "/bin:/usr/bin";
	char *res = 0;
	while (*path && !res) {
		const char *e = xstrchr(path, ':');
		usz l = e ? (usz)(e - path) : xstrlen(path);
		xstr b;
		xstr_init(&b);
		xstr_addn(&b, path, l);
		xstr_addc(&b, '/');
		xstr_add(&b, nm);
		if (xisexec(b.p))
			res = xstr_take(&b);
		else
			xstr_free(&b);
		if (!e)
			break;
		path = e + 1;
	}
	xfree(nm);
	return res ? vstr(res) : vstr_copy("");
}

#if XOS_HAS_PROC

static long run_xb_external(const char *path, char **a) {
	int argc = 0;
	while (a[argc])
		argc++;

	const char *vm = xvm_self_path;
	char *from_env = 0;
#if XOS_HAS_ENV
	const char *e = xgetenv("XAL_VM");
	if (e && e[0])
		vm = from_env = xstrdup(e);
#endif
	char *exe = 0;
	if (vm && vm[0] && xexists(vm))
		exe = xstrdup(vm);
	if (!exe) {
		val w = nat_which(xstrdup("xvm"));
		if (w.s && w.s[0])
			exe = xstrdup(w.s);
	}
	xfree(from_env);
	if (!exe)
		return -1;

	char **av = xcalloc((usz)(argc + 3), sizeof(char *));
	av[0] = exe;
	av[1] = xstrdup(path);
	for (int i = 0; i < argc; i++)
		av[i + 2] = xstrdup(a[i]);
	av[argc + 2] = 0;

	long rc = xrun(av, 0);
	free_argv(av);
	return rc;
}
#else
static long run_xb_external(const char *path, char **a) {
	(void)path;
	(void)a;
	return -1;
}
#endif

static val nat_run_xb(char *path, val argsv) {
	char **a = argv_of(argsv, path);
	if (!a) {
		xfree(path);
		return vint(-1);
	}
	int argc = 0;
	while (a[argc])
		argc++;

	usz len = 0;
	char *raw = xslurp(path, &len);
	if (!raw) {
		free_argv(a);
		xfree(path);
		return vint(-1);
	}

	long rc = -1;
	if (!(xvm_caps() & XVM_CAP_XB))
		rc = -1;
	else if (len < 4 || xstrncmp(raw, XAL_MAGIC, 4))
		rc = -1;
	else
		rc = xvm_run_nested((const u8 *)raw, len, argc, a);
	xfree(raw);

	if (rc < 0)
		rc = run_xb_external(path, a);

	free_argv(a);
	xfree(path);
	return vint(rc >= 0 ? rc : -1);
}

static val nat_sort(list *src) {
	val out = list_copy(src);
	list *o = out.l;
	for (usz i = 1; i < o->len; i++) {
		val key = o->it[i];
		char *ks = tostr(key);
		usz j = i;
		while (j > 0) {
			char *ps = tostr(o->it[j - 1]);
			int c = xstrcmp(ps, ks);
			xfree(ps);
			if (c <= 0)
				break;
			o->it[j] = o->it[j - 1];
			j--;
		}
		xfree(ks);
		o->it[j] = key;
	}
	return out;
}

static val nat_replace(char *hay, char *nd, char *rp) {
	usz nl = xstrlen(nd);
	if (!nl) {
		xfree(nd);
		xfree(rp);
		return vstr(hay);
	}
	xstr o;
	xstr_init(&o);
	for (char *p = hay; *p;) {
		if (!xstrncmp(p, nd, nl)) {
			xstr_add(&o, rp);
			p += nl;
		} else
			xstr_addc(&o, *p++);
	}
	xfree(hay);
	xfree(nd);
	xfree(rp);
	return vstr(xstr_take(&o));
}

static val nat_parallel_map(val fn, list *src) {
	long fi = find_fn(fn);
	if (fi < 0 || !src->len)
		return vlist(list_new());
	usz len = src->len;
	long *ids = xmalloc(len * sizeof(long));
	int ncpu = xos_cpu_count();
	if (ncpu < 1)
		ncpu = 1;
	list *out = list_new();
	usz next = 0, done = 0;
	while (done < len) {
		usz batch = 0;
		while (next < len && (int)batch < ncpu) {
			list *al = list_new();
			list_push(al, src->it[next]);
			vm_thread *t = thread_new((u32)fi, al);
			ids[next] = thread_spawn(t);
			next++;
			batch++;
		}
		for (usz k = 0; k < batch; k++)
			list_push(out, thread_join(ids[done + k]));
		done += batch;
	}
	xfree(ids);
	return vlist(out);
}

val native_call(int id, val *args, int n) {
	switch (id) {
	case NAT_PRINT:
		for (int i = 0; i < n; i++) {
			char *s = tostr(args[i]);
			if (i)
				xout(" ");
			xout(s);
			xfree(s);
		}
		xout("\n");
		return vint(0);
	case NAT_PUTS:
		for (int i = 0; i < n; i++) {
			char *s = tostr(args[i]);
			xout(s);
			xfree(s);
		}
		return vint(0);
	case NAT_LEN: {
		if (!n)
			return vint(0);
		if (args[0].k == V_LIST)
			return vint(args[0].l ? (long)args[0].l->len : 0);
		char *s = tostr(args[0]);
		long l = (long)xstrlen(s);
		xfree(s);
		return vint(l);
	}
	case NAT_STR:
		return vstr(S(0));
	case NAT_INT:
		return vint(I(0, 0));

	case NAT_READ: {
		int fd = (int)I(0, 0);
		long want = I(1, 4096);
		if (want < 0)
			want = 0;
		char *b = xmalloc((usz)want + 1);
		isz r = fd == 0 ? xos_con_read(b, (usz)want) : xread(fd, b, (usz)want);
		if (r <= 0) {
			r = 0;
			at_eof = 1;
		} else if (fd == 0) {
			at_eof = 0;
		} else {
			at_eof = (long)r < want;
		}
		b[r] = 0;
		return vstr(b);
	}
	case NAT_WRITE: {
		int fd = (int)I(0, 1);
		char *s = S(1);
		isz r = xos_write(fd, s, xstrlen(s));
		xfree(s);
		return vint((long)r);
	}
	case NAT_OPEN: {
		char *p = S(0);
		char *m = SD(1, "r");
		int fl = !xstrcmp(m, "w") ? XOS_O_WRITE | XOS_O_CREATE | XOS_O_TRUNC :
		         !xstrcmp(m, "a") ? XOS_O_WRITE | XOS_O_CREATE | XOS_O_APPEND : XOS_O_READ;
		int fd = xopen(p, fl, 0644);
		xfree(p);
		xfree(m);
		return vint(fd);
	}
	case NAT_CLOSE:
		return vint(n ? xclose((int)toint(args[0])) : -1);

	case NAT_EXEC:
	case NAT_SPAWN:
	case NAT_EXECV: {
		char **a = n ? argv_of(args[0], "") : 0;
		if (!a)
			return vint(-1);
		char *prog = resolve_prog(a[0]);
		if (prog) {
			xfree(a[0]);
			a[0] = prog;
		}
		long r = id == NAT_EXEC ? xrun(a, 0) : id == NAT_SPAWN ? xspawn(a, 0) : xos_proc_exec(a, 0);
		free_argv(a);
		return vint(r);
	}
	case NAT_WAIT:
		return vint(n ? xwait(toint(args[0])) : -1);
	case NAT_WAITPID: {
		long pid = I(0, 0);
		int nohang = (int)I(1, 0);
		return vint(nohang ? xos_proc_trywait(pid) : xos_proc_wait(pid));
	}
	case NAT_GETPID:
		return vint(xos_proc_getpid());
	case NAT_KILL:
		return vint(xos_proc_kill(I(0, 0), (int)I(1, 0)));
	case NAT_EXIT: {
		int code = (int)I(0, 0);
		if (xvm_nest_depth > 0) {
			xvm_exit_code = code;
			xvm_exit_req = 1;
			return vint(code);
		}
		xos_exit(code);
		return vint(0);
	}
	case NAT_FORK:
		return vint(xos_proc_fork());
	case NAT_DUP2:
		return vint(n >= 2 ? xos_dup2((int)toint(args[0]), (int)toint(args[1])) : -1);
	case NAT_PIPE: {
		int fds[2];
		if (xos_pipe(fds) != 0)
			return vlist(list_new());
		list *l = list_new();
		list_push(l, vint(fds[0]));
		list_push(l, vint(fds[1]));
		return vlist(l);
	}
	case NAT_SIGNAL:
		return vint(n >= 2 ? xos_signal((int)toint(args[0]), (int)toint(args[1])) : -1);
	case NAT_SYS: {
#ifndef XAL_SH_PATH
#define XAL_SH_PATH "/bin/msh"
#endif
#ifndef XAL_SH_PATH2
#define XAL_SH_PATH2 "/bin/sh"
#endif
		char *c = S(0);
		char *sh[] = { (char *)XAL_SH_PATH, (char *)"-c", c, 0 };
		int rc = xexists(XAL_SH_PATH) ? xrun(sh, 0) : -1;
		if (rc < 0) {
			char *sh2[] = { (char *)XAL_SH_PATH2, (char *)"-c", c, 0 };
			rc = xrun(sh2, 0);
		}
		xfree(c);
		return vint(rc);
	}

	case NAT_GETENV: {
		char *k = S(0);
		const char *v = xgetenv(k);
		xfree(k);
		return vstr_copy(v ? v : "");
	}
	case NAT_SETENV: {
		char *k = S(0), *v = S(1);
		int rc = xsetenv(k, v);
		xfree(k);
		xfree(v);
		return vint(rc);
	}
	case NAT_CHDIR: {
		char *p = SD(0, ".");
		int rc = xchdir(p);
		xfree(p);
		return vint(rc);
	}
	case NAT_GETCWD: {
		char b[4096];
		return vstr_copy(xgetcwd(b, sizeof b) >= 0 ? b : "");
	}

	case NAT_EXISTS:
	case NAT_ISDIR:
	case NAT_ISEXEC:
	case NAT_MTIME:
	case NAT_MKDIRP:
	case NAT_UNLINK: {
		char *p = S(0);
		long r = id == NAT_EXISTS ? xexists(p) : id == NAT_ISDIR ? xisdir(p) : id == NAT_ISEXEC ? xisexec(p) : id == NAT_MTIME ? xmtime(p) : id == NAT_MKDIRP ? xmkdirp(p, 0755) : xunlink(p);
		xfree(p);
		return vint(r);
	}
	case NAT_RENAME: {
		char *a = S(0), *b = S(1);
		int r = xrename(a, b);
		xfree(a);
		xfree(b);
		return vint(r);
	}
	case NAT_READDIR: {
		char *p = SD(0, ".");
		list *l = list_new();
		xos_dir *d = xos_dir_open(p);
		if (d) {
			const char *e;
			while ((e = xos_dir_next(d)))
				list_push(l, vstr_copy(e));
			xos_dir_close(d);
		}
		xfree(p);
		return vlist(l);
	}
	case NAT_READFILE: {
		char *p = S(0);
		char *d = xslurp(p, 0);
		xfree(p);
		return d ? vstr(d) : vstr_copy("");
	}
	case NAT_WRITEFILE: {
		char *p = S(0), *d = S(1);
		int fd = xopen(p, XOS_O_WRITE | XOS_O_CREATE | XOS_O_TRUNC, 0644);
		long rc = -1;
		if (fd >= 0) {
			rc = (long)xos_write(fd, d, xstrlen(d));
			xclose(fd);
		}
		xfree(p);
		xfree(d);
		return vint(rc);
	}

	case NAT_SPLIT:
		return nat_split(S(0), SD(1, " "));
	case NAT_JOIN: {
		if (!ISLIST(0))
			return vstr_copy("");
		char *sep = SD(1, " ");
		xstr o;
		xstr_init(&o);
		for (usz i = 0; i < args[0].l->len; i++) {
			if (i)
				xstr_add(&o, sep);
			char *s = tostr(args[0].l->it[i]);
			xstr_add(&o, s);
			xfree(s);
		}
		xfree(sep);
		return vstr(xstr_take(&o));
	}
	case NAT_SUBSTR: {
		char *s = S(0);
		long a = I(1, 0), sl = (long)xstrlen(s);
		long ln = n > 2 ? toint(args[2]) : sl - a;
		if (a < 0)
			a = 0;
		if (a > sl)
			a = sl;
		if (ln < 0 || a + ln > sl)
			ln = sl - a;
		char *r = xstrndup(s + a, (usz)ln);
		xfree(s);
		return vstr(r);
	}
	case NAT_FIND: {
		char *h = S(0), *nd = S(1);
		char *q = xstrstr(h, nd);
		long r = q ? (long)(q - h) : -1;
		xfree(h);
		xfree(nd);
		return vint(r);
	}
	case NAT_EQ: {
		char *a = S(0), *b = S(1);
		int r = !xstrcmp(a, b);
		xfree(a);
		xfree(b);
		return vint(r);
	}
	case NAT_ORD: {
		char *s = S(0);
		long i = I(1, 0);
		long r = (i >= 0 && i < (long)xstrlen(s)) ? (u8)s[i] : -1;
		xfree(s);
		return vint(r);
	}
	case NAT_CHR: {
		char b[2] = { (char)I(0, 0), 0 };
		return vstr_copy(b);
	}
	case NAT_TRIM: {
		char *s = S(0), *a = s;
		while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r')
			a++;
		usz l = xstrlen(a);
		while (l && (a[l - 1] == ' ' || a[l - 1] == '\t' || a[l - 1] == '\n' || a[l - 1] == '\r'))
			a[--l] = 0;
		char *r = xstrdup(a);
		xfree(s);
		return vstr(r);
	}
	case NAT_UPPER:
	case NAT_LOWER: {
		char *s = S(0);
		for (char *q = s; *q; q++) {
			if (id == NAT_UPPER && *q >= 'a' && *q <= 'z')
				*q = (char)(*q - 32);
			else if (id == NAT_LOWER && *q >= 'A' && *q <= 'Z')
				*q = (char)(*q + 32);
		}
		return vstr(s);
	}
	case NAT_WHICH:
		return nat_which(S(0));
	case NAT_REPLACE:
		return nat_replace(S(0), S(1), S(2));
	case NAT_STARTSWITH:
	case NAT_ENDSWITH: {
		char *a = S(0), *b = S(1);
		int r = id == NAT_STARTSWITH ? xstrprefix(a, b) : xstrsuffix(a, b);
		xfree(a);
		xfree(b);
		return vint(r);
	}
	case NAT_REPEAT: {
		char *a = S(0);
		long cnt = I(1, 0);
		xstr o;
		xstr_init(&o);
		for (long i = 0; i < cnt && i < 1000000; i++)
			xstr_add(&o, a);
		xfree(a);
		return vstr(xstr_take(&o));
	}

	case NAT_ARGV: {
		long i = I(0, 0);
		if (i < 0 || i >= vm_argc)
			return vstr_copy("");
		return vstr_copy(vm_argv[i]);
	}
	case NAT_ARGC:
		return vint(vm_argc);
	case NAT_ABS: {
		long v = I(0, 0);
		return vint(v < 0 ? -v : v);
	}
	case NAT_MIN: {
		long a = I(0, 0), b = I(1, 0);
		return vint(a < b ? a : b);
	}
	case NAT_MAX: {
		long a = I(0, 0), b = I(1, 0);
		return vint(a > b ? a : b);
	}
	case NAT_NOW:
		return vint(xos_time_sec());
	case NAT_SLEEP:
		xos_sleep_ms(I(0, 0));
		return vint(0);

	case NAT_INPUT: {
		xstr b;
		xstr_init(&b);
		char c;
		isz r = 0;
		int got = 0;
		while ((r = xos_con_read(&c, 1)) == 1 && c != '\n') {
			xstr_addc(&b, c);
			got = 1;
		}
		if (r > 0 || got)
			at_eof = 0;
		else
			at_eof = 1;
		return vstr(xstr_take(&b));
	}
	case NAT_EOF:
		return vint(at_eof);
	case NAT_RAWMODE:
		return vint(xos_term_raw((int)I(0, 1)));
	case NAT_GETKEY: {
		unsigned char c;
		if (xos_con_read(&c, 1) != 1)
			return vint(-1);
		return vint((long)c);
	}
	case NAT_TERMSIZE: {
		int w, h;
		xos_term_size(&w, &h);
		list *l = list_new();
		list_push(l, vint(w));
		list_push(l, vint(h));
		return vlist(l);
	}
	case NAT_ISATTY:
		return vint(xos_isatty((int)I(0, 0)));

	case NAT_SORT:
		return ISLIST(0) ? nat_sort(args[0].l) : (n ? args[0] : vint(0));
	case NAT_COPY:
		return ISLIST(0) ? list_copy(args[0].l) : vlist(list_new());
	case NAT_INSERT: {
		if (n < 3 || !ISLIST(0))
			return n ? args[0] : vint(0);
		list *l = args[0].l;
		long at = toint(args[1]);
		if (at < 0)
			at = 0;
		if (at > (long)l->len)
			at = (long)l->len;
		list_push(l, vint(0));
		for (usz i = l->len - 1; i > (usz)at; i--)
			l->it[i] = l->it[i - 1];
		l->it[at] = args[2];
		return args[0];
	}
	case NAT_REMOVE: {
		if (n < 2 || !ISLIST(0))
			return n ? args[0] : vint(0);
		list *l = args[0].l;
		long at = toint(args[1]);
		if (at < 0 || at >= (long)l->len)
			return args[0];
		for (usz i = (usz)at; i + 1 < l->len; i++)
			l->it[i] = l->it[i + 1];
		l->len--;
		return args[0];
	}

	case NAT_SPAWN_THREAD: {
		if (n < 1)
			return vint(-1);
		long fi = find_fn(args[0]);
		if (fi < 0) {
			xerr("xvm: spawn_thread: no such function\n");
			return vint(-1);
		}
		list *al = ISLIST(1) ? args[1].l : 0;
		vm_thread *t = thread_new((u32)fi, al);
		return vint(thread_spawn(t));
	}
	case NAT_JOIN_THREAD:
		return n ? thread_join(toint(args[0])) : vint(0);
	case NAT_THREAD_ID:
		return vint(vm_cur ? vm_cur->id : 0);
	case NAT_THREAD_CPU:
	case NAT_SET_AFFINITY:
		return vint(id == NAT_THREAD_CPU ? 0 : -1);
	case NAT_NPROC:
	case NAT_CPU_COUNT:
		return vint(xos_cpu_count());
	case NAT_CHAN_NEW:
		return vint(chan_new());
	case NAT_CHAN_SEND:
		return vint(n < 2 ? -1 : chan_send(toint(args[0]), args[1]));
	case NAT_CHAN_RECV:
	case NAT_CHAN_TRYRECV: {
		if (n < 1)
			return vint(0);
		int ok;
		return chan_recv(toint(args[0]), id == NAT_CHAN_RECV, &ok);
	}
	case NAT_CHAN_CLOSE:
		return vint(n ? chan_close(toint(args[0])) : -1);
	case NAT_PARALLEL_MAP:
		if (n < 2 || !ISLIST(1))
			return vlist(list_new());
		return nat_parallel_map(args[0], args[1].l);

	case NAT_GC_COLLECT:
		gc_collect();
		return vint(0);
	case NAT_GC_COUNT:
		return vint(gc_count());
	case NAT_GC_STAT: {
		list *l = list_new();
		list_push(l, vint(gc_enabled));
		list_push(l, vint(gc_count()));
		list_push(l, vint(gc_bytes()));
		return vlist(l);
	}

	case NAT_RUN_XB:
		return nat_run_xb(S(0), n > 1 ? args[1] : vint(0));
	case NAT_CAPS: {
		u32 have = xvm_caps();
		list *l = list_new();
		for (usz i = 0; i < sizeof cap_names / sizeof cap_names[0]; i++)
			if (have & cap_names[i].bit)
				list_push(l, vstr_copy(cap_names[i].name));
		return vlist(l);
	}
	case NAT_HAS_CAP: {
		char *nm = S(0);
		u32 bit = xvm_cap_find(nm);
		xfree(nm);
		return vint(bit && (xvm_caps() & bit) ? 1 : 0);
	}
	}
	return vint(0);
}
