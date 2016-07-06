/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/edf.h"
#include	"tos.h"
#include	<trace.h>
#include	"ureg.h"

enum
{
	Qdir,
	Qtrace,
	Qtracepids,
	Qargs,
	Qctl,
	Qfd,
	Qfpregs,
	Qgdbregs,
	Qkregs,
	Qmem,
	Qnote,
	Qnoteid,
	Qnotepg,
	Qns,
	Qproc,
	Qregs,
	Qsegment,
	Qstatus,
	Qtext,
	Qwait,
	Qprofile,
	Qsyscall,
	Qcore,
	Qtls,
	Qpager,
};

enum
{
	CMclose,
	CMclosefiles,
	CMfixedpri,
	CMhang,
	CMkill,
	CMnohang,
	CMnoswap,
	CMpri,
	CMprivate,
	CMprofile,
	CMstart,
	CMstartstop,
	CMstartsyscall,
	CMstop,
	CMwaitstop,
	CMwired,
	CMtrace,
	/* real time */
	CMperiod,
	CMdeadline,
	CMcost,
	CMsporadic,
	CMdeadlinenotes,
	CMadmit,
	CMextra,
	CMexpel,
	CMevent,
	CMcore,
};

enum{
	Nevents = 0x4000,
	Emask = Nevents - 1,
	Ntracedpids = 1024,
};

#define STATSIZE	(2*KNAMELEN+NUMSIZE + 9*NUMSIZE + 6*NUMSIZE + 2*NUMSIZE + 1)

/*
 * Status, fd, and ns are left fully readable (0444) because of their use in debugging,
 * particularly on shared servers.
 * Arguably, ns and fd shouldn't be readable; if you'd prefer, change them to 0000
 */
Dirtab procdir[] =
{
	"args",		{Qargs},	0,			0660,
	"ctl",		{Qctl},		0,			0000,
	"fd",		{Qfd},		0,			0444,
	"fpregs",	{Qfpregs},	0,			0000,
	"kregs",	{Qkregs},	sizeof(Ureg),		0600,
	"mem",		{Qmem},		0,			0000,
	"note",		{Qnote},	0,			0000,
	"noteid",	{Qnoteid},	0,			0664,
	"notepg",	{Qnotepg},	0,			0000,
	"ns",		{Qns},		0,			0444,
	"proc",		{Qproc},	0,			0400,
	"gdbregs",	{Qgdbregs},	GDB_NUMREGBYTES,	0000,
	"regs",		{Qregs},	sizeof(Ureg),		0000,
	"segment",	{Qsegment},	0,			0444,
	"status",	{Qstatus},	STATSIZE,		0444,
	"text",		{Qtext},	0,			0000,
	"wait",		{Qwait},	0,			0400,
	"profile",	{Qprofile},	0,			0400,
	"syscall",	{Qsyscall},	0,			0400,
	"core",		{Qcore},	0,			0444,
	"tls",		{Qtls},		0,			0600,
	"pager",	{Qpager},	0,			0600|DMEXCL,

};

static
Cmdtab proccmd[] = {
	CMclose,		"close",		2,
	CMclosefiles,		"closefiles",		1,
	CMfixedpri,		"fixedpri",		2,
	CMhang,			"hang",			1,
	CMnohang,		"nohang",		1,
	CMnoswap,		"noswap",		1,
	CMkill,			"kill",			1,
	CMpri,			"pri",			2,
	CMprivate,		"private",		1,
	CMprofile,		"profile",		1,
	CMstart,		"start",		1,
	CMstartstop,		"startstop",		1,
	CMstartsyscall,		"startsyscall",		1,
	CMstop,			"stop",			1,
	CMwaitstop,		"waitstop",		1,
	CMwired,		"wired",		2,
	CMtrace,		"trace",		0,
	CMperiod,		"period",		2,
	CMdeadline,		"deadline",		2,
	CMcost,			"cost",			2,
	CMsporadic,		"sporadic",		1,
	CMdeadlinenotes,	"deadlinenotes",	1,
	CMadmit,		"admit",		1,
	CMextra,		"extra",		1,
	CMexpel,		"expel",		1,
	CMevent,		"event",		1,
	CMcore,			"core",			2,
};

/*
 * Qids are, in path:
 *	 4 bits of file type (qids above)
 *	23 bits of process slot number + 1
 *	     in vers,
 *	32 bits of pid, for consistency checking
 * If notepg, c->pgrpid.path is pgrp slot, .vers is noteid.
 */
#define	QSHIFT	5	/* location in qid of proc slot # */
#define	SLOTBITS 23	/* number of bits in the slot */
#define	QIDMASK	((1<<QSHIFT)-1)
#define	SLOTMASK	((1<<SLOTBITS)-1 << QSHIFT)

#define QID(q)		((((uint32_t)(q).path)&QIDMASK)>>0)
#define SLOT(q)		(((((uint32_t)(q).path)&SLOTMASK)>>QSHIFT)-1)
#define PID(q)		((q).vers)
#define NOTEID(q)	((q).vers)

static void	procctlreq(Proc*, char*, int);
static int	procctlmemio(Proc*, uintptr_t, int, void*, int);
static Chan*	proctext(Chan*, Proc*);
static Segment* txt2data(Proc*, Segment*);
static int	procstopped(void*);
static void	mntscan(Mntwalk*, Proc*);

static Traceevent *tevents;
static char *tpids, *tpidsc, *tpidse;
static Lock tlock;
static int topens;
static int tproduced, tconsumed;
static void notrace(Proc*, int, int64_t);

void (*proctrace)(Proc*, int, int64_t) = notrace;

static void
profclock(Ureg *ur, Timer *ti)
{
	Proc *up = externup();
	Tos *tos;

	if(up == nil || up->state != Running)
		return;

	/* user profiling clock */
	if(userureg(ur)){
		tos = (Tos*)(USTKTOP-sizeof(Tos));
		tos->clock += TK2MS(1);
		segclock(userpc(ur));
	}
}

