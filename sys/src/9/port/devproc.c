#include	"u.h"
#include	<trace.h>
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ureg.h"
#include	"../port/edf.h"

enum
{
	Qdir,
	Qtrace,
	Qargs,
	Qctl,
	Qfd,
	Qfpregs,
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
};

enum{
	Nevents = 0x4000,
	Emask = Nevents - 1,
};

#define	STATSIZE	(2*KNAMELEN+12+9*12)
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
	"fpregs",	{Qfpregs},	sizeof(FPsave),		0000,
	"kregs",	{Qkregs},	sizeof(Ureg),		0400,
	"mem",		{Qmem},		0,			0000,
	"note",		{Qnote},	0,			0000,
	"noteid",	{Qnoteid},	0,			0664,
	"notepg",	{Qnotepg},	0,			0000,
	"ns",		{Qns},		0,			0444,
	"proc",		{Qproc},	0,			0400,
	"regs",		{Qregs},	sizeof(Ureg),		0000,
	"segment",	{Qsegment},	0,			0444,
	"status",	{Qstatus},	STATSIZE,		0444,
	"text",		{Qtext},	0,			0000,
	"wait",		{Qwait},	0,			0400,
	"profile",	{Qprofile},	0,			0400,
	"syscall",	{Qsyscall},	0,			0400,	
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
};

/* Segment type from portdat.h */
static char *sname[]={ "Text", "Data", "Bss", "Stack", "Shared", "Phys", };

/*
 * Qids are, in path:
 *	 5 bits of file type (qids above)
 *	26 bits of process slot number + 1
 *	     in vers,
 *	32 bits of pid, for consistency checking
 * If notepg, c->pgrpid.path is pgrp slot, .vers is noteid.
 */
#define	QSHIFT	5	/* location in qid of proc slot # */

#define	QID(q)		((((ulong)(q).path) & ((1<<QSHIFT)-1)) >> 0)
#define	SLOT(q)		(((((ulong)(q).path) & ~(1UL<<31)) >> QSHIFT) - 1)
#define	PID(q)		((q).vers)
#define	NOTEID(q)	((q).vers)

void	procctlreq(Proc*, char*, int);
int	procctlmemio(Proc*, ulong, int, void*, int);
Chan*	proctext(Chan*, Proc*);
Segment* txt2data(Proc*, Segment*);
int	procstopped(void*);
void	mntscan(Mntwalk*, Proc*);

static Traceevent *tevents;
static Lock tlock;
static int topens;
static int tproduced, tconsumed;
void (*proctrace)(Proc*, int, vlong);

extern int unfair;

static void
profclock(Ureg *ur, Timer *)
{
	Tos *tos;

	if(up == 0 || up->state != Running)
		return;

	/* user profiling clock */
	if(userureg(ur)){
		tos = (Tos*)(USTKTOP-sizeof(Tos));
		tos->clock += TK2MS(1);
		segclock(ur->pc);
	}
}

static int
procgen(Chan *c, char *name, Dirtab *tab, int, int s, Dir *dp)
{
	Qid qid;
	Proc *p;
	char *ename;
	Segment *q;
	ulong pid, path, perm, len;

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

		if(name != nil){
			/* ignore s and use name to find pid */
			pid = strtol(name, &ename, 10);
			if(pid==0 || ename[0]!='\0')
				return -1;
			s = procindex(pid);
			if(s < 0)
				return -1;
		}
		else if(--s >= conf.nproc)
			return -1;

		p = proctab(s);
		pid = p->pid;
		if(pid == 0)
			return 0;
		sprint(up->genbuf, "%lud", pid);
		/*
		 * String comparison is done in devwalk so name must match its formatted pid
		*/
		if(name != nil && strcmp(name, up->genbuf) != 0)
			return -1;
		mkqid(&qid, (s+1)<<QSHIFT, pid, QTDIR);
		devdir(c, qid, up->genbuf, 0, p->user, DMDIR|0555, dp);
		return 1;
	}
	if(c->qid.path == Qtrace){
		strcpy(up->genbuf, "trace");
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

	/* p->procmode determines default mode for files in /proc */
	p = proctab(SLOT(c->qid));
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
	case Qprofile:
		q = p->seg[TSEG];
		if(q && q->profile) {
			len = (q->top-q->base)>>LRESPROF;
			len *= sizeof(*q->profile);
		}
		break;
	}

	mkqid(&qid, path|tab->qid.path, c->qid.vers, QTFILE);
	devdir(c, qid, tab->name, len, p->user, perm, dp);
	return 1;
}

