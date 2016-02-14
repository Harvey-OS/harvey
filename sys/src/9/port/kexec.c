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
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

#include <edf.h>
#include	<a.out.h>
#include "kexec.h"


/* XXX: MOVE ME TO K10 */

enum {
	Maxslot = 32,
};

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
	Proc *up = externup();
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

	p->scallnr = up->scallnr;
	memmove(p->arg, up->arg, sizeof(up->arg));
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	if(p->dot)
		incref(&p->dot->r);

	memmove(p->note, up->note, sizeof(p->note));
	p->nnote = up->nnote;
	p->notified = 0;
	p->lastnote = up->lastnote;
	p->notify = up->notify;
	p->ureg = 0;
	p->dbgreg = 0;

	kstrdup(&p->user, eve);
	if(kpgrp == 0)
		kpgrp = newpgrp();
	p->pgrp = kpgrp;
	incref(&kpgrp->r);

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
printhello(void)
{
	print("hello\n");
}

void
printargs(char *arg)
{
	print("%#p %s\n", arg, arg);
}
