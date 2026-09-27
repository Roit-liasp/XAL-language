/*
 xc.c - portable utility core implementation.
*/

#include "xc.h"

#include <stdarg.h>

static void *xcp(void *d, const void *s, usz n) {
	u8 *dd = (u8 *)d;
	const u8 *ss = (const u8 *)s;
	for (usz i = 0; i < n; i++)
		dd[i] = ss[i];
	return d;
}

static void *xset(void *d, int c, usz n) {
	u8 *dd = (u8 *)d;
	for (usz i = 0; i < n; i++)
		dd[i] = (u8)c;
	return d;
}

void *xmalloc(usz n) {
	void *p = xos_alloc(n ? n : 1);
	if (!p)
		xdie("xc: out of memory\n");
	return p;
}

void *xrealloc(void *p, usz n) {
	void *q = xos_realloc(p, n ? n : 1);
	if (!q)
		xdie("xc: out of memory\n");
	return q;
}

void *xcalloc(usz n, usz s) {
	n = n ? n : 1;
	s = s ? s : 1;
	void *p = xmalloc(n * s);
	xset(p, 0, n * s);
	return p;
}

void xfree(void *p) {
	xos_free(p);
}

usz xstrlen(const char *s) {
	const char *p = s;
	while (*p)
		p++;
	return (usz)(p - s);
}

int xstrcmp(const char *a, const char *b) {
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (int)(u8)*a - (int)(u8)*b;
}

int xstrncmp(const char *a, const char *b, usz n) {
	while (n && *a && *a == *b) {
		a++;
		b++;
		n--;
	}
	if (!n)
		return 0;
	return (int)(u8)*a - (int)(u8)*b;
}

char *xstrdup(const char *s) {
	usz n = xstrlen(s);
	char *d = xmalloc(n + 1);
	xcp(d, s, n + 1);
	return d;
}

char *xstrndup(const char *s, usz n) {
	char *d = xmalloc(n + 1);
	xcp(d, s, n);
	d[n] = 0;
	return d;
}

char *xstrchr(const char *s, int c) {
	for (; *s; s++)
		if (*s == (char)c)
			return (char *)s;
	return c ? 0 : (char *)s;
}

char *xstrrchr(const char *s, int c) {
	const char *r = 0;
	for (; *s; s++)
		if (*s == (char)c)
			r = s;
	return (char *)r;
}

char *xstrstr(const char *h, const char *n) {
	usz ln = xstrlen(n);
	if (!ln)
		return (char *)h;
	for (; *h; h++)
		if (!xstrncmp(h, n, ln))
			return (char *)h;
	return 0;
}

usz xstrlcpy(char *d, const char *s, usz n) {
	usz l = xstrlen(s);
	if (n) {
		usz c = l < n - 1 ? l : n - 1;
		xcp(d, s, c);
		d[c] = 0;
	}
	return l;
}

int xstrprefix(const char *s, const char *p) {
	return !xstrncmp(s, p, xstrlen(p));
}

int xstrsuffix(const char *s, const char *p) {
	usz ls = xstrlen(s), lp = xstrlen(p);
	return ls >= lp && !xstrcmp(s + ls - lp, p);
}

long xatol(const char *s) {
	long v = 0;
	int neg = 0;
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s == '-') {
		neg = 1;
		s++;
	} else if (*s == '+')
		s++;
	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');
	return neg ? -v : v;
}

int xltoa(long v, char *buf, usz n) {
	char tmp[32];
	int i = 0, j = 0, neg = v < 0;
	unsigned long u = neg ? (unsigned long)(-(v + 1)) + 1 : (unsigned long)v;
	do {
		tmp[i++] = (char)('0' + (int)(u % 10));
		u /= 10;
	} while (u);
	if (neg)
		tmp[i++] = '-';
	if ((usz)i >= n)
		return -1;
	while (i)
		buf[j++] = tmp[--i];
	buf[j] = 0;
	return j;
}

void xstr_init(xstr *s) {
	s->p = xmalloc(32);
	s->p[0] = 0;
	s->len = 0;
	s->cap = 32;
}

void xstr_free(xstr *s) {
	xfree(s->p);
	s->p = 0;
	s->len = s->cap = 0;
}

void xstr_clear(xstr *s) {
	s->len = 0;
	if (s->p)
		s->p[0] = 0;
}

static void xstr_grow(xstr *s, usz need) {
	if (s->len + need + 1 <= s->cap)
		return;
	while (s->cap < s->len + need + 1)
		s->cap *= 2;
	s->p = xrealloc(s->p, s->cap);
}

void xstr_addc(xstr *s, char c) {
	xstr_grow(s, 1);
	s->p[s->len++] = c;
	s->p[s->len] = 0;
}

void xstr_addn(xstr *s, const char *t, usz n) {
	xstr_grow(s, n);
	xcp(s->p + s->len, t, n);
	s->len += n;
	s->p[s->len] = 0;
}

void xstr_add(xstr *s, const char *t) {
	xstr_addn(s, t, xstrlen(t));
}

void xstr_addl(xstr *s, long v) {
	char b[32];
	xltoa(v, b, sizeof b);
	xstr_add(s, b);
}

char *xstr_take(xstr *s) {
	char *p = s->p;
	s->p = 0;
	s->len = s->cap = 0;
	return p;
}

void xvec_init(xvec *v) {
	v->it = xmalloc(8 * sizeof(void *));
	v->len = 0;
	v->cap = 8;
}

void xvec_free(xvec *v) {
	xfree(v->it);
	v->it = 0;
	v->len = v->cap = 0;
}

void xvec_push(xvec *v, void *p) {
	if (v->len == v->cap) {
		v->cap *= 2;
		v->it = xrealloc(v->it, v->cap * sizeof(void *));
	}
	v->it[v->len++] = p;
}

