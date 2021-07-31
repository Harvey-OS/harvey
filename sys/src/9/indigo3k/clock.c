#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	"ureg.h"

/*
 * 8254 programmable interval timer
 *
 * Counter2 is clocked at MASTER_FREQ (defined below).  The
 * output of counter2 is the clock for both counter0 and counter1.
 * Counter0 output is tied to INTR3; counter1 output is tied to
 * INTR4.
 */

#define MASTER_FREQ	1000000

typedef struct Pt_clock	Pt_clock;
struct Pt_clock
{
	uchar	fill0[3];
	uchar	c0;		/* counter 0 port */
	uchar	fill1[3];
	uchar	c1;		/* counter 1 port */
	uchar	fill2[3];
	uchar	c2;		/* counter 2 port */
	uchar	fill3[3];
	uchar	cw;		/* control word */
};

/*
 * Control words
 */
#define	PTCW_SC(x)	((x)<<6)	/* select counter x */
#define	PTCW_RBCMD	(3<<6)		/* read-back command */
#define	PTCW_CLCMD	(0<<4)		/* counter latch command */
#define	PTCW_LSB	(1<<4)		/* r/w least signif. byte only */
#define	PTCW_MSB	(2<<4)		/* r/w most signif. byte only */
#define	PTCW_16B	(3<<4)		/* r/w 16 bits, lsb then msb */
#define	PTCW_BCD	0x01		/* operate in BCD mode */

/*
 * Modes
 */
#define	MODE_ITC	(0<<1)		/* interrupt on terminal count */
#define	MODE_HROS	(1<<1)		/* hw retriggerable one-shot */
#define	MODE_RG		(2<<1)		/* rate generator */
#define	MODE_SQW	(3<<1)		/* square wave generator */
#define	MODE_STS	(4<<1)		/* software triggered strobe */
#define	MODE_HTS	(5<<1)		/* hardware triggered strobe */

void
delay(int ms)
{
	int i;

	ms *= 7000;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

void
Xdelay(int us)
{
	int i;

	wbflush();
	us *= 7;	/* experimentally determined */
	for(i=0; i<us; i++)
		;
}

/* #define	PROFILING /**/
#ifdef PROFILING
#undef TIME1
#define	TIME1	211		/* profiling clock; prime; about 10ms per tick */
#define	NPROF	50000
ulong	profcnt[MAXMACH*NPROF];
#endif

#define C2FREQ	(100000L)
#define C1FREQ	(200L)
#define C0FREQ	(HZ)

void
clockinit(void)
{
	Pt_clock *pt = IO(Pt_clock, PT_CLOCK);
	int s = splhi();

	pt->cw = PTCW_SC(2)|PTCW_16B|MODE_RG;
	pt->c2 = MASTER_FREQ/C2FREQ;
	Xdelay(4);
	pt->c2 = (MASTER_FREQ/C2FREQ)>>8;

	pt->cw = PTCW_SC(1)|PTCW_16B|MODE_RG;
	pt->c1 = C2FREQ/C1FREQ;
	Xdelay(4);
	pt->c1 = (C2FREQ/C1FREQ)>>8;

	pt->cw = PTCW_SC(0)|PTCW_16B|MODE_RG;
	pt->c0 = C2FREQ/C0FREQ;
	Xdelay(4);
	pt->c0 = (C2FREQ/C0FREQ)>>8;

	splx(s);
}

void
clock(Ureg *ur)
{
	Proc *p;
	int nrun = 0;
	static int netlight;

	m->ticks++;
	/*
	 *  network light
	 */
	if((m->ticks & 0xf) == 0){
		if(netlight == 0){
			if(m->nettime){
				netlight = 1;
				lightbits(0, 4);/**/
			}
		}else{
			if(m->ticks - m->nettime > HZ/3){
				lightbits(4, 0);/**/
				netlight = 0;
				m->nettime = 0;
			}
		}
	}

	/*
	 *  account for time
	 */
	p = m->proc;
	if(p){
		nrun = 1;
		p->pc = ur->pc;
		if (p->state==Running)
			p->time[p->insyscall]++;
	}
	nrun = (nrdy+nrun)*1000;
	MACHP(0)->load = (MACHP(0)->load*19+nrun)/20;
	checkalarms();
	mouseclock();
	sccclock();
	kproftimer(ur->pc);
	if(u && (ur->status&IEP) && p && p->state==Running){
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if(ur->status&KUP)
			*(ulong*)(USTKTOP-BY2WD) += TK2MS(1);
	}
}
