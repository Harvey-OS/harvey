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
	Dogsectimeout = 30,	/* must be ≤ 34 s. to fit interval in a ulong */
};

/*
 * More accurate time
 */
#define MS2TMR(t)	((ulong)(((uvlong)(t) * m->cpuhz)/1000))
#define US2TMR(t)	((ulong)(((uvlong)(t) * m->cpuhz)/1000000))

#define CONSOLE 0

typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct Fpinst	Fpinst;
typedef ulong		Inst;			/* an arm instruction */
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Lowmemcache Lowmemcache;
typedef struct Memcache	Memcache;
typedef struct MMMU	MMMU;
typedef struct Mach	Mach;
typedef u32int		Mreg;			/* Msr - bloody UART */
typedef struct Page	Page;
typedef struct Pcisiz	Pcisiz;
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

enum {
	Maxfpregs	= 32,	/* could be 16 or 32, see Mach.fpnregs */
	Nfpctlregs	= 16,
};

/* vfp control regs.  most are read-only */
enum {
	Fpsid =	0,
	Fpscr =	1,			/* rw */
	Mvfr1 =	6,
	Mvfr0 =	7,
	Fpexc =	8,			/* rw */
	Fpexinst= 9,			/* optional, for exceptions */
	Fpexinst2=10,
};

#ifndef _FPI_H
#define Vlong Fpivlong		/* avoid conflict with ../port/clock.c */
#include "../port/fpi.h"
#endif

/*
 * emulated or vfp3 floating point
 */
typedef union Fprepr Fprepr;
union Fprepr {
	struct {
		Internal;	/* sw emulation representation */
		Vlong	irepr;	/* integer when above is NaN (vmov, vcvt) */
		int	useirepr; /* flag: use irepr, not Internal */
	};
	uvlong	dbl;		/* copy of double hardware reg */
};

struct FPsave
{
	ulong	status;
	ulong	control;
	Fprepr	regs[Maxfpregs];
	int	fpstate;
	uintptr	pc;		/* of failed fp instr. */
};

#undef Vlong

/*
 * the fp instruction to emulate, and its broken-out opcode
 * and coproc fields; ureg->pc is its address.
 */
struct Fpinst {	
	Inst	inst;
	ulong	op;
	ulong	coproc;
	uint	vd;		/* vfp reg d */
};

/*
 * FPsave.fpstate
 */
enum
{
	FPinit,
	FPactive,		/* hw enabled */
	FPinactive,		/* hw present but off */
	FPemu,			/* hw absent */

	/* bit or'd with the state */
	FPillegal= 0x100,
};

struct Confmem
{
	uintptr	base;		/* phys address */
	usize	npage;
	uintptr	limit;
	uintptr	kbase;		/* set by xalloc for devproc */
	uintptr	klimit;
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	Confmem	mem[1];		/* physical memory */
	/* npage may exclude kernel pages */
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
	int	cpurev;
	int	cpupart;
};

/*
 *  MMU stuff in Mach.
 */
struct MMMU
{
	PTE*	mmul1;		/* l1 for this processor */
	int	mmul1lo;	/* index of mmul1 after text+data ptes */
	int	mmul1hi;	/* index of mmul1 of lowest stack pte */
	int	mmupid;
};

/*
 *  MMU stuff in proc
 */
/*
 * for worst-case 64KB caches, address bits 12-13, use NCOLOR 4.
 * we actually use 32KB L1 cache, 4K pages.
 * in any case, the arm v7-a architecture hides the need to color pages.
 */
#define NCOLOR	1
struct PMMU
{
	Page*	mmul2;
	Page*	mmul2cache;	/* free mmu pages */
};

#define noprint (m == nil || !m->printok)

#include "../port/portdat.h"

struct Mach
{
	/* offsets known to asm */
	int	machno;			/* physical id of processor */
	uintptr	splpc;			/* pc of last caller to splhi */

	Proc*	proc;			/* current process */

	MMMU;
	/* end of offsets known to asm */

	int	flushmmu;		/* flush current proc mmu state */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	int	inclockintr;

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

	int	probing;		/* probeaddr() state */
	int	trapped;
	Lock	probelock;
	int	inidlehands;
	int	printok;		/* initialised enough to print()? */

	int	cpumhz;
	uvlong	cpuhz;			/* speed of cpu */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	/* vfp3 fpu */
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

	int	stack[1];		/* 1248 bytes is typical usage */
};

/*
 * Fake kmap.
 */
typedef void		KMap;
#define	VA(k)		((uintptr)(k))
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kunmap(k)

