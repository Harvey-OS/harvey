#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	"arp.h"
#include 	"ipdat.h"

static	Timer 	*timers;	/* List of active timers */
static	QLock 	tl;		/* Protect timer list */
static	Rendez	Tcpack;
Rendez	tcpflowr;

static void
deltimer(Timer *t)
{
	if(timers == t)
		timers = t->next;

	if(t->next)
		t->next->prev = t->prev;

	if(t->prev)
		t->prev->next = t->next;
}

/*
 * Poke each tcp connection to recompute window size and
 * acknowledgement timer
 */

void
tcpflow(void *x)
{
	Ipifc *ifc;
	Ipconv *cp, **p, **etab;

	ifc = x;
	etab = &ifc->conv[Nipconv];

	for(;;) {
		sleep(&tcpflowr, return0, 0);

		for(p = ifc->conv; p < etab; p++) {
			cp = *p;
			if(cp == 0)
				break;
			if(cp->readq && cp->ref != 0 && !QFULL(cp->readq->next)) {
				tcprcvwin(cp);
				tcpacktimer(cp);
			}
		}
	}
}

void
tcpackproc(void *junk)
{
	Timer *t, *tp, *timeo;

	USED(junk);
	for(;;) {
		timeo = 0;

		qlock(&tl);
		for(t = timers;t != 0; t = tp) {
			tp = t->next;
 			if(t->state == TimerON) {
				t->count--;
				if(t->count == 0) {
					deltimer(t);
					t->state = TimerDONE;
					t->next = timeo;
					timeo = t;
				}
			}
		}
		qunlock(&tl);

		for(;;) {
			t = timeo;
			if(t == 0)
				break;

			timeo = t->next;
			if(t->state == TimerDONE)
			if(t->func)
				(*t->func)(t->arg);
		}
		tsleep(&Tcpack, return0, 0, MSPTICK);
	}
}

void
tcpgo(Timer *t)
{
	if(t == 0 || t->start == 0)
		return;

	qlock(&tl);
	t->count = t->start;
	if(t->state != TimerON) {
		t->state = TimerON;
		t->prev = 0;
		t->next = timers;
		if(t->next)
			t->next->prev = t;
		timers = t;
	}
	qunlock(&tl);
}

void
tcphalt(Timer *t)
{
	if(t == 0)
		return;

	qlock(&tl);
	if(t->state == TimerON)
		deltimer(t);
	t->state = TimerOFF;
	qunlock(&tl);
}