static void
_proctrace(Proc* p, Tevent etype, vlong ts)
{
	Traceevent *te;

	if (p->trace == 0 || topens == 0 ||
		tproduced - tconsumed >= Nevents)
		return;

	te = &tevents[tproduced&Emask];
	te->pid = p->pid;
	te->etype = etype;
	if (ts == 0)
		te->time = todget(nil);
	else
		te->time = ts;
	tproduced++;
}

static void
procinit(void)
{
	if(conf.nproc >= (1<<(31-QSHIFT))-1)
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

static int
procstat(Chan *c, uchar *db, int n)
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
			tproduced = tconsumed = 0;
		}
		proctrace = _proctrace;
		unlock(&tlock);
		poperror();

		c->mode = openmode(omode);
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
		
	p = proctab(SLOT(c->qid));
	qlock(&p->debug);
	if(waserror()){
		qunlock(&p->debug);
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
		qunlock(&p->debug);
		poperror();
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

	case Qargs:
	case Qnoteid:
	case Qstatus:
	case Qwait:
	case Qregs:
	case Qfpregs:
	case Qsyscall:	
		nonone(p);
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
		pprint("procopen %#lux\n", QID(c->qid));
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
	qunlock(&p->debug);
	poperror();

	return tc;
}

static int
procwstat(Chan *c, uchar *db, int n)
{
	Proc *p;
	Dir *d;

	if(c->qid.type&QTDIR)
		error(Eperm);

	if(QID(c->qid) == Qtrace)
		return devwstat(c, db, n);
		
	p = proctab(SLOT(c->qid));
	nonone(p);
	d = nil;
	if(waserror()){
		free(d);
		qunlock(&p->debug);
		nexterror();
	}
	qlock(&p->debug);

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
	/* p->procmode determines default mode for files in /proc */
	if(d->mode != ~0UL)
		p->procmode = d->mode&0777;

	poperror();
	free(d);
	qunlock(&p->debug);
	return n;
}


static long
procoffset(long offset, char *va, int *np)
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

	return sprint(buf, "%lud", c->qid.vers);
}

int
procfdprint(Chan *c, int fd, int w, char *s, int ns)
{
	int n;

	if(w == 0)
		w = procqidwidth(c);
	n = snprint(s, ns, "%3d %.2s %C %4ld (%.16llux %*lud %.2ux) %5ld %8lld %s\n",
		fd,
		&"r w rw"[(c->mode&3)<<1],
		devtab[c->type]->dc, c->dev,
		c->qid.path, w, c->qid.vers, c->qid.type,
		c->iounit, c->offset, c->path->s);
	return n;
}

