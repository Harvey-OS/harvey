/*
 * things specific to 386 and later cpus not covered by other headers.
 */
#ifdef DREGS				/* BIOS32 */
typedef struct BIOS32si	BIOS32si;
typedef struct BIOS32ci	BIOS32ci;
#pragma incomplete BIOS32si

typedef struct BIOS32ci {		/* BIOS32 Calling Interface */
	uint	eax;
	uint	ebx;
	uint	ecx;
	uint	edx;
	uint	esi;
	uint	edi;
} BIOS32ci;
#endif

typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef union FPsave	FPsave;
typedef struct FPssestate FPssestate;
typedef struct FPstate	FPstate;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct MMU	MMU;
typedef struct Mach	Mach;
typedef struct Msi	Msi;
typedef struct PCArch	PCArch;
typedef struct Pcidev	Pcidev;
typedef struct Page	Page;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef struct Segdesc	Segdesc;
typedef struct SFPssestate SFPssestate;
typedef vlong		Tval;
typedef struct Ureg	Ureg;
typedef struct Vctl	Vctl;

#pragma incomplete Pcidev
#pragma incomplete Ureg

#define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */

char *confname[MAXCONF];
char *confval[MAXCONF];
int nconf;

#ifndef KMESGSIZE
#define KMESGSIZE (32*1024)
#endif
#ifndef STAGESIZE
#define STAGESIZE 2048
#endif
#ifndef Intrsvcret
#define Intrsvcret int
#endif

enum {
	Cpuidlen = 12,
};

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	(I_MAGIC)

struct Lock
{
	ulong	key;		/* offset known to assembly */
	ulong	sr;
	ulong	pc;
	Proc	*p;
	Mach	*m;
	ushort	isilock;
	long	lockcycles;
};

struct Label
{
	ulong	sp;
	ulong	pc;
};


/*
 * FPsave.status
 */
enum
{
	/* this is a state */
	FPinit=		0,
	FPactive=	1,
	FPinactive=	2,

	/* the following is a bit that can be or'd into the state */
	FPillegal=	0x100,
};

struct	FPstate			/* x87 fpu state */
{
	ushort	control;
	ushort	r1;
	ushort	status;
	ushort	r2;
	ushort	tag;
	ushort	r3;
	ulong	pc;
	ushort	selector;
	ushort	r4;
	ulong	operand;
	ushort	oselector;
	ushort	r5;
	uchar	regs[80];	/* floating point registers */
};

struct	FPssestate		/* SSE fp state (32-bit layout) */
{
	ushort	fcw;		/* control */
	ushort	fsw;		/* status */
	ushort	ftw;		/* tag */
	ushort	fop;		/* opcode */
	ulong	fpuip;		/* pc */
	ushort	cs;		/* pc segment */
	ushort	r1;		/* reserved */
	ulong	fpudp;		/* data pointer */
	ushort	ds;		/* data pointer segment */
	ushort	r2;
	ulong	mxcsr;		/* MXCSR register state */
	ulong	mxcsr_mask;	/* MXCSR mask register */
	uchar	xregs[480];	/* extended regs: MM[0-7], XMM[0-7], rsv'd */
};

struct	SFPssestate		/* SSE fp state with alignment slop */
{
	FPssestate;
	uchar	alignpad[FPalign]; /* slop to allow copying to aligned addr */
	ulong	magic;		/* debugging: check for overrun by FXSAVE */
};

/*
 * the FP regs must be stored here, not somewhere pointed to from here.
 * port code assumes this.
 */
union FPsave {
	FPstate;
	SFPssestate;
};

struct Confmem
{
	ulong	base;
	ulong	npage;
	ulong	kbase;		/* set by xalloc, used by devproc */
	ulong	klimit;
};

/* system configuration: what hardware exists and how are we using it */
struct Conf
{
	ulong	nmach;		/* processors */
	ulong	cachelinesz;
	int	x86type;	/* manufacturer, usually */
	ushort	monmin;		/* smallest monitor line */
	ushort	monmax;		/* largest monitor line */
	uchar	havetsc;
	int	vm;		/* bits for vm types we could be in */
	ulong	monitor;	/* has monitor? */

	Confmem	mem[4];		/* physical memory */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	/* npage may exclude kernel pages */
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */

	ulong	nproc;		/* processes */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */

	ulong	ialloc;		/* max interrupt time allocation in bytes */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
	int	nuart;		/* number of uart devices */

	/* most cpuid results are the same across all cpus */
	int	cpuidax;	/* of Procsig */
	int	cpuidcx;	/* " */
	int	cpuid5cx;	/* of Procmon */
	int	cpuid7bx;	/* of Procid7 */
	char*	cpuidtype;	/* specific model */
	char	cpuidid[Cpuidlen+BY2WD]; /* " */
	char	brand[3*4*BY2WD + 1];	/* 3 sets of 4 registers */
};

enum Vms {
	Othervm		= 1<<0,
	Parallels	= 1<<1,
	Vmwarehyp	= 1<<2,
	Virtualbox	= 1<<3,
};

/*
 *  MMU stuff in proc
 */
