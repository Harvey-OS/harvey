#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"
#include	"realtime.h"
#include	"../port/edf.h"

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

static int taskno;

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
	assert(t->csns.n == 0);
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
		seprintresources(buf, buf + sizeof(buf));
		qunlock(&edfschedlock);
		poperror();
		return readstr(offs, v, n, buf);
		break;
	case Qdump:
		p = buf;
		e = p + sizeof(buf);
		qlock(&edfschedlock);
		qunlock(&edfschedlock);
		seprint(p, e, "\n");
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
		if (t->csns.n){
			p = seprint(p, e, " resources='");
			p = seprintcsn(p, e, &t->csns);
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
	CSN *l;

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
				DEBUG("Task %d, T=%T\n", t->taskno, ticks2time(t->T));
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
				DEBUG("Task %d, D=%T\n", t->taskno, ticks2time(t->D));
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
				DEBUG("Task %d, C=%T\n", t->taskno, ticks2time(t->C));
			}else if (strcmp(a, "resources") == 0){
				if (v == nil)
					error("resources: value missing");
				edf->edfexpel(t);
				if (add < 0)
					error("can't remove resources yet");
				if (add == 0){
					List *l;
	
					while (l = t->csns.next) {
						r = l->i;
						assert(r);
						if (delist(&r->tasks, t))
							taskfree(t);
						if (delist(&t->csns, r))
							resourcefree(r);
					}
					assert(t->csns.n == 0);
					add = 1;
					USED(add);
				}
				v = parseresource(&t->csns, nil, v);
				if (v && *v)
					error("resources: parse error");
			}else if (strcmp(a, "acquire") == 0){
				if (v == nil)
					error("acquire: value missing");
				if ((r = resource(v, 0)) == nil)
					error("acquire: no such resource");
				for (l = (CSN*)t->csns.next; l; l = (CSN*)l->next){
					if (l->i == r){
DEBUG("l->p (0x%p) == t->curcsn (0x%p) && l->S (%T) != 0\n", l->p, t->curcsn, ticks2time(l->S));
						if(l->p == t->curcsn && l->S != 0)
						break;
					}
				}
				if (l == nil)
					error("acquire: no access or resource exhausted");
				edf->resacquire(t, l);
			}else if (strcmp(a, "release") == 0){
				if (v == nil)
					error("release: value missing");
				if ((r = resource(v, 0)) == nil)
					error("release: no such resource");
				if (t->curcsn->i != r)
					error("release: release not held or illegal release order");
				edf->resrelease(t);
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
