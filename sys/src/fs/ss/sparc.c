#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

ulong	rom;		/* open boot rom vector */
ulong	romputcxsegm;

static int duartok;

static int
duartrecv(void)
{
	if(duartok)
		return sccgetc(0);
	if(getsysspaceb(SERIALPORT+4) & (1<<0))
		return getsysspaceb(SERIALPORT+6) & 0xFF;
	return 0;
}

static void
duartxmit(int c)
{
	if(duartok)
		sccputc(0, c);
	else{
		while((getsysspaceb(SERIALPORT+4) & (1<<2)) == 0)
			delay(4);
		putsysspaceb(SERIALPORT+6, c);
	}
}

void
consinit(void (*puts)(char*, int))
{
	KMap *k;

	k = kmappa(EIADUART, PTEIO|PTENOCACHE);
	sccsetup((void*)(k->va), EIAFREQ);
	consgetc = duartrecv;
	consputc = duartxmit;
	consputs = puts;
	sccspecial(0, kbdchar, conschar, 9600);
	duartok = 1;
}

void
consreset(void)
{
}

void
vecinit(void)
{
	trapinit();
}

void
lockinit(void)
{
}

typedef struct Ctr Ctr;
struct Ctr
{
	ulong	ctr0;
	ulong	lim0;
	ulong	ctr1;
	ulong	lim1;
};
Ctr	*ctr;

void
clockinit(void)
{
	KMap *k;

	k = kmappa(CLOCK, PTENOCACHE|PTEIO);
	ctr = (Ctr*)k->va;
	ctr->lim1 = (CLOCKFREQ/HZ)<<10;
}

void
clockreload(ulong n)
{
	int i;

	i = ctr->lim1;				/* clear interrupt */
	USED(i, n);
}

void
launchinit(void)
{
}

void
userinit(void (*f)(void), void *arg, char *text)
{
	User *p;

	p = newproc();

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (((ulong)init0) - 8);	/* 8 because of RETURN in gotolabel */
	p->sched.sp = (ulong)p->stack + sizeof(p->stack);
	p->sched.sp &= ~7;			/* SP must be 8-byte aligned */
	p->start = f;
	p->text = text;
	p->arg = arg;
	dofilter(&p->time);
	ready(p);
}

void
lights(int n, int on)
{
	USED(n);
	USED(on);
}

void
pokewcp(void)
{
}

void
delay(int ms)
{
	int i;

	ms *= mconf.usec_delay;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

void
firmware(void)
{
	delay(200);
	putenab(ENABRESET);
}

/*
 *  set up the lance
 */
void
lancesetup(Lance *lp)
{
	KMap *k;
	uchar *cp;
	ulong pa, va;
	int i;

	k = kmappa(ETHER, PTEIO|PTENOCACHE);
	lp->rdp = (void*)(k->va+0);
	lp->rap = (void*)(k->va+2);
	k = kmappa(EEPROM, PTEIO|PTENOCACHE);
	cp = (uchar*)(k->va+0x7da);
	for(i=0; i<6; i++)
		lp->ea[i] = *cp++;
	kunmap(k);

	lp->lognrrb = 7;
	lp->logntrb = 5;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->sep = 1;
	lp->busctl = BSWP | ACON | BCON;

	/*
	 * Allocate area for lance init block and descriptor rings
	 *
	 * Addresses handed to the lance must be 0xFFnnnnnn
	 * as the lance only has a 24-bit address.
	 */
	pa = (ulong)ialloc(BY2PG, BY2PG);

	/* map at LANCESEGM */
	k = kmappa(pa, PTEMAINMEM|PTENOCACHE);
	lp->lanceram = (ushort*)k->va;
	lp->lm = (Lanmem*)k->va;

	/*
	 * Allocate space in host memory for the io buffers.
	 */
	i = (lp->nrrb+lp->ntrb)*sizeof(Enpkt);
	i = (i+(BY2PG-1))/BY2PG;
	pa = (ulong)ialloc(i*BY2PG, BY2PG)&~KZERO;
	va = kmapregion(pa, i*BY2PG, PTEMAINMEM|PTENOCACHE);

	lp->lrp = (Enpkt*)va;
	lp->rp = (Enpkt*)va;
	lp->ltp = lp->lrp+lp->nrrb;
	lp->tp = lp->rp+lp->nrrb;
}

