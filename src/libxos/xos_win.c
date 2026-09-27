/*
 xos_win.c - XOS backend for Windows (Win32 API, no POSIX layer).
 NOTE: written against the Win32 API documentation, not compile-tested
*/

#include "xos.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int xos_errno(void) {
	return (int)GetLastError();
}

const char *xos_errstr(int e) {
	switch ((DWORD)e) {
	case 0:
		return "ok";
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return "not found";
	case ERROR_ACCESS_DENIED:
		return "access denied";
	case ERROR_ALREADY_EXISTS:
		return "already exists";
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return "out of memory";
	case ERROR_HANDLE_EOF:
		return "end of file";
	default:
		return "win32 error";
	}
}

void xos_exit(int code) {
	ExitProcess((UINT)code);
	for (;;) {
	}
}

void *xos_alloc(usz n) {
	return HeapAlloc(GetProcessHeap(), 0, (SIZE_T)(n ? n : 1));
}

void *xos_realloc(void *p, usz n) {
	if (!p)
		return xos_alloc(n);
	if (!n) {
		HeapFree(GetProcessHeap(), 0, p);
		return 0;
	}
	void *q = HeapReAlloc(GetProcessHeap(), 0, p, (SIZE_T)n);
	return q;
}

void xos_free(void *p) {
	if (p)
		HeapFree(GetProcessHeap(), 0, p);
}

static HANDLE con_handle(int fd) {
	DWORD which = fd == 0 ? STD_INPUT_HANDLE : fd == 2 ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
	return GetStdHandle(which);
}

isz xos_con_write(int fd, const void *buf, usz n) {
	DWORD wrote = 0;
	if (!WriteFile(con_handle(fd), buf, (DWORD)n, &wrote, 0))
		return -1;
	return (isz)wrote;
}

isz xos_con_read(void *buf, usz n) {
	DWORD got = 0;
	if (!ReadFile(con_handle(0), buf, (DWORD)n, &got, 0))
		return -1;
	return got ? (isz)got : -1;
}

#define FD_TABLE 256
static HANDLE fd_tab[FD_TABLE];
static CRITICAL_SECTION fd_lock;
static int fd_lock_ready;

static void fd_init(void) {
	if (!fd_lock_ready) {
		InitializeCriticalSection(&fd_lock);
		fd_lock_ready = 1;
	}
}

static int fd_put(HANDLE h) {
	fd_init();
	EnterCriticalSection(&fd_lock);
	int fd = -1;
	for (int i = 0; i < FD_TABLE; i++)
		if (!fd_tab[i]) {
			fd_tab[i] = h;
			fd = i + 3;
			break;
		}
	LeaveCriticalSection(&fd_lock);
	return fd;
}

static HANDLE fd_get(int fd) {
	if (fd >= 0 && fd <= 2)
		return con_handle(fd);
	if (fd < 3 || fd >= FD_TABLE + 3)
		return INVALID_HANDLE_VALUE;
	return fd_tab[fd - 3];
}

static void fd_del(int fd) {
	if (fd < 3 || fd >= FD_TABLE + 3)
		return;
	EnterCriticalSection(&fd_lock);
	fd_tab[fd - 3] = 0;
	LeaveCriticalSection(&fd_lock);
}

int xos_open(const char *path, int flags, int mode) {
	(void)mode;
	DWORD access = 0, creat = 0;
	int rd = flags & XOS_O_READ, wr = flags & XOS_O_WRITE;
	if (rd && wr)
		access = GENERIC_READ | GENERIC_WRITE;
	else if (wr)
		access = GENERIC_WRITE;
	else
		access = GENERIC_READ;
	if ((flags & XOS_O_CREATE) && (flags & XOS_O_TRUNC))
		creat = CREATE_ALWAYS;
	else if (flags & XOS_O_CREATE)
		creat = (flags & XOS_O_APPEND) ? OPEN_ALWAYS : OPEN_ALWAYS;
	else
		creat = OPEN_EXISTING;
	HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, 0, creat, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	if (flags & XOS_O_APPEND)
		SetFilePointer(h, 0, 0, FILE_END);
	int fd = fd_put(h);
	if (fd < 0) {
		CloseHandle(h);
		return -1;
	}
	return fd;
}

int xos_close(int fd) {
	if (fd < 3)
		return 0;
	HANDLE h = fd_get(fd);
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	fd_del(fd);
	return CloseHandle(h) ? 0 : -1;
}

isz xos_read(int fd, void *buf, usz n) {
	DWORD got = 0;
	if (!ReadFile(fd_get(fd), buf, (DWORD)n, &got, 0))
		return -1;
	return (isz)got;
}

isz xos_write(int fd, const void *buf, usz n) {
	DWORD wrote = 0;
	if (!WriteFile(fd_get(fd), buf, (DWORD)n, &wrote, 0))
		return -1;
	return (isz)wrote;
}

