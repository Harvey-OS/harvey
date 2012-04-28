typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	signed char	schar;

#define	SIGN(n)	(1UL<<(n-1))

typedef	struct	Vlong	Vlong;
struct	Vlong
{
	union
	{
		struct
		{
			uint	lo;
			uint	hi;
		}w32;
		struct
		{
			ushort	lols;
			ushort	loms;
			ushort	hils;
			ushort	hims;
		}w16;
	}uv;
};

void	abort(void);

void _subv(Vlong*, Vlong, Vlong);

void
_d2v(Vlong *y, double d)
{
	union { double d; struct Vlong V; } x;
	uint xhi, xlo, ylo, yhi;
	int sh;

	x.d = d;

	xhi = (x.V.uv.w32.hi & 0xfffff) | 0x100000;
	xlo = x.V.uv.w32.lo;
	sh = 1075 - ((x.V.uv.w32.hi >> 20) & 0x7ff);

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
	if(x.V.uv.w32.hi & SIGN(32)) {
		if(ylo != 0) {
			ylo = -ylo;
			yhi = ~yhi;
		} else
			yhi = -yhi;
	}

	y->uv.w32.hi = yhi;
	y->uv.w32.lo = ylo;
}

void
_f2v(Vlong *y, float f)
{

	_d2v(y, f);
}

double
_v2d(Vlong x)
{
	if(x.uv.w32.hi & SIGN(32)) {
		if(x.uv.w32.lo) {
			x.uv.w32.lo = -x.uv.w32.lo;
			x.uv.w32.hi = ~x.uv.w32.hi;
		} else
			x.uv.w32.hi = -x.uv.w32.hi;
		return -((long)x.uv.w32.hi*4294967296. + x.uv.w32.lo);
	}
	return (long)x.uv.w32.hi*4294967296. + x.uv.w32.lo;
}

float
_v2f(Vlong x)
{
	return _v2d(x);
}

uint	_div64by32(Vlong, uint, uint*);
void	_mul64by32(Vlong*, Vlong, uint);

static void
slowdodiv(Vlong num, Vlong den, Vlong *q, Vlong *r)
{
	uint numlo, numhi, denhi, denlo, quohi, quolo, t;
	int i;

	numhi = num.uv.w32.hi;
	numlo = num.uv.w32.lo;
	denhi = den.uv.w32.hi;
	denlo = den.uv.w32.lo;

	/*
	 * get a divide by zero
	 */
	if(denlo==0 && denhi==0) {
		numlo = numlo / denlo;
	}

	/*
	 * set up the divisor and find the number of iterations needed
	 */
	if(numhi >= SIGN(32)) {
		quohi = SIGN(32);
		quolo = 0;
	} else {
		quohi = numhi;
		quolo = numlo;
	}
	i = 0;
	while(denhi < quohi || (denhi == quohi && denlo < quolo)) {
		denhi = (denhi<<1) | (denlo>>31);
		denlo <<= 1;
		i++;
	}

	quohi = 0;
	quolo = 0;
	for(; i >= 0; i--) {
		quohi = (quohi<<1) | (quolo>>31);
		quolo <<= 1;
		if(numhi > denhi || (numhi == denhi && numlo >= denlo)) {
			t = numlo;
			numlo -= denlo;
			if(numlo > t)
				numhi--;
			numhi -= denhi;
			quolo |= 1;
		}
		denlo = (denlo>>1) | (denhi<<31);
		denhi >>= 1;
	}

	if(q) {
		q->uv.w32.lo = quolo;
		q->uv.w32.hi = quohi;
	}
	if(r) {
		r->uv.w32.lo = numlo;
		r->uv.w32.hi = numhi;
	}
}

/* conservative */
unsigned long long V2uv(Vlong *n)
{
	unsigned long long ret = n->uv.w32.hi;
	ret <<= 32;
	ret |= n->uv.w32.lo;
	return ret;
}

void uv2V(unsigned long long num, Vlong *n)
{
	n->uv.w32.hi = num >> 32;
	n->uv.w32.lo = num;
}

static void
dodiv(Vlong num, Vlong den, Vlong *qp, Vlong *rp)
{
	unsigned long long lnum, lden, lq, lr;

	lnum = V2uv(&num);
	lden = V2uv(&den);
	lq = lnum / lden;
	lr = lnum % lden;
	uv2V(lq, qp);
	uv2V(lr, rp);
}

