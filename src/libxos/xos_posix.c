/*
 xos_posix.c - XOS backend for POSIX systems (Linux, BSD, macOS).
*/

#define _GNU_SOURCE
#include "xos.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#if XOS_HAS_TERM
#include <termios.h>
#include <sys/ioctl.h>
#endif
#if XOS_HAS_THREADS
#include <pthread.h>
#endif

extern char **environ;

int xos_errno(void) {
	return errno;
}

const char *xos_errstr(int e) {
	const char *s = strerror(e);
	return s ? s : "unknown error";
}

void xos_exit(int code) {
	_exit(code);
	for (;;) {
	}
}

void *xos_alloc(usz n) {
	return malloc(n ? n : 1);
}

void *xos_realloc(void *p, usz n) {
	return realloc(p, n ? n : 1);
}

void xos_free(void *p) {
	free(p);
}

isz xos_con_write(int fd, const void *buf, usz n) {
	const char *p = (const char *)buf;
	usz done = 0;
	while (done < n) {
		ssize_t r = write(fd, p + done, n - done);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		done += (usz)r;
	}
	return (isz)done;
}

isz xos_con_read(void *buf, usz n) {
	ssize_t r;
	do
		r = read(0, buf, n);
	while (r < 0 && errno == EINTR);
	return (isz)r;
}

static int mapflags(int flags) {
	int f = 0;
	int rd = flags & XOS_O_READ, wr = flags & XOS_O_WRITE;
	if (rd && wr)
		f = O_RDWR;
	else if (wr)
		f = O_WRONLY;
	else
		f = O_RDONLY;
	if (flags & XOS_O_CREATE)
		f |= O_CREAT;
	if (flags & XOS_O_TRUNC)
		f |= O_TRUNC;
	if (flags & XOS_O_APPEND)
		f |= O_APPEND;
	return f;
}

int xos_open(const char *path, int flags, int mode) {
	int fd;
	do
		fd = open(path, mapflags(flags), (mode_t)mode);
	while (fd < 0 && errno == EINTR);
	return fd;
}

int xos_close(int fd) {
	return close(fd);
}

isz xos_read(int fd, void *buf, usz n) {
	ssize_t r;
	do
		r = read(fd, buf, n);
	while (r < 0 && errno == EINTR);
	return (isz)r;
}

isz xos_write(int fd, const void *buf, usz n) {
	const char *p = (const char *)buf;
	usz done = 0;
	while (done < n) {
		ssize_t r = write(fd, p + done, n - done);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		done += (usz)r;
	}
	return (isz)done;
}

int xos_stat(const char *path, xos_finfo *fi) {
	struct stat st;
	memset(fi, 0, sizeof *fi);
	if (stat(path, &st) < 0)
		return -1;
	fi->exists = 1;
	fi->isdir = S_ISDIR(st.st_mode) ? 1 : 0;
	fi->isreg = S_ISREG(st.st_mode) ? 1 : 0;
	fi->isexec = (S_ISREG(st.st_mode) && access(path, X_OK) == 0) ? 1 : 0;
	fi->size = (long)st.st_size;
	fi->mtime = (long)st.st_mtime;
	return 0;
}

int xos_mkdir(const char *path, int mode) {
	if (mkdir(path, (mode_t)mode) == 0)
		return 0;
	if (errno == EEXIST) {
		xos_finfo fi;
		if (xos_stat(path, &fi) == 0 && fi.isdir)
			return 0;
	}
	return -1;
}

int xos_unlink(const char *path) {
	return unlink(path);
}

int xos_rename(const char *a, const char *b) {
	return rename(a, b);
}

int xos_chdir(const char *path) {
	return chdir(path);
}

int xos_getcwd(char *buf, usz n) {
	if (!getcwd(buf, (size_t)n))
		return -1;
	return (int)strlen(buf);
}

struct xos_dir {
	DIR *d;
	char name[1024];
};