static int
procfds(Proc *p, char *va, int count, long offset)
{
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
	lock(f);
	if(waserror()){
		unlock(f);
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
	unlock(f);
	qunlock(&p->debug);
	poperror();

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
			proctrace = nil;
		unlock(&tlock);
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
eventsavailable(void *)
{
	return tproduced > tconsumed;
}

static long
procread(Chan *c, void *va, long n, vlong off)
{
	/* NSEG*32 was too small for worst cases */
	char *a, flag[10], *sps, *srv, statbuf[NSEG*64];
	int i, j, m, navail, ne, pid, rsize;
	long l;
	uchar *rptr;
	ulong offset;
	Confmem *cm;
	Mntwalk *mw;
	Proc *p;
	Segment *sg, *s;
	Ureg kur;
	Waitq *wq;
	
	a = va;
	offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, procgen);

	if(QID(c->qid) == Qtrace){
		if(!eventsavailable(nil))
			return 0;

		rptr = (uchar*)va;
		navail = tproduced - tconsumed;
		if(navail > n / sizeof(Traceevent))
			navail = n / sizeof(Traceevent);
		while(navail > 0) {
			ne = ((tconsumed & Emask) + navail > Nevents)? 
					Nevents - (tconsumed & Emask): navail;
			memmove(rptr, &tevents[tconsumed & Emask], 
					ne * sizeof(Traceevent));

			tconsumed += ne;
			rptr += ne * sizeof(Traceevent);
			navail -= ne;
		}
		return rptr - (uchar*)va;
	}

	p = proctab(SLOT(c->qid));
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	switch(QID(c->qid)){
	case Qargs:
		qlock(&p->debug);
		j = procargs(p, up->genbuf, sizeof up->genbuf);
		qunlock(&p->debug);
		if(offset >= j)
			return 0;
		if(offset+n > j)
			n = j-offset;
		memmove(a, &up->genbuf[offset], n);
		return n;
	case Qsyscall:
		if(!p->syscalltrace)
			return 0;
		n = readstr(offset, a, n, p->syscalltrace);
		return n;

	case Qmem:
		if(offset < KZERO)
			return procctlmemio(p, offset, n, va, 1);

		if(!iseve())
			error(Eperm);

		/* validate kernel addresses */
		if(offset < (ulong)end) {
			if(offset+n > (ulong)end)
				n = (ulong)end - offset;
			memmove(a, (char*)offset, n);
			return n;
		}
		for(i=0; i<nelem(conf.mem); i++){
			cm = &conf.mem[i];
			/* klimit-1 because klimit might be zero! */
			if(cm->kbase <= offset && offset <= cm->klimit-1){
				if(offset+n >= cm->klimit-1)
					n = cm->klimit - offset;
				memmove(a, (char*)offset, n);
				return n;
			}
		}
		error(Ebadarg);

	case Qprofile:
		s = p->seg[TSEG];
		if(s == 0 || s->profile == 0)
			error("profile is off");
		i = (s->top-s->base)>>LRESPROF;
		i *= sizeof(*s->profile);
		if(offset >= i)
			return 0;
		if(offset+n > i)
			n = i - offset;
		memmove(a, ((char*)s->profile)+offset, n);
		return n;

	case Qnote:
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			nexterror();
		}
		if(p->pid != PID(c->qid))
			error(Eprocdied);
		if(n < 1)	/* must accept at least the '\0' */
			error(Etoosmall);
		if(p->nnote == 0)
			n = 0;
		else {
			m = strlen(p->note[0].msg) + 1;
			if(m > n)
				m = n;
			memmove(va, p->note[0].msg, m);
			((char*)va)[m-1] = '\0';
			p->nnote--;
			memmove(p->note, p->note+1, p->nnote*sizeof(Note));
			n = m;
		}
		if(p->nnote == 0)
			p->notepending = 0;
		poperror();
		qunlock(&p->debug);
		return n;

	case Qproc:
		if(offset >= sizeof(Proc))
			return 0;
		if(offset+n > sizeof(Proc))
			n = sizeof(Proc) - offset;
		memmove(a, ((char*)p)+offset, n);
		return n;

	case Qregs:
		rptr = (uchar*)p->dbgreg;
		rsize = sizeof(Ureg);
		goto regread;

	case Qkregs:
		memset(&kur, 0, sizeof(Ureg));
		setkernur(&kur, p);
		rptr = (uchar*)&kur;
		rsize = sizeof(Ureg);
		goto regread;

	case Qfpregs:
		rptr = (uchar*)&p->fpsave;
		rsize = sizeof(FPsave);
	regread:
		if(rptr == 0)
			error(Enoreg);
		if(offset >= rsize)
			return 0;
		if(offset+n > rsize)
			n = rsize - offset;
		memmove(a, rptr+offset, n);
		return n;

	case Qstatus:
		if(offset >= STATSIZE)
			return 0;
		if(offset+n > STATSIZE)
			n = STATSIZE - offset;

		sps = p->psstate;
		if(sps == 0)
			sps = statename[p->state];
		memset(statbuf, ' ', sizeof statbuf);
		readstr(0, statbuf+0*KNAMELEN, KNAMELEN-1, p->text);
		readstr(0, statbuf+1*KNAMELEN, KNAMELEN-1, p->user);
		readstr(0, statbuf+2*KNAMELEN, 11, sps);
		j = 2*KNAMELEN + 12;

		for(i = 0; i < 6; i++) {
			l = p->time[i];
			if(i == TReal)
				l = MACHP(0)->ticks - l;
			l = TK2MS(l);
			readnum(0, statbuf+j+NUMSIZE*i, NUMSIZE, l, NUMSIZE);
		}
		/* ignore stack, which is mostly non-existent */
		l = 0;
		for(i=1; i<NSEG; i++){
			s = p->seg[i];
			if(s)
				l += s->top - s->base;
		}
		readnum(0, statbuf+j+NUMSIZE*6, NUMSIZE, l>>10, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*7, NUMSIZE, p->basepri, NUMSIZE);
		readnum(0, statbuf+j+NUMSIZE*8, NUMSIZE, p->priority, NUMSIZE);
		memmove(a, statbuf+offset, n);
		return n;

	case Qsegment:
		j = 0;
		for(i = 0; i < NSEG; i++) {
			sg = p->seg[i];
			if(sg == 0)
				continue;
			j += sprint(statbuf+j, "%-6s %c%c %.8lux %.8lux %4ld\n",
				sname[sg->type&SG_TYPE],
				sg->type&SG_RONLY ? 'R' : ' ',
				sg->profile ? 'P' : ' ',
				sg->base, sg->top, sg->ref);
		}
		if(offset >= j)
			return 0;
		if(offset+n > j)
			n = j-offset;
		if(n == 0 && offset == 0)
			exhausted("segments");
		memmove(a, &statbuf[offset], n);
		return n;

	case Qwait:
		if(!canqlock(&p->qwaitr))
			error(Einuse);

		if(waserror()) {
			qunlock(&p->qwaitr);
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

		qunlock(&p->qwaitr);
		poperror();
		n = snprint(a, n, "%d %lud %lud %lud %q",
			wq->w.pid,
			wq->w.time[TUser], wq->w.time[TSys], wq->w.time[TReal],
			wq->w.msg);
		free(wq);
		return n;

	case Qns:
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			nexterror();
		}
		if(p->pgrp == nil || p->pid != PID(c->qid))
			error(Eprocdied);
		mw = c->aux;
		if(mw == nil)
			error(Enomem);
		if(mw->cddone){
			qunlock(&p->debug);
			poperror();
			return 0;
		}
		mntscan(mw, p);
		if(mw->mh == 0){
			mw->cddone = 1;
			i = snprint(a, n, "cd %s\n", p->dot->path->s);
			qunlock(&p->debug);
			poperror();
			return i;
		}
		int2flag(mw->cm->mflag, flag);
		if(strcmp(mw->cm->to->path->s, "#M") == 0){
			srv = srvname(mw->cm->to->mchan);
			i = snprint(a, n, "mount %s %s %s %s\n", flag,
				srv==nil? mw->cm->to->mchan->path->s : srv,
				mw->mh->from->path->s, mw->cm->spec? mw->cm->spec : "");
			free(srv);
		}else
			i = snprint(a, n, "bind %s %s %s\n", flag,
				mw->cm->to->path->s, mw->mh->from->path->s);
		qunlock(&p->debug);
		poperror();
		return i;

	case Qnoteid:
		return readnum(offset, va, n, p->noteid, NUMSIZE);
	case Qfd:
		return procfds(p, va, n, offset);
	}
	error(Egreg);
	return 0;		/* not reached */
}

