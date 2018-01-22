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
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static struct {
	int thread;
	int die;
	QLock	qlock;
	NbnsAlarm *head;
} alarmlist = { -1 };

#define MaxLong ((1 << (sizeof(int32_t) * 4 - 1)) - 1)

void
alarmist(void *v)
{
	for (;;) {
		int64_t now;
		int32_t snooze;
//print("running\n");
		qlock(&alarmlist.qlock);
		if (alarmlist.die) {
			qunlock(&alarmlist.qlock);
			break;
		}
		now = nsec() / 1000000;
		while (alarmlist.head && alarmlist.head->expirems <= now) {
//print("expiring because %lld > %lld\n", alarmlist.head->expirems, now);
			sendul(alarmlist.head->c, 1);
			alarmlist.head = alarmlist.head->next;
		}
		if (alarmlist.head) {
			int64_t vsnooze = alarmlist.head->expirems - now;
			if (vsnooze > MaxLong)
				snooze = MaxLong;
			else
				snooze = vsnooze;
		}
		else
			snooze = 60 * 1000;
//print("snoozing for %ld\n", snooze);
		qunlock(&alarmlist.qlock);
		sleep(snooze);
	}
}

NbnsAlarm *
nbnsalarmnew(void)
{
	NbnsAlarm *a;
	a = mallocz(sizeof(*a), 1);
	if (a == nil)
		return nil;
	a->c = chancreate(sizeof(uint32_t), 1);
	if (a->c == nil) {
		free(a);
		return nil;
	}
	return a;
}

void
nbnsalarmcancel(NbnsAlarm *a)
{
	NbnsAlarm **ap;
	int rv;
	qlock(&alarmlist.qlock);
	for (ap = &alarmlist.head; *ap && *ap != a; ap = &(*ap)->next)
		;
	if (*ap) {
		*ap = a->next;
		if (ap == &alarmlist.head)
			threadint(alarmlist.thread);
	}
	qunlock(&alarmlist.qlock);
	do {
		uint32_t v;
		rv = nbrecv(a->c, &v);
	} while (rv != 0);
}

void
nbnsalarmend(void)
{
	qlock(&alarmlist.qlock);
	if (alarmlist.thread >= 0) {
		alarmlist.die = 1;
		threadint(alarmlist.thread);
	}
	qunlock(&alarmlist.qlock);
}

void
nbnsalarmset(NbnsAlarm *a, uint32_t millisec)
{
	NbnsAlarm **ap;
	nbnsalarmcancel(a);
	a->expirems = nsec() / 1000000 + millisec;
	qlock(&alarmlist.qlock);
	for (ap = &alarmlist.head; *ap; ap = &(*ap)->next)
		if (a->expirems < (*ap)->expirems)
			break;
	a->next = (*ap);
	*ap = a;
	if (alarmlist.thread < 0)
		alarmlist.thread = proccreate(alarmist, nil, 16384);
	else
		threadint(alarmlist.thread);
	qunlock(&alarmlist.qlock);
}

void
nbnsalarmfree(NbnsAlarm **ap)
{
	NbnsAlarm *a;
	a = *ap;
	if (a) {
		nbnsalarmcancel(a);
		chanfree(a->c);
		free(a);
		*ap = nil;
	}
}
