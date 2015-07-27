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
#include "kexec.h"


/* XXX: MOVE ME TO K10 */

enum {
	Maxslot = 32,
};

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
} Khdr;

enum {
	AsmNONE		= 0,
	AsmMEMORY	= 1,
	AsmRESERVED	= 2,
	AsmACPIRECLAIM	= 3,
	AsmACPINVS	= 4,

	AsmDEV		= 5,
};

Proc*
setupseg(int core)
{
	Mach *m = machp();
	Segment *s;
	uintptr_t  ka;
	Proc *p;
	static Pgrp *kpgrp;
	Segment *tseg;
	int sno;

	// XXX: we're going to need this for locality domains.
	USED(core);

	p = newproc();
	p->psstate = 0;
	p->procmode = 0640;
	p->kp = 1;
	p->noswap = 1;

	p->scallnr = m->externup->scallnr;
	memmove(p->arg, m->externup->arg, sizeof(m->externup->arg));
	p->nerrlab = 0;
	p->slash = m->externup->slash;
	p->dot = m->externup->dot;
	if(p->dot)
		incref(p->dot);

	memmove(p->note, m->externup->note, sizeof(p->note));
	p->nnote = m->externup->nnote;
	p->notified = 0;
	p->lastnote = m->externup->lastnote;
	p->notify = m->externup->notify;
	p->ureg = 0;
	p->dbgreg = 0;

	kstrdup(&p->user, eve);
	if(kpgrp == 0)
		kpgrp = newpgrp();
	p->pgrp = kpgrp;
	incref(kpgrp);

	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = sys->ticks;

	procpriority(p, PriKproc, 0);


	// XXX: kluge 4 pages of address space for this.
	// how will it expand up? gives us <50 kprocs as is.
	
	/*
	  * we create the color and core at allocation time, not execution.  This
	  *  is probably not the best idea but it's a start.
	  */

	sno = 0;

	// XXX: now that we are asmalloc we are no long proc. 
	/* Stack */
	ka = (uintptr_t)KADDR(asmalloc(0, BIGPGSZ, AsmMEMORY, 1));
	tseg = newseg(SG_STACK|SG_READ|SG_WRITE, ka, 1);
	tseg = p->seg[sno++];

	ka = (uintptr_t)KADDR(asmalloc(0, BIGPGSZ, AsmMEMORY, 1));
	s = newseg(SG_TEXT|SG_READ|SG_EXEC, ka, 1);
	p->seg[sno++] = s;
//	s->color = acpicorecolor(core);

	/* Data. Shared. */
	// XXX; Now that the address space is all funky how are we going to handle shared data segments?
	ka = (uintptr_t)KADDR(asmalloc(0, BIGPGSZ, AsmMEMORY, 2));
	s = newseg(SG_DATA|SG_READ|SG_WRITE, ka, 1);
	p->seg[sno++] = s;
	s->color = tseg->color;

	/* BSS. Uses asm from data map. */
	p->seg[sno++] = newseg(SG_BSS|SG_READ|SG_WRITE, ka+BIGPGSZ, 1);
	p->seg[sno++]->color= tseg->color;


	nixprepage(-1);
	
	return p;
}

