#include <u.h>
#include <libc.h>
#include <mp.h>
#include "dat.h"

int loops = 1;

long randomreg;

void
srand(long seed)
{
	randomreg = seed;
}

long
lrand(void)
{
	randomreg = randomreg*104381 + 81761;
	return randomreg;
}

void
prng(uchar *p, int n)
{
	while(n-- > 0)
		*p++ = lrand();
}

void
testconv(char *str)
{
	mpint *b;
	char *p;

	b = strtomp(str, nil, 16, nil);

	p = mptoa(b, 10, nil, 0);
	print("%s = ", p);
	strtomp(p, nil, 10, b);
	free(p);
	print("%B\n", b);

	p = mptoa(b, 16, nil, 0);
	print("%s = ", p);
	strtomp(p, nil, 16, b);
	free(p);
	print("%B\n", b);

	p = mptoa(b, 32, nil, 0);
	print("%s = ", p);
	strtomp(p, nil, 32, b);
	free(p);
	print("%B\n", b);

	p = mptoa(b, 64, nil, 0);
	print("%s = ", p);
	strtomp(p, nil, 64, b);
	free(p);
	print("%B\n", b);

	mpfree(b);
}

void
testshift(char *str)
{
	mpint *b1, *b2;
	int i;

	b1 = strtomp(str, nil, 16, nil);
	b2 = mpnew(0);
	for(i = 0; i < 64; i++){
		mpleft(b1, i, b2);
		print("%2.2d %B\n", i, b2);
	}
	for(i = 0; i < 64; i++){
		mpright(b2, i, b1);
		print("%2.2d %B\n", i, b1);
	}
	mpfree(b1);
	mpfree(b2);
}

void
testaddsub(char *str)
{
	mpint *b1, *b2;
	int i;

	b1 = strtomp(str, nil, 16, nil);
	b2 = mpnew(0);
	for(i = 0; i < 16; i++){
		mpadd(b1, b2, b2);
		print("%2.2d %B\n", i, b2);
	}
	for(i = 0; i < 16; i++){
		mpsub(b2, b1, b2);
		print("%2.2d %B\n", i, b2);
	}
	mpfree(b1);
	mpfree(b2);
}

void
testvecdigmuladd(char *str, mpdigit d)
{
	mpint *b, *b2;
	int i;
	vlong now;

	b = strtomp(str, nil, 16, nil);
	b2 = mpnew(0);

	mpbits(b2, (b->top+1)*Dbits);
	now = nsec();
	for(i = 0; i < loops; i++){
		memset(b2->p, 0, b2->top*Dbytes);
		mpvecdigmuladd(b->p, b->top, d, b2->p);
	}
	if(loops > 1)
		print("%lld ns for a %d*%d vecdigmul\n", (nsec()-now)/loops, b->top*Dbits, Dbits);
	mpnorm(b2);
	print("0 + %B * %ux = %B\n", b, d, b2);

	mpfree(b);
	mpfree(b2);
}

void
testvecdigmulsub(char *str, mpdigit d)
{
	mpint *b, *b2;
	int i;
	vlong now;

	b = strtomp(str, nil, 16, nil);
	b2 = mpnew(0);

	mpbits(b2, (b->top+1)*Dbits);
	now = nsec();
	for(i = 0; i < loops; i++){
		memset(b2->p, 0, b2->top*Dbytes);
		mpvecdigmulsub(b->p, b->top, d, b2->p);
	}
	if(loops > 1)
		print("%lld ns for a %d*%d vecdigmul\n", (nsec()-now)/loops, b->top*Dbits, Dbits);
	mpnorm(b2);
	print("0 - %B * %ux = %B\n", b, d, b2);

	mpfree(b);
	mpfree(b2);
}

void
testmul(char *str)
{
	mpint *b, *b1, *b2;
	vlong now;
	int i;

	b = strtomp(str, nil, 16, nil);
	b1 = mpcopy(b);
	b2 = mpnew(0);

	now = nsec();
	for(i = 0; i < loops; i++)
		mpmul(b, b1, b2);
	if(loops > 1)
		print("%lld µs for a %d*%d mult\n", (nsec()-now)/(loops*1000),
			b->top*Dbits, b1->top*Dbits);
	print("%B * %B = %B\n", b, b1, b2);

	mpfree(b);
	mpfree(b1);
	mpfree(b2);
}

void
testmul2(mpint *b, mpint *b1)
{
	mpint *b2;
	vlong now;
	int i;

	b2 = mpnew(0);

	now = nsec();
	for(i = 0; i < loops; i++)
		mpmul(b, b1, b2);
	if(loops > 1)
		print("%lld µs for a %d*%d mult\n", (nsec()-now)/(loops*1000), b->top*Dbits, b1->top*Dbits);
	print("%B * ", b);
	print("%B = ", b1);
	print("%B\n", b2);

	mpfree(b2);
}

