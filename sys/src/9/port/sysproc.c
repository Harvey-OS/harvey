/*
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"tos.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

#include <edf.h>
#include	<trace.h>

#undef DBG
#define DBG if(0)print


void
sysrfork(Ar0* ar0, ...)
{
	Proc *up = externup();
	Proc *p;
	int flag, i, n, pid;
	Fgrp *ofg;
	Pgrp *opg;
	Rgrp *org;
	Egrp *oeg;
	Mach *wm;
	va_list list;
	va_start(list, ar0);

	/*
	 * int rfork(int);
	 */
	flag = va_arg(list, int);
	va_end(list);

	/* Check flags before we commit */
	if((flag & (RFFDG|RFCFDG)) == (RFFDG|RFCFDG))
		error(Ebadarg);
	if((flag & (RFNAMEG|RFCNAMEG)) == (RFNAMEG|RFCNAMEG))
		error(Ebadarg);
	if((flag & (RFENVG|RFCENVG)) == (RFENVG|RFCENVG))
		error(Ebadarg);
	if((flag & (RFPREPAGE|RFCPREPAGE)) == (RFPREPAGE|RFCPREPAGE))
		error(Ebadarg);
	if((flag & (RFCORE|RFCCORE)) == (RFCORE|RFCCORE))
		error(Ebadarg);
	if(flag & RFCORE && up->wired != nil)
		error("wired proc cannot move to ac");

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
			up->egrp->r.ref = 1;
			if(flag & RFENVG)
				envcpy(up->egrp, oeg);
			closeegrp(oeg);
		}
		if(flag & RFNOTEG)
			up->noteid = incref(&noteidalloc);
		if(flag & (RFPREPAGE|RFCPREPAGE)){
			up->prepagemem = flag&RFPREPAGE;
			nixprepage(-1);
		}
		if(flag & RFCORE){
			up->ac = getac(up, -1);
			up->procctl = Proc_toac;
		}else if(flag & RFCCORE){
			if(up->ac != nil)
				up->procctl = Proc_totc;
		}

		ar0->i = 0;
		return;
	}

	p = newproc();

	if(flag & RFCORE){
		if(!waserror()){
			p->ac = getac(p, -1);
			p->procctl = Proc_toac;
			poperror();
		}else{
			print("warning: rfork: no available ac for the child, it runs in the tc\n");
			p->procctl = 0;
		}
	}

	if(up->trace)
		p->trace = 1;
	p->scallnr = up->scallnr;
	memmove(p->arg, up->arg, sizeof(up->arg));
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	incref(&p->dot->r);

	memmove(p->note, up->note, sizeof(p->note));
	p->privatemem = up->privatemem;
	p->noswap = up->noswap;
	p->nnote = up->nnote;
	p->notified = 0;
	p->lastnote = up->lastnote;
	p->notify = up->notify;
	p->ureg = up->ureg;
	p->prepagemem = up->prepagemem;
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
		incref(&p->fgrp->r);
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
		incref(&p->pgrp->r);
	}
	if(flag & RFNOMNT)
		up->pgrp->noattach = 1;

	if(flag & RFREND)
		p->rgrp = newrgrp();
	else {
		incref(&up->rgrp->r);
		p->rgrp = up->rgrp;
	}

	/* Environment group */
	if(flag & (RFENVG|RFCENVG)) {
		p->egrp = smalloc(sizeof(Egrp));
		p->egrp->r.ref = 1;
		if(flag & RFENVG)
			envcpy(p->egrp, up->egrp);
	}
	else {
		p->egrp = up->egrp;
		incref(&p->egrp->r);
	}
	p->hang = up->hang;
	p->procmode = up->procmode;

	/* Craft a return frame which will cause the child to pop out of
	 * the scheduler in user mode with the return register zero
	 */
	sysrforkchild(p, up);

	p->parent = up;
	p->parentpid = up->pid;
	if(flag&RFNOWAIT)
		p->parentpid = 0;
	else {
		lock(&up->exl);
		up->nchild++;
		unlock(&up->exl);
	}
	if((flag&RFNOTEG) == 0)
		p->noteid = up->noteid;

	pid = p->pid;
	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = sys->ticks;

	if(flag & (RFPREPAGE|RFCPREPAGE)){
		p->prepagemem = flag&RFPREPAGE;
		/*
		 * BUG: this is prepaging our memory, not
		 * that of the child, but at least we
		 * will do the copy on write.
		 */
		nixprepage(-1);
	}

	kstrdup(&p->text, up->text);
	kstrdup(&p->user, up->user);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	mmuflush();
	p->basepri = up->basepri;
	p->priority = up->basepri;
	p->fixedpri = up->fixedpri;
	p->mp = up->mp;

	wm = up->wired;
	if(wm)
		procwired(p, wm->machno);
	p->color = up->color;
	ready(p);
	sched();

	ar0->i = pid;
}
#if 0
static uint64_t
vl2be(uint64_t v)
{
	uint8_t *p;

	p = (uint8_t*)&v;
	return ((uint64_t)((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3])<<32)
	      |((uint64_t)(p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7]);
}


