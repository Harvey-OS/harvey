/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include <libsec.h>
#include <ctype.h>

extern void jacobian_affine(mpint *p,
	mpint *X, mpint *Y, mpint *Z);
extern void jacobian_dbl(mpint *p, mpint *a,
	mpint *X1, mpint *Y1, mpint *Z1,
	mpint *X3, mpint *Y3, mpint *Z3);
extern void jacobian_add(mpint *p, mpint *a,
	mpint *X1, mpint *Y1, mpint *Z1,
	mpint *X2, mpint *Y2, mpint *Z2,
	mpint *X3, mpint *Y3, mpint *Z3);

void
ecassign(ECdomain *dom, ECpoint *a, ECpoint *b)
{
	if((b->inf = a->inf) != 0)
		return;
	mpassign(a->x, b->x);
	mpassign(a->y, b->y);
	if(b->z != nil){
		mpassign(a->z != nil ? a->z : mpone, b->z);
		return;
	}
	if(a->z != nil){
		b->z = mpcopy(a->z);
		jacobian_affine(dom->p, b->x, b->y, b->z);
		mpfree(b->z);
		b->z = nil;
	}
}

void
ecadd(ECdomain *dom, ECpoint *a, ECpoint *b, ECpoint *s)
{
	if(a->inf && b->inf){
		s->inf = 1;
		return;
	}
	if(a->inf){
		ecassign(dom, b, s);
		return;
	}
	if(b->inf){
		ecassign(dom, a, s);
		return;
	}

	if(s->z == nil){
		s->z = mpcopy(mpone);
		ecadd(dom, a, b, s);
		if(!s->inf)
			jacobian_affine(dom->p, s->x, s->y, s->z);
		mpfree(s->z);
		s->z = nil;
		return;
	}

	if(a == b)
		jacobian_dbl(dom->p, dom->a,
			a->x, a->y, a->z != nil ? a->z : mpone,
			s->x, s->y, s->z);
	else
		jacobian_add(dom->p, dom->a,
			a->x, a->y, a->z != nil ? a->z : mpone,
			b->x, b->y, b->z != nil ? b->z : mpone,
			s->x, s->y, s->z);
	s->inf = mpcmp(s->z, mpzero) == 0;
}

void
ecmul(ECdomain *dom, ECpoint *a, mpint *k, ECpoint *s)
{
	ECpoint ns, na;
	mpint *l;

	if(a->inf || mpcmp(k, mpzero) == 0){
		s->inf = 1;
		return;
	}
	ns.inf = 1;
	ns.x = mpnew(0);
	ns.y = mpnew(0);
	ns.z = mpnew(0);
	na.x = mpnew(0);
	na.y = mpnew(0);
	na.z = mpnew(0);
	ecassign(dom, a, &na);
	l = mpcopy(k);
	l->sign = 1;
	while(mpcmp(l, mpzero) != 0){
		if(l->p[0] & 1)
			ecadd(dom, &na, &ns, &ns);
		ecadd(dom, &na, &na, &na);
		mpright(l, 1, l);
	}
	if(k->sign < 0 && !ns.inf){
		ns.y->sign = -1;
		mpmod(ns.y, dom->p, ns.y);
	}
	ecassign(dom, &ns, s);
	mpfree(ns.x);
	mpfree(ns.y);
	mpfree(ns.z);
	mpfree(na.x);
	mpfree(na.y);
	mpfree(na.z);
	mpfree(l);
}

int
ecverify(ECdomain *dom, ECpoint *a)
{
	mpint *p, *q;
	int r;

	if(a->inf)
		return 1;

	assert(a->z == nil);	/* need affine coordinates */
	p = mpnew(0);
	q = mpnew(0);
	mpmodmul(a->y, a->y, dom->p, p);
	mpmodmul(a->x, a->x, dom->p, q);
	mpmodadd(q, dom->a, dom->p, q);
	mpmodmul(q, a->x, dom->p, q);
	mpmodadd(q, dom->b, dom->p, q);
	r = mpcmp(p, q);
	mpfree(p);
	mpfree(q);
	return r == 0;
}

