typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct ISAConf	ISAConf;
typedef struct Imap	Imap;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct Notsave	Notsave;
typedef struct PCArch	PCArch;
typedef struct PMMU	PMMU;
typedef struct Page	Page;
typedef struct Pcidev	Pcidev;
typedef struct Proc	Proc;
typedef struct Sys	Sys;
typedef vlong		Tval;
typedef struct Ureg	Ureg;
typedef struct Vctl	Vctl;

#pragma incomplete Ureg
#pragma incomplete Imap
#pragma incomplete Mach

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
	ulong	key;			/* semaphore (non-zero = locked) */
	ulong	sr;
	ulong	pc;
	Proc	*p;
	Mach	*m;
	ulong	pid;
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
	/* Floating point states */
	FPinit = 0,
	FPactive = 1,
	FPinactive = 2,
	/* Bit that's or-ed in during note handling (FP is illegal in note handlers) */
	FPillegal = 0x100,
};

/*
 * This structure must agree with fpsave and fprestore asm routines
 */
struct FPsave
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
	Confmem	mem[2];
	ulong	npage0;		/* total physical pages of memory */
	ulong	npage1;		/* total physical pages of memory */
	ulong	npage;		/* total physical pages of memory */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	monitor;	/* has display? */
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
	Ureg	*mmureg;		/* pointer to ureg structure */
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

struct IMM;
typedef struct IMM IMM;

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
/*0x00*/	int	machno;	/* physical id of processor */
/*0x04*/	ulong	splpc;	/* pc that called splhi() */
/*0x08*/	Proc	*proc;	/* current process on this processor */
	/* Debugging/statistics for software TLB in l.s (therefore, also known by l.s) */
/*0x0c*/	ulong	tlbfault;	/* type of last miss */
/*0x10*/	ulong	imiss;	/* number of instruction misses */
/*0x14*/	ulong	dmiss;	/* number of data misses */

	/* ordering from here on irrelevant */

	Imap*	imap;
#ifndef ucuconf
	IMM*	immr;
#endif
	ulong	ticks;		/* of the clock since boot time */
	Label	sched;		/* scheduler wakeup */
	Lock	alarmlock;	/* access to alarm list */
	void	*alarm;		/* alarms bound to this clock */
	int	inclockintr;
	int	cputype;
	ulong	loopconst;
	Perf	perf;		/* performance counters */

	Proc*	readied;	/* for runproc */
	ulong	schedticks;	/* next forced context switch */

	ulong	clkin;		/* basic clock frequency */
	ulong	vco_out;
	vlong	cpuhz;
	uvlong	cyclefreq;	/* Frequency of user readable cycle counter */
	ulong	bushz;
	ulong	dechz;
	ulong	tbhz;
	ulong	cpmhz;		/* communications processor module frequency */
	ulong	brghz;		/* baud rate generator frequency */

	ulong	pcclast;
	uvlong	fastclock;

	int	tlbpurge;	/* # of tlb purges */
	int	pfault;		/* # of page faults */
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	flushmmu;	/* make current proc flush it's mmu state */
	int	ilockdepth;

	ulong	ptabbase;	/* start of page table in kernel virtual space */
	int	slotgen;	/* next pte (byte offset) when pteg is full */
	int	mmupid;		/* next mmu pid to use */
	int	sweepcolor;
	int	trigcolor;
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
	char	*type;
	ulong	port;
	int	irq;
	ulong	dma;
	ulong	mem;
	ulong	size;
	ulong	freq;

	int	nopt;
	char	*opt[NISAOPT];
};

struct Vctl {
	Vctl*	next;		/* handlers on this vector */

	char	name[KNAMELEN];	/* of driver */
	int	isintr;		/* interrupt or fault/trap */
	int	irq;

	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;		/* argument to call it with */
};

extern Mach mach0;

extern register Mach *m;
extern register Proc *up;

extern FPsave initfp;