int xos_stat(const char *path, xos_finfo *fi) {
	WIN32_FILE_ATTRIBUTE_DATA d;
	memset(fi, 0, sizeof *fi);
	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &d))
		return -1;
	fi->exists = 1;
	fi->isdir = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
	fi->isreg = fi->isdir ? 0 : 1;
	fi->isexec = 0;
	{
		usz l = 0;
		while (path[l])
			l++;
		if (l >= 4) {
			const char *q = path + l - 4;
			if (q[0] == '.' && (q[1] == 'e' || q[1] == 'E') && (q[2] == 'x' || q[2] == 'X') &&
			    (q[3] == 'e' || q[3] == 'E'))
				fi->isexec = 1;
		}
	}
	fi->size = (long)((ULARGE_INTEGER) { .LowPart = d.nFileSizeLow, .HighPart = d.nFileSizeHigh })
	                 .QuadPart;
	{
		ULARGE_INTEGER t;
		t.LowPart = d.ftLastWriteTime.dwLowDateTime;
		t.HighPart = d.ftLastWriteTime.dwHighDateTime;
		fi->mtime = (long)((t.QuadPart - 116444736000000000ULL) / 10000000ULL);
	}
	return 0;
}

int xos_mkdir(const char *path, int mode) {
	(void)mode;
	if (CreateDirectoryA(path, 0))
		return 0;
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		xos_finfo fi;
		if (xos_stat(path, &fi) == 0 && fi.isdir)
			return 0;
	}
	return -1;
}

int xos_unlink(const char *path) {
	return DeleteFileA(path) ? 0 : -1;
}

