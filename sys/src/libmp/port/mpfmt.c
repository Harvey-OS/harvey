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
#include "dat.h"

static int
to64(mpint *b, char *buf, int len)
{
	uint8_t *p;
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
	uint8_t *p;
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
modbillion(int rem, uint32_t r, char *out, char *buf)
{
	uint32_t rr;
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
mpfmt(Fmt *fmt)
{
	mpint *b;
	char *p;

	b = va_arg(fmt->args, mpint*);
	if(b == nil)
		return fmtstrcpy(fmt, "*");

	p = mptoa(b, fmt->prec, nil, 0);
	fmt->flags &= ~FmtPrec;

	if(p == nil)
		return fmtstrcpy(fmt, "*");
	else{
		fmtstrcpy(fmt, p);
		free(p);
		return 0;
	}
}

char*
mptoa(mpint *b, int base, char *buf, int len)
{
	char *out;
	int rv, alloced;

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
