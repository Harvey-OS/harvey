typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef u32int Mreg;				/* Msr - bloody UART */
typedef struct Page	Page;
typedef struct FPsave	FPsave;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef struct Softtlb	Softtlb;
typedef struct Sys	Sys;
typedef uvlong		Tval;
typedef struct Ureg	Ureg;

#pragma incomplete Ureg

#define MAXSYSARG	5	/* for mount(fd, mpt, flag, arg, srv) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	Q_MAGIC

/*
 * intc bits, as of 18 aug 2009.
 * specific to rae's virtex4 design
 * vanilla design defines Intuarttx.
 */
enum {
	Bitllfifo,
	Bittemac,
	Bitdma,
	Bitdma2,
	Bituart,
	Bitmiiphy,
	Bitqtmmacfail,			/* qtm only */
	Bitqtmdraminit,			/* qtm only */

	Intllfifo=1<<Bitllfifo,		/* local-link FIFO */
	Inttemac= 1<<Bittemac,
	Intdma	= 1<<Bitdma,
	Intdma2	= 1<<Bitdma2,
	Intuart = 1<<Bituart,
	Intmiiphy = 1<<Bitmiiphy,
	Intqtmmacfail= 1<<Bitqtmmacfail,
	Intqtmdraminit= 1<<Bitqtmdraminit,

	/* led bits */
	Ledtrap = 1<<0,		/* states: processing a trap */
	Ledkern = 1<<1,		/* running a kernel proc */
	Leduser = 1<<2,		/* running a user proc */
	Ledidle = 1<<3,		/* idling */
	Ledpanic = 1<<4,	/* panicing */
	Ledpulse = 1<<5,	/* flags: still alive */
	Ledethinwait = 1<<6,	/* ethernet i/o waits */
	Ledethoutwait = 1<<7,
};

#define LEDKERN

/*
 *  machine dependent definitions used by ../port/dat.h
 */

struct Lock
{
	ulong	key;
	ulong	sr;
	ulong	pc;
	Proc	*p;
	Mach	*m;
	ushort	isilock;
	ulong	magic;
};

struct Label
{
	ulong	sp;
	ulong	pc;
};

/*
 * emulated floating point
 */
struct FPsave
{
	union {
		double	fpscrd;
		struct {
			ulong	pad;
			ulong	fpscr;
		};
	};
	int	fpistate;
	ulong	emreg[32][3];
};

/*
 * FPsave.status
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,

	/* bit or'd with the state */
	FPillegal= 0x100,
};

struct Confmem
{
	ulong	base;
	ulong	npage;
	ulong	kbase;
	ulong	klimit;
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	Confmem	mem[4];
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	monitor;		/* has display? */
	ulong	ialloc;		/* bytes available for interrupt time allocation */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
};

/*
 *  mmu goo in the Proc structure
 */
#define NCOLOR 1
struct PMMU
{
	int	mmupid;
};

/* debugging */
#define TRIGGER()	*(int *)(PHYSSRAM + 0xf0) = 0

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */
/*
 * Fake kmap
 */
typedef	void		KMap;
#define	VA(k)		PTR2UINT(k)
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kmapinval()
#define	kunmap(k)

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor [0*4] */
	ulong	splpc;			/* pc that called splhi() [1*4] */
	Proc	*proc;			/* current process on this processor [2*4] */
	uintptr	pstlb;	/* physical address of Mach.stlb [3*4] */
	int	utlbhi;		/* lowest tlb index in use by kernel [4*4] */
	int	utlbnext;		/* next tlb entry to use for user (round robin) [5*4] */
	int	tlbfault;		/* number of tlb i/d misses [6*4] */

	/* ordering from here on irrelevant */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	inclockintr;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;	/* next forced context switch */

	Mach	*me;		/* debugging: should be my own address */

	long	oscclk;	/* oscillator frequency (MHz) */
	long	cpuhz;	/* general system clock (cycles) */
	long	clockgen;	/* clock generator frequency (cycles) */
	long	vcohz;
	long	pllhz;
	long	plbhz;
	long	opbhz;
	long	epbhz;
	long	pcihz;
	int	cputype;
	ulong	delayloop;
	uvlong	cyclefreq;	/* frequency of user readable cycle clock */

	Mach	*me2;		/* debugging: should be my own address */

	uvlong	fastclock;
	Perf	perf;			/* performance counters */

	int	tlbpurge;		/* ... */
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	flushmmu;		/* make current proc flush its mmu state */
	int	ilockdepth;

	int	lastpid;		/* last TLB pid allocated on this machine */
	QLock	stlblock;	/* prevent context switch during tlb update */
	Softtlb*	stlb;		/* software tlb cache (see also pstlb above) */
	Proc*	pidproc[NTLBPID];	/* which proc owns a given pid */

	ulong	spuriousintr;
	int	lastintr;

	ulong	magic;		/* debugging; also check for stack overflow */

	/* MUST BE LAST */
	int	stack[1];
};

struct Softtlb {
	u32int	hi;	/* tlb hi, except that low order 10 bits have (pid[8]<<2) */
	u32int		lo;
};

struct
{
	Lock;
	short	machs;
	short	exiting;
	short	ispanic;
	int	thunderbirdsarego; /* lets the added processors continue to schedinit */
}active;

/*
 *  a parsed plan9.ini line
 */
#define NISAOPT		8

struct ISAConf {
	char		*type;
	ulong	port;
	int	irq;
	ulong	dma;
	ulong	mem;
	ulong	size;
	ulong	freq;

	int	nopt;
	char	*opt[NISAOPT];
};

#define	MACHP(n)	((Mach *)((int)&mach0 + (n)*BY2PG))
extern Mach		mach0;

extern register Mach	*m;
extern register Proc	*up;

/*
 * Horrid. But the alternative is 'defined'.
 */
#ifdef _DBGC_
#define DBGFLG		(dbgflg[_DBGC_])
#else
#define DBGFLG		(0)
#endif /* _DBGC_ */

// #define DBG(...)	if(DBGFLG) dbgprint(__VA_ARGS__)

typedef struct {
	ulong	lasttm;		/* last mutation start in seconds */
	ulong	startticks;
	ulong	lastticks;
	ulong	count;
	ulong	totticks;

	ulong	period;		/* in seconds */
} Mutstats;
extern Mutstats mutstats;

char	dbgflg[256];
ulong	intrs1sec;			/* count interrupts in this second */
uintptr	memsz;
int	okprint;
int	securemem;
int	vflag;

#define dbgprint	print		/* for now */
