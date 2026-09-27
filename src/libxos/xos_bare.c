/*
 xos_bare.c - freestanding backend:
 custom kernels, bootloaders, microcontrollers, BIOS/UEFI apps...

 Assumes nothing except a C compiler:
   - no libc
   - no syscalls
   - no filesystem
   - no multitasking
   - no privilege model

 Yes, I understand that porting will be difficult for some due to the requirements, but sorry perhaps I’ll find a way to simplify it in the future.
*/

#include "xos.h"

#if defined(__GNUC__)
#define XOS_WEAK __attribute__((weak))
#else
#define XOS_WEAK
#endif

XOS_WEAK void xos_bare_out(const char *buf, usz n) {
	(void)buf;
	(void)n;
}
XOS_WEAK void xos_bare_err(const char *buf, usz n) {
	(void)buf;
	(void)n;
}
XOS_WEAK int xos_bare_in(void) {
	return -1;
}
XOS_WEAK long xos_bare_time_ms(void) {
	static long tick;
	return ++tick;
}

void *memcpy(void *d, const void *s, usz n) {
	u8 *dd = (u8 *)d;
	const u8 *ss = (const u8 *)s;
	for (usz i = 0; i < n; i++)
		dd[i] = ss[i];
	return d;
}

void *memmove(void *d, const void *s, usz n) {
	u8 *dd = (u8 *)d;
	const u8 *ss = (const u8 *)s;
	if (dd < ss) {
		for (usz i = 0; i < n; i++)
			dd[i] = ss[i];
	} else {
		while (n--)
			dd[n] = ss[n];
	}
	return d;
}

void *memset(void *d, int c, usz n) {
	u8 *dd = (u8 *)d;
	for (usz i = 0; i < n; i++)
		dd[i] = (u8)c;
	return d;
}

#ifndef XOS_BARE_HEAP
#define XOS_BARE_HEAP (8u * 1024u * 1024u)
#endif

typedef struct blk {
	usz size;
	int free;
	struct blk *next;
} blk;

static u8 arena[XOS_BARE_HEAP] __attribute__((aligned(16)));
static blk *heap_head;
static int heap_ready;

#define BLK_HDR ((usz)sizeof(blk))
#define ALIGN16(x) (((x) + 15u) & ~(usz)15u)

static void heap_init(void) {
	heap_head = (blk *)arena;
	heap_head->size = XOS_BARE_HEAP - BLK_HDR;
	heap_head->free = 1;
	heap_head->next = 0;
	heap_ready = 1;
}

static u8 *blk_pay(blk *b) {
	return (u8 *)b + BLK_HDR;
}

void *xos_alloc(usz n) {
	if (!heap_ready)
		heap_init();
	n = ALIGN16(n ? n : 1);
	for (blk *b = heap_head; b; b = b->next) {
		if (!b->free || b->size < n)
			continue;
		if (b->size >= n + BLK_HDR + 16) {
			blk *rest = (blk *)(blk_pay(b) + n);
			rest->size = b->size - n - BLK_HDR;
			rest->free = 1;
			rest->next = b->next;
			b->size = n;
			b->next = rest;
		}
		b->free = 0;
		return blk_pay(b);
	}
	return 0;
}

void xos_free(void *p) {
	if (!p)
		return;
	blk *b = (blk *)((u8 *)p - BLK_HDR);
	b->free = 1;
	while (b->next && b->next->free) {
		b->size += BLK_HDR + b->next->size;
		b->next = b->next->next;
	}
	for (blk *q = heap_head; q && q->next; q = q->next) {
		if (q->free && q->next == b) {
			q->size += BLK_HDR + b->size;
			q->next = b->next;
			break;
		}
	}
}

void *xos_realloc(void *p, usz n) {
	if (!p)
		return xos_alloc(n);
	if (!n) {
		xos_free(p);
		return 0;
	}
	blk *b = (blk *)((u8 *)p - BLK_HDR);
	n = ALIGN16(n);
	if (b->next && b->next->free && b->size + BLK_HDR + b->next->size >= n) {
		b->size += BLK_HDR + b->next->size;
		b->next = b->next->next;
		return p;
	}
	if (b->size >= n)
		return p;
	void *q = xos_alloc(n);
	if (!q)
		return 0;
	memcpy(q, p, b->size < n ? b->size : n);
	xos_free(p);
	return q;
}

static int bare_err;

int xos_errno(void) {
	return bare_err;
}

const char *xos_errstr(int e) {
	switch (e) {
	case 0:
		return "ok";
	case 1:
		return "not supported on this platform";
	case 2:
		return "out of memory";
	case 3:
		return "not found";
	default:
		return "unknown error";
	}
}

void xos_exit(int code) {
	const char msg[] = "xos_bare: exit\n";
	xos_bare_err(msg, sizeof msg - 1);
	(void)code;
	for (;;) {
	}
}

isz xos_con_write(int fd, const void *buf, usz n) {
	if (fd == 2)
		xos_bare_err((const char *)buf, n);
	else
		xos_bare_out((const char *)buf, n);
	return (isz)n;
}

isz xos_con_read(void *buf, usz n) {
	u8 *p = (u8 *)buf;
	usz got = 0;
	while (got < n) {
		int c = xos_bare_in();
		if (c < 0)
			break;
		p[got++] = (u8)c;
		if (n > 1 && c == '\n')
			break;
	}
	return got ? (isz)got : -1;
}