static int
procgen(Chan *c, char *name, Dirtab *tab, int j, int s, Dir *dp)
{
	Proc *up = externup();
	Qid qid;
	Proc *p;
	char *ename;
	int pid, sno;
	uint32_t path, perm, len;

	if(s == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, "#p", 0, eve, 0555, dp);
		return 1;
	}

	if(c->qid.path == Qdir){
		if(s == 0){
			strcpy(up->genbuf, "trace");
			mkqid(&qid, Qtrace, -1, QTFILE);
			devdir(c, qid, up->genbuf, 0, eve, 0444, dp);
			return 1;
		}
		if(s == 1){
			strcpy(up->genbuf, "tracepids");
			mkqid(&qid, Qtracepids, -1, QTFILE);
			devdir(c, qid, up->genbuf, 0, eve, 0444, dp);
			return 1;
		}
		s -= 2;
		if(name != nil){
			/* ignore s and use name to find pid */
			pid = strtol(name, &ename, 10);
			if(pid<=0 || ename[0]!='\0')
				return -1;
			s = psindex(pid);
			if(s < 0)
				return -1;
		}
		else if(s >= conf.nproc)
			return -1;

		if((p = psincref(s)) == nil || (pid = p->pid) == 0)
			return 0;
		snprint(up->genbuf, sizeof up->genbuf, "%u", pid);
		/*
		 * String comparison is done in devwalk so
		 * name must match its formatted pid.
		 */
		if(name != nil && strcmp(name, up->genbuf) != 0)
			return -1;
		mkqid(&qid, (s+1)<<QSHIFT, pid, QTDIR);
		devdir(c, qid, up->genbuf, 0, p->user, DMDIR|0555, dp);
		psdecref(p);
		return 1;
	}
	if(c->qid.path == Qtrace){
		strcpy(up->genbuf, "trace");
		mkqid(&qid, Qtrace, -1, QTFILE);
		devdir(c, qid, up->genbuf, 0, eve, 0444, dp);
		return 1;
	}
	if(c->qid.path == Qtracepids){
		strcpy(up->genbuf, "tracepids");
		mkqid(&qid, Qtrace, -1, QTFILE);
		devdir(c, qid, up->genbuf, 0, eve, 0444, dp);
		return 1;
	}
	if(s >= nelem(procdir))
		return -1;
	if(tab)
		panic("procgen");

	tab = &procdir[s];
	path = c->qid.path&~(((1<<QSHIFT)-1));	/* slot component */

	if((p = psincref(SLOT(c->qid))) == nil)
		return -1;
	perm = tab->perm;
	if(perm == 0)
		perm = p->procmode;
	else	/* just copy read bits */
		perm |= p->procmode & 0444;

	len = tab->length;
	switch(QID(c->qid)) {
	case Qwait:
		len = p->nwait;	/* incorrect size, but >0 means there's something to read */
		break;
	case Qprofile: /* TODO(aki): test this */
		len = 0;
		for(sno = 0; sno < NSEG; sno++){
			if(p->seg[sno] != nil && (p->seg[sno]->type & SG_EXEC) != 0){
				Segment *s;
				s = p->seg[sno];
				if(s->profile)
					len += ((s->top-s->base)>>LRESPROF) * sizeof s->profile[0];
			}
		}
		break;
	}

	mkqid(&qid, path|tab->qid.path, c->qid.vers, QTFILE);
	devdir(c, qid, tab->name, len, p->user, perm, dp);
	psdecref(p);
	return 1;
}

static void
notrace(Proc* p, int n, int64_t m)
{
}
static Lock tlck;

static void
_proctrace(Proc* p, int etype, int64_t ts)
{
	Traceevent *te;
	int tp;

	ilock(&tlck);
	if (p->trace == 0 || topens == 0 ||
		tproduced - tconsumed >= Nevents){
		iunlock(&tlck);
		return;
	}
	tp = tproduced++;
	iunlock(&tlck);

	te = &tevents[tp&Emask];
	te->pid = p->pid;
	te->etype = etype;
	if (ts == 0)
		te->time = todget(nil);
	else
		te->time = ts;
	te->core = machp()->machno;
}

void
proctracepid(Proc *p)
{
	if(p->trace == 1 && proctrace != notrace){
		p->trace = 2;
		ilock(&tlck);
		tpidsc = seprint(tpidsc, tpidse, "%d %s\n", p->pid, p->text);
		iunlock(&tlck);
	}
}

static void
procinit(void)
{
	if(conf.nproc >= (SLOTMASK>>QSHIFT) - 1)
		print("warning: too many procs for devproc\n");
	addclock0link((void (*)(void))profclock, 113);	/* Relative prime to HZ */
}

static Chan*
procattach(char *spec)
{
	return devattach('p', spec);
}

static Walkqid*
procwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, procgen);
}

static int32_t
procstat(Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, 0, 0, procgen);
}

/*
 *  none can't read or write state on other
 *  processes.  This is to contain access of
 *  servers running as none should they be
 *  subverted by, for example, a stack attack.
 */
static void
nonone(Proc *p)
{
	Proc *up = externup();
	if(p == up)
		return;
	if(strcmp(up->user, "none") != 0)
		return;
	if(iseve())
		return;
	error(Eperm);
}

