typedef struct Asm Asm;
typedef struct Conf Conf;
typedef struct Fxsave Fxsave;
typedef struct ISAConf ISAConf;
typedef struct Kmesg Kmesg;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct MCPU MCPU;
typedef struct MFPU MFPU;
typedef struct MMMU MMMU;
typedef struct Mach Mach;
typedef u64int Mpl;		/* only need If bit; short would suffice */
typedef u64int Mreg;
typedef struct Msi Msi;
typedef struct Page Page;
typedef struct Pcidev Pcidev;
typedef struct PFPU PFPU;
typedef struct PMMU PMMU;
typedef struct PNOTIFY PNOTIFY;
typedef u64int PTE;
typedef struct Proc Proc;
typedef struct Sipi Sipi;
typedef struct Sipireboot Sipireboot;
typedef struct Sys Sys;
typedef struct Syscomm Syscomm;
typedef uintptr uintmem;		/* physical address.  horrible name */
typedef struct Ureg Ureg;
typedef struct Vctl Vctl;

#pragma incomplete Ureg

#define SYSCALLTYPE	64	/* nominal syscall trap type (not used) */
#define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */

#define INTRSVCVOID
#define Intrsvcret void

/* incorporates extended-model and -family bits from cpuid instruction */
/* 0x0FFM0FM0		x/case.*0/ s/0x000(.)06(.)0/0x\1\2/ */
#define X86FAMILY(x)	((((x)>>8) & 0x0F) + (((x)>>20) & 0xFF))
#define X86MODEL(x)	((((x)>>4) & 0x0F) | (((x)>>16) & 0x0F)<<4)

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	(S_MAGIC)

/*
 *  machine dependent definitions used by ../port/portdat.h
 */
struct Lock
{
	int	key;
	int	isilock;
	union {
//		Mpl	pl;
		Mreg	sr;
	};
	uintptr	pc;
	Proc*	p;
	Mach*	m;
	ulong	noninit;		/* must be zero */
};

struct Label
{
	uintptr	sp;
	uintptr	pc;
};

struct Fxsave {				/* FXSAVE64 layout */
	u16int	fcw;			/* x87 control word */
	u16int	fsw;			/* x87 status word */
	uchar	ftw;			/* x87 tag word */
	uchar	zero;			/* 0 */
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
	int	fpustate;		/* per-process logical state */
	/* actual save area must be 16-byte aligned for save & restore instr.s */
	uchar	fxsave[sizeof(Fxsave)+FPalign-1];
	void*	fpusave;		/* points into fxsave */
};

enum Vms {
	Othervm		= 1<<0,
	Parallels	= 1<<1,
	Vmwarehyp	= 1<<2,
	Virtualbox	= 1<<3,
};

/*
 *  MMU stuff in Proc
 */
