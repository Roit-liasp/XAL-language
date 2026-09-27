#ifndef XCORE_H
#define XCORE_H

#include "xos.h"

#define XAL_MAGIC "XALB"
#define XAL_FMT_VERSION 1u

#define XCORE_OPS(X)                    \
	X(OP_NOP, "nop")                    \
	X(OP_PUSHI, "pushi")                \
	X(OP_PUSHS, "pushs")                \
	X(OP_LOAD, "load")                  \
	X(OP_STORE, "store")                \
	X(OP_GLOAD, "gload")                \
	X(OP_GSTORE, "gstore")              \
	X(OP_ADD, "add")                    \
	X(OP_SUB, "sub")                    \
	X(OP_MUL, "mul")                    \
	X(OP_DIV, "div")                    \
	X(OP_MOD, "mod")                    \
	X(OP_NEG, "neg")                    \
	X(OP_EQ, "eq")                      \
	X(OP_NE, "ne")                      \
	X(OP_LT, "lt")                      \
	X(OP_LE, "le")                      \
	X(OP_GT, "gt")                      \
	X(OP_GE, "ge")                      \
	X(OP_NOT, "not")                    \
	X(OP_JMP, "jmp")                    \
	X(OP_JZ, "jz")                      \
	X(OP_JNZ, "jnz")                    \
	X(OP_CALL, "call")                  \
	X(OP_NCALL, "ncall")                \
	X(OP_RET, "ret")                    \
	X(OP_POP, "pop")                    \
	X(OP_DUP, "dup")                    \
	X(OP_NEWLIST, "newlist")            \
	X(OP_INDEX, "index")                \
	X(OP_SETIDX, "setidx")              \
	X(OP_APPEND, "append")              \
	X(OP_LEN, "len")                    \
	X(OP_CONCAT, "concat")              \
	X(OP_HALT, "halt")

enum {
#define X_ENUM(n, s) n,
	XCORE_OPS(X_ENUM)
#undef X_ENUM
	OP__COUNT
};

#define XCORE_NATS(X)                   \
	X(NAT_PRINT, "print")               \
	X(NAT_PUTS, "puts")                 \
	X(NAT_LEN, "len")                   \
	X(NAT_STR, "str")                   \
	X(NAT_INT, "int")                   \
	X(NAT_READ, "read")                 \
	X(NAT_WRITE, "write")               \
	X(NAT_OPEN, "open")                 \
	X(NAT_CLOSE, "close")               \
	X(NAT_EXEC, "exec")                 \
	X(NAT_SPAWN, "spawn")               \
	X(NAT_WAIT, "wait")                 \
	X(NAT_EXIT, "exit")                 \
	X(NAT_GETENV, "getenv")             \
	X(NAT_SETENV, "setenv")             \
	X(NAT_CHDIR, "chdir")               \
	X(NAT_GETCWD, "getcwd")             \
	X(NAT_EXISTS, "exists")             \
	X(NAT_ISDIR, "isdir")               \
	X(NAT_MTIME, "mtime")               \
	X(NAT_MKDIRP, "mkdirp")             \
	X(NAT_UNLINK, "unlink")             \
	X(NAT_READDIR, "readdir")           \
	X(NAT_READFILE, "readfile")         \
	X(NAT_WRITEFILE, "writefile")       \
	X(NAT_SPLIT, "split")               \
	X(NAT_JOIN, "join")                 \
	X(NAT_SUBSTR, "substr")             \
	X(NAT_FIND, "find")                 \
	X(NAT_ARGV, "argv")                 \
	X(NAT_ARGC, "argc")                 \
	X(NAT_SYS, "sys")                   \
	X(NAT_EQ, "eq")                     \
	X(NAT_ORD, "ord")                   \
	X(NAT_CHR, "chr")                   \
	X(NAT_RENAME, "rename")             \
	X(NAT_ISEXEC, "isexec")             \
	X(NAT_INPUT, "input")               \
	X(NAT_FORK, "fork")                 \
	X(NAT_EXECV, "execv")               \
	X(NAT_DUP2, "dup2")                 \
	X(NAT_PIPE, "pipe")                 \
	X(NAT_TRIM, "trim")                 \
	X(NAT_UPPER, "upper")               \
	X(NAT_LOWER, "lower")               \
	X(NAT_WHICH, "which")               \
	X(NAT_SIGNAL, "signal")             \
	X(NAT_SLEEP, "sleep")               \
	X(NAT_EOF, "eof")                   \
	X(NAT_RAWMODE, "rawmode")           \
	X(NAT_GETKEY, "getkey")             \
	X(NAT_TERMSIZE, "termsize")         \
	X(NAT_ISATTY, "isatty")             \
	X(NAT_SORT, "sort")                 \
	X(NAT_COPY, "copy")                 \
	X(NAT_INSERT, "insert")             \
	X(NAT_REMOVE, "remove")             \
	X(NAT_REPLACE, "replace")           \
	X(NAT_STARTSWITH, "startswith")     \
	X(NAT_ENDSWITH, "endswith")         \
	X(NAT_REPEAT, "repeat")             \
	X(NAT_ABS, "abs")                   \
	X(NAT_MIN, "min")                   \
	X(NAT_MAX, "max")                   \
	X(NAT_NOW, "now")                   \
	X(NAT_SPAWN_THREAD, "spawn_thread") \
	X(NAT_JOIN_THREAD, "join_thread")   \
	X(NAT_THREAD_ID, "thread_id")       \
	X(NAT_THREAD_CPU, "thread_cpu")     \
	X(NAT_SET_AFFINITY, "set_affinity") \
	X(NAT_NPROC, "nproc")               \
	X(NAT_CPU_COUNT, "cpu_count")       \
	X(NAT_CHAN_NEW, "chan_new")         \
	X(NAT_CHAN_SEND, "chan_send")       \
	X(NAT_CHAN_RECV, "chan_recv")       \
	X(NAT_CHAN_TRYRECV, "chan_tryrecv") \
	X(NAT_CHAN_CLOSE, "chan_close")     \
	X(NAT_PARALLEL_MAP, "parallel_map") \
	X(NAT_GC_COLLECT, "gc_collect")     \
	X(NAT_GC_COUNT, "gc_count")         \
	X(NAT_GC_STAT, "gc_stat")           \
	X(NAT_RUN_XB, "run_xb")             \
	X(NAT_GETPID, "getpid")             \
	X(NAT_KILL, "kill")                 \
	X(NAT_WAITPID, "waitpid")           \
	X(NAT_CAPS, "caps")                 \
	X(NAT_HAS_CAP, "has_cap")