static Chan*
procopen(Chan *c, int omode)
{
	Proc *up = externup();
	Proc *p;
	Pgrp *pg;
	Chan *tc;
	int pid;

	if(c->qid.type & QTDIR)
		return devopen(c, omode, 0, 0, procgen);

	if(QID(c->qid) == Qtrace){
		if (omode != OREAD)
			error(Eperm);
		lock(&tlock);
		if (waserror()){
			unlock(&tlock);
			nexterror();
		}
		if (topens > 0)
			error("already open");
		topens++;
		if (tevents == nil){
			tevents = (Traceevent*)malloc(sizeof(Traceevent) * Nevents);
			if(tevents == nil)
				error(Enomem);
			tpids = malloc(Ntracedpids * 20);
			if(tpids == nil){
				free(tpids);
				tpids = nil;
				error(Enomem);
			}
			tpidsc = tpids;
			tpidse = tpids + Ntracedpids * 20;
			*tpidsc = 0;
			tproduced = tconsumed = 0;
		}
		proctrace = _proctrace;
		poperror();
		unlock(&tlock);

		c->mode = openmode(omode);
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	if(QID(c->qid) == Qtracepids){
		if (omode != OREAD)
			error(Eperm);
		c->mode = openmode(omode);
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	if((p = psincref(SLOT(c->qid))) == nil)
		error(Eprocdied);
	qlock(&p->debug);
	if(waserror()){
		qunlock(&p->debug);
		psdecref(p);
		nexterror();
	}
	pid = PID(c->qid);
	if(p->pid != pid)
		error(Eprocdied);

	omode = openmode(omode);

	switch(QID(c->qid)){
	case Qtext:
		if(omode != OREAD)
			error(Eperm);
		tc = proctext(c, p);
		tc->offset = 0;
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		cclose(c);
		return tc;

	case Qproc:
	case Qkregs:
	case Qsegment:
	case Qprofile:
	case Qfd:
		if(omode != OREAD)
			error(Eperm);
		break;

	case Qnote:
		if(p->privatemem)
			error(Eperm);
		break;

	case Qmem:
	case Qctl:
		if(p->privatemem)
			error(Eperm);
		nonone(p);
		break;

	case Qtls:
		if(p->pid != up->pid)
			error(Eperm);
		nonone(p);
		break;

	case Qargs:
	case Qnoteid:
	case Qstatus:
	case Qwait:
	case Qgdbregs:
	case Qregs:
	case Qfpregs:
	case Qsyscall:
	case Qcore:
		nonone(p);
		break;

	case Qpager:
		p->resp = qopen(1024, Qmsg, nil, 0);
		p->req = qopen(1024, Qmsg, nil, 0);
		print("p %d sets resp %p req %p\n", p->pid, p->resp, p->req);
		c->aux = p;
		break;

	case Qns:
		if(omode != OREAD)
			error(Eperm);
		c->aux = malloc(sizeof(Mntwalk));
		break;

	case Qnotepg:
		nonone(p);
		pg = p->pgrp;
		if(pg == nil)
			error(Eprocdied);
		if(omode!=OWRITE || pg->pgrpid == 1)
			error(Eperm);
		c->pgrpid.path = pg->pgrpid+1;
		c->pgrpid.vers = p->noteid;
		break;

	default:
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		pprint("procopen %#llux\n", c->qid.path);
		error(Egreg);
	}

	/* Affix pid to qid */
	if(p->state != Dead)
		c->qid.vers = p->pid;

	/* make sure the process slot didn't get reallocated while we were playing */
	coherence();
	if(p->pid != pid)
		error(Eprocdied);

	tc = devopen(c, omode, 0, 0, procgen);
	poperror();
	qunlock(&p->debug);
	psdecref(p);

	return tc;
}

static int32_t
procwstat(Chan *c, uint8_t *db, int32_t n)
{
	Proc *up = externup();
	Proc *p;
	Dir *d;

	if(c->qid.type & QTDIR)
		error(Eperm);

	if(QID(c->qid) == Qtrace)
		return devwstat(c, db, n);

	if((p = psincref(SLOT(c->qid))) == nil)
		error(Eprocdied);
	nonone(p);
	d = nil;
	qlock(&p->debug);
	if(waserror()){
		qunlock(&p->debug);
		psdecref(p);
		free(d);
		nexterror();
	}

	if(p->pid != PID(c->qid))
		error(Eprocdied);

	if(strcmp(up->user, p->user) != 0 && strcmp(up->user, eve) != 0)
		error(Eperm);

	d = smalloc(sizeof(Dir)+n);
	n = convM2D(db, n, &d[0], (char*)&d[1]);
	if(n == 0)
		error(Eshortstat);
	if(!emptystr(d->uid) && strcmp(d->uid, p->user) != 0){
		if(strcmp(up->user, eve) != 0)
			error(Eperm);
		else
			kstrdup(&p->user, d->uid);
	}
	if(d->mode != (uint32_t)~0UL)
		p->procmode = d->mode&0777;

	poperror();
	qunlock(&p->debug);
	psdecref(p);
	free(d);

	return n;
}


static int32_t
procoffset(int32_t offset, char *va, int *np)
{
	if(offset > 0) {
		offset -= *np;
		if(offset < 0) {
			memmove(va, va+*np+offset, -offset);
			*np = -offset;
		}
		else
			*np = 0;
	}
	return offset;
}

static int
procqidwidth(Chan *c)
{
	char buf[32];

	return sprint(buf, "%lu", c->qid.vers);
}

int
procfdprint(Chan *c, int fd, int w, char *s, int ns)
{
	int n;

	if(w == 0)
		w = procqidwidth(c);
	n = snprint(s, ns, "%3d %.2s %C %4ud (%.16llux %*lud %.2ux) %5ld %8lld %s\n",
		fd,
		&"r w rw"[(c->mode&3)<<1],
		c->dev->dc, c->devno,
		c->qid.path, w, c->qid.vers, c->qid.type,
		c->iounit, c->offset, c->path->s);
	return n;
}

static int
procfds(Proc *p, char *va, int count, int32_t offset)
{
	Proc *up = externup();
	Fgrp *f;
	Chan *c;
	char buf[256];
	int n, i, w, ww;
	char *a;

	/* print to buf to avoid holding fgrp lock while writing to user space */
	if(count > sizeof buf)
		count = sizeof buf;
	a = buf;

	qlock(&p->debug);
	f = p->fgrp;
	if(f == nil){
		qunlock(&p->debug);
		return 0;
	}
	lock(&f->r.l);
	if(waserror()){
		unlock(&f->r.l);
		qunlock(&p->debug);
		nexterror();
	}

	n = readstr(0, a, count, p->dot->path->s);
	n += snprint(a+n, count-n, "\n");
	offset = procoffset(offset, a, &n);
	/* compute width of qid.path */
	w = 0;
	for(i = 0; i <= f->maxfd; i++) {
		c = f->fd[i];
		if(c == nil)
			continue;
		ww = procqidwidth(c);
		if(ww > w)
			w = ww;
	}
	for(i = 0; i <= f->maxfd; i++) {
		c = f->fd[i];
		if(c == nil)
			continue;
		n += procfdprint(c, i, w, a+n, count-n);
		offset = procoffset(offset, a, &n);
	}
	poperror();
	unlock(&f->r.l);
	qunlock(&p->debug);

	/* copy result to user space, now that locks are released */
	memmove(va, buf, n);

	return n;
}

static void
procclose(Chan * c)
{
	if(QID(c->qid) == Qtrace){
		lock(&tlock);
		if(topens > 0)
			topens--;
		if(topens == 0)
			proctrace = notrace;
		unlock(&tlock);
	}
	if(QID(c->qid) == Qpager){
		print("leaking queueus for pager\n");
	}
	if(QID(c->qid) == Qns && c->aux != 0)
		free(c->aux);
}

static void
int2flag(int flag, char *s)
{
	if(flag == 0){
		*s = '\0';
		return;
	}
	*s++ = '-';
	if(flag & MAFTER)
		*s++ = 'a';
	if(flag & MBEFORE)
		*s++ = 'b';
	if(flag & MCREATE)
		*s++ = 'c';
	if(flag & MCACHE)
		*s++ = 'C';
	*s = '\0';
}

static int
procargs(Proc *p, char *buf, int nbuf)
{
	int j, k, m;
	char *a;
	int n;

	a = p->args;
	if(p->setargs){
		snprint(buf, nbuf, "%s [%s]", p->text, p->args);
		return strlen(buf);
	}
	n = p->nargs;
	for(j = 0; j < nbuf - 1; j += m){
		if(n <= 0)
			break;
		if(j != 0)
			buf[j++] = ' ';
		m = snprint(buf+j, nbuf-j, "%q",  a);
		k = strlen(a) + 1;
		a += k;
		n -= k;
	}
	return j;
}

static int
eventsavailable(void *v)
{
	return tproduced > tconsumed;
}

static int32_t
procread(Chan *c, void *va, int32_t n, int64_t off)
{
	Proc *up = externup();
	Proc *p;
	Mach *ac, *wired;
	int64_t l;
	int32_t r;
	Waitq *wq;
	Ureg kur;
	uint8_t *rptr;
	Confmem *cm;
	Mntwalk *mw;
	Segment *sg, *s;
	int i, j, navail, pid, rsize, sno;
	char flag[10], *sps, *srv, *statbuf;
	uintptr_t offset, profoff, u;
	int tesz;
	uintptr_t gdbregs[DBG_MAX_REG_NUM];

	if(c->qid.type & QTDIR)
		return devdirread(c, va, n, 0, 0, procgen);

	offset = off;

	if(QID(c->qid) == Qtrace){
		if(!eventsavailable(nil))
			return 0;

		rptr = va;
		tesz = BIT32SZ + BIT32SZ + BIT64SZ + BIT32SZ;
		navail = tproduced - tconsumed;
		if(navail > n / tesz)
			navail = n / tesz;
		while(navail > 0) {
			PBIT32(rptr, tevents[tconsumed & Emask].pid);
			rptr += BIT32SZ;
			PBIT32(rptr, tevents[tconsumed & Emask].etype);
			rptr += BIT32SZ;
			PBIT64(rptr, tevents[tconsumed & Emask].time);
			rptr += BIT64SZ;
			PBIT32(rptr, tevents[tconsumed & Emask].core);
			rptr += BIT32SZ;
			tconsumed++;
			navail--;
		}
		return rptr - (uint8_t*)va;
	}

	if(QID(c->qid) == Qtracepids)
		if(tpids == nil)
			return 0;
		else
			return readstr(off, va, n, tpids);

	if((p = psincref(SLOT(c->qid))) == nil || p->pid != PID(c->qid))
		error(Eprocdied);

	switch(QID(c->qid)){
	default:
		psdecref(p);
		break;
	case Qargs:
		qlock(&p->debug);
		j = procargs(p, up->genbuf, sizeof up->genbuf);
		qunlock(&p->debug);
		psdecref(p);
		if(offset >= j)
			return 0;
		if(offset+n > j)
			n = j-offset;
		memmove(va, &up->genbuf[offset], n);
		return n;

	case Qsyscall:
		if(p->syscalltrace == nil)
			return 0;
		return readstr(offset, va, n, p->syscalltrace);

	case Qcore:
		i = 0;
		ac = p->ac;
		wired = p->wired;
		if(ac != nil)
			i = ac->machno;
		else if(wired != nil)
			i = wired->machno;
		statbuf = smalloc(STATSIZE);
		snprint(statbuf, STATSIZE, "%d\n", i);
		n = readstr(offset, va, n, statbuf);
		free(statbuf);
		return n;

	case Qmem:
		if(offset < KZERO || (offset >= USTKTOP-USTKSIZE && offset < USTKTOP)){
			r = procctlmemio(p, offset, n, va, 1);
			psdecref(p);
			return r;
		}

		if(!iseve()){
			psdecref(p);
			error(Eperm);
		}

		/* validate kernel addresses */
		if(offset < PTR2UINT(end)) {
			if(offset+n > PTR2UINT(end))
				n = PTR2UINT(end) - offset;
			memmove(va, UINT2PTR(offset), n);
			psdecref(p);
			return n;
		}
		for(i=0; i<nelem(conf.mem); i++){
			cm = &conf.mem[i];
			/* klimit-1 because klimit might be zero! */
			if(cm->kbase <= offset && offset <= cm->klimit-1){
				if(offset+n >= cm->klimit-1)
					n = cm->klimit - offset;
				memmove(va, UINT2PTR(offset), n);
				psdecref(p);
				return n;
			}
		}
		psdecref(p);
		error(Ebadarg);

	case Qprofile:
		profoff = 0;
		for(sno = 0; sno < NSEG; sno++){
			if(p->seg[sno] == nil)
				continue;
			if((p->seg[sno]->type & SG_EXEC) == 0)
				continue;
			if(p->seg[sno]->profile == nil)
				continue;
			s = p->seg[sno];
			i = ((s->top-s->base)>>LRESPROF) * sizeof s->profile[0];
			if(offset >= profoff+i){
				profoff += i;
				continue;
			}
			if(offset+n > profoff+i)
				n = profoff+i - offset;
			memmove(va, ((char*)s->profile)+(offset-profoff), n);
			psdecref(p);
			return n;
		}
		psdecref(p);
		if(sno == NSEG)
			error("profile is off");
		return 0;

	case Qnote:
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			psdecref(p);
			nexterror();
		}
		if(p->pid != PID(c->qid))
			error(Eprocdied);
		if(n < 1)	/* must accept at least the '\0' */
			error(Etoosmall);
		if(p->nnote == 0)
			n = 0;
		else {
			i = strlen(p->note[0].msg) + 1;
			if(i > n)
				i = n;
			rptr = va;
			memmove(rptr, p->note[0].msg, i);
			rptr[i-1] = '\0';
			p->nnote--;
			memmove(p->note, p->note+1, p->nnote*sizeof(Note));
			n = i;
		}
		if(p->nnote == 0)
			p->notepending = 0;
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		return n;

	case Qproc:
		if(offset >= sizeof(Proc)){
			psdecref(p);
			return 0;
		}
		if(offset+n > sizeof(Proc))
			n = sizeof(Proc) - offset;
		memmove(va, ((char*)p)+offset, n);
		psdecref(p);
		return n;

	case Qregs:
		rptr = (uint8_t*)p->dbgreg;
		rsize = sizeof(Ureg);
	regread:
		if(rptr == 0){
			psdecref(p);
			error(Enoreg);
		}
		if(offset >= rsize){
			psdecref(p);
			return 0;
		}
		if(offset+n > rsize)
			n = rsize - offset;
		memmove(va, rptr+offset, n);
		psdecref(p);
		return n;

		/* Sorry about the code duplication. TODO: clean this up? */
	case Qgdbregs:
		rptr = (uint8_t*)&gdbregs[0];
		// not sizeof; it's an odd number of 32-bit words ... yuck.
		rsize = GDB_NUMREGBYTES;

		if(rptr == 0){
			psdecref(p);
			error(Enoreg);
		}
		if(offset >= rsize){
			psdecref(p);
			return 0;
		}
		if(offset+n > rsize)
			n = rsize - offset;
		ureg2gdb(p->dbgreg, gdbregs);
print("Qgdbregs: va %p, rptr +offset %p, n %d\n", va, rptr+offset, n);
		memmove(va, rptr+offset, n);
		psdecref(p);
		return n;

	case Qkregs:
		memset(&kur, 0, sizeof(Ureg));
		setkernur(&kur, p);
		rptr = (uint8_t*)&kur;
		rsize = sizeof(Ureg);
		goto regread;

	case Qfpregs:
		r = fpudevprocio(p, va, n, offset, 0);
		psdecref(p);
		return r;

	case Qstatus:
		if(offset >= STATSIZE){
			psdecref(p);
			return 0;
		}
		if(offset+n > STATSIZE)
			n = STATSIZE - offset;

		sps = p->psstate;
		if(sps == 0)
			sps = statename[p->state];
		statbuf = smalloc(STATSIZE);
		memset(statbuf, ' ', STATSIZE);
		sprint(statbuf, "%-*.*s%-*.*s%-12.11s",
			KNAMELEN, KNAMELEN-1, p->text,
			KNAMELEN, KNAMELEN-1, p->user,
			sps);
		j = 2*KNAMELEN + 12;

		for(i = 0; i < 6; i++) {
			l = p->time[i];
			if(i == TReal)
				l = sys->ticks - l;
			l = TK2MS(l);
			readnum(0, statbuf+j+NUMSIZE*i, NUMSIZE, l, NUMSIZE);
		}
		/* ignore stacks, which are typically not faulted in */
		u = 0;
		for(i=0; i<NSEG; i++){
			s = p->seg[i];
			if(s != nil && (s->type&SG_TYPE) != SG_STACK)
				u += s->top - s->base;
		}
		readnum(0, statbuf+j+NUMSIZE*6, NUMSIZE, u>>10u, NUMSIZE);	/* wrong size */
		readnum(0, statbuf+j+NUMSIZE*7, NUMSIZE, p->basepri, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*8, NUMSIZE, p->priority, NUMSIZE);

		/*
		 * NIX: added # of traps, syscalls, and iccs
		 */
		readnum(0, statbuf+j+NUMSIZE*9, NUMSIZE, p->ntrap, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*10, NUMSIZE, p->nintr, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*11, NUMSIZE, p->nsyscall, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*12, NUMSIZE, p->nicc, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*13, NUMSIZE, p->nactrap, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*14, NUMSIZE, p->nacsyscall, NUMSIZE);

		/*
		 * external pager support, random stuff.
		 */
		if (0) print("qstatus p %p pid %d req %p\n", p, p->pid, p->req);
		readnum(0,statbuf+j+NUMSIZE*15, NUMSIZE, p->req ? 1 : 0, NUMSIZE);
		readnum(0,statbuf+j+NUMSIZE*16, NUMSIZE, p->resp ? 1 : 0, NUMSIZE);
		statbuf[j+NUMSIZE*17] = '\n';
		if(offset+n > j+NUMSIZE*17+1)
			n = j+NUMSIZE*17+1-offset;

		memmove(va, statbuf+offset, n);
		free(statbuf);
		psdecref(p);
		return n;

	case Qsegment:
		j = 0;
		statbuf = smalloc(STATSIZE);
		for(i = 0; i < NSEG; i++) {
			sg = p->seg[i];
			if(sg == 0)
				continue;
			j += sprint(statbuf+j, "%-6s %c%c%c %c %p %p %4d\n",
				segtypes[sg->type&SG_TYPE],
				(sg->type&SG_READ) != 0 ? 'r' : '-',
				(sg->type&SG_WRITE) != 0 ? 'w' : '-',
				(sg->type&SG_EXEC) != 0 ? 'x' : '-',
				sg->profile ? 'P' : ' ',
				sg->base, sg->top, sg->r.ref);
		}
		psdecref(p);
		if(offset >= j){
			free(statbuf);
			return 0;
		}
		if(offset+n > j)
			n = j-offset;
		if(n == 0 && offset == 0){
			free(statbuf);
			exhausted("segments");
		}
		memmove(va, statbuf+offset, n);
		free(statbuf);
		return n;

	case Qwait:
		if(!canqlock(&p->qwaitr)){
			psdecref(p);
			error(Einuse);
		}

		if(waserror()) {
			qunlock(&p->qwaitr);
			psdecref(p);
			nexterror();
		}

		lock(&p->exl);
		if(up == p && p->nchild == 0 && p->waitq == 0) {
			unlock(&p->exl);
			error(Enochild);
		}
		pid = p->pid;
		while(p->waitq == 0) {
			unlock(&p->exl);
			sleep(&p->waitr, haswaitq, p);
			if(p->pid != pid)
				error(Eprocdied);
			lock(&p->exl);
		}
		wq = p->waitq;
		p->waitq = wq->next;
		p->nwait--;
		unlock(&p->exl);

		poperror();
		qunlock(&p->qwaitr);
		psdecref(p);
		n = snprint(va, n, "%d %lu %lu %lu %q",
			wq->w.pid,
			wq->w.time[TUser], wq->w.time[TSys], wq->w.time[TReal],
			wq->w.msg);
		free(wq);
		return n;

	case Qns:
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			psdecref(p);
			nexterror();
		}
		if(p->pgrp == nil || p->pid != PID(c->qid))
			error(Eprocdied);
		mw = c->aux;
		if(mw->cddone){
			poperror();
			qunlock(&p->debug);
			psdecref(p);
			return 0;
		}
		mntscan(mw, p);
		if(mw->mh == 0){
			mw->cddone = 1;
			i = snprint(va, n, "cd %s\n", p->dot->path->s);
			poperror();
			qunlock(&p->debug);
			psdecref(p);
			return i;
		}
		int2flag(mw->cm->mflag, flag);
		if(strcmp(mw->cm->to->path->s, "#M") == 0){
			srv = srvname(mw->cm->to->mchan);
			i = snprint(va, n, "mount %s %s %s %s\n", flag,
				srv==nil? mw->cm->to->mchan->path->s : srv,
				mw->mh->from->path->s, mw->cm->spec? mw->cm->spec : "");
			free(srv);
		}else
			i = snprint(va, n, "bind %s %s %s\n", flag,
				mw->cm->to->path->s, mw->mh->from->path->s);
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		return i;

	case Qnoteid:
		r = readnum(offset, va, n, p->noteid, NUMSIZE);
		psdecref(p);
		return r;
	case Qfd:
		r = procfds(p, va, n, offset);
		psdecref(p);
		return r;
	case Qtls:
		statbuf = smalloc(STATSIZE);
		j = snprint(statbuf, STATSIZE, "tls 0x%p\n", p->tls);
		psdecref(p);
		if(offset >= j){
			free(statbuf);
			return 0;
		}
		if(offset+n > j)
			n = j-offset;
		memmove(va, statbuf+offset, n);
		free(statbuf);
		return n;

	case Qpager:
		p = c->aux;
		n = qread(p->req, va, n);
		print("read pager: %p\n", n);
		break;
	}
	error(Egreg);
	return 0;			/* not reached */
}

