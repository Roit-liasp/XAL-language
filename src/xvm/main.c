#include "xvm.h"

#ifndef XVM_FREESTANDING

#define XVM_VERSION "XVM V0.8"

static int caps_report(const char *path) {
	usz len = 0;
	char *raw = xslurp(path, &len);
	if (!raw) {
		xfmt(2, "xvm: %s: %e\n", path);
		return 1;
	}
	xcore_image im;
	if (xcore_image_read((const u8 *)raw, len, &im) < 0) {
		xfmt(2, "xvm: %s: bad bytecode image: %s\n", path, xcore_image_err());
		xfree(raw);
		return 1;
	}
	xfree(raw);
	char need[256], have[256], missing[256];
	u32 req = xvm_image_caps(&im);
	xvm_caps_format(req, need, sizeof need);
	xvm_caps_format(xvm_caps(), have, sizeof have);
	xvm_caps_format(req & ~xvm_caps(), missing, sizeof missing);
	xout("needs:   ");
	xout(need[0] ? need : "(nothing)");
	xout("\n");
	xout("host:    ");
	xout(have);
	xout("\n");
	if (missing[0]) {
		xout("missing: ");
		xout(missing);
		xout("\n");
		xcore_image_free(&im);
		return 1;
	}
	xout("ok: can run here\n");
	xcore_image_free(&im);
	return 0;
}

static int run_file(const char *path, int argc, char **argv) {
	usz len = 0;
	char *raw = xslurp(path, &len);
	if (!raw) {
		xfmt(2, "xvm: %s: %e\n", path);
		return 1;
	}
	xcore_image im;
	if (xcore_image_read((const u8 *)raw, len, &im) < 0) {
		xfmt(2, "xvm: %s: bad bytecode image: %s\n", path, xcore_image_err());
		xfree(raw);
		return 1;
	}
	xfree(raw);
	xcore_image *heap = xmalloc(sizeof im);
	*heap = im;
	return xvm_run_image(heap, argc, argv);
}

int main(int argc, char **argv) {
	int argi = 1;
	xvm_self_path = argc > 0 ? argv[0] : 0;

	while (argi < argc && !xstrcmp(argv[argi], "--no-xmc")) {
		gc_enabled = 0;
		argi++;
	}
	if (argi < argc && (!xstrcmp(argv[argi], "-v") || !xstrcmp(argv[argi], "--version"))) {
		xout(XVM_VERSION "\n");
		return 0;
	}
	if (argi < argc && !xstrcmp(argv[argi], "--caps")) {
		if (argi + 1 >= argc) {
			xerr("usage: xvm --caps program.xb\n");
			return 2;
		}
		return caps_report(argv[argi + 1]);
	}

	const char *base = xstrrchr(argv[0], '/');
	base = base ? base + 1 : argv[0];
	if (xstrcmp(base, "xvm") && xstrcmp(base, "xvm.exe")) {
		xstr s;
		xstr_init(&s);
		const char *dir = xgetenv("XAL_LIB");
		xstr_add(&s, dir ? dir : "/lib/xal");
		xstr_addc(&s, '/');
		xstr_add(&s, base);
		xstr_add(&s, ".xb");
		if (!xexists(s.p)) {
			xfmt(2, "xvm: %s: bytecode not found\n", s.p);
			return 127;
		}
		int rc = run_file(s.p, argc - (argi - 1), argv + (argi - 1));
		xstr_free(&s);
		return rc;
	}

	if (argi >= argc) {
		xerr("usage: xvm [--no-xmc] program.xb [args]\n");
		return 2;
	}
	return run_file(argv[argi], argc - argi, argv + argi);
}

#endif
