#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

static Alarms	alarms;
static Rendez	alarmr;

void
alarmkproc(void*)
{
	Proc *rp;
	ulong now;

	for(;;){
		now = MACHP(0)->ticks;
		qlock(&alarms);
		/*
		 * the odd test of now vs. rp->alarm is to cope with
		 * now wrapping around.
		 */
		while((rp = alarms.head) && (long)(now - rp->alarm) >= 0){
			if(rp->alarm != 0L){
				if(canqlock(&rp->debug)){
					if(!waserror()){
						postnote(rp, 0, "alarm", NUser);
						poperror();
					}
					qunlock(&rp->debug);
					rp->alarm = 0L;
				}else
					break;
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

	if(p && (long)(now - p->alarm) >= 0)
		wakeup(&alarmr);
}

ulong
procalarm(ulong time)
{
	Proc **l, *f;
	ulong when, old;

	if(up->alarm)
		old = tk2ms(up->alarm - MACHP(0)->ticks);
	else
		old = 0;
	if(time == 0) {
		up->alarm = 0;
		return old;
	}
	when = ms2tk(time)+MACHP(0)->ticks;
	if(when == 0)		/* ticks have wrapped to 0? */
		when = 1;	/* distinguish a wrapped alarm from no alarm */

	qlock(&alarms);
	l = &alarms.head;
	for(f = *l; f; f = f->palarm) {
		if(up == f){
			*l = f->palarm;
			break;
		}
		l = &f->palarm;
	}

	up->palarm = 0;
	if(alarms.head) {
		l = &alarms.head;
		for(f = *l; f; f = f->palarm) {
			if((long)(f->alarm - when) >= 0) {
				up->palarm = f;
				*l = up;
				goto done;
			}
			l = &f->palarm;
		}
		*l = up;
	}
	else
		alarms.head = up;
done:
	up->alarm = when;
	qunlock(&alarms);

	return old;
}
