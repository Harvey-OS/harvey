/*
 * implementation of rfork, exec, wait, exits, and
 * note and synchronisation system calls
 */
#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/edf.h"
#include	<a.out.h>

enum {
	Procarglen = 128,
};

void
sysr1(Ar0* ar0, va_list list)
{
	USED(list);
	ar0->i = 0;
}

static void
sysrforknoproc(int flag)
{
	Fgrp *ofg;
	Pgrp *opg;
	Rgrp *org;
	Egrp *oeg;

	if(flag & (RFMEM|RFNOWAIT))
		error(Ebadarg);
	if(flag & (RFFDG|RFCFDG)) {
		ofg = up->fgrp;
		up->fgrp = dupfgrp(flag & RFFDG? ofg: nil);
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
}

static void
initnewproc(Proc *p)
{
	p->trace = up->trace;
	p->scallnr = up->scallnr;
	memmove(p->arg.arg, up->arg.arg, sizeof(up->arg.arg));
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

	p->hang = up->hang;
	p->procmode = up->procmode;
	p->basepri = p->priority = up->basepri;
	p->fixedpri = up->fixedpri;

	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = sys->ticks;

	kstrdup(&p->text, up->text);
	kstrdup(&p->user, up->user);
}

void
sysrfork(Ar0* ar0, va_list list)
{
	Proc *p;
	int flag, i, n, pid;
	Mach *wm;

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
		sysrforknoproc(flag);
		ar0->i = 0;
		return;
	}

	p = newproc();
	initnewproc(p);

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
	if(flag & (RFFDG|RFCFDG))
		p->fgrp = dupfgrp(flag & RFFDG? up->fgrp: nil);
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
	} else {
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
	} else {
		p->egrp = up->egrp;
		incref(p->egrp);
	}

	/*
	 * Craft a return frame which will cause the child to pop out of
	 * the scheduler in user mode with the return register zero.
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

	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	pid = p->pid;
	mmuflush();
	p->mp = up->mp;
	wm = up->wired;
	if(wm)
		procwired(p, wm->machno);
	ready(p);
	sched();

	ar0->i = pid;
}

static uvlong
vl2be(uvlong v)
{
	uchar *p;

	p = (uchar*)&v;
	return ((uvlong)((p[0]<<24 | p[1]<<16 | p[2]<<8) | p[3])<<32)
	      | ((uvlong)(p[4]<<24 | p[5]<<16 | p[6]<<8) | p[7]);
}

static ulong
l2be(long l)
{
	uchar *cp;

	cp = (uchar*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}

/* data extracted (or deduced) from Exec header and exec() arg.s */
typedef struct Execdata Execdata;
struct Execdata {
	long	hdrsz;
	long	magic;
	long	textsz;
	long	datasz;
	long	bsssz;
	uintptr	textlim;
	uintptr	textmin;
	uintptr	datalim;
	uintptr	bsslim;
	uintptr	entry;

	uintptr	stack;
	int	argc;

	char	line[sizeof(Exec)];		/* tokenised into progarg */
	char	*progarg[sizeof(Exec)/2+1];
	char	elem[sizeof(up->genbuf)];
};

/* this is file layout, so can't tolerate bogus padding */
typedef struct Hdr Hdr;
struct Hdr {
	Exec;
	uvlong hdr[1];
};

static void
checkhdr(Execdata *ed, Hdr *hdrp)
{
	/*
	 * #! has had its chance, now we need a real binary.
	 */
	ed->magic = l2be(hdrp->magic);
	if(ed->hdrsz != sizeof(Hdr) || ed->magic != AOUT_MAGIC)
		error(Ebadexec); /* error("exec header too small or bad magic"); */
	if(ed->magic & HDR_MAGIC){
		ed->entry = vl2be(hdrp->hdr[0]);
		ed->hdrsz = sizeof(Hdr);
	} else {
		ed->entry = l2be(hdrp->entry);
		ed->hdrsz = sizeof(Exec);
	}

	ed->textsz = l2be(hdrp->text);
	ed->datasz = l2be(hdrp->data);
	ed->bsssz = l2be(hdrp->bss);

	ed->textmin = ROUNDUP(UTZERO+ed->hdrsz+ed->textsz, PGSZ);
	ed->textlim = UTROUND(ed->textmin);
	ed->datalim = ROUNDUP(ed->textlim+ed->datasz, PGSZ);
	ed->bsslim = ROUNDUP(ed->textlim+ed->datasz+ed->bsssz, PGSZ);

	/*
	 * Check the binary header for consistency,
	 * e.g. the entry point is within the text segment and
	 * the segments don't overlap each other.
	 */
	if(ed->entry < UTZERO+ed->hdrsz ||
	    ed->entry >= UTZERO+ed->hdrsz+ed->textsz)
		error(Ebadexec); /* error("exec header entry inconsistent"); */

	if(ed->textsz >= ed->textlim || ed->datasz > ed->datalim ||
	    ed->bsssz > ed->bsslim || ed->textlim >= USTKTOP ||
	    ed->datalim >= USTKTOP || ed->bsslim >= USTKTOP ||
	    ed->datalim < ed->textlim || ed->bsslim < ed->datalim)
		error(Ebadexec); /* error("exec header sizes inconsistent"); */
}