void
_divvu(Vlong *q, Vlong n, Vlong d)
{

	if(n.uv.w32.hi == 0 && d.uv.w32.hi == 0) {
		q->uv.w32.hi = 0;
		q->uv.w32.lo = n.uv.w32.lo / d.uv.w32.lo;
		return;
	}
	dodiv(n, d, q, 0);
}

void
_modvu(Vlong *r, Vlong n, Vlong d)
{

	if(n.uv.w32.hi == 0 && d.uv.w32.hi == 0) {
		r->uv.w32.hi = 0;
		r->uv.w32.lo = n.uv.w32.lo % d.uv.w32.lo;
		return;
	}
	dodiv(n, d, 0, r);
}

static void
vneg(Vlong *v)
{

	if(v->uv.w32.lo == 0) {
		v->uv.w32.hi = -v->uv.w32.hi;
		return;
	}
	v->uv.w32.lo = -v->uv.w32.lo;
	v->uv.w32.hi = ~v->uv.w32.hi;
}

void
_divv(Vlong *q, Vlong n, Vlong d)
{
	long nneg, dneg;

	if(n.uv.w32.hi == (((long)n.uv.w32.lo)>>31) && d.uv.w32.hi == (((long)d.uv.w32.lo)>>31)) {
		q->uv.w32.lo = (long)n.uv.w32.lo / (long)d.uv.w32.lo;
		q->uv.w32.hi = ((long)q->uv.w32.lo) >> 31;
		return;
	}
	nneg = n.uv.w32.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = d.uv.w32.hi >> 31;
	if(dneg)
		vneg(&d);
	dodiv(n, d, q, 0);
	if(nneg != dneg)
		vneg(q);
}

void
_modv(Vlong *r, Vlong n, Vlong d)
{
	long nneg, dneg;

	if(n.uv.w32.hi == (((long)n.uv.w32.lo)>>31) && d.uv.w32.hi == (((long)d.uv.w32.lo)>>31)) {
		r->uv.w32.lo = (long)n.uv.w32.lo % (long)d.uv.w32.lo;
		r->uv.w32.hi = ((long)r->uv.w32.lo) >> 31;
		return;
	}
	nneg = n.uv.w32.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = d.uv.w32.hi >> 31;
	if(dneg)
		vneg(&d);
	dodiv(n, d, 0, r);
	if(nneg)
		vneg(r);
}

void
_rshav(Vlong *r, Vlong a, int b)
{
	long t;

	t = a.uv.w32.hi;
	if(b >= 32) {
		r->uv.w32.hi = t>>31;
		if(b >= 64) {
			/* this is illegal re C standard */
			r->uv.w32.lo = t>>31;
			return;
		}
		r->uv.w32.lo = t >> (b-32);
		return;
	}
	if(b <= 0) {
		r->uv.w32.hi = t;
		r->uv.w32.lo = a.uv.w32.lo;
		return;
	}
	r->uv.w32.hi = t >> b;
	r->uv.w32.lo = (t << (32-b)) | (a.uv.w32.lo >> b);
}

void
_rshlv(Vlong *r, Vlong a, int b)
{
	uint t;

	t = a.uv.w32.hi;
	if(b >= 32) {
		r->uv.w32.hi = 0;
		if(b >= 64) {
			/* this is illegal re C standard */
			r->uv.w32.lo = 0;
			return;
		}
		r->uv.w32.lo = t >> (b-32);
		return;
	}
	if(b <= 0) {
		r->uv.w32.hi = t;
		r->uv.w32.lo = a.uv.w32.lo;
		return;
	}
	r->uv.w32.hi = t >> b;
	r->uv.w32.lo = (t << (32-b)) | (a.uv.w32.lo >> b);
}

void
_lshv(Vlong *r, Vlong a, int b)
{
	uint t;

	t = a.uv.w32.lo;
	if(b >= 32) {
		r->uv.w32.lo = 0;
		if(b >= 64) {
			/* this is illegal re C standard */
			r->uv.w32.hi = 0;
			return;
		}
		r->uv.w32.hi = t << (b-32);
		return;
	}
	if(b <= 0) {
		r->uv.w32.lo = t;
		r->uv.w32.hi = a.uv.w32.hi;
		return;
	}
	r->uv.w32.lo = t << b;
	r->uv.w32.hi = (t >> (32-b)) | (a.uv.w32.hi << b);
}

