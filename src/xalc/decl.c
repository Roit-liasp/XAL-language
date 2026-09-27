#include "xal.h"

static char *dir_of(const char *path) {
	const char *slash = xstrrchr(path, '/');
	if (!slash)
		return 0;
	usz dlen = (usz)(slash - path);
	if (!dlen)
		return xstrdup("/");
	return xstrndup(path, dlen);
}

char *xa_resolve(compiler *cp, const char *name) {
	if (xstrchr(name, '.'))
		return xstrdup(name);
	if (cp->cur && xa_local_idx(cp->cur, name, 0) >= 0)
		return xstrdup(name);
	if (cp->cur_module) {
		xstr scoped;
		xstr_init(&scoped);
		xstr_add(&scoped, cp->cur_module);
		xstr_addc(&scoped, '.');
		xstr_add(&scoped, name);
		for (usz i = 0; i < cp->pub_fns.len; i++) {
			if (!xstrcmp(cp->pub_fns.it[i], scoped.p)) {
				char *r = xstrdup(scoped.p);
				xstr_free(&scoped);
				return r;
			}
		}
		for (usz i = 0; i < cp->pub_globals.len; i++) {
			if (!xstrcmp(cp->pub_globals.it[i], scoped.p)) {
				char *r = xstrdup(scoped.p);
				xstr_free(&scoped);
				return r;
			}
		}
		xstr_free(&scoped);
	}
	for (usz i = 0; i < cp->pub_fns.len; i++) {
		char *pf = cp->pub_fns.it[i];
		char *dot = xstrchr(pf, '.');
		if (dot && !xstrcmp(dot + 1, name))
			return xstrdup(pf);
	}
	for (usz i = 0; i < cp->pub_globals.len; i++) {
		char *pg = cp->pub_globals.it[i];
		char *dot = xstrchr(pg, '.');
		if (dot && !xstrcmp(dot + 1, name))
			return xstrdup(pg);
	}
	return xstrdup(name);
}

void xa_function(compiler *cp, int is_pub) {
	lexer *lx = cp->lx;
	lex_expect(lx, "fn");
	if (lx->cur.k != L_ID)
		lex_fail(lx, "expected function name");
	char *raw_name = xstrdup(lx->cur.s);
	lex_advance(lx);

	char *fn_name = 0;
	if (is_pub && cp->cur_module) {
		xstr sc;
		xstr_init(&sc);
		xstr_add(&sc, cp->cur_module);
		xstr_addc(&sc, '.');
		xstr_add(&sc, raw_name);
		fn_name = xstr_take(&sc);
		xvec_push(&cp->pub_fns, xstrdup(fn_name));
	} else {
		fn_name = xstrdup(raw_name);
	}
	xfree(raw_name);

	fnrec *f = xa_fn_find(cp, fn_name);
	if (!f) {
		f = xcalloc(1, sizeof *f);
		f->name = xstrdup(fn_name);
		f->entry = -1;
		xvec_init(&f->locals);
		xvec_push(&cp->fns, f);
	}
	if (f->entry >= 0) {
		xstr m;
		xstr_init(&m);
		xstr_add(&m, "duplicate function '");
		xstr_add(&m, fn_name);
		xstr_addc(&m, '\'');
		lex_fail(lx, m.p);
	}
	if (cp->fns.len > XCORE_MAX_FNS)
		lex_fail(lx, "too many functions");

	int skip = cp->init_entry >= 0 ? xa_emit(cp, OP_JMP, 0) : -1;

	f->entry = (int)cp->code.len;
	cp->cur = f;
	lex_expect(lx, "(");
	int np = 0;
	if (!lex_at(lx, ")")) {
		do {
			if (lx->cur.k != L_ID)
				lex_fail(lx, "expected parameter");
			xa_local_idx(f, lx->cur.s, 1);
			np++;
			lex_advance(lx);
		} while (lex_accept(lx, ","));
	}
	lex_expect(lx, ")");
	f->nparams = np;
	xa_block(cp);
	xa_emit(cp, OP_PUSHI, 0);
	xa_emit(cp, OP_RET, 0);
	cp->cur = 0;
	if (skip >= 0)
		xa_patch(cp, skip, (long)cp->code.len);
	xfree(fn_name);
}

static void try_path(xstr *out, const char *dir, const char *modname) {
	xstr_clear(out);
	if (dir && dir[0]) {
		xstr_add(out, dir);
		xstr_addc(out, '/');
	}
	xstr_add(out, modname);
	if (!xstrsuffix(out->p, ".xa"))
		xstr_add(out, ".xa");
}