static void
mntscan(Mntwalk *mw, Proc *p)
{
	Pgrp *pg;
	Mount *t;
	Mhead *f;
	int best, i, last, nxt;

	pg = p->pgrp;
	rlock(&pg->ns);

	nxt = 0;
	best = (int)(~0U>>1);		/* largest 2's complement int */

	last = 0;
	if(mw->mh)
		last = mw->cm->mountid;

	for(i = 0; i < MNTHASH; i++) {
		for(f = pg->mnthash[i]; f; f = f->hash) {
			for(t = f->mount; t; t = t->next) {
				if(mw->mh == 0 ||
				  (t->mountid > last && t->mountid < best)) {
					mw->cm = t;
					mw->mh = f;
					best = mw->cm->mountid;
					nxt = 1;
				}
			}
		}
	}
	if(nxt == 0)
		mw->mh = 0;

	runlock(&pg->ns);
}

static int32_t
procwrite(Chan *c, void *va, int32_t n, int64_t off)
{
	Proc *up = externup();
	Proc *p, *t;
	int i, id, l;
	char *args, buf[ERRMAX];
	uintptr_t offset;

	if(c->qid.type & QTDIR)
		error(Eisdir);

	/* Use the remembered noteid in the channel rather
	 * than the process pgrpid
	 */
	if(QID(c->qid) == Qnotepg) {
		pgrpnote(NOTEID(c->pgrpid), va, n, NUser);
		return n;
	}

	if((p = psincref(SLOT(c->qid))) == nil)
		error(Eprocdied);

	qlock(&p->debug);
	if(waserror()){
		qunlock(&p->debug);
		psdecref(p);
		nexterror();
	}
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	offset = off;

	switch(QID(c->qid)){
	case Qargs:
		if(n == 0)
			error(Eshort);
		if(n >= ERRMAX)
			error(Etoobig);
		memmove(buf, va, n);
		args = malloc(n+1);
		if(args == nil)
			error(Enomem);
		memmove(args, buf, n);
		l = n;
		if(args[l-1] != 0)
			args[l++] = 0;
		free(p->args);
		p->nargs = l;
		p->args = args;
		p->setargs = 1;
		break;

	case Qmem:
		if(p->state != Stopped)
			error(Ebadctl);

		n = procctlmemio(p, offset, n, va, 0);
		break;

	case Qregs:
		if(offset >= sizeof(Ureg))
			n = 0;
		else if(offset+n > sizeof(Ureg))
			n = sizeof(Ureg) - offset;
		if(p->dbgreg == 0)
			error(Enoreg);
		setregisters(p->dbgreg, (char*)(p->dbgreg)+offset, va, n);
		break;

	case Qfpregs:
		n = fpudevprocio(p, va, n, offset, 1);
		break;

	case Qctl:
		procctlreq(p, va, n);
		break;

	case Qnote:
		if(p->kp)
			error(Eperm);
		if(n >= ERRMAX-1)
			error(Etoobig);
		memmove(buf, va, n);
		buf[n] = 0;
		if(!postnote(p, 0, buf, NUser))
			error("note not posted");
		break;
	case Qnoteid:
		id = atoi(va);
		if(id == p->pid) {
			p->noteid = id;
			break;
		}
		for(i = 0; (t = psincref(i)) != nil; i++){
			if(t->state == Dead || t->noteid != id){
				psdecref(t);
				continue;
			}
			if(strcmp(p->user, t->user) != 0){
				psdecref(t);
				error(Eperm);
			}
			psdecref(t);
			p->noteid = id;
			break;
		}
		if(p->noteid != id)
			error(Ebadarg);
		break;

	case Qtls:
		if(n >= sizeof buf)
			error(Etoobig);
		memmove(buf, va, n);
		buf[n] = '\0';
		if(memcmp(buf, "tls ", 4) == 0){
			char *s;
			for(s = buf; *s != '\0' && (*s < '0' || *s > '9'); s++)
				;
			if(*s >= '0' && *s <= '9'){
				p->tls = (uintptr_t)strtoull(s, nil, 0); // a-tol-whex! a-tol-whex!
				poperror();
				qunlock(&p->debug);
				psdecref(p);
				return n;
			}
		}
		error(Ebadarg);

	case Qpager:
		p = c->aux;
		if (p && p->resp)
			n = qwrite(p->resp, va, n);
		break;

	default:
		poperror();
		qunlock(&p->debug);
		psdecref(p);
		pprint("unknown qid %#llux in procwrite\n", c->qid.path);
		error(Egreg);
	}
	poperror();
	qunlock(&p->debug);
	psdecref(p);
	return n;
}

