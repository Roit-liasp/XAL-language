#include "xos.h"

int xvm_run_mem(const u8 *buf, usz len, int argc, char **argv);
extern const unsigned char prog_xb[];
extern const unsigned long prog_xb_len;

static long sys3(long n, long a, long b, long c) {
	long r;
	__asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
	return r;
}
void xos_bare_out(const char *b, usz n) { sys3(1, 1, (long)b, (long)n); }
void xos_bare_err(const char *b, usz n) { sys3(1, 2, (long)b, (long)n); }
int xos_bare_in(void) { unsigned char c; return sys3(0, 0, (long)&c, 1) == 1 ? c : -1; }
void bare_main(void) {
	int rc = xvm_run_mem(prog_xb, prog_xb_len, 0, 0);
	sys3(60, rc, 0, 0);
	for (;;) {}
}
