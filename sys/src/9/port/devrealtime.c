#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"
#include	"../port/devrealtime.h"
#include "../port/edf.h"

#pragma	varargck	type	"T"		vlong

/* debugging */
extern int			edfprint;
extern Edfinterface	realedf, *edf;

static Schedevent *events;
static int nevents, revent, wevent;
static Rendez eventr;
static QLock elock;
static Ref logopens;
static Ref debugopens;
static uvlong fasthz;	
static int taskno;

static int		timeconv(Fmt *);
static char *	parsetime(Time *, char *);

enum {
	Qistask = 0x10000,
	Qdir = 0,
	Qrealtime,
	Qclone,
	Qdebug,
	Qdump,
	Qlog,
	Qnblog,
	Qresrc,
	Qtask,
	Qtime,

	Nevents = 10000,
	Clockshift = 17,		// Good to about 10GHz clock and max. 5(s)
};

Dirtab schedrootdir[]={
	".",			{Qdir, 0, QTDIR},		0,	DMDIR|0555,
	"realtime",	{Qrealtime, 0, QTDIR},	0,	DMDIR|0555,
};

Dirtab scheddir[]={
	".",			{Qrealtime, 0, QTDIR},	0,	DMDIR|0555,
	"clone",		{Qclone},				0,	0666,
	"debug",		{Qdebug},				0,	0444,
	"dump",		{Qdump},				0,	0444,
	"log",		{Qlog},				0,	0444,	/* one open only */
	"nblog",		{Qnblog},				0,	0444,	/* nonblocking version of log */
	"resources",	{Qresrc},				0,	0444,
	"task",		{Qtask, 0, QTDIR},		0,	DMDIR|0555,
	"time",		{Qtime},				0,	0444,
};

static char *schedstatename[] = {
	[SRelease] =	"Release",
	[SRun] =		"Run",
	[SPreempt] =	"Preempt",
	[SBlock] =		"Block",
	[SResume] =	"Resume",
	[SDeadline] =	"Deadline",
	[SYield] =		"Yield",
	[SSlice] =		"Slice",
	[SExpel] =		"Expel",
};

Task *
findtask(int taskno)
{
	List *l;
	Task *t;

	for (l = tasks.next; l; l = l->next)
		if ((t = l->i) && t->taskno == taskno)
			return t;
	return nil;
}

static char*
dumptask(char *p, char *e, Task *t, Ticks now)
{
	vlong n;
	char c;

	p = seprint(p, e, "{%s, D%U, Δ%U,T%U, C%U, S%U",
		edfstatename[t->state], t->D, t->Delta, t->T, t->C, t->S);
	n = t->r - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", r%c%U", c, (uvlong)n);
	n = t->d - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", d%c%U", c, (uvlong)n);
	n = t->t - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", t%c%U}", c, (uvlong)n);
	return p;
}

static char*
dumpq(char *p, char *e, Taskq *q, Ticks now)
{
	Task *t;

	t = q->head;
	for(;;){
		if (t == nil)
			return seprint(p, e, "\n");
		p = dumptask(p, e, t, now);
		t = t->rnext;
		if (t)
			seprint(p, e, ", ");
	}
	return nil;
}

static Task *
taskinit(void)
{
	Dirtab *d;
	Task *t;

	t = malloc(sizeof(Task));
	if (t == nil)
		error("taskinit: malloc");
	d = &t->dir;
	if (up->user)
		kstrdup(&t->user, up->user);
	else
		kstrdup(&t->user, eve);
	t->state = EdfExpelled;
	t->taskno = ++taskno;
	snprint(d->name, sizeof d->name, "%d", t->taskno);
	mkqid(&d->qid, Qistask | t->taskno, 0, QTFILE);
	d->length = 0;
	d->perm = 0600;
	enlist(&tasks, t);
	incref(t);
	return t;
}

void
taskfree(Task *t)
{
	if (decref(t))
		return;
	assert(t->procs.n == 0);
	assert(t->res.n == 0);
	free(t->user);
	free(t);
}

