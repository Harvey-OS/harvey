#include	<alef.h>
#include	"run.h"

void
ALEF_sched(Task *t)
{
	void *s;
	Task *me;
	Tdb *tdb;

	tdb = t->tdb;
	tdb->lock();
	/* Synchronous swap between tasks ? */
	if(PROC.tdb == tdb) {
		me = tdb->ctask;
		me->link = nil;
		if(tdb->runhd)
			tdb->runtl->link = me;
		else
			tdb->runhd = me;
		tdb->runtl = me;

		tdb->ctask = t;
		tdb->unlock();
		s = ALEF_switch(me, t, nil);
		if(s)
			free(s);
		return;
	}

	if(tdb->ctask == nil)
		tdb->ctask = t;
	else {
		t->link = nil;
		if(tdb->runhd)
			tdb->runtl->link = t;
		else
			tdb->runhd = t;
		tdb->runtl = t;
	}
	if(tdb->sleeper)
		rendezvous(tdb, 0);
	tdb->sleeper = 0;
	tdb->unlock();
}
