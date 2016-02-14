/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

/*
 *  some hacks for commonality twixt inferno and plan9
 */

char*
commonuser(void)
{
	Proc *up = externup();
	return up->user;
}

char*
commonerror(void)
{
	Proc *up = externup();
	return up->errstr;
}

int
bootpread(char *c, uint32_t u, int i)
{
	return	0;
}
