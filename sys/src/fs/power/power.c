#include	"all.h"
#include 	"io.h"
#include 	"mem.h"

ulong
meminit(void)
{
	long x, i, *l;

	x = 0x12345678;
	for(i=4; i<128; i+=4){
		l = (long*)(KSEG1|(i*1024L*1024L));
/*		if(probe(l, sizeof(long)))
			break;				/**/
		*l = x;
		wbflush();
		*(ulong*)KSEG1 = *(ulong*)KSEG1;	/* clear latches */
		if(*l != x)
			break;
		x += 0x3141526;
	}
	return i*1024*1024;
}

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
	*LED = ~l;
}

void
nlights(int x)
{
	int i;

	for(i = 0; i < 8; i++) {
		lights(i, x&1);
		x = x>>1;
	}
}

void
delay(int ms)
{
	int i, j;

	for(i=0; i<ms; i++)
		for(j=0; j<7000; j++)	/* experimentally determined */
			;
}

/*
 * AMD 82C54 timer
 *
 * ctr2 is clocked at 3.6864 MHz.
 * ctr2 output clocks ctr0 and ctr1.
 * ctr0 drives INTR2.  ctr1 drives INTR4.
 * To get 100Hz, 36864==9*4096=36*1024 so clock ctr2 every 1024 and ctr0 every 36.
 */

struct	Timer
{
	uchar	cnt0,
		junk0[3];
	uchar	cnt1,
		junk1[3];
	uchar	cnt2,
		junk2[3];
	uchar	ctl,
		junk3[3];
};

#define	TIME0	((36*MS2HZ)/10)
#define	TIME1	0xFFFFFFFF	/* later, make this a profiling clock */
#define	TIME2	1024
#define	CTR(x)	((x)<<6)	/* which counter x */
#define	SET16	0x30		/* lsbyte then msbyte */
#define	MODE2	0x04		/* interval timer */

static
struct
{
	int	nfilter;
	Filter*	filters[100];
	int	time;
	int	duart;
} f;

void
clockinit(void)
{
	Timer *t;
	int i;

	t = TIMERREG;
	t->ctl = CTR(2)|SET16|MODE2;
	t->cnt2 = TIME2&0xFF;
	t->cnt2 = (TIME2>>8)&0xFF;
	t->ctl = CTR(1)|SET16|MODE2;
	t->cnt1 = TIME1&0xFF;
	t->cnt1 = (TIME1>>8)&0xFF;
	t->ctl = CTR(0)|SET16|MODE2;
	t->cnt0 = TIME0;
	t->cnt0 = (TIME0>>8)&0xFF;
	i = *CLRTIM0;
	USED(i);
	i = *CLRTIM1;
	USED(i);
	m->ticks = 0;
	f.duart = -1;
}

void
clockreload(ulong n)
{
	int i;

	if (n & INTR2) {
		i = *CLRTIM0;
		USED(i);
	}
	if (n & INTR4) {
		i = *CLRTIM1;
		USED(i);
	}
}

typedef struct Beef	Beef;
struct	Beef
{
	long	deadbeef;
	long	sum;
	long	cpuid;
	long	virid;
	long	erno;
	void	(*launch)(void);
	void	(*rend)(void);
	long	junk1[4];
	long	isize;
	long	dsize;
	long	nonbss;
	long	junk2[18];
};

void
launch(int n)
{
	Beef *p;
	long i, s;
	ulong *ptr;

	p = (Beef*) 0xb0000500 + n;
	p->launch = newstart;
	p->sum = 0;
	s = 0;
	ptr = (ulong*)p;
	for (i = 0; i < sizeof(Beef)/sizeof(ulong); i++)
		s += *ptr++;
	p->sum = -(s+1);

	for(i=0; i<5000; i++) {
		if(p->launch == 0)
			break;
		delay(1);
	}

}

int
nbits(ulong x)
{
	int n;

	for(n=0; x; x>>=1)
		if(x & 1)
			n++;
	return n;
}

void
tlbinit(void)
{
	int i;
	ulong phys;

	for(i=0; i<NTLB; i++)
		puttlbx(i, KZERO | PTEPID(i), 0);

	/* Map cyclones */
	phys = 0x30010000|PTEGLOBL|PTEVALID|PTEWRITE|PTEUNCACHED;
	for(i=0; i<4; i++) {
		puttlbx(i, KSEG2|(i*BY2PG), phys);
		phys += 16*1024*1024;
	}
}

void
launchinit(void)
{
	int i;

	tlbinit();

	probeflag = 0;
	for(i=1; i<conf.nmach; i++)
		launch(i);
	for(i=0; i<1000000; i++)
		if(active.machs == (1<<conf.nmach) - 1) {
			print("all %d launched\n", nbits(active.machs));
			return;
		}
	print("only %d launched (%.2x)\n", nbits(active.machs), active.machs);
}

void
online(void)
{
	machinit();
	lock(&active);
	active.machs |= 1<<m->machno;
	unlock(&active);
	tlbinit();
	clockinit();
	schedinit();
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
