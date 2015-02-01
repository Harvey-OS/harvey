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

typedef struct ZRead	ZRead;

struct ZRead
{
	ulong	adler;
	void	*rr;
	int	(*r)(void*, void*, int);
};

static int
zlread(void *vzr, void *buf, int n)
{
	ZRead *zr;

	zr = vzr;
	n = (*zr->r)(zr->rr, buf, n);
	if(n <= 0)
		return n;
	zr->adler = adler32(zr->adler, buf, n);
	return n;
}

int
deflatezlib(void *wr, int (*w)(void*, void*, int), void *rr, int (*r)(void*, void*, int), int level, int debug)
{
	ZRead zr;
	uchar buf[4];
	int ok;

	buf[0] = ZlibDeflate | ZlibWin32k;

	/* bogus zlib encoding of compression level */
	buf[1] = ((level > 2) + (level > 5) + (level > 8)) << 6;

	/* header check field */
	buf[1] |= 31 - ((buf[0] << 8) | buf[1]) % 31;
	if((*w)(wr, buf, 2) != 2)
		return FlateOutputFail;

	zr.rr = rr;
	zr.r = r;
	zr.adler = 1;
	ok = deflate(wr, w, &zr, zlread, level, debug);
	if(ok != FlateOk)
		return ok;

	buf[0] = zr.adler >> 24;
	buf[1] = zr.adler >> 16;
	buf[2] = zr.adler >> 8;
	buf[3] = zr.adler;
	if((*w)(wr, buf, 4) != 4)
		return FlateOutputFail;

	return FlateOk;
}
