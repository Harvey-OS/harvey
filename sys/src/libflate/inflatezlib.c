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
#include <flate.h>
#include "zlib.h"

typedef struct ZWrite	ZWrite;

struct ZWrite
{
	ulong	adler;
	void	*wr;
	int	(*w)(void*, void*, int);
};

static int
zlwrite(void *vzw, void *buf, int n)
{
	ZWrite *zw;

	zw = vzw;
	zw->adler = adler32(zw->adler, buf, n);
	n = (*zw->w)(zw->wr, buf, n);
	if(n <= 0)
		return n;
	return n;
}

int
inflatezlib(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*))
{
	ZWrite zw;
	ulong v;
	int c, i;

	c = (*get)(getr);
	if(c < 0)
		return FlateInputFail;
	i = (*get)(getr);
	if(i < 0)
		return FlateInputFail;

	if(((c << 8) | i) % 31)
		return FlateCorrupted;
	if((c & ZlibMeth) != ZlibDeflate
	|| (c & ZlibCInfo) > ZlibWin32k)
		return FlateCorrupted;

	zw.wr = wr;
	zw.w = w;
	zw.adler = 1;
	i = inflate(&zw, zlwrite, getr, get);
	if(i != FlateOk)
		return i;

	v = 0;
	for(i = 0; i < 4; i++){
		c = (*get)(getr);
		if(c < 0)
			return FlateInputFail;
		v = (v << 8) | c;
	}
	if(zw.adler != v)
		return FlateCorrupted;

	return FlateOk;
}
