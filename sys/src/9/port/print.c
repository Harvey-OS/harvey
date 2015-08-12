/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

static Lock fmtl;

void
_fmtlock(void)
{
	lock(&fmtl);
}

void
_fmtunlock(void)
{
	unlock(&fmtl);
}

int
_efgfmt(Fmt *fmt)
{
	return -1;
}

void
fmtinit(void)
{
	quotefmtinstall();
	archfmtinstall();
}
