#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/edf.h"
#include	<a.out.h>
#include	<ptrace.h>

void
sysr1(Ar0* ar0, va_list list)
{
	USED(list);

	ar0->i = 0;
}

void
sysrfork(Ar0* ar0, va_list list)
{
	Proc *p;
	int flag, i, n, pid;
	Fgrp *ofg;
	Pgrp *opg;
	Rgrp *org;
	Egrp *oeg;
	Mach *wm;
	void (*pt)(Proc*, int, vlong, vlong);
	u64int ptarg;

	/*
	 * int rfork(int);
	 */
	flag = va_arg(list, int);

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

		ar0->i = 0;
		return;
	}

	p = newproc();

	p->trace = up->trace;
	p->scallnr = up->scallnr;
	memmove(p->arg, up->arg, sizeof(up->arg));
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	incref(p->dot);

	memmove(p->note, up->note, sizeof(p->note));
	p->privatemem = up->privatemem;
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
		if(up->seg[i] != nil)
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
		p->pgrp->noattach = 1;

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
	if(wm != nil)
		procwired(p, wm->machno);
	if(p->trace && (pt = proctrace) != nil){
		strncpy((char*)&ptarg, p->text, sizeof ptarg);
		pt(p, SName, 0, ptarg);
	}
	p->color = up->color;
	ready(p);
	sched();

	ar0->i = pid;
}

static uvlong
vl2be(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3])<<32)
	      |((uvlong)(p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7]);
}

