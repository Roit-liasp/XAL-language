#ifndef XC_H
#define XC_H

#include "xos.h"

void *xmalloc(usz n);
void *xrealloc(void *p, usz n);
void *xcalloc(usz n, usz s);
void xfree(void *p);

usz xstrlen(const char *s);
int xstrcmp(const char *a, const char *b);
int xstrncmp(const char *a, const char *b, usz n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, usz n);
char *xstrchr(const char *s, int c);
char *xstrrchr(const char *s, int c);
char *xstrstr(const char *h, const char *n);
usz xstrlcpy(char *d, const char *s, usz n);
int xstrprefix(const char *s, const char *p);
int xstrsuffix(const char *s, const char *p);

long xatol(const char *s);
int xltoa(long v, char *buf, usz n);

typedef struct {
	char *p;
	usz len, cap;
} xstr;

void xstr_init(xstr *s);
void xstr_free(xstr *s);
void xstr_clear(xstr *s);
void xstr_addc(xstr *s, char c);
void xstr_add(xstr *s, const char *t);
void xstr_addn(xstr *s, const char *t, usz n);
void xstr_addl(xstr *s, long v);
char *xstr_take(xstr *s);

typedef struct {
	void **it;
	usz len, cap;
} xvec;

void xvec_init(xvec *v);
void xvec_free(xvec *v);
void xvec_push(xvec *v, void *p);
void *xvec_pop(xvec *v);
void xvec_sort(xvec *v, int (*cmp)(const void *, const void *));

int xopen(const char *path, int flags, int mode);
isz xread(int fd, void *buf, usz n);
isz xwrite(int fd, const void *buf, usz n);
int xclose(int fd);
char *xslurp(const char *path, usz *len);

void xout(const char *s);
void xerr(const char *s);
void xfmt(int fd, const char *fmt, ...); // %s %d %l %c %x %e %%
void xdie(const char *fmt, ...);

int xexists(const char *path);
int xisdir(const char *path);
int xisexec(const char *path);
long xmtime(const char *path);
long xfilesize(const char *path);
int xmkdirp(const char *path, int mode);
int xunlink(const char *path);
int xrename(const char *a, const char *b);
int xchdir(const char *path);
int xgetcwd(char *buf, usz n);

char **xenv_clone(char **env);
char *xgetenv(const char *k);
int xsetenv(const char *k, const char *v);

long xspawn(char **argv, char **env);
int xwait(long pid);
int xrun(char **argv, char **env);

#endif