Dev procdevtab = {
	.dc = 'p',
	.name = "proc",

	.reset = devreset,
	.init = procinit,
	.shutdown = devshutdown,
	.attach = procattach,
	.walk = procwalk,
	.stat = procstat,
	.open = procopen,
	.create = devcreate,
	.close = procclose,
	.read = procread,
	.bread = devbread,
	.write = procwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = procwstat,
};

static Chan*
proctext(Chan *c, Proc *p)
{
	Proc *up = externup();
	Chan *tc;
	Image *i;
	Segment *s;
	int sno;

	for(sno = 0; sno < NSEG; sno++)
		if(p->seg[sno] != nil)
			if((p->seg[sno]->type & SG_EXEC) != 0)
				break;
	if(sno == NSEG)
		error(Enonexist);
	s = p->seg[sno];

	if(p->state==Dead)
		error(Eprocdied);

	lock(&s->r.l);
	i = s->image;
	if(i == 0) {
		unlock(&s->r.l);
		error(Eprocdied);
	}
	unlock(&s->r.l);

	lock(&i->r.l);
	if(waserror()) {
		unlock(&i->r.l);
		nexterror();
	}

	tc = i->c;
	if(tc == 0)
		error(Eprocdied);

	if(incref(&tc->r) == 1 || (tc->flag&COPEN) == 0 || tc->mode!=OREAD) {
		cclose(tc);
		error(Eprocdied);
	}

	if(p->pid != PID(c->qid)){
		cclose(tc);
		error(Eprocdied);
	}

	poperror();
	unlock(&i->r.l);

	return tc;
}

