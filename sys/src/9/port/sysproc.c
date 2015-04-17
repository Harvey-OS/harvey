/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
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
#include 	<trace.h>

/* this is ugly but we need libmach in the kernel. So this is a first pass.
 * FIX ME.
 */
/*
 *	Common a.out header describing all architectures
 */
typedef struct Fhdr
{
	char	*name;		/* identifier of executable */
	uint8_t	type;		/* file type - see codes above */
	uint8_t	hdrsz;		/* header size */
	uint8_t	_magic;		/* _MAGIC() magic */
	uint8_t	spare;
	int32_t	magic;		/* magic number */
	uint64_t	txtaddr;	/* text address */
	int64_t	txtoff;		/* start of text in file */
	uint64_t	dataddr;	/* start of data segment */
	int64_t	datoff;		/* offset to data seg in file */
	int64_t	symoff;		/* offset of symbol table in file */
	uint64_t	entry;		/* entry point */
	int64_t	sppcoff;	/* offset of sp-pc table in file */
	int64_t	lnpcoff;	/* offset of line number-pc table in file */
	int32_t	txtsz;		/* text size */
	int32_t	datsz;		/* size of data seg */
	int32_t	bsssz;		/* size of bss */
	int32_t	symsz;		/* size of symbol table */
	int32_t	sppcsz;		/* size of sp-pc table */
	int32_t	lnpcsz;		/* size of line number-pc table */
} Fhdr;