/*
 * The new stack is created in ESEG, temporarily mapped elsewhere.
 * The stack contains, in descending address order:
 *	a structure containing housekeeping and profiling data (Tos);
 *	argument strings;
 *	array of vectors to the argument strings with a terminating nil (argv).
 * When the exec is committed, this temporary stack in ESEG will become SSEG.
 * The architecture-dependent code which jumps to the new image will
 * also push a count of the argument array onto the stack (argc).
 *
 * Called with up->seglock held.
 */
static void
newstack(Execdata *ed)
{
	Tos *tos;

	up->seg[ESEG] = newseg(SG_STACK, TSTKTOP-USTKSIZE, TSTKTOP);
	coherence();

	/*
	 * Stack is a pointer into the temporary stack
	 * segment, and will move as items are pushed.
	 * First, the top-of-stack structure.
	 */
	ed->stack = TOS(TSTKTOP);
	tos = (Tos*)ed->stack;
	tos->cyclefreq = m->cpuhz;
	cycles((uvlong*)&tos->pcycles);
	tos->pcycles = -tos->pcycles;
	tos->kcycles = tos->pcycles;
	tos->clock = 0;
}

static int
stashprocargs(char *dst, char *src, int n, int nargs)
{
	n = MIN(n, Procarglen - nargs);
	if(n > 0) {
		memmove(dst, src, n);
		nargs += n;
	}
	return nargs;
}

void
dumpstk(void *stk)
{
	ulong *lp;

	for (lp = stk; lp < (ulong *)(up->kstack + KSTACK); lp++)
		iprint("%#p: %#lux\n", lp, *lp);
}

/*
 * Copy the strings pointed to by the syscall argument argv into
 * the temporary stack segment, being careful to check both argv and
 * the strings it points to are valid.
 */
static int
copyargs(Execdata *ed, char **argv, char *args, int nargs)
{
	int i, n;
	char *a, *p;

	evenaddr(PTR2UINT(argv));
	for(i = 0; ; i++, argv++){
		a = *(char**)validaddr(argv, sizeof(char**), 0);
		if(a == nil)
			break;
		a = validaddr(a, 1, 0);
		n = ((char*)vmemchr(a, 0, 0x7fffffff) - a) + 1;

		/*
		 * This futzing is so argv[0] gets validated even
		 * though it will be thrown away if this is a shell script.
		 */
		if(ed->argc > 0 && i == 0)
			continue;

		/*
		 * Before copying the string into the temporary stack,
		 * which might involve a demand-page, check the string
		 * will not overflow the bottom of the stack.
		 */
		ed->stack -= n;
		if(ed->stack < TSTKTOP-USTKSIZE)
			error(Enovmem);
		p = UINT2PTR(ed->stack);
		memmove(p, a, n);
		p[n-1] = '\0';
		ed->argc++;
		nargs = stashprocargs(&args[nargs], p, n, nargs);
	}
	if(ed->argc < 1)
		error(Ebadexec); /* error("exec header #! no args (2)"); */
	return nargs;
}

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
static char **
newargv(Execdata *ed)
{
	int i;
	char *p;
	char **argv;

	p = UINT2PTR(ed->stack);
	/*
	 * the second arg was ed->argc, followed by ed->argc+1 in next line.
	 * +2 is for a nil ptr & 64-bit alignment padding.
	 */
	ed->stack = sysexecstack(ed->stack, ed->argc + 2);
	if(ed->stack - (ed->argc+2)*sizeof(char**) - PGSZ < TSTKTOP-USTKSIZE)
		error(Ebadexec);	/* error("exec: stack too small"); */

	argv = (char**)ed->stack;
	*--argv = nil;
	for(i = 0; i < ed->argc; i++){
		*--argv = p + (USTKTOP-TSTKTOP);
		p += strlen(p) + 1;
	}
	return argv;
}

