#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<a.out.h>

int	shargs(char*, int, char**);

long
sysr1(ulong *arg)
{
	print("[%s %s %lud] r1 = %lud\n", up->user, up->text, up->pid, arg[0]);
	return 0;
}

long
sysrfork(ulong *arg)
{
	Proc *p;
	int n, i;
	Fgrp *ofg;
	Pgrp *opg;
	Rgrp *org;
	Egrp *oeg;
	ulong pid, flag;
	Mach *wm;

	flag = arg[0];
	/* Check flags before we commit */
	if((flag & (RFFDG|RFCFDG)) == (RFFDG|RFCFDG))
		error(Ebadarg);
	if((flag & (RFNAMEG|RFCNAMEG)) == (RFNAMEG|RFCNAMEG))
		error(Ebadarg);
	if((flag & (RFENVG|RFCENVG)) == (RFENVG|RFCENVG))
		error(Ebadarg);

	if((flag&RFPROC) == 0) {
		if(flag & (RFMEM|RFNOWAIT))
			error(Ebadarg);
		if(flag & (RFFDG|RFCFDG)) {
			ofg = up->fgrp;
			if(flag & RFFDG)
				up->fgrp = dupfgrp(ofg);
			else
				up->fgrp = dupfgrp(nil);
			closefgrp(ofg);
		}
		if(flag & (RFNAMEG|RFCNAMEG)) {
			opg = up->pgrp;
			up->pgrp = newpgrp();
			if(flag & RFNAMEG)
				pgrpcpy(up->pgrp, opg);
			/* inherit noattach */
			up->pgrp->noattach = opg->noattach;
			closepgrp(opg);
		}
		if(flag & RFNOMNT)
			up->pgrp->noattach = 1;
		if(flag & RFREND) {
			org = up->rgrp;
			up->rgrp = newrgrp();
			closergrp(org);
		}
		if(flag & (RFENVG|RFCENVG)) {
			oeg = up->egrp;
			up->egrp = smalloc(sizeof(Egrp));
			up->egrp->ref = 1;
			if(flag & RFENVG)
				envcpy(up->egrp, oeg);
			closeegrp(oeg);
		}
		if(flag & RFNOTEG)
			up->noteid = incref(&noteidalloc);
		return 0;
	}

	p = newproc();

	p->fpsave = up->fpsave;
	p->scallnr = up->scallnr;
	p->s = up->s;
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	incref(p->dot);

	memmove(p->note, up->note, sizeof(p->note));
	p->nnote = up->nnote;
	p->notified = 0;
	p->lastnote = up->lastnote;
	p->notify = up->notify;
	p->ureg = up->ureg;
	p->dbgreg = 0;

	/* Make a new set of memory segments */
	n = flag & RFMEM;
	qlock(&p->seglock);
	if(waserror()){
		qunlock(&p->seglock);
		nexterror();
	}
	for(i = 0; i < NSEG; i++)
		if(up->seg[i])
			p->seg[i] = dupseg(up->seg, i, n);
	qunlock(&p->seglock);
	poperror();

	/* File descriptors */
	if(flag & (RFFDG|RFCFDG)) {
		if(flag & RFFDG)
			p->fgrp = dupfgrp(up->fgrp);
		else
			p->fgrp = dupfgrp(nil);
	}
	else {
		p->fgrp = up->fgrp;
		incref(p->fgrp);
	}

	/* Process groups */
	if(flag & (RFNAMEG|RFCNAMEG)) {
		p->pgrp = newpgrp();
		if(flag & RFNAMEG)
			pgrpcpy(p->pgrp, up->pgrp);
		/* inherit noattach */
		p->pgrp->noattach = up->pgrp->noattach;
	}
	else {
		p->pgrp = up->pgrp;
		incref(p->pgrp);
	}
	if(flag & RFNOMNT)
		up->pgrp->noattach = 1;

	if(flag & RFREND)
		p->rgrp = newrgrp();
	else {
		incref(up->rgrp);
		p->rgrp = up->rgrp;
	}

	/* Environment group */
	if(flag & (RFENVG|RFCENVG)) {
		p->egrp = smalloc(sizeof(Egrp));
		p->egrp->ref = 1;
		if(flag & RFENVG)
			envcpy(p->egrp, up->egrp);
	}
	else {
		p->egrp = up->egrp;
		incref(p->egrp);
	}
	p->hang = up->hang;
	p->procmode = up->procmode;

	/* Craft a return frame which will cause the child to pop out of
	 * the scheduler in user mode with the return register zero
	 */
	forkchild(p, up->dbgreg);

	p->parent = up;
	p->parentpid = up->pid;
	if(flag&RFNOWAIT)
		p->parentpid = 1;
	else {
		lock(&up->exl);
		up->nchild++;
		unlock(&up->exl);
	}
	if((flag&RFNOTEG) == 0)
		p->noteid = up->noteid;

	p->fpstate = up->fpstate;
	pid = p->pid;
	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = MACHP(0)->ticks;
	memmove(p->text, up->text, NAMELEN);
	memmove(p->user, up->user, NAMELEN);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	flushmmu();
	p->priority = up->priority;
	p->basepri = up->basepri;
	p->fixedpri = up->fixedpri;
	p->mp = up->mp;
	wm = up->wired;
	if(wm)
		procwired(p, wm->machno);
	ready(p);
	sched();
	return pid;
}