void
sysrfork(Ar0* ar0, ...)
{
	Mach *m = machp();
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
	if(flag & RFCORE && m->externup->wired != nil)
		error("wired proc cannot move to ac");		

	if((flag&RFPROC) == 0) {
		if(flag & (RFMEM|RFNOWAIT))
			error(Ebadarg);
		if(flag & (RFFDG|RFCFDG)) {
			ofg = m->externup->fgrp;
			if(flag & RFFDG)
				m->externup->fgrp = dupfgrp(ofg);
			else
				m->externup->fgrp = dupfgrp(nil);
			closefgrp(ofg);
		}
		if(flag & (RFNAMEG|RFCNAMEG)) {
			opg = m->externup->pgrp;
			m->externup->pgrp = newpgrp();
			if(flag & RFNAMEG)
				pgrpcpy(m->externup->pgrp, opg);
			/* inherit noattach */
			m->externup->pgrp->noattach = opg->noattach;
			closepgrp(opg);
		}
		if(flag & RFNOMNT)
			m->externup->pgrp->noattach = 1;
		if(flag & RFREND) {
			org = m->externup->rgrp;
			m->externup->rgrp = newrgrp();
			closergrp(org);
		}
		if(flag & (RFENVG|RFCENVG)) {
			oeg = m->externup->egrp;
			m->externup->egrp = smalloc(sizeof(Egrp));
			m->externup->egrp->ref = 1;
			if(flag & RFENVG)
				envcpy(m->externup->egrp, oeg);
			closeegrp(oeg);
		}
		if(flag & RFNOTEG)
			m->externup->noteid = incref(&noteidalloc);
		if(flag & (RFPREPAGE|RFCPREPAGE)){
			m->externup->prepagemem = flag&RFPREPAGE;
			nixprepage(-1);
		}
		if(flag & RFCORE){
			m->externup->ac = getac(m->externup, -1);
			m->externup->procctl = Proc_toac;
		}else if(flag & RFCCORE){
			if(m->externup->ac != nil)
				m->externup->procctl = Proc_totc;
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

	if(m->externup->trace)
		p->trace = 1;
	p->scallnr = m->externup->scallnr;
	memmove(p->arg, m->externup->arg, sizeof(m->externup->arg));
	p->nerrlab = 0;
	p->slash = m->externup->slash;
	p->dot = m->externup->dot;
	incref(p->dot);

	memmove(p->note, m->externup->note, sizeof(p->note));
	p->privatemem = m->externup->privatemem;
	p->noswap = m->externup->noswap;
	p->nnote = m->externup->nnote;
	p->notified = 0;
	p->lastnote = m->externup->lastnote;
	p->notify = m->externup->notify;
	p->ureg = m->externup->ureg;
	p->prepagemem = m->externup->prepagemem;
	p->dbgreg = 0;

	/* Make a new set of memory segments */
	n = flag & RFMEM;
	qlock(&p->seglock);
	if(waserror()){
		qunlock(&p->seglock);
		nexterror();
	}
	for(i = 0; i < NSEG; i++)
		if(m->externup->seg[i])
			p->seg[i] = dupseg(m->externup->seg, i, n);
	qunlock(&p->seglock);
	poperror();

	/* File descriptors */
	if(flag & (RFFDG|RFCFDG)) {
		if(flag & RFFDG)
			p->fgrp = dupfgrp(m->externup->fgrp);
		else
			p->fgrp = dupfgrp(nil);
	}
	else {
		p->fgrp = m->externup->fgrp;
		incref(p->fgrp);
	}

	/* Process groups */
	if(flag & (RFNAMEG|RFCNAMEG)) {
		p->pgrp = newpgrp();
		if(flag & RFNAMEG)
			pgrpcpy(p->pgrp, m->externup->pgrp);
		/* inherit noattach */
		p->pgrp->noattach = m->externup->pgrp->noattach;
	}
	else {
		p->pgrp = m->externup->pgrp;
		incref(p->pgrp);
	}
	if(flag & RFNOMNT)
		m->externup->pgrp->noattach = 1;

	if(flag & RFREND)
		p->rgrp = newrgrp();
	else {
		incref(m->externup->rgrp);
		p->rgrp = m->externup->rgrp;
	}

	/* Environment group */
	if(flag & (RFENVG|RFCENVG)) {
		p->egrp = smalloc(sizeof(Egrp));
		p->egrp->ref = 1;
		if(flag & RFENVG)
			envcpy(p->egrp, m->externup->egrp);
	}
	else {
		p->egrp = m->externup->egrp;
		incref(p->egrp);
	}
	p->hang = m->externup->hang;
	p->procmode = m->externup->procmode;

	/* Craft a return frame which will cause the child to pop out of
	 * the scheduler in user mode with the return register zero
	 */
	sysrforkchild(p, m->externup);

	p->parent = m->externup;
	p->parentpid = m->externup->pid;
	if(flag&RFNOWAIT)
		p->parentpid = 0;
	else {
		lock(&m->externup->exl);
		m->externup->nchild++;
		unlock(&m->externup->exl);
	}
	if((flag&RFNOTEG) == 0)
		p->noteid = m->externup->noteid;

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

	kstrdup(&p->text, m->externup->text);
	kstrdup(&p->user, m->externup->user);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	mmuflush();
	p->basepri = m->externup->basepri;
	p->priority = m->externup->basepri;
	p->fixedpri = m->externup->fixedpri;
	p->mp = m->externup->mp;

	wm = m->externup->wired;
	if(wm)
		procwired(p, wm->machno);
	p->color = m->externup->color;
	ready(p);
	sched();

	ar0->i = pid;
}

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

typedef struct {
	Exec;
	uint64_t hdr[1];
} Hdr;

/*
 * flags can ONLY specify that you want an AC for you, or
 * that you want an XC for you.
 * 
 */
static void
execac(Ar0* ar0, int flags, char *ufile, char **argv)
{
	Mach *m = machp();
	Hdr hdr;
	Fgrp *f;
	Tos *tos;
	Chan *chan, *ichan;
	Image *img;
	Segment *s;
	int argc, i, n;
	char *a, *elem, *file, *p;
	char line[sizeof(Exec)], *progarg[sizeof(Exec)/2+1];
	int32_t hdrsz, magic, textsz, datasz, bsssz;
	uintptr_t textlim, datalim, bsslim, entry, stack;
	//	static int colorgen;


	file = nil;
	elem = nil;
	switch(flags){
	case EXTC:
	case EXXC:
		break;
	case EXAC:
		m->externup->ac = getac(m->externup, -1);
		break;
	default:
		error("unknown execac flag");
	}
	if(waserror()){
		DBG("execac: failing: %s\n", m->externup->errstr);
		free(file);
		free(elem);
		if(flags == EXAC && m->externup->ac != nil)
			m->externup->ac->proc = nil;
		m->externup->ac = nil;
		nexterror();
	}

	/*
	 * Open the file, remembering the final element and the full name.
	 */
	argc = 0;
	file = validnamedup(ufile, 1);
	DBG("execac: up %#p file %s\n", m->externup, file);
	if(m->externup->trace)
		proctracepid(m->externup);
	ichan = namec(file, Aopen, OEXEC, 0);
	if(waserror()){
		cclose(ichan);
		nexterror();
	}
	kstrdup(&elem, m->externup->genbuf);

	/*
	 * Read the header.
	 * If it's a #!, fill in progarg[] with info then read a new header
	 * from the file indicated by the #!.
	 * The #! line must be less than sizeof(Exec) in size,
	 * including the terminating \n.
	 */
	hdrsz = ichan->dev->read(ichan, &hdr, sizeof(Hdr), 0);
	if(hdrsz < 2)
		error(Ebadexec);
	p = (char*)&hdr;
	if(p[0] == '#' && p[1] == '!'){
		p = memccpy(line, (char*)&hdr, '\n',
			    MIN(sizeof(Exec), hdrsz));
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
		chan = nil;	/* in case namec errors out */
		USED(chan);
		chan = namec(p, Aopen, OEXEC, 0);
		hdrsz = chan->dev->read(chan, &hdr, sizeof(Hdr), 0);
		if(hdrsz < 2)
			error(Ebadexec);
	}else{
		chan = ichan;
		incref(ichan);
	}

	/* chan is the chan to use, initial or not. ichan is irrelevant now */
	cclose(ichan);
	poperror();


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

	textlim = UTROUND(UTZERO+hdrsz+textsz);
	datalim = BIGPGROUND(textlim+datasz);
	bsslim = BIGPGROUND(textlim+datasz+bsssz);

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

	if(m->externup->ac != nil && m->externup->ac != m)
		m->externup->color = corecolor(m->externup->ac->machno);
	else
		m->externup->color = corecolor(m->machno);

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
	qlock(&m->externup->seglock);
	if(waserror()){
		if(m->externup->seg[ESEG] != nil){
			putseg(m->externup->seg[ESEG]);
			m->externup->seg[ESEG] = nil;
		}
		qunlock(&m->externup->seglock);
		nexterror();
	}
	m->externup->seg[ESEG] = newseg(SG_STACK, TSTKTOP-USTKSIZE, USTKSIZE/BIGPGSZ);
	m->externup->seg[ESEG]->color = m->externup->color;

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
	 * so m->externup->args can be created later.
	 * Although the argument vectors are being pushed onto the stack in
	 * the temporary segment, the values must be adjusted to reflect
	 * the segment address after it replaces the current SSEG.
	 */
	a = p = UINT2PTR(stack);
	stack = sysexecstack(stack, argc);
	if(stack-(argc+1)*sizeof(char**)-BIGPGSZ < TSTKTOP-USTKSIZE)
		error(Ebadexec);

	argv = (char**)stack;
	*--argv = nil;
	for(i = 0; i < argc; i++){
		*--argv = p + (USTKTOP-TSTKTOP);
		p += strlen(p) + 1;
	}

	/*
	 * Make a good faith copy of the args in m->externup->args using the strings
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
	free(m->externup->text);
	m->externup->text = elem;
	elem = nil;
	free(m->externup->args);
	m->externup->args = p;
	m->externup->nargs = n;
	poperror();				/* p (m->externup->args) */

	/*
	 * Close on exec
	 */
	f = m->externup->fgrp;
	for(i=0; i<=f->maxfd; i++)
		fdclose(i, CCEXEC);

	/*
	 * Free old memory.
	 * Special segments maintained across exec.
	 */
	for(i = SSEG; i <= HSEG; i++) {
		putseg(m->externup->seg[i]);
		m->externup->seg[i] = nil;		/* in case of error */
	}
	for(i = HSEG+1; i< NSEG; i++) {
		s = m->externup->seg[i];
		if(s && (s->type&SG_CEXEC)) {
			putseg(s);
			m->externup->seg[i] = nil;
		}
	}

	/* Text.  Shared. Attaches to cache image if possible
	 * but prepaged if EXAC
	 */
	img = attachimage(SG_TEXT|SG_RONLY, chan, m->externup->color, UTZERO, (textlim-UTZERO)/BIGPGSZ);
	s = img->s;
	m->externup->seg[TSEG] = s;
	s->flushme = 1;
	s->fstart = 0;
	s->flen = hdrsz+textsz;
 	if(img->color != m->externup->color){
 		m->externup->color = img->color;
 	}
	unlock(img);

	/* Data. Shared. */
	s = newseg(SG_DATA, textlim, (datalim-textlim)/BIGPGSZ);
	m->externup->seg[DSEG] = s;
	s->color = m->externup->color;

	/* Attached by hand */
	incref(img);
	s->image = img;
	s->fstart = hdrsz+textsz;
	s->flen = datasz;

	/* BSS. Zero fill on demand for TS */
	m->externup->seg[BSEG] = newseg(SG_BSS, datalim, (bsslim-datalim)/BIGPGSZ);
	m->externup->seg[BSEG]->color= m->externup->color;

	/*
	 * Move the stack
	 */
	s = m->externup->seg[ESEG];
	m->externup->seg[ESEG] = nil;
	m->externup->seg[SSEG] = s;
	/* the color of the stack was decided when we created it before,
	 * it may have nothing to do with the color of other segments.
	 */
	qunlock(&m->externup->seglock);
	poperror();				/* seglock */

	s->base = USTKTOP-USTKSIZE;
	s->top = USTKTOP;
	relocateseg(s, USTKTOP-TSTKTOP);

	/*
	 *  '/' processes are higher priority.
	 */
	if(chan->dev->dc == L'/')
		m->externup->basepri = PriRoot;
	m->externup->priority = m->externup->basepri;
	poperror();				/* chan, elem, file */
	cclose(chan);
	free(file);

	/*
	 *  At this point, the mmu contains info about the old address
	 *  space and needs to be flushed
	 */
	mmuflush();
	if(m->externup->prepagemem || flags == EXAC)
		nixprepage(-1);
	qlock(&m->externup->debug);
	m->externup->nnote = 0;
	m->externup->notify = 0;
	m->externup->notified = 0;
	m->externup->privatemem = 0;
	sysprocsetup(m->externup);
	qunlock(&m->externup->debug);
	if(m->externup->hang)
		m->externup->procctl = Proc_stopme;

	ar0->v = sysexecregs(entry, TSTKTOP - PTR2UINT(argv), argc);

	if(flags == EXAC){
		m->externup->procctl = Proc_toac;
		m->externup->prepagemem = 1;
	}

	DBG("execac up %#p done\n"
		"textsz %lx datasz %lx bsssz %lx hdrsz %lx\n"
		"textlim %ullx datalim %ullx bsslim %ullx\n", m->externup,
		textsz, datasz, bsssz, hdrsz, textlim, datalim, bsslim);
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

static void crackhdr(Ar0 *ar0, Chan *c, Fhdr *fp)
{
}

static void
machexec(Ar0* ar0, int flags, char *ufile, char **argv)
{
	Mach *m = machp();
	Fhdr f;
	// just catch the error and ignore it for now.
	if (waserror()) {
		return;
	}
	Chan *c = namec(m->externup->genbuf, Aopen, OREAD, 0);

	// call crackhdr
	crackhdr(ar0, c, &f);
	// ar0->i will be -1; leave until alvaro fills this in, just return,
	// and the regular a.out exec will take over.
	// Until this works, just set ar0->i to -1;
	// once this works, it replaces execac.
	// NOTE: does not need to have full functionality of execac;
	// just getting a process going on timesharing cores is ok for now.
	poperror();
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
	machexec(ar0, EXTC, file, argv);
	if (ar0->i == -1)
		execac(ar0, EXTC, file, argv);
}

void
sysr1(Ar0* ar, ...)
{
	print("sysr1() called. recompile your binary\n");
}

void
sysnixsyscall(Ar0* ar, ...)
{
	print("nixsyscall() called. recompile your binary\n");
}

int
return0(void* v)
{
	return 0;
}

void
syssleep(Ar0* ar0, ...)
{
	Mach *m = machp();
	int32_t ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * int sleep(long millisecs);
	 */
	ms = va_arg(list, int32_t);
	va_end(list);

	ar0->i = 0;
	if(ms <= 0) {
		if (m->externup->edf && (m->externup->edf->flags & Admitted))
			edfyield();
		else
			yield();
		return;
	}
	if(ms < TK2MS(1))
		ms = TK2MS(1);
	tsleep(&m->externup->sleep, return0, 0, ms);
}

void
sysalarm(Ar0* ar0, ...)
{
	unsigned long ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * long alarm(unsigned long millisecs);
	 * Odd argument type...
	 */
	ms = va_arg(list, unsigned long);
	va_end(list);

	ar0->l = procalarm(ms);
}

void
sysexits(Ar0* ar0, ...)
{
	Mach *m = machp();
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
sys_wait(Ar0* ar0, ...)
{
	int pid;
	Waitmsg w;
	OWaitmsg *ow;
	va_list list;
	va_start(list, ar0);

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
	va_end(list);

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
	p = validaddr(p, n, 1);
	va_end(list);

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
	Mach *m = machp();
	va_list va;

	if(m->externup == nil)
		return;

	va_start(va, fmt);
	vseprint(m->externup->syserrstr, m->externup->syserrstr+ERRMAX, fmt, va);
	va_end(va);
}

static void
generrstr(char *buf, int32_t n)
{
	Mach *m = machp();
	char *p, tmp[ERRMAX];

	if(n <= 0)
		error(Ebadarg);
	p = validaddr(buf, n, 1);
	if(n > sizeof tmp)
		n = sizeof tmp;
	memmove(tmp, p, n);

	/* make sure it's NUL-terminated */
	tmp[n-1] = '\0';
	memmove(p, m->externup->syserrstr, n);
	p[n-1] = '\0';
	memmove(m->externup->syserrstr, tmp, n);
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
	generrstr(err, nerr);
	va_end(list);

	ar0->i = 0;
}

void
sys_errstr(Ar0* ar0, ...)
{
	char *p;
	va_list list;
	va_start(list, ar0);

	/*
	 * int errstr(char* err);
	 *
	 * Deprecated; backwards compatibility only.
	 */
	p = va_arg(list, char*);
	generrstr(p, 64);
	va_end(list);

	ar0->i = 0;
}

void
sysnotify(Ar0* ar0, ...)
{
	Mach *m = machp();
	void (*f)(void*, char*);
	va_list list;
	va_start(list, ar0);

	/*
	 * int notify(void (*f)(void*, char*));
	 */
	f = (void (*)(void*, char*))va_arg(list, void*);

	if(f != nil)
		validaddr(f, sizeof(void (*)(void*, char*)), 0);
	m->externup->notify = f;
	va_end(list);

	ar0->i = 0;
}

void
sysnoted(Ar0* ar0, ...)
{
	Mach *m = machp();
	int v;
	va_list list;
	va_start(list, ar0);

	/*
	 * int noted(int v);
	 */
	v = va_arg(list, int);
	va_end(list);

	if(v != NRSTR && !m->externup->notified)
		error(Egreg);

	ar0->i = 0;
}

void
sysrendezvous(Ar0* ar0, ...)
{
	Mach *m = machp();
	Proc *p, **l;
	uintptr_t tag, val;
	va_list list;
	va_start(list, ar0);

	/*
	 * void* rendezvous(void*, void*);
	 */
	tag = PTR2UINT(va_arg(list, void*));
	va_end(list);

	l = &REND(m->externup->rgrp, tag);
	m->externup->rendval = ~0;

	lock(m->externup->rgrp);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = PTR2UINT(va_arg(list, void*));

			while(p->mach != 0)
				;
			ready(p);
			unlock(m->externup->rgrp);

			ar0->v = UINT2PTR(val);
			return;
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	m->externup->rendtag = tag;
	m->externup->rendval = PTR2UINT(va_arg(list, void*));
	m->externup->rendhash = *l;
	*l = m->externup;
	m->externup->state = Rendezvous;
	if(m->externup->trace)
		proctrace(m->externup, SLock, 0);
	unlock(m->externup->rgrp);

	sched();

	ar0->v = UINT2PTR(m->externup->rendval);
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
 * is called while holding two locks - r and m->externup->rlock - so it cannot
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
	Mach *m = machp();
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
tsemacquire(Segment* s, int* addr, int32_t ms)
{
	Mach *m = machp();
	int acquired;
	uint32_t t;
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
		tsleep(&phore, semawoke, &phore, ms);
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
	Mach *m = machp();
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

	if((s = seg(m->externup, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = semacquire(s, addr, block);
}

void
systsemacquire(Ar0* ar0, ...)
{
	Mach *m = machp();
	Segment *s;
	int *addr, ms;
	va_list list;
	va_start(list, ar0);

	/*
	 * int tsemacquire(long* addr, uint32_t ms);
	 * should be (and will be implemented below as) perhaps
	 * int tsemacquire(int* addr, uint32_t ms);
	 */
	addr = va_arg(list, int*);
	addr = validaddr(addr, sizeof(int), 1);
	evenaddr(PTR2UINT(addr));
	ms = va_arg(list, uint32_t);
	va_end(list);

	if((s = seg(m->externup, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(*addr < 0)
		error(Ebadarg);

	ar0->i = tsemacquire(s, addr, ms);
}

void
syssemrelease(Ar0* ar0, ...)
{
	Mach *m = machp();
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

	if((s = seg(m->externup, PTR2UINT(addr), 0)) == nil)
		error(Ebadarg);
	if(delta < 0 || *addr < 0)
		error(Ebadarg);

	ar0->i = semrelease(s, addr, delta);
}
