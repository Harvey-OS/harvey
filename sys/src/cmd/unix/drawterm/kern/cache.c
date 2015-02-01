/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"


void
cinit(void)
{
}

void
copen(Chan *c)
{
	USED(c);
}

int
cread(Chan *c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);

	return 0;
}

void
cupdate(Chan *c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);
}

void
cwrite(Chan* c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);
}