void
_andv(Vlong *r, Vlong a, Vlong b)
{
	r->uv.w32.hi = a.uv.w32.hi & b.uv.w32.hi;
	r->uv.w32.lo = a.uv.w32.lo & b.uv.w32.lo;
}

void
_orv(Vlong *r, Vlong a, Vlong b)
{
	r->uv.w32.hi = a.uv.w32.hi | b.uv.w32.hi;
	r->uv.w32.lo = a.uv.w32.lo | b.uv.w32.lo;
}

void
_xorv(Vlong *r, Vlong a, Vlong b)
{
	r->uv.w32.hi = a.uv.w32.hi ^ b.uv.w32.hi;
	r->uv.w32.lo = a.uv.w32.lo ^ b.uv.w32.lo;
}

void
_vpp(Vlong *l, Vlong *r)
{

	l->uv.w32.hi = r->uv.w32.hi;
	l->uv.w32.lo = r->uv.w32.lo;
	r->uv.w32.lo++;
	if(r->uv.w32.lo == 0)
		r->uv.w32.hi++;
}

void
_vmm(Vlong *l, Vlong *r)
{

	l->uv.w32.hi = r->uv.w32.hi;
	l->uv.w32.lo = r->uv.w32.lo;
	if(r->uv.w32.lo == 0)
		r->uv.w32.hi--;
	r->uv.w32.lo--;
}

void
_ppv(Vlong *l, Vlong *r)
{

	r->uv.w32.lo++;
	if(r->uv.w32.lo == 0)
		r->uv.w32.hi++;
	l->uv.w32.hi = r->uv.w32.hi;
	l->uv.w32.lo = r->uv.w32.lo;
}

void
_mmv(Vlong *l, Vlong *r)
{

	if(r->uv.w32.lo == 0)
		r->uv.w32.hi--;
	r->uv.w32.lo--;
	l->uv.w32.hi = r->uv.w32.hi;
	l->uv.w32.lo = r->uv.w32.lo;
}

void
_vasop(Vlong *ret, void *lv, void fn(Vlong*, Vlong, Vlong), int type, Vlong rv)
{
	Vlong t, u;

	u.uv.w32.lo = 0;
	u.uv.w32.hi = 0;
	switch(type) {
	default:
		abort();
		break;

	case 1:	/* schar */
		t.uv.w32.lo = *(schar*)lv;
		t.uv.w32.hi = t.uv.w32.lo >> 31;
		fn(&u, t, rv);
		*(schar*)lv = u.uv.w32.lo;
		break;

	case 2:	/* uchar */
		t.uv.w32.lo = *(uchar*)lv;
		t.uv.w32.hi = 0;
		fn(&u, t, rv);
		*(uchar*)lv = u.uv.w32.lo;
		break;

	case 3:	/* short */
		t.uv.w32.lo = *(short*)lv;
		t.uv.w32.hi = t.uv.w32.lo >> 31;
		fn(&u, t, rv);
		*(short*)lv = u.uv.w32.lo;
		break;

	case 4:	/* ushort */
		t.uv.w32.lo = *(ushort*)lv;
		t.uv.w32.hi = 0;
		fn(&u, t, rv);
		*(ushort*)lv = u.uv.w32.lo;
		break;

	case 9:	/* int */
		t.uv.w32.lo = *(int*)lv;
		t.uv.w32.hi = t.uv.w32.lo >> 31;
		fn(&u, t, rv);
		*(int*)lv = u.uv.w32.lo;
		break;

	case 10:	/* uint */
		t.uv.w32.lo = *(uint*)lv;
		t.uv.w32.hi = 0;
		fn(&u, t, rv);
		*(uint*)lv = u.uv.w32.lo;
		break;

	case 5:	/* long */
		t.uv.w32.lo = *(long*)lv;
		t.uv.w32.hi = t.uv.w32.lo >> 31;
		fn(&u, t, rv);
		*(long*)lv = u.uv.w32.lo;
		break;

	case 6:	/* uint */
		t.uv.w32.lo = *(uint*)lv;
		t.uv.w32.hi = 0;
		fn(&u, t, rv);
		*(uint*)lv = u.uv.w32.lo;
		break;

	case 7:	/* vlong */
	case 8:	/* uvlong */
		fn(&u, *(Vlong*)lv, rv);
		*(Vlong*)lv = u;
		break;
	}
	*ret = u;
}