int xos_open(const char *path, int flags, int mode) {
	(void)path;
	(void)flags;
	(void)mode;
	bare_err = 1;
	return -1;
}

int xos_close(int fd) {
	(void)fd;
	bare_err = 1;
	return -1;
}

isz xos_read(int fd, void *buf, usz n) {
	(void)fd;
	(void)buf;
	(void)n;
	bare_err = 1;
	return -1;
}

isz xos_write(int fd, const void *buf, usz n) {
	if (fd >= 0 && fd <= 2)
		return xos_con_write(fd, buf, n);
	bare_err = 1;
	return -1;
}

int xos_stat(const char *path, xos_finfo *fi) {
	(void)path;
	if (fi) {
		fi->exists = fi->isdir = fi->isreg = fi->isexec = 0;
		fi->size = fi->mtime = 0;
	}
	bare_err = 3;
	return -1;
}

int xos_mkdir(const char *path, int mode) {
	(void)path;
	(void)mode;
	bare_err = 1;
	return -1;
}

int xos_unlink(const char *path) {
	(void)path;
	bare_err = 1;
	return -1;
}

int xos_rename(const char *a, const char *b) {
	(void)a;
	(void)b;
	bare_err = 1;
	return -1;
}

int xos_chdir(const char *path) {
	(void)path;
	bare_err = 1;
	return -1;
}

int xos_getcwd(char *buf, usz n) {
	if (buf && n) {
		buf[0] = '/';
		if (n > 1)
			buf[1] = 0;
		return 1;
	}
	return -1;
}

xos_dir *xos_dir_open(const char *path) {
	(void)path;
	bare_err = 1;
	return 0;
}

const char *xos_dir_next(xos_dir *d) {
	(void)d;
	return 0;
}

void xos_dir_close(xos_dir *d) {
	(void)d;
}

const char *xos_env_get(const char *key) {
	(void)key;
	return 0;
}

int xos_env_set(const char *key, const char *val) {
	(void)key;
	(void)val;
	bare_err = 1;
	return -1;
}

char **xos_env_all(void) {
	return 0;
}

long xos_time_sec(void) {
	return xos_bare_time_ms() / 1000;
}

long xos_time_ms(void) {
	return xos_bare_time_ms();
}

void xos_sleep_ms(long ms) {
	long t0 = xos_bare_time_ms();
	while (xos_bare_time_ms() - t0 < ms)
		;
}

long xos_proc_spawn(char *const argv[], char *const env[]) {
	(void)argv;
	(void)env;
	bare_err = 1;
	return -1;
}

int xos_proc_wait(long pid) {
	(void)pid;
	bare_err = 1;
	return -1;
}

int xos_proc_exec(char *const argv[], char *const env[]) {
	(void)argv;
	(void)env;
	bare_err = 1;
	return -1;
}

long xos_proc_getpid(void) {
	return -1;
}

int xos_proc_kill(long pid, int sig) {
	(void)pid;
	(void)sig;
	bare_err = 1;
	return -1;
}

int xos_proc_trywait(long pid) {
	(void)pid;
	bare_err = 1;
	return -1;
}

long xos_proc_fork(void) {
	bare_err = 1;
	return -1;
}

int xos_pipe(int fds[2]) {
	(void)fds;
	bare_err = 1;
	return -1;
}

int xos_dup2(int oldfd, int newfd) {
	(void)oldfd;
	(void)newfd;
	bare_err = 1;
	return -1;
}

int xos_signal(int sig, int mode) {
	(void)sig;
	(void)mode;
	bare_err = 1;
	return -1;
}

int xos_isatty(int fd) {
	return fd >= 0 && fd <= 2 ? 1 : 0;
}

int xos_term_raw(int on) {
	(void)on;
	bare_err = 1;
	return -1;
}

int xos_term_size(int *w, int *h) {
	if (w)
		*w = 80;
	if (h)
		*h = 24;
	return -1;
}

struct xos_mtx {
	int dummy;
};
struct xos_cv {
	int dummy;
};
struct xos_thr {
	int dummy;
};

xos_mtx *xos_mtx_new(void) {
	xos_mtx *m = (xos_mtx *)xos_alloc(sizeof *m);
	if (!m)
		xos_exit(1);
	m->dummy = 0;
	return m;
}

void xos_mtx_lock(xos_mtx *m) {
	(void)m;
}

void xos_mtx_unlock(xos_mtx *m) {
	(void)m;
}

void xos_mtx_free(xos_mtx *m) {
	xos_free(m);
}

xos_cv *xos_cv_new(void) {
	xos_cv *c = (xos_cv *)xos_alloc(sizeof *c);
	if (!c)
		xos_exit(1);
	c->dummy = 0;
	return c;
}

void xos_cv_wait(xos_cv *c, xos_mtx *m) {
	(void)c;
	(void)m;
}

void xos_cv_signal(xos_cv *c) {
	(void)c;
}

void xos_cv_broadcast(xos_cv *c) {
	(void)c;
}

void xos_cv_free(xos_cv *c) {
	xos_free(c);
}

xos_thr *xos_thread_create(void (*fn)(void *), void *arg) {
	(void)fn;
	(void)arg;
	return 0;
}

int xos_thread_join(xos_thr *t) {
	(void)t;
	return -1;
}

int xos_cpu_count(void) {
	return 1;
}
