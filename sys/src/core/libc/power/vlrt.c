typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	signed char	schar;

#define	SIGN(n)	(1UL<<(n-1))

typedef	struct	Vlong	Vlong;
struct	Vlong
{
	ulong	hi;
	ulong	lo;
};

void	abort(void);
void	_divu64(Vlong, Vlong, Vlong*, Vlong*);

void
_d2v(Vlong *y, double d)
{
	union { double d; Vlong; } x;
	ulong xhi, xlo, ylo, yhi;
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
				ylo = (xlo >> sh) | (xhi << (32-sh));
				yhi = xhi >> sh;
			}
		} else {
			if(sh == 32) {
				ylo = xhi;
			} else
			if(sh < 64) {
				ylo = xhi >> (sh-32);
			}
		}
	} else {
		/* v = (hi||lo) << -sh */
		sh = -sh;
		if(sh <= 10) {
			ylo = xlo << sh;
			yhi = (xhi << sh) | (xlo >> (32-sh));
		} else {
			/* overflow */
			yhi = d;	/* causes something awful */
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
		return -((long)x.hi*4294967296. + x.lo);
	}
	return (long)x.hi*4294967296. + x.lo;
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
	long nneg, dneg;

	if(n.hi == (((long)n.lo)>>31) && d.hi == (((long)d.lo)>>31)) {
		q->lo = (long)n.lo / (long)d.lo;
		q->hi = ((long)q->lo) >> 31;
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
	long nneg, dneg;

	if(n.hi == (((long)n.lo)>>31) && d.hi == (((long)d.lo)>>31)) {
		r->lo = (long)n.lo % (long)d.lo;
		r->hi = ((long)r->lo) >> 31;
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
_vasop(Vlong *ret, void *lv, void fn(Vlong*, Vlong, Vlong), int type, Vlong rv)
{
	Vlong t, u;

	u = *ret;
	switch(type) {
	default:
		abort();
		break;

	case 1:	/* schar */
		t.lo = *(schar*)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(schar*)lv = u.lo;
		break;

	case 2:	/* uchar */
		t.lo = *(uchar*)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uchar*)lv = u.lo;
		break;

	case 3:	/* short */
		t.lo = *(short*)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(short*)lv = u.lo;
		break;

	case 4:	/* ushort */
		t.lo = *(ushort*)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(ushort*)lv = u.lo;
		break;

	case 9:	/* int */
		t.lo = *(int*)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(int*)lv = u.lo;
		break;

	case 10:	/* uint */
		t.lo = *(uint*)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(uint*)lv = u.lo;
		break;

	case 5:	/* long */
		t.lo = *(long*)lv;
		t.hi = t.lo >> 31;
		fn(&u, t, rv);
		*(long*)lv = u.lo;
		break;

	case 6:	/* ulong */
		t.lo = *(ulong*)lv;
		t.hi = 0;
		fn(&u, t, rv);
		*(ulong*)lv = u.lo;
		break;

	case 7:	/* vlong */
	case 8:	/* uvlong */
		fn(&u, *(Vlong*)lv, rv);
		*(Vlong*)lv = u;
		break;
	}
	*ret = u;
}