int
ecpubverify(ECdomain *dom, ECpub *a)
{
	ECpoint p;
	int r;

	if(a->inf)
		return 0;
	if(!ecverify(dom, a))
		return 0;
	p.x = mpnew(0);
	p.y = mpnew(0);
	p.z = mpnew(0);
	ecmul(dom, a, dom->n, &p);
	r = p.inf;
	mpfree(p.x);
	mpfree(p.y);
	mpfree(p.z);
	return r;
}

static void
fixnibble(uint8_t *a)
{
	if(*a >= 'a')
		*a -= 'a'-10;
	else if(*a >= 'A')
		*a -= 'A'-10;
	else
		*a -= '0';
}

static int
octet(char **s)
{
	uint8_t c, d;

	c = *(*s)++;
	if(!isxdigit(c))
		return -1;
	d = *(*s)++;
	if(!isxdigit(d))
		return -1;
	fixnibble(&c);
	fixnibble(&d);
	return (c << 4) | d;
}

static mpint*
halfpt(ECdomain *dom, char *s, char **rptr, mpint *out)
{
	char *buf, *r;
	int n;
	mpint *ret;

	n = ((mpsignif(dom->p)+7)/8)*2;
	if(strlen(s) < n)
		return 0;
	buf = malloc(n+1);
	buf[n] = 0;
	memcpy(buf, s, n);
	ret = strtomp(buf, &r, 16, out);
	*rptr = s + (r - buf);
	free(buf);
	return ret;
}

static int
mpleg(mpint *a, mpint *b)
{
	int r, k;
	mpint *m, *n, *t;

	r = 1;
	m = mpcopy(a);
	n = mpcopy(b);
	for(;;){
		if(mpcmp(m, n) > 0)
			mpmod(m, n, m);
		if(mpcmp(m, mpzero) == 0){
			r = 0;
			break;
		}
		if(mpcmp(m, mpone) == 0)
			break;
		k = mplowbits0(m);
		if(k > 0){
			if(k & 1)
				switch(n->p[0] & 15){
				case 3: case 5: case 11: case 13:
					r = -r;
				}
			mpright(m, k, m);
		}
		if((n->p[0] & 3) == 3 && (m->p[0] & 3) == 3)
			r = -r;
		t = m;
		m = n;
		n = t;
	}
	mpfree(m);
	mpfree(n);
	return r;
}

static int
mpsqrt(mpint *n, mpint *p, mpint *r)
{
	mpint *a, *t, *s, *xp, *xq, *yp, *yq, *zp, *zq, *N;

	if(mpleg(n, p) == -1)
		return 0;
	a = mpnew(0);
	t = mpnew(0);
	s = mpnew(0);
	N = mpnew(0);
	xp = mpnew(0);
	xq = mpnew(0);
	yp = mpnew(0);
	yq = mpnew(0);
	zp = mpnew(0);
	zq = mpnew(0);
	for(;;){
		for(;;){
			mpnrand(p, genrandom, a);
			if(mpcmp(a, mpzero) > 0)
				break;
		}
		mpmul(a, a, t);
		mpsub(t, n, t);
		mpmod(t, p, t);
		if(mpleg(t, p) == -1)
			break;
	}
	mpadd(p, mpone, N);
	mpright(N, 1, N);
	mpmul(a, a, t);
	mpsub(t, n, t);
	mpassign(a, xp);
	uitomp(1, xq);
	uitomp(1, yp);
	uitomp(0, yq);
	while(mpcmp(N, mpzero) != 0){
		if(N->p[0] & 1){
			mpmul(xp, yp, zp);
			mpmul(xq, yq, zq);
			mpmul(zq, t, zq);
			mpadd(zp, zq, zp);
			mpmod(zp, p, zp);
			mpmul(xp, yq, zq);
			mpmul(xq, yp, s);
			mpadd(zq, s, zq);
			mpmod(zq, p, yq);
			mpassign(zp, yp);
		}
		mpmul(xp, xp, zp);
		mpmul(xq, xq, zq);
		mpmul(zq, t, zq);
		mpadd(zp, zq, zp);
		mpmod(zp, p, zp);
		mpmul(xp, xq, zq);
		mpadd(zq, zq, zq);
		mpmod(zq, p, xq);
		mpassign(zp, xp);
		mpright(N, 1, N);
	}
	if(mpcmp(yq, mpzero) != 0)
		abort();
	mpassign(yp, r);
	mpfree(a);
	mpfree(t);
	mpfree(s);
	mpfree(N);
	mpfree(xp);
	mpfree(xq);
	mpfree(yp);
	mpfree(yq);
	mpfree(zp);
	mpfree(zq);
	return 1;
}

