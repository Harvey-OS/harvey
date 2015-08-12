/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef signed char schar;

#define SIGN(n) (1UL << (n - 1))

typedef struct Vlong Vlong;
struct Vlong {
	uint32_t hi;
	uint32_t lo;
};

void abort(void);
void _divu64(Vlong, Vlong, Vlong *, Vlong *);

void
_d2v(Vlong *y, double d)
{
	union {
		double d;
		Vlong;
	} x;
	uint32_t xhi, xlo, ylo, yhi;
	int sh;

	x.d = d;

	xhi = (x.hi & 0xfffff) | 0x100000;
	xlo = x.lo;
	sh = 1075 - ((x.hi >> 20) & 0x7ff);

	ylo = 0;
	yhi = 0;
	if(sh >= 0) {
		/* v = (hi||lo) >> sh */
		if(sh < 32) {
			if(sh == 0) {
				ylo = xlo;
				yhi = xhi;
			} else {
				ylo = (xlo >> sh) | (xhi << (32 - sh));
				yhi = xhi >> sh;
			}
		} else {
			if(sh == 32) {
				ylo = xhi;
			} else if(sh < 64) {
				ylo = xhi >> (sh - 32);
			}
		}
	} else {
		/* v = (hi||lo) << -sh */
		sh = -sh;
		if(sh <= 10) {
			ylo = xlo << sh;
			yhi = (xhi << sh) | (xlo >> (32 - sh));
		} else {
			/* overflow */
			yhi = d; /* causes something awful */
		}
	}
	if(x.hi & SIGN(32)) {
		if(ylo != 0) {
			ylo = -ylo;
			yhi = ~yhi;
		} else
			yhi = -yhi;
	}

	y->hi = yhi;
	y->lo = ylo;
}

void
_f2v(Vlong *y, float f)
{

	_d2v(y, f);
}

double
_v2d(Vlong x)
{
	if(x.hi & SIGN(32)) {
		if(x.lo) {
			x.lo = -x.lo;
			x.hi = ~x.hi;
		} else
			x.hi = -x.hi;
		return -((int32_t)x.hi * 4294967296. + x.lo);
	}
	return (int32_t)x.hi * 4294967296. + x.lo;
}

float
_v2f(Vlong x)
{
	return _v2d(x);
}

void
_divvu(Vlong *q, Vlong n, Vlong d)
{

	if(n.hi == 0 && d.hi == 0) {
		q->hi = 0;
		q->lo = n.lo / d.lo;
		return;
	}
	_divu64(n, d, q, 0);
}

void
_modvu(Vlong *r, Vlong n, Vlong d)
{

	if(n.hi == 0 && d.hi == 0) {
		r->hi = 0;
		r->lo = n.lo % d.lo;
		return;
	}
	_divu64(n, d, 0, r);
}

static void
vneg(Vlong *v)
{

	if(v->lo == 0) {
		v->hi = -v->hi;
		return;
	}
	v->lo = -v->lo;
	v->hi = ~v->hi;
}

void
_divv(Vlong *q, Vlong n, Vlong d)
{
	int32_t nneg, dneg;

	if(n.hi == (((int32_t)n.lo) >> 31) && d.hi == (((int32_t)d.lo) >> 31)) {
		q->lo = (int32_t)n.lo / (int32_t)d.lo;
		q->hi = ((int32_t)q->lo) >> 31;
		return;
	}
	nneg = n.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = d.hi >> 31;
	if(dneg)
		vneg(&d);
	_divu64(n, d, q, 0);
	if(nneg != dneg)
		vneg(q);
}

void
_modv(Vlong *r, Vlong n, Vlong d)
{
	int32_t nneg, dneg;

	if(n.hi == (((int32_t)n.lo) >> 31) && d.hi == (((int32_t)d.lo) >> 31)) {
		r->lo = (int32_t)n.lo % (int32_t)d.lo;
		r->hi = ((int32_t)r->lo) >> 31;
		return;
	}
	nneg = n.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = d.hi >> 31;
	if(dneg)
		vneg(&d);
	_divu64(n, d, 0, r);
	if(nneg)
		vneg(r);
}

void
_vasop(Vlong *ret, void *lv, void fn(Vlong *, Vlong, Vlong), int type, Vlong rv)
{
	Vlong t, u;

	u = *ret;
	switch(type) {
	default:
		abort();
		break;

	case 1: /* schar */
		t.lo = *(schar *)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(schar *)lv = u.lo;
		break;

	case 2: /* uchar */
		t.lo = *(uint8_t *)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uint8_t *)lv = u.lo;
		break;

	case 3: /* short */
		t.lo = *(int16_t *)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(int16_t *)lv = u.lo;
		break;

	case 4: /* ushort */
		t.lo = *(uint16_t *)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uint16_t *)lv = u.lo;
		break;

	case 9: /* int */
		t.lo = *(int *)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(int *)lv = u.lo;
		break;

	case 10: /* uint */
		t.lo = *(uint *)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uint *)lv = u.lo;
		break;

	case 5: /* long */
		t.lo = *(int32_t *)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(int32_t *)lv = u.lo;
		break;

	case 6: /* ulong */
		t.lo = *(uint32_t *)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uint32_t *)lv = u.lo;
		break;

	case 7: /* vlong */
	case 8: /* uvlong */
		fn(&u, *(Vlong *)lv, rv);
		*(Vlong *)lv = u;
		break;
	}
	*ret = u;
}
