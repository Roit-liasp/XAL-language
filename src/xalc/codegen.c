#include "xal.h"

void icode_init(icode *c) {
	c->v = 0;
	c->len = 0;
	c->cap = 0;
}

int icode_emit(icode *c, u32 op, long a) {
	if (c->len == c->cap) {
		c->cap = c->cap ? c->cap * 2 : 64;
		c->v = xrealloc(c->v, c->cap * sizeof(xcore_insn));
	}
	c->v[c->len].op = op;
	c->v[c->len].a = a;
	return (int)c->len++;
}

void icode_patch(icode *c, int at, long a) {
	c->v[at].a = a;
}

int xa_emit(compiler *cp, u32 op, long a) {
	return icode_emit(&cp->code, op, a);
}

void xa_patch(compiler *cp, int at, long a) {
	icode_patch(&cp->code, at, a);
}

int xa_str_intern(compiler *cp, const char *s) {
	for (usz i = 0; i < cp->strs.len; i++)
		if (!xstrcmp(cp->strs.it[i], s))
			return (int)i;
	xvec_push(&cp->strs, xstrdup(s));
	return (int)cp->strs.len - 1;
}

int xa_local_idx(fnrec *f, const char *n, int create) {
	for (usz i = 0; i < f->locals.len; i++)
		if (!xstrcmp(f->locals.it[i], n))
			return (int)i;
	if (!create)
		return -1;
	xvec_push(&f->locals, xstrdup(n));
	if ((int)f->locals.len > f->nlocals)
		f->nlocals = (int)f->locals.len;
	return (int)f->locals.len - 1;
}

int xa_global_idx(compiler *cp, const char *n, int create) {
	for (usz i = 0; i < cp->globals.len; i++)
		if (!xstrcmp(cp->globals.it[i], n))
			return (int)i;
	if (!create)
		return -1;
	xvec_push(&cp->globals, xstrdup(n));
	return (int)cp->globals.len - 1;
}

fnrec *xa_fn_find(compiler *cp, const char *n) {
	for (usz i = 0; i < cp->fns.len; i++) {
		fnrec *f = cp->fns.it[i];
		if (!xstrcmp(f->name, n))
			return f;
	}
	return 0;
}

int xa_fn_index(compiler *cp, const char *n) {
	for (usz i = 0; i < cp->fns.len; i++) {
		fnrec *f = cp->fns.it[i];
		if (!xstrcmp(f->name, n))
			return (int)i;
	}
	return -1;
}

fnrec *xa_fn_touch(compiler *cp, const char *n) {
	fnrec *f = xa_fn_find(cp, n);
	if (f)
		return f;
	f = xcalloc(1, sizeof *f);
	f->name = xstrdup(n);
	f->entry = -1;
	xvec_init(&f->locals);
	xvec_push(&cp->fns, f);
	return f;
}

static void call_args(compiler *cp, int *count) {
	int n = 0;
	lex_expect(cp->lx, "(");
	if (!lex_at(cp->lx, ")")) {
		do {
			xa_expr(cp);
			n++;
		} while (lex_accept(cp->lx, ","));
	}
	lex_expect(cp->lx, ")");
	*count = n;
	if (n > XCORE_MAX_ARGS)
		lex_fail(cp->lx, "too many arguments (max 32)");
}

static void emit_call(compiler *cp, char *resolved, int argc) {
	int nat = xcore_nat_find(resolved);
	if (nat >= 0) {
		xa_emit(cp, OP_NCALL, XCORE_PACK(nat, argc));
		return;
	}
	fnrec *f = xa_fn_touch(cp, resolved);
	int fi = xa_fn_index(cp, resolved);
	if (fi > XCORE_MAX_FNS)
		lex_fail(cp->lx, "too many functions");
	(void)f;
	xa_emit(cp, OP_CALL, XCORE_PACK(fi, argc));
}

static void postfix(compiler *cp) {
	while (lex_at(cp->lx, "[")) {
		lex_advance(cp->lx);
		xa_expr(cp);
		lex_expect(cp->lx, "]");
		xa_emit(cp, OP_INDEX, 0);
	}
}