static ulong
l2be(long l)
{
	uchar *cp;

	cp = (uchar*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}

long
sysexec(ulong *arg)
{
	Segment *s, *ts;
	ulong t, d, b;
	int i;
	Chan *tc;
	char **argv, **argp;
	char *a, *charp, *file;
	char *progarg[sizeof(Exec)/2+1], elem[NAMELEN];
	ulong ssize, spage, nargs, nbytes, n, bssend;
	int indir;
	Exec exec;
	char line[sizeof(Exec)];
	Fgrp *f;
	Image *img;
	ulong magic, text, entry, data, bss;

	validaddr(arg[0], 1, 0);
	file = (char*)arg[0];
	indir = 0;
	for(;;){
		tc = namec(file, Aopen, OEXEC, 0);
		if(waserror()){
			cclose(tc);
			nexterror();
		}
		if(!indir)
			strcpy(elem, up->elem);

		n = devtab[tc->type]->read(tc, &exec, sizeof(Exec), 0);
		if(n < 2)
			error(Ebadexec);
		magic = l2be(exec.magic);
		text = l2be(exec.text);
		entry = l2be(exec.entry);
		if(n==sizeof(Exec) && (magic == AOUT_MAGIC)){
			if((text&KZERO) == KZERO
			|| entry < UTZERO+sizeof(Exec)
			|| entry >= UTZERO+sizeof(Exec)+text)
				error(Ebadexec);
			break; /* for binary */
		}

		/*
		 * Process #! /bin/sh args ...
		 */
		memmove(line, &exec, sizeof(Exec));
		if(indir || line[0]!='#' || line[1]!='!')
			error(Ebadexec);
		n = shargs(line, n, progarg);
		if(n == 0)
			error(Ebadexec);
		indir = 1;
		/*
		 * First arg becomes complete file name
		 */
		progarg[n++] = file;
		progarg[n] = 0;
		validaddr(arg[1], BY2WD, 1);
		arg[1] += BY2WD;
		file = progarg[0];
		progarg[0] = elem;
		poperror();
		cclose(tc);
	}

	data = l2be(exec.data);
	bss = l2be(exec.bss);
	t = (UTZERO+sizeof(Exec)+text+(BY2PG-1)) & ~(BY2PG-1);
	d = (t + data + (BY2PG-1)) & ~(BY2PG-1);
	bssend = t + data + bss;
	b = (bssend + (BY2PG-1)) & ~(BY2PG-1);
	if(((t|d|b) & KZERO) == KZERO)
		error(Ebadexec);

	/*
	 * Args: pass 1: count
	 */
	nbytes = BY2WD;		/* hole for profiling clock at top of stack */
	nargs = 0;
	if(indir){
		argp = progarg;
		while(*argp){
			a = *argp++;
			nbytes += strlen(a) + 1;
			nargs++;
		}
	}
	evenaddr(arg[1]);
	argp = (char**)arg[1];
	validaddr((ulong)argp, BY2WD, 0);
	while(*argp){
		a = *argp++;
		if(((ulong)argp&(BY2PG-1)) < BY2WD)
			validaddr((ulong)argp, BY2WD, 0);
		validaddr((ulong)a, 1, 0);
		nbytes += (vmemchr(a, 0, 0x7FFFFFFF) - a) + 1;
		nargs++;
	}
	ssize = BY2WD*(nargs+1) + ((nbytes+(BY2WD-1)) & ~(BY2WD-1));

	/*
	 * 8-byte align SP for those (e.g. sparc) that need it.
	 * execregs() will subtract another 4 bytes for argc.
	 */
	if((ssize+4) & 7)
		ssize += 4;
	spage = (ssize+(BY2PG-1)) >> PGSHIFT;

	/*
	 * Build the stack segment, putting it in kernel virtual for the moment
	 */
	if(spage > TSTKSIZ)
		error(Enovmem);

	qlock(&up->seglock);
	if(waserror()){
		qunlock(&up->seglock);
		nexterror();
	}
	up->seg[ESEG] = newseg(SG_STACK, TSTKTOP-USTKSIZE, USTKSIZE/BY2PG);

	/*
	 * Args: pass 2: assemble; the pages will be faulted in
	 */
	argv = (char**)(TSTKTOP - ssize);
	charp = (char*)(TSTKTOP - nbytes);
	if(indir)
		argp = progarg;
	else
		argp = (char**)arg[1];

	for(i=0; i<nargs; i++){
		if(indir && *argp==0) {
			indir = 0;
			argp = (char**)arg[1];
		}
		*argv++ = charp + (USTKTOP-TSTKTOP);
		n = strlen(*argp) + 1;
		memmove(charp, *argp++, n);
		charp += n;
	}

	memmove(up->text, elem, NAMELEN);

	/*
	 * Committed.
	 * Free old memory.
	 * Special segments are maintained across exec
	 */
	for(i = SSEG; i <= BSEG; i++) {
		putseg(up->seg[i]);
		/* prevent a second free if we have an error */
		up->seg[i] = 0;
	}
	for(i = BSEG+1; i < NSEG; i++) {
		s = up->seg[i];
		if(s != 0 && (s->type&SG_CEXEC)) {
			putseg(s);
			up->seg[i] = 0;
		}
	}

	/*
	 * Close on exec
	 */
	f = up->fgrp;
	for(i=0; i<=f->maxfd; i++)
		fdclose(i, CCEXEC);

	/* Text.  Shared. Attaches to cache image if possible */
	/* attachimage returns a locked cache image */
	img = attachimage(SG_TEXT|SG_RONLY, tc, UTZERO, (t-UTZERO)>>PGSHIFT);
	ts = img->s;
	up->seg[TSEG] = ts;
	ts->flushme = 1;
	ts->fstart = 0;
	ts->flen = sizeof(Exec)+text;
	unlock(img);

	/* Data. Shared. */
	s = newseg(SG_DATA, t, (d-t)>>PGSHIFT);
	up->seg[DSEG] = s;

	/* Attached by hand */
	incref(img);
	s->image = img;
	s->fstart = ts->fstart+ts->flen;
	s->flen = data;

	/* BSS. Zero fill on demand */
	up->seg[BSEG] = newseg(SG_BSS, d, (b-d)>>PGSHIFT);

	/*
	 * Move the stack
	 */
	s = up->seg[ESEG];
	up->seg[ESEG] = 0;
	up->seg[SSEG] = s;
	qunlock(&up->seglock);
	poperror();
	s->base = USTKTOP-USTKSIZE;
	s->top = USTKTOP;
	relocateseg(s, USTKTOP-TSTKTOP);

	/*
	 *  '/' processes are higher priority (hack to make /ip more responsive).
	 */
	if(devtab[tc->type]->dc == L'/')
		up->basepri = PriRoot;
	up->priority = up->basepri;
	poperror();
	cclose(tc);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	flushmmu();
	qlock(&up->debug);
	up->nnote = 0;
	up->notify = 0;
	up->notified = 0;
	procsetup(up);
	qunlock(&up->debug);
	if(up->hang)
		up->procctl = Proc_stopme;

	return execregs(entry, ssize, nargs);
}

int
shargs(char *s, int n, char **ap)
{
	int i;

	s += 2;
	n -= 2;		/* skip #! */
	for(i=0; s[i]!='\n'; i++)
		if(i == n-1)
			return 0;
	s[i] = 0;
	*ap = 0;
	i = 0;
	for(;;) {
		while(*s==' ' || *s=='\t')
			s++;
		if(*s == 0)
			break;
		i++;
		*ap++ = s;
		*ap = 0;
		while(*s && *s!=' ' && *s!='\t')
			s++;
		if(*s == 0)
			break;
		else
			*s++ = 0;
	}
	return i;
}

int
return0(void*)
{
	return 0;
}

long
syssleep(ulong *arg)
{

	int n;

	n = arg[0];
	if(n <= 0) {
		up->priority = 0;
		sched();
		return 0;
	}
	if(n < TK2MS(1))
		n = TK2MS(1);
	tsleep(&up->sleep, return0, 0, n);
	return 0;
}

long
sysalarm(ulong *arg)
{
	return procalarm(arg[0]);
}

long
sysexits(ulong *arg)
{
	char *status;
	char *inval = "invalid exit string";
	char buf[ERRLEN];

	status = (char*)arg[0];
	if(status){
		if(waserror())
			status = inval;
		else{
			validaddr((ulong)status, 1, 0);
			if(vmemchr(status, 0, ERRLEN) == 0){
				memmove(buf, status, ERRLEN);
				buf[ERRLEN-1] = 0;
				status = buf;
			}
		}
		poperror();

	}
	pexit(status, 1);
	return 0;		/* not reached */
}

long
syswait(ulong *arg)
{
	if(arg[0]){
		validaddr(arg[0], sizeof(Waitmsg), 1);
		evenaddr(arg[0]);
	}
	return pwait((Waitmsg*)arg[0]);
}

long
sysdeath(ulong*)
{
	pprint("deprecated system call\n");
	pexit("Suicide", 0);
	return 0;	/* not reached */
}

void
werrstr(char *fmt, ...)
{
	va_list va;

	if(up == nil)
		return;

	va_start(va, fmt);
	doprint(up->error, up->error+ERRLEN, fmt, va);
	va_end(va);
}

long
syserrstr(ulong *arg)
{
	char *e, tmp[ERRLEN];

	validaddr(arg[0], ERRLEN, 1);
	e = (char*)arg[0];
	memmove(tmp, e, ERRLEN);
	memmove(e, up->error, ERRLEN);
	memmove(up->error, tmp, ERRLEN);
	return 0;
}

long
sysnotify(ulong *arg)
{
	if(arg[0] != 0)
		validaddr(arg[0], sizeof(ulong), 0);
	up->notify = (int(*)(void*, char*))(arg[0]);
	return 0;
}

long
sysnoted(ulong *arg)
{
	if(arg[0]!=NRSTR && !up->notified)
		error(Egreg);
	return 0;
}

long
syssegbrk(ulong *arg)
{
	int i;
	ulong addr;
	Segment *s;

	addr = arg[0];
	for(i = 0; i < NSEG; i++) {
		s = up->seg[i];
		if(s == 0 || addr < s->base || addr >= s->top)
			continue;
		switch(s->type&SG_TYPE) {
		case SG_TEXT:
		case SG_DATA:
		case SG_STACK:
			error(Ebadarg);
		default:
			return ibrk(arg[1], i);
		}
	}

	error(Ebadarg);
	return 0;		/* not reached */
}

long
syssegattach(ulong *arg)
{
	return segattach(up, arg[0], (char*)arg[1], arg[2], arg[3]);
}

long
syssegdetach(ulong *arg)
{
	int i;
	ulong addr;
	Segment *s;

	qlock(&up->seglock);
	if(waserror()){
		qunlock(&up->seglock);
		nexterror();
	}

	s = 0;
	addr = arg[0];
	for(i = 0; i < NSEG; i++)
		if(s = up->seg[i]) {
			qlock(&s->lk);
			if((addr >= s->base && addr < s->top) ||
			   (s->top == s->base && addr == s->base))
				goto found;
			qunlock(&s->lk);
		}

	error(Ebadarg);

found:
	/* Check we are not detaching the current stack segment */
	if((ulong)arg >= s->base && (ulong)arg < s->top) {
		qunlock(&s->lk);
		error(Ebadarg);
	}
	up->seg[i] = 0;
	qunlock(&s->lk);
	putseg(s);
	qunlock(&up->seglock);
	poperror();

	/* Ensure we flush any entries from the lost segment */
	flushmmu();
	return 0;
}

long
syssegfree(ulong *arg)
{
	Segment *s;
	ulong from, pages;

	from = PGROUND(arg[0]);
	s = seg(up, from, 1);
	if(s == 0)
		error(Ebadarg);

	pages = (arg[1]+BY2PG-1)/BY2PG;

	if(from+pages*BY2PG > s->top) {
		qunlock(&s->lk);
		error(Ebadarg);
	}

	mfreeseg(s, from, pages);
	qunlock(&s->lk);
	flushmmu();

	return 0;
}

/* For binary compatibility */
long
sysbrk_(ulong *arg)
{
	return ibrk(arg[0], BSEG);
}

long
sysrendezvous(ulong *arg)
{
	ulong tag;
	ulong val;
	Proc *p, **l;

	tag = arg[0];
	l = &REND(up->rgrp, tag);

	lock(up->rgrp);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = arg[1];

			while(p->mach != 0)
				;
			ready(p);
			unlock(up->rgrp);
			return val;
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	up->rendtag = tag;
	up->rendval = arg[1];
	up->rendhash = *l;
	*l = up;
	up->state = Rendezvous;
	unlock(up->rgrp);

	sched();

	return up->rendval;
}
