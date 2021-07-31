/* Common routines for _dtoa and strtod */

#include "fconv.h"

double _tens[] = {
		1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
		1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
		1e20, 1e21, 1e22
		};
double _bigtens[] = { 1e16, 1e32, 1e64, 1e128, 1e256 };
double _tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128, 1e-256 };

static Bigint *freelist[Kmax+1];

Bigint *
_Balloc(int k)
{
	int x;
	Bigint *rv;

	if(rv = freelist[k]){
		freelist[k] = rv->next;
	}else{
		x = 1 << k;
		rv = (Bigint *)malloc(sizeof(Bigint) + (x-1)*sizeof(long));
		rv->k = k;
		rv->maxwds = x;
	}
	rv->sign = rv->wds = 0;
	return rv;
}

void
_Bfree(Bigint *v)
{
	if(v){
		v->next = freelist[v->k];
		freelist[v->k] = v;
	}
}

Bigint *
_multadd(Bigint *b, int m, int a)	/* multiply by m and add a */
{
	int i, wds;
	unsigned long *x, y;
	unsigned long xi, z;
	Bigint *b1;

	wds = b->wds;
	x = b->x;
	i = 0;
	do{
		xi = *x;
		y = (xi & 0xffff) * m + a;
		z = (xi >> 16) * m + (y >> 16);
		a = (int)(z >> 16);
		*x++ = (z << 16) + (y & 0xffff);
	}while(++i < wds);
	if(a){
		if(wds >= b->maxwds){
			b1 = Balloc(b->k+1);
			Bcopy(b1, b);
			Bfree(b);
			b = b1;
		}
		b->x[wds++] = a;
		b->wds = wds;
	}
	return b;
}

int
_hi0bits(register unsigned long x)
{
	int k = 0;

	if(!(x & 0xffff0000)){
		k = 16;
		x <<= 16;
	}
	if(!(x & 0xff000000)){
		k += 8;
		x <<= 8;
	}
	if(!(x & 0xf0000000)){
		k += 4;
		x <<= 4;
	}
	if(!(x & 0xc0000000)){
		k += 2;
		x <<= 2;
	}
	if(!(x & 0x80000000)){
		k++;
		if(!(x & 0x40000000))
			return 32;
	}
	return k;
}

static int
lo0bits(unsigned long *y)
{
	int k;
	unsigned long x = *y;

	if(x & 7){
		if(x & 1)
			return 0;
		if (x & 2){
			*y = x >> 1;
			return 1;
		}
		*y = x >> 2;
		return 2;
	}
	k = 0;
	if(!(x & 0xffff)){
		k = 16;
		x >>= 16;
	}
	if(!(x & 0xff)){
		k += 8;
		x >>= 8;
	}
	if(!(x & 0xf)){
		k += 4;
		x >>= 4;
	}
	if(!(x & 0x3)){
		k += 2;
		x >>= 2;
	}
	if(!(x & 1)){
		k++;
		x >>= 1;
		if(!x & 1)
			return 32;
	}
	*y = x;
	return k;
}

Bigint *
_i2b(int i)
{
	Bigint *b;

	b = Balloc(1);
	b->x[0] = i;
	b->wds = 1;
	return b;
}

Bigint *
_mult(Bigint *a, Bigint *b)
{
	Bigint *c;
	int k, wa, wb, wc;
	unsigned long carry, y, z;
	unsigned long *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
	unsigned long z2;

	if (a->wds < b->wds){
		c = a;
		a = b;
		b = c;
	}
	k = a->k;
	wa = a->wds;
	wb = b->wds;
	wc = wa + wb;
	if(wc > a->maxwds)
		k++;
	c = Balloc(k);
	for(x = c->x, xa = x + wc; x < xa; x++)
		*x = 0;
	xa = a->x;
	xae = xa + wa;
	xb = b->x;
	xbe = xb + wb;
	xc0 = c->x;
	for(; xb < xbe; xb++, xc0++){
		if (y = *xb & 0xffff){
			x = xa;
			xc = xc0;
			carry = 0;
			do{
				z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
				carry = z >> 16;
				z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
				carry = z2 >> 16;
				Storeinc(xc, z2, z);
			}while(x < xae);
			*xc = carry;
		}
		if(y = *xb >> 16){
			x = xa;
			xc = xc0;
			carry = 0;
			z2 = *xc;
			do{
				z = (*x & 0xffff) * y + (*xc >> 16) + carry;
				carry = z >> 16;
				Storeinc(xc, z, z2);
				z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
				carry = z2 >> 16;
			}while(x < xae);
			*xc = z2;
		}
	}
	for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
	c->wds = wc;
	return c;
}

static Bigint *p5s;

