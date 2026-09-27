#ifndef XAL_H
#define XAL_H

#include "xcore.h"
#include "xc.h"

typedef enum { L_EOF, L_ID, L_NUM, L_STR, L_PUNCT, L_KW } lkind;

typedef struct {
	lkind k;
	char *s;
	long n;
} token;

typedef struct {
	const char *src;
	usz pos;
	long line;
	token cur;
	const char *file;
} lexer;

void lex_init(lexer *lx, const char *src, const char *file);
void lex_free(lexer *lx);
void lex_advance(lexer *lx);
int lex_at(lexer *lx, const char *s);
int lex_accept(lexer *lx, const char *s);
void lex_expect(lexer *lx, const char *s);
void lex_fail(lexer *lx, const char *msg);

typedef struct {
	xcore_insn *v;
	usz len, cap;
} icode;

void icode_init(icode *c);
int icode_emit(icode *c, u32 op, long a);
void icode_patch(icode *c, int at, long a);

typedef struct {
	char *name;
	int nparams;
	int nlocals;
	int entry;
	xvec locals;
} fnrec;

typedef struct compiler compiler;

struct compiler {
	icode code;
	xvec strs;
	xvec fns;
	xvec globals;
	fnrec *cur;
	lexer *lx;
	xvec breaks, conts;
	int loopdepth;
	char *cur_module;
	char *cur_dir;
	xvec pub_fns;
	xvec pub_globals;
	xvec imported_mods;
	xvec imported_files;
	const char *main_file;
	int init_entry;
};

void xa_expr(compiler *cp);
void xa_stmt(compiler *cp);
void xa_block(compiler *cp);
int xa_emit(compiler *cp, u32 op, long a);
void xa_patch(compiler *cp, int at, long a);
int xa_str_intern(compiler *cp, const char *s);
int xa_local_idx(fnrec *f, const char *n, int create);
int xa_global_idx(compiler *cp, const char *n, int create);
fnrec *xa_fn_find(compiler *cp, const char *n);
int xa_fn_index(compiler *cp, const char *n);
fnrec *xa_fn_touch(compiler *cp, const char *n);

void xa_function(compiler *cp, int is_pub);
void xa_import(compiler *cp, const char *modname);
char *xa_resolve(compiler *cp, const char *name);
void xa_compile_source(compiler *cp, const char *src, const char *filename);
void xa_stmt_pub_let(compiler *cp);
void xa_compiler_init(compiler *cp, const char *main_file);

int xa_write_image(compiler *cp, int initidx, const char *outpath);
int xa_dump_image(const char *path);

#endif