#define NCOLOR 1
struct PMMU
{
	Page*	mmupdb;			/* page directory base */
	Page*	mmufree;		/* unused page table pages */
	Page*	mmuused;		/* used page table pages */
	Page*	kmaptable;		/* page table used by kmap */
	uint	lastkmap;		/* last entry used by kmap */
	int	nkmap;			/* number of current kmaps */
};

#include "../port/portdat.h"

typedef struct {
	ulong	link;			/* link (old TSS selector) */
	ulong	esp0;			/* privilege level 0 stack pointer */
	ulong	ss0;			/* privilege level 0 stack selector */
	ulong	esp1;			/* privilege level 1 stack pointer */
	ulong	ss1;			/* privilege level 1 stack selector */
	ulong	esp2;			/* privilege level 2 stack pointer */
	ulong	ss2;			/* privilege level 2 stack selector */
	ulong	xcr3;			/* page directory base register - not used because we don't use trap gates */
	ulong	eip;			/* instruction pointer */
	ulong	eflags;			/* flags register */
	ulong	eax;			/* general registers */
	ulong 	ecx;
	ulong	edx;
	ulong	ebx;
	ulong	esp;
	ulong	ebp;
	ulong	esi;
	ulong	edi;
	ulong	es;			/* segment selectors */
	ulong	cs;
	ulong	ss;
	ulong	ds;
	ulong	fs;
	ulong	gs;
	ulong	ldt;			/* selector for task's LDT */
	ulong	iomap;			/* I/O map base address + T-bit */
} Tss;

struct Segdesc
{
	ulong	d0;
	ulong	d1;
};

enum X86types {
	Intel,
	Amd,
	Sis,
	Centaur,
};

/* per-cpu state & stack; system-wide state should be in conf or active. */
struct Mach
{
	/* start of KNOWN TO ASSEMBLY */
	int	machno;			/* physical id of processor*/
	ulong	splpc;			/* pc of last caller to splhi */
	/* end of KNOWN TO ASSEMBLY */

	Proc*	externup;		/* extern register Proc *up */
	Proc*	proc;			/* current process on this processor */

	Tss*	tss;			/* tss for this processor */
	Segdesc	*gdt;			/* gdt for this processor */

	ulong*	pdb;			/* page directory base for this processor (va) */
	Page*	pdbpool;
	int	pdbcnt;
	int	pdballoc;
	int	pdbfree;

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Proc*	readied;		/* for runproc */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	ulong	schedticks;		/* next forced context switch */
	uint	loopconst;
	int	inclockintr;
	ulong	spuriousintr;		/* non-sysstat statistics */
	int	lastintr;
	uvlong	tscticks;
	uvlong	mwaitcycles;

	int	ilockdepth;
	int	flushmmu;		/* make current proc flush it's mmu state */
	FPsave	*fpsavalign;
	Lock	apictimerlock;
	ulong	mword;			/* old contents for mwait */

	int	tlbfault;		/* counters for sysstat */
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;

	Perf	perf;			/* performance counters */

	/* these don't vary enough to matter between cpus */
	int	cpumhz;
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */
	uvlong	cpuhz;
	uchar	havepge;

	/* only cpuid leaf 0xb (topology) on intel & Procsig's DX vary by cpu */
	int	cpuiddx;		/* of Procsig */

	vlong	mtrrcap;
	vlong	mtrrdef;
	vlong	mtrrfix[11];
	vlong	mtrrvar[32];		/* 256 max. */

	/*
	 * grows down from (char *)m+MACHSIZE toward here.
	 * max. seen used is 1472 bytes.
	 */
	int	stack[1];
};

/*
 * KMap the structure doesn't exist, but the functions do.
 */
typedef struct KMap		KMap;
#define	VA(k)		((void*)(k))
KMap*	kmap(Page*);
void	kunmap(KMap*);

/* current state of the system and its cpus, mostly software state */
struct
{
	Lock;
	ulong	machsmap[(MAXMACH+BI2WD-1)/BI2WD];  /* bitmap of active CPUs */
	int	nmachs;			/* number of bits set in machs(map) */
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
	int	thunderbirdsarego; /* let added processors continue to schedinit */
	int	rebooting;		/* just idle cpus > 0 */
}active;

/*
 *  routines for things outside the PC model, like power management
 */
struct PCArch
{
	char*	id;
	int	(*ident)(void);		/* this should be in the model */
	void	(*reset)(void);		/* this should be in the model */

	void	(*intrinit)(void);
	int	(*intrenable)(Vctl*);
	int	(*intrvecno)(int);
	int	(*intrdisable)(int);
	void	(*introff)(void);
	void	(*intron)(void);

	void	(*clockenable)(void);
	uvlong	(*fastclock)(uvlong*);
	void	(*timerset)(uvlong);

	void	(*resetothers)(void);	/* put other cpus into reset */
};

