#include "xal.h"

static const char *keywords[] = {
	"fn", "let", "if", "else", "while", "for", "return", "break", "continue",
	"true", "false", "nil", "and", "or", "not", "use", "in",
	"module", "import", "pub", "case", "default", 0
};

static int is_kw(const char *s) {
	for (int i = 0; keywords[i]; i++)
		if (!xstrcmp(keywords[i], s))
			return 1;
	return 0;
}

static int idc0(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int idc(char c) {
	return idc0(c) || (c >= '0' && c <= '9');
}

void lex_fail(lexer *lx, const char *msg) {
	xfmt(2, "xal: %s:%l: %s\n", lx->file, lx->line, msg);
	xos_exit(1);
}

static token lex_scan(lexer *lx) {
	token t;
	t.s = 0;
	t.n = 0;
	const char *s = lx->src;
	for (;;) {
		while (s[lx->pos] == ' ' || s[lx->pos] == '\t' || s[lx->pos] == '\r' || s[lx->pos] == '\n') {
			if (s[lx->pos] == '\n')
				lx->line++;
			lx->pos++;
		}
		if (s[lx->pos] == '#' || (s[lx->pos] == '/' && s[lx->pos + 1] == '/')) {
			while (s[lx->pos] && s[lx->pos] != '\n')
				lx->pos++;
			continue;
		}
		break;
	}
	char c = s[lx->pos];
	if (!c) {
		t.k = L_EOF;
		t.s = xstrdup("");
		return t;
	}
	if (idc0(c)) {
		usz start = lx->pos;
		while (idc(s[lx->pos]))
			lx->pos++;
		t.s = xstrndup(s + start, lx->pos - start);
		t.k = is_kw(t.s) ? L_KW : L_ID;
		return t;
	}
	if (c >= '0' && c <= '9') {
		long v = 0;
		if (c == '0' && (s[lx->pos + 1] == 'x' || s[lx->pos + 1] == 'X')) {
			lx->pos += 2;
			for (;;) {
				char h = s[lx->pos];
				int d;
				if (h >= '0' && h <= '9')
					d = h - '0';
				else if (h >= 'a' && h <= 'f')
					d = h - 'a' + 10;
				else if (h >= 'A' && h <= 'F')
					d = h - 'A' + 10;
				else
					break;
				v = v * 16 + d;
				lx->pos++;
			}
		} else {
			while (s[lx->pos] >= '0' && s[lx->pos] <= '9')
				v = v * 10 + (s[lx->pos++] - '0');
		}
		t.k = L_NUM;
		t.n = v;
		t.s = xstrdup("num");
		return t;
	}
	if (c == '"' || c == '\'') {
		char q = c;
		lx->pos++;
		xstr b;
		xstr_init(&b);
		while (s[lx->pos] && s[lx->pos] != q) {
			if (s[lx->pos] == '\\' && s[lx->pos + 1]) {
				lx->pos++;
				char e = s[lx->pos++];
				xstr_addc(&b, e == 'n' ? '\n' : e == 't' ? '\t' : e == '0' ? '\0' : e == 'r' ? '\r' : e);
				continue;
			}
			if (s[lx->pos] == '\n')
				lx->line++;
			xstr_addc(&b, s[lx->pos++]);
		}
		if (!s[lx->pos])
			lex_fail(lx, "unterminated string literal");
		lx->pos++;
		t.k = L_STR;
		t.s = xstr_take(&b);
		t.n = 0;
		return t;
	}
	static const char *ops[] = { "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/=", "..", 0 };
	for (int i = 0; ops[i]; i++)
		if (!xstrncmp(s + lx->pos, ops[i], 2)) {
			lx->pos += 2;
			t.k = L_PUNCT;
			t.s = xstrdup(ops[i]);
			return t;
		}
	lx->pos++;
	char buf[2] = { c, 0 };
	t.k = L_PUNCT;
	t.s = xstrdup(buf);
	return t;
}

void lex_init(lexer *lx, const char *src, const char *file) {
	lx->src = src;
	lx->pos = 0;
	lx->line = 1;
	lx->file = file;
	lx->cur = lex_scan(lx);
}

void lex_free(lexer *lx) {
	xfree(lx->cur.s);
	lx->cur.s = 0;
}

void lex_advance(lexer *lx) {
	xfree(lx->cur.s);
	lx->cur = lex_scan(lx);
}

int lex_at(lexer *lx, const char *s) {
	return (lx->cur.k == L_PUNCT || lx->cur.k == L_KW) && !xstrcmp(lx->cur.s, s);
}

int lex_accept(lexer *lx, const char *s) {
	if (lex_at(lx, s)) {
		lex_advance(lx);
		return 1;
	}
	return 0;
}

void lex_expect(lexer *lx, const char *s) {
	if (!lex_accept(lx, s)) {
		xstr m;
		xstr_init(&m);
		xstr_add(&m, "expected '");
		xstr_add(&m, s);
		xstr_add(&m, "' near '");
		xstr_add(&m, lx->cur.s);
		xstr_add(&m, "'");
		lex_fail(lx, m.p);
	}
}
