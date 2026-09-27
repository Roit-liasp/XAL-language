/*
 xcore.c - implementation of the shared command core.
*/

#include "xcore.h"

const char *const xcore_op_name[OP__COUNT] = {
#define X_NAME(n, s) s,
	XCORE_OPS(X_NAME)
#undef X_NAME
};

const char *const xcore_nat_name[NAT__COUNT] = {
#define X_NAME(n, s) s,
	XCORE_NATS(X_NAME)
#undef X_NAME
};

static int xc_strlen(const char *s) {
	int n = 0;
	while (s[n])
		n++;
	return n;
}

static int xc_streq(const char *a, const char *b) {
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

int xcore_nat_find(const char *name) {
	for (int i = 0; i < NAT__COUNT; i++)
		if (xc_streq(xcore_nat_name[i], name))
			return i;
	return -1;
}

typedef struct {
	u8 *p;
	usz len, cap;
	int oom;
} wbuf;

static void wneed(wbuf *w, usz n) {
	if (w->oom)
		return;
	if (w->len + n <= w->cap)
		return;
	usz cap = w->cap ? w->cap : 256;
	while (cap < w->len + n)
		cap *= 2;
	u8 *q = (u8 *)xos_realloc(w->p, cap);
	if (!q) {
		w->oom = 1;
		return;
	}
	w->p = q;
	w->cap = cap;
}

static void wput(wbuf *w, const void *src, usz n) {
	wneed(w, n);
	if (w->oom)
		return;
	const u8 *s = (const u8 *)src;
	for (usz i = 0; i < n; i++)
		w->p[w->len + i] = s[i];
	w->len += n;
}

static void wu32(wbuf *w, u32 v) {
	u8 b[4] = { (u8)(v & 255), (u8)((v >> 8) & 255), (u8)((v >> 16) & 255), (u8)((v >> 24) & 255) };
	wput(w, b, 4);
}

static void wi64(wbuf *w, long v) {
	unsigned long u = (unsigned long)v;
	u8 b[8];
	for (int i = 0; i < 8; i++)
		b[i] = (u8)((u >> (i * 8)) & 255);
	wput(w, b, 8);
}

u8 *xcore_image_write(const xcore_image *im, usz *outlen) {
	wbuf w;
	w.p = 0;
	w.len = w.cap = 0;
	w.oom = 0;

	wput(&w, XAL_MAGIC, 4);
	wu32(&w, im->version ? im->version : XAL_FMT_VERSION);
	wu32(&w, im->nstrs);
	for (u32 i = 0; i < im->nstrs; i++) {
		u32 l = (u32)xc_strlen(im->strs[i]);
		wu32(&w, l);
		wput(&w, im->strs[i], l);
	}
	wu32(&w, im->nfns);
	for (u32 i = 0; i < im->nfns; i++) {
		wu32(&w, im->fns[i].entry);
		wu32(&w, im->fns[i].nparams);
		wu32(&w, im->fns[i].nlocals);
		wu32(&w, im->fns[i].nameidx);
	}
	wu32(&w, im->nglobals);
	wu32(&w, im->mainidx);
	wu32(&w, im->initidx);
	wu32(&w, im->ncode);
	for (u32 i = 0; i < im->ncode; i++) {
		wu32(&w, im->code[i].op);
		wi64(&w, im->code[i].a);
	}
	if (w.oom) {
		xos_free(w.p);
		return 0;
	}
	if (outlen)
		*outlen = w.len;
	return w.p;
}

static const char *rerr = 0;

const char *xcore_image_err(void) {
	return rerr ? rerr : "ok";
}

typedef struct {
	const u8 *p;
	usz len, pos;
	int err;
} rbuf;

static int rneed(rbuf *r, usz n) {
	if (r->err)
		return 0;
	if (n > r->len || r->pos + n > r->len) {
		r->err = 1;
		return 0;
	}
	return 1;
}

static u32 ru32(rbuf *r) {
	if (!rneed(r, 4))
		return 0;
	u32 v = (u32)r->p[r->pos] | ((u32)r->p[r->pos + 1] << 8) | ((u32)r->p[r->pos + 2] << 16) |
	        ((u32)r->p[r->pos + 3] << 24);
	r->pos += 4;
	return v;
}

static long ri64(rbuf *r) {
	if (!rneed(r, 8))
		return 0;
	unsigned long v = 0;
	for (int i = 7; i >= 0; i--)
		v = (v << 8) | r->p[r->pos + (usz)i];
	r->pos += 8;
	return (long)v;
}

static const u8 *rtake(rbuf *r, usz n) {
	if (!rneed(r, n))
		return 0;
	const u8 *q = r->p + r->pos;
	r->pos += n;
	return q;
}

static char *rdup(const u8 *s, usz n) {
	char *d = (char *)xos_alloc(n + 1);
	if (!d)
		return 0;
	for (usz i = 0; i < n; i++)
		d[i] = (char)s[i];
	d[n] = 0;
	return d;
}

int xcore_image_read(const u8 *buf, usz len, xcore_image *out) {
	rerr = 0;
	if (out) {
		out->strs = 0;
		out->fns = 0;
		out->code = 0;
		out->nstrs = out->nfns = out->ncode = 0;
	}
	if (!buf || len < 8) {
		rerr = "truncated header";
		return -1;
	}
	rbuf r;
	r.p = buf;
	r.len = len;
	r.pos = 0;
	r.err = 0;

	if (buf[0] != 'X' || buf[1] != 'A' || buf[2] != 'L' || buf[3] != 'B') {
		rerr = "bad magic (not an XALB image)";
		return -1;
	}
	r.pos = 4;
	u32 ver = ru32(&r);
	if (r.err) {
		rerr = "truncated header";
		return -1;
	}
	if (ver != XAL_FMT_VERSION) {
		rerr = "unsupported image version";
		return -1;
	}

	xcore_image im;
	im.version = ver;
	im.strs = 0;
	im.fns = 0;
	im.code = 0;
	im.nstrs = im.nfns = im.ncode = 0;
	im.nglobals = im.mainidx = im.initidx = 0;

	im.nstrs = ru32(&r);
	if (im.nstrs > 0x00ffffffu) {
		rerr = "string count out of range";
		return -1;
	}
	if (im.nstrs) {
		im.strs = (char **)xos_alloc((usz)im.nstrs * sizeof(char *));
		if (!im.strs) {
			rerr = "out of memory";
			im.nstrs = 0;
			return -1;
		}
		for (u32 i = 0; i < im.nstrs; i++)
			im.strs[i] = 0;
	}
	for (u32 i = 0; i < im.nstrs && !r.err; i++) {
		u32 l = ru32(&r);
		const u8 *s = rtake(&r, l);
		if (r.err)
			break;
		im.strs[i] = rdup(s, l);
		if (!im.strs[i]) {
			rerr = "out of memory";
			xcore_image_free(&im);
			return -1;
		}
	}
	if (r.err) {
		rerr = "truncated string table";
		xcore_image_free(&im);
		return -1;
	}

	im.nfns = ru32(&r);
	if (im.nfns > XCORE_MAX_FNS) {
		rerr = "function count out of range";
		xcore_image_free(&im);
		return -1;
	}
	if (im.nfns) {
		im.fns = (xcore_fn *)xos_alloc((usz)im.nfns * sizeof(xcore_fn));
		if (!im.fns) {
			rerr = "out of memory";
			xcore_image_free(&im);
			return -1;
		}
	}
	for (u32 i = 0; i < im.nfns && !r.err; i++) {
		im.fns[i].entry = ru32(&r);
		im.fns[i].nparams = ru32(&r);
		im.fns[i].nlocals = ru32(&r);
		im.fns[i].nameidx = ru32(&r);
	}
	if (r.err) {
		rerr = "truncated function table";
		xcore_image_free(&im);
		return -1;
	}

	im.nglobals = ru32(&r);
	im.mainidx = ru32(&r);
	im.initidx = ru32(&r);
	im.ncode = ru32(&r);
	if (r.err) {
		rerr = "truncated image header";
		xcore_image_free(&im);
		return -1;
	}
	if (im.ncode > 0x0fffffffu) {
		rerr = "code size out of range";
		xcore_image_free(&im);
		return -1;
	}
	if (im.ncode) {
		im.code = (xcore_insn *)xos_alloc((usz)im.ncode * sizeof(xcore_insn));
		if (!im.code) {
			rerr = "out of memory";
			xcore_image_free(&im);
			return -1;
		}
	}
	for (u32 i = 0; i < im.ncode && !r.err; i++) {
		im.code[i].op = ru32(&r);
		im.code[i].a = ri64(&r);
	}
	if (r.err) {
		rerr = "truncated code section";
		xcore_image_free(&im);
		return -1;
	}

	if (im.mainidx >= im.nfns) {
		rerr = "main function index out of range";
		xcore_image_free(&im);
		return -1;
	}
	if (im.initidx >= im.nfns) {
		rerr = "init function index out of range";
		xcore_image_free(&im);
		return -1;
	}
	for (u32 i = 0; i < im.nfns; i++) {
		if (im.fns[i].entry >= im.ncode) {
			rerr = "function entry out of range";
			xcore_image_free(&im);
			return -1;
		}
		if (im.fns[i].nameidx >= im.nstrs) {
			rerr = "function name index out of range";
			xcore_image_free(&im);
			return -1;
		}
		if (im.fns[i].nlocals > 0x00ffffffu || im.fns[i].nparams > im.fns[i].nlocals) {
			rerr = "function frame size out of range";
			xcore_image_free(&im);
			return -1;
		}
	}
	for (u32 i = 0; i < im.ncode; i++) {
		if (im.code[i].op >= OP__COUNT) {
			rerr = "unknown opcode in image";
			xcore_image_free(&im);
			return -1;
		}
	}

	if (out)
		*out = im;
	return 0;
}

void xcore_image_free(xcore_image *im) {
	if (!im)
		return;
	if (im->strs) {
		for (u32 i = 0; i < im->nstrs; i++)
			xos_free(im->strs[i]);
		xos_free(im->strs);
		im->strs = 0;
	}
	xos_free(im->fns);
	im->fns = 0;
	xos_free(im->code);
	im->code = 0;
	im->nstrs = im->nfns = im->ncode = 0;
}