/* cpuid instruction result register bits */
enum {
	/* m->cpuiddx from Procsig */
	Fpuonchip = 1<<0,
	Vmex	= 1<<1,		/* virtual-mode extensions */
	Pse	= 1<<3,		/* page size extensions */
	Tsc	= 1<<4,		/* time-stamp counter */
	Cpumsr	= 1<<5,		/* model-specific registers, rdmsr/wrmsr */
	Pae	= 1<<6,		/* physical-addr extensions */
	Mce	= 1<<7,		/* machine-check exception */
	Cmpxchg8b = 1<<8,
	Cpuapic	= 1<<9,		/* have local apic */
	Mtrr	= 1<<12,	/* memory-type range regs.  */
	Pge	= 1<<13,	/* page global extension */
	Pse2	= 1<<17,	/* more page size extensions */
	Clflush = 1<<19,
	Mmx	= 1<<23,
	Fxsr	= 1<<24,	/* have SSE FXSAVE/FXRSTOR */
	Sse	= 1<<25,	/* thus sfence instr. */
	Sse2	= 1<<26,	/* thus mfence & lfence instr.s (2000-) */
};
enum {
	/* conf.cpuidcx from Procsig */
	Monitor	= 1<<3,		/* have monitor/mwait instr.s */
	Hypervisor= 1<<31,	/* running on a hypervisor */
};
enum {
	/* conf.cpuid5cx from Procmon */
	Mwaitext= 1<<0,		/* MWAIT extensions supported */
	Mwaitintrhi= 1<<1,	/* interrupts break MWAIT, even at splhi */
};

enum {				/* mwait extensions */
	Mwaitintrbreakshi = 1<<0,
};

enum {
	CR4tsd =	1 << 2,		/* disable RDTSC in user mode */
	CR4pse =	1 << 4,		/* page size extensions */
	CR4mce =	1 << 6,		/* machine check exception */
	CR4pge =	1 << 7,		/* page global enabled */
	CR4Osfxsr =	1 << 9,		/* fxsave/fxrstor instrs. */
	CR4smep =	1 << 20, /* disable execution by kernel of user pages */
};


/*
 *  a parsed plan9.ini line
 */
#define NISAOPT		8

struct ISAConf {
	char	*type;
	uintptr	port;
	int	irq;
	ulong	dma;
	uintptr	mem;
	usize	size;
	ulong	freq;

	int	nopt;
	char	*opt[NISAOPT];
};

extern PCArch	*arch;			/* PC architecture */

/*
 * Each processor sees its own Mach structure at address MACHADDR.
 * However, the Mach structures must also be available via the per-processor
 * MMU information array machp, mainly for disambiguation and access to
 * the clock which is only maintained by the bootstrap processor (0).
 */
Mach* machp[MAXMACH];
	
#define	MACHP(n)	(machp[n])

extern Mach	*m;
#define up	(((Mach*)MACHADDR)->externup)

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

/*
 * MSRs (model-specific registers).
 * these are architectural (i.e. not actually model-specific once defined)
 * except as noted.
 */
enum {
	Msrmcaddr,		/* machine-check address (pentium) */
	Msrmctype,		/* machine-check type (pentium) */
	Msrmonsize =	6,	/* monitor/mwait size */
	Msrtr12	=	0xe,	/* test register 12 (some pentium steppings) */
	Msrtsc =	0x10,	/* time-stamp counter */
	Msrapicbase =	0x1b,
	Msri64feat =	0x3a,
	Msrpmc0 =	0xc1,	/* intel only */
	Msrpmc1,
	Msrpmc2,
	/* etc. through pmc7 */
	MTRRCap =	0xfe,
	Msrpevsel0 =	0x186,	/* perf. event select 0; intel only */
	Msrpevsel1,
	/* etc. through evsel3 */
	Msrmiscen =	0x1a0,	/* enable misc. cpu features */
	/*
	 * MTRR Physical base/mask are pairs, indexed by
	 *	MTRRPhys{Base|Mask}N = MTRRPhys{Base|Mask}0 + 2*N
	 */
	MTRRPhysBase0 =	0x200,
	MTRRPhysMask0 =	0x201,
	/* ... */
	Msrmtrrfix0 =	0x250,
	Msrmtrrfix1 =	0x258,
	Msrmtrrfix2,
	Msrmtrrfix3 =	0x268,
	/* ... */
	MTRRDefaultType=0x2ff,
	Msrpebsen =	0x3f1,

	/* AMD-specific */
	AMsrpevsel0 =	0xc0010000,	/* and 3 more */
	AMsrpmc0 =	0xc0010004,	/* and 3 more */
};

enum {				/* mtrr fields */
	Capvcnt = 0xff,		/* mask: # of variable-range MTRRs we have */
	Capfix	= 1<<8,		/* have fixed MTRRs? */
	Capwc	= 1<<10,	/* have write combining? */

	Deftype = 0xff,		/* default MTRR type */
	Deffixena = 1<<10,	/* fixed-range MTRR enable */
	Defena	= 1<<11,	/* MTRR enable */

	Mtrrvalid = 1<<11,
};

enum {					/* bios cmos (nvram) registers */
	Cmosreset = 0xf,
};

enum {					/* Cmosreset values */
	Rstpwron = 0,			/* perform power-on reset */
	Rstnetboot= 4,			/* INT 19h reboot */
	Rstwarm	 = 0xa,	/* perform warm start: JMP via 32-bit ptr. at 0x467 */
};
