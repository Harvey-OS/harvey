#include <alef.h>

aggr Vlong
{
	uint hi;
	uint lo;
};

union Cheat
{
	Vlong;
	lint l;
	float d;
};

/*
 * v1 / v2
 */
void
dodiv(Vlong num, Vlong den, Vlong *q, Vlong *r)
{
	uint numlo, numhi, denhi, denlo, quohi, quolo, t;
	int i;

	numhi = num.hi;
	numlo = num.lo;
	denhi = den.hi;
	denlo = den.lo;

	if(!denlo && !denhi){
		numlo = numlo / denlo;
	}

	/*
	 * set up the divisor and find the number of iterations needed
	 */
	if(numhi >= (1<<31)){
		quohi = 1<<31;
		quolo = 0;
	}else{
		quohi = numhi;
		quolo = numlo;
	}
	i = 0;
	if(quohi || (quolo & (1<<31))){
		while(denhi < quohi || denhi == quohi && denlo < quolo){
			denhi = (denhi<<1) | (denlo>>31);
			denlo <<= 1;
			i++;
		}
	}else{
		while(denlo < quolo){
			denlo <<= 1;
			i++;
		}
	}

	quolo = quohi = 0;
	for(; i >= 0; i--){
		quohi = (quohi<<1) | (quolo>>31);
		quolo <<= 1;
		if(numhi > denhi || numhi == denhi && numlo >= denlo){
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
		q->lo = quolo;
		q->hi = quohi;
	}
	if(r) {
		r->lo = numlo;
		r->hi = numhi;
	}
}

void
vneg(Vlong *v)
{

	if(v->lo == 0) {
		v->hi = -v->hi;
		return;
	}
	v->lo = -v->lo;
	v->hi = ~v->hi;
}

Vlong
ALEF_ldivu(Vlong n, Vlong d)
{
	Vlong q;

	if(n.hi == 0 && d.hi == 0){
		q.hi = 0;
		q.lo = n.lo / d.lo;
		return q;
	}
	dodiv(n, d, &q, nil);
	return q;
}

Vlong
ALEF_lmodu(Vlong n, Vlong d)
{
	Vlong r;

	if(n.hi == 0 && d.hi == 0){
		r.hi = 0;
		r.lo = n.lo % d.lo;
		return r;
	}
	dodiv(n, d, nil, &r);
	return r;
}

Vlong
ALEF_ldiv(Vlong n, Vlong d)
{
	Vlong q;
	int nneg, dneg;

	if(n.hi == (((int)n.lo)>>31) && d.hi == (((int)d.lo)>>31)){
		q.lo = (int)n.lo / (int)d.lo;
		q.hi = ((int)q.lo) >> 31;
		return q;
	}
	nneg = n.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = d.hi >> 31;
	if(dneg)
		vneg(&d);
	dodiv(n, d, &q, nil);
	if(nneg != dneg)
		vneg(&q);
	return q;
}

Vlong
ALEF_lmod(Vlong n, Vlong d)
{
	Vlong r;
	int nneg, dneg;

	if(n.hi == (((int)n.lo)>>31) && d.hi == (((int)d.lo)>>31)){
		r.lo = (int)n.lo % (int)d.lo;
		r.hi = ((int)r.lo) >> 31;
		return r;
	}
	nneg = (int)n.hi >> 31;
	if(nneg)
		vneg(&n);
	dneg = (int)d.hi >> 31;
	if(dneg)
		vneg(&d);
	dodiv(n, d, nil, &r);
	if(nneg)
		vneg(&r);
	return r;
}

lint
ALEF_ldtol(float d)
{
	int sh;
	lint y;
	Cheat x;
	uint xhi, xlo, ylo, yhi;

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
	if(x.hi & 0x80000000) {
		if(ylo != 0) {
			ylo = -ylo;
			yhi = ~yhi;
		} else
			yhi = -yhi;
	}

	x.lo = ylo;
	x.hi = yhi;
	return x.l;
}

float
ALEF_lltod(Vlong d)
{
	float f;

	if(d.hi & 0x80000000) {
		vneg(&d);
		f = (float)d.hi*4294967296.0 + (float)d.lo;
		f = -f;
	}
	else
		f = (float)d.hi*4294967296.0 + (float)d.lo;

	return f;
}
