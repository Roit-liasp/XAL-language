#include "xvm.h"

VM_TLS vm_thread *vm_cur;

vm_thread *vm_threads;
xos_mtx *vm_lock;

static long next_id = 1;

void thread_register(vm_thread *t) {
	xos_mtx_lock(vm_lock);
	t->next = vm_threads;
	vm_threads = t;
	xos_mtx_unlock(vm_lock);
}

void thread_unregister(vm_thread *t) {
	xos_mtx_lock(vm_lock);
	vm_thread **pp = &vm_threads;
	while (*pp && *pp != t)
		pp = &(*pp)->next;
	if (*pp)
		*pp = t->next;
	xos_mtx_unlock(vm_lock);
}

vm_thread *thread_by_id(long id) {
	xos_mtx_lock(vm_lock);
	vm_thread *t = vm_threads;
	while (t && t->id != id)
		t = t->next;
	xos_mtx_unlock(vm_lock);
	return t;
}

vm_thread *thread_new(u32 fnidx, list *args) {
	vm_thread *t = xcalloc(1, sizeof *t);
	t->stack_cap = 256;
	t->stack = xmalloc(t->stack_cap * sizeof(val));
	t->result = vint(0);
	t->fnidx = fnidx;
	t->args = args;
	xos_mtx_lock(vm_lock);
	t->id = next_id++;
	xos_mtx_unlock(vm_lock);
	return t;
}

static void thread_body(void *arg) {
	vm_thread *t = arg;
	vm_thread *saved = vm_cur;
	vm_cur = t;
	int rc = vm_run(t, t->fnidx, t->args);
	t->result = t->sp ? t->stack[t->sp - 1] : vint(rc);
	t->finished = 1;
	vm_cur = saved;
}

long thread_spawn(vm_thread *t) {
	thread_register(t);
#if XOS_HAS_THREADS
	t->os = xos_thread_create(thread_body, t);
	if (t->os)
		return t->id;
#endif
	thread_body(t);
	return t->id;
}

val thread_join(long id) {
	vm_thread *t = thread_by_id(id);
	if (!t || t == vm_cur)
		return vint(0);
#if XOS_HAS_THREADS
	if (t->os) {
		xos_thread_join(t->os);
		t->os = 0;
	}
#endif
	val r = t->result;
	thread_unregister(t);
	xfree(t->stack);
	xfree(t);
	return r;
}

typedef struct chan_msg {
	val v;
	struct chan_msg *next;
} chan_msg;

struct chan {
	long id;
	xos_mtx *lock;
	xos_cv *cv;
	chan_msg *head, *tail;
	int closed;
	struct chan *next;
};

static chan *chans;
static long next_chan = 1;

static chan *chan_find(long id) {
	xos_mtx_lock(vm_lock);
	chan *c = chans;
	while (c && c->id != id)
		c = c->next;
	xos_mtx_unlock(vm_lock);
	return c;
}

long chan_new(void) {
	chan *c = xcalloc(1, sizeof *c);
	c->lock = xos_mtx_new();
	c->cv = xos_cv_new();
	xos_mtx_lock(vm_lock);
	c->id = next_chan++;
	c->next = chans;
	chans = c;
	xos_mtx_unlock(vm_lock);
	return c->id;
}

int chan_send(long id, val v) {
	chan *c = chan_find(id);
	if (!c)
		return -1;
	xos_mtx_lock(c->lock);
	if (c->closed) {
		xos_mtx_unlock(c->lock);
		return -1;
	}
	chan_msg *m = xcalloc(1, sizeof *m);
	m->v = v;
	if (c->tail)
		c->tail->next = m;
	else
		c->head = m;
	c->tail = m;
	xos_cv_signal(c->cv);
	xos_mtx_unlock(c->lock);
	return 0;
}

val chan_recv(long id, int block, int *ok) {
	*ok = 0;
	chan *c = chan_find(id);
	if (!c)
		return vint(0);
	xos_mtx_lock(c->lock);
#if XOS_HAS_THREADS
	while (block && !c->head && !c->closed)
		xos_cv_wait(c->cv, c->lock);
#else
	(void)block;
#endif
	if (!c->head) {
		xos_mtx_unlock(c->lock);
		return vint(0);
	}
	chan_msg *m = c->head;
	c->head = m->next;
	if (!c->head)
		c->tail = 0;
	val v = m->v;
	xfree(m);
	xos_mtx_unlock(c->lock);
	*ok = 1;
	return v;
}

int chan_close(long id) {
	chan *c = chan_find(id);
	if (!c)
		return -1;
	xos_mtx_lock(c->lock);
	c->closed = 1;
	xos_cv_broadcast(c->cv);
	xos_mtx_unlock(c->lock);
	return 0;
}

void chan_mark_all(void (*mark)(val)) {
	for (chan *c = chans; c; c = c->next)
		for (chan_msg *m = c->head; m; m = m->next)
			mark(m->v);
}
