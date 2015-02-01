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
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

void
mlock(MLock *l)
{

	if(l->key != 0 && l->key != 1)
		panic("uninitialized lock");
	if (l->key)
		panic("lock");
	l->key = 1;
}

void
unmlock(MLock *l)
{

	if(l->key != 0 && l->key != 1)
		panic("uninitialized unlock");
	if (!l->key)
		panic("unlock");
	l->key = 0;
}

int
canmlock(MLock *l)
{
	if(l->key != 0 && l->key != 1)
		panic("uninitialized canlock");
	if (l->key)
		return 0;
	l->key = 1;
	return 1;
}
