/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "../plan9/lib.h"
#include "../plan9/sys9.h"
#define _LOCK_EXTENSION
#include <lock.h>
//#include <lib9.h>

void
lock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return;	/* changed from 0 -> 1: we hold lock */
	/* otherwise wait in kernel */
	while(_SEMACQUIRE(&l->sem, 1) < 0){
		/* interrupted; try again */
	}
}

void
unlock(Lock *l)
{
	if(adec(&l->key) == 0)
		return;	/* changed from 1 -> 0: no contention */
	_SEMRELEASE(&l->sem, 1);
}

int
canlock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return 1;	/* changed from 0 -> 1: success */
	/* Undo increment (but don't miss wakeup) */
	if(adec(&l->key) == 0)
		return 0;	/* changed from 1 -> 0: no contention */
	_SEMRELEASE(&l->sem, 1);
	return 0;
}