void
_p2v(Vlong *ret, void *p)
{
	long t;

	t = (unsigned long long)p;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = ((unsigned long long)p)>>32;
}

void
_sl2v(Vlong *ret, long sl)
{
	long t;

	t = sl;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = t >> 31;
}

void
_ul2v(Vlong *ret, uint ul)
{
	long t;

	t = ul;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = 0;
}

void
_si2v(Vlong *ret, int si)
{
	long t;

	t = si;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = t >> 31;
}

void
_ui2v(Vlong *ret, uint ui)
{
	long t;

	t = ui;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = 0;
}

void
_sh2v(Vlong *ret, long sh)
{
	long t;

	t = (sh << 16) >> 16;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = t >> 31;
}

void
_uh2v(Vlong *ret, uint ul)
{
	long t;

	t = ul & 0xffff;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = 0;
}

void
_sc2v(Vlong *ret, long uc)
{
	long t;

	t = (uc << 24) >> 24;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = t >> 31;
}

void
_uc2v(Vlong *ret, uint ul)
{
	long t;

	t = ul & 0xff;
	ret->uv.w32.lo = t;
	ret->uv.w32.hi = 0;
}

long
_v2sc(Vlong rv)
{
	long t;

	t = rv.uv.w32.lo & 0xff;
	return (t << 24) >> 24;
}

long
_v2uc(Vlong rv)
{

	return rv.uv.w32.lo & 0xff;
}

long
_v2sh(Vlong rv)
{
	long t;

	t = rv.uv.w32.lo & 0xffff;
	return (t << 16) >> 16;
}

long
_v2uh(Vlong rv)
{

	return rv.uv.w32.lo & 0xffff;
}

long
_v2sl(Vlong rv)
{

	return rv.uv.w32.lo;
}

long
_v2ul(Vlong rv)
{

	return rv.uv.w32.lo;
}

long
_v2si(Vlong rv)
{

	return rv.uv.w32.lo;
}

long
_v2ui(Vlong rv)
{

	return rv.uv.w32.lo;
}

int
_testv(Vlong rv)
{
	return rv.uv.w32.lo || rv.uv.w32.hi;
}

int
_eqv(Vlong lv, Vlong rv)
{
	return lv.uv.w32.lo == rv.uv.w32.lo && lv.uv.w32.hi == rv.uv.w32.hi;
}

int
_nev(Vlong lv, Vlong rv)
{
	return lv.uv.w32.lo != rv.uv.w32.lo || lv.uv.w32.hi != rv.uv.w32.hi;
}

int
_ltv(Vlong lv, Vlong rv)
{
	return (long)lv.uv.w32.hi < (long)rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo < rv.uv.w32.lo);
}

int
_lev(Vlong lv, Vlong rv)
{
	return (long)lv.uv.w32.hi < (long)rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo <= rv.uv.w32.lo);
}

int
_gtv(Vlong lv, Vlong rv)
{
	return (long)lv.uv.w32.hi > (long)rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo > rv.uv.w32.lo);
}

int
_gev(Vlong lv, Vlong rv)
{
	return (long)lv.uv.w32.hi > (long)rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo >= rv.uv.w32.lo);
}

int
_lov(Vlong lv, Vlong rv)
{
	return lv.uv.w32.hi < rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo < rv.uv.w32.lo);
}

int
_lsv(Vlong lv, Vlong rv)
{
	return lv.uv.w32.hi < rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo <= rv.uv.w32.lo);
}

int
_hiv(Vlong lv, Vlong rv)
{
	return lv.uv.w32.hi > rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo > rv.uv.w32.lo);
}

int
_hsv(Vlong lv, Vlong rv)
{
	return lv.uv.w32.hi > rv.uv.w32.hi || 
		(lv.uv.w32.hi == rv.uv.w32.hi && lv.uv.w32.lo >= rv.uv.w32.lo);
}