void
mntscan(Mntwalk *mw, Proc *p)
{
	Pgrp *pg;
	Mount *t;
	Mhead *f;
	int nxt, i;
	ulong last, bestmid;

	pg = p->pgrp;
	rlock(&pg->ns);

	nxt = 0;
	bestmid = ~0;

	last = 0;
	if(mw->mh)
		last = mw->cm->mountid;

	for(i = 0; i < MNTHASH; i++) {
		for(f = pg->mnthash[i]; f; f = f->hash) {
			for(t = f->mount; t; t = t->next) {
				if(mw->mh == 0 ||
				  (t->mountid > last && t->mountid < bestmid)) {
					mw->cm = t;
					mw->mh = f;
					bestmid = mw->cm->mountid;
					nxt = 1;
				}
			}
		}
	}
	if(nxt == 0)
		mw->mh = 0;

	runlock(&pg->ns);
}

static long
procwrite(Chan *c, void *va, long n, vlong off)
{
	int id, m;
	Proc *p, *t, *et;
	char *a, *arg, buf[ERRMAX];
	ulong offset = off;

	a = va;
	if(c->qid.type & QTDIR)
		error(Eisdir);

	p = proctab(SLOT(c->qid));

	/* Use the remembered noteid in the channel rather
	 * than the process pgrpid
	 */
	if(QID(c->qid) == Qnotepg) {
		pgrpnote(NOTEID(c->pgrpid), va, n, NUser);
		return n;
	}

	qlock(&p->debug);
	if(waserror()){
		qunlock(&p->debug);
		nexterror();
	}
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	switch(QID(c->qid)){
	case Qargs:
		if(n == 0)
			error(Eshort);
		if(n >= ERRMAX)
			error(Etoobig);
		arg = malloc(n+1);
		if(arg == nil)
			error(Enomem);
		memmove(arg, va, n);
		m = n;
		if(arg[m-1] != 0)
			arg[m++] = 0;
		free(p->args);
		p->nargs = m;
		p->args = arg;
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
		if(offset >= sizeof(FPsave))
			n = 0;
		else if(offset+n > sizeof(FPsave))
			n = sizeof(FPsave) - offset;
		memmove((uchar*)&p->fpsave+offset, va, n);
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
		id = atoi(a);
		if(id == p->pid) {
			p->noteid = id;
			break;
		}
		t = proctab(0);
		for(et = t+conf.nproc; t < et; t++) {
			if(t->state == Dead)
				continue;
			if(id == t->noteid) {
				if(strcmp(p->user, t->user) != 0)
					error(Eperm);
				p->noteid = id;
				break;
			}
		}
		if(p->noteid != id)
			error(Ebadarg);
		break;
	default:
		pprint("unknown qid in procwrite\n");
		error(Egreg);
	}
	poperror();
	qunlock(&p->debug);
	return n;
}