static char **
copystack(Execdata *ed, char *args, va_list *listp)
{
	int i, n, nargs;
	char **argv;

	nargs = 0;
	/*
	 * Push any arguments found from a #! header.
	 */
	for(i = 0; i < ed->argc; i++){
		n = strlen(ed->progarg[i])+1;
		ed->stack -= n;
		memmove(UINT2PTR(ed->stack), ed->progarg[i], n);
		nargs = stashprocargs(&args[nargs], ed->progarg[i], n, nargs);
	}

	/*
	 * Copy the strings pointed to by the syscall argument argv into
	 * the temporary stack segment, being careful to check both argv and
	 * the strings it points to are valid.
	 */
	argv = va_arg(*listp, char**);
	nargs = copyargs(ed, argv, args, nargs);

	/*
	 * construct a new argv vector on the new stack.
	 */
	argv = newargv(ed);

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
	kstrdup(&up->text, ed->elem);
	free(up->args);
	up->args = args;
	up->nargs = nargs;
	return argv;
}

static void
closeonexec(void)
{
	int i;
	Fgrp *f;

	f = up->fgrp;
	for(i=0; i<=f->maxfd; i++)
		fdclose(i, CCEXEC);
}

/*
 * Free old memory.
 * Special segments maintained across exec.
 */
static void
freeoldmem(void)
{
	int i;
	Segment *s;

	for(i = SSEG; i <= BSEG; i++) {
		putseg(up->seg[i]);
		up->seg[i] = nil;		/* in case of error */
	}
	for(i = BSEG+1; i< NSEG; i++) {
		s = up->seg[i];
		if(s && s->type & SG_CEXEC) {
			putseg(s);
			up->seg[i] = nil;
		}
	}
}

static void
adjnewsegs(Chan *chan, Execdata *ed)
{
	Image *img;
	Segment *s;

	/*
	 * Text.  Shared.  Attaches to cache image if possible.
	 * attachimage returns a locked cache image.
	 */
	img = attachimage(SG_TEXT|SG_RONLY, chan, UTZERO, ed->textmin);
	s = img->s;
	up->seg[TSEG] = s;
	s->flushme = 1;
	s->fstart = 0;
	s->flen = ed->hdrsz+ed->textsz;
	unlock(img);

	/* Data. Shared. */
	s = newseg(SG_DATA, ed->textlim, ed->datalim);
	up->seg[DSEG] = s;

	/* Attached by hand */
	incref(img);
	s->image = img;
	s->fstart = ed->hdrsz+ed->textsz;
	s->flen = ed->datasz;

	/* BSS. Zero fill on demand */
	up->seg[BSEG] = newseg(SG_BSS, ed->datalim, ed->bsslim);

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
}

static void
setpris(Chan *chan)
{
	/*
	 *  '/' processes are higher priority.
	 */
	if(chan->dev->dc == L'/')
		up->basepri = PriRoot;
	up->priority = up->basepri;
}

static void
cleanuser(void)
{
	mmuflush();
	qlock(&up->debug);
	up->nnote = up->notified = 0;
	up->notify = 0;
	up->privatemem = 0;
	sysprocsetup(up);
	qunlock(&up->debug);
	if(up->hang)
		up->procctl = Proc_stopme;
}

