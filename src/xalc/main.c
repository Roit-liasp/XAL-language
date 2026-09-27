/*
 main.c - xalc CLI:
   xalc [-o out.xb] file.xa    compile
   xalc -d file.xb             disassemble a bytecode image
   xalc -v                     version
*/

#include "xal.h"

#define XAL_VERSION "XALC V0.8"

static int compile_file(const char *in, const char *out) {
	char *src = xslurp(in, 0);
	if (!src) {
		xfmt(2, "xal: %s: %e\n", in);
		return 1;
	}
	compiler cp;
	xa_compiler_init(&cp, in);

	xa_compile_source(&cp, src, in);

	if (cp.init_entry < 0)
		cp.init_entry = (int)cp.code.len;
	xa_emit(&cp, OP_PUSHI, 0);
	xa_emit(&cp, OP_RET, 0);

	fnrec *initf = xcalloc(1, sizeof *initf);
	initf->name = xstrdup("__init");
	initf->entry = cp.init_entry;
	initf->nparams = 0;
	initf->nlocals = 0;
	xvec_init(&initf->locals);
	xvec_push(&cp.fns, initf);
	int initidx = (int)cp.fns.len - 1;

	int rc = xa_write_image(&cp, initidx, out);
	xfree(src);
	return rc;
}

// I'm already tired of all this...

int main(int argc, char **argv) {
	const char *out = 0;
	int dump = 0;
	int i = 1;
	for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
		if (argv[i][1] == 'o' && i + 1 < argc)
			out = argv[++i];
		else if (argv[i][1] == 'd')
			dump = 1;
		else if (argv[i][1] == 'v') {
			xout(XAL_VERSION "\n");
			return 0;
		} else {
			xerr("usage: xalc [-o out.xb] file.xa | xal -d file.xb\n");
			return 2;
		}
	}
	if (i >= argc) {
		xerr("usage: xalc [-o out.xb] file.xa | xal -d file.xb\n");
		return 2;
	}
	const char *in = argv[i];

	if (dump)
		return xa_dump_image(in);

	xstr def;
	xstr_init(&def);
	if (!out) {
		xstr_add(&def, in);
		if (xstrsuffix(def.p, ".xa")) {
			def.p[def.len - 3] = 0;
			def.len -= 3;
		}
		xstr_add(&def, ".xb");
		out = def.p;
	}
	int rc = compile_file(in, out);
	xstr_free(&def);
	return rc;
}