/*
 * the zeroth element of the table MUST be the directory itself for ..
*/
int
schedgen(Chan *c, char*, Dirtab *, int, int i, Dir *dp)
{
	Dirtab *tab;
	int ntab;
	char *owner;
	ulong taskindex;
	Qid qid;
	Task *t;
	List *l;

	if((ulong)c->qid.path & Qistask){
		qlock(&edfschedlock);
		taskindex = (ulong)c->qid.path & (Qistask-1);
		if ((t = findtask(taskindex)) == nil){
			qunlock(&edfschedlock);
			return -1;
		}
	}else if((ulong)c->qid.path == Qtask){
		qlock(&edfschedlock);

		taskindex = i;
		SET(t);
		for (l = tasks.next; l; l = l->next)
			if ((t = l->i) && taskindex-- == 0)
				break;
		if (l == nil){
			qunlock(&edfschedlock);
			return -1;
		}
	}else {
		if((ulong)c->qid.path == Qdir){
			tab = schedrootdir;
			ntab = nelem(schedrootdir);
		}else{
			tab = scheddir;
			ntab = nelem(scheddir);
		}
		if(i != DEVDOTDOT){
			/* skip over the first element, that for . itself */
			i++;
			if(i >= ntab)
				return -1;
			tab += i;
		}
		devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
		return 1;
	}
	if(i == DEVDOTDOT){
		mkqid(&qid, Qtask, 0, QTDIR);
		devdir(c, qid, ".", 0, eve, 0555, dp);
	}else{
		owner = t->user;
		if (owner == nil)
			owner = eve;
		tab = &t->dir;
		devdir(c, tab->qid, tab->name, tab->length, owner, tab->perm, dp);
	}
	qunlock(&edfschedlock);
	return 1;
}

static void
_devrt(Task *t, Ticks t1, SEvent etype)
{
	if (logopens.ref == 0 || nevents == Nevents)
		return;

	if(edfprint)iprint("state %s\n", schedstatename[etype]);
	events[wevent].tid = t->taskno;
	events[wevent].ts = 0;
	if (t1)
		events[wevent].ts = ticks2time(t1);
	else
		events[wevent].ts = 0;
	events[wevent].etype = etype;

	if (!canqlock(&elock)) 
		return;

	wevent = (wevent + 1) % Nevents;
	if (nevents < Nevents)
		nevents++;
	else
		revent = (revent + 1) % Nevents;

	if(edfprint)iprint("wakesched\n");
	/* To avoid circular wakeup when used in combination with 
	 * EDF scheduling.
	 */
	if (eventr.p && eventr.p->state == Wakeme)
		wakeup(&eventr);

	qunlock(&elock);
}

static void
devrtinit(void)
{
	fmtinstall('T', timeconv);
	fmtinstall('U', timeconv);
	fastticks(&fasthz);
	devrt = _devrt;
	events = (Schedevent *)malloc(sizeof(Schedevent) * Nevents);
	assert(events);
	nevents = revent = wevent = 0;
	edf = &realedf;
}

static Chan *
devrtattach(char *param)
{
	return devattach('R', param);
}

static Walkqid *
devrtwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, schedgen);
}

static int
devrtstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, nil, 0, schedgen);
}

static Chan *
devrtopen(Chan *c, int mode)
{
	Task *t;

	switch ((ulong)c->qid.path){
	case Qlog:
	case Qnblog:
		if (mode != OREAD) 
			error(Eperm);
		incref(&logopens);
		if (logopens.ref > 1){
			decref(&logopens);
			error("already open");
		}
		break;
	case Qdebug:
		if (mode != OREAD) 
			error(Eperm);
		incref(&debugopens);
		if (debugopens.ref > 1){
			decref(&debugopens);
			error("already open");
		}
		break;
	case Qdump:
		if (mode != OREAD) 
			error(Eperm);
		break;
	case Qclone:
		if (mode == OREAD) 
			error(Eperm);
		edf->edfinit();
		qlock(&edfschedlock);
		/* open a new task */
		t = taskinit();
		c->qid.vers = t->taskno;
		qunlock(&edfschedlock);
		break;
	}
//	print("open %lux, mode %o\n", (ulong)c->qid.path, mode);
	return devopen(c, mode, nil, 0, schedgen);
}

static void
devrtclose(Chan *c)
{
	switch ((ulong)c->qid.path){
	case Qlog:
	case Qnblog:
		nevents = revent = wevent = 0;
		decref(&logopens);
		break;
	case Qdebug:
		nevents = revent = wevent = 0;
		decref(&debugopens);
		break;
	}
}

static int
eventsavailable(void *)
{
	return nevents > 0;
}

