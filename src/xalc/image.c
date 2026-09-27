#include "xal.h"

int xa_write_image(compiler *cp, int initidx, const char *outpath) {
	int mainidx = xa_fn_index(cp, "main");
	if (mainidx < 0) {
		int entry = xa_emit(cp, OP_PUSHI, 0);
		xa_emit(cp, OP_RET, 0);
		fnrec *mf = xcalloc(1, sizeof *mf);
		mf->name = xstrdup("main");
		mf->entry = entry;
		mf->nparams = 0;
		mf->nlocals = 0;
		xvec_init(&mf->locals);
		xvec_push(&cp->fns, mf);
		mainidx = xa_fn_index(cp, "main");
		xfmt(2, "xal: warning: no main function - library image\n");
	}
	for (usz i = 0; i < cp->fns.len; i++) {
		fnrec *f = cp->fns.it[i];
		if (f->entry < 0) {
			xfmt(2, "xal: undefined function: %s\n", f->name);
			return 1;
		}
	}

	for (usz i = 0; i < cp->fns.len; i++) {
		fnrec *f = cp->fns.it[i];
		xa_str_intern(cp, f->name);
	}

	xcore_image im;
	unsigned char *b = (unsigned char *)&im;
	for (usz i = 0; i < sizeof im; i++)
		b[i] = 0;
	im.version = XAL_FMT_VERSION;
	im.nstrs = (u32)cp->strs.len;
	im.strs = (char **)cp->strs.it;
	im.nfns = (u32)cp->fns.len;
	im.nglobals = (u32)cp->globals.len;
	im.mainidx = (u32)mainidx;
	im.initidx = (u32)initidx;
	im.ncode = (u32)cp->code.len;

	xcore_fn *fns = xmalloc((usz)(cp->fns.len ? cp->fns.len : 1) * sizeof(xcore_fn));
	for (usz i = 0; i < cp->fns.len; i++) {
		fnrec *f = cp->fns.it[i];
		fns[i].entry = (u32)f->entry;
		fns[i].nparams = (u32)f->nparams;
		fns[i].nlocals = (u32)(f->nlocals > f->nparams ? f->nlocals : f->nparams);
		fns[i].nameidx = (u32)xa_str_intern(cp, f->name);
	}
	im.fns = fns;
	im.code = cp->code.v;

	usz len = 0;
	u8 *buf = xcore_image_write(&im, &len);
	xfree(fns);
	if (!buf) {
		xerr("xal: out of memory writing image\n");
		return 1;
	}

	int fd = xopen(outpath, XOS_O_WRITE | XOS_O_CREATE | XOS_O_TRUNC, 0755);
	if (fd < 0) {
		xfmt(2, "xal: %s: %e\n", outpath);
		xos_free(buf);
		return 1;
	}
	xwrite(fd, buf, len);
	xclose(fd);
	xos_free(buf);
	return 0;
}

static void print_str(const char *s) {
	xout("\"");
	for (const char *p = s; *p; p++) {
		switch (*p) {
		case '"':
			xout("\\\"");
			break;
		case '\\':
			xout("\\\\");
			break;
		case '\n':
			xout("\\n");
			break;
		case '\t':
			xout("\\t");
			break;
		default:
			if ((u8)*p < 32) {
				char b[5];
				b[0] = '\\';
				b[1] = 'x';
				static const char hx[] = "0123456789abcdef";
				b[2] = hx[((u8)*p >> 4) & 15];
				b[3] = hx[(u8)*p & 15];
				xos_con_write(1, b, 4);
			} else
				xos_con_write(1, p, 1);
		}
	}
	xout("\"");
}