struct
{
	Lock;
	ulong	machsmap[(MAXMACH+BI2WD-1)/BI2WD];
	int	nmachs;			/* number of bits set in machs(map) */
	char	wfi[MAXMACH];		/* byte map of CPUs in WFI state */
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
	int	thunderbirdsarego;	/* lets the added processors continue to schedinit */
}active;

extern register Mach* m;			/* R10 */
extern register Proc* up;			/* R9 */

extern Memcache cachel[];		/* arm arch v7 supports 1-7 */
extern ulong intrcount[MAXMACH];
extern int irqtooearly;
extern Mach* machaddr[MAXMACH];
extern ulong memsize;
extern int navailcpus;
extern ulong sgicnt[MAXMACH];
void (*wfiloop)(void);

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

#define	MACHP(n) machaddr[n]

/*
 *  hardware info about a device.  mainly for devsd.
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

/* characteristics of a given arm cache level */
struct Memcache {
	uint	waysh;		/* shifts for set/way register */
	uint	setsh;

	uint	log2linelen;

	uint	level;		/* 1 is nearest processor, 2 further away */
	uint	type;
	uint	external;	/* flag */
	uint	l1ip;		/* l1 I policy */

	uint	nways;		/* associativity */
	uint	nsets;
	uint	linelen;	/* bytes per cache line */
	uint	setsways;
};
enum Cachetype {
	Nocache,
	Ionly,
	Donly,
	Splitid,
	Unified,
};
enum {
	Intcache,
	Extcache,
};

/*
 * cache capabilities.  write-back vs write-through is controlled
 * by the Buffered bit in PTEs.
 *
 * see cache.v7.s and Memcache in dat.h
 */
enum {
	Cawt	= 1 << 31,
	Cawb	= 1 << 30,
	Cara	= 1 << 29,
	Cawa	= 1 << 28,
};

/* non-architectural L2 cache */
typedef struct Cacheimpl Cacheimpl;
struct Cacheimpl {
	void	(*info)(Memcache *);
	void	(*on)(void);
	void	(*off)(void);

	void	(*inv)(void);
	void	(*wb)(void);
	void	(*wbinv)(void);

	void	(*invse)(void *, int);
	void	(*wbse)(void *, int);
	void	(*wbinvse)(void *, int);
};
Cacheimpl *l2cache;

/* pmu = power management unit */
enum Irqs {
	/*
	 * 1st 32 gic irqs reserved for cpu; private interrupts.
	 *  0—15 are software-generated by other cpus;
	 * 16—31 are private peripheral intrs.
	 */
	Cpu0irq		= 0,
	Cpu1irq,
	/* ... */
	Cpu15irq	= 15,
	Glbtmrirq	= 27,
	Loctmrirq	= 29,
	Wdtmrirq	= 30,

	/* shared interrupts */
	Ctlr0base	= (1+0)*32,		/* primary ctlr */
	Tn0irq		= Ctlr0base + 0,	/* tegra timers */
	Tn1irq		= Ctlr0base + 1,
	Rtcirq		= Ctlr0base + 2,

	Ctlr1base	= (1+1)*32,		/* secondary ctlr */
	Uartirq		= Ctlr1base + 4,
	Tn2irq		= Ctlr1base + 9,	/* tegra timers */
	Tn3irq		= Ctlr1base + 10,
	/* +24 is cpu0_pmu_intr, +25 is cpu1_pmu_intr */

	Ctlr2base	= (1+2)*32,		/* ternary ctlr */
	Extpmuirq	= Ctlr2base + 22,

	Ctlr3base	= (1+3)*32,		/* quad ctlr */
	Pcieirq		= Ctlr3base + 2,	/* ether8169 */
	Pciemsiirq	= Ctlr3base + 3,
};

struct Soc {			/* addr's of SoC controllers */
	uintptr clkrst;
	uintptr	power;
	uintptr	exceptvec;
	uintptr	sema;
	uintptr	l2cache;
	uintptr	flow;

	/* private memory region */
	uintptr	scu;
	uintptr	intr;		/* `cpu interface' */
	/* private-peripheral-interrupt cortex-a clocks */
	uintptr	glbtmr;
	uintptr	loctmr;

	uintptr	intrdist;

	uintptr	uart[5];

	/* shared-peripheral-interrupt tegra2 clocks */
	uintptr	rtc;		/* real-time clock */
	uintptr	tmr[4];
	uintptr	µs;

	uintptr	pci;
	uintptr	ether;

	uintptr	ehci;
	uintptr	ide;

	uintptr	nand;
	uintptr	nor;

	uintptr	spi[4];
	uintptr	twsi;
	uintptr	mmc[4];
	uintptr	gpio[7];
} soc;
extern Soc soc;
