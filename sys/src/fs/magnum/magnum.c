#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

void
duartinit(void)
{
	sccsetup(SCCADDR, SCCFREQ);
	sccspecial(1, kbdchar, conschar, 9600);
}

int
duartrecv(void)
{
	return sccgetc(1);
}

void
duartxmit(int c)
{
	sccputc(1, c);
}

void
duartreset(void)
{
}

void
vecinit(void)
{
	ulong *p, *q;
	int size, i;

	p = (ulong*)EXCEPTION;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;

	p = (ulong*)UTLBMISS;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;

	invalicache(0, 32*1024);

	for(i = 0; i < NTLB; i++)
		puttlbx(i, 0, 0);
}

enum {
	ZeroVal		= 0x12345678,
	ZeroInc		= 0x03141526,
};

ulong
meminit(void)
{
	long x, i;
	ulong *mirror, *zero, save0;
	ulong creg;

	/*
	 * put the memory system into a
	 * known state.
	 */
	creg = *CREG;
	*CREG = creg & ~EnableParity;
	DMA1->mode = Purgefifo|Clearerror;
	wbflush();

	/*
	 * set up for 4MB SIMMS, then look for a
	 * mirror at 8MB.
	 * save anything we value at physical 0,
	 * it's probably part of the utlbmiss handler.
	 */
	setsimmtype(4);
	zero = (ulong*)KSEG1;
	save0 = *zero;
	mirror = (ulong*)(KSEG1+8*1024*1024);
	x = ZeroVal+ZeroInc;
	*zero = ZeroVal;
	*mirror = x;
	wbflush();
	if((*mirror != x) || (*mirror == *zero))
		setsimmtype(1);

	/*
	 * size memory, looking at every 8MB.
	 * this should catch the one possible 1MB bank
	 * after one or more 4MB banks.
	 */
	for(i = 8; i < 128; i += 8){
		mirror = (ulong*)(KSEG1+(i*1024L*1024L));
		*mirror = x;
		*zero = ZeroVal;
		wbflush();
		if(*mirror != x || *mirror == *zero)
			break;
		x += ZeroInc;
		zero = mirror;
	}

	/*
	 * restore things and enable parity checking.
	 * we have to scrub memory to reset any parity errors
	 * first, the easiest way is to copy memory to
	 * itself.
	 * there may be a better way.
	 */
	*(ulong*)KSEG1 = save0;
	memmove((ulong*)KSEG1, (ulong*)KSEG0, i*1024*1024);
	wbflush();
	*CREG = creg|EnableParity;
	wbflush();
	DMA1->mode = Enable|Autoreload;
	wbflush();
	return i*1024*1024;
}

void
lockinit(void)
{
}

#define COUNT		((6250000L*MS2HZ)/(1000L))

void
clockinit(void)
{
	*TCOUNT = 0;
	*TBREAK = COUNT;
	wbflush();
}

void
clockreload(ulong n)
{
	ulong dt;

	USED(n);
	do {
		*TBREAK += COUNT;	/* just let it overflow and wrap around */
		wbflush();
		dt = *TBREAK - *TCOUNT;
	} while (dt == 0 || dt > COUNT);
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
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->stack + sizeof(p->stack);
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

	ms *= 7000;			/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

/*
 *  set up the lance
 */
void
lancesetup(Lance *lp)
{
	Nvram *nv = NVRAM;
	int i;
	ulong x;

	lp->rap = LANCERAP;
	lp->rdp = LANCERDP;

	/*
	 *  get lance address
	 */
	for(i = 0; i < 6; i++)
		lp->ea[i] = nv[i].val;

	/*
	 *  number of buffers
	 */
	lp->lognrrb = 7;
	lp->logntrb = 7;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->sep = 1;
	lp->busctl = BSWP;

	/*
	 * Allocate area for lance init block and descriptor rings.
	 * The rings must be quad-ushort aligned, asking for BY2PG
	 * guarantees that.
	 */
	x = (ulong)ialloc(4*1024, BY2PG);
	lp->lanceram = (ushort *)(x|UNCACHED);
	lp->lm = (Lanmem*)x;

	/*
	 * Allocate space in host memory for the io buffers.
	 * The Lance only has a 24-bit bus, so the space we use
	 * better be in the low 16MB of memory.
	 */
	x = (ulong)ialloc((lp->nrrb + lp->ntrb)*sizeof(Enpkt), 0);
	if((x + ((lp->nrrb + lp->ntrb)*sizeof(Enpkt))) > (KZERO+16*1024*1024))
		panic("lance: ialloc return > 16MB");

	lp->lrp = (Enpkt*)x;
	lp->ltp = lp->lrp + lp->nrrb;
	lp->rp = (Enpkt*)(x|UNCACHED);
	lp->tp = lp->lrp + lp->nrrb;
}