int xos_rename(const char *a, const char *b) {
	return MoveFileExA(a, b, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

int xos_chdir(const char *path) {
	return SetCurrentDirectoryA(path) ? 0 : -1;
}

int xos_getcwd(char *buf, usz n) {
	DWORD r = GetCurrentDirectoryA((DWORD)n, buf);
	if (!r || r >= (DWORD)n)
		return -1;
	return (int)r;
}

struct xos_dir {
	HANDLE h;
	WIN32_FIND_DATAA fd;
	int first;
	char name[MAX_PATH];
};

xos_dir *xos_dir_open(const char *path) {
	char pat[MAX_PATH + 3];
	usz l = 0;
	while (path[l] && l < MAX_PATH - 3) {
		pat[l] = path[l];
		l++;
	}
	pat[l++] = '\\';
	pat[l++] = '*';
	pat[l] = 0;
	xos_dir *d = (xos_dir *)xos_alloc(sizeof *d);
	if (!d)
		return 0;
	d->h = FindFirstFileA(pat, &d->fd);
	if (d->h == INVALID_HANDLE_VALUE) {
		xos_free(d);
		return 0;
	}
	d->first = 1;
	d->name[0] = 0;
	return d;
}

const char *xos_dir_next(xos_dir *d) {
	if (!d)
		return 0;
	for (;;) {
		if (!d->first) {
			if (!FindNextFileA(d->h, &d->fd))
				return 0;
		}
		d->first = 0;
		const char *n = d->fd.cFileName;
		if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
			continue;
		usz i = 0;
		while (n[i] && i < sizeof d->name - 1) {
			d->name[i] = n[i];
			i++;
		}
		d->name[i] = 0;
		return d->name;
	}
}

void xos_dir_close(xos_dir *d) {
	if (!d)
		return;
	FindClose(d->h);
	xos_free(d);
}

const char *xos_env_get(const char *key) {
	static char buf[32768];
	DWORD n = GetEnvironmentVariableA(key, buf, sizeof buf);
	if (!n || n >= sizeof buf)
		return 0;
	return buf;
}

int xos_env_set(const char *key, const char *val) {
	return SetEnvironmentVariableA(key, val) ? 0 : -1;
}

char **xos_env_all(void) {
	static char **cached;
	if (cached)
		return cached;
	char *env = GetEnvironmentStringsA();
	if (!env)
		return 0;
	usz count = 0;
	for (char *p = env; *p; p += lstrlenA(p) + 1)
		count++;
	cached = (char **)xos_alloc((count + 1) * sizeof(char *));
	if (!cached) {
		FreeEnvironmentStringsA(env);
		return 0;
	}
	usz i = 0;
	for (char *p = env; *p; p += lstrlenA(p) + 1) {
		usz l = (usz)lstrlenA(p);
		char *c = (char *)xos_alloc(l + 1);
		if (!c)
			break;
		for (usz k = 0; k < l; k++)
			c[k] = p[k];
		c[l] = 0;
		cached[i++] = c;
	}
	cached[i] = 0;
	FreeEnvironmentStringsA(env);
	return cached;
}

long xos_time_sec(void) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER t;
	t.LowPart = ft.dwLowDateTime;
	t.HighPart = ft.dwHighDateTime;
	return (long)((t.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

long xos_time_ms(void) {
	return (long)GetTickCount64();
}

void xos_sleep_ms(long ms) {
	if (ms > 0)
		Sleep((DWORD)ms);
}

static char *join_argv(char *const argv[]) {
	usz need = 1;
	for (int i = 0; argv[i]; i++)
		need += lstrlenA(argv[i]) + 3;
	char *line = (char *)xos_alloc(need);
	if (!line)
		return 0;
	usz pos = 0;
	for (int i = 0; argv[i]; i++) {
		if (i)
			line[pos++] = ' ';
		const char *a = argv[i];
		int sp = 0;
		for (const char *q = a; *q; q++)
			if (*q == ' ' || *q == '\t')
				sp = 1;
		if (sp)
			line[pos++] = '"';
		for (const char *q = a; *q; q++) {
			if (*q == '"')
				line[pos++] = '\\';
			line[pos++] = *q;
		}
		if (sp)
			line[pos++] = '"';
	}
	line[pos] = 0;
	return line;
}

static char *join_env(char *const env[]) {
	if (!env)
		return 0;
	usz need = 2;
	for (int i = 0; env[i]; i++)
		need += lstrlenA(env[i]) + 1;
	char *blob = (char *)xos_alloc(need);
	if (!blob)
		return 0;
	usz pos = 0;
	for (int i = 0; env[i]; i++) {
		const char *e = env[i];
		while (*e)
			blob[pos++] = *e++;
		blob[pos++] = 0;
	}
	blob[pos++] = 0;
	return blob;
}

long xos_proc_spawn(char *const argv[], char *const env[]) {
	if (!argv || !argv[0])
		return -1;
	char *line = join_argv(argv);
	if (!line)
		return -1;
	char *envblob = join_env(env);
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof si);
	si.cb = sizeof si;
	memset(&pi, 0, sizeof pi);
	BOOL ok = CreateProcessA(0, line, 0, 0, TRUE, 0, envblob, 0, &si, &pi);
	xos_free(line);
	xos_free(envblob);
	if (!ok)
		return -1;
	CloseHandle(pi.hThread);
	return (long)(intptr_t)pi.hProcess;
}

int xos_proc_wait(long pid) {
	HANDLE h = (HANDLE)(intptr_t)pid;
	if (!h || h == INVALID_HANDLE_VALUE)
		return -1;
	WaitForSingleObject(h, INFINITE);
	DWORD code = 0;
	GetExitCodeProcess(h, &code);
	CloseHandle(h);
	return (int)code;
}

int xos_proc_exec(char *const argv[], char *const env[]) {
	long pid = xos_proc_spawn(argv, env);
	if (pid < 0)
		return -1;
	int rc = xos_proc_wait(pid);
	ExitProcess((UINT)rc);
	return -1;
}

long xos_proc_getpid(void) {
	return (long)GetCurrentProcessId();
}

int xos_proc_kill(long pid, int sig) {
	HANDLE h = (HANDLE)(intptr_t)pid;
	if (!h || h == INVALID_HANDLE_VALUE)
		return -1;
	return TerminateProcess(h, (UINT)(sig ? sig : 1)) ? 0 : -1;
}

int xos_proc_trywait(long pid) {
	HANDLE h = (HANDLE)(intptr_t)pid;
	if (!h || h == INVALID_HANDLE_VALUE)
		return -1;
	DWORD w = WaitForSingleObject(h, 0);
	if (w == WAIT_TIMEOUT)
		return -2;
	if (w != WAIT_OBJECT_0)
		return -1;
	DWORD code = 0;
	GetExitCodeProcess(h, &code);
	CloseHandle(h);
	return (int)code;
}

long xos_proc_fork(void) {
	return -1;
}

static int fd_setslot(int fd, HANDLE h) {
	if (fd >= 0 && fd <= 2) {
		DWORD which = fd == 0 ? STD_INPUT_HANDLE : fd == 2 ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
		return SetStdHandle(which, h) ? 0 : -1;
	}
	if (fd < 3 || fd >= FD_TABLE + 3)
		return -1;
	fd_init();
	EnterCriticalSection(&fd_lock);
	HANDLE old = fd_tab[fd - 3];
	fd_tab[fd - 3] = h;
	LeaveCriticalSection(&fd_lock);
	if (old)
		CloseHandle(old);
	return 0;
}

int xos_pipe(int fds[2]) {
	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof sa);
	sa.nLength = sizeof sa;
	sa.bInheritHandle = TRUE;
	HANDLE r, w;
	if (!CreatePipe(&r, &w, &sa, 0))
		return -1;
	int fr = fd_put(r), fw = fd_put(w);
	if (fr < 0 || fw < 0) {
		if (fr >= 0) {
			HANDLE h = fd_get(fr);
			if (h && h != INVALID_HANDLE_VALUE)
				CloseHandle(h);
			fd_del(fr);
		}
		CloseHandle(r);
		CloseHandle(w);
		return -1;
	}
	fds[0] = fr;
	fds[1] = fw;
	return 0;
}

int xos_dup2(int oldfd, int newfd) {
	if (oldfd == newfd)
		return 0;
	HANDLE h = fd_get(oldfd);
	if (h == INVALID_HANDLE_VALUE || !h)
		return -1;
	HANDLE dup;
	if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &dup, 0, TRUE, DUPLICATE_SAME_ACCESS))
		return -1;
	if (fd_setslot(newfd, dup) != 0) {
		CloseHandle(dup);
		return -1;
	}
	return 0;
}