void *xvec_pop(xvec *v) {
	return v->len ? v->it[--v->len] : 0;
}

void xvec_sort(xvec *v, int (*cmp)(const void *, const void *)) {
	for (usz i = 1; i < v->len; i++) {
		void *p = v->it[i];
		usz j = i;
		while (j > 0 && cmp(&v->it[j - 1], &p) > 0) {
			v->it[j] = v->it[j - 1];
			j--;
		}
		v->it[j] = p;
	}
}

int xopen(const char *path, int flags, int mode) {
	return xos_open(path, flags, mode);
}

isz xread(int fd, void *buf, usz n) {
	return xos_read(fd, buf, n);
}

isz xwrite(int fd, const void *buf, usz n) {
	return xos_con_write(fd, buf, n);
}

int xclose(int fd) {
	return xos_close(fd);
}

char *xslurp(const char *path, usz *len) {
	int fd = xos_open(path, XOS_O_READ, 0);
	if (fd < 0)
		return 0;
	usz cap = 4096, n = 0;
	char *b = xmalloc(cap);
	for (;;) {
		if (n == cap) {
			cap *= 2;
			b = xrealloc(b, cap);
		}
		isz r = xos_read(fd, b + n, cap - n);
		if (r < 0) {
			xfree(b);
			xos_close(fd);
			return 0;
		}
		if (!r)
			break;
		n += (usz)r;
	}
	xos_close(fd);
	b = xrealloc(b, n + 1);
	b[n] = 0;
	if (len)
		*len = n;
	return b;
}

void xout(const char *s) {
	xos_con_write(1, s, xstrlen(s));
}

void xerr(const char *s) {
	xos_con_write(2, s, xstrlen(s));
}

static void vfmt(int fd, const char *fmt, va_list ap) {
	xstr o;
	xstr_init(&o);
	for (const char *p = fmt; *p; p++) {
		if (*p != '%') {
			xstr_addc(&o, *p);
			continue;
		}
		p++;
		switch (*p) {
		case 's': {
			const char *s = va_arg(ap, const char *);
			xstr_add(&o, s ? s : "(null)");
			break;
		}
		case 'd':
			xstr_addl(&o, (long)va_arg(ap, int));
			break;
		case 'l':
			xstr_addl(&o, va_arg(ap, long));
			break;
		case 'c':
			xstr_addc(&o, (char)va_arg(ap, int));
			break;
		case 'x': {
			unsigned long v = va_arg(ap, unsigned long);
			char b[17];
			int i = 0;
			do {
				int d = (int)(v & 15);
				b[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
				v >>= 4;
			} while (v);
			while (i)
				xstr_addc(&o, b[--i]);
			break;
		}
		case 'e':
			xstr_add(&o, xos_errstr(xos_errno()));
			break;
		case '%':
			xstr_addc(&o, '%');
			break;
		default:
			xstr_addc(&o, '%');
			if (*p)
				xstr_addc(&o, *p);
		}
		if (!*p)
			break;
	}
	xos_con_write(fd, o.p, o.len);
	xstr_free(&o);
}

void xfmt(int fd, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfmt(fd, fmt, ap);
	va_end(ap);
}

void xdie(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfmt(2, fmt, ap);
	va_end(ap);
	xos_exit(1);
}

int xexists(const char *path) {
	xos_finfo fi;
	return xos_stat(path, &fi) == 0;
}

int xisdir(const char *path) {
	xos_finfo fi;
	return xos_stat(path, &fi) == 0 && fi.isdir;
}

int xisexec(const char *path) {
	xos_finfo fi;
	return xos_stat(path, &fi) == 0 && fi.isreg && fi.isexec;
}

long xmtime(const char *path) {
	xos_finfo fi;
	if (xos_stat(path, &fi) < 0)
		return -1;
	return fi.mtime;
}

long xfilesize(const char *path) {
	xos_finfo fi;
	if (xos_stat(path, &fi) < 0)
		return -1;
	return fi.size;
}

int xmkdirp(const char *path, int mode) {
	char buf[4096];
	xstrlcpy(buf, path, sizeof buf);
	for (char *p = buf + 1; *p; p++) {
		if (*p != '/')
			continue;
		*p = 0;
		if (xos_mkdir(buf, mode) < 0)
			return -1;
		*p = '/';
	}
	if (xos_mkdir(buf, mode) < 0)
		return -1;
	return 0;
}

int xunlink(const char *path) {
	return xos_unlink(path);
}

int xrename(const char *a, const char *b) {
	return xos_rename(a, b);
}

int xchdir(const char *path) {
	return xos_chdir(path);
}

int xgetcwd(char *buf, usz n) {
	return xos_getcwd(buf, n);
}

char **xenv_clone(char **env) {
	if (!env)
		env = xos_env_all();
	if (!env) {
		char **c = xmalloc(sizeof(char *));
		c[0] = 0;
		return c;
	}
	usz n = 0;
	while (env[n])
		n++;
	char **c = xmalloc((n + 1) * sizeof(char *));
	for (usz i = 0; i < n; i++)
		c[i] = xstrdup(env[i]);
	c[n] = 0;
	return c;
}

char *xgetenv(const char *k) {
	return (char *)xos_env_get(k);
}

int xsetenv(const char *k, const char *v) {
	return xos_env_set(k, v);
}

long xspawn(char **argv, char **env) {
	return xos_proc_spawn(argv, env ? env : xos_env_all());
}

int xwait(long pid) {
	return xos_proc_wait(pid);
}

int xrun(char **argv, char **env) {
	long pid = xspawn(argv, env);
	if (pid < 0)
		return -1;
	return xwait(pid);
}
