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
	xsummary();
	print("[%s %s %d] r1 = %d\n", u->p->user, u->p->text, u->p->pid, arg[0]);
	return 0;
}

long
sysrfork(ulong *arg)
{
	KMap *k;
	Pgrp *opg;
	Egrp *oeg;
	Fgrp *ofg;
	int n, i;
	Proc *p, *parent;
	ulong upa, pid, flag;
	/*
	 * used to compute last valid system stack address for copy
	 */
	int lastvar;	

	flag = arg[0];
	p = u->p;
	if((flag&RFPROC) == 0) {
		if(flag & (RFNAMEG|RFCNAMEG)) {
			if((flag & (RFNAMEG|RFCNAMEG)) == (RFNAMEG|RFCNAMEG))
				error(Ebadarg);
			opg = p->pgrp;
			p->pgrp = newpgrp();
			if(flag & RFNAMEG)
				pgrpcpy(p->pgrp, opg);
			closepgrp(opg);
		}
		if(flag & (RFENVG|RFCENVG)) {
			if((flag & (RFENVG|RFCENVG)) == (RFENVG|RFCENVG))
				error(Ebadarg);
			oeg = p->egrp;
			p->egrp = smalloc(sizeof(Egrp));
			p->egrp->ref = 1;
			if(flag & RFENVG)
				envcpy(p->egrp, oeg);
			closeegrp(oeg);
		}
		if(flag & RFFDG) {
			ofg = p->fgrp;
			p->fgrp = dupfgrp(ofg);
			closefgrp(ofg);
		}
		else
		if(flag & RFCFDG) {
			ofg = p->fgrp;
			p->fgrp = smalloc(sizeof(Fgrp));
			p->fgrp->ref = 1;
			closefgrp(ofg);
		}
		if(flag & RFNOTEG)
			p->noteid = incref(&noteidalloc);
		if(flag & (RFMEM|RFNOWAIT))
			error(Ebadarg);
		return 0;
	}
	/* Check flags before we commit */
	if((flag & (RFFDG|RFCFDG)) == (RFFDG|RFCFDG))
		error(Ebadarg);
	if((flag & (RFNAMEG|RFCNAMEG)) == (RFNAMEG|RFCNAMEG))
		error(Ebadarg);
	if((flag & (RFENVG|RFCENVG)) == (RFENVG|RFCENVG))
		error(Ebadarg);

	p = newproc();
	parent = u->p;

	/* Page va of upage used as check in mapstack */
	p->upage = newpage(0, 0, USERADDR|(p->pid&0xFFFF));
	k = kmap(p->upage);
	upa = VA(k);

	/* Save time: only copy u-> data and useful stack */
	memmove((void*)upa, u, sizeof(User));
	n = USERADDR+BY2PG - (ulong)&lastvar;
	n = (n+32) & ~(BY2WD-1);
	memmove((void*)(upa+BY2PG-n), (void*)(USERADDR+BY2PG-n), n);
	((User *)upa)->p = p;
	kunmap(k);

	/* Make a new set of memory segments */
	n = flag & RFMEM;
	for(i = 0; i < NSEG; i++)
		if(parent->seg[i])
			p->seg[i] = dupseg(parent->seg, i, n);
	
	/* Refs */
	incref(u->dot);	

	/* File descriptors */
	if(flag & (RFFDG|RFCFDG)) {
		if(flag & RFFDG)
			p->fgrp = dupfgrp(parent->fgrp);
		else {
			p->fgrp = smalloc(sizeof(Fgrp));
			p->fgrp->ref = 1;
		}
	}
	else {
		p->fgrp = parent->fgrp;
		incref(p->fgrp);
	}

	/* Process groups */
	if(flag & (RFNAMEG|RFCNAMEG)) {	
		p->pgrp = newpgrp();
		if(flag & RFNAMEG)
			pgrpcpy(p->pgrp, parent->pgrp);
	}
	else {
		p->pgrp = parent->pgrp;
		incref(p->pgrp);
	}

	/* Environment group */
	if(flag & (RFENVG|RFCENVG)) {
		p->egrp = smalloc(sizeof(Egrp));
		p->egrp->ref = 1;
		if(flag & RFENVG)
			envcpy(p->egrp, parent->egrp);
	}
	else {
		p->egrp = parent->egrp;
		incref(p->egrp);
	}

	p->hang = parent->hang;
	p->procmode = parent->procmode;

	if(setlabel(&p->sched)){
		/*
		 *  use u->p instead of p, because we don't trust the compiler, after a
		 *  gotolabel, to find the correct contents of a local variable.
		 */
		p = u->p;
		p->state = Running;
		p->mach = m;
		m->proc = p;
		spllo();
		return 0;
	}

	p->parent = parent;
	p->parentpid = parent->pid;
	if(flag&RFNOWAIT)
		p->parentpid = 1;
	else {
		lock(&parent->exl);
		parent->nchild++;
		unlock(&parent->exl);
	}
	if((flag&RFNOTEG) == 0)
		p->noteid = parent->noteid;

	p->fpstate = parent->fpstate;
	pid = p->pid;
	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = MACHP(0)->ticks;
	memmove(p->text, parent->text, NAMELEN);
	memmove(p->user, parent->user, NAMELEN);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	flushmmu();
	p->priority = u->p->priority;
	p->basepri = u->p->basepri;
	p->mp = u->p->mp;
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
	Proc *p;
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

	p = u->p;
	validaddr(arg[0], 1, 0);
	file = (char*)arg[0];
	indir = 0;
    Header:
	tc = namec(file, Aopen, OEXEC, 0);
	if(waserror()){
		close(tc);
		nexterror();
	}
	if(!indir)
		strcpy(elem, u->elem);

	n = (*devtab[tc->type].read)(tc, &exec, sizeof(Exec), 0);
	if(n < 2)
    Err:
		error(Ebadexec);
	magic = l2be(exec.magic);
	text = l2be(exec.text);
	entry = l2be(exec.entry);
	if(n==sizeof(Exec) && magic==AOUT_MAGIC){
		if((text&KZERO)
		|| entry < UTZERO+sizeof(Exec)
		|| entry >= UTZERO+sizeof(Exec)+text)
			goto Err;
		goto Binary;
	}

	/*
	 * Process #! /bin/sh args ...
	 */
	memmove(line, &exec, sizeof(Exec));
	if(indir || line[0]!='#' || line[1]!='!')
		goto Err;
	n = shargs(line, n, progarg);
	if(n == 0)
		goto Err;
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
	close(tc);
	goto Header;

    Binary:
	poperror();
	data = l2be(exec.data);
	bss = l2be(exec.bss);
	t = (UTZERO+sizeof(Exec)+text+(BY2PG-1)) & ~(BY2PG-1);
	d = (t + data + (BY2PG-1)) & ~(BY2PG-1);
	bssend = t + data + bss;
	b = (bssend + (BY2PG-1)) & ~(BY2PG-1);
	if((t|d|b) & KZERO)
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

	p->seg[ESEG] = newseg(SG_STACK, TSTKTOP-USTKSIZE, USTKSIZE/BY2PG);

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

	memmove(p->text, elem, NAMELEN);

	/*
	 * Committed.
	 * Free old memory.
	 * Special segments are maintained accross exec
	 */
	for(i = SSEG; i <= BSEG; i++) {
		putseg(p->seg[i]);
		/* prevent a second free if we have an error */
		p->seg[i] = 0;	   
	}
	for(i = BSEG+1; i < NSEG; i++) {
		s = p->seg[i];
		if(s != 0 && (s->type&SG_CEXEC)) {
			putseg(s);
			p->seg[i] = 0;
		}
	}

	/*
	 * Close on exec
	 */
	f = u->p->fgrp;
	for(i=0; i<=f->maxfd; i++)
		fdclose(i, CCEXEC);

	/* Text.  Shared. Attaches to cache image if possible */
	/* attachimage returns a locked cache image */
	img = attachimage(SG_TEXT|SG_RONLY, tc, UTZERO, (t-UTZERO)>>PGSHIFT);
	ts = img->s;
	p->seg[TSEG] = ts;
	ts->flushme = 1;
	ts->fstart = 0;
	ts->flen = sizeof(Exec)+text;
	unlock(img);

	/* Data. Shared. */
	s = newseg(SG_DATA, t, (d-t)>>PGSHIFT);
	p->seg[DSEG] = s;

	/* Attached by hand */
	incref(img);
	s->image = img;
	s->fstart = ts->fstart+ts->flen;
	s->flen = data;

	/* BSS. Zero fill on demand */
	p->seg[BSEG] = newseg(SG_BSS, d, (b-d)>>PGSHIFT);

	/*
	 * Move the stack
	 */
	s = p->seg[ESEG];
	p->seg[ESEG] = 0;
	p->seg[SSEG] = s;
	s->base = USTKTOP-USTKSIZE;
	s->top = USTKTOP;
	relocateseg(s, TSTKTOP-USTKTOP);

	/*
	 *  '/' processes are higher priority (hack to make /ip more responsive).
	 */
	if(devchar[tc->type] == L'/')
		u->p->basepri = PriRoot;
	u->p->priority = u->p->basepri;
	close(tc);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	flushmmu();
	qlock(&p->debug);
	u->nnote = 0;
	u->notify = 0;
	u->notified = 0;
	procsetup(p);
	qunlock(&p->debug);
	if(p->hang)
		p->procctl = Proc_stopme;

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
	for(;;){
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
return0(void *a)
{
	USED(a);
	return 0;
}

long
syssleep(ulong *arg)
{
	int n;

	n = arg[0];
	if(n == 0){
		sched();	/* yield */
		return 0;
	}
	if(MS2TK(n) == 0)	/* sleep for at least one tick */
		n = TK2MS(1);
	tsleep(&u->p->sleep, return0, 0, n);

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
sysdeath(ulong *arg)
{
	USED(arg);
	pprint("deprecated system call\n");
	pexit("Suicide", 0);
	return 0;	/* not reached */
}

long
syserrstr(ulong *arg)
{
	char tmp[ERRLEN];

	validaddr(arg[0], ERRLEN, 1);
	memmove(tmp, (char*)arg[0], ERRLEN);
	memmove((char*)arg[0], u->error, ERRLEN);
	memmove(u->error, tmp, ERRLEN);
	return 0;
}

long
sysnotify(ulong *arg)
{
	USED(arg);
	if(arg[0] != 0)
		validaddr(arg[0], sizeof(ulong), 0);
	u->notify = (int(*)(void*, char*))(arg[0]);
	return 0;
}

long
sysnoted(ulong *arg)
{
	if(arg[0]!=NRSTR && !u->notified)
		error(Egreg);
	return 0;
}

long
syssegbrk(ulong *arg)
{
	Segment *s;
	int i;

	for(i = 0; i < NSEG; i++) {
		if(s = u->p->seg[i]) {
			if(arg[0] >= s->base && arg[0] < s->top) {
				switch(s->type&SG_TYPE) {
				case SG_TEXT:
				case SG_DATA:
				case SG_STACK:
					error(Ebadarg);
				default:
					return ibrk(arg[1], i);
				}
			}
		}
	}

	error(Ebadarg);
	return 0;		/* not reached */
}

long
syssegattach(ulong *arg)
{
	return segattach(u->p, arg[0], (char*)arg[1], arg[2], arg[3]);
}

long
syssegdetach(ulong *arg)
{
	int i;
	Segment *s;

	s = 0;
	for(i = 0; i < NSEG; i++)
		if(s = u->p->seg[i]) {
			qlock(&s->lk);
			if((arg[0] >= s->base && arg[0] < s->top) || 
			   (s->top == s->base && arg[0] == s->base))
				goto found;
			qunlock(&s->lk);
		}

	error(Ebadarg);

found:
	if((ulong)arg >= s->base && (ulong)arg < s->top) {
		qunlock(&s->lk);
		error(Ebadarg);
	}
	u->p->seg[i] = 0;
	qunlock(&s->lk);
	putseg(s);

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
	s = seg(u->p, from, 1);
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

/* For binary compatability */
long
sysbrk_(ulong *arg)
{
	return ibrk(arg[0], BSEG);
}

long
sysrendezvous(ulong *arg)
{
	Proc *p, **l;
	int tag;
	ulong val;

	tag = arg[0];
	l = &REND(u->p->pgrp, tag);

	lock(u->p->pgrp);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = arg[1];
			/* Hard race avoidance */
			while(p->mach != 0)
				;
			ready(p);
			unlock(u->p->pgrp);
			return val;	
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	p = u->p;
	p->rendtag = tag;
	p->rendval = arg[1];
	p->rendhash = *l;
	*l = p;
	u->p->state = Rendezvous;
	unlock(p->pgrp);

	sched();

	return u->p->rendval;
}