void
procstopwait(Proc *p, int ctl)
{
	Proc *up = externup();
	int pid;

	if(p->pdbg)
		error(Einuse);
	if(procstopped(p) || p->state == Broken)
		return;

	if(ctl != 0)
		p->procctl = ctl;
	p->pdbg = up;
	pid = p->pid;
	qunlock(&p->debug);
	up->psstate = "Stopwait";
	if(waserror()) {
		p->pdbg = 0;
		qlock(&p->debug);
		nexterror();
	}
	sleep(&up->sleep, procstopped, p);
	poperror();
	qlock(&p->debug);
	if(p->pid != pid)
		error(Eprocdied);
}

static void
procctlcloseone(Proc *p, Fgrp *f, int fd)
{
	Chan *c;

	c = f->fd[fd];
	if(c == nil)
		return;
	f->fd[fd] = nil;
	unlock(&f->r.l);
	qunlock(&p->debug);
	cclose(c);
	qlock(&p->debug);
	lock(&f->r.l);
}

void
procctlclosefiles(Proc *p, int all, int fd)
{
	int i;
	Fgrp *f;

	f = p->fgrp;
	if(f == nil)
		error(Eprocdied);

	lock(&f->r.l);
	f->r.ref++;
	if(all)
		for(i = 0; i < f->maxfd; i++)
			procctlcloseone(p, f, i);
	else
		procctlcloseone(p, f, fd);
	unlock(&f->r.l);
	closefgrp(f);
}