#define NCOLOR 8
struct PMMU
{
	Page*	mmuptp[Npglvls];	/* page table pages for each level */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct PNOTIFY
{
//	void	emptiness;		/* generates an acid syntax error */
	uchar	emptiness;
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

/* system configuration: what hardware exists and how are we using it */
/* dreg from 9 */
struct Conf
{
	ulong	monitor;	/* has monitor? */
};
Conf conf;

#include "../port/portdat.h"

/*
 *  CPU stuff in Mach.
 */
struct MCPU {
	u32int	cpuinfo[2][4];		/* CPUID instruction output E[ABCD]X */
	int	ncpuinfos;		/* number of standard entries */
	int	ncpuinfoe;		/* number of extended entries */
	int	isintelcpu;		/* flag; not used yet */
};

/*
 *  per-cpu physical FPU state in Mach.
 */
struct MFPU {
	u16int	fcw;			/* x87 control word */
	u32int	mxcsr;			/* MMX control and status */
	u32int	mxcsrmask;		/* supported MMX feature bits */

	uvlong	fpsaves;		/* counters */
	uvlong	fprestores;
};

/*
 *  MMU stuff in Mach.
 */
enum
{
	NPGSZ = 4			/* max. number of page sizes */
};

struct MMMU
{
	Page*	pml4;			/* pml4 for this processor */
	PTE*	pmap;			/* unused as of yet */

	uint	pgszlg2[NPGSZ];		/* actually per system */
	uintmem	pgszmask[NPGSZ];
	int	npgsz;

	Page	pml4kludge;		/* we need a page */
};

enum X86types {
	Intel,
	Amd,
	Sis,
	Centaur,
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

	MMMU;

	uintptr	stack;
	uchar*	vsvm;		/* wretched x86 stuff: gdt, tss, intr stack */
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

	int	tlbfault;		/* per-cpu stats */
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;

	int	mmuflush;		/* make current proc flush its mmu state */
	int	ilockdepth;
	uintptr	ilockpc;
	Perf	perf;			/* performance counters */

	int	lastintr;		/* debugging: irq of last intr */

	Lock	apictimerlock;
	uchar	clockintrsok;		/* safe to call timerintr */
	vlong	cpuhz;	/* also frequency of user readable cycle counter */
	int	cpumhz;
	u64int	rdtsc;			/* tsc value at boot */

	MFPU;
	MCPU;
};

struct Kmesg {
	struct  Kmesghdr {
		ulong	magic;
		uint	size;	/* of buf; could change across reboots */
		uint	n;	/* bytes in use */
	};
	char	buf[KMESGSIZE - sizeof(struct Kmesghdr)];
};

/*
 * This is the low memory map, between 0x100000 and 0x120000 (128K).
 * It is located there to allow fundamental datastructures to be
 * created and used before knowing where free memory begins
 * (e.g. there may be modules located after the kernel BSS end).
 * The layout is known in the bootstrap code in l32p.s and l16sipi.s,
 * mmu.c and sipi.c, and also mem.h
 * It is logically two parts: the per processor data structures
 * for the bootstrap processor (stack, Mach, vsvm, and page tables),
 * and the global information about the system (syspage, ptrpage).
 * Some of the elements must be aligned on page boundaries, hence
 * the unions.
 */

/*
 * portion common to all cpus; cpus other than 0 allocate their own in sipi().
 * The sipi code assumes pml4 follows machstk and that the address of either
 * can be derived from the other, so keep them together and in order.
 * Currently 48K.
 */
struct Syscomm {
	uchar	machstk[MACHSTKSZ];

	PTE	pml4[PTSZ/sizeof(PTE)];	/* initial page tables */
	PTE	pdp[PTSZ/sizeof(PTE)];
	PTE	pd[PTSZ/sizeof(PTE)];
	PTE	pt[PTSZ/sizeof(PTE)];

	uchar	vsvmpage[PGSZ];		/* gdt, tss, intr stack */

	union {
		Mach	mach;
		uchar	machpage[MACHSZ];
	};
};
struct Sys {
	Syscomm;

	/* rest is statically allocated once for cpu0 to share with all cpus */
	union {
		struct {
			u64int	pmstart;	/* physical memory */
			u64int	pmoccupied;	/* how much is occupied */
			u64int	pmend;		/* total span */

			uintptr	vmstart;	/* base address for malloc */
			uintptr	vmunused;	/* 1st unused va */
			uintptr	vmunmapped;	/* 1st unmapped va */
			uintptr	vmend;		/* 1st unusable va */

			/* epoch is tsc value on cpu0 at boot */
			u64int	epoch;		/* crude time synchronisation */
			uvlong	tscoff[MACHMAX]; /* diff from cpu0's tsc */
			int	nmach;		/* how many machs */
			int	nonline;	/* how many machs are online */
			uint	ticks;		/* since boot (type?) */
			uint	copymode;	/* 0 is COW, 1 is copy on ref */

			Lock	kmsglock;
			char	haveintrbreaks;
		};
		uchar	syspage[PGSZ];
	};

	union {
		Mach*	machptr[MACHMAX];
		uchar	ptrpage[PGSZ];
	};

	PTE	pd2g[PTSZ/sizeof(PTE)];		/* for 2nd GB of kernel space */
	PTE	pt2g[PTSZ/sizeof(PTE)];

	union {
		uchar	sysextend[SYSEXTEND];	/* includes room for growth */
		struct {
			uchar	gap[SYSEXTEND - sizeof(Kmesg)];
			/* must be last so we can avoid zeroing it at start */
			Kmesg	kmsg;
		};
	};
};
CTASSERT(sizeof(Syscomm) == LOW0SYS, Syscomm_c_as_offsets_differ);
CTASSERT(sizeof(Sys) == LOW0END, Sys_c_as_offsets_differ);
CTASSERT(sizeof(Kmesg) == KMESGSIZE, Kmesg_wrong_size);

extern Sys* sys;
extern uintptr kernmem;			/* used by KADDR, PADDR */
extern int mallocok;			/* flag: mallocinit() has been called */

/*
 * KMap
 */
typedef void KMap;
extern KMap* kmap(Page*);

#define kunmap(k)
#define VA(k)		(uintptr)(k)

struct
{
	Lock;
//	union {				/* bitmap of active CPUs */
//		uvlong	machs;		/* backward compatible to 64 cpus */
		ulong	machsmap[(MACHMAX+BI2WD-1)/BI2WD];
//	};
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
	int	thunderbirdsarego; /* let added CPUs continue to schedinit */
	int	rebooting;		/* just idle cpus > 0 */
} active;

enum {				/* cpuid standard function codes */
	Highstdfunc = 0,	/* also returns vendor string */
	Procsig,
	Proctlbcache,
	Procserial,
	/* check Highstdfunc to see if these are allowed */
	Procmon = 5,		/* monitor/mwait capabilities */
	Procid7 = 7,

	Hypercpuid  = 0x40000000,	/* hypervisor's cpu id */

	Highextfunc = 0x80000000,
	Extfeature,
	Extbrand0,
	Extbrand1,
	Extbrand2,
	Extcap = Highextfunc + 8,
};
enum {				/* cpuidinfo result array indices */
	Ax, Bx, Cx, Dx,
};

/*
 *  a parsed plan9.ini line
 *  only still used by pnp, uarts, ether.
 */
#define NISAOPT		8

struct ISAConf {
	char	*type;
	uintptr	port;
//	int	irq;		/* see Intrcommon */
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

/*
 * Horrid.
 */
#ifdef _DBGC_
#define DBGFLG		(dbgflg[_DBGC_])
#else
#define DBGFLG		0
#endif /* _DBGC_ */

#define DBG(...)	if(!DBGFLG){}else dbgprint(__VA_ARGS__)

extern char dbgflg[256];

#define dbgprint	print		/* for now */

#pragma	varargck	type	"P"	uintmem

/* acpi/multiboot memory types */
enum {
	AsmNONE		= 0,
	AsmMEMORY	= 1,
	AsmRESERVED	= 2,
	AsmACPIRECLAIM	= 3,
	AsmACPINVS	= 4,
	AsmDEV		= 5,		/* device registers */
};

struct Sipireboot {
	void	*tramp;
	void	*physentry;	/* kernel properties */
	void	*src;
	int	len;
};

/*
 * Parameters are passed to the bootstrap code via a vector
 * in low memory indexed by the APIC number of the processor.
 * The layout, size, and location have to be kept in sync
 * with the handler code in l16sipi.s and offsets in mem.h.
 */
struct Sipi {
	u32int	pml4;
	u32int	args;		/* Sipireboot* for cpu 0 during reboot */
	uintptr	stack;
	Mach*	mach;
	uintptr	pc;
};

/* cpuid instruction result register bits */
enum {
	/* Procsig DX */
	Fpuonchip = 1<<0,
	Vmex	= 1<<1,		/* virtual-mode extensions */
	Psep	= 1<<3,		/* page size extensions */
	Tsc	= 1<<4,		/* time-stamp counter */
	Cpumsr	= 1<<5,		/* model-specific registers, rdmsr/wrmsr */
	Paep	= 1<<6,		/* physical-addr extensions */
	Mcep	= 1<<7,		/* machine-check exception */
	Cmpxchg8b = 1<<8,
	Cpuapic	= 1<<9,		/* have local apic */
	Mtrr	= 1<<12,	/* memory-type range regs. */
	Pgep	= 1<<13,	/* page global extension */
	Pse2	= 1<<17,	/* more page size extensions */
	Clflush = 1<<19,
	Mmx	= 1<<23,
	Fxsr	= 1<<24,	/* have SSE FXSAVE/FXRSTOR */
	Sse	= 1<<25,	/* thus sfence instr. */
	Sse2	= 1<<26,	/* thus mfence & lfence instr.s (2000-) */

	/* Procsig CX */
	Monitor	= 1<<3,		/* have monitor/mwait instr.s (2014-) */
	Eist	= 1<<7,		/* undocumented reserved magic */
	Hypervisor= 1<<31,	/* running on a hypervisor */

	/* Procmon CX */
	Pwrmgmt	= 1<<0,		/* mwait power mgmt. extensions */
	Alwaysbreak = 1<<1,	/* intrs always breaks mwait extension */

	/* Procid7 CX with input CX 0 */
	L5pt	= 1<<16,	/* supports 5-level page tables */

	/* Extfeature DX */
	Page1gb = 1<<26,	/* support for 1-GB pages */
	Nx	= 1<<20,	/* no-execute page protection */
};

enum mwaitext {
	Intrbreaks = 1,		/* intr. always break mwait, even at splhi */
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

enum {					/* bios cmos (nvram) registers */
	Cmosreset = 0xf,
};

enum {					/* Cmosreset values */
	Rstpwron = 0,			/* perform power-on reset */
	Rstnetboot= 4,			/* INT 19h reboot */
	Rstwarm	 = 0xa,	/* perform warm start: JMP via 32-bit ptr. at 0x467 */
};

enum {
	Wdogms = 200,			/* interval to restart watchdog */
};