Dev procdevtab = {
	'p',
	"proc",

	devreset,
	procinit,
	devshutdown,
	procattach,
	procwalk,
	procstat,
	procopen,
	devcreate,
	procclose,
	procread,
	devbread,
	procwrite,
	devbwrite,
	devremove,
	procwstat,
};

Chan*
proctext(Chan *c, Proc *p)
{
	Chan *tc;
	Image *i;
	Segment *s;

	s = p->seg[TSEG];
	if(s == 0)
		error(Enonexist);
	if(p->state==Dead)
		error(Eprocdied);

	lock(s);
	i = s->image;
	if(i == 0) {
		unlock(s);
		error(Eprocdied);
	}
	unlock(s);

	lock(i);
	if(waserror()) {
		unlock(i);
		nexterror();
	}

	tc = i->c;
	if(tc == 0)
		error(Eprocdied);

	if(incref(tc) == 1 || (tc->flag&COPEN) == 0 || tc->mode!=OREAD) {
		cclose(tc);
		error(Eprocdied);
	}

	if(p->pid != PID(c->qid)){
		cclose(tc);
		error(Eprocdied);
	}

	unlock(i);
	poperror();

	return tc;
}

void
procstopwait(Proc *p, int ctl)
{
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
	unlock(f);
	qunlock(&p->debug);
	cclose(c);
	qlock(&p->debug);
	lock(f);
}