static long
devrtread(Chan *c, void *v, long n, vlong offs)
{
	char *p, *e;
	char buf[1024];
	long n0;
	int navail;
	Task *t;
	int s, i;
	Ticks now;
	Time tim;
	List *l;
	Resource *r;

	n0 = n;
//	print("schedread 0x%lux\n", (ulong)c->qid.path);
	buf[0] = '\0';
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, v, n, schedrootdir, nelem(schedrootdir), devgen);

	case Qrealtime:
		return devdirread(c, v, n, scheddir, nelem(scheddir), devgen);

	case Qtask:
		return devdirread(c, v, n, nil, 0, schedgen);

	case Qtime:
		if (n < sizeof(Time))
			error(Ebadarg);
		now = fastticks(nil);
		tim = ticks2time(now);
 		memmove(v, &tim, sizeof(Time));
		n -= sizeof(Ticks);
		if (n >= sizeof(Ticks)){
			memmove((char*)v + sizeof(Time), &now, sizeof(Ticks));
			n -= sizeof(Ticks);
		}
		if (n >= sizeof(Ticks)){
			memmove((char*)v + sizeof(Time) + sizeof(Ticks), &fasthz, sizeof(Ticks));
			n -= sizeof(Ticks);
		}
		break;

	case Qnblog:
		if (eventsavailable(nil))
			goto getevnt;
		break;

	case Qlog:

		while (!eventsavailable(nil))
			sleep(&eventr, eventsavailable, nil);
	getevnt:

		p = (char *)v;

		navail = nevents;
		if (navail > n / sizeof(Schedevent))
			navail = n / sizeof(Schedevent);
		n -= navail * sizeof(Schedevent);

		qlock(&elock);
		while (navail > 0) {
			int ncopy;

			ncopy = (revent + navail > Nevents)? Nevents - revent: navail;
			memmove(p, &events[revent], ncopy * sizeof(Schedevent));
			revent = (revent+ ncopy) % Nevents;
			p += ncopy * sizeof(Schedevent);
			navail -= ncopy;
			nevents -= ncopy;
		}
		qunlock(&elock);
		break;
	case Qresrc:
		qlock(&edfschedlock);
		if(waserror()){
			qunlock(&edfschedlock);
			nexterror();
		}
		p = buf;
		e = p + sizeof(buf);
		for (l = resources.next; l; l = l->next){
			r = l->i;
			assert(r);
			p = seprint(p, e, "name=%s", r->name);
			if (r->tasks.n){
				List *l;

				p = seprint(p, e, " tasks='");
				for (l = r->tasks.next; l; l = l->next) {
					if (l != r->tasks.next)
						p = seprint(p, e, " ");
					p = seprint(p, e, "%d", ((Task *)l->i)->taskno);
				}
				p = seprint(p, e, "'");
			}
			if (r->Delta)
				p = seprint(p, e, " Δ=%T", ticks2time(r->Delta));
			else if (r->testDelta)
				p = seprint(p, e, " testΔ=%T", ticks2time(r->testDelta));
			p = seprint(p, e, "\n");
		}
		qunlock(&edfschedlock);
		poperror();
		return readstr(offs, v, n, buf);
		break;
	case Qdump:
		p = buf;
		e = p + sizeof(buf);
		qlock(&edfschedlock);
		qunlock(&edfschedlock);
		p = seprint(p, e, "\n");
		return readstr(offs, v, n, buf);
	case Qdebug:
		p = buf;
		e = p + sizeof(buf);
		ilock(&edflock);
		now = fastticks(nil);
		for (i = 0; i < conf.nmach; i++){
			p = seprint(p, e, "edfstack[%d]\n", i);
			p = dumpq(p, e, edfstack + i, now);
		}
		p = seprint(p, e, "qreleased\n");
		p = dumpq(p, e, &qreleased, now);
		p = seprint(p, e, "qwaitrelease\n");
		p = dumpq(p, e, &qwaitrelease, now);
		p = seprint(p, e, "qextratime\n");
		dumpq(p, e, &qextratime, now);
		iunlock(&edflock);
		return readstr(offs, v, n, buf);
	case Qclone:
		s = c->qid.vers;
		goto common;
	default:
		if ((c->qid.path & Qistask) == 0)
			error(Enonexist);
		s = (ulong)c->qid.path & (Qistask - 1);
	common:
		qlock(&edfschedlock);
		t = findtask(s);
		if (t == nil){
			qunlock(&edfschedlock);
			error(Enonexist);
		}
		p = buf;
		e = p + sizeof(buf);
		p = seprint(p, e, "task=%d", s);
		p = seprint(p, e, " state=%s", edfstatename[t->state]);
		if (t->T)
			p = seprint(p, e, " T=%T", ticks2time(t->T));
		if (t->D)
			p = seprint(p, e, " D=%T", ticks2time(t->D));
		if (t->C)
			p = seprint(p, e, " C=%T", ticks2time(t->C));
		if (t->Delta)
			p = seprint(p, e, " Δ=%T", ticks2time(t->Delta));
		else if (t->testDelta)
			p = seprint(p, e, " testΔ=%T", ticks2time(t->testDelta));
		p = seprint(p, e, " yieldonblock=%d", (t->flags & Verbose) != 0);
		if (t->res.n){
			p = seprint(p, e, " resources='");
			for (l = t->res.next; l; l = l->next){
				r = l->i;
				assert(r);
				if (l != t->res.next)
					p = seprint(p, e, " ");
				p = seprint(p, e, "%s", r->name);
			}
			p = seprint(p, e, "'");
		}
		if (t->procs.n){
			p = seprint(p, e, " procs='");
			for (l = t->procs.next; l; l = l->next){
				Proc *pr = l->i;
				assert(pr);
				if (l != t->procs.next)
					p = seprint(p, e, " ");
				p = seprint(p, e, "%lud", pr->pid);
			}
			p = seprint(p, e, "'");
		}
		if (t->periods)
			p = seprint(p, e, " n=%lud", t->periods);
		if (t->missed)
			p = seprint(p, e, " m=%lud", t->missed);
		if (t->preemptions)
			p = seprint(p, e, " p=%lud", t->preemptions);
		if (t->total)
			p = seprint(p, e, " t=%T", ticks2time(t->total));
		if (t->aged)
			p = seprint(p, e, " c=%T", ticks2time(t->aged));
		seprint(p, e, "\n");
		qunlock(&edfschedlock);
		return readstr(offs, v, n, buf);
	}
	return n0 - n;
}