static uint32_t
l2be(int32_t l)
{
	uint8_t *cp;

	cp = (uint8_t*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}
#endif

/*
 * flags can ONLY specify that you want an AC for you, or
 * that you want an XC for you.
 */
static void
execac(Ar0* ar0, int flags, char *ufile, char **argv)
{
	Proc *up = externup();
	Fgrp *fg;
	Tos *tos;
	Chan *chan, *ichan;
	Image *img;
	Segment *s;
	Ldseg *ldseg;
	int argc, i, n, nldseg;
	char *a, *elem, *file, *p;
	char line[64], *progarg[sizeof(line)/2+1];
	int32_t hdrsz;
	uintptr_t entry, stack;


	file = nil;
	elem = nil;
	switch(flags){
	case EXTC:
	case EXXC:
		break;
	case EXAC:
		up->ac = getac(up, -1);
		break;
	default:
		error("unknown execac flag");
	}
	if(waserror()){
		DBG("execac: failing: %s\n", up->errstr);
		free(file);
		free(elem);
		if(flags == EXAC && up->ac != nil)
			up->ac->proc = nil;
		up->ac = nil;
		nexterror();
	}

	/*
	 * Open the file, remembering the final element and the full name.
	 */
	argc = 0;
	file = validnamedup(ufile, 1);
	DBG("execac: up %#p file %s\n", up, file);
	if(up->trace)
		proctracepid(up);
	ichan = namec(file, Aopen, OEXEC, 0);
	if(waserror()){
		iprint("ERROR ON OPEN\n");
		cclose(ichan);
		nexterror();
	}
	kstrdup(&elem, up->genbuf);

	/*
	 * Read the header.
	 * If it's a #!, fill in progarg[] with info then read a new header
	 * from the file indicated by the #!.
	 * The #! line must be less than sizeof(Exec) in size,
	 * including the terminating \n.
	 */
	hdrsz = ichan->dev->read(ichan, line, sizeof line, 0);
	if(hdrsz < 2)
		error(Ebadexec);
	if(line[0] == '#' && line[1] == '!'){
		p = memchr(line, '\n', MIN(sizeof line, hdrsz));
		if(p == nil)
			error(Ebadexec);
		*p = '\0';
		argc = tokenize(line+2, progarg, nelem(progarg));
		if(argc == 0)
			error(Ebadexec);

		/* The original file becomes an extra arg after #! line */
		progarg[argc++] = file;

		/*
		 * Take the #! $0 as a file to open, and replace
		 * $0 with the original path's name.
		 */
		p = progarg[0];
		progarg[0] = elem;
		chan = nil;	/* in case namec errors out */
		USED(chan);
		chan = namec(p, Aopen, OEXEC, 0);
	}else{
		chan = ichan;
		incref(&ichan->r);
	}
	/* chan is the chan to use, initial or not. ichan is irrelevant now */
	cclose(ichan);
	poperror();

	/*
	 * #! has had its chance, now we need a real binary.
	 */

	nldseg = elf64ldseg(chan, &entry, &ldseg, cputype, BIGPGSZ);
	if(nldseg == 0){
		print("execac: elf64ldseg returned 0 segs!\n");
		error(Ebadexec);
	}

	/* TODO(aki): not sure I see the point
	if(up->ac != nil && up->ac != machp())
		up->color = corecolor(up->ac->machno);
	else
		up->color = corecolor(machp()->machno);
	*/

	/*
	 * The new stack is temporarily mapped elsewhere.
	 * The stack contains, in descending address order:
	 *	a structure containing housekeeping and profiling data (Tos);
	 *	argument strings;
	 *	array of vectors to the argument strings with a terminating
	 *	nil (argv).
	 * When the exec is committed, this temporary stack is relocated
	 * to become the actual stack segment.
	 * The architecture-dependent code which jumps to the new image
	 * will also push a count of the argument array onto the stack (argc).
	 */
	qlock(&up->seglock);
	int sno = -1;
	if(waserror()){
		if(sno != -1 && up->seg[sno] != nil){
			putseg(up->seg[sno]);
			up->seg[sno] = nil;
		}
		qunlock(&up->seglock);
		nexterror();
	}

	for(i = 0; i < NSEG; i++)
		if(up->seg[i] == nil)
			break;
	if(i == NSEG)
		error("exeac: no free segment slots");
	sno = i;
	up->seg[sno] = newseg(SG_STACK|SG_READ|SG_WRITE, TSTKTOP-USTKSIZE, USTKSIZE/BIGPGSZ);
	up->seg[sno]->color = up->color;

	/*
	 * Stack is a pointer into the temporary stack
	 * segment, and will move as items are pushed.
	 */
	stack = TSTKTOP-sizeof(Tos);

	/*
	 * First, the top-of-stack structure.
	 */
	tos = (Tos*)stack;
	tos->cyclefreq = sys->cyclefreq;
	cycles((uint64_t*)&tos->pcycles);
	tos->pcycles = -tos->pcycles;
	tos->kcycles = tos->pcycles;
	tos->clock = 0;

	/*
	 * Next push any arguments found from a #! header.
	 */
	for(i = 0; i < argc; i++){
		n = strlen(progarg[i])+1;
		stack -= n;
		memmove(UINT2PTR(stack), progarg[i], n);
	}

	/*
	 * Copy the strings pointed to by the syscall argument argv into
	 * the temporary stack segment, being careful to check
	 * the strings argv points to are valid.
	 */
	for(i = 0;; i++, argv++){

		a = *(char**)validaddr(argv, sizeof(char**), 0);
		if(a == nil)
			break;
		a = validaddr(a, 1, 0);
		n = ((char*)vmemchr(a, 0, 0x7fffffff) - a) + 1;

		/*
		 * This futzing is so argv[0] gets validated even
		 * though it will be thrown away if this is a shell
		 * script.
		 */
		if(argc > 0 && i == 0)
			continue;
		/*
		 * Before copying the string into the temporary stack,
		 * which might involve a demand-page, check the string
		 * will not overflow the bottom of the stack.
		 */
		stack -= n;
		if(stack < TSTKTOP-USTKSIZE)
			error(Enovmem);
		p = UINT2PTR(stack);
		memmove(p, a, n);
		p[n-1] = 0;
		argc++;
	}
	if(argc < 1)
		error(Ebadexec);

	/*
	 * Before pushing the argument pointers onto the temporary stack,
	 * which might involve a demand-page, check there is room for the
	 * terminating nil pointer, plus pointers, plus some slop for however
	 * argc might be passed on the stack by sysexecregs (give a page
	 * of slop, it is an overestimate, but why not).
	 * Sysexecstack does any architecture-dependent stack alignment.
	 * Keep a copy of the start of the argument strings before alignment
	 * so up->args can be created later.
	 * Although the argument vectors are being pushed onto the stack in
	 * the temporary segment, the values must be adjusted to reflect
	 * the segment address after it replaces the current SSEG.
	 */
	a = p = UINT2PTR(stack);
	stack = sysexecstack(stack, argc);
	if(stack-(argc+2)*sizeof(char**)-BIGPGSZ < TSTKTOP-USTKSIZE) {
		//iprint("stck too small?\n");
		error(Ebadexec);
	}

	argv = (char**)stack;
	*--argv = nil;
	for(i = 0; i < argc; i++){
		*--argv = p + (USTKTOP-TSTKTOP);
		p += strlen(p) + 1;
	}
	*--argv = (void *)(uintptr_t) argc;

	/*
	 * Make a good faith copy of the args in up->args using the strings
	 * in the temporary stack segment. The length must be > 0 as it
	 * includes the \0 on the last argument and argc was checked earlier
	 * to be > 0. After the memmove, compensate for any UTF character
	 * boundary before placing the terminating \0.
	 */
	n = p - a;
	if(n <= 0)
		error(Egreg);
	if(n > 128)
		n = 128;

	p = smalloc(n);
	if(waserror()){
		free(p);
		nexterror();
	}

	memmove(p, a, n);
	while(n > 0 && (p[n-1] & 0xc0) == 0x80)
		n--;
	p[n-1] = '\0';

	/*
	 * All the argument processing is now done, ready to commit.
	 */
	free(up->text);
	up->text = elem;
	elem = nil;
	free(up->args);
	up->args = p;
	up->nargs = n;
	poperror();				/* p (up->args) */

	/*
	 * Close on exec
	 */
	fg = up->fgrp;
	for(i=0; i<=fg->maxfd; i++)
		fdclose(i, CCEXEC);

	/*
	 * Free old memory, except for the temp stack (obviously)
	 */
	s = up->seg[sno];
	for(i = 0; i < NSEG; i++) {
		if(up->seg[i] != s)
			putseg(up->seg[i]);
		up->seg[i] = nil;
	}

	/* put the stack in first */
	sno = 0;
	up->seg[sno++] = s;
	s->base = USTKTOP-USTKSIZE;
	s->top = USTKTOP;
	relocateseg(s, USTKTOP-TSTKTOP);

	img = nil;
	uintptr_t datalim;
	datalim = 0;
	for(i = 0; i < nldseg; i++){

		if(img == nil){
			img = attachimage(ldseg[i].type, chan, up->color,
				ldseg[i].pg0vaddr,
				(ldseg[i].pg0off+ldseg[i].memsz+BIGPGSZ-1)/BIGPGSZ
			);
			s = img->s;
			s->flushme = 1;
			if(img->color != up->color)
				up->color = img->color;
			unlock(&img->r.l);
		} else {
			s = newseg(ldseg[i].type, ldseg[i].pg0vaddr, (ldseg[i].pg0off+ldseg[i].memsz+BIGPGSZ-1)/BIGPGSZ);
			s->color = up->color;
			incref(&img->r);
			s->image = img;
		}

		s->ldseg = ldseg[i];
		up->seg[sno++] = s;
		if(datalim < ldseg[i].pg0vaddr+ldseg[i].memsz)
			datalim = ldseg[i].pg0vaddr+ldseg[i].memsz;
	}

	/* BSS. Zero fill on demand for TS */
	s = newseg(SG_BSS|SG_READ|SG_WRITE, (datalim + BIGPGSZ-1) & ~(BIGPGSZ-1), 0);
	up->seg[sno++] = s;
	s->color= up->color;

	for(i = 0; i < sno; i++){
		s = up->seg[i];
		DBG(
			"execac %d %s(%c%c%c) %p:%p va %p off %p fsz %d msz %d\n",
			up->pid, segtypes[s->type & SG_TYPE],
			(s->type & SG_READ) != 0 ? 'r' : '-',
			(s->type & SG_WRITE) != 0 ? 'w' : '-',
			(s->type & SG_EXEC) != 0 ? 'x' : '-',
			s->base, s->top,
			s->ldseg.pg0vaddr+s->ldseg.pg0off,
			s->ldseg.pg0fileoff+s->ldseg.pg0off,
			s->ldseg.filesz,
			s->ldseg.memsz
		);
	}

	/* the color of the stack was decided when we created it before,
	 * it may have nothing to do with the color of other segments.
	 */
	qunlock(&up->seglock);
	poperror();				/* seglock */


	/*
	 *  '/' processes are higher priority
	 *		aki: why bother?
	 *
	 *	if(chan->dev->dc == L'/')
	 *		up->basepri = PriRoot;
	 */
	up->priority = up->basepri;

	poperror();				/* chan, elem, file */
	cclose(chan);
	free(file);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	mmuflush();
	if(up->prepagemem || flags == EXAC)
		nixprepage(-1);
	qlock(&up->debug);
	up->nnote = 0;
	up->notify = 0;
	up->notified = 0;
	up->privatemem = 0;
	sysprocsetup(up);
	qunlock(&up->debug);
	if(up->hang)
		up->procctl = Proc_stopme;

	/* we need to compte the value of &argv in user mode and then push that. */
	ar0->v = sysexecregs(entry, TSTKTOP - PTR2UINT(argv), ((void *)tos) + (USTKTOP-TSTKTOP)/sizeof(void *));

	if(flags == EXAC){
		up->procctl = Proc_toac;
		up->prepagemem = 1;
	}
}

void
sysexecac(Ar0* ar0, ...)
{
	int flags;
	char *file, **argv;
	va_list list;
	va_start(list, ar0);

	/*
	 * void* execac(int flags, char* name, char* argv[]);
	 */

	flags = va_arg(list, unsigned int);
	file = va_arg(list, char*);
	file = validaddr(file, 1, 0);
	argv = va_arg(list, char**);
	va_end(list);
	evenaddr(PTR2UINT(argv));
	execac(ar0, flags, file, argv);
}

void
sysexec(Ar0* ar0, ...)
{
	char *file, **argv;
	va_list list;

	va_start(list, ar0);

	/*
	 * void* exec(char* name, char* argv[]);
	 */
	file = va_arg(list, char*);
	file = validaddr(file, 1, 0);
	argv = va_arg(list, char**);
	va_end(list);
	evenaddr(PTR2UINT(argv));
	execac(ar0, EXTC, file, argv);
}

int
return0(void* v)
{
	return 0;
}

void
syssleep(Ar0* ar0, ...)
{
	Proc *up = externup();
	int64_t ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * int sleep(long millisecs);
	 */
	ms = va_arg(list, int64_t);
	va_end(list);

	ar0->i = 0;
	if(ms <= 0) {
		if (up->edf && (up->edf->flags & Admitted))
			edfyield();
		else
			yield();
		return;
	}
	if(ms < TK2MS(1))
		ms = TK2MS(1);
	tsleep(&up->sleep, return0, 0, ms);
}

void
sysalarm(Ar0* ar0, ...)
{
	uint64_t ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * long alarm(unsigned long millisecs);
	 * Odd argument type...
	 */
	ms = va_arg(list, uint64_t);
	va_end(list);

	ar0->vl = procalarm(ms);
}

void
sysexits(Ar0* ar0, ...)
{
	Proc *up = externup();
	char *status;
	char *inval = "invalid exit string";
	char buf[ERRMAX];
	va_list list;
	va_start(list, ar0);

	/*
	 * void exits(char *msg);
	 */
	status = va_arg(list, char*);
	va_end(list);

	if(status){
		if(waserror())
			status = inval;
		else{
			status = validaddr(status, 1, 0);
			if(vmemchr(status, 0, ERRMAX) == 0){
				memmove(buf, status, ERRMAX);
				buf[ERRMAX-1] = 0;
				status = buf;
			}
			poperror();
		}

	}
	pexit(status, 1);
}

void
sysawait(Ar0* ar0, ...)
{
	int i;
	int pid;
	Waitmsg w;
	usize n;
	char *p;
	va_list list;
	va_start(list, ar0);

	/*
	 * int await(char* s, int n);
	 * should really be
	 * usize await(char* s, usize n);
	 */
	p = va_arg(list, char*);
	n = va_arg(list, int32_t);
	va_end(list);
	p = validaddr(p, n, 1);

	pid = pwait(&w);
	if(pid < 0){
		ar0->i = -1;
		return;
	}
	i = snprint(p, n, "%d %lud %lud %lud %q",
		w.pid,
		w.time[TUser], w.time[TSys], w.time[TReal],
		w.msg);

	ar0->i = i;
}

void
werrstr(char *fmt, ...)
{
	Proc *up = externup();
	va_list va;

	if(up == nil)
		return;

	va_start(va, fmt);
	vseprint(up->syserrstr, up->syserrstr+ERRMAX, fmt, va);
	va_end(va);
}

static void
generrstr(char *buf, int32_t n)
{
	Proc *up = externup();
	char *p, tmp[ERRMAX];

	if(n <= 0)
		error(Ebadarg);
	p = validaddr(buf, n, 1);
	if(n > sizeof tmp)
		n = sizeof tmp;
	memmove(tmp, p, n);

	/* make sure it's NUL-terminated */
	tmp[n-1] = '\0';
	memmove(p, up->syserrstr, n);
	p[n-1] = '\0';
	memmove(up->syserrstr, tmp, n);
}

void
syserrstr(Ar0* ar0, ...)
{
	char *err;
	usize nerr;
	va_list list;
	va_start(list, ar0);

	/*
	 * int errstr(char* err, uint nerr);
	 * should really be
	 * usize errstr(char* err, usize nerr);
	 * but errstr always returns 0.
	 */
	err = va_arg(list, char*);
	nerr = va_arg(list, usize);
	va_end(list);
	generrstr(err, nerr);

	ar0->i = 0;
}

void
sysnotify(Ar0* ar0, ...)
{
	Proc *up = externup();
	void (*f)(void*, char*);
	va_list list;
	va_start(list, ar0);

	/*
	 * int notify(void (*f)(void*, char*));
	 */
	f = (void (*)(void*, char*))va_arg(list, void*);
	va_end(list);

	if(f != nil)
		validaddr(f, sizeof(void (*)(void*, char*)), 0);
	up->notify = f;

	ar0->i = 0;
}

void
sysnoted(Ar0* ar0, ...)
{
	Proc *up = externup();
	int v;
	va_list list;
	va_start(list, ar0);

	/*
	 * int noted(int v);
	 */
	v = va_arg(list, int);
	va_end(list);

	if(v != NRSTR && !up->notified)
		error(Egreg);

	ar0->i = 0;
}

void
sysrendezvous(Ar0* ar0, ...)
{
	Proc *up = externup();
	Proc *p, **l;
	uintptr_t tag, val;
	va_list list;
	va_start(list, ar0);

	/*
	 * void* rendezvous(void*, void*);
	 */
	tag = PTR2UINT(va_arg(list, void*));

	l = &REND(up->rgrp, tag);
	up->rendval = ~0;

	lock(&up->rgrp->r.l);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = PTR2UINT(va_arg(list, void*));

			while(p->mach != 0)
				;
			ready(p);
			unlock(&up->rgrp->r.l);

			ar0->v = UINT2PTR(val);
			return;
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	up->rendtag = tag;
	up->rendval = PTR2UINT(va_arg(list, void*));
	va_end(list);
	up->rendhash = *l;
	*l = up;
	up->state = Rendezvous;
	if(up->trace)
		proctrace(up, SLock, 0);
	unlock(&up->rgrp->r.l);

	sched();

	ar0->v = UINT2PTR(up->rendval);
}

/*
 * The implementation of semaphores is complicated by needing
 * to avoid rescheduling in syssemrelease, so that it is safe
 * to call from real-time processes.  This means syssemrelease
 * cannot acquire any qlocks, only spin locks.
 *
 * Semacquire and semrelease must both manipulate the semaphore
 * wait list.  Lock-free linked lists only exist in theory, not
 * in practice, so the wait list is protected by a spin lock.
 *
 * The semaphore value *addr is stored in user memory, so it
 * cannot be read or written while holding spin locks.
 *
 * Thus, we can access the list only when holding the lock, and
 * we can access the semaphore only when not holding the lock.
 * This makes things interesting.  Note that sleep's condition function
 * is called while holding two locks - r and up->rlock - so it cannot
 * access the semaphore value either.
 *
 * An acquirer announces its intention to try for the semaphore
 * by putting a Sema structure onto the wait list and then
 * setting Sema.waiting.  After one last check of semaphore,
 * the acquirer sleeps until Sema.waiting==0.  A releaser of n
 * must wake up n acquirers who have Sema.waiting set.  It does
 * this by clearing Sema.waiting and then calling wakeup.
 *
 * There are three interesting races here.

 * The first is that in this particular sleep/wakeup usage, a single
 * wakeup can rouse a process from two consecutive sleeps!
 * The ordering is:
 *
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) set Sema.waiting = 0
 * 	(a) check Sema.waiting inside sleep, return w/o sleeping
 * 	(a) try for semaphore, fail
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) call wakeup(a)
 * 	(a) wake up again
 *
 * This is okay - semacquire will just go around the loop
 * again.  It does mean that at the top of the for(;;) loop in
 * semacquire, phore.waiting might already be set to 1.
 *
 * The second is that a releaser might wake an acquirer who is
 * interrupted before he can acquire the lock.  Since
 * release(n) issues only n wakeup calls -- only n can be used
 * anyway -- if the interrupted process is not going to use his
 * wakeup call he must pass it on to another acquirer.
 *
 * The third race is similar to the second but more subtle.  An
 * acquirer sets waiting=1 and then does a final canacquire()
 * before going to sleep.  The opposite order would result in
 * missing wakeups that happen between canacquire and
 * waiting=1.  (In fact, the whole point of Sema.waiting is to
 * avoid missing wakeups between canacquire() and sleep().) But
 * there can be spurious wakeups between a successful
 * canacquire() and the following semdequeue().  This wakeup is
 * not useful to the acquirer, since he has already acquired
 * the semaphore.  Like in the previous case, though, the
 * acquirer must pass the wakeup call along.
 *
 * This is all rather subtle.  The code below has been verified
 * with the spin model /sys/src/9/port/semaphore.p.  The
 * original code anticipated the second race but not the first
 * or third, which were caught only with spin.  The first race
 * is mentioned in /sys/doc/sleep.ps, but I'd forgotten about it.
 * It was lucky that my abstract model of sleep/wakeup still managed
 * to preserve that behavior.
 *
 * I remain slightly concerned about memory coherence
 * outside of locks.  The spin model does not take
 * queued processor writes into account so we have to
 * think hard.  The only variables accessed outside locks
 * are the semaphore value itself and the boolean flag
 * Sema.waiting.  The value is only accessed with CAS,
 * whose job description includes doing the right thing as
 * far as memory coherence across processors.  That leaves
 * Sema.waiting.  To handle it, we call coherence() before each
 * read and after each write.		- rsc
 */

/* Add semaphore p with addr a to list in seg. */
static void
semqueue(Segment* s, int* addr, Sema* p)
{
	memset(p, 0, sizeof *p);
	p->addr = addr;

	lock(&s->sema.rend.l);	/* uses s->sema.Rendez.Lock, but no one else is */
	p->next = &s->sema;
	p->prev = s->sema.prev;
	p->next->prev = p;
	p->prev->next = p;
	unlock(&s->sema.rend.l);
}

/* Remove semaphore p from list in seg. */
static void
semdequeue(Segment* s, Sema* p)
{
	lock(&s->sema.rend.l);
	p->next->prev = p->prev;
	p->prev->next = p->next;
	unlock(&s->sema.rend.l);
}

/* Wake up n waiters with addr on list in seg. */
static void
semwakeup(Segment* s, int* addr, int n)
{
	Sema *p;

	lock(&s->sema.rend.l);
	for(p = s->sema.next; p != &s->sema && n > 0; p = p->next){
		if(p->addr == addr && p->waiting){
			p->waiting = 0;
			coherence();
			wakeup(&p->rend);
			n--;
		}
	}
	unlock(&s->sema.rend.l);
}

/* Add delta to semaphore and wake up waiters as appropriate. */
static int
semrelease(Segment* s, int* addr, int delta)
{
	int value;

	do
		value = *addr;
	while(!CASW(addr, value, value+delta));
	semwakeup(s, addr, delta);

	return value+delta;
}

/* Try to acquire semaphore using compare-and-swap */
static int
canacquire(int* addr)
{
	int value;

	while((value = *addr) > 0){
		if(CASW(addr, value, value-1))
			return 1;
	}

	return 0;
}

/* Should we wake up? */
static int
semawoke(void* p)
{
	coherence();
	return !((Sema*)p)->waiting;
}

/* Acquire semaphore (subtract 1). */
static int
semacquire(Segment* s, int* addr, int block)
{
	Proc *up = externup();
	int acquired;
	Sema phore;

	if(canacquire(addr))
		return 1;
	if(!block)
		return 0;

	acquired = 0;
	semqueue(s, addr, &phore);
	for(;;){
		phore.waiting = 1;
		coherence();
		if(canacquire(addr)){
			acquired = 1;
			break;
		}
		if(waserror())
			break;
		sleep(&phore.rend, semawoke, &phore);
		poperror();
	}
	semdequeue(s, &phore);
	coherence();	/* not strictly necessary due to lock in semdequeue */
	if(!phore.waiting)
		semwakeup(s, addr, 1);
	if(!acquired)
		nexterror();

	return 1;
}

/* Acquire semaphore or time-out */
static int
tsemacquire(Segment* s, int* addr, int64_t ms)
{
	Proc *up = externup();
	int acquired;
	uint64_t t;
	Sema phore;

	if(canacquire(addr))
		return 1;
	if(ms == 0)
		return 0;

	acquired = 0;
	semqueue(s, addr, &phore);
	for(;;){
		phore.waiting = 1;
		coherence();
		if(canacquire(addr)){
			acquired = 1;
			break;
		}
		if(waserror())
			break;
		t = sys->ticks;
		tsleep(&phore.rend, semawoke, &phore, ms);
		ms -= TK2MS(sys->ticks-t);
		poperror();
		if(ms <= 0)
			break;
	}
	semdequeue(s, &phore);
	coherence();	/* not strictly necessary due to lock in semdequeue */
	if(!phore.waiting)
		semwakeup(s, addr, 1);
	if(ms <= 0)
		return 0;
	if(!acquired)
		nexterror();
	return 1;
}

void
syssemacquire(Ar0* ar0, ...)
{
	Proc *up = externup();
	Segment *s;
	int *addr, block;
	va_list list;
	va_start(list, ar0);

	/*
	 * int semacquire(long* addr, int block);
	 * should be (and will be implemented below as) perhaps
	 * int semacquire(int* addr, int block);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	block = va_arg(list, int);
	va_end(list);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = semacquire(s, addr, block);
}

void
systsemacquire(Ar0* ar0, ...)
{
	Proc *up = externup();
	Segment *s;
	int *addr;
	uint64_t ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * int tsemacquire(long* addr, uint64_t ms);
	 * should be (and will be implemented below as) perhaps
	 * int tsemacquire(int* addr, uint64_t ms);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	ms = va_arg(list, uint64_t);
	va_end(list);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = tsemacquire(s, addr, ms);
}

void
syssemrelease(Ar0* ar0, ...)
{
	Proc *up = externup();
	Segment *s;
	int *addr, delta;
	va_list list;
	va_start(list, ar0);

	/*
	 * long semrelease(long* addr, long count);
	 * should be (and will be implemented below as) perhaps
	 * int semrelease(int* addr, int count);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	delta = va_arg(list, int);
	va_end(list);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(delta < 0 || *addr < 0)
		error(Ebadarg);

	ar0->i = semrelease(s, addr, delta);
}
