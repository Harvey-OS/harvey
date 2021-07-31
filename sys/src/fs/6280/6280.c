#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

ulong
meminit(void)
{
	long x, i, *l;

	x = 0x12345678;
	for(i=4; i<128; i+=4){
		l = (long*)(KSEG1|(i*1024L*1024L));
		if(probe(l, sizeof(long)))
			break;
		*l = x;
		wbflush();
		*(ulong*)KSEG1 = *(ulong*)KSEG1;	/* clear latches */
		if(*l != x)
			break;
		x += 0x3141526;
	}
	return i*1024*1024;
}

extern void _wback2loop(void);
extern void _inval2loop(void);
extern void _cachefloop(void);

void
vecinit(void)
{
	ulong *p, *q;
	int size;

	p = (ulong*)EXCEPTION;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;
	p = (ulong*)UTLBMISS;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;
	p = (ulong*)WBACK2LOOP;
	q = (ulong*)_wback2loop;
	for(size=0; size<64; size++)
		*p++ = *q++;
	p = (ulong*)INVAL2LOOP;
	q = (ulong*)_inval2loop;
	for(size=0; size<64; size++)
		*p++ = *q++;
	p = (ulong*)CACHEFLOOP;
	q = (ulong*)_cachefloop;
	for(size=0; size<64; size++)
		*p++ = *q++;
	cacheflush();
}

void
lockinit(void)
{
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

/*
 * should be called splhi
 * so there is no confusion
 * about cpu number
 */
void
lights(int n, int on)
{
	int l;

	l = m->lights;
	if(on)
		l |= (1<<n);
	else
		l &= ~(1<<n);
	m->lights = l;
	*LEDREG = ~l;
}

void
delay(int ms)
{
	int i, j;

	for(i=0; i<ms; i++)
		for(j=0; j<15000; j++)	/* experimentally indetermined, use SBC timer */
			;
}

void
clockinit(void)
{
	SBCREG->compare = SBCREG->count + (60*1000)*MS2HZ;
	intrclr(TIMERIE);
	SBCREG->intmask |= TIMERIE;
}

void
clockreload(ulong n)
{

	/*
	 * this will drift, we need to use the TOD clock
	 * to keep time accurate
	 */
	USED(n);
	SBCREG->compare = SBCREG->count + (60*1000)*MS2HZ;
}
