typedef struct Asm Asm;
typedef struct Fxsave Fxsave;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct MCPU MCPU;
typedef struct MFPU MFPU;
typedef struct MMMU MMMU;
typedef struct Mach Mach;
typedef u64int Mpl;
typedef Mpl Mreg;				/* GAK */
typedef struct Page Page;
typedef struct Pcidev Pcidev;
typedef struct PFPU PFPU;
typedef struct PMMU PMMU;
typedef struct PNOTIFY PNOTIFY;
typedef u64int PTE;
typedef struct Proc Proc;
typedef struct Sys Sys;
typedef u64int uintmem;				/* horrible name */
typedef vlong Tval;
typedef struct Ureg Ureg;
typedef struct Vctl Vctl;

#pragma incomplete Ureg

#define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	(S_MAGIC)

/*
 *  machine dependent definitions used by ../port/portdat.h
 */
struct Lock
{
	u32int	key;
	int	isilock;
	union {					/* GAK */
		Mpl	pl;
		Mreg	sr;
	};
	uintptr	pc;
	Proc*	p;
	Mach*	m;
};

struct Label
{
	uintptr	sp;
	uintptr	pc;
};

struct Fxsave {
	u16int	fcw;			/* x87 control word */
	u16int	fsw;			/* x87 status word */
	u8int	ftw;			/* x87 tag word */
	u8int	zero;			/* 0 */
	u16int	fop;			/* last x87 opcode */
	u64int	rip;			/* last x87 instruction pointer */
	u64int	rdp;			/* last x87 data pointer */
	u32int	mxcsr;			/* MMX control and status */
	u32int	mxcsrmask;		/* supported MMX feature bits */
	uchar	st[128];		/* shared 64-bit media and x87 regs */
	uchar	xmm[256];		/* 128-bit media regs */
	uchar	ign[96];		/* reserved, ignored */
};

/*
 *  FPU stuff in Proc
 */
struct PFPU {
	int	fpustate;
	uchar	fxsave[sizeof(Fxsave)+15];
	void*	fpusave;
};

/*
 *  MMU stuff in Proc
 */
#define NCOLOR 8
struct PMMU
{
	Page*	mmuptp[4];		/* page table pages for each level */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct PNOTIFY
{
	void	emptiness;
};

/*
 * Address Space Map.
 * Low duty cycle.
 */
struct Asm
{
	uintmem	addr;
	uintmem	size;
	int	type;
	int	location;
	Asm*	next;
	uintmem base;	/* used by port; ROUNDUP(addr, PGSZ) */
	uintmem	limit;	/* used by port; ROUNDDN(addr+size, PGSZ) */
	uintptr	kbase;	/* used by port; kernel for base, used by devproc */
};
extern Asm* asmlist;

#include "../port/portdat.h"

/*
 *  CPU stuff in Mach.
 */
struct MCPU {
	u32int	cpuinfo[2][4];		/* CPUID instruction output E[ABCD]X */
	int	ncpuinfos;		/* number of standard entries */
	int	ncpuinfoe;		/* number of extended entries */
	int	isintelcpu;		/*  */
};

/*
 *  FPU stuff in Mach.
 */
struct MFPU {
	u16int	fcw;			/* x87 control word */
	u32int	mxcsr;			/* MMX control and status */
	u32int	mxcsrmask;		/* supported MMX feature bits */
};

/*
 *  MMU stuff in Mach.
 */
enum
{
	NPGSZ = 4
};

struct MMMU
{
	Page*	pml4;			/* pml4 for this processor */
	PTE*	pmap;			/* unused as of yet */

	uint	pgszlg2[NPGSZ];		/* per Mach or per Sys? */
	uintmem	pgszmask[NPGSZ];
	uint	pgsz[NPGSZ];
	int	npgsz;