void
kforkexecac(Proc *p, int core, char *ufile, char **argv)
{
	Mach *m = machp();
	Khdr hdr;
	Tos *tos;
	Chan *chan;
	int argc, i, n, sno;
	char *a, *elem, *file, *args;
	int32_t hdrsz, magic, textsz, datasz, bsssz;
	uintptr_t textlim, datalim, bsslim, entry, tbase, tsize, dbase, dsize, bbase, bsize, sbase, ssize, stack;
	Mach *mp;
	//	static Pgrp *kpgrp;
	
	DBG("kexec on core %d\n", core);
	// XXX: since this is kernel code we can't do attachimage,
	// we should be reading the file into kernel memory.
	// this only matters if we are using ufile.
	// YYY: look at dev reboot for help.

	file = nil;
	elem = nil;
	chan = nil;
	mp = nil;
	
	USED(chan);

	if(waserror()){
		DBG("kforkexecac: failing: %s\n", m->externup->errstr);
		if(file)
			free(file);
		if(elem)
			free(elem);
		if(chan)
			cclose(chan);
		if(core > 0 && mp != nil)
			mp->proc = nil;
		if(core != 0)
			p->ac = nil;
		nexterror();
	}

	if(core != 0)
		p->ac = getac(p, core);

	argc = 0;
	if(ufile != nil){
		panic("ufile not implemented yet");
		file = validnamedup(ufile, 1);
		DBG("kforkexecac: up %#p file %s\n", m->externup, file);
		chan = namec(file, Aopen, OEXEC, 0);
		kstrdup(&elem, m->externup->genbuf);
	
		hdrsz = chan->dev->read(chan, &hdr, sizeof(Khdr), 0);
		DBG("wrote ufile\n");

		if(hdrsz < 2)
			error(Ebadexec);
	}else{
		/* somebody already wrote in our text segment */
		for(sno = 0; sno < NSEG; sno++)
			if(p->seg[sno] != nil)
				if((p->seg[sno]->type & SG_EXEC) != 0)
					break;
		if(sno == NSEG)
			error("kforkexecac: no text segment!");
		hdr = *(Khdr*)p->seg[sno]->base;
		hdrsz = sizeof(Khdr);
	}

//	p = (char*)&hdr;
	magic = l2be(hdr.magic);
	DBG("badexec3\n");
	
	if(hdrsz != sizeof(Khdr) || magic != AOUT_MAGIC)
		error(Ebadexec);
	if(magic & HDR_MAGIC){
		entry = vl2be(hdr.hdr[0]);
		hdrsz = sizeof(Khdr);
	}
	else{
		entry = l2be(hdr.entry);
		hdrsz = sizeof(Exec);
	}

	textsz = l2be(hdr.text);
	datasz = l2be(hdr.data);
	bsssz = l2be(hdr.bss);

	panic("aki broke it before it even got working.");
/* TODO(aki): figure out what to do with this.
	tbase = p->seg[TSEG]->base;
	tsize = tbase - p->seg[TSEG]->top;
	dbase = p->seg[DSEG]->base;
	dsize = dbase - p->seg[DSEG]->top;
	bbase = p->seg[BSEG]->base;
	bsize = bbase - p->seg[BSEG]->top;
	sbase = p->seg[SSEG]->base;
	ssize = sbase - p->seg[SSEG]->top;
*/

	// XXX: we are no longer contiguous.
	textlim = ROUNDUP(hdrsz+textsz, BIGPGSZ);
	// XXX: we are going to be at least two pages here.
	datalim = BIGPGROUND(datasz);
	bsslim = BIGPGROUND(datalim+bsssz);

	// XXX: this is pretty fragile
	memmove((void*)dbase, (void*)(entry+textsz), datasz);
	DBG("writing data dbase %#p tbase %#p textsz %ld datasz %ld\n", dbase, tbase, textsz, datasz);
//	memmove((void*)dbase, (void*)"testing data", 13);
	/*
	 * Check the binary header for consistency,
	 * e.g. the entry point is within the text segment and
	 * the segments don't overlap each other.
	 */
	// XXX: max instruction size on amd64 is 15 bytes provide a check for consistency.
	DBG("kexec: entry %#p tbase %#p hdrsz %ld  textsz %ld\n", entry, tbase, hdrsz, textsz);	
	if(entry < tbase+hdrsz || entry >= tbase+hdrsz+textsz)
		error(Ebadexec);
	// XXX: what about the kernel stack we are making here?
	DBG("kexec: testing if sizes overflow limits\n");	
	if(textsz >= textlim || datasz > datalim || bsssz > bsslim)
		error(Ebadexec);
	DBG("kexec: do the top of the segments overflow limits?\n");	
	if(textlim >= tbase+tsize || datalim >= dbase+dsize || bsslim >= bbase+bsize)
		error(Ebadexec);

	DBG("kexec: is bss below data?\n");	
	if(bsslim < datalim)
		error(Ebadexec);
	/*
	Interesting thought, the previously allocated segments for
	data and text are shared and constant.  The BSS and the stack
	are not.  What you really want is the ability to make an
	executable text and data and then create child executables on
	top of that.  This will lower external fragmentation and allow
	a bunch of communicating shared memory processes (ie.  go) in
	kernel space.
	
	Fundamentally this means that the allocation of the text and
	the data should be separate from the bss and the stack.  This
	will require that you change the linkers as well to allow the
	separation of data and bss sections.
	*/

	/*
	 * Stack is a pointer into the temporary stack
	 * segment, and will move as items are pushed.
	 */
	 
	 // need to work something out here with the stack.
	stack = sbase+ssize-sizeof(Tos);


	 /*
	  * XXX: When we are linking this how do we set the tos? We will need to change trap right?
	  */
	tos = (Tos*)stack;
	tos->cyclefreq = m->cyclefreq;
	cycles((uint64_t*)&tos->pcycles);
	tos->pcycles = -tos->pcycles;
	tos->kcycles = tos->pcycles;
	tos->clock = 0;

	DBG("kexec: argument processing\n");
	if(0)
	for(i = 0;; i++, argv++){
		a = *(char**)validaddr(argv, sizeof(char**), 0);
		if(a == nil)
			break;
		a = validaddr(a, 1, 0);
		n = ((char*)vmemchr(a, 0, 0x7fffffff) - a) + 1;

		if(argc > 0 && i == 0)
			continue;

		stack -= n;
		if(stack < sbase+ssize-4096)
			error(Enovmem);
		args = UINT2PTR(stack);
		memmove(args, a, n);
		args[n-1] = 0;
		argc++;
	}
	// DBG("kexec: ensuring we have argc\n");
	if(0)
	if(argc < 1)
		error(Ebadexec);

	a = args = UINT2PTR(stack);
	stack = sysexecstack(stack, argc);
	// XXX: look through math on this. look at ../../9/port/ exec.c
	// YYY: this looks like a Jimism for 9k.
	// DBG("kexec: ensuring the stack \n");
	if(0)
	if(stack-(argc+1)*sizeof(char**)-BIGPGSZ < sbase+ssize-4096)
		error(Ebadexec);

	argv = (char**)stack;
	*--argv = nil;
	// XXX: replace USTKTOP with a new variable representing the top of stack.
	if(0)
	for(i = 0; i < argc; i++){
		*--argv = args + (USTKTOP-sbase+ssize);
		args += strlen(args) + 1;
	}

	DBG("argsing\n");
	n = args - a;
	if(0)
	if(n <= 0)
		error(Egreg);
	if(n > 128)
		n = 128;
	DBG("kexec: allocating args\n");
	// XXX: hangs in smalloc, not sure why.
//	args = smalloc(n);
//	if(waserror()){
//		DBG("erroring\n");
//		free(args);
//		nexterror();
//	}
//	DBG("kexec: moving args\n");
//	memmove(args, a, n);
//	if(0)
//	while(n > 0 && (args[n-1] & 0xc0) == 0x80)
//		n--;
//	args[n-1] = '\0';

	kstrdup(&p->text, "kexecproc");
	p->args = nil;
	//elem;
//	elem = nil;
//	p->args = args;
//	p->nargs = n;
	poperror();				/* p (m->externup->args) */





/*
	qlock(&p->debug);

	sysprocsetup(p);
	qunlock(&p->debug);
*/

	// why is this sched and not ureg?
	p->sched.pc = entry;
	// the real question here is how do you set up the stack?
	p->sched.sp = PTR2UINT(stack-BY2SE);
	p->sched.sp = STACKALIGN(p->sched.sp);


	// XXX: what does it imply if you have a kproc that runs on an ac?
	if(core > 0){
		DBG("kexec: coring %d\n", core);
		mp = p->ac;
		mp->icc->flushtlb = 1;
		mp->icc->rc = ICCOK;

		DBG("kexec: exotic proc on cpu%d\n", mp->machno);
		qlock(&p->debug);
		if(waserror()){
			DBG("kexec: had error");
			qunlock(&p->debug);
			nexterror();
		}
		p->nicc++;
		p->state = Exotic;
		p->psstate = 0;
		DBG("kexec: unlocking");
		qunlock(&p->debug);
		poperror();
		mfence();
		mp->icc->fn = (void*)entry;
		sched();
	}else{
		DBG("kexec: readying\n");
		ready(p);
		p->newtlb = 1;
		mmuflush();
	}
	DBG("kforkexecac up %#p done\n"
		"textsz %lx datasz %lx bsssz %lx hdrsz %lx\n"
		"textlim %ullx datalim %ullx bsslim %ullx\n", m->externup,
		textsz, datasz, bsssz, hdrsz, textlim, datalim, bsslim);
}

void
syskforkexecac(Ar0* ar0, ...)
{
//	int core;
//	uintptr base, size;
//	char *file, **argv;
	va_list list;
	va_start(list, ar0);
	//XXX: get system call working.
	USED(ar0); USED(list);

	// XXX: fix sysexecregs
	panic("syskforkexecac: don't call me yet");
	/*
	 * void* syskforkexecac(uintptr base, size, int core, char *ufile, char **argv)
	 */
//	base = va_arg(list, uintptr);
//	size = va_arg(list, uintptr);
//	core = va_arg(list, unsigned int);
//	file = va_arg(list, char*);
//	file = validaddr(file, 1, 0);
//	argv = va_arg(list, char**);
	va_end(list);
//	evenaddr(PTR2UINT(argv));
	// XXX: going to need to setup segs here.
	//kforkexecac(p, core, file, argv);
	// this is not going to work. I need to think about it. 
	// ar0->v = sysexecregs(entry, stack - PTR2UINT(argv), argc);
	
}


void
printhello(void)
{
	print("hello\n");
}

void
printargs(char *arg)
{
	print("%#p %s\n", arg, arg);
}