ECpoint*
strtoec(ECdomain *dom, char *s, char **rptr, ECpoint *ret)
{
	int allocd, o;
	mpint *r;

	allocd = 0;
	if(ret == nil){
		allocd = 1;
		ret = mallocz(sizeof(*ret), 1);
		if(ret == nil)
			return nil;
		ret->x = mpnew(0);
		ret->y = mpnew(0);
	}
	ret->inf = 0;
	o = 0;
	switch(octet(&s)){
	case 0:
		ret->inf = 1;
		break;
	case 3:
		o = 1;
	case 2:
		if(halfpt(dom, s, &s, ret->x) == nil)
			goto err;
		r = mpnew(0);
		mpmul(ret->x, ret->x, r);
		mpadd(r, dom->a, r);
		mpmul(r, ret->x, r);
		mpadd(r, dom->b, r);
		if(!mpsqrt(r, dom->p, r)){
			mpfree(r);
			goto err;
		}
		if((r->p[0] & 1) != o)
			mpsub(dom->p, r, r);
		mpassign(r, ret->y);
		mpfree(r);
		if(!ecverify(dom, ret))
			goto err;
		break;
	case 4:
		if(halfpt(dom, s, &s, ret->x) == nil)
			goto err;
		if(halfpt(dom, s, &s, ret->y) == nil)
			goto err;
		if(!ecverify(dom, ret))
			goto err;
		break;
	}
	if(ret->z != nil && !ret->inf)
		mpassign(mpone, ret->z);
	return ret;

err:
	if(rptr)
		*rptr = s;
	if(allocd){
		mpfree(ret->x);
		mpfree(ret->y);
		free(ret);
	}
	return nil;
}

ECpriv*
ecgen(ECdomain *dom, ECpriv *p)
{
	if(p == nil){
		p = mallocz(sizeof(*p), 1);
		if(p == nil)
			return nil;
		p->ecpoint.x = mpnew(0);
		p->ecpoint.y = mpnew(0);
		p->d = mpnew(0);
	}
	for(;;){
		mpnrand(dom->n, genrandom, p->d);
		if(mpcmp(p->d, mpzero) > 0)
			break;
	}
	ecmul(dom, &dom->G, p->d, (struct ECpoint *)p);
	return p;
}

void
ecdsasign(ECdomain *dom, ECpriv *priv, uint8_t *dig, int len, mpint *r, mpint *s)
{
	ECpriv tmp;
	mpint *E, *t;

	tmp.ecpoint.x = mpnew(0);
	tmp.ecpoint.y = mpnew(0);
	tmp.ecpoint.z = nil;
	tmp.d = mpnew(0);
	E = betomp(dig, len, nil);
	t = mpnew(0);
	if(mpsignif(dom->n) < 8*len)
		mpright(E, 8*len - mpsignif(dom->n), E);
	for(;;){
		ecgen(dom, &tmp);
		mpmod(tmp.ecpoint.x, dom->n, r);
		if(mpcmp(r, mpzero) == 0)
			continue;
		mpmul(r, priv->d, s);
		mpadd(E, s, s);
		mpinvert(tmp.d, dom->n, t);
		mpmodmul(s, t, dom->n, s);
		if(mpcmp(s, mpzero) != 0)
			break;
	}
	mpfree(t);
	mpfree(E);
	mpfree(tmp.ecpoint.x);
	mpfree(tmp.ecpoint.y);
	mpfree(tmp.d);
}

