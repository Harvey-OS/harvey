/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>

int
encodefmt(Fmt *f)
{
	char *out;
	char *buf;
	int len;
	int ilen;
	int rv;
	uchar *b;
	char *p;
	char obuf[64];	// rsc optimization

	if(!(f->flags&FmtPrec) || f->prec < 1)
		goto error;

	b = va_arg(f->args, uchar*);
	if(b == 0)
		return fmtstrcpy(f, "<nil>");

	ilen = f->prec;
	f->prec = 0;
	f->flags &= ~FmtPrec;
	switch(f->r){
	case '<':
		len = (8*ilen+4)/5 + 3;
		break;
	case '[':
		len = (8*ilen+5)/6 + 4;
		break;
	case 'H':
		len = 2*ilen + 1;
		break;
	default:
		goto error;
	}

	if(len > sizeof(obuf)){
		buf = malloc(len);
		if(buf == nil)
			goto error;
	} else
		buf = obuf;

	// convert
	out = buf;
	switch(f->r){
	case '<':
		rv = enc32(out, len, b, ilen);
		break;
	case '[':
		rv = enc64(out, len, b, ilen);
		break;
	case 'H':
		rv = enc16(out, len, b, ilen);
		if(rv >= 0 && (f->flags & FmtLong))
			for(p = buf; *p; p++)
				*p = tolower(*p);
		break;
	default:
		rv = -1;
		break;
	}
	if(rv < 0)
		goto error;

	fmtstrcpy(f, buf);
	if(buf != obuf)
		free(buf);
	return 0;

error:
	return fmtstrcpy(f, "<encodefmt>");
}
