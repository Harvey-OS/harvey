typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct Notsave	Notsave;
typedef struct Page	Page;
typedef struct PCArch	PCArch;
typedef struct Pcidev	Pcidev;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef struct Sys	Sys;
typedef struct Ureg	Ureg;
typedef struct Vctl	Vctl;
typedef long		Tval;

#pragma incomplete Ureg

#define MAXSYSARG	5	/* for mount(fd, mpt, flag, arg, srv) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	Q_MAGIC

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
};

struct Label
{
	ulong	sp;
	ulong	pc;
};

/*
 * Proc.fpstate
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
};

/*
 * This structure must agree with fpsave and fprestore asm routines
 */
struct	FPsave
{
	double	fpreg[32];
	union {
		double	fpscrd;
		struct {
			ulong	pad;
			ulong	fpscr;
		};
	};
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
	Confmem	mem[1];
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

/*
 *  things saved in the Proc structure during a notify
 */
struct Notsave
{
	ulong	UNUSED;
};

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */
/*
 * Fake kmap
 */
typedef	void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kunmap(k)

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc that called splhi() */
	Proc	*proc;			/* current process on this processor */

	/* ordering from here on irrelevant */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	inclockintr;
	int	cputype;
	ulong	loopconst;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;	/* next forced context switch */

	vlong	cpuhz;
	ulong	bushz;
	ulong	dechz;
	ulong	tbhz;
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	ulong	pcclast;
	uvlong	fastclock;
	Perf	perf;			/* performance counters */

	int	tlbfault;		/* only used by devproc; no access to tlb */
	int	tlbpurge;		/* ... */
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	flushmmu;		/* make current proc flush it's mmu state */
	int	ilockdepth;

	ulong	ptabbase;		/* start of page table in kernel virtual space */
	int		slotgen;		/* next pte (byte offset) when pteg is full */
	int		mmupid;		/* next mmu pid to use */
	int		sweepcolor;
	int		trigcolor;
	Rendez	sweepr;

	ulong	spuriousintr;
	int	lastintr;

	/* MUST BE LAST */
	int	stack[1];
};

struct
{
	Lock;
	short	machs;
	short	exiting;
	short	ispanic;
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

#define	MACHP(n)	((Mach *)((int)&mach0+n*BY2PG))
extern Mach		mach0;

extern register Mach	*m;
extern register Proc	*up;

extern FPsave initfp;
