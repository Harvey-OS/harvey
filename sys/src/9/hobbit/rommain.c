#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "init.h"

#include "machine.h"
#include "rom.h"

Conf conf;
Mach *m;
Mach *mach0;
User *u;

static PPage0 *ppage0 = (PPage0*)RAMBASE;

void
main(void)
{
	active.exiting = 0;
	active.machs = 1;
	machinit();
	confinit();
	xinit();
	printinit();

	/*
	 * Clear interrupt enables and
	 * enable I/O.
	 */
	*IOCTLADDR = 0;
	IRQADDR->mask0 = 0;
	IRQADDR->mask1 = 0;
	delay(10);
	*IOCTLADDR = IOENABLE|SCSIENABLE;

	eiasetup(0, EIAADDR);
	eiaspecial(0, &printq, &kbdq, 19200);
	mmuinit();
	pageinit();
	procinit0();
	initseg();
	clockinit();
	chandevreset();
	streaminit();
	swapinit();
	userinit();
	schedinit();
}

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->mmask = 1<<m->machno;
}

static void
meminit(void)
{
	ulong *stb, word, pgnum, segpa, va, x;

	va = (ulong)&ppage0->memsize+1024*1024;
	stb = (ulong*)KSTBADDR;
	x = 0x12345678;
	do{
		segpa = va & ~SEGOFFSET;
		pgnum = PGNUM(va);
		stb[SEGNUM(va)] = NPSTE(segpa, pgnum+1, STE_C|STE_U|STE_W|STE_V);
		mmuflushpte(va);
		word = *(ulong*)va;
		ppage0->memsize = x;
		if(word != *(ulong*)va)
			break;
		va += 1024*1024;
		x += 0x3141526;
	}while(1);
	if(pgnum == 0)
		stb[SEGNUM(va)] = 0;
	else
		stb[SEGNUM(va)] = NPSTE(segpa, pgnum, STE_C|STE_W|STE_V);
	mmuflushtlb();
	ppage0->memsize = va-(ulong)&ppage0->memsize;
}

void
confinit(void)
{
	ulong ktop;

	meminit();
	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.base0 = ktop;

	conf.npage0 = ((RAMBASE+4*1024*1024)-ktop)/BY2PG;

	conf.npage = (ppage0->memsize)/BY2PG;
	conf.upages = 200;

	conf.nmach = 1;
	conf.nproc = 20;
	conf.nimage = 20;
	conf.nswap = 0;

	conf.copymode = 0;			/* copy on write = 0 */

	conf.ipif = 2;
	conf.ip = 16;
	conf.arp = 8;
	conf.frag = 64;
}

void
init0(void)
{
	u->nerrlab = 0;
	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;
	spllo();
	
	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	touser();
}

void
userinit(void)
{
	Proc *p;
	Segment *s;
	User *up;
	KMap *k;
	Ureg *ur;
	Page *pg;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = smalloc(sizeof(Fgrp));
	p->fgrp->ref = 1;
	p->procmode = 0640;

	strcpy(p->text, "*init*");
	strcpy(p->user, eve);
	savefpregs(&initfp);
	p->fpstate = FPinit;

	/*
	 * User
	 */
	p->upage = newpage(1, 0, USERADDR|(p->pid&0xFFFF));
	k = kmap(p->upage);
	up = (User*)VA(k);
	up->p = p;
	ur = (Ureg*)(VA(k)+ISPOFFSET-0x20);
	ur->sp = USTKTOP-16;
	ur->msp = USTKTOP;
	ur->pc = UTZERO+0x20;
	ur->psw = PSW_VP|PSW_UL|PSW_IPL|PSW_X|PSW_S;
	kunmap(k);

	p->sched.psw = PSW_VP|PSW_UL|PSW_IPL|PSW_S;
	p->sched.pc = (ulong)init0;
	p->sched.sp = USERADDR+SPOFFSET-16;
	p->sched.msp = USERADDR+SPOFFSET;
	p->sched.isp = UREGADDR;

	/*
	 * User Stack
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BY2PG);
	segpage(s, pg);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	p->seg[TSEG] = s;
	segpage(s, newpage(1, 0, UTZERO));
	k = kmap(s->map[0]->pages[0]);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

void
exit(int ispanic)
{
	USED(ispanic);

	u = 0;
	lock(&active);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);
	spllo();
	while(active.machs || consactive())
		delay(100);
	softreset();
}