void
procctlclosefiles(Proc *p, int all, int fd)
{
	int i;
	Fgrp *f;

	f = p->fgrp;
	if(f == nil)
		error(Eprocdied);

	lock(f);
	f->ref++;
	if(all)
		for(i = 0; i < f->maxfd; i++)
			procctlcloseone(p, f, i);
	else
		procctlcloseone(p, f, fd);
	unlock(f);
	closefgrp(f);
}

static char *
parsetime(vlong *rt, char *s)
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

void
procctlreq(Proc *p, char *va, int n)
{
	Segment *s;
	int npc, pri;
	Cmdbuf *cb;
	Cmdtab *ct;
	vlong time;
	char *e;
	void (*pt)(Proc*, int, vlong);

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
		s = p->seg[TSEG];
		if(s == 0 || (s->type&SG_TYPE) != SG_TEXT)
			error(Ebadctl);
		if(s->profile != 0)
			free(s->profile);
		npc = (s->top-s->base)>>LRESPROF;
		s->profile = malloc(npc*sizeof(*s->profile));
		if(s->profile == 0)
			error(Enomem);
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
		procwired(p, atoi(cb->f[1]));
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
		p->edf->T = time/1000;	/* Edf times are in µs */
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
		pt = proctrace;
		if(up->trace && pt)
			pt(up, SUser, 0);
		break;
	}

	poperror();
	free(cb);
}

int
procstopped(void *a)
{
	Proc *p = a;
	return p->state == Stopped;
}

int
procctlmemio(Proc *p, ulong offset, int n, void *va, int read)
{
	KMap *k;
	Pte *pte;
	Page *pg;
	Segment *s;
	ulong soff, l;
	char *a = va, *b;

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
		if(fixfault(s, offset, read, 0) == 0)
			break;
		poperror();
		s->steal--;
	}
	poperror();
	pte = s->map[soff/PTEMAPMEM];
	if(pte == 0)
		panic("procctlmemio");
	pg = pte->pages[(soff&(PTEMAPMEM-1))/BY2PG];
	if(pagedout(pg))
		panic("procctlmemio1");

	l = BY2PG - (offset&(BY2PG-1));
	if(n > l)
		n = l;

	k = kmap(pg);
	if(waserror()) {
		s->steal--;
		kunmap(k);
		nexterror();
	}
	b = (char*)VA(k);
	b += offset&(BY2PG-1);
	if(read == 1)
		memmove(a, b, n);	/* This can fault */
	else
		memmove(b, a, n);
	kunmap(k);
	poperror();

	/* Ensure the process sees text page changes */
	if(s->flushme)
		memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));

	s->steal--;

	if(read == 0)
		p->newtlb = 1;

	return n;
}

Segment*
txt2data(Proc *p, Segment *s)
{
	int i;
	Segment *ps;

	ps = newseg(SG_DATA, s->base, s->size);
	ps->image = s->image;
	incref(ps->image);
	ps->fstart = s->fstart;
	ps->flen = s->flen;
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
	incref(ps->image);
	ps->fstart = s->fstart;
	ps->flen = s->flen;
	ps->flushme = 1;

	return ps;
}