xos_dir *xos_dir_open(const char *path) {
	DIR *d = opendir(path);
	if (!d)
		return 0;
	xos_dir *xd = (xos_dir *)xos_alloc(sizeof *xd);
	if (!xd) {
		closedir(d);
		return 0;
	}
	xd->d = d;
	xd->name[0] = 0;
	return xd;
}

const char *xos_dir_next(xos_dir *xd) {
	if (!xd)
		return 0;
	struct dirent *e;
	for (;;) {
		e = readdir(xd->d);
		if (!e)
			return 0;
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strncpy(xd->name, e->d_name, sizeof xd->name - 1);
		xd->name[sizeof xd->name - 1] = 0;
		return xd->name;
	}
}

void xos_dir_close(xos_dir *xd) {
	if (!xd)
		return;
	closedir(xd->d);
	xos_free(xd);
}

const char *xos_env_get(const char *key) {
	return getenv(key);
}

int xos_env_set(const char *key, const char *val) {
	return setenv(key, val, 1);
}

char **xos_env_all(void) {
	return environ;
}

long xos_time_sec(void) {
	return (long)time(0);
}

long xos_time_ms(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return (long)time(0) * 1000;
	return (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void xos_sleep_ms(long ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	while (nanosleep(&ts, &ts) < 0 && errno == EINTR)
		;
}

long xos_proc_spawn(char *const argv[], char *const env[]) {
	if (!argv || !argv[0])
		return -1;
	pid_t pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		execve(argv[0], argv, env ? env : environ);
		_exit(127);
	}
	return (long)pid;
}

int xos_proc_wait(long pid) {
	int st = 0;
	while (waitpid((pid_t)pid, &st, 0) < 0)
		if (errno != EINTR)
			return -1;
	if (WIFEXITED(st))
		return WEXITSTATUS(st);
	if (WIFSIGNALED(st))
		return 128 + WTERMSIG(st);
	return -1;
}

int xos_proc_exec(char *const argv[], char *const env[]) {
	if (!argv || !argv[0])
		return -1;
	execve(argv[0], argv, env ? env : environ);
	return -1;
}

long xos_proc_getpid(void) {
	return (long)getpid();
}

int xos_proc_kill(long pid, int sig) {
	if (pid <= 0)
		return -1;
	return kill((pid_t)pid, sig ? sig : SIGTERM) == 0 ? 0 : -1;
}

static int decode_status(int st) {
	if (WIFEXITED(st))
		return WEXITSTATUS(st);
	if (WIFSIGNALED(st))
		return 128 + WTERMSIG(st);
	return -2;
}

int xos_proc_trywait(long pid) {
	if (pid <= 0)
		return -1;
	int st = 0;
	pid_t r = waitpid((pid_t)pid, &st, WNOHANG);
	if (r == 0)
		return -2;
	if (r < 0)
		return -1;
	return decode_status(st);
}

long xos_proc_fork(void) {
	pid_t pid = fork();
	if (pid < 0)
		return -1;
	return (long)pid;
}

int xos_pipe(int fds[2]) {
	int p[2];
	if (pipe(p) != 0)
		return -1;
	fds[0] = p[0];
	fds[1] = p[1];
	return 0;
}

int xos_dup2(int oldfd, int newfd) {
	return dup2(oldfd, newfd) < 0 ? -1 : 0;
}

int xos_signal(int sig, int mode) {
	void (*h)(int) = mode == XOS_SIG_IGN ? SIG_IGN : SIG_DFL;
	return signal(sig, h) == SIG_ERR ? -1 : 0;
}

#if XOS_HAS_TERM
static struct termios tio_saved;
static int tio_have_saved;

int xos_isatty(int fd) {
	return isatty(fd) ? 1 : 0;
}

int xos_term_raw(int on) {
	if (on) {
		struct termios t;
		if (tcgetattr(0, &t) < 0)
			return -1;
		if (!tio_have_saved) {
			tio_saved = t;
			tio_have_saved = 1;
		}
		t.c_lflag &= ~(tcflag_t)(ECHO | ICANON | ISIG | IEXTEN);
		t.c_iflag &= ~(tcflag_t)(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
		t.c_oflag &= ~(tcflag_t)OPOST;
		t.c_cc[VMIN] = 1;
		t.c_cc[VTIME] = 0;
		return tcsetattr(0, TCSAFLUSH, &t) == 0 ? 0 : -1;
	}
	if (tio_have_saved)
		return tcsetattr(0, TCSAFLUSH, &tio_saved) == 0 ? 0 : -1;
	return 0;
}

int xos_term_size(int *w, int *h) {
	struct winsize ws;
	if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col) {
		if (w)
			*w = ws.ws_col;
		if (h)
			*h = ws.ws_row;
		return 0;
	}
	if (w)
		*w = 80;
	if (h)
		*h = 24;
	return -1;
}
#else
int xos_isatty(int fd) {
	(void)fd;
	return 0;
}
int xos_term_raw(int on) {
	(void)on;
	return -1;
}
int xos_term_size(int *w, int *h) {
	if (w)
		*w = 80;
	if (h)
		*h = 24;
	return -1;
}
#endif

#if XOS_HAS_THREADS
struct xos_mtx {
	pthread_mutex_t m;
};
struct xos_cv {
	pthread_cond_t c;
};
struct xos_thr {
	pthread_t t;
};

xos_mtx *xos_mtx_new(void) {
	xos_mtx *m = (xos_mtx *)xos_alloc(sizeof *m);
	if (!m)
		xos_exit(1);
	pthread_mutex_init(&m->m, 0);
	return m;
}

void xos_mtx_lock(xos_mtx *m) {
	pthread_mutex_lock(&m->m);
}

void xos_mtx_unlock(xos_mtx *m) {
	pthread_mutex_unlock(&m->m);
}

void xos_mtx_free(xos_mtx *m) {
	pthread_mutex_destroy(&m->m);
	xos_free(m);
}

xos_cv *xos_cv_new(void) {
	xos_cv *c = (xos_cv *)xos_alloc(sizeof *c);
	if (!c)
		xos_exit(1);
	pthread_cond_init(&c->c, 0);
	return c;
}

void xos_cv_wait(xos_cv *c, xos_mtx *m) {
	pthread_cond_wait(&c->c, &m->m);
}

void xos_cv_signal(xos_cv *c) {
	pthread_cond_signal(&c->c);
}

void xos_cv_broadcast(xos_cv *c) {
	pthread_cond_broadcast(&c->c);
}

void xos_cv_free(xos_cv *c) {
	pthread_cond_destroy(&c->c);
	xos_free(c);
}

typedef struct {
	void (*fn)(void *);
	void *arg;
} thr_start;

static void *thr_trampoline(void *p) {
	thr_start *s = (thr_start *)p;
	void (*fn)(void *) = s->fn;
	void *arg = s->arg;
	xos_free(s);
	fn(arg);
	return 0;
}

xos_thr *xos_thread_create(void (*fn)(void *), void *arg) {
	xos_thr *t = (xos_thr *)xos_alloc(sizeof *t);
	thr_start *s = (thr_start *)xos_alloc(sizeof *s);
	if (!t || !s) {
		xos_free(t);
		xos_free(s);
		return 0;
	}
	s->fn = fn;
	s->arg = arg;
	if (pthread_create(&t->t, 0, thr_trampoline, s) != 0) {
		xos_free(t);
		xos_free(s);
		return 0;
	}
	return t;
}

int xos_thread_join(xos_thr *t) {
	if (!t)
		return -1;
	int rc = pthread_join(t->t, 0);
	xos_free(t);
	return rc == 0 ? 0 : -1;
}

int xos_cpu_count(void) {
	long n = sysconf(_SC_NPROCESSORS_ONLN);
	return n > 0 ? (int)n : 1;
}
#else
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
#endif