static Resource *
resource(char *name, int add)
{
	Resource *r;
	List *l;

	for (l = resources.next; l; l = l->next)
		if ((r = l->i) && strcmp(r->name, name) == 0)
			return r;
	if (add < 0)
		return nil;
	r = malloc(sizeof(Resource));
	if (r == nil)
		error("resource: malloc");
	kstrdup(&r->name, name);
	enlist(&resources, r);
	return r;
}

void
resourcefree(Resource *r)
{
	if (decref(r))
		return;
	delist(&resources, r);
	assert(r->tasks.n == 0);
	free(r->name);
	free(r);
}

static void
removetask(Task *t)
{
	Proc *p;
	Resource *r;
	List *l;

	edf->edfexpel(t);
	while (l = t->procs.next){
		p = l->i;
		assert(p);
		p->task = nil;
		delist(&t->procs, p);
	}
	while (p = t->runq.head){
		/* put runnable procs on regular run queue */
		t->runq.head = p->rnext;
		ready(p);
		t->runq.n--;
	}
	t->runq.tail = nil;
	assert(t->runq.n == 0);
	while (l = t->res.next){
		r = l->i;
		if (delist(&r->tasks, t))
			taskfree(t);
		if (delist(&t->res, r))
			resourcefree(r);
	}
	free(t->user);
	t->user = nil;
	t->state = EdfUnused;
	if (delist(&tasks, t))
		taskfree(t);
}

