#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ureg.h"

#include	"devtab.h"

enum{
	Qdir,
	Qctl,
	Qmem,
	Qnote,
	Qnoteid,
	Qnotepg,
	Qproc,
	Qsegment,
	Qstatus,
	Qtext,
	Qwait,
};

#define	STATSIZE	(2*NAMELEN+12+7*12)
Dirtab procdir[] =
{
	"ctl",		{Qctl},		0,			0000,
	"mem",		{Qmem},		0,			0000,
	"note",		{Qnote},	0,			0000,
	"noteid",	{Qnoteid},	0,			0666,
	"notepg",	{Qnotepg},	0,			0000,
	"proc",		{Qproc},	sizeof(Proc),		0444,
	"segment",	{Qsegment},	0,			0444,
	"status",	{Qstatus},	STATSIZE,		0444,
	"text",		{Qtext},	0,			0000,
	"wait",		{Qwait},	0,			0400,
};

/* Segment type from portdat.h */
char *sname[]={ "Text", "Data", "Bss", "Stack", "Shared", "Phys", "Shdata" };

/*
 * Qids are, in path:
 *	 4 bits of file type (qids above)
 *	23 bits of process slot number + 1
 *	     in vers,
 *	32 bits of pid, for consistency checking
 * If notepg, c->pgrpid.path is pgrp slot, .vers is noteid.
 */
#define	NPROC	(sizeof procdir/sizeof(Dirtab))
#define	QSHIFT	4	/* location in qid of proc slot # */

#define	QID(q)		(((q).path&0x0000000F)>>0)
#define	SLOT(q)		((((q).path&0x07FFFFFF0)>>QSHIFT)-1)
#define	PID(q)		((q).vers)
#define	NOTEID(q)	((q).vers)

void	procctlreq(Proc*, char*, int);
int	procctlmemio(Proc*, ulong, int, void*, int);
Chan*	proctext(Chan*, Proc*);
Segment* txt2data(Proc*, Segment*);
int	procstopped(void*);

int
procgen(Chan *c, Dirtab *tab, int ntab, int s, Dir *dp)
{
	Qid qid;
	Proc *p;
	char buf[NAMELEN];
	ulong pid, path, perm, len;

	USED(ntab);
	if(c->qid.path == CHDIR){
		if(s >= conf.nproc)
			return -1;
		p = proctab(s);
		pid = p->pid;
		if(pid == 0)
			return 0;
		sprint(buf, "%d", pid);
		qid = (Qid){CHDIR|((s+1)<<QSHIFT), pid};
		devdir(c, qid, buf, 0, p->user, CHDIR|0555, dp);
		return 1;
	}
	if(s >= NPROC)
		return -1;
	if(tab)
		panic("procgen");

	tab = &procdir[s];
	path = c->qid.path&~(CHDIR|((1<<QSHIFT)-1));	/* slot component */

	p = proctab(SLOT(c->qid));
	perm = tab->perm;
	if(perm == 0)
		perm = p->procmode;

	len = tab->length;
	if(QID(c->qid) == Qwait)
		len = p->nwait * sizeof(Waitmsg);

	qid = (Qid){path|tab->qid.path, c->qid.vers};
	devdir(c, qid, tab->name, len, p->user, perm, dp);
	return 1;
}

void
procinit(void)
{
	if(conf.nproc >= (1<<(16-QSHIFT))-1)
		print("warning: too many procs for devproc\n");
}

void
procreset(void)
{
}

Chan*
procattach(char *spec)
{
	return devattach('p', spec);
}

Chan*
procclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
procwalk(Chan *c, char *name)
{
	if(strcmp(name, "..") == 0) {
		c->qid.path = Qdir|CHDIR;
		return 1;
	}

	return devwalk(c, name, 0, 0, procgen);
}

void
procstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, procgen);
}

Chan *
procopen(Chan *c, int omode)
{
	Proc *p;
	Pgrp *pg;
	Chan *tc;

	if(c->qid.path & CHDIR)
		return devopen(c, omode, 0, 0, procgen);

	p = proctab(SLOT(c->qid));
	pg = p->pgrp;
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	omode = openmode(omode);

	switch(QID(c->qid)){
	case Qtext:
		tc = proctext(c, p);
		tc->offset = 0;
		return tc;

	case Qctl:
	case Qnote:
	case Qnoteid:
	case Qmem:
	case Qsegment:
	case Qproc:
	case Qstatus:
	case Qwait:
		break;

	case Qnotepg:
		if(omode!=OWRITE || pg->pgrpid == 1)
			error(Eperm);
		c->pgrpid.path = pg->pgrpid+1;
		c->pgrpid.vers = p->noteid;
		break;

	default:
		pprint("procopen %lux\n", c->qid);
		error(Egreg);
	}

	/* Affix pid to qid */
	if(p->state != Dead)
		c->qid.vers = p->pid;

	return devopen(c, omode, 0, 0, procgen);
}

