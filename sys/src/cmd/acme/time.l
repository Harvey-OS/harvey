#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

intern chan(Timer*)[100]	ctimer;
intern int					msecfd;

intern uint
msec()
{
	byte buf[16];
	int n;

	seek(msecfd, 0, 0);
	n = read(msecfd, buf, sizeof buf - 1);
	check n > 0;
	buf[n] = 0;
	return strtoui(buf, nil, 10);
}

intern void
timeproc()
{
	int i, nt, na, dt;
	Timer **t, *x;
	uint old, new;
	chan (int)[1] bug;

	alloc bug;
	bug <-= 0;
	timerpid = getpid();
	rfork(RFFDG);
	msecfd = open("/dev/msec", OREAD);
	check msecfd >= 0;
	t = nil;
	na = 0;
	nt = 0;
	old = msec();
	for(;;){
		sleep(1);	/* will sleep minimum incr */
		new = msec();
		dt = new-old;
		old = new;
		if(dt < 0)	/* timer wrapped; go around, losing a tick */
			continue;
		for(i=0; i<nt; i++){
			x = t[i];
			x->dt -= dt;
			if(x->dt <= 0){
				/*
				 * avoid possible deadlock if client is
				 * now sending on ctimer
				 */
				if(x->c?){
					x->c <-= 0;
					memmove(&t[i], &t[i+1], (nt-i-1)*sizeof t[0]);
					--nt;
					--i;
				}
			}
		}
		if(nt == 0){
			x = <-ctimer;
		gotit:
			if(nt == na){
				na += 10;
				t = realloc(t, na*sizeof(Timer*));
			}
			t[nt++] = x;
			old = msec();
		}else while(?ctimer)	/* avoid race between ? and <- */
			alt{
			case x = <-ctimer:
				goto gotit;
			case <-bug:
				bug <-= 0;
				break;
			}
	}
}

void
timerinit()
{
	alloc ctimer;
	proc timeproc();
}

/*
 * timeralloc() and timerfree() don't lock, so can only be
 * called from the main proc.
 */

intern	Timer *timer;

Timer*
timerstart(int dt)
{
	Timer *t;

	t = timer;
	if(t)
		timer = timer->next;
	else{
		t = malloc(sizeof(Timer));
		alloc t->c;
	}
	t->next = nil;
	t->dt = dt;
	ctimer <-= t;
	return t;
}

void
timerstop(Timer *t)
{
	t->next = timer;
	timer = t;
}

void
timerwaittask(Timer *timer)
{
	<-(timer->c);
	timerstop(timer);
}