ulong
l2be(long l)
{
	uchar *cp;

	cp = (uchar*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}

typedef struct {
	Exec;
	uvlong hdr[1];
} Hdr;

void
sysexec(Ar0* ar0, va_list list)
{
	Hdr hdr;
	Fgrp *f;
	Tos *tos;
	Chan *chan;
	Image *img;
	Segment *s;
	int argc, i, n, nargs;
	char *a, *args, **argv, elem[sizeof(up->genbuf)], *file, *p;
	char line[sizeof(Exec)], *progarg[sizeof(Exec)/2+1];
	long hdrsz, magic, textsz, datasz, bsssz;
	uintptr textlim, textmin, datalim, bsslim, entry, stack;
	void (*pt)(Proc*, int, vlong, vlong);
	u64int ptarg;

	/*
	 * void* exec(char* name, char* argv[]);
	 */

	/*
	 * Remember the full name of the file,
	 * open it, and remember the final element of the
	 * name left in up->genbuf by namec.
	 */
	p = va_arg(list, char*);
	p = validaddr(p, 1, 0);
	file = validnamedup(p, 1);
	if(waserror()){
		free(file);
		nexterror();
	}
	chan = namec(file, Aopen, OEXEC, 0);
	if(waserror()){
		cclose(chan);
		nexterror();
	}
	strncpy(elem, up->genbuf, sizeof(elem));

	/*
	 * Read the header.
	 * If it's a #!, fill in progarg[] with info then read a new header
	 * from the file indicated by the #!.
	 * The #! line must be less than sizeof(Exec) in size,
	 * including the terminating \n.
	 */
	hdrsz = chan->dev->read(chan, &hdr, sizeof(Hdr), 0);
	if(hdrsz < 2)
		error(Ebadexec);
	p = (char*)&hdr;
	argc = 0;
	if(p[0] == '#' && p[1] == '!'){
		p = memccpy(line, (char*)&hdr, '\n', MIN(sizeof(Exec), hdrsz));
		if(p == nil)
			error(Ebadexec);
		*(p-1) = '\0';
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
		poperror();			/* chan */
		cclose(chan);

		chan = namec(p, Aopen, OEXEC, 0);
		if(waserror()){
			cclose(chan);
			nexterror();
		}
		hdrsz = chan->dev->read(chan, &hdr, sizeof(Hdr), 0);
		if(hdrsz < 2)
			error(Ebadexec);
	}

	/*
	 * #! has had its chance, now we need a real binary.
	 */
	magic = l2be(hdr.magic);
	if(hdrsz != sizeof(Hdr) || magic != AOUT_MAGIC)
		error(Ebadexec);
	if(magic & HDR_MAGIC){
		entry = vl2be(hdr.hdr[0]);
		hdrsz = sizeof(Hdr);
	}
	else{
		entry = l2be(hdr.entry);
		hdrsz = sizeof(Exec);
	}

	textsz = l2be(hdr.text);
	datasz = l2be(hdr.data);
	bsssz = l2be(hdr.bss);

	textmin = ROUNDUP(UTZERO+hdrsz+textsz, PGSZ);
	textlim = UTROUND(textmin);
	datalim = ROUNDUP(textlim+datasz, PGSZ);
	bsslim = ROUNDUP(textlim+datasz+bsssz, PGSZ);

	/*
	 * Check the binary header for consistency,
	 * e.g. the entry point is within the text segment and
	 * the segments don't overlap each other.
	 */
	if(entry < UTZERO+hdrsz || entry >= UTZERO+hdrsz+textsz)
		error(Ebadexec);

	if(textsz >= textlim || datasz > datalim || bsssz > bsslim
	|| textlim >= USTKTOP || datalim >= USTKTOP || bsslim >= USTKTOP
	|| datalim < textlim || bsslim < datalim)
		error(Ebadexec);

	up->color = corecolor(m->machno);

	/*
	 * The new stack is created in ESEG, temporarily mapped elsewhere.
	 * The stack contains, in descending address order:
	 *	a structure containing housekeeping and profiling data (Tos);
	 *	argument strings;
	 *	array of vectors to the argument strings with a terminating
	 *	nil (argv).
	 * When the exec is committed, this temporary stack in ESEG will
	 * become SSEG.
	 * The architecture-dependent code which jumps to the new image
	 * will also push a count of the argument array onto the stack (argc).
	 */
	qlock(&up->seglock);
	if(waserror()){
		if(up->seg[ESEG] != nil){
			putseg(up->seg[ESEG]);
			up->seg[ESEG] = nil;
		}
		qunlock(&up->seglock);
		nexterror();
	}
	up->seg[ESEG] = newseg(SG_STACK, TSTKTOP-USTKSIZE, TSTKTOP);
	up->seg[ESEG]->color = up->color;

	/*
	 * Stack is a pointer into the temporary stack
	 * segment, and will move as items are pushed.
	 */
	stack = TSTKTOP-sizeof(Tos);

	/*
	 * First, the top-of-stack structure.
	 */
	tos = (Tos*)stack;
	tos->cyclefreq = m->cyclefreq;
	cycles((uvlong*)&tos->pcycles);
	tos->pcycles = -tos->pcycles;
	tos->kcycles = tos->pcycles;
	tos->clock = 0;

	/*
	 * As the pass is made over the arguments and they are pushed onto
	 * the temporary stack, make a good faith copy in args for up->args.
	 */
	args = smalloc(128);
	if(waserror()){
		free(args);
		nexterror();
	}
	nargs = 0;

	/*
	 * Next push any arguments found from a #! header.
	 */
	for(i = 0; i < argc; i++){
		n = strlen(progarg[i])+1;
		stack -= n;
		memmove(UINT2PTR(stack), progarg[i], n);

		if((n = MIN(n, 128-nargs)) <= 0)
			continue;
		memmove(&args[nargs], progarg[i], n);
		nargs += n;
	}

	/*
	 * Copy the strings pointed to by the syscall argument argv into
	 * the temporary stack segment, being careful to check both argv and
	 * the strings it points to are valid.
	 */
	argv = va_arg(list, char**);
	evenaddr(PTR2UINT(argv));
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

		if((n = MIN(n, 128-nargs)) <= 0)
			continue;
		memmove(&args[nargs], p, n);
		nargs += n;
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
	p = UINT2PTR(stack);
	stack = sysexecstack(stack, argc);
	if(stack-(argc+1)*sizeof(char**)-segpgsize(up->seg[ESEG]) < TSTKTOP-USTKSIZE)
		error(Ebadexec);

	argv = (char**)stack;
	*--argv = nil;
	for(i = 0; i < argc; i++){
		*--argv = p + (USTKTOP-TSTKTOP);
		p += strlen(p) + 1;
	}

	/*
	 * Fix up the up->args copy in args. The length must be > 0 as it
	 * includes the \0 on the last argument and argc was checked earlier
	 * to be > 0. Compensate for any UTF character boundary before
	 * placing the terminating \0.
	 */
	if(nargs <= 0)
		error(Egreg);

	while(nargs > 0 && (args[nargs-1] & 0xc0) == 0x80)
		nargs--;
	args[nargs-1] = '\0';

	/*
	 * All the argument processing is now done, ready to commit.
	 */
	kstrdup(&up->text, elem);
	free(up->args);
	up->args = args;
	up->nargs = nargs;
	poperror();				/* args */

	/*
	 * Close on exec
	 */
	f = up->fgrp;
	for(i=0; i<=f->maxfd; i++)
		fdclose(i, CCEXEC);

	/*
	 * Free old memory.
	 * Special segments maintained across exec.
	 */
	for(i = SSEG; i <= BSEG; i++) {
		putseg(up->seg[i]);
		up->seg[i] = nil;		/* in case of error */
	}
	for(i = BSEG+1; i< NSEG; i++) {
		s = up->seg[i];
		if(s && (s->type&SG_CEXEC)) {
			putseg(s);
			up->seg[i] = nil;
		}
	}

	if(up->trace && (pt = proctrace) != nil){
		strncpy((char*)&ptarg, elem, sizeof ptarg);
		pt(up, SName, 0, ptarg);
	}

	/* Text.  Shared. Attaches to cache image if possible */
	/* attachimage returns a locked cache image */

	img = attachimage(SG_TEXT|SG_RONLY, chan, up->color, UTZERO, textmin);
	s = img->s;
	up->seg[TSEG] = s;
	s->flushme = 1;
	s->fstart = 0;
	s->flen = hdrsz+textsz;
	if(img->color != up->color){
		up->color = img->color;
	}
	unlock(img);

	/* Data. Shared. */
	s = newseg(SG_DATA, textlim, datalim);
	up->seg[DSEG] = s;
	s->color = up->color;

	/* Attached by hand */
	incref(img);
	s->image = img;
	s->fstart = hdrsz+textsz;
	s->flen = datasz;

	/* BSS. Zero fill on demand */
	up->seg[BSEG] = newseg(SG_BSS, datalim, bsslim);
	up->seg[BSEG]->color= up->color;

	/*
	 * Move the stack
	 */
	s = up->seg[ESEG];
	up->seg[ESEG] = nil;
	up->seg[SSEG] = s;
	qunlock(&up->seglock);
	poperror();				/* seglock */

	s->base = USTKTOP-USTKSIZE;
	s->top = USTKTOP;
	relocateseg(s, USTKTOP-TSTKTOP);

	/*
	 *  '/' processes are higher priority.
	 */
	if(chan->dev->dc == L'/')
		up->basepri = PriRoot;
	up->priority = up->basepri;
	poperror();				/* chan */
	cclose(chan);
	poperror();				/* file */
	free(file);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	mmuflush();
	qlock(&up->debug);
	up->nnote = 0;
	up->notify = 0;
	up->notified = 0;
	up->privatemem = 0;
	sysprocsetup(up);
	qunlock(&up->debug);
	if(up->hang)
		up->procctl = Proc_stopme;

	ar0->v = sysexecregs(entry, TSTKTOP - PTR2UINT(argv), argc);
}

int
return0(void*)
{
	return 0;
}

void
syssleep(Ar0* ar0, va_list list)
{
	long ms;

	/*
	 * int sleep(long millisecs);
	 */
	ms = va_arg(list, long);

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
sysalarm(Ar0* ar0, va_list list)
{
	unsigned long ms;

	/*
	 * long alarm(unsigned long millisecs);
	 * Odd argument type...
	 */
	ms = va_arg(list, unsigned long);

	ar0->l = procalarm(ms);
}

void
sysexits(Ar0*, va_list list)
{
	char *status;
	char *inval = "invalid exit string";
	char buf[ERRMAX];

	/*
	 * void exits(char *msg);
	 */
	status = va_arg(list, char*);

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
sys_wait(Ar0* ar0, va_list list)
{
	int pid;
	Waitmsg w;
	OWaitmsg *ow;

	/*
	 * int wait(Waitmsg* w);
	 *
	 * Deprecated; backwards compatibility only.
	 */
	ow = va_arg(list, OWaitmsg*);
	if(ow == nil){
		ar0->i = pwait(nil);
		return;
	}

	ow = validaddr(ow, sizeof(OWaitmsg), 1);
	evenaddr(PTR2UINT(ow));
	pid = pwait(&w);
	if(pid >= 0){
		readnum(0, ow->pid, NUMSIZE, w.pid, NUMSIZE);
		readnum(0, ow->time+TUser*NUMSIZE, NUMSIZE, w.time[TUser], NUMSIZE);
		readnum(0, ow->time+TSys*NUMSIZE, NUMSIZE, w.time[TSys], NUMSIZE);
		readnum(0, ow->time+TReal*NUMSIZE, NUMSIZE, w.time[TReal], NUMSIZE);
		strncpy(ow->msg, w.msg, sizeof(ow->msg));
		ow->msg[sizeof(ow->msg)-1] = '\0';
	}

	ar0->i = pid;
}

void
sysawait(Ar0* ar0, va_list list)
{
	int i;
	int pid;
	Waitmsg w;
	usize n;
	char *p;

	/*
	 * int await(char* s, int n);
	 * should really be
	 * usize await(char* s, usize n);
	 */
	p = va_arg(list, char*);
	n = va_arg(list, long);
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
	va_list va;

	if(up == nil)
		return;

	va_start(va, fmt);
	vseprint(up->syserrstr, up->syserrstr+ERRMAX, fmt, va);
	va_end(va);
}

static void
generrstr(char *buf, long n)
{
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
syserrstr(Ar0* ar0, va_list list)
{
	char *err;
	usize nerr;

	/*
	 * int errstr(char* err, uint nerr);
	 * should really be
	 * usize errstr(char* err, usize nerr);
	 * but errstr always returns 0.
	 */
	err = va_arg(list, char*);
	nerr = va_arg(list, usize);
	generrstr(err, nerr);

	ar0->i = 0;
}

void
sys_errstr(Ar0* ar0, va_list list)
{
	char *p;

	/*
	 * int errstr(char* err);
	 *
	 * Deprecated; backwards compatibility only.
	 */
	p = va_arg(list, char*);
	generrstr(p, 64);

	ar0->i = 0;
}

void
sysnotify(Ar0* ar0, va_list list)
{
	void (*f)(void*, char*);

	/*
	 * int notify(void (*f)(void*, char*));
	 */
	f = (void (*)(void*, char*))va_arg(list, void*);

	if(f != nil)
		validaddr(f, sizeof(void (*)(void*, char*)), 0);
	up->notify = f;

	ar0->i = 0;
}

void
sysnoted(Ar0* ar0, va_list list)
{
	int v;

	/*
	 * int noted(int v);
	 */
	v = va_arg(list, int);

	if(v != NRSTR && !up->notified)
		error(Egreg);

	ar0->i = 0;
}

void
sysrendezvous(Ar0* ar0, va_list list)
{
	Proc *p, **l;
	uintptr tag, val, pc;
	void (*pt)(Proc*, int, vlong, vlong);

	/*
	 * void* rendezvous(void*, void*);
	 */
	tag = PTR2UINT(va_arg(list, void*));

	l = &REND(up->rgrp, tag);
	up->rendval = ~0;

	lock(up->rgrp);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = PTR2UINT(va_arg(list, void*));

			while(p->mach != 0)
				;
			ready(p);
			unlock(up->rgrp);

			ar0->v = UINT2PTR(val);
			return;
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	up->rendtag = tag;
	up->rendval = PTR2UINT(va_arg(list, void*));
	up->rendhash = *l;
	*l = up;
	up->state = Rendezvous;
	if(up->trace && (pt = proctrace) != nil){
		pc = (uintptr)sysrendezvous;
		pt(up, SSleep, 0, Rendezvous|(pc<<8));
	}
	unlock(up->rgrp);

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

	lock(&s->sema);	/* uses s->sema.Rendez.Lock, but no one else is */
	p->next = &s->sema;
	p->prev = s->sema.prev;
	p->next->prev = p;
	p->prev->next = p;
	unlock(&s->sema);
}

/* Remove semaphore p from list in seg. */
static void
semdequeue(Segment* s, Sema* p)
{
	lock(&s->sema);
	p->next->prev = p->prev;
	p->prev->next = p->next;
	unlock(&s->sema);
}

/* Wake up n waiters with addr on list in seg. */
static void
semwakeup(Segment* s, int* addr, int n)
{
	Sema *p;

	lock(&s->sema);
	for(p = s->sema.next; p != &s->sema && n > 0; p = p->next){
		if(p->addr == addr && p->waiting){
			p->waiting = 0;
			coherence();
			wakeup(p);
			n--;
		}
	}
	unlock(&s->sema);
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
		sleep(&phore, semawoke, &phore);
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
tsemacquire(Segment* s, int* addr, long ms)
{
	int acquired;
	ulong t;
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
		t = m->ticks;
		tsleep(&phore, semawoke, &phore, ms);
		ms -= TK2MS(m->ticks-t);
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
syssemacquire(Ar0* ar0, va_list list)
{
	Segment *s;
	int *addr, block;

	/*
	 * int semacquire(long* addr, int block);
	 * should be (and will be implemented below as) perhaps
	 * int semacquire(int* addr, int block);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	block = va_arg(list, int);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = semacquire(s, addr, block);
}

void
systsemacquire(Ar0* ar0, va_list list)
{
	Segment *s;
	int *addr, ms;

	/*
	 * int tsemacquire(long* addr, ulong ms);
	 * should be (and will be implemented below as) perhaps
	 * int tsemacquire(int* addr, ulong ms);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	ms = va_arg(list, ulong);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = tsemacquire(s, addr, ms);
}

void
syssemrelease(Ar0* ar0, va_list list)
{
	Segment *s;
	int *addr, delta;

	/*
	 * long semrelease(long* addr, long count);
	 * should be (and will be implemented below as) perhaps
	 * int semrelease(int* addr, int count);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	delta = va_arg(list, int);

	if((s = seg(up, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(delta < 0 || *addr < 0)
		error(Ebadarg);

	ar0->i = semrelease(s, addr, delta);
}

void
sysnsec(Ar0* ar0, va_list)
{
	ar0->vl = todget(nil);
}
