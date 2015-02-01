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

typedef struct Block	Block;

struct Block
{
	uchar	*pos;
	uchar	*limit;
};

static int
blread(void *vb, void *buf, int n)
{
	Block *b;

	b = vb;
	if(n > b->limit - b->pos)
		n = b->limit - b->pos;
	memmove(buf, b->pos, n);
	b->pos += n;
	return n;
}

static int
blwrite(void *vb, void *buf, int n)
{
	Block *b;

	b = vb;

	if(n > b->limit - b->pos)
		n = b->limit - b->pos;
	memmove(b->pos, buf, n);
	b->pos += n;
	return n;
}

int
deflateblock(uchar *dst, int dsize, uchar *src, int ssize, int level, int debug)
{
	Block bd, bs;
	int ok;

	bs.pos = src;
	bs.limit = src + ssize;

	bd.pos = dst;
	bd.limit = dst + dsize;

	ok = deflate(&bd, blwrite, &bs, blread, level, debug);
	if(ok != FlateOk)
		return ok;
	return bd.pos - dst;
}
