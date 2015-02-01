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
#include "dat.h"
#include "fns.h"
#include "error.h"

Label*
pwaserror(void)
{
	if(up->nerrlab == NERR)
		panic("error stack overflow");
	return &up->errlab[up->nerrlab++];
}

void
nexterror(void)
{
	longjmp(up->errlab[--up->nerrlab].buf, 1);
}

void
error(char *e)
{
	kstrcpy(up->errstr, e, ERRMAX);
	setjmp(up->errlab[NERR-1].buf);
	nexterror();
}
