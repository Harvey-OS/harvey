#include "/sys/src/libmp/port/os.h"
#include <mp.h>
#include <libsec.h>
#include "/sys/src/libmp/port/dat.h"

static struct {
	int	inited;

	uchar	t64[256];
	uchar	t32[256];
	uchar	t16[256];
	uchar	t10[256];
} tab;

enum {
	INVAL=	255
};

static char set64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char set32[] = "23456789abcdefghijkmnpqrstuvwxyz";
static char set16[] = "0123456789ABCDEF0123456789abcdef";
static char set10[] = "0123456789";

static void
init(void)
{
	char *p;

	memset(tab.t64, INVAL, sizeof(tab.t64));
	memset(tab.t32, INVAL, sizeof(tab.t32));
	memset(tab.t16, INVAL, sizeof(tab.t16));
	memset(tab.t10, INVAL, sizeof(tab.t10));

	for(p = set64; *p; p++)
		tab.t64[*p] = p-set64;
	for(p = set32; *p; p++)
		tab.t32[*p] = p-set32;
	for(p = set16; *p; p++)
		tab.t16[*p] = (p-set16)%16;
	for(p = set10; *p; p++)
		tab.t10[*p] = (p-set10);

	tab.inited = 1;
}

static char*
from16(char *a, mpint *b)
{
	char *p, *next;
	int i;
	mpdigit x;

	b->top = 0;
	for(p = a; *p; p++)
		if(tab.t16[*p] == INVAL)
			break;
	next = p;
	while(p > a){
		x = 0;
		for(i = 0; i < Dbits; i += 4){
			if(p <= a)
				break;
			x |= tab.t16[*--p]<<i;
		}
		b->p[b->top++] = x;
	}
	return next;
}

static ulong mppow10[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

static char*
from10(char *a, mpint *b)
{
	ulong x, y;
	mpint *pow, *r;
	int i;

	pow = mpnew(0);
	r = mpnew(0);

	b->top = 0;
	for(;;){
		// do a billion at a time in native arithmetic
		x = 0;
		for(i = 0; i < 9; i++){
			y = tab.t10[*a];
			if(y == INVAL)
				break;
			a++;
			x *= 10;
			x += y;
		}
		if(i == 0)
			break;

		// accumulate into mpint
		uitomp(mppow10[i], pow);
		uitomp(x, r);
		mpmul(b, pow, b);
		mpadd(b, r, b);
		if(i != 9)
			break;
	}
	mpfree(pow);
	mpfree(r);
	return a;
}

static char*
from64(char *a, mpint *b)
{
	char *buf = a;
	uchar *p;
	int n, m;

	for(; tab.t64[*a] != INVAL; a++)
		;
	n = a-buf;
	p = malloc(n);
	if(p == nil)
		return a;
	m = dec64(p, n, buf, n);
	betomp(p, m, b);
	free(p);
	return a;
}

static char*
from32(char *a, mpint *b)
{
	char *buf = a;
	uchar *p;
	int n, m;

	for(; tab.t64[*a] != INVAL; a++)
		;
	n = a-buf;
	p = malloc(n);
	if(p == nil)
		return a;
	m = dec32(p, n, buf, n);
	betomp(p, m, b);
	free(p);
	return a;
}

mpint*
strtomp(char *a, char **pp, int base, mpint *b)
{
	int sign;
	char *e, *e0;
	char *(*from)(char*, mpint*);
	uchar *t;

	if(tab.inited == 0)
		init();

	switch(base){
	case 10:
		t = tab.t10;
		from = from10;
		break;
	default:
	case 16:
		t = tab.t16;
		from = from16;
		break;
	case 32:
		t = tab.t32;
		from = from32;
		break;
	case 64:
		t = tab.t64;
		from = from64;
		break;
	}

	while(*a==' ' || *a=='\t' || *a=='\n' || *a=='\r')
		a++;

	sign = 1;
	for(;; a++){
		switch(*a){
		case '-':
			sign *= -1;
			continue;
		}
		break;
	}

	for(e=a; t[*e] != INVAL; e++)
		;

	if(e == a)
		return nil;

	if(b == nil)
		b = mpnew(0);

	mpbits(b, (e-a)*8);

	e0 = from(a, b);
	assert(e0 == e);

	b->sign = sign;
	mpnorm(b);
	
	if(pp != nil)
		*pp = e;

	return b;
}
