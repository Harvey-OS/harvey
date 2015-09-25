/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015  Giacomo Tesio
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

typedef struct PendingWakeup PendingWakeup;
struct PendingWakeup
{
	uint64_t time;
	Proc *p;
	PendingWakeup *next;
};

/* alarms: linked list, sorted by wakeup time, protected by qlock(&l)
 * wakeupafter inserts new items, forgivewakeup remove them,
 * awakekproc consume the expired ones and clearwakeups remove those
 * survived to their process.
 */
static PendingWakeup *alarms;
static QLock l;

static Rendez	awaker;

/*
 * Actually wakeup a process
 */
static void
wakeupProc(Proc *p)
{
	Mpl pl;
	Rendez *r;
	Proc *d, **l;

	/* this loop is to avoid lock ordering problems. */
	for(;;){
		pl = splhi();
		lock(&p->rlock);
		r = p->r;

		/* waiting for a wakeup? */
		if(r == nil)
			break;	/* no */

		/* try for the second lock */
		if(canlock(r)){
			if(p->state != Wakeme || r->_p != p)
				panic("wakeup: state %d %d %d", r->_p != p, p->r != r, p->state);
			p->r = nil;
			r->_p = nil;
			ready(p);
			unlock(r);
			break;
		}

		/* give other process time to get out of critical section and try again */
		unlock(&p->rlock);
		splx(pl);
		sched();
	}
	unlock(&p->rlock);
	splx(pl);

	if(p->state != Rendezvous){
		if(p->state == Semdown)
			ready(p);
		return;
	}
	/* Try and pull out of a rendezvous */
	lock(p->rgrp);
	if(p->state == Rendezvous) {
		p->rendval = ~0;
		l = &REND(p->rgrp, p->rendtag);
		for(d = *l; d; d = d->rendhash) {
			if(d == p) {
				*l = p->rendhash;
				break;
			}
			l = &d->rendhash;
		}
		ready(p);
	}
	unlock(p->rgrp);
}

void
awakekproc(void* v)
{
	Proc *up = externup();
	PendingWakeup *toAwake, *tail;
	uint64_t now;

	for(;;){
		now = sys->ticks;
		toAwake = nil;

		/* search for processes to wakeup */
		qlock(&l);
		tail = alarms;
		while(tail && tail->time <= now){
			toAwake = tail;
			--toAwake->p->wakeups;
			tail = tail->next;
		}
		if(toAwake != nil){
			toAwake->next = nil;
			toAwake = alarms;
			alarms = tail;
		}
		qunlock(&l);

		/* wakeup sleeping processes */
		while(toAwake != nil){
			/* debugged processes will miss wakeups, but the
			 * alternatives seem even worse
			 */
			if(canqlock(&toAwake->p->debug)){
				if(!waserror()){
					wakeupProc(toAwake->p);
					poperror();
				}
				qunlock(&toAwake->p->debug);
			}
			tail = toAwake->next;
			free(toAwake);
			toAwake = tail;
		}

		sleep(&awaker, return0, 0);
	}
}

/*
 * called on pexit
 */
void
clearwakeups(Proc *p)
{
	PendingWakeup *w, **next, *freelist, **fend;

	freelist = nil;

	/* find all PendingWakeup* to free and remove them from alarms */
	qlock(&l);
	if(p->wakeups){
		/* note: qlock(&l) && p->wakeups > 0 => alarms != nil */
		next = &alarms;
		fend = &freelist;
clearnext:
		w = *next;
		while (w != nil && w->p == p) {
			/* found one to remove */
			*fend = w;			/* append to freelist */
			*next = w->next;	/* remove from alarms */
			fend = &w->next;	/* move fend to end of freelist */
			*fend = nil;		/* clean the end of freelist */
			--p->wakeups;
			w = *next;			/* next to analyze */
		}
		if(p->wakeups){
			/* note: p->wakeups > 0 => w != nil
			 *       p->wakeups > 0 && w->p != p => w->next != nil
			 */
			next = &w->next;
			goto clearnext;
		}
	}
	qunlock(&l);

	/* free the found PendingWakeup* (out of the lock) */
	w = freelist;
	while(w != nil) {
		freelist = w->next;
		free(w);
		w = freelist;
	}
}

/*
 * called every clock tick
 */
void
checkwakeups(void)
{
	PendingWakeup *p;
	uint32_t now;

	p = alarms;
	now = sys->ticks;

	if(p && p->time <= now)
		wakeup(&awaker);
}

static int64_t
wakeupafter(int64_t ms)
{
	Proc *up = externup();
	PendingWakeup *w, *new, **last;
	int64_t when;

	when = ms2tk(ms) + sys->ticks + 2; /* +2 against round errors and cpu's clocks misalignment */
	new = mallocz(sizeof(PendingWakeup), 1);
	if(new == nil)
		return 0;
	new->p = up;
	new->time = when;

	qlock(&l);
	last = &alarms;
	for(w = *last; w != nil && w->time <= when; w = w->next) {
		last = &w->next;
	}
	new->next = w;
	*last = new;
	++up->wakeups;
	qunlock(&l);

	return -when;
}

static int64_t
forgivewakeup(int64_t time)
{
	Proc *up = externup();
	PendingWakeup *w, **last;

	qlock(&l);
	if(alarms == nil || up->wakeups == 0){
		qunlock(&l);
		return 0;	// nothing to do
	}

	last = &alarms;
	for(w = *last; w != nil && w->time < time; w = w->next) {
		last = &w->next;
	}
	while(w != nil && w->time == time && w->p != up){
		last = &w->next;
	}
	if(w == nil || w->time > time || w->p != up){
		/* wakeup not found */
		qunlock(&l);
		return 0;
	}

	*last = w->next;
	--up->wakeups;
	qunlock(&l);

	free(w);

	return -time;
}

int64_t
procawake(int64_t ms)
{
	if(ms == 0)
		return -sys->ticks; // nothing to do
	if(ms < 0)
		return forgivewakeup(-ms);
	return wakeupafter(ms);
}
