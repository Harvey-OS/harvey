#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"netif.h"

#include	<ptrace.h>


enum {
	Qdir,
	Qctl,
	Qdata,
};

enum {
	CMsize,
	CMtrace,
};

static Dirtab tracedir[]=
{
	".",			{Qdir, 0, QTDIR},	0,		DMDIR|0555,
	"ptracectl",	{Qctl},		0,		0664,
	"ptrace",		{Qdata},	0,		0440,
};

static Lock tlk;
static int topens;
static int tproduced, tconsumed, ntevents, ntmask;
static uchar *tevents;

static Chan*
traceattach(char *spec)
{
	return devattach(L'σ', spec);
}

static Walkqid*
tracewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, tracedir, nelem(tracedir), devgen);
}

static long
tracestat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, tracedir, nelem(tracedir), devgen);
}

static void
_ptrace(Proc *p, int etype, vlong ts, vlong arg)
{
	int i;
	uchar *te;

	if (p->trace == 0 || topens == 0)
		return;

	lock(&tlk);
	if (p->trace == 0 || topens == 0 || tproduced - tconsumed >= ntevents){
		unlock(&tlk);
		return;
	}
	if(ts == 0)
		ts = todget(nil);
	i = (tproduced&ntmask) * PTsize;
	te = &tevents[i];
	PBIT32(te, (int)p->pid);
	te += BIT32SZ;
	PBIT32(te, etype);
	te += BIT32SZ;
	PBIT32(te, m->machno);
	te += BIT32SZ;
	PBIT64(te, ts);
	te += BIT64SZ;
	PBIT64(te, arg);
	tproduced++;
	if(etype == SDead)
		p->trace = 0;
	unlock(&tlk);
}


static Chan*
traceopen(Chan *c, int omode)
{
	int q;

	q = c->qid.path;
	switch(q){
	case Qdata:
		lock(&tlk);
		if (waserror()){
			unlock(&tlk);
			nexterror();
		}
		if(topens > 0)
			error(Einuse);
		if(ntevents == 0)
			error("must set trace size first");
		if(up->trace != 0)
			error("a traced process can't open the trace device");
		tproduced = tconsumed = 0;
		proctrace = _ptrace;
		topens++;
		poperror();
		unlock(&tlk);
		break;
	}
	c = devopen(c, omode, tracedir, nelem(tracedir), devgen);
	return c;
}

static void
traceclose(Chan *c)
{
	int q;

	q = c->qid.path;
	switch(q){
	case Qdata:
		if(c->flag & COPEN){
			lock(&tlk);
			topens--;
			tproduced = tconsumed = 0;
			proctrace = nil;
			unlock(&tlk);
		}
	}
}

static long
traceread(Chan *c, void *a, long n, vlong offset)
{
	int q, xopens, xevents, xprod, xcons, navail, ne, i;
	char *s, *e;

	q = c->qid.path;
	switch(q){
	case Qdir:
		return devdirread(c, a, n, tracedir, nelem(tracedir), devgen);
		break;
	case Qctl:
		e = up->genbuf + sizeof up->genbuf;
		lock(&tlk);
		xopens = topens;
		xevents = ntevents;
		xprod = tproduced;
		xcons = tconsumed;
		unlock(&tlk);
		s = seprint(up->genbuf, e, "opens %d\n", xopens);
		s = seprint(s, e, "size %d\n", xevents);
		s = seprint(s, e, "produced %d\n", xprod);
		seprint(s, e, "consumed %d\n", xcons);
		return readstr(offset, a, n, up->genbuf);
		break;
	case Qdata:
		/* no locks! */
		navail = tproduced - tconsumed;
		if(navail <= 0)
			return 0;
		if(navail > n / PTsize)
			navail = n / PTsize;
		s = a;
		e = a;
		while(navail > 0) {
			if((tconsumed & ntmask) + navail > ntevents)
				ne = ntevents - (tconsumed & ntmask);
			else
				ne = navail;
			i = ne * PTsize;
			memmove(e, &tevents[(tconsumed & ntmask)*PTsize], i);

			tconsumed += ne;
			e += i;
			navail -= ne;
		}
		return e - s;
		break;
	default:
		error(Egreg);
	}
	error("not yet implemented");
	return -1;
}

static Cmdtab proccmd[] =
{
	CMsize,		"size",		2,
	CMtrace,	"trace",	0,
};

static long
tracewrite(Chan *c, void *a, long n, vlong)
{
	int i, q, sz, msk, pid;
	Cmdbuf *cb;
	Cmdtab *ct;
	uchar *nt;
	Proc *p;

	q = c->qid.path;
	switch(q){
	case Qctl:
		break;
	default:
		error(Egreg);
	}
	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, proccmd, nelem(proccmd));

	switch(ct->index){
	case CMsize:
		sz = atoi(cb->f[1]);
		if(sz == 0){
			lock(&tlk);
			ntevents = 0;
			ntmask = 0;
			unlock(&tlk);
		}
		msk = sz-1;
		if((sz&msk) != 0)
			error("wrong size. use a power of two.");
		if(sz > 512*1024)
			error("size is too large");
		lock(&tlk);
		if(waserror()){
			unlock(&tlk);
			nexterror();
		}
		if(sz > ntevents){
			nt = realloc(tevents, PTsize * sz);
			if(nt == nil)
				error("not enough memory");
			tevents = nt;
			ntevents = sz;
			ntmask = msk;
		}
		tproduced = 0;
		tconsumed = 0;
		poperror();
		unlock(&tlk);
		break;
	case CMtrace:
		if(cb->nf != 2 && cb->nf != 3)
			error(Ebadctl);
		pid = atoi(cb->f[1]);
		i = psindex(pid);
		if(i < 0)
			error(Eprocdied);
		p = psincref(i);
		if(p == nil)
			error(Eprocdied);
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			psdecref(p);
			nexterror();
		}
		if(p->pid != pid)
			error(Eprocdied);
		if(cb->nf == 2)
			p->trace ^= p->trace;
		else
			p->trace = (atoi(cb->f[2]) != 0);
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		break;
	}

	poperror();
	free(cb);
	return n;
}

Dev ptracedevtab = {
	L'σ',
	"ptrace",
	devreset,
	devinit,
	devshutdown,
	traceattach,
	tracewalk,
	tracestat,
	traceopen,
	devcreate,
	traceclose,
	traceread,
	devbread,
	tracewrite,
	devbwrite,
	devremove,
	devwstat,
};
