/*
 * Time.
 *
 * HZ should divide 1000 evenly, ideally.
 * 100, 125, 200, 250 and 333 are okay.
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

enum {
	Mhz	= 1000 * 1000,
};

typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Memcache	Memcache;
typedef struct MMMU	MMMU;
typedef struct Mach	Mach;
typedef struct Notsave	Notsave;
typedef struct Page	Page;
typedef struct PhysUart	PhysUart;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef u32int		PTE;
typedef struct Uart	Uart;
typedef struct Ureg	Ureg;
typedef uvlong		Tval;

#pragma incomplete Ureg

#define MAXSYSARG	5	/* for mount(fd, mpt, flag, arg, srv) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	(E_MAGIC)

struct Lock
{
	ulong	key;
	u32int	sr;
	uintptr	pc;
	Proc*	p;
	Mach*	m;
	int	isilock;
};

struct Label
{
	uintptr	sp;
	uintptr	pc;
};

enum {
	Maxfpregs	= 32,	/* could be 16 or 32, see Mach.fpnregs */
	Nfpctlregs	= 16,
};

/*
 * emulated or vfp3 floating point
 */
struct FPsave
{
	ulong	status;
	ulong	control;
	/*
	 * vfp3 with ieee fp regs; uvlong is sufficient for hardware but
	 * each must be able to hold an Internal from fpi.h for sw emulation.
	 */
	ulong	regs[Maxfpregs][3];

	int	fpstate;
	uintptr	pc;		/* of failed fp instr. */
};

/*
 * FPsave.fpstate
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
	FPemu,

	/* bits or'd with the state */
	FPillegal= 0x100,
};

struct Confmem
{
	uintptr	base;
	usize	npage;
	uintptr	limit;
	uintptr	kbase;
	uintptr	klimit;
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	Confmem	mem[1];		/* physical memory */
	ulong	npage;		/* total physical pages of memory */
	usize	upages;		/* user page pool */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ialloc;		/* max interrupt time allocation in bytes */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	hz;		/* processor cycle freq */
	ulong	mhz;
	int	monitor;	/* flag */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct Notsave {
	int	emptiness;
};

/*
 *  MMU stuff in Mach.
 */
struct MMMU
{
	PTE*	mmul1;		/* l1 for this processor */
	int	mmul1lo;
	int	mmul1hi;
	int	mmupid;
};

/*
 *  MMU stuff in proc
 */
#define NCOLOR	1		/* 1 level cache, don't worry about VCE's */
struct PMMU
{
	Page*	mmul2;
	Page*	mmul2cache;	/* free mmu pages */
};

#include "../port/portdat.h"

struct Mach
{
	int	machno;			/* physical id of processor */
	uintptr	splpc;			/* pc of last caller to splhi */

	Proc*	proc;			/* current process */

	MMMU;
	int	flushmmu;		/* flush current proc mmu state */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */

	Proc*	readied;		/* for runproc */
	ulong	schedticks;		/* next forced context switch */

	int	cputype;
	ulong	delayloop;

	/* stats */
	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	uvlong	fastclock;		/* last sampled value */
	ulong	spuriousintr;
	int	lastintr;
	int	ilockdepth;
	Perf	perf;			/* performance counters */


	int	cpumhz;
	uvlong	cpuhz;			/* speed of cpu */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	/* vfp2 or vfp3 fpu */
	int	havefp;
	int	havefpvalid;
	int	fpon;
	int	fpconfiged;
	int	fpnregs;
	ulong	fpscr;			/* sw copy */
	int	fppid;			/* pid of last fault */
	uintptr	fppc;			/* addr of last fault */
	int	fpcnt;			/* how many consecutive at that addr */

	/* save areas for exceptions, hold R0-R4 */
	u32int	sfiq[5];
	u32int	sirq[5];
	u32int	sund[5];
	u32int	sabt[5];
	u32int	smon[5];		/* probably not needed */
	u32int	ssys[5];

	int	stack[1];
};

/*
 * Fake kmap.
 */
typedef void		KMap;
#define	VA(k)		((uintptr)(k))
#define	kmap(p)		(KMap*)((p)->pa|kseg0)
#define	kunmap(k)

struct
{
	Lock;
	int	machs;			/* bitmap of active CPUs */
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
}active;

extern register Mach* m;			/* R10 */
extern register Proc* up;			/* R9 */
extern uintptr kseg0;
extern Mach* machaddr[MAXMACH];
extern ulong memsize;
extern int normalprint;

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

#define	MACHP(n)	(machaddr[n])

/*
 * Horrid. But the alternative is 'defined'.
 */
#ifdef _DBGC_
#define DBGFLG		(dbgflg[_DBGC_])
#else
#define DBGFLG		(0)
#endif /* _DBGC_ */

int vflag;
extern char dbgflg[256];

#define dbgprint	print		/* for now */

/*
 *  hardware info about a device
 */
typedef struct {
	ulong	port;
	int	size;
} Devport;

struct DevConf
{
	ulong	intnum;			/* interrupt number */
	char	*type;			/* card type, malloced */
	int	nports;			/* Number of ports */
	Devport	*ports;			/* The ports themselves */
};

