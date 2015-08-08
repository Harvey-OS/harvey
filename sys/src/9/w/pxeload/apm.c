/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

Apminfo apm;

void
apminit(void)
{
	if(getconf("apm0") && apm.haveinfo)
		changeconf("apm0=ax=%x ebx=%x cx=%x dx=%x di=%x esi=%x\n",
		           apm.ax, apm.ebx, apm.cx, apm.dx, apm.di, apm.esi);
}
