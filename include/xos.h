#ifndef XOS_H
#define XOS_H

#include <stddef.h>

#ifndef XOS_HAS_FS
#define XOS_HAS_FS 1
#endif
#ifndef XOS_HAS_DIR
#define XOS_HAS_DIR 1
#endif
#ifndef XOS_HAS_ENV
#define XOS_HAS_ENV 1
#endif
#ifndef XOS_HAS_PROC
#define XOS_HAS_PROC 1
#endif
#ifndef XOS_HAS_TERM
#define XOS_HAS_TERM 1
#endif
#ifndef XOS_HAS_THREADS
#define XOS_HAS_THREADS 1
#endif
#ifndef XOS_HAS_CWD
#define XOS_HAS_CWD 1
#endif

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long usz;
typedef long isz;

int xos_errno(void);
const char *xos_errstr(int e);

void xos_exit(int code);

void *xos_alloc(usz n);
void *xos_realloc(void *p, usz n);
void xos_free(void *p);

isz xos_con_write(int fd, const void *buf, usz n);
isz xos_con_read(void *buf, usz n);

#define XOS_O_READ 0x01
#define XOS_O_WRITE 0x02
#define XOS_O_CREATE 0x04
#define XOS_O_TRUNC 0x08
#define XOS_O_APPEND 0x10

int xos_open(const char *path, int flags, int mode);
int xos_close(int fd);
isz xos_read(int fd, void *buf, usz n);
isz xos_write(int fd, const void *buf, usz n);

typedef struct {
	int exists, isdir, isreg, isexec;
	long size;
	long mtime;
} xos_finfo;

int xos_stat(const char *path, xos_finfo *fi);
int xos_mkdir(const char *path, int mode);
int xos_unlink(const char *path);
int xos_rename(const char *a, const char *b);
int xos_chdir(const char *path);
int xos_getcwd(char *buf, usz n);

typedef struct xos_dir xos_dir;
xos_dir *xos_dir_open(const char *path);
const char *xos_dir_next(xos_dir *d);
void xos_dir_close(xos_dir *d);

const char *xos_env_get(const char *key);
int xos_env_set(const char *key, const char *val);
char **xos_env_all(void);

long xos_time_sec(void);
long xos_time_ms(void);
void xos_sleep_ms(long ms);

long xos_proc_spawn(char *const argv[], char *const env[]);
int xos_proc_wait(long pid);
int xos_proc_exec(char *const argv[], char *const env[]);

long xos_proc_getpid(void);
int  xos_proc_kill(long pid, int sig);
int  xos_proc_trywait(long pid);
long xos_proc_fork(void);
int  xos_pipe(int fds[2]);
int  xos_dup2(int oldfd, int newfd);
#define XOS_SIG_DFL 0
#define XOS_SIG_IGN 1
int  xos_signal(int sig, int mode);

int xos_isatty(int fd);
int xos_term_raw(int on);
int xos_term_size(int *w, int *h);

typedef struct xos_mtx xos_mtx;
typedef struct xos_cv xos_cv;
typedef struct xos_thr xos_thr;

xos_mtx *xos_mtx_new(void);
void xos_mtx_lock(xos_mtx *m);
void xos_mtx_unlock(xos_mtx *m);
void xos_mtx_free(xos_mtx *m);

xos_cv *xos_cv_new(void);
void xos_cv_wait(xos_cv *c, xos_mtx *m);
void xos_cv_signal(xos_cv *c);
void xos_cv_broadcast(xos_cv *c);
void xos_cv_free(xos_cv *c);

xos_thr *xos_thread_create(void (*fn)(void *arg), void *arg);
int xos_thread_join(xos_thr *t);
int xos_cpu_count(void);

#endif