void
testdigdiv(char *str, mpdigit d)
{
	mpint *b;
	mpdigit q;
	int i;
	vlong now;

	b = strtomp(str, nil, 16, nil);
	now = nsec();
	for(i = 0; i < loops; i++)
		mpdigdiv(b->p, d, &q);
	if(loops > 1)
		print("%lld ns for a %d / %d div\n", (nsec()-now)/loops, 2*Dbits, Dbits);
	print("%B / %ux = %ux\n", b, d, q);
	mpfree(b);
}

void
testdiv(mpint *x, mpint *y)
{
	mpint *b2, *b3;
	vlong now;
	int i;

	b2 = mpnew(0);
	b3 = mpnew(0);
	now = nsec();
	for(i = 0; i < loops; i++)
		mpdiv(x, y, b2, b3);
	if(loops > 1)
		print("%lld µs for a %d/%d div\n", (nsec()-now)/(1000*loops),
			x->top*Dbits, y->top*Dbits);
	print("%B / %B = %B %B\n", x, y, b2, b3);
	mpfree(b2);
	mpfree(b3);
}

void
testmod(mpint *x, mpint *y)
{
	mpint *r;
	vlong now;
	int i;

	r = mpnew(0);
	now = nsec();
	for(i = 0; i < loops; i++)
		mpmod(x, y, r);
	if(loops > 1)
		print("%lld µs for a %d/%d mod\n", (nsec()-now)/(1000*loops),
			x->top*Dbits, y->top*Dbits);
	print("%B mod %B = %B\n", x, y, r);
	mpfree(r);
}

void
testinvert(mpint *x, mpint *y)
{
	mpint *r, *d1, *d2;
	vlong now;
	int i;

	r = mpnew(0);
	d1 = mpnew(0);
	d2 = mpnew(0);
	now = nsec();
	mpextendedgcd(x, y, r, d1, d2);
	mpdiv(x, r, x, d1);
	mpdiv(y, r, y, d1);
	for(i = 0; i < loops; i++)
		mpinvert(x, y, r);
	if(loops > 1)
		print("%lld µs for a %d in %d invert\n", (nsec()-now)/(1000*loops),
			x->top*Dbits, y->top*Dbits);
	print("%B**-1 mod %B = %B\n", x, y, r);
	mpmul(r, x, d1);
	mpmod(d1, y, d2);
	print("%B*%B mod %B = %B\n", x, r, y, d2);
	mpfree(r);
	mpfree(d1);
	mpfree(d2);
}

void
testsub1(char *a, char *b)
{
	mpint *b1, *b2, *b3;

	b1 = strtomp(a, nil, 16, nil);
	b2 = strtomp(b, nil, 16, nil);
	b3 = mpnew(0);
	mpsub(b1, b2, b3);
	print("%B - %B = %B\n", b1, b2, b3);
}

void
testmul1(char *a, char *b)
{
	mpint *b1, *b2, *b3;

	b1 = strtomp(a, nil, 16, nil);
	b2 = strtomp(b, nil, 16, nil);
	b3 = mpnew(0);
	mpmul(b1, b2, b3);
	print("%B * %B = %B\n", b1, b2, b3);
}

void
testexp(char *base, char *exp, char *mod)
{
	mpint *b, *e, *m, *res;
	int i;
	uvlong now;

	b = strtomp(base, nil, 16, nil);
	e = strtomp(exp, nil, 16, nil);
	res = mpnew(0);
	if(mod != nil)
		m = strtomp(mod, nil, 16, nil);
	else
		m = nil;
	now = nsec();
	for(i = 0; i < loops; i++)
		mpexp(b, e, m, res);
	if(loops > 1)
		print("%ulldµs for a %d to the %d bit exp\n", (nsec()-now)/(loops*1000),
			b->top*Dbits, e->top*Dbits);
	if(m != nil)
		print("%B ^ %B mod %B == %B\n", b, e, m, res);
	else
		print("%B ^ %B == %B\n", b, e, res);
	mpfree(b);
	mpfree(e);
	mpfree(res);
	if(m != nil)
		mpfree(m);
}

