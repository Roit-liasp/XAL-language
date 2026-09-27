#ifndef XVM_H
#define XVM_H

#include "xcore.h"
#include "xc.h"

typedef enum { V_INT, V_STR, V_LIST } vkind;

typedef struct list list;

typedef struct {
	vkind k;
	long i;
	char *s;
	list *l;
} val;

struct list {
	val *it;
	usz len, cap;
	int gc_mark;
	list *gc_next;
};

val vint(long i);
val vstr(char *owned);
val vstr_copy(const char *s);
val vlist(list *l);

list *list_new(void);
void list_push(list *l, val v);

long truth(val v);
char *tostr(val v);
long toint(val v);

extern int gc_enabled;
void gc_maybe(void);
void gc_collect(void);
long gc_count(void);
long gc_bytes(void);

#define VM_MAX_FRAMES 512

typedef struct {
	u32 ret;
	val *locals;
	u32 nlocals;
} frame;

typedef struct vm_thread {
	xos_thr *os;
	int finished;
	val result;
	val *stack;
	usz sp, stack_cap;
	frame frames[VM_MAX_FRAMES];
	int fdepth;
	long id;
	u32 fnidx;
	list *args;
	struct vm_thread *next;
} vm_thread;

#if XOS_HAS_THREADS && defined(__GNUC__)
#define VM_TLS __thread
#else
#define VM_TLS
#endif
extern VM_TLS vm_thread *vm_cur;
extern vm_thread *vm_threads;
extern xos_mtx *vm_lock;

vm_thread *thread_new(u32 fnidx, list *args);
long thread_spawn(vm_thread *t);
val thread_join(long id);
void thread_register(vm_thread *t);
void thread_unregister(vm_thread *t);
vm_thread *thread_by_id(long id);

typedef struct chan chan;
long chan_new(void);
int chan_send(long id, val v);
val chan_recv(long id, int block, int *ok);
int chan_close(long id);
void chan_mark_all(void (*mark)(val));

extern xcore_image *vm_image;
extern val *vm_globals;
extern u32 vm_nglobals;
extern int vm_argc;
extern char **vm_argv;

void push(val v);
val pop(void);
int vm_run(vm_thread *t, u32 fnidx, list *args);
void vmdie(const char *msg);

int xvm_run_image(xcore_image *im, int argc, char **argv);
int xvm_run_mem(const u8 *buf, usz len, int argc, char **argv);

void *vm_state_save(void);
void vm_state_restore(void *p);

int xvm_run_nested(const u8 *buf, usz len, int argc, char **argv);
extern int xvm_nest_depth;
extern int xvm_exit_req;
extern int xvm_exit_code;
extern const char *xvm_self_path;

u32 xvm_caps(void);
void xvm_caps_set(u32 mask);
void xvm_caps_enable(u32 mask);
void xvm_caps_disable(u32 mask);
u32 xvm_image_caps(const xcore_image *im);
const char *xvm_cap_name(u32 bit);
void xvm_caps_format(u32 mask, char *buf, usz n);
int xvm_caps_check(const xcore_image *im, char *buf, usz n);
extern int xvm_caps_strict;
extern u32 xvm_caps_missing;
extern const u32 xvm_nat_caps[NAT__COUNT];
void xvm_nat_unsupported(int id);

val native_call(int id, val *args, int n);

#endif
