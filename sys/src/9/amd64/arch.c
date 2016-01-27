/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * EPISODE 12B
 * How to recognise different types of trees from quite a long way away.
 * NO. 1
 * THE LARCH
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>
#include "ureg.h"

/* the rules are different for different compilers. We need to define up. */
// Initialize it to force it into data.
// That way, if we set them in assembly, they won't get zero'd by the bss init in main
// N.B. There was an interesting hack in plan 9 c. You could grab up to two registers for your
// program. In the case of Plan 9, m was r15, and up was r14. Very slick, and if there is a way to do
// this in gcc or clang I don't know it. This also nicely handled per cpu info; R15/14 were always right for
// your core and context.
//Mach *m = (void *)0;

int
incref(Ref *r)
{
	int x;

	lock(&r->l);
	x = ++r->ref;
	unlock(&r->l);
	return x;
}

int
decref(Ref *r)
{
	int x;

	lock(&r->l);
	x = --r->ref;
	unlock(&r->l);
	if(x < 0)
		panic("decref pc=%#p", getcallerpc());

	return x;
}

void
procrestore(Proc *p)
{
	uint64_t t;

	if(p->kp)
		return;
	cycles(&t);
	p->pcycles -= t;

	fpuprocrestore(p);
}

/*
 *  Save the mach dependent part of the process state.
 *  NB: the caller should mmuflushtlb after procsave().
 *  procsave/procrestore don't touch the mmu, they
 *  care about fpu, mostly.
 */
void
procsave(Proc *p)
{
	uint64_t t;

	cycles(&t);
	p->pcycles += t;

	fpuprocsave(p);
}

static void
linkproc(void)
{
	Proc *up = externup();
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc dying", 0);
}

void
kprocchild(Proc* p, void (*func)(void*), void* arg)
{
	/*
	 * gotolabel() needs a word on the stack in
	 * which to place the return PC used to jump
	 * to linkproc().
	 */
	p->sched.pc = PTR2UINT(linkproc);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-BY2SE);
	p->sched.sp = STACKALIGN(p->sched.sp);

	p->kpfun = func;
	p->kparg = arg;
}

/*
 *  put the processor in the halt state if we've no processes to run.
 *  an interrupt will get us going again.
 *  The boot TC in nix can't halt, because it must stay alert in
 *  case an AC makes a handler process ready.
 *  We should probably use mwait in that case.
 */
void
idlehands(void)
{
	if(machp()->NIX.nixtype != NIXAC)
 		halt();
}

void
ureg2gdb(Ureg *u, uintptr_t *g)
{
	g[GDB_AX] = u->ax;
	g[GDB_BX] = u->bx;
	g[GDB_CX] = u->cx;
	g[GDB_DX] = u->dx;
	g[GDB_SI] = u->si;
	g[GDB_DI] = u->di;
	g[GDB_BP] = u->bp;
	g[GDB_SP] = u->sp;
	g[GDB_R8] = u->r8;
	g[GDB_R9] = u->r9;
	g[GDB_R10] = u->r10;
	g[GDB_R11] = u->r11;
	g[GDB_R12] = u->r12;
	g[GDB_R13] = u->r13;
	g[GDB_R14] = u->r14;
	g[GDB_R15] = u->r15;
	g[GDB_PC] = u->ip;

	/* it's weird, docs say 5 32-bit fields
	 * but I count 4 if we pack these. Fix me
	 */
	g[GDB_PS] = 0; // u->PS;
	g[GDB_CS] = 0; // u->CS;
	g[GDB_SS] = 0; // u->SS;
	g[GDB_DS] = 0; // u->DS;
	g[GDB_ES] = 0; // u->ES;
	g[GDB_FS] = 0; // u->FS;
	g[GDB_GS] = 0; // u->GS;
}

void
gdb2ureg(uintptr_t *g, Ureg *u)
{
	u->ax = g[GDB_AX];
	u->bx = g[GDB_BX];
	u->cx = g[GDB_CX];
	u->dx = g[GDB_DX];
	u->si = g[GDB_SI];
	u->di = g[GDB_DI];
	u->bp = g[GDB_BP];
	u->sp = g[GDB_SP];
	u->r8 = g[GDB_R8];
	u->r9 = g[GDB_R9];
	u->r10 = g[GDB_R10];
	u->r11 = g[GDB_R11];
	u->r12 = g[GDB_R12];
	u->r13 = g[GDB_R13];
	u->r14 = g[GDB_R14];
	u->r15 = g[GDB_R15];
	u->ip = g[GDB_PC];

	/* it's weird but gdb seems to have no way to
	 * express the sp. Hmm.
	 */
	u->flags = g[GDB_PS];
	/* is there any point to this? */
	u->cs = g[GDB_CS];
	u->ss = g[GDB_SS];
}