int xa_dump_image(const char *path) {
	usz len = 0;
	char *raw = xslurp(path, &len);
	if (!raw) {
		xfmt(2, "xal: %s: %e\n", path);
		return 1;
	}
	xcore_image im;
	if (xcore_image_read((const u8 *)raw, len, &im) < 0) {
		xfmt(2, "xal: %s: bad bytecode image: %s\n", path, xcore_image_err());
		xfree(raw);
		return 1;
	}
	xfree(raw);

	xfmt(1, "%s: XALB v%l  %l strings  %l functions  %l globals  %l insns\n", path, (long)im.version, (long)im.nstrs, (long)im.nfns, (long)im.nglobals, (long)im.ncode);
	xfmt(1, "main=#%l init=#%l\n\n", (long)im.mainidx, (long)im.initidx);

	xout("functions:\n");
	for (u32 i = 0; i < im.nfns; i++) {
		char b[32];
		xstr line;
		xstr_init(&line);
		xstr_add(&line, "  #");
		xltoa((long)i, b, sizeof b);
		xstr_add(&line, b);
		xstr_add(&line, "  ");
		xstr_add(&line, im.strs[im.fns[i].nameidx]);
		xstr_add(&line, "  params=");
		xltoa((long)im.fns[i].nparams, b, sizeof b);
		xstr_add(&line, b);
		xstr_add(&line, " locals=");
		xltoa((long)im.fns[i].nlocals, b, sizeof b);
		xstr_add(&line, b);
		xstr_add(&line, " entry=");
		xltoa((long)im.fns[i].entry, b, sizeof b);
		xstr_add(&line, b);
		xstr_addc(&line, '\n');
		xout(line.p);
		xstr_free(&line);
	}
	xout("\ncode:\n");
	for (u32 pc = 0; pc < im.ncode; pc++) {
		u32 op = im.code[pc].op;
		long a = im.code[pc].a;
		char b[32];
		xstr line;
		xstr_init(&line);
		xstr_addc(&line, ' ');
		xstr_addc(&line, ' ');
		xstr_addc(&line, ' ');
		xstr_addc(&line, ' ');
		xltoa((long)pc, b, sizeof b);
		for (usz pad = xstrlen(b); pad < 6; pad++)
			xstr_addc(&line, ' ');
		xstr_add(&line, b);
		xstr_add(&line, "  ");
		xstr_add(&line, xcore_op_name[op]);
		switch (op) {
		case OP_PUSHS:
			xstr_addc(&line, ' ');
			xout(line.p);
			if ((u32)a < im.nstrs)
				print_str(im.strs[a]);
			else {
				char b2[32];
				xltoa(a, b2, sizeof b2);
				xout(b2);
			}
			xout("\n");
			break;
		case OP_CALL:
		case OP_NCALL: {
			u32 idx = XCORE_IDX(a);
			u32 argc = XCORE_ARGC(a);
			xstr_add(&line, "  ");
			if (op == OP_CALL && idx < im.nfns)
				xstr_add(&line, im.strs[im.fns[idx].nameidx]);
			else if (op == OP_NCALL && idx < (u32)NAT__COUNT)
				xstr_add(&line, xcore_nat_name[idx]);
			else {
				xstr_addl(&line, (long)idx);
			}
			xstr_add(&line, " argc=");
			xstr_addl(&line, (long)argc);
			xstr_addc(&line, '\n');
			xout(line.p);
			break;
		}
		case OP_NOP:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_NEG:
		case OP_EQ:
		case OP_NE:
		case OP_LT:
		case OP_LE:
		case OP_GT:
		case OP_GE:
		case OP_NOT:
		case OP_RET:
		case OP_POP:
		case OP_DUP:
		case OP_NEWLIST:
		case OP_INDEX:
		case OP_SETIDX:
		case OP_APPEND:
		case OP_LEN:
		case OP_CONCAT:
		case OP_HALT:
			xstr_addc(&line, '\n');
			xout(line.p);
			break;
		default:
			xstr_addc(&line, ' ');
			xstr_addl(&line, a);
			xstr_addc(&line, '\n');
			xout(line.p);
		}
		xstr_free(&line);
	}
	xcore_image_free(&im);
	return 0;
}