#define XVM_CAP_NONE    0x0000u
#define XVM_CAP_IO      0x0001u
#define XVM_CAP_FS      0x0002u
#define XVM_CAP_DIR     0x0004u
#define XVM_CAP_ENV     0x0008u
#define XVM_CAP_CWD     0x0010u
#define XVM_CAP_PROC    0x0020u
#define XVM_CAP_KILL    0x0040u
#define XVM_CAP_FORK    0x0080u
#define XVM_CAP_PIPE    0x0100u
#define XVM_CAP_SIGNAL  0x0200u
#define XVM_CAP_TERM    0x0400u
#define XVM_CAP_THREADS 0x0800u
#define XVM_CAP_TIME    0x1000u
#define XVM_CAP_XB      0x2000u
#define XVM_CAP_UNKNOWN 0x8000u

enum {
#define X_ENUM(n, s) n,
	XCORE_NATS(X_ENUM)
#undef X_ENUM
	NAT__COUNT
};

extern const char *const xcore_op_name[OP__COUNT];
extern const char *const xcore_nat_name[NAT__COUNT];
int xcore_nat_find(const char *name);

#define XCORE_MAX_ARGS 32
#define XCORE_PACK(idx, argc) ((long)(u32)(idx) | ((long)(u32)(argc) << 16))
#define XCORE_IDX(a) ((u32)((a) & 0xffff))
#define XCORE_ARGC(a) ((u32)(((a) >> 16) & 0xffff))
#define XCORE_MAX_FNS 0xffff

typedef struct {
	u32 op;
	long a;
} xcore_insn;

typedef struct {
	u32 entry, nparams, nlocals, nameidx;
} xcore_fn;

typedef struct {
	u32 version;
	u32 nstrs;
	char **strs;
	u32 nfns;
	xcore_fn *fns;
	u32 nglobals, mainidx, initidx;
	u32 ncode;
	xcore_insn *code;
} xcore_image;

u8 *xcore_image_write(const xcore_image *im, usz *outlen);

int xcore_image_read(const u8 *buf, usz len, xcore_image *out);
const char *xcore_image_err(void);

void xcore_image_free(xcore_image *im);

#endif