void xa_import(compiler *cp, const char *modname) {
	for (usz i = 0; i < cp->imported_mods.len; i++)
		if (!xstrcmp(cp->imported_mods.it[i], modname))
			return;
	xvec_push(&cp->imported_mods, xstrdup(modname));

	xstr cand;
	xstr_init(&cand);
	const char *chosen = 0;

	char *main_dir = cp->main_file ? dir_of(cp->main_file) : 0;
	const char *dirs[3];
	int ndirs = 0;
	if (cp->cur_dir)
		dirs[ndirs++] = cp->cur_dir;
	if (main_dir)
		dirs[ndirs++] = main_dir;
	dirs[ndirs++] = "";
	for (int i = 0; i < ndirs && !chosen; i++) {
		try_path(&cand, dirs[i], modname);
		if (xexists(cand.p))
			chosen = cand.p;
	}
	if (!chosen) {
		char *xl = xgetenv("XAL_LIB");
		try_path(&cand, xl ? xl : "/lib/xal", modname);
		if (xexists(cand.p))
			chosen = cand.p;
	}
	if (!chosen) {
		xfmt(2, "xal: module not found: %s (looked in %s, cwd, $XAL_LIB)\n", modname, cp->cur_dir ? cp->cur_dir : "main file dir");
		xos_exit(1);
	}

	for (usz i = 0; i < cp->imported_files.len; i++)
		if (!xstrcmp(cp->imported_files.it[i], chosen)) {
			xstr_free(&cand);
			xfree(main_dir);
			return;
		}
	xvec_push(&cp->imported_files, xstrdup(chosen));

	char *src = xslurp(chosen, 0);
	if (!src) {
		xfmt(2, "xal: cannot read imported module '%s': %e\n", chosen);
		xos_exit(1);
	}

	char *save_mod = cp->cur_module;
	cp->cur_module = 0;
	xa_compile_source(cp, src, chosen);
	cp->cur_module = save_mod;

	xfree(src);
	xstr_free(&cand);
	xfree(main_dir);
}

void xa_compile_source(compiler *cp, const char *src, const char *filename) {
	lexer *old_lx = cp->lx;
	char *old_dir = cp->cur_dir;
	cp->cur_dir = dir_of(filename);

	lexer *lx = xcalloc(1, sizeof(lexer));
	lex_init(lx, src, filename);
	cp->lx = lx;

	while (lx->cur.k != L_EOF) {
		if (lex_at(lx, "module")) {
			lex_advance(lx);
			lex_expect(lx, "(");
			if (lx->cur.k != L_ID)
				lex_fail(lx, "expected module name");
			xfree(cp->cur_module);
			cp->cur_module = xstrdup(lx->cur.s);
			lex_advance(lx);
			lex_expect(lx, ")");
			lex_accept(lx, ";");
			continue;
		}
		if (lex_at(lx, "import")) {
			lex_advance(lx);
			if (lx->cur.k != L_STR)
				lex_fail(lx, "expected string module name after import");
			char *mod = xstrdup(lx->cur.s);
			lex_advance(lx);
			lex_accept(lx, ";");
			xa_import(cp, mod);
			xfree(mod);
			continue;
		}
		if (lex_at(lx, "use")) {
			lex_advance(lx);
			while (lx->cur.k != L_EOF && !lex_at(lx, ";"))
				lex_advance(lx);
			lex_accept(lx, ";");
			continue;
		}
		int is_pub = 0;
		if (lex_at(lx, "pub")) {
			lex_advance(lx);
			is_pub = 1;
		}
		if (lex_at(lx, "fn")) {
			xa_function(cp, is_pub);
			continue;
		}
		if (cp->init_entry < 0)
			cp->init_entry = (int)cp->code.len;
		if (is_pub) {
			if (lex_at(lx, "let")) {
				xa_stmt_pub_let(cp);
				continue;
			}
			lex_fail(lx, "expected 'fn' or 'let' after 'pub'");
		}
		xa_stmt(cp);
	}
	lex_free(lx);
	xfree(lx);
	cp->lx = old_lx;
	xfree(cp->cur_dir);
	cp->cur_dir = old_dir;
}

void xa_stmt_pub_let(compiler *cp) {
	lexer *lx = cp->lx;
	lex_advance(lx);
	if (lx->cur.k != L_ID)
		lex_fail(lx, "expected name after let");
	char *raw_name = xstrdup(lx->cur.s);
	lex_advance(lx);
	char *var_name = 0;
	if (cp->cur_module) {
		xstr sc;
		xstr_init(&sc);
		xstr_add(&sc, cp->cur_module);
		xstr_addc(&sc, '.');
		xstr_add(&sc, raw_name);
		var_name = xstr_take(&sc);
		xvec_push(&cp->pub_globals, xstrdup(var_name));
	} else {
		var_name = xstrdup(raw_name);
		xvec_push(&cp->pub_globals, xstrdup(var_name));
	}
	xfree(raw_name);

	if (lex_accept(lx, "="))
		xa_expr(cp);
	else
		xa_emit(cp, OP_PUSHI, 0);
	xa_emit(cp, OP_GSTORE, xa_global_idx(cp, var_name, 1));
	xfree(var_name);
	lex_accept(lx, ";");
}

void xa_compiler_init(compiler *cp, const char *main_file) {
	unsigned char *b = (unsigned char *)cp;
	for (usz i = 0; i < sizeof(*cp); i++)
		b[i] = 0;
	icode_init(&cp->code);
	xvec_init(&cp->strs);
	xvec_init(&cp->fns);
	xvec_init(&cp->globals);
	xvec_init(&cp->breaks);
	xvec_init(&cp->conts);
	xvec_init(&cp->pub_fns);
	xvec_init(&cp->pub_globals);
	xvec_init(&cp->imported_mods);
	xvec_init(&cp->imported_files);
	cp->main_file = main_file;
	cp->init_entry = -1;
}
