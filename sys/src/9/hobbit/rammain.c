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

#define ppage0		((PPage0*)KZERO)

static char fs[Nvaluelen+1];
extern char station[NAMELEN];

struct {
	char	*name;
	char	*val;
} bootenv[] = {
	{ "sysname",	sysname, },
	{ "station",	station, },
	{ "fs",		fs, },
	0
};
static char *sp;
static char argbuf[128];

static char*
pusharg(char *p)
{
	int n;

	n = strlen(p)+1;
	sp -= n;
	memmove(sp, p, n);
	return sp;
}

static char*
bootenvget(char *name)
{
	int n, nentry;

	nentry = (ppage0->env.nentry < Nenv) ? ppage0->env.nentry: Nenv;
	for(n = 0; n < nentry; n++){
		if(strcmp(ppage0->env.entry[n].name, name) == 0)
			return ppage0->env.entry[n].value;
	}
	return 0;
}

static void
arginit(void)
{
	int i;
	char **av, *p;

	/*
	 * get boot env variables
	 */
	if(*sysname == 0){
		for(i = 0; bootenv[i].name; i++){
			if(p = bootenvget(bootenv[i].name)){
				strncpy(bootenv[i].val, p, NAMELEN);
				bootenv[i].val[NAMELEN-1] = '\0';
			}
		}
	}

	/*
	 *  pack args into buffer
	 */
	av = (char**)argbuf;
	sp = argbuf + sizeof(argbuf);
	for(i = 0; i < ppage0->argc; i++)
		av[i] = pusharg(ppage0->argv[i]);
	av[i++] = pusharg(fs);
	av[i] = 0;
}

void
main(void)
{
	active.exiting = 0;
	active.machs = 1;
	machinit();
	arginit();
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
	*IOCTLADDR = IOENABLE;

	eiasetup(0, EIAADDR);
	screeninit(0);

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

static ulong
meminit(void)
{
	ulong *stb, n, pa, pages;

	stb = (ulong*)KSTBADDR;
	pa = RAMBASE;
	for(pages = ppage0->memsize/BY2PG; pages; pages -= n){
		n = pages < (SEGSIZE/BY2PG) ? pages: (SEGSIZE/BY2PG);
		stb[SEGNUM((ulong)KADDR(pa))] = NPSTE(pa, n, STE_C|STE_W|STE_V);
		mmuflushpte((ulong)KADDR(pa));
		pa += SEGSIZE;
	}
	mmuflushtlb();
	return RAMBASE+ppage0->memsize;
}

void
confinit(void)
{
	int mul;
	ulong i, ktop;

	i = meminit();

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.base0 = ktop;
	conf.npage0 = (i-ktop)/BY2PG;

	conf.npage = conf.npage0;
	conf.upages = (conf.npage*70)/100;

	i = ppage0->memsize/(1024*1024);
	mul = (i+7)/8;
	conf.nmach = 1;
	conf.nproc = 20 + 50*mul;
	conf.nimage = 200;
	conf.nswap = conf.nproc*80;

	conf.copymode = 0;			/* copy on write = 0 */

	conf.ipif = 4;
	conf.ip = mul*64;
	conf.arp = 32;
	conf.frag = 128;
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

	if(!waserror()){
		ksetterm("hobbit %s");
		ksetenv("cputype", "hobbit");
		ksetenv("sysname", sysname);
		poperror();
	}

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
	char **av;
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
	ur->sp = USTKTOP-sizeof(argbuf);
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
	k = kmap(pg);
	for(av = (char**)argbuf; *av; av++)
		*av += (USTKTOP - sizeof(argbuf)) - (ulong)argbuf;
	memmove((uchar*)VA(k) + BY2PG - sizeof(argbuf), argbuf, sizeof argbuf);
	kunmap(k);

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
	print("cpu %d exiting\n", m->machno);
	while (active.machs || consactive())
		delay(100);
	softreset();
}