static void primary(compiler *cp) {
	lexer *lx = cp->lx;
	if (lx->cur.k == L_NUM) {
		xa_emit(cp, OP_PUSHI, lx->cur.n);
		lex_advance(lx);
		return;
	}
	if (lx->cur.k == L_STR) {
		xa_emit(cp, OP_PUSHS, xa_str_intern(cp, lx->cur.s));
		lex_advance(lx);
		return;
	}
	if (lex_at(lx, "true")) {
		lex_advance(lx);
		xa_emit(cp, OP_PUSHI, 1);
		return;
	}
	if (lex_at(lx, "false") || lex_at(lx, "nil")) {
		lex_advance(lx);
		xa_emit(cp, OP_PUSHI, 0);
		return;
	}
	if (lex_at(lx, "not")) {
		lex_advance(lx);
		primary(cp);
		xa_emit(cp, OP_NOT, 0);
		return;
	}
	if (lex_accept(lx, "-")) {
		primary(cp);
		xa_emit(cp, OP_NEG, 0);
		return;
	}
	if (lex_accept(lx, "(")) {
		xa_expr(cp);
		lex_expect(lx, ")");
		return;
	}
	if (lex_accept(lx, "[")) {
		xa_emit(cp, OP_NEWLIST, 0);
		if (!lex_at(lx, "]")) {
			do {
				xa_expr(cp);
				xa_emit(cp, OP_APPEND, 0);
			} while (lex_accept(lx, ","));
		}
		lex_expect(lx, "]");
		postfix(cp);
		return;
	}
	if (lx->cur.k == L_ID) {
		xstr full_name;
		xstr_init(&full_name);
		xstr_add(&full_name, lx->cur.s);
		lex_advance(lx);
		while (lex_accept(lx, ".")) {
			if (lx->cur.k != L_ID)
				lex_fail(lx, "expected identifier after '.'");
			xstr_addc(&full_name, '.');
			xstr_add(&full_name, lx->cur.s);
			lex_advance(lx);
		}
		char *resolved = xa_resolve(cp, full_name.p);
		xstr_free(&full_name);

		if (lex_at(lx, "(")) {
			int argc = 0;
			call_args(cp, &argc);
			emit_call(cp, resolved, argc);
			xfree(resolved);
			postfix(cp);
			return;
		}

		int li = cp->cur ? xa_local_idx(cp->cur, resolved, 0) : -1;
		if (li >= 0)
			xa_emit(cp, OP_LOAD, li);
		else
			xa_emit(cp, OP_GLOAD, xa_global_idx(cp, resolved, 1));
		xfree(resolved);
		postfix(cp);
		return;
	}
	lex_fail(lx, "unexpected token in expression");
}

static void mul_expr(compiler *cp) {
	primary(cp);
	for (;;) {
		if (lex_accept(cp->lx, "*")) {
			primary(cp);
			xa_emit(cp, OP_MUL, 0);
		} else if (lex_accept(cp->lx, "/")) {
			primary(cp);
			xa_emit(cp, OP_DIV, 0);
		} else if (lex_accept(cp->lx, "%")) {
			primary(cp);
			xa_emit(cp, OP_MOD, 0);
		} else
			return;
	}
}

static void add_expr(compiler *cp) {
	mul_expr(cp);
	for (;;) {
		if (lex_accept(cp->lx, "+")) {
			mul_expr(cp);
			xa_emit(cp, OP_ADD, 0);
		} else if (lex_accept(cp->lx, "-")) {
			mul_expr(cp);
			xa_emit(cp, OP_SUB, 0);
		} else if (lex_accept(cp->lx, "..")) {
			mul_expr(cp);
			xa_emit(cp, OP_CONCAT, 0);
		} else
			return;
	}
}

static void cmp_expr(compiler *cp) {
	add_expr(cp);
	for (;;) {
		if (lex_accept(cp->lx, "==")) {
			add_expr(cp);
			xa_emit(cp, OP_EQ, 0);
		} else if (lex_accept(cp->lx, "!=")) {
			add_expr(cp);
			xa_emit(cp, OP_NE, 0);
		} else if (lex_accept(cp->lx, "<=")) {
			add_expr(cp);
			xa_emit(cp, OP_LE, 0);
		} else if (lex_accept(cp->lx, ">=")) {
			add_expr(cp);
			xa_emit(cp, OP_GE, 0);
		} else if (lex_accept(cp->lx, "<")) {
			add_expr(cp);
			xa_emit(cp, OP_LT, 0);
		} else if (lex_accept(cp->lx, ">")) {
			add_expr(cp);
			xa_emit(cp, OP_GT, 0);
		} else
			return;
	}
}