static char *
parsetime(int64_t *rt, char *s)
{
	uint64_t ticks;
	uint32_t l;
	char *e, *p;
	static int p10[] = {100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	if (s == nil)
		return("missing value");
	ticks=strtoull(s, &e, 10);
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
	if (*e == '\0' || strcmp(e, "s") == 0){
		ticks = 1000000000 * ticks + l;
	}else if (strcmp(e, "ms") == 0){
		ticks = 1000000 * ticks + l/1000;
	}else if (strcmp(e, "µs") == 0 || strcmp(e, "us") == 0){
		ticks = 1000 * ticks + l/1000000;
	}else if (strcmp(e, "ns") != 0)
		return "unrecognized unit";
	*rt = ticks;
	return nil;
}

static void
procctlreq(Proc *p, char *va, int n)
{
	Proc *up = externup();
	Segment *s;
	int npc, pri, core, sno;
	Cmdbuf *cb;
	Cmdtab *ct;
	int64_t time;
	char *e;

	if(p->kp)	/* no ctl requests to kprocs */
		error(Eperm);

	cb = parsecmd(va, n);
	if(waserror()){
		free(cb);
		nexterror();
	}

	ct = lookupcmd(cb, proccmd, nelem(proccmd));

	switch(ct->index){
	case CMclose:
		procctlclosefiles(p, 0, atoi(cb->f[1]));
		break;
	case CMclosefiles:
		procctlclosefiles(p, 1, 0);
		break;
	case CMhang:
		p->hang = 1;
		break;
	case CMkill:
		switch(p->state) {
		case Broken:
			unbreak(p);
			break;
		case Stopped:
		case Semdown:
			p->procctl = Proc_exitme;
			postnote(p, 0, "sys: killed", NExit);
			ready(p);
			break;
		default:
			p->procctl = Proc_exitme;
			postnote(p, 0, "sys: killed", NExit);
		}
		break;
	case CMnohang:
		p->hang = 0;
		break;
	case CMnoswap:
		p->noswap = 1;
		break;
	case CMpri:
		pri = atoi(cb->f[1]);
		if(pri > PriNormal && !iseve())
			error(Eperm);
		procpriority(p, pri, 0);
		break;
	case CMfixedpri:
		pri = atoi(cb->f[1]);
		if(pri > PriNormal && !iseve())
			error(Eperm);
		procpriority(p, pri, 1);
		break;
	case CMprivate:
		p->privatemem = 1;
		break;
	case CMprofile:
		for(sno = 0; sno < NSEG; sno++){
			if(p->seg[sno] != nil && (p->seg[sno]->type & SG_EXEC) != 0){
				s = p->seg[sno];
				if(s->profile != 0)
					free(s->profile);
				npc = (s->top-s->base)>>LRESPROF;
				s->profile = malloc(npc * sizeof s->profile[0]);
				if(s->profile == 0)
					error(Enomem);
			}
		}
		break;
	case CMstart:
		if(p->state != Stopped)
			error(Ebadctl);
		ready(p);
		break;
	case CMstartstop:
		if(p->state != Stopped)
			error(Ebadctl);
		p->procctl = Proc_traceme;
		ready(p);
		procstopwait(p, Proc_traceme);
		break;
	case CMstartsyscall:
		if(p->state != Stopped)
			error(Ebadctl);
		p->procctl = Proc_tracesyscall;
		ready(p);
		procstopwait(p, Proc_tracesyscall);
		break;
	case CMstop:
		procstopwait(p, Proc_stopme);
		break;
	case CMwaitstop:
		procstopwait(p, 0);
		break;
	case CMwired:
		core = atoi(cb->f[1]);
		procwired(p, core);
		sched();
		break;
	case CMtrace:
		switch(cb->nf){
		case 1:
			p->trace ^= 1;
			break;
		case 2:
			p->trace = (atoi(cb->f[1]) != 0);
			break;
		default:
			error("args");
		}
		break;
	/* real time */
	case CMperiod:
		if(p->edf == nil)
			edfinit(p);
		if(e=parsetime(&time, cb->f[1]))	/* time in ns */
			error(e);
		edfstop(p);
		p->edf->T = time/1000;			/* Edf times are in µs */
		break;
	case CMdeadline:
		if(p->edf == nil)
			edfinit(p);
		if(e=parsetime(&time, cb->f[1]))
			error(e);
		edfstop(p);
		p->edf->D = time/1000;
		break;
	case CMcost:
		if(p->edf == nil)
			edfinit(p);
		if(e=parsetime(&time, cb->f[1]))
			error(e);
		edfstop(p);
		p->edf->C = time/1000;
		break;
	case CMsporadic:
		if(p->edf == nil)
			edfinit(p);
		p->edf->flags |= Sporadic;
		break;
	case CMdeadlinenotes:
		if(p->edf == nil)
			edfinit(p);
		p->edf->flags |= Sendnotes;
		break;
	case CMadmit:
		if(p->edf == 0)
			error("edf params");
		if(e = edfadmit(p))
			error(e);
		break;
	case CMextra:
		if(p->edf == nil)
			edfinit(p);
		p->edf->flags |= Extratime;
		break;
	case CMexpel:
		if(p->edf)
			edfstop(p);
		break;
	case CMevent:
		if(up->trace)
			proctrace(up, SUser, 0);
		break;
	case CMcore:
		core = atoi(cb->f[1]);
		if(core >= MACHMAX)
			error("wrong core number");
		else if(core == 0){
			if(p->ac == nil)
				error("not running in an ac");
			p->procctl = Proc_totc;
			if(p != up && p->state == Exotic){
				/* see the comment in postnote */
				intrac(p);
			}
		}else{
			if(p->ac != nil)
				error("running in an ac");
			if(core < 0)
				p->ac = getac(p, -1);
			else
				p->ac = getac(p, core);
			p->procctl = Proc_toac;
			p->prepagemem = 1;
		}
		break;
	}
	poperror();
	free(cb);
}

static int
procstopped(void *a)
{
	Proc *p = a;
	return p->state == Stopped;
}

static int
procctlmemio(Proc *p, uintptr_t offset, int n, void *va, int read)
{
	Proc *up = externup();
	KMap *k;
	Pte *pte;
	Page *pg;
	Segment *s;
	uintptr_t soff, l;	/* hmmmm */
	uint8_t *b;
	uintmem pgsz;

	for(;;) {
		s = seg(p, offset, 1);
		if(s == 0)
			error(Ebadarg);

		if(offset+n >= s->top)
			n = s->top-offset;

		if(!read && (s->type&SG_TYPE) == SG_TEXT)
			s = txt2data(p, s);

		s->steal++;
		soff = offset-s->base;
		if(waserror()) {
			s->steal--;
			nexterror();
		}
		if(fixfault(s, offset, read, 0, s->color) == 0)
			break;
		poperror();
		s->steal--;
	}
	poperror();
	pte = s->map[soff/PTEMAPMEM];
	if(pte == 0)
		panic("procctlmemio");
	pgsz = sys->pgsz[s->pgszi];
	pg = pte->pages[(soff&(PTEMAPMEM-1))/pgsz];
	if(pagedout(pg))
		panic("procctlmemio1");

	l = pgsz - (offset&(pgsz-1));
	if(n > l)
		n = l;

	k = kmap(pg);
	if(waserror()) {
		s->steal--;
		kunmap(k);
		nexterror();
	}
	b = (uint8_t*)VA(k);
	b += offset&(pgsz-1);
	if(read == 1)
		memmove(va, b, n);	/* This can fault */
	else
		memmove(b, va, n);
	poperror();
	kunmap(k);

	/* Ensure the process sees text page changes */
	if(s->flushme)
		memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));

	s->steal--;

	if(read == 0)
		p->newtlb = 1;

	return n;
}

static Segment*
txt2data(Proc *p, Segment *s)
{
	int i;
	Segment *ps;

	ps = newseg(SG_DATA, s->base, s->size);
	ps->image = s->image;
	incref(&ps->image->r);
	ps->ldseg = s->ldseg;
	ps->flushme = 1;

	qlock(&p->seglock);
	for(i = 0; i < NSEG; i++)
		if(p->seg[i] == s)
			break;
	if(i == NSEG)
		panic("segment gone");

	qunlock(&s->lk);
	putseg(s);
	qlock(&ps->lk);
	p->seg[i] = ps;
	qunlock(&p->seglock);

	return ps;
}

Segment*
data2txt(Segment *s)
{
	Segment *ps;

	ps = newseg(SG_TEXT, s->base, s->size);
	ps->image = s->image;
	incref(&ps->image->r);
	ps->ldseg = s->ldseg;
	ps->flushme = 1;

	return ps;
}