static long
devrtwrite(Chan *c, void *va, long n, vlong)
{
	char *a, *v, *e, *args[16], *rargs[16], buf[512];
	int i, j, s, nargs, nrargs, add;
	Resource *r;
	Task *t;
	Ticks ticks;
	Time time;
	long pid;
	Proc *p;

	a = va;
	if (c->mode == OREAD)
		error(Eperm);
	switch((ulong)c->qid.path){
	case Qclone:
		s = c->qid.vers;
		goto common;
	default:
		if ((c->qid.path & Qistask) == 0)
			error(Enonexist);
		s = (ulong)c->qid.path & (Qistask - 1);
	common:
		qlock(&edfschedlock);
		if (waserror()){
			qunlock(&edfschedlock);
			nexterror();
		}
		t = findtask(s);
		if (t == nil)
			error(Enonexist);
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		nargs = tokenize(buf, args, nelem(args));
		for (i = 0; i < nargs; i++){
			a = args[i];
			add  = 0;
			if (v = strchr(a, '=')){
				*v = '\0';
				if (v != a && v[-1] == '+'){
					add = 1;
					v[-1] = '\0';
				} else if (v != a && v[-1] == '-'){
					add = -1;
					v[-1] = '\0';
				}
				v++;
			}
			if (strcmp(a, "T") == 0){
				if (e=parsetime(&time, v))
					error(e);
				ticks = time2ticks(time);
				edf->edfexpel(t);
				switch(add){
				case -1:
					if (ticks > t->T)
						t->T = 0;
					else
						t->T -= ticks;
					break;
				case 0:
					t->T = ticks;
					break;
				case 1:
					t->T += ticks;
					break;
				}
				if (t->T < time2ticks(10000000/HZ))
					error("period too short");
			}else if (strcmp(a, "D") == 0){
				if (e=parsetime(&time, v))
					error(e);
				ticks = time2ticks(time);
				edf->edfexpel(t);
				switch(add){
				case -1:
					if (ticks > t->D)
						t->D = 0;
					else
						t->D -= ticks;
					break;
				case 0:
					t->D = ticks;
					break;
				case 1:
					t->D += ticks;
					break;
				}
			}else if (strcmp(a, "C") == 0){
				if (e=parsetime(&time, v))
					error(e);
				ticks = time2ticks(time);
				edf->edfexpel(t);
				switch(add){
				case -1:
					if (ticks > t->C)
						t->C = 0;
					else
						t->C -= ticks;
					break;
				case 0:
					t->C = ticks;
					break;
				case 1:
					t->C += ticks;
					break;
				}
				if (t->C < time2ticks(10000000/HZ))
					error("cost too small");
			}else if (strcmp(a, "resources") == 0){
				if (v == nil)
					error("resources: value missing");
				edf->edfexpel(t);
				if (add == 0){
					List *l;

					while (l = t->res.next) {
						r = l->i;
						assert(r);
						if (delist(&r->tasks, t))
							taskfree(t);
						if (delist(&t->res, r))
							resourcefree(r);
					}
					assert(t->res.n == 0);
					add = 1;
				}
				nrargs = tokenize(v, rargs, nelem(rargs));
				for (j = 0; j < nrargs; j++)
					if (r = resource(rargs[j], add)){
						if (add > 0){
							if (enlist(&r->tasks, t))
								incref(t);
							if (enlist(&t->res, r))
								incref(r);
						}else{
							if (delist(&r->tasks, t))
								taskfree(t);
							if (delist(&t->res, r))
								resourcefree(r);
						}
					}else
						error("resource not found");
			}else if (strcmp(a, "procs") == 0){
				if (v == nil)
					error("procs: value missing");
				if (add <= 0){
					edf->edfexpel(t);
				}
				if (add == 0){
					List *l;

					while (l = t->procs.next){
						p = l->i;
						assert(p->task == t);
						delist(&t->procs, p);
						p->task = nil;
					}
					add = 1;
				}
				nrargs = tokenize(v, rargs, nelem(rargs));
				for (j = 0; j < nrargs; j++){
					if (strcmp("self", rargs[j]) == 0){
						p = up;
					}else{
						pid = atoi(rargs[j]);
						if (pid <= 0)
							error("bad process number");
						s = procindex(pid);
						if(s < 0)
							error("no such process");
						p = proctab(s);
					}
					if(p->task && p->task != t)
						error("proc belongs to another task");
					if (add > 0){
						enlist(&t->procs, p);
						p->task = t;
					}else{
						delist(&t->procs, p);
						p->task = nil;
					}
				}
			}else if (strcmp(a, "admit") == 0){
				if (e = edf->edfadmit(t))
					error(e);
			}else if (strcmp(a, "expel") == 0){
				edf->edfexpel(t);
			}else if (strcmp(a, "remove") == 0){
				removetask(t);
				poperror();
				qunlock(&edfschedlock);
				return n;	/* Ignore any subsequent commands */
			}else if (strcmp(a, "verbose") == 0){
				if (t->flags & Verbose)
					t->flags &= ~Verbose;
				else
					t->flags |= Verbose;
			}else if (strcmp(a, "yieldonblock") == 0){
				if (v == nil)
					error("yieldonblock: value missing");
				if (add != 0)
					error("yieldonblock: cannot increment/decrement");
				if (atoi(v) == 0)
					t->flags &= ~Useblocking;
				else
					t->flags |= Useblocking;
			}else if (strcmp(a, "yield") == 0){
				if (edf->isedf(up) && up->task == t){
					edf->edfdeadline(up);	/* schedule next release */
					qunlock(&edfschedlock);
					sched();
					qlock(&edfschedlock);
				}else
					error("yield outside task");
			}else
				error("unrecognized command");
		}
		poperror();
		qunlock(&edfschedlock);
	}
	return n;
}