static void and_expr(compiler *cp) {
	cmp_expr(cp);
	while (lex_at(cp->lx, "&&") || lex_at(cp->lx, "and")) {
		lex_advance(cp->lx);
		int j = xa_emit(cp, OP_JZ, 0);
		cmp_expr(cp);
		int e = xa_emit(cp, OP_JMP, 0);
		xa_patch(cp, j, (long)cp->code.len);
		xa_emit(cp, OP_PUSHI, 0);
		xa_patch(cp, e, (long)cp->code.len);
	}
}

void xa_expr(compiler *cp) {
	and_expr(cp);
	while (lex_at(cp->lx, "||") || lex_at(cp->lx, "or")) {
		lex_advance(cp->lx);
		int j = xa_emit(cp, OP_JNZ, 0);
		and_expr(cp);
		int e = xa_emit(cp, OP_JMP, 0);
		xa_patch(cp, j, (long)cp->code.len);
		xa_emit(cp, OP_PUSHI, 1);
		xa_patch(cp, e, (long)cp->code.len);
	}
}

static void store_name(compiler *cp, const char *name, int declare) {
	if (cp->cur) {
		int li = xa_local_idx(cp->cur, name, declare);
		if (li >= 0) {
			xa_emit(cp, OP_STORE, li);
			return;
		}
		int gi = xa_global_idx(cp, name, 0);
		if (gi >= 0) {
			xa_emit(cp, OP_GSTORE, gi);
			return;
		}
		li = xa_local_idx(cp->cur, name, 1);
		xa_emit(cp, OP_STORE, li);
		return;
	}
	xa_emit(cp, OP_GSTORE, xa_global_idx(cp, name, 1));
}

static void load_name(compiler *cp, const char *name) {
	int li = cp->cur ? xa_local_idx(cp->cur, name, 0) : -1;
	if (li >= 0)
		xa_emit(cp, OP_LOAD, li);
	else
		xa_emit(cp, OP_GLOAD, xa_global_idx(cp, name, 1));
}

static int tmp_slot(compiler *cp, const char *tag, long n, u32 *storeop, u32 *loadop) {
	xstr t;
	xstr_init(&t);
	xstr_add(&t, "__");
	xstr_add(&t, tag);
	xstr_addl(&t, (long)cp->code.len);
	xstr_addc(&t, '_');
	xstr_addl(&t, n);
	int idx = cp->cur ? xa_local_idx(cp->cur, t.p, 1) : xa_global_idx(cp, t.p, 1);
	if (storeop)
		*storeop = cp->cur ? OP_STORE : OP_GSTORE;
	if (loadop)
		*loadop = cp->cur ? OP_LOAD : OP_GLOAD;
	xstr_free(&t);
	return idx;
}