int
ecdsaverify(ECdomain *dom, ECpub *pub, uint8_t *dig, int len, mpint *r, mpint *s)
{
	mpint *E, *t, *u1, *u2;
	ECpoint R, S;
	int ret;

	if(mpcmp(r, mpone) < 0 || mpcmp(s, mpone) < 0 || mpcmp(r, dom->n) >= 0 || mpcmp(r, dom->n) >= 0)
		return 0;
	E = betomp(dig, len, nil);
	if(mpsignif(dom->n) < 8*len)
		mpright(E, 8*len - mpsignif(dom->n), E);
	t = mpnew(0);
	u1 = mpnew(0);
	u2 = mpnew(0);
	R.x = mpnew(0);
	R.y = mpnew(0);
	R.z = mpnew(0);
	S.x = mpnew(0);
	S.y = mpnew(0);
	S.z = mpnew(0);
	mpinvert(s, dom->n, t);
	mpmodmul(E, t, dom->n, u1);
	mpmodmul(r, t, dom->n, u2);
	ecmul(dom, &dom->G, u1, &R);
	ecmul(dom, pub, u2, &S);
	ecadd(dom, &R, &S, &R);
	ret = 0;
	if(!R.inf){
		jacobian_affine(dom->p, R.x, R.y, R.z);
		mpmod(R.x, dom->n, t);
		ret = mpcmp(r, t) == 0;
	}
	mpfree(E);
	mpfree(t);
	mpfree(u1);
	mpfree(u2);
	mpfree(R.x);
	mpfree(R.y);
	mpfree(R.z);
	mpfree(S.x);
	mpfree(S.y);
	mpfree(S.z);
	return ret;
}

static char *code = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

void
base58enc(uint8_t *src, char *dst, int len)
{
	mpint *n, *r, *b;
	char *sdst, t;

	sdst = dst;
	n = betomp(src, len, nil);
	b = uitomp(58, nil);
	r = mpnew(0);
	while(mpcmp(n, mpzero) != 0){
		mpdiv(n, b, n, r);
		*dst++ = code[mptoui(r)];
	}
	for(; *src == 0; src++)
		*dst++ = code[0];
	dst--;
	while(dst > sdst){
		t = *sdst;
		*sdst++ = *dst;
		*dst-- = t;
	}
}

int
base58dec(char *src, uint8_t *dst, int len)
{
	mpint *n, *b, *r;
	char *t;

	n = mpnew(0);
	r = mpnew(0);
	b = uitomp(58, nil);
	for(; *src; src++){
		t = strchr(code, *src);
		if(t == nil){
			mpfree(n);
			mpfree(r);
			mpfree(b);
			werrstr("invalid base58 char");
			return -1;
		}
		uitomp(t - code, r);
		mpmul(n, b, n);
		mpadd(n, r, n);
	}
	mptober(n, dst, len);
	mpfree(n);
	mpfree(r);
	mpfree(b);
	return 0;
}

void
ecdominit(ECdomain *dom, void (*init)(mpint *p, mpint *a, mpint *b, mpint *x, mpint *y, mpint *n, mpint *h))
{
	memset(dom, 0, sizeof(*dom));
	dom->p = mpnew(0);
	dom->a = mpnew(0);
	dom->b = mpnew(0);
	dom->G.x = mpnew(0);
	dom->G.y = mpnew(0);
	dom->n = mpnew(0);
	dom->h = mpnew(0);
	if(init){
		(*init)(dom->p, dom->a, dom->b, dom->G.x, dom->G.y, dom->n, dom->h);
		dom->p = mpfield(dom->p);
	}
}

void
ecdomfree(ECdomain *dom)
{
	mpfree(dom->p);
	mpfree(dom->a);
	mpfree(dom->b);
	mpfree(dom->G.x);
	mpfree(dom->G.y);
	mpfree(dom->n);
	mpfree(dom->h);
	memset(dom, 0, sizeof(*dom));
}

int
ecencodepub(ECdomain *dom, ECpub *pub, uint8_t *data, int len)
{
	int n;

	n = (mpsignif(dom->p)+7)/8;
	if(len < 1 + 2*n)
		return 0;
	len = 1 + 2*n;
	data[0] = 0x04;
	mptober(pub->x, data+1, n);
	mptober(pub->y, data+1+n, n);
	return len;
}

ECpub*
ecdecodepub(ECdomain *dom, uint8_t *data, int len)
{
	ECpub *pub;
	int n;

	n = (mpsignif(dom->p)+7)/8;
	if(len != 1 + 2*n || data[0] != 0x04)
		return nil;
	pub = mallocz(sizeof(*pub), 1);
	if(pub == nil)
		return nil;
	pub->x = betomp(data+1, n, nil);
	pub->y = betomp(data+1+n, n, nil);
	if(!ecpubverify(dom, pub)){
		ecpubfree(pub);
		pub = nil;
	}
	return pub;
}

void
ecpubfree(ECpub *p)
{
	if(p == nil)
		return;
	mpfree(p->x);
	mpfree(p->y);
	free(p);
}