Bigint *
_pow5mult(Bigint *b, int k)
{
	Bigint *b1, *p5, *p51;
	int i;
	static int p05[3] = { 5, 25, 125 };

	if(i = k & 3)
		b = multadd(b, p05[i-1], 0);

	if(!(k >>= 2))
		return b;
	if(!(p5 = p5s)){
		/* first time */
		p5 = p5s = i2b(625);
		p5->next = 0;
	}
	for(;;){
		if(k & 1){
			b1 = mult(b, p5);
			Bfree(b);
			b = b1;
		}
		if(!(k >>= 1))
			break;
		if(!(p51 = p5->next)){
			p51 = p5->next = mult(p5,p5);
			p51->next = 0;
		}
		p5 = p51;
	}
	return b;
}

Bigint *
_lshift(Bigint *b, int k)
{
	int i, k1, n, n1;
	Bigint *b1;
	unsigned long *x, *x1, *xe, z;

	n = k >> 5;
	k1 = b->k;
	n1 = n + b->wds + 1;
	for(i = b->maxwds; n1 > i; i <<= 1)
		k1++;
	b1 = Balloc(k1);
	x1 = b1->x;
	for(i = 0; i < n; i++)
		*x1++ = 0;
	x = b->x;
	xe = x + b->wds;
	if(k &= 0x1f){
		k1 = 32 - k;
		z = 0;
		do{
			*x1++ = *x << k | z;
			z = *x++ >> k1;
		}while(x < xe);
		if(*x1 = z)
			++n1;
	}else do
		*x1++ = *x++;
	while(x < xe);
	b1->wds = n1 - 1;
	Bfree(b);
	return b1;
}

int
_cmp(Bigint *a, Bigint *b)
{
	unsigned long *xa, *xa0, *xb, *xb0;
	int i, j;

	i = a->wds;
	j = b->wds;
	if(i -= j)
		return i;
	xa0 = a->x;
	xa = xa0 + j;
	xb0 = b->x;
	xb = xb0 + j;
	for(;;){
		if(*--xa != *--xb)
			return *xa < *xb ? -1 : 1;
		if(xa <= xa0)
			break;
	}
	return 0;
}

Bigint *
_diff(Bigint *a, Bigint *b)
{
	Bigint *c;
	int i, wa, wb;
	long borrow, y;	/* We need signed shifts here. */
	unsigned long *xa, *xae, *xb, *xbe, *xc;
	long z;

	i = cmp(a,b);
	if(!i){
		c = Balloc(0);
		c->wds = 1;
		c->x[0] = 0;
		return c;
	}
	if(i < 0){
		c = a;
		a = b;
		b = c;
		i = 1;
	}else
		i = 0;
	c = Balloc(a->k);
	c->sign = i;
	wa = a->wds;
	xa = a->x;
	xae = xa + wa;
	wb = b->wds;
	xb = b->x;
	xbe = xb + wb;
	xc = c->x;
	borrow = 0;
	do{
		y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
		borrow = z >> 16;
		Sign_Extend(borrow, z);
		Storeinc(xc, z, y);
	}while(xb < xbe);
	while(xa < xae){
		y = (*xa & 0xffff) + borrow;
		borrow = y >> 16;
		Sign_Extend(borrow, y);
		z = (*xa++ >> 16) + borrow;
		borrow = z >> 16;
		Sign_Extend(borrow, z);
		Storeinc(xc, z, y);
	}
	while(!*--xc)
		wa--;
	c->wds = wa;
	return c;
}

Bigint *
_d2b(double darg, int *e, int *bits)
{
	Bigint *b;
	int de, i, k;
	unsigned long *x, y, z;
	Dul d;
	d.d = darg;
#define d0 word0(d)
#define d1 word1(d)

	b = Balloc(1);
	x = b->x;

	z = d0 & Frac_mask;
	d0 &= 0x7fffffff;	/* clear sign bit, which we ignore */
	de = (int)(d0 >> Exp_shift);
	if(Sudden_Underflow){
		z |= Exp_msk11;
	}else{
		if(de)
			z |= Exp_msk1;
	}
	if(y = d1){
		if(k = lo0bits(&y)){
			x[0] = y | z << 32 - k;
			z >>= k;
		}else
			x[0] = y;
		i = b->wds = (x[1] = z) ? 2 : 1;
	}else{
		k = lo0bits(&z);
		x[0] = z;
		i = b->wds = 1;
		k += 32;
	}

	if(Sudden_Underflow){
		*e = de - Bias - (P-1) + k;
		*bits = P - k;
	}else{
		if(de){
			*e = de - Bias - (P-1) + k;
			*bits = P - k;
		}else{
			*e = de - Bias - (P-1) + 1 + k;
			*bits = 32*i - hi0bits(x[i-1]);
		}
	}
	return b;
}
