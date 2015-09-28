/*
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

void
lock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return;	/* changed from 0 -> 1: we hold lock */
	/* otherwise wait in kernel */
	while(semacquire(&l->sem, 1) < 0){
		/* interrupted; try again */
	}
}

void
unlock(Lock *l)
{
	if(adec(&l->key) == 0)
		return;	/* changed from 1 -> 0: no contention */
	semrelease(&l->sem, 1);
}

int
canlock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return 1;	/* changed from 0 -> 1: success */
	/* Undo increment (but don't miss wakeup) */
	if(adec(&l->key) == 0)
		return 0;	/* changed from 1 -> 0: no contention */
	semrelease(&l->sem, 1);
	return 0;
}

int
lockt(Lock *l, uint32_t ms)
{
	int semr;
	int64_t start, end;

	if(ainc(&l->key) == 1)
		return 1;	/* changed from 0 -> 1: we hold lock */
	/* otherwise wait in kernel */
	semr = 0;
	start = nsec() / (1000 * 1000);
	end = start + ms;
	while(start < end && (semr = tsemacquire(&l->sem, ms)) < 0){
		/* interrupted; try again */
		start = nsec() / (1000 * 1000);
		ms = end - start;
	}
	/* copy canlock semantic for return values */
	if(semr == 1)
		return 1;	/* success, lock acquired */
	return 0;		/* timed out or interrupt at timeout */
}