static void assign_or_call(compiler *cp) {
	lexer *lx = cp->lx;
	xstr full_name;
	xstr_init(&full_name);
	xstr_add(&full_name, lx->cur.s);
	lex_advance(lx);
	while (lex_accept(lx, ".")) {
		if (lx->cur.k != L_ID)
			lex_fail(lx, "expected identifier after '.'");
		xstr_addc(&full_name, '.');
		xstr_add(&full_name, lx->cur.s);
		lex_advance(lx);
	}
	char *name = xa_resolve(cp, full_name.p);
	xstr_free(&full_name);

	if (lex_at(lx, "[")) {
		u32 sop = OP_STORE, lop = OP_LOAD;
		int tidx[16];
		int nidx = 0;
		while (lex_accept(lx, "[")) {
			xa_expr(cp);
			lex_expect(lx, "]");
			if (nidx >= 16)
				lex_fail(lx, "too many subscripts (max 16)");
			tidx[nidx] = tmp_slot(cp, "ix", nidx, &sop, &lop);
			xa_emit(cp, sop, tidx[nidx]);
			nidx++;
		}

		int is_assign = lex_at(lx, "=") || lex_at(lx, "+=") || lex_at(lx, "-=") || lex_at(lx, "*=") || lex_at(lx, "/=");
		if (!is_assign) {
			load_name(cp, name);
			for (int i = 0; i < nidx; i++) {
				xa_emit(cp, lop, tidx[i]);
				xa_emit(cp, OP_INDEX, 0);
			}
			xa_emit(cp, OP_POP, 0);
			xfree(name);
			return;
		}

		char op = lx->cur.s[0];
		int compound = xstrcmp(lx->cur.s, "=") != 0;
		lex_advance(lx);

		if (!compound) {
			load_name(cp, name);
			for (int i = 0; i + 1 < nidx; i++) {
				xa_emit(cp, lop, tidx[i]);
				xa_emit(cp, OP_INDEX, 0);
			}
			xa_emit(cp, lop, tidx[nidx - 1]);
			xa_expr(cp);
			xa_emit(cp, OP_SETIDX, 0);
			xfree(name);
			return;
		}
		int tv = tmp_slot(cp, "tv", 0, &sop, &lop);
		load_name(cp, name);
		for (int i = 0; i < nidx; i++) {
			xa_emit(cp, lop, tidx[i]);
			xa_emit(cp, OP_INDEX, 0);
		}
		xa_expr(cp);
		xa_emit(cp, op == '+' ? OP_ADD : op == '-' ? OP_SUB : op == '*' ? OP_MUL : OP_DIV, 0);
		xa_emit(cp, sop, tv);
		load_name(cp, name);
		for (int i = 0; i + 1 < nidx; i++) {
			xa_emit(cp, lop, tidx[i]);
			xa_emit(cp, OP_INDEX, 0);
		}
		xa_emit(cp, lop, tidx[nidx - 1]);
		xa_emit(cp, lop, tv);
		xa_emit(cp, OP_SETIDX, 0);
		xfree(name);
		return;
	}
	if (lex_at(lx, "=") || lex_at(lx, "+=") || lex_at(lx, "-=") || lex_at(lx, "*=") || lex_at(lx, "/=")) {
		char op = lx->cur.s[0];
		int compound = xstrcmp(lx->cur.s, "=") != 0;
		lex_advance(lx);
		if (compound)
			load_name(cp, name);
		xa_expr(cp);
		if (compound)
			xa_emit(cp, op == '+' ? OP_ADD : op == '-' ? OP_SUB : op == '*' ? OP_MUL : OP_DIV, 0);
		store_name(cp, name, 0);
		xfree(name);
		return;
	}
	if (lex_at(lx, "(")) {
		int argc = 0;
		call_args(cp, &argc);
		emit_call(cp, name, argc);
		xa_emit(cp, OP_POP, 0);
		xfree(name);
		return;
	}
	xfree(name);
	lex_fail(lx, "unexpected statement syntax");
}

void xa_block(compiler *cp) {
	lex_expect(cp->lx, "{");
	while (!lex_at(cp->lx, "}") && cp->lx->cur.k != L_EOF)
		xa_stmt(cp);
	lex_expect(cp->lx, "}");
}

static void stmt_let(compiler *cp, int is_pub) {
	lexer *lx = cp->lx;
	lex_advance(lx);
	if (lx->cur.k != L_ID)
		lex_fail(lx, "expected name after let");
	char *raw_name = xstrdup(lx->cur.s);
	lex_advance(lx);
	char *var_name = 0;
	if (is_pub && cp->cur_module) {
		xstr sc;
		xstr_init(&sc);
		xstr_add(&sc, cp->cur_module);
		xstr_addc(&sc, '.');
		xstr_add(&sc, raw_name);
		var_name = xstr_take(&sc);
		xvec_push(&cp->pub_globals, xstrdup(var_name));
	} else {
		var_name = xstrdup(raw_name);
	}
	xfree(raw_name);

	if (lex_accept(lx, "="))
		xa_expr(cp);
	else
		xa_emit(cp, OP_PUSHI, 0);
	if (cp->cur)
		xa_emit(cp, OP_STORE, xa_local_idx(cp->cur, var_name, 1));
	else
		xa_emit(cp, OP_GSTORE, xa_global_idx(cp, var_name, 1));
	xfree(var_name);
	lex_accept(lx, ";");
}