void
testgcd(void)
{
	mpint *a, *b, *d, *x, *y, *t1, *t2;
	int i;
	uvlong now, then;
	uvlong etime;

	d = mpnew(0);
	x = mpnew(0);
	y = mpnew(0);
	t1 = mpnew(0);
	t2 = mpnew(0);

	etime = 0;

	a = strtomp("4EECAB3E04C4E6BC1F49D438731450396BF272B4D7B08F91C38E88ADCD281699889AFF872E2204C80CCAA8E460797103DE539D5DF8335A9B20C0B44886384F134C517287202FCA914D8A5096446B40CD861C641EF9C2730CB057D7B133F4C2B16BBD3D75FDDBD9151AAF0F9144AAA473AC93CF945DBFE0859FB685D5CBD0A8B3", nil, 16, nil);
	b = strtomp("C41CFBE4D4846F67A3DF7DE9921A49D3B42DC33728427AB159CEC8CBBDB12B5F0C244F1A734AEB9840804EA3C25036AD1B61AFF3ABBC247CD4B384224567A863A6F020E7EE9795554BCD08ABAD7321AF27E1E92E3DB1C6E7E94FAAE590AE9C48F96D93D178E809401ABE8A534A1EC44359733475A36A70C7B425125062B1142D", nil, 16, nil);
	mpextendedgcd(a, b, d, x, y);
	print("gcd %B*%B+%B*%B = %B?\n", a, x, b, y, d);
	mpfree(a);
	mpfree(b);

	for(i = 0; i < loops; i++){
		a = mprand(2048, prng, nil);
		b = mprand(2048, prng, nil);
		then = nsec();
		mpextendedgcd(a, b, d, x, y);
		now = nsec();
		etime += now-then;
		mpmul(a, x, t1);
		mpmul(b, y, t2);
		mpadd(t1, t2, t2);
		if(mpcmp(d, t2) != 0)
			print("%d gcd %B*%B+%B*%B != %B\n", i, a, x, b, y, d);
//		else
//			print("%d euclid %B*%B+%B*%B == %B\n", i, a, x, b, y, d);
		mpfree(a);
		mpfree(b);
	}

	mpfree(x);
	mpfree(y);
	mpfree(d);
	mpfree(t1);
	mpfree(t2);

	if(loops > 1)
		print("binary %llud\n", etime);
}

extern int _unnormalizedwarning = 1;

void
main(int argc, char **argv)
{
	mpint *x, *y;

	ARGBEGIN{
	case 'n':
		loops = atoi(ARGF());
		break;
	}ARGEND;

	fmtinstall('B', mpconv);
	fmtinstall('Q', mpconv);
	srand(0);
	mpsetminbits(2*Dbits);
	testshift("1111111111111111");
	testaddsub("fffffffffffffffff");
	testdigdiv("1234567812345678", 0x76543218);
	testdigdiv("1ffff", 0xffff);
	testdigdiv("ffff", 0xffff);
	testdigdiv("fff", 0xffff);
	testdigdiv("effffffff", 0xffff);
	testdigdiv("ffffffff", 0x1);
	testdigdiv("ffffffff", 0);
	testdigdiv("200000000", 2);
	testdigdiv("ffffff00fffffff1", 0xfffffff1);
	testvecdigmuladd("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2);
	testconv("0");
	testconv("-abc0123456789abcedf");
	testconv("abc0123456789abcedf");
	testvecdigmulsub("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2);
	testsub1("1FFFFFFFE00000000", "FFFFFFFE00000001");
	testmul1("ffffffff", "f");
	testmul("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
	testmul1("100000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000004FFFFFFFFFFFFFFFE0000000200000000000000000000000000000003FFFFFFFFFFFFFFFE0000000200000000000000000000000000000002FFFFFFFFFFFFFFFE0000000200000000000000000000000000000001FFFFFFFFFFFFFFFE0000000200000000000000000000000000000000FFFFFFFFFFFFFFFE0000000200000000FFFFFFFE00000001", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
	testmul1("1000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001", "1000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001");
	testmul1("1000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001", "1000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001000000000000000000000001");
	testmul1("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
	x = mprand(256, prng, nil);
	y = mprand(128, prng, nil);
	testdiv(x, y);
	x = mprand(2048, prng, nil);
	y = mprand(1024, prng, nil);
	testdiv(x, y);
//	x = mprand(4*1024, prng, nil);
//	y = mprand(4*1024, prng, nil);
//	testmul2(x, y);
	testsub1("677132C9", "-A26559B6");
	testgcd();
	x = mprand(512, prng, nil);
	x->sign = -1;
	y = mprand(256, prng, nil);
	testdiv(x, y);
	testmod(x, y);
	x->sign = 1;
	testinvert(y, x);
	testexp("111111111", "222", "1000000000000000000000");
	testexp("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
#ifdef asdf
#endif adsf
	print("done\n");
	exits(0);
}
