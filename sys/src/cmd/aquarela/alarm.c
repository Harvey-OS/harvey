#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static struct {
	int thread;
	int die;
	QLock;
	NbnsAlarm *head;
} alarmlist = { -1 };

#define MaxLong ((1 << (sizeof(long) * 8 - 1)) - 1)

void
alarmist(void *)
{
	for (;;) {
		vlong now;
		long snooze;
//print("running\n");
		qlock(&alarmlist);
		if (alarmlist.die) {
			qunlock(&alarmlist);
			break;
		}
		now = nsec() / 1000000;
		while (alarmlist.head && alarmlist.head->expirems <= now) {
//print("expiring because %lld > %lld\n", alarmlist.head->expirems, now);
			sendul(alarmlist.head->c, 1);
			alarmlist.head = alarmlist.head->next;
		}
		if (alarmlist.head) {
			vlong vsnooze = alarmlist.head->expirems - now;
			if (vsnooze > MaxLong)
				snooze = MaxLong;
			else
				snooze = vsnooze;
		}
		else
			snooze = 60 * 1000;
//print("snoozing for %ld\n", snooze);
		qunlock(&alarmlist);
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
	a->c = chancreate(sizeof(ulong), 1);
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
	qlock(&alarmlist);
	for (ap = &alarmlist.head; *ap && *ap != a; ap = &(*ap)->next)
		;
	if (*ap) {
		*ap = a->next;
		if (ap == &alarmlist.head)
			threadint(alarmlist.thread);
	}
	qunlock(&alarmlist);
	do {
		ulong v;
		rv = nbrecv(a->c, &v);
	} while (rv != 0);
}

void
nbnsalarmend(void)
{
	qlock(&alarmlist);
	if (alarmlist.thread >= 0) {
		alarmlist.die = 1;
		threadint(alarmlist.thread);
	}
	qunlock(&alarmlist);
}

void
nbnsalarmset(NbnsAlarm *a, ulong millisec)
{
	NbnsAlarm **ap;
	nbnsalarmcancel(a);
	a->expirems = nsec() / 1000000 + millisec;
	qlock(&alarmlist);
	for (ap = &alarmlist.head; *ap; ap = &(*ap)->next)
		if (a->expirems < (*ap)->expirems)
			break;
	a->next = (*ap);
	*ap = a;
	if (alarmlist.thread < 0)
		alarmlist.thread = proccreate(alarmist, nil, 16384);
	else
		threadint(alarmlist.thread);
	qunlock(&alarmlist);
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