static void stmt_if(compiler *cp) {
	lexer *lx = cp->lx;
	lex_advance(lx);
	xa_expr(cp);
	int j = xa_emit(cp, OP_JZ, 0);
	xa_block(cp);
	if (lex_at(lx, "else")) {
		lex_advance(lx);
		int e = xa_emit(cp, OP_JMP, 0);
		xa_patch(cp, j, (long)cp->code.len);
		if (lex_at(lx, "if"))
			xa_stmt(cp);
		else
			xa_block(cp);
		xa_patch(cp, e, (long)cp->code.len);
	} else
		xa_patch(cp, j, (long)cp->code.len);
}

static void stmt_case(compiler *cp) {
	lexer *lx = cp->lx;
	lex_advance(lx);
	xa_expr(cp);

	xstr t;
	xstr_init(&t);
	xstr_add(&t, "__case_val");
	xstr_addl(&t, (long)cp->code.len);
	int cvi = cp->cur ? xa_local_idx(cp->cur, t.p, 1) : xa_global_idx(cp, t.p, 1);
	u32 listop = cp->cur ? OP_STORE : OP_GSTORE;
	u32 loadop = cp->cur ? OP_LOAD : OP_GLOAD;
	xa_emit(cp, listop, cvi);
	xstr_free(&t);

	lex_expect(lx, "{");
	xvec end_jumps;
	xvec_init(&end_jumps);

	while (!lex_at(lx, "}") && lx->cur.k != L_EOF) {
		if (lex_at(lx, "default")) {
			lex_advance(lx);
			xa_block(cp);
			long *je = xmalloc(sizeof(long));
			*je = xa_emit(cp, OP_JMP, 0);
			xvec_push(&end_jumps, je);
			continue;
		}
		xvec cond_jumps;
		xvec_init(&cond_jumps);
		do {
			xa_emit(cp, loadop, cvi);
			xa_expr(cp);
			xa_emit(cp, OP_EQ, 0);
			long *jm = xmalloc(sizeof(long));
			*jm = xa_emit(cp, OP_JNZ, 0);
			xvec_push(&cond_jumps, jm);
		} while (lex_accept(lx, ","));

		int jskip = xa_emit(cp, OP_JMP, 0);
		for (usz i = 0; i < cond_jumps.len; i++) {
			long *jm = cond_jumps.it[i];
			xa_patch(cp, (int)*jm, (long)cp->code.len);
			xfree(jm);
		}
		xvec_free(&cond_jumps);

		xa_block(cp);
		long *je = xmalloc(sizeof(long));
		*je = xa_emit(cp, OP_JMP, 0);
		xvec_push(&end_jumps, je);
		xa_patch(cp, jskip, (long)cp->code.len);
	}
	lex_expect(lx, "}");
	for (usz i = 0; i < end_jumps.len; i++) {
		long *je = end_jumps.it[i];
		xa_patch(cp, (int)*je, (long)cp->code.len);
		xfree(je);
	}
	xvec_free(&end_jumps);
}

static void loop_pop_jumps(compiler *cp, usz bsave, usz csave, int top, int contpt) {
	while (cp->breaks.len > bsave) {
		long *b = xvec_pop(&cp->breaks);
		xa_patch(cp, (int)*b, (long)cp->code.len);
		xfree(b);
	}
	while (cp->conts.len > csave) {
		long *b = xvec_pop(&cp->conts);
		xa_patch(cp, (int)*b, contpt < 0 ? top : contpt);
		xfree(b);
	}
}

static void stmt_while(compiler *cp) {
	lex_advance(cp->lx);
	int top = (int)cp->code.len;
	xa_expr(cp);
	int j = xa_emit(cp, OP_JZ, 0);
	usz bsave = cp->breaks.len, csave = cp->conts.len;
	cp->loopdepth++;
	xa_block(cp);
	cp->loopdepth--;
	xa_emit(cp, OP_JMP, top);
	xa_patch(cp, j, (long)cp->code.len);
	loop_pop_jumps(cp, bsave, csave, top, -1);
}