void
proccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
procremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
procwstat(Chan *c, char *db)
{
	Proc *p;
	Dir d;

	if(c->qid.path&CHDIR)
		error(Eperm);

	convM2D(db, &d);
	p = proctab(SLOT(c->qid));
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	if(strcmp(u->p->user, p->user) != 0 && strcmp(u->p->user, eve) != 0)
		error(Eperm);

	p->procmode = d.mode&0777;
}

void
procclose(Chan * c)
{
	USED(c);
}

long
procread(Chan *c, void *va, long n, ulong offset)
{
	Proc *p;
	Page *pg;
	KMap *k;
	Segment *s;
	int i, j;
	long l;
	User *up;
	Segment *sg;
	Waitq *wq;
	char statbuf[NSEG*32];
	char *a = va, *b, *sps;

	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, procgen);

	p = proctab(SLOT(c->qid));
	if(p->pid != PID(c->qid))
		error(Eprocdied);

	switch(QID(c->qid)){
	case Qmem:
		/* ugly math: USERADDR+BY2PG may be == 0 */
		if(offset >= USERADDR && offset <= USERADDR+BY2PG-1) {
			if(offset+n >= USERADDR+BY2PG-1)
				n = USERADDR+BY2PG - offset;
			pg = p->upage;
			if(pg==0 || p->pid!=PID(c->qid))
				error(Eprocdied);
			k = kmap(pg);
			b = (char*)VA(k);
			memmove(a, b+(offset-USERADDR), n);
			kunmap(k);
			return n;
		}

		if(offset >= KZERO) {
			/* Protect crypt key memory */
			if(offset < palloc.cmemtop && offset+n > palloc.cmembase)
				error(Eperm);

			/* validate physical kernel addresses */
			if(offset < (ulong)end) {
				if(offset+n > (ulong)end)
					n = (ulong)end - offset;
				memmove(a, (char*)offset, n);
				return n;
			}
			if(offset >= conf.base0 && offset < conf.npage0){
				if(offset+n > conf.npage0)
					n = conf.npage0 - offset;
				memmove(a, (char*)offset, n);
				return n;
			}
			if(offset >= conf.base1 && offset < conf.npage1){
				if(offset+n > conf.npage1)
					n = conf.npage1 - offset;
				memmove(a, (char*)offset, n);
				return n;
			}
		}

		return procctlmemio(p, offset, n, va, 1);

	case Qnote:
		qlock(&p->debug);
		if(waserror()){
			qunlock(&p->debug);
			nexterror();
		}
		if(p->pid != PID(c->qid))
			error(Eprocdied);
		k = kmap(p->upage);
		up = (User*)VA(k);
		if(up->p != p){
			kunmap(k);
			pprint("note read u/p mismatch");
			error(Egreg);
		}
		if(n < ERRLEN)
			error(Etoosmall);
		if(up->nnote == 0)
			n = 0;
		else{
			memmove(va, up->note[0].msg, ERRLEN);
			up->nnote--;
			memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));
			n = ERRLEN;
		}
		if(up->nnote == 0)
			p->notepending = 0;
		kunmap(k);
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

	case Qstatus:
		if(offset >= STATSIZE)
			return 0;
		if(offset+n > STATSIZE)
			n = STATSIZE - offset;

		sps = p->psstate;
		if(sps == 0)
			sps = statename[p->state];
		memset(statbuf, ' ', sizeof statbuf);
		memmove(statbuf+0*NAMELEN, p->text, strlen(p->text));
		memmove(statbuf+1*NAMELEN, p->user, strlen(p->user));
		memmove(statbuf+2*NAMELEN, sps, strlen(sps));
		j = 2*NAMELEN + 12;

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
		memmove(a, statbuf+offset, n);
		return n;

	case Qsegment:
		j = 0;
		for(i = 0; i < NSEG; i++)
			if(sg = p->seg[i])
				j += sprint(&statbuf[j], "%-6s %c %.8lux %.8lux %4d\n",
				sname[sg->type&SG_TYPE], sg->type&SG_RONLY ? 'R' : ' ',
				sg->base, sg->top, sg->ref);
		if(offset >= j)
			return 0;
		if(offset+n > j)
			n = j-offset;
		if(n == 0 && offset == 0)
			exhausted("segments");
		memmove(a, &statbuf[offset], n);
		return n;

	case Qwait:
		if(n < sizeof(Waitmsg))
			error(Etoosmall);

		if(!canqlock(&p->qwaitr))
			error(Einuse);

		if(waserror()) {
			qunlock(&p->qwaitr);
			nexterror();
		}

		lock(&p->exl);
		if(u->p == p && p->nchild == 0 && p->waitq == 0) {
			unlock(&p->exl);
			error(Enochild);
		}
		while(p->waitq == 0) {
			unlock(&p->exl);
			sleep(&p->waitr, haswaitq, p);
			lock(&p->exl);
		}
		wq = p->waitq;
		p->waitq = wq->next;
		p->nwait--;
		unlock(&p->exl);

		qunlock(&p->qwaitr);
		poperror();
		memmove(a, &wq->w, sizeof(Waitmsg));
		free(wq);
		return sizeof(Waitmsg);
	case Qnoteid:
		return readnum(offset, va, n, p->noteid, NUMSIZE);
	}
	error(Egreg);
	return 0;		/* not reached */
}


