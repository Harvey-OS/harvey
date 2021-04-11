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
#include <venti.h>
#include "cvt.h"

static int
checksize(int n)
{
	if(n < 256 || n > VtMaxLumpSize) {
		werrstr("bad block size");
		return -1;
	}
	return 0;
}

void
vtrootpack(VtRoot *r, u8 *p)
{
	u8 *op = p;

	U16PUT(p, VtRootVersion);
	p += 2;
	memmove(p, r->name, sizeof(r->name));
	p += sizeof(r->name);
	memmove(p, r->type, sizeof(r->type));
	p += sizeof(r->type);
	memmove(p, r->score, VtScoreSize);
	p +=  VtScoreSize;
	U16PUT(p, r->blocksize);
	p += 2;
	memmove(p, r->prev, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtRootSize);
}

int
vtrootunpack(VtRoot *r, u8 *p)
{
	u8 *op = p;
	uint vers;
	memset(r, 0, sizeof(*r));

	vers = U16GET(p);
	if(vers != VtRootVersion) {
		werrstr("unknown root version");
		return -1;
	}
	p += 2;
	memmove(r->name, p, sizeof(r->name));
	r->name[sizeof(r->name)-1] = 0;
	p += sizeof(r->name);
	memmove(r->type, p, sizeof(r->type));
	r->type[sizeof(r->type)-1] = 0;
	p += sizeof(r->type);
	memmove(r->score, p, VtScoreSize);
	p +=  VtScoreSize;
	r->blocksize = U16GET(p);
	if(checksize(r->blocksize) < 0)
		return -1;
	p += 2;
	memmove(r->prev, p, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtRootSize);
	return 0;
}