/* this shouldn't be so long, but use of waserror() complicates it. */
void
sysexec(Ar0* ar0, va_list list)
{
	char *args, *file;
	char **argv;
	char *p;
	Chan *chan;
	Execdata exhdrargs;
	Execdata *ed;
	Hdr hdr;
	static int beenhere;

	/*
	 * void* exec(char* name, char* argv[]);
	 */
	ed = &exhdrargs;
	memset(ed, 0, sizeof *ed);

	/*
	 * Remember the full name of the file, open it, and remember the
	 * final element of the name left in up->genbuf by namec.
	 */
	file = validnamedup(validaddr(va_arg(list, char*), 1, 0), 1);
	if(waserror()){
		free(file);
		nexterror();
	}
	chan = namec(file, Aopen, OEXEC, 0);
	if(waserror()){
		cclose(chan);
		nexterror();
	}
	strncpy(ed->elem, up->genbuf, sizeof(ed->elem));

	/*
	 * Read the header and populate hdr and *ed.
	 * If it's a #!, fill in ed->progarg[] with info then read a new header
	 * from the file indicated by the #!.
	 * The #! line must be shorter than sizeof(Exec),
	 * including the terminating \n.
	 */
	memset(&hdr, 0, sizeof hdr);
	p = (char*)&hdr;
	ed->hdrsz = chan->dev->read(chan, p, sizeof(Hdr), 0);
	if(ed->hdrsz < 2)
		error(Ebadexec);	/* error("exec header too small"); */
	ed->argc = 0;
	if(p[0] == '#' && p[1] == '!'){
		p = memccpy(ed->line, p, '\n', MIN(sizeof(Exec), ed->hdrsz));
		if(p == nil)
			error(Ebadexec); /* error("exec header #! no newline"); */
		*(p-1) = '\0';
		ed->argc = tokenize(ed->line+2, ed->progarg, nelem(ed->progarg));
		if(ed->argc == 0)
			error(Ebadexec); /* error("exec header #! no args"); */

		/* The original file becomes an extra arg after #! line */
		ed->progarg[ed->argc++] = file;

		/*
		 * Take the #! $0 as a file to open, and replace
		 * $0 with the original path's name.
		 */
		p = ed->progarg[0];
		ed->progarg[0] = ed->elem;
		poperror();			/* chan; replaced below */
		cclose(chan);

		/* open interpreter binary on replacement channel */
		chan = namec(p, Aopen, OEXEC, 0);
		if(waserror()){
			cclose(chan);
			nexterror();
		}
		ed->hdrsz = chan->dev->read(chan, &hdr, sizeof(Hdr), 0);
		if(ed->hdrsz < 2)
			error(Ebadexec); /* error("exec header too small"); */
	}
	checkhdr(ed, &hdr);

	qlock(&up->seglock);
	if(waserror()){
		if(up->seg[ESEG] != nil){
			putseg(up->seg[ESEG]);
			up->seg[ESEG] = nil;
		}
		qunlock(&up->seglock);
		nexterror();
	}
	newstack(ed);

	/*
	 * As the pass is made over the arguments and they are pushed onto
	 * the temporary stack, make a good faith copy in args for up->args. 
	 */
	args = smalloc(Procarglen);
	if(waserror()){
		free(args);
		nexterror();
	}
	argv = copystack(ed, args, &list);
	poperror();				/* args */

	closeonexec();
	freeoldmem();
	adjnewsegs(chan, ed);	/* unlocks seglock & pops error for it */
	setpris(chan);

	poperror();				/* chan */
	cclose(chan);
	poperror();				/* file */
	free(file);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	cleanuser();

	/*
	 * this return value (base of Tos on user stack) becomes the
	 * argument to _main in libc's main9.s.  ar0 typically points
	 * at the return register on the Ureg on the stack.
	 */
	ar0->v = sysexecregs(ed->entry, TSTKTOP - PTR2UINT(argv), ed->argc);
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
	va_end(list);
	if(status)
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
	va_end(list);
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
	int i, pid;
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
	va_end(list);
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
	va_end(list);
	if(f != nil)
		validaddr(f, sizeof(void (*)(void*, char*)), 0);
	up->notify = f;

	ar0->i = 0;
}

/* mostly a no-op here, but see noted() call in syscall() */
void
sysnoted(Ar0* ar0, va_list list)
{
	int v;

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
sysrendezvous(Ar0* ar0, va_list list)
{
	Proc *p, **l;
	uintptr tag, val;

	/*
	 * void* rendezvous(void*, void*);
	 */
	tag = PTR2UINT(va_arg(list, void*));

	l = &REND(up->rgrp, tag);
	up->rendval = ~0ull;

	lock(up->rgrp);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = PTR2UINT(va_arg(list, void*));
			va_end(list);
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
	va_end(list);
	up->rendhash = *l;
	*l = up;
	up->state = Rendezvous;
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

	do {
		value = *addr;
	} while(!CASW((uint *)addr, value, value+delta));
	semwakeup(s, addr, delta);

	return value+delta;
}

/* Try to acquire semaphore using compare-and-swap */
static int
canacquire(int* addr)
{
	int value;

	while((value = *addr) > 0)
		if(CASW((uint *)addr, value, value-1))
			return 1;
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
sysnsec(Ar0 *ar0, va_list)
{
	ar0->vl = todget(nil);
}
