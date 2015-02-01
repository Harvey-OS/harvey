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
#include <ureg.h>

int	__noterestore(void);

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	/*
	 * song and dance to get around the kernel smashing r7 in noted
	 */
	r->r8 = ret;
	if(ret == 0)
		r->r8 = 1;
	r->r9 = j[JMPBUFPC] - JMPBUFDPC;
	r->pc = (ulong)__noterestore;
	r->npc = (ulong)__noterestore + 4;
	r->sp = j[JMPBUFSP];
	noted(NCONT);
}