int xos_signal(int sig, int mode) {
	return signal(sig, mode == XOS_SIG_IGN ? SIG_IGN : SIG_DFL) == SIG_ERR ? -1 : 0;
}

static DWORD tio_saved;
static int tio_have_saved;

int xos_isatty(int fd) {
	DWORD mode;
	return GetConsoleMode(con_handle(fd), &mode) ? 1 : 0;
}

int xos_term_raw(int on) {
	HANDLE h = con_handle(0);
	DWORD mode;
	if (!GetConsoleMode(h, &mode))
		return -1;
	if (on) {
		if (!tio_have_saved) {
			tio_saved = mode;
			tio_have_saved = 1;
		}
		DWORD raw = ENABLE_WINDOW_INPUT;
		return SetConsoleMode(h, raw) ? 0 : -1;
	}
	if (tio_have_saved)
		return SetConsoleMode(h, tio_saved) ? 0 : -1;
	return 0;
}

int xos_term_size(int *w, int *h) {
	CONSOLE_SCREEN_BUFFER_INFO ci;
	if (GetConsoleScreenBufferInfo(con_handle(1), &ci)) {
		if (w)
			*w = ci.srWindow.Right - ci.srWindow.Left + 1;
		if (h)
			*h = ci.srWindow.Bottom - ci.srWindow.Top + 1;
		return 0;
	}
	if (w)
		*w = 80;
	if (h)
		*h = 24;
	return -1;
}

struct xos_mtx {
	SRWLOCK l;
};
struct xos_cv {
	CONDITION_VARIABLE c;
};
struct xos_thr {
	HANDLE h;
};

xos_mtx *xos_mtx_new(void) {
	xos_mtx *m = (xos_mtx *)xos_alloc(sizeof *m);
	if (!m)
		xos_exit(1);
	InitializeSRWLock(&m->l);
	return m;
}
void xos_mtx_lock(xos_mtx *m) {
	AcquireSRWLockExclusive(&m->l);
}
void xos_mtx_unlock(xos_mtx *m) {
	ReleaseSRWLockExclusive(&m->l);
}
void xos_mtx_free(xos_mtx *m) {
	xos_free(m);
}

xos_cv *xos_cv_new(void) {
	xos_cv *c = (xos_cv *)xos_alloc(sizeof *c);
	if (!c)
		xos_exit(1);
	InitializeConditionVariable(&c->c);
	return c;
}
void xos_cv_wait(xos_cv *c, xos_mtx *m) {
	SleepConditionVariableSRW(&c->c, &m->l, INFINITE, 0);
}
void xos_cv_signal(xos_cv *c) {
	WakeConditionVariable(&c->c);
}
void xos_cv_broadcast(xos_cv *c) {
	WakeAllConditionVariable(&c->c);
}
void xos_cv_free(xos_cv *c) {
	xos_free(c);
}

typedef struct {
	void (*fn)(void *);
	void *arg;
} thr_start;

static DWORD WINAPI thr_trampoline(LPVOID p) {
	thr_start *s = (thr_start *)p;
	s->fn(s->arg);
	xos_free(s);
	return 0;
}

xos_thr *xos_thread_create(void (*fn)(void *), void *arg) {
	thr_start *s = (thr_start *)xos_alloc(sizeof *s);
	if (!s)
		return 0;
	s->fn = fn;
	s->arg = arg;
	xos_thr *t = (xos_thr *)xos_alloc(sizeof *t);
	if (!t) {
		xos_free(s);
		return 0;
	}
	t->h = CreateThread(0, 0, thr_trampoline, s, 0, 0);
	if (!t->h) {
		xos_free(s);
		xos_free(t);
		return 0;
	}
	return t;
}

int xos_thread_join(xos_thr *t) {
	if (!t)
		return -1;
	WaitForSingleObject(t->h, INFINITE);
	CloseHandle(t->h);
	xos_free(t);
	return 0;
}

int xos_cpu_count(void) {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors > 0 ? (int)si.dwNumberOfProcessors : 1;
}