static void
devrtremove(Chan *c)
{
	int s;
	Task *t;

	if ((c->qid.path & Qistask) == 0)
		error(Eperm);
	s = (ulong)c->qid.path & (Qistask - 1);
	t = findtask(s);
	if (t == nil)
		error(Enonexist);
	qlock(&edfschedlock);
	if (waserror()){
		qunlock(&edfschedlock);
		nexterror();
	}
	removetask(t);
	poperror();
	qunlock(&edfschedlock);
}

Dev realtimedevtab = {
	'R',
	"scheduler",

	devreset,
	devrtinit,
	devshutdown,
	devrtattach,
	devrtwalk,
	devrtstat,
	devrtopen,
	devcreate,
	devrtclose,
	devrtread,
	devbread,
	devrtwrite,
	devbwrite,
	devrtremove,
	devwstat,
};

static int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	Time t;
	Ticks ticks;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		ticks = va_arg(f->args, Ticks);
		t = ticks2time(ticks);
		break;
	case 'T':		// Time in nanoseconds
		t = va_arg(f->args, Time);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t < 0) {
		sign = "-";
		t = -t;
	}
	else
		sign = "";
	if (t > Onesecond)
		sprint(buf, "%s%d.%.3ds", sign, (int)(t / Onesecond), (int)(t % Onesecond)/1000000);
	else if (t > Onemillisecond)
		sprint(buf, "%s%d.%.3dms", sign, (int)(t / Onemillisecond), (int)(t % Onemillisecond)/1000);
	else if (t > Onemicrosecond)
		sprint(buf, "%s%d.%.3dµs", sign, (int)(t / Onemicrosecond), (int)(t % Onemicrosecond));
	else
		sprint(buf, "%s%dns", sign, (int)t);
	return fmtstrcpy(f, buf);
}

static char *
parsetime(Time *rt, char *s)
{
	uvlong ticks;
	ulong l;
	char *e, *p;
	static int p10[] = {100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	if (s == nil)
		return("missing value");
	ticks=strtoul(s, &e, 10);
	if (*e == '.'){
		p = e+1;
		l = strtoul(p, &e, 10);
		if(e-p > nelem(p10))
			return "too many digits after decimal point";
		if(e-p == 0)
			return "ill-formed number";
		l *= p10[e-p-1];
	}else
		l = 0;
	if (*e){
		if(strcmp(e, "s") == 0)
			ticks = 1000000000 * ticks + l;
		else if (strcmp(e, "ms") == 0)
			ticks = 1000000 * ticks + l/1000;
		else if (strcmp(e, "µs") == 0 || strcmp(e, "us") == 0)
			ticks = 1000 * ticks + l/1000000;
		else if (strcmp(e, "ns") != 0)
			return "unrecognized unit";
	}
	*rt = ticks;
	return nil;
}

int
putlist(Head *h, List *l)
{
	l->next = h->next;
	h->next = l;
	h->n++;
	return 1;
}

int
enlist(Head *h, void *i)
{
	List *l;

	for (l = h->next; l; l = l->next)
		if (l->i == i){
			/* already on the list */
			return 0;
		}
	l = malloc(sizeof(List));
	if (l == nil)
		panic("malloc in enlist");
	l->i = i;
	l->next = h->next;
	h->next = l;
	h->n++;
	return 1;
}

int
delist(Head *h, void *i)
{
	List *l, **p;

	for (p = &h->next; l = *p; p = &l->next)
		if (l->i == i){
			*p = l->next;
			free(l);
			h->n--;
			return 1;
		}
	return 0;
}

void *
findlist(Head *h, void *i)
{
	List *l;

	for (l = h->next; l; l = l->next)
		if (l->i == i)
			return i;
	return nil;
}
