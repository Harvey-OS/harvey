#include "os.h"
#include <mp.h>
#include <libsec.h>
#include "dat.h"

static int
to64(mpint *b, char *buf, int len)
{
	uchar *p;
	int n, rv;

	p = nil;
	n = mptobe(b, nil, 0, &p);
	if(n < 0)
		return -1;
	rv = enc64(buf, len, p, n);
	free(p);
	return rv;
}

static int
to32(mpint *b, char *buf, int len)
{
	uchar *p;
	int n, rv;

	// leave room for a multiple of 5 buffer size
	n = b->top*Dbytes + 5;
	p = malloc(n);
	if(p == nil)
		return -1;
	n = mptobe(b, p, n, nil);
	if(n < 0)
		return -1;

	// round up buffer size, enc32 only accepts a multiple of 5
	if(n%5)
		n += 5 - (n%5);
	rv = enc32(buf, len, p, n);
	free(p);
	return rv;
}

static char set16[] = "0123456789ABCDEF";

static int
to16(mpint *b, char *buf, int len)
{
	mpdigit *p, x;
	int i, j;
	char *out, *eout;

	if(len < 1)
		return -1;

	out = buf;
	eout = buf+len;
	for(p = &b->p[b->top-1]; p >= b->p; p--){
		x = *p;
		for(i = Dbits-4; i >= 0; i -= 4){
			j = 0xf & (x>>i);
			if(j != 0 || out != buf){
				if(out >= eout)
					return -1;
				*out++ = set16[j];
			}
		}
	}
	if(out == buf)
		*out++ = '0';
	if(out >= eout)
		return -1;
	*out = 0;
	return 0;
}

static char*
modbillion(int rem, ulong r, char *out, char *buf)
{
	ulong rr;
	int i;

	for(i = 0; i < 9; i++){
		rr = r%10;
		r /= 10;
		if(out <= buf)
			return nil;
		*--out = '0' + rr;
		if(rem == 0 && r == 0)
			break;
	}
	return out;
}

static int
to10(mpint *b, char *buf, int len)
{
	mpint *d, *r, *billion;
	char *out;

	if(len < 1)
		return -1;

	d = mpcopy(b);
	r = mpnew(0);
	billion = uitomp(1000000000, nil);
	out = buf+len;
	*--out = 0;
	do {
		mpdiv(d, billion, d, r);
		out = modbillion(d->top, r->p[0], out, buf);
		if(out == nil)
			break;
	} while(d->top != 0);
	mpfree(d);
	mpfree(r);
	mpfree(billion);

	if(out == nil)
		return -1;
	len -= out-buf;
	if(out != buf)
		memmove(buf, out, len);
	return 0;
}

int
mpconv(va_list *arg, Fconv *f)
{
	mpint *b;
	char *p;

	b = va_arg(*arg, mpint*);
	p = mptoa(b, f->f2, nil, 0);
	f->f2 = -1000;

	if(p == nil){
		strconv("*", f);
	} else {
		strconv(p, f);
		free(p);
	}

	return sizeof(mpint*);
}

#ifdef TESTING
// this exists for testing and should normally be disabled
extern int _unnormalizedwarning;
#endif TESTING

char*
mptoa(mpint *b, int base, char *buf, int len)
{
	char *out;
	int rv, alloced;

#ifdef TESTING
	// this exists for testing and should normally be disabled
	if(_unnormalizedwarning && b->top && b->p[b->top-1] == 0)
		fprint(2, "unnormalized\n");
#endif TESTING

	alloced = 0;
	if(buf == nil){
		len = ((b->top+1)*Dbits+2)/3 + 1;
		buf = malloc(len);
		if(buf == nil)
			return nil;
		alloced = 1;
	}

	if(len < 2)
		return nil;

	out = buf;
	if(b->sign < 0){
		*out++ = '-';
		len--;
	}
	switch(base){
	case 64:
		rv = to64(b, out, len);
		break;
	case 32:
		rv = to32(b, out, len);
		break;
	default:
	case 16:
		rv = to16(b, out, len);
		break;
	case 10:
		rv = to10(b, out, len);
		break;
	}
	if(rv < 0){
		if(alloced)
			free(buf);
		return nil;
	}
	return buf;
}
