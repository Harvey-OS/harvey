/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

void
bwatchReset(uchar score[VtScoreSize])
{
	USED(score);
}

void
bwatchInit(void)
{
}

void
bwatchSetBlockSize(uint)
{
}

void
bwatchDependency(Block *b)
{
	USED(b);
}

void
bwatchLock(Block *b)
{
	USED(b);
}

void
bwatchUnlock(Block *b)
{
	USED(b);
}