long
procwrite(Chan *c, void *va, long n, ulong offset)
{
	int id;
	User *up;
	KMap *k;
	Ureg *ur;
	User *pxu;
	Page *pg;
	ulong hi;
	char *a, *b;
	char buf[ERRLEN];
	Proc *p, *t, *et;

	if(c->qid.path & CHDIR)
		error(Eisdir);

	a = va;
	p = proctab(SLOT(c->qid));

	/* Use the remembered noteid in the channel
	 * rather than the process pgrpid
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
	case Qmem:
		if(p->state != Stopped)
			error(Ebadctl);

		if(offset >= USERADDR && offset <= USERADDR+BY2PG-1) {
			pg = p->upage;
			if(pg==0 || p->pid!=PID(c->qid))
				error(Eprocdied);
			k = kmap(pg);
			b = (char*)VA(k);
			pxu = (User*)b;
			hi = offset+n;
			/* Check for floating point registers */
			if(offset >= (ulong)&u->fpsave &&
			   hi <= (ulong)&u->fpsave+sizeof(FPsave)){
				memmove(b+(offset-USERADDR), a, n);
				break;
			}
			/* Check user register set for process at kernel entry */
			ur = pxu->dbgreg;
			if(offset < (ulong)ur || hi > (ulong)ur+sizeof(Ureg)) {
				kunmap(k);
				error(Ebadarg);
			}
			ur = (Ureg*)(b+((ulong)ur-USERADDR));
			setregisters(ur, b+(offset-USERADDR), a, n);
			kunmap(k);
		}
		else	/* Try user memory segments */
			n = procctlmemio(p, offset, n, va, 0);
		break;

	case Qctl:
		procctlreq(p, va, n);
		break;

	case Qnote:
		if(p->kp)
			error(Eperm);
		k = kmap(p->upage);
		up = (User*)VA(k);
		if(up->p != p){
			kunmap(k);
			pprint("note write u/p mismatch");
			error(Egreg);
		}
		kunmap(k);
		if(n >= ERRLEN-1)
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

Chan *
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
		close(tc);
		error(Eprocdied);
	}

	if(p->pid != PID(c->qid))
		error(Eprocdied);

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
	if(procstopped(p))
		return;

	if(ctl != 0)
		p->procctl = ctl;
	p->pdbg = u->p;
	pid = p->pid;
	qunlock(&p->debug);
	u->p->psstate = "Stopwait";
	if(waserror()) {
		p->pdbg = 0;
		qlock(&p->debug);
		nexterror();
	}
	sleep(&u->p->sleep, procstopped, p);
	poperror();
	qlock(&p->debug);
	if(p->pid != pid)
		error(Eprocdied);
}

void
procctlreq(Proc *p, char *va, int n)
{
	int i;
	char buf[NAMELEN+1];

	if(n > NAMELEN)
		n = NAMELEN;
	strncpy(buf, va, n);
	buf[NAMELEN] = '\0';

	if(strncmp(buf, "stop", 4) == 0)
		procstopwait(p, Proc_stopme);
	else if(strncmp(buf, "kill", 4) == 0) {
		switch(p->state) {
		case Broken:
			unbreak(p);
			break;
		case Stopped:
			postnote(p, 0, "sys: killed", NExit);
			p->procctl = Proc_exitme;
			ready(p);
			break;
		default:
			postnote(p, 0, "sys: killed", NExit);
			p->procctl = Proc_exitme;
		}
	}
	else if(strncmp(buf, "hang", 4) == 0)
		p->hang = 1;
	else if(strncmp(buf, "nohang", 6) == 0)
		p->hang = 0;
	else if(strncmp(buf, "waitstop", 8) == 0)
		procstopwait(p, 0);
	else if(strncmp(buf, "startstop", 9) == 0) {
		if(p->state != Stopped)
			error(Ebadctl);
		p->procctl = Proc_traceme;
		ready(p);
		procstopwait(p, Proc_traceme);
	}
	else if(strncmp(buf, "start", 5) == 0) {
		if(p->state != Stopped)
			error(Ebadctl);
		ready(p);
	}
	else if(strncmp(buf, "pri", 3) == 0){
		if(n < 4)
			error(Ebadctl);
		i = atoi(buf+4);
		if(i < 0)
			i = 0;
		if(i >= Nrq)
			i = Nrq - 1;
		if(i < p->basepri && !iseve())
			error(Eperm);
		p->basepri = i;
	}
	else
		error(Ebadctl);
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

		if(read == 0 && (s->type&SG_TYPE) == SG_TEXT)
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
	b = (char*)VA(k);
	if(read == 1)
		memmove(a, b+(offset&(BY2PG-1)), n);
	else
		memmove(b+(offset&(BY2PG-1)), a, n);

	kunmap(k);

	s->steal--;
	qunlock(&s->lk);

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

	for(i = 0; i < NSEG; i++)
		if(p->seg[i] == s)
			break;
	if(p->seg[i] != s)
		panic("segment gone");

	qunlock(&s->lk);
	putseg(s);
	qlock(&ps->lk);
	p->seg[i] = ps;

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