static void stmt_for(compiler *cp) {
	lexer *lx = cp->lx;
	lex_advance(lx);
	if (lx->cur.k != L_ID)
		lex_fail(lx, "expected loop variable");
	char *var = xstrdup(lx->cur.s);
	lex_advance(lx);
	lex_expect(lx, "in");
	xa_expr(cp);

	xstr t;
	xstr_init(&t);
	xstr_add(&t, "__it");
	xstr_addl(&t, (long)cp->code.len);
	int vi = cp->cur ? xa_local_idx(cp->cur, t.p, 1) : xa_global_idx(cp, t.p, 1);
	u32 listop = cp->cur ? OP_STORE : OP_GSTORE;
	u32 loadop = cp->cur ? OP_LOAD : OP_GLOAD;
	xa_emit(cp, listop, vi);
	xstr_clear(&t);
	xstr_add(&t, "__ix");
	xstr_addl(&t, (long)cp->code.len);
	int ii = cp->cur ? xa_local_idx(cp->cur, t.p, 1) : xa_global_idx(cp, t.p, 1);
	xa_emit(cp, OP_PUSHI, 0);
	xa_emit(cp, listop, ii);

	int top = (int)cp->code.len;
	xa_emit(cp, loadop, ii);
	xa_emit(cp, loadop, vi);
	xa_emit(cp, OP_LEN, 0);
	xa_emit(cp, OP_LT, 0);
	int j = xa_emit(cp, OP_JZ, 0);
	xa_emit(cp, loadop, vi);
	xa_emit(cp, loadop, ii);
	xa_emit(cp, OP_INDEX, 0);
	int uv = cp->cur ? xa_local_idx(cp->cur, var, 1) : xa_global_idx(cp, var, 1);
	xa_emit(cp, listop, uv);

	usz bsave = cp->breaks.len, csave = cp->conts.len;
	cp->loopdepth++;
	xa_block(cp);
	cp->loopdepth--;

	int contpt = (int)cp->code.len;
	xa_emit(cp, loadop, ii);
	xa_emit(cp, OP_PUSHI, 1);
	xa_emit(cp, OP_ADD, 0);
	xa_emit(cp, listop, ii);
	xa_emit(cp, OP_JMP, top);
	xa_patch(cp, j, (long)cp->code.len);
	loop_pop_jumps(cp, bsave, csave, top, contpt);

	xstr_free(&t);
	xfree(var);
}

void xa_stmt(compiler *cp) {
	lexer *lx = cp->lx;
	if (lex_accept(lx, ";"))
		return;
	int is_pub = 0;
	if (lex_at(lx, "pub")) {
		lex_advance(lx);
		is_pub = 1;
	}
	if (lex_at(lx, "let")) {
		stmt_let(cp, is_pub);
		return;
	}
	if (is_pub)
		lex_fail(lx, "expected 'fn' or 'let' after 'pub'");
	if (lex_at(lx, "if")) {
		stmt_if(cp);
		return;
	}
	if (lex_at(lx, "case")) {
		stmt_case(cp);
		return;
	}
	if (lex_at(lx, "while")) {
		stmt_while(cp);
		return;
	}
	if (lex_at(lx, "for")) {
		stmt_for(cp);
		return;
	}
	if (lex_at(lx, "return")) {
		lex_advance(lx);
		if (lex_at(lx, ";") || lex_at(lx, "}"))
			xa_emit(cp, OP_PUSHI, 0);
		else
			xa_expr(cp);
		xa_emit(cp, OP_RET, 0);
		lex_accept(lx, ";");
		return;
	}
	if (lex_at(lx, "break")) {
		if (!cp->loopdepth)
			lex_fail(lx, "break outside of a loop");
		lex_advance(lx);
		long *b = xmalloc(sizeof *b);
		*b = xa_emit(cp, OP_JMP, 0);
		xvec_push(&cp->breaks, b);
		lex_accept(lx, ";");
		return;
	}
	if (lex_at(lx, "continue")) {
		if (!cp->loopdepth)
			lex_fail(lx, "continue outside of a loop");
		lex_advance(lx);
		long *b = xmalloc(sizeof *b);
		*b = xa_emit(cp, OP_JMP, 0);
		xvec_push(&cp->conts, b);
		lex_accept(lx, ";");
		return;
	}
	if (lex_at(lx, "{")) {
		xa_block(cp);
		return;
	}
	if (lx->cur.k == L_ID) {
		assign_or_call(cp);
		lex_accept(lx, ";");
		return;
	}
	xa_expr(cp);
	xa_emit(cp, OP_POP, 0);
	lex_accept(lx, ";");
}