	Page	pml4kludge;		/* GAK: we need a page */
};

/*
 * Per processor information.
 *
 * The offsets of the first few elements may be known
 * to low-level assembly code, so do not re-order:
 *	machno	- no dependency, convention
 *	splpc	- splhi, spllo, splx
 *	proc	- syscallentry
 */
struct Mach
{
	int	machno;			/* physical id of processor */
	uintptr	splpc;			/* pc of last caller to splhi */

	Proc*	proc;			/* current process on this processor */

	int	apicno;
	int	online;
	int	mode;			/* fold into online? GAK */

	MMMU;

	uintptr	stack;
	uchar*	vsvm;
	void*	gdt;
	void*	tss;

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	int	inclockintr;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;		/* next forced context switch */

	int	color;

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	mmuflush;		/* make current proc flush it's mmu state */
	int	ilockdepth;
	uintptr	ilockpc;
	Perf	perf;			/* performance counters */

	int	lastintr;

	Lock	apictimerlock;
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */
	vlong	cpuhz;
	int	cpumhz;
	u64int	rdtsc;

	MFPU;
	MCPU;
};

/*
 * This is the low memory map, between 0x100000 and 0x110000.
 * It is located there to allow fundamental datastructures to be
 * created and used before knowing where free memory begins
 * (e.g. there may be modules located after the kernel BSS end).
 * The layout is known in the bootstrap code in l32p.s.
 * It is logically two parts: the per processor data structures
 * for the bootstrap processor (stack, Mach, vsvm, and page tables),
 * and the global information about the system (syspage, ptrpage).
 * Some of the elements must be aligned on page boundaries, hence
 * the unions.
 */
struct Sys {
	uchar	machstk[MACHSTKSZ];

	PTE	pml4[PTSZ/sizeof(PTE)];	/*  */
	PTE	pdp[PTSZ/sizeof(PTE)];
	PTE	pd[PTSZ/sizeof(PTE)];
	PTE	pt[PTSZ/sizeof(PTE)];

	uchar	vsvmpage[4*KiB];

	union {
		Mach	mach;
		uchar	machpage[MACHSZ];
	};

	union {
		struct {
			u64int	pmstart;	/* physical memory */
			u64int	pmoccupied;	/* how much is occupied */
			u64int	pmend;		/* total span */
			u64int	pmpaged;	/* how much assigned to page pool */

			uintptr	vmstart;	/* base address for malloc */
			uintptr	vmunused;	/* 1st unused va */
			uintptr	vmunmapped;	/* 1st unmapped va */
			uintptr	vmend;		/* 1st unusable va */

			u64int	epoch;		/* crude time synchronisation */
			int	nmach;		/* how many machs */
			int	nonline;	/* how many machs are online */
			uint	ticks;		/* since boot (type?) */
			uint	copymode;	/* 0 is COW, 1 is copy on ref */
		};
		uchar	syspage[4*KiB];
	};

	union {
		Mach*	machptr[MACHMAX];
		uchar	ptrpage[4*KiB];
	};

	uchar	_57344_[2][4*KiB];		/* unused */
};

extern Sys* sys;

/*
 * KMap
 */
typedef void KMap;
extern KMap* kmap(Page*);

#define kunmap(k)
#define VA(k)		PTR2UINT(k)

struct
{
	Lock;
	uint	machs;			/* bitmap of active CPUs */
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
	int	thunderbirdsarego;	/* F.A.B. */
}active;

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

/*
 * The Mach structures must be available via the per-processor
 * MMU information array machptr, mainly for disambiguation and access to
 * the clock which is only maintained by the bootstrap processor (0).
 */
extern register Mach* m;			/* R15 */
extern register Proc* up;			/* R14 */

extern uintptr kseg0;

/*
 * Horrid.
 */
#ifdef _DBGC_
#define DBGFLG		(dbgflg[_DBGC_])
#else
#define DBGFLG		(0)
#endif /* _DBGC_ */

#define DBG(...)	if(!DBGFLG){}else dbgprint(__VA_ARGS__)

extern char dbgflg[256];

#define dbgprint	print		/* for now */

#pragma	varargck	type	"P"	uintmem	/* GAK */
