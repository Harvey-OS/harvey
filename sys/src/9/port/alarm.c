#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

Alarms	alarms;
Rendez	alarmr;
Talarm	talarm;

void
alarmkproc(void *arg)
{
	Proc *rp;
	ulong now;

	USED(arg);

	for(;;){
		now = MACHP(0)->ticks;
		qlock(&alarms);
		while((rp = alarms.head) && rp->alarm <= now) {
			if(rp->alarm != 0L) {
				if(!canqlock(&rp->debug))
					break;

				if(!waserror()) {
					postnote(rp, 0, "alarm", NUser);
					poperror();
				}
				qunlock(&rp->debug);
				rp->alarm = 0L;
			}
			alarms.head = rp->palarm;
		}
		qunlock(&alarms);

		sleep(&alarmr, return0, 0);
	}
}

/*
 *  called every clock tick
 */
void
checkalarms(void)
{
	Proc *p;
	ulong now;

	p = alarms.head;
	now = MACHP(0)->ticks;

	if(p && p->alarm <= now)
		wakeup(&alarmr);

	if(talarm.list == 0 || canlock(&talarm) == 0)
		return;

	for(;;) {
		p = talarm.list;
		if(p == 0)
			break;

		if(p->twhen == 0) {
			talarm.list = p->tlink;
			p->trend = 0;
			continue;
		}
		if(now < p->twhen)
			break;
		wakeup(p->trend);
		talarm.list = p->tlink;
		p->trend = 0;
	}

	unlock(&talarm);
}

ulong
procalarm(ulong time)
{
	Proc **l, *f, *p;
	ulong when, old;

	p = u->p;
	if(p->alarm)
		old = TK2MS(p->alarm - MACHP(0)->ticks);
	else
		old = 0;
	if(time == 0) {
		p->alarm = 0;
		return old;
	}
	when = MS2TK(time)+MACHP(0)->ticks;

	qlock(&alarms);
	l = &alarms.head;
	for(f = *l; f; f = f->palarm) {
		if(p == f){
			*l = f->palarm;
			break;
		}
		l = &f->palarm;
	}

	p->palarm = 0;
	if(alarms.head) {
		l = &alarms.head;
		for(f = *l; f; f = f->palarm) {
			if(f->alarm > when) {
				p->palarm = f;
				*l = p;
				goto done;
			}
			l = &f->palarm;
		}
		*l = p;
	}
	else
		alarms.head = p;
done:
	p->alarm = when;
	qunlock(&alarms);

	return old;			
}
