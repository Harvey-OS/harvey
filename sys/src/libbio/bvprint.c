/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>

static int
fmtBflush(Fmt *f)
{
	Biobufhdr *bp;

	bp = f->farg;
	bp->ocount = (int8_t*)f->to - (int8_t*)f->stop;
	if(Bflush(bp) < 0)
		return 0;
	f->stop = bp->ebuf;
	f->to = (int8_t*)f->stop + bp->ocount;
	f->start = f->to;
	return 1;
}

int
Bvprint(Biobufhdr *bp, int8_t *fmt, va_list arg)
{
	int n;
	Fmt f;

	f.runes = 0;
	f.stop = bp->ebuf;
	f.start = (int8_t*)f.stop + bp->ocount;
	f.to = f.start;
	f.flush = fmtBflush;
	f.farg = bp;
	f.nfmt = 0;
	f.args = arg;
	n = dofmt(&f, fmt);
	bp->ocount = (int8_t*)f.to - (int8_t*)f.stop;
	return n;
}
