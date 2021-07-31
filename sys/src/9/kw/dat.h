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
typedef struct Pcidev	Pcidev;
typedef struct PhysUart	PhysUart;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef u32int		PTE;
typedef struct Soc	Soc;
typedef struct Uart	Uart;
typedef struct Ureg	Ureg;
typedef uvlong		Tval;

#pragma incomplete Pcidev
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

/*
 * emulated floating point
 */
struct FPsave
{
	ulong	status;
	ulong	control;
	ulong	regs[8][3];

	int	fpstate;
};

/*
 * FPsave.status
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
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
	ulong	monitor;	/* has monitor? */
	Confmem	mem[1];		/* physical memory */
	ulong	npage;		/* total physical pages of memory */
	usize	upages;		/* user page pool */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ialloc;		/* max interrupt time allocation in bytes */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
//	ulong	hz;		/* processor cycle freq */
//	ulong	mhz;
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
	int	inclockintr;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;		/* next forced context switch */

	int	cputype;
	int	socrev;			/* system-on-chip revision */
	ulong	delayloop;

	/* stats */
	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	vlong	fastclock;		/* last sampled value */
	uvlong	inidle;			/* time spent in idlehands() */
	ulong	spuriousintr;
	int	lastintr;
	int	ilockdepth;
	Perf	perf;			/* performance counters */

//	int	cpumhz;
	uvlong	cpuhz;			/* speed of cpu */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	/* save areas for exceptions */
	u32int	sfiq[5];
	u32int	sirq[5];
	u32int	sund[5];
	u32int	sabt[5];
#define fiqstack sfiq
#define irqstack sirq
#define abtstack sabt
#define undstack sund

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

enum {
	Frequency	= 1200*1000*1000,	/* the processor clock */
};

extern register Mach* m;			/* R10 */
extern register Proc* up;			/* R9 */

extern uintptr kseg0;
extern Mach* machaddr[MAXMACH];

enum {
	Nvec = 8,	/* # of vectors at start of lexception.s */
};

/*
 * Layout of physical 0.
 */
typedef struct Vectorpage {
	void	(*vectors[Nvec])(void);
	uint	vtable[Nvec];
} Vectorpage;

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

enum {
	Dcache,
	Icache,
	Unified,
};

/* characteristics of a given cache level */
struct Memcache {
	uint	level;		/* 1 is nearest processor, 2 further away */
	uint	kind;		/* I, D or unified */

	uint	size;
	uint	nways;		/* associativity */
	uint	nsets;
	uint	linelen;	/* bytes per cache line */
	uint	setsways;

	uint	log2linelen;
	uint	waysh;		/* shifts for set/way register */
	uint	setsh;
};

struct Soc {			/* addr's of SoC controllers */
	uintptr	cpu;
	uintptr	devid;
	uintptr	l2cache;
	uintptr	sdramc;
//	uintptr	sdramd;		/* unused */

	uintptr	iocfg;
	uintptr addrmap;
	uintptr	intr;
	uintptr	nand;
	uintptr	cesa;		/* crypto accel. */
	uintptr	ehci;
	uintptr spi;
	uintptr	twsi;

	uintptr	analog;
	uintptr	pci;
	uintptr	pcibase;

	uintptr	rtc;		/* real-time clock */
	uintptr	clock;

	uintptr	sata[3];
	uintptr	uart[2];
	uintptr	gpio[2];
} soc;
extern Soc soc;
