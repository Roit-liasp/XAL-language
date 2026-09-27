#include <stdio.h>
#include <stdlib.h>
#include "xvm.h"
#include "xcore.h"
#include "xos.h"

static int fails = 0;

static void check(int ok, const char *what) {
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	if (!ok) fails++;
}

static u8 *slurp(const char *path, usz *len) {
	FILE *f = fopen(path, "rb");
	long n;
	u8 *b;
	if (!f) return 0;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	b = (u8 *)malloc((size_t)(n > 0 ? n : 1));
	if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); fclose(f); return 0; }
	fclose(f);
	*len = (usz)n;
	return b;
}

int main(int argc, char **argv) {
	usz plen = 0, clen = 0;
	u8 *pbuf, *cbuf;
	xcore_image pim;
	xcore_image *pheap;
	vm_thread *mt;
	char *av[2];
	void *saved;
	int rc_child, rc_parent, ok_many, i;

	if (argc < 3) {
		fprintf(stderr, "usage: nested parent.xb child.xb\n");
		return 2;
	}
	setvbuf(stdout, 0, _IOLBF, 0);
	pbuf = slurp(argv[1], &plen);
	cbuf = slurp(argv[2], &clen);
	if (!pbuf || !cbuf) { fprintf(stderr, "nested: cannot read images\n"); return 2; }
	if (xcore_image_read(pbuf, plen, &pim) < 0) {
		fprintf(stderr, "nested: bad parent image: %s\n", xcore_image_err());
		return 2;
	}
	pheap = (xcore_image *)xmalloc(sizeof pim);
	*pheap = pim;

	av[0] = (char *)"parent";
	av[1] = 0;

	if (!vm_lock) vm_lock = xos_mtx_new();

	vm_image = pheap;
	vm_argc = 1;
	vm_argv = av;
	vm_nglobals = pheap->nglobals;
	vm_globals = (val *)xmalloc((usz)(pheap->nglobals ? pheap->nglobals : 1) * sizeof(val));
	for (u32 k = 0; k < vm_nglobals; k++) vm_globals[k] = vint(0);
	mt = thread_new(pheap->initidx, 0);
	vm_cur = mt;
	thread_register(mt);
	vm_run(mt, pheap->initidx, 0);
	mt->sp = 0;

	saved = vm_state_save();
	check(saved != 0, "vm_state_save return");
	printf("- nested child:\n");
	rc_child = xvm_run_mem(cbuf, clen, 1, av);
	printf("- сhild done rc=%d\n", rc_child);
	check(rc_child == 42, "(42)");

	vm_state_restore(saved);
	printf("- parent continues:\n");
	rc_parent = vm_run(mt, pheap->mainidx, 0);
	check((rc_parent & 255) == 7, "(7)");

	ok_many = 1;
	for (i = 0; i < 200; i++) {
		void *s2 = vm_state_save();
		int rc = xvm_run_mem(cbuf, clen, 1, av);
		vm_state_restore(s2);
		if (rc != 42) ok_many = 0;
	}
	check(ok_many, "200 OK");

	printf("%s\n", fails ? "NESTED: FAIL" : "NESTED: OK");
	return fails ? 1 : 0;
}
