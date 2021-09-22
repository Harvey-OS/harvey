typedef struct Asm Asm;
typedef struct Clint Clint;
typedef struct Ioconf Ioconf;
typedef struct ISAConf ISAConf;
typedef struct Kmesg Kmesg;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct Mach Mach;
typedef struct MCPU MCPU;
typedef struct Membank Membank;
typedef struct MFPU MFPU;
typedef struct MMMU MMMU;
typedef uvlong Mpl;
typedef uvlong Mreg;
typedef struct Msi Msi;
typedef struct Page Page;
typedef struct Pcidev Pcidev;
typedef struct PFPU PFPU;
typedef struct Plic Plic;
typedef struct PMMU PMMU;
typedef struct PNOTIFY PNOTIFY;
typedef struct Proc Proc;
typedef uvlong PTE;
typedef struct Reboot Reboot;
typedef struct Sbiret Sbiret;
typedef struct Soc Soc;
typedef struct Sys Sys;
typedef struct Syscomm Syscomm;
typedef uintptr uintmem;		/* physical address.  horrible name */
typedef struct Ureg Ureg;
typedef struct Vctl Vctl;
typedef struct Wdog Wdog;

#pragma incomplete Ureg

#define SYSCALLTYPE	8	/* nominal syscall trap type (not used) */
#define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */

#define INTRSVCVOID
#define Intrsvcret void

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	Y_MAGIC

/*
 *  machine dependent definitions used by ../port/portdat.h
 */
struct Lock
{
	int	key;
	ulong	noninit;		/* must be zero */
	union {
//		Mpl	pl;
		Mreg	sr;
	};
	uintptr	pc;
	Proc*	p;
	Mach*	m;
	char	isilock;
};

struct Label
{
	uintptr	sp;
	uintptr	pc;
};

/*
 *  FPU stuff in Proc
 */
struct PFPU {
	int	fpustate;		/* per-process logical state */
	int	fcsr;
	double	fpusave[32];
	int	fptraps;		/* debugging count */
};

/*
 *  MMU stuff in Proc
 */
#define NCOLOR 8
struct PMMU
{
	Page*	mmuptp[NPGSZ];		/* page table pages for each level */
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

#include "../port/portdat.h"

/*
 *  CPU stuff in Mach.
 */
struct MCPU {
	char	empty;
};

/*
 *  per-cpu physical FPU state in Mach.
 */
struct MFPU {
	int	turnedfpoff;		/* so propagate to Ureg */
	uvlong	fpsaves;		/* counters */
	uvlong	fprestores;
};

/*
 *  MMU stuff in Mach.
 */
struct MMMU
{
	Page*	ptroot;			/* ptroot for this processor */
	Page	ptrootpriv;		/* we need a page */
//	PTE*	pmap;			/* unused as of yet */

	/* page characteristics are actually per system */
	int	npgsz;	/* # sizes in use; also # of levels in page tables */
	uint	pgszlg2[NPGSZ];
	uintmem	pgszmask[NPGSZ];
};

/*
 * Per processor information.
 *
 * The offsets of the first few elements may be known
 * to low-level assembly code, so do not re-order:
 *	machno, machmode - rebootcode
 *	splpc		- splhi, spllo, splx
 *	proc, regsave	- strap, mtrap
 *	online, hartid	- start
 */
struct Mach
{
	int	machno;			/* 0 logical id of processor */
	uintptr	splpc;			/* 8 pc of last caller to splhi */
	Proc*	proc;			/* 16 current process on this cpu */
	uintptr	regsave[2*5]; /* 24 (super mach)×(r2 r3 r4 r6 r9) saved @ trap entry */
	int	online;			/* 104 flag: actually enabled */
	int	hartid;			/* 108 riscv cpu id; not always machno */
	uchar	bootmachmode;		/* 112 flag: machine mode at boot time */

	uintptr	stack;			/* non-syscall stack base */
	int	plicctxt;		/* plic context: f(hartid, priv) */
	int	waiting;		/* flag: waiting for m->online */
	ulong	ipiwait;		/* flag: in wfi, waiting for ipi */

	MMMU;

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
//	Lock	alarmlock;		/* access to alarm list */
//	void*	alarm;			/* alarms bound to this clock */
//	int	inclockintr;
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
	uchar	virtdevaddrs;		/* flag: use virtual device addresses */
	uchar	probing;		/* flag: probing an address */
	uchar	probebad;		/* flag: probe failed: bad address */
	uchar	clockintrsok;		/* safe to call timerintr */
	int	clockintrdepth;

	vlong	cpuhz;	/* also frequency of user readable cycle counter */
	int	cpumhz;
//	u64int	rdtsc;			/* tsc value at boot */
	uvlong	boottsc;		/* my tsc's value at boot */

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
 * Sys is system-wide (not per-cpu) data at a fixed address.
 * It is located there to allow fundamental datastructures to be
 * created and used before knowing where free memory begins
 * (e.g. there may be modules located after the kernel BSS end).
 *
 * The layout is known in the bootstrap code in start.s, mmu.c, and mem.h
 * via the LOW0* definitions.
 * It is logically two parts: the per processor data structures
 * for the bootstrap processor (stack, Mach, and page tables),
 * and the global information about the system (syspage, ptrpage).
 * Some of the elements must be aligned on page boundaries, hence
 * the unions.
 */

/*
 * parameters to reboot trampoline code; layout is known to riscv64.h, mtrap.s
 * and rebootcode.s
 */
struct Reboot {
	void	(*tramp)(Reboot *);	/* tramp pa */
	uintptr	phyentry;
	uintptr	phycode;
	uintptr	rebootsize;
	uintptr	secstall;		/* flag: secondaries loop while set */
	uintptr	satp;			/* initial satp contents for sec's */
};

/*
 * portion common to all cpus; cpus other than 0 allocate their own in
 * newcpupages(); that code assumes ptroot follows machstk and that the
 * address of either can be derived from the other, so keep them together and
 * in order.
 */
struct Syscomm {
	uchar	machstk[MACHSTKSZ];		/* must be multiple of 4K */
	PTE	pteroot[PTSZ/sizeof(PTE)];	/* initial page tables */
	union {
		Mach	mach;
		uchar	machpage[MACHSZ];
	};
};

/*
 * Sys lives at the top of the first memory bank minus 1MB.
 * sizeof(Sys) is 616K currently.
 */
struct Sys {
	Syscomm;
	/* rest is statically allocated once for cpu0 to share with all cpus */

	uchar reboottramp[PGSZ];	/* copy reboot trampoline here */
	union {
		uchar	syspage[PGSZ];	/* do not zero */
		struct {
			Reboot;		/* location known to assembler */

			uintptr	pmbase;		/* start of physical memory */
			uintptr	pmstart;	/* available physical memory */
			uintptr	pmoccupied;	/* how much is occupied */
			/* pmend is little used in rv; see memory.c */
			uintptr	pmend;		/* total span, including gaps */

			uintptr	vmstart;	/* base address for malloc */
			uintptr	vmunused;	/* 1st unused va   " " */
			uintptr	vmunmapped;	/* 1st unmapped va " " */
			uintptr	vmend;		/* 1st unusable va " " */

			PTE	*freepage;	/* next free PT in pts */

			uvlong	timesync;	/* crude time synchronisation */
			/* computed constants to avoid mult. & div. */
			uvlong	clintsperµs;
			uvlong	clintsperhz;	/* clint ticks per HZ */
			uvlong	minclints;	/* min. interval until intr */
			uvlong	tscperclint;	/* cycles per clint tick */
			uvlong	tscperhz;
			uvlong	tscperminclints;
			uint	ticks;		/* since boot (type?) */

//			int	nmach;		/* how many machs */
			int	nonline;	/* how many machs are online */
			uint	copymode;	/* 0 is COW, 1 is copy on ref */

			Lock	kmsglock;	/* manually zero this lock */
		};
	};

	union {
		Mach*	machptr[MACHMAX];
		uchar	ptrpage[PGSZ];
	};

	PTE	waspd2g[PTSZ/sizeof(PTE)];
	PTE	waspt2g[PTSZ/sizeof(PTE)];
	uchar	pts[PTSZ * EARLYPAGES];	/* allocated via freepage in pdmap */

	union {
		uchar	sysextend[SYSEXTEND];	/* includes room for growth */
		struct {
			uchar	gap[SYSEXTEND - sizeof(Kmesg)];
			/* must be last so we can avoid zeroing it at start */
			Kmesg	kmsg;		/* do not zero */
		};
	};
};
CTASSERT(sizeof(Syscomm) == LOW0SYS, Syscomm_c_as_offsets_differ);
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
extern register Mach* m;			/* R7 */
extern register Proc* up;			/* R6 */

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

#pragma	varargck	type	"L"	Mpl
#pragma	varargck	type	"P"	uintmem
#pragma	varargck	type	"R"	Mreg

/* acpi/multiboot memory types */
enum {
	AsmNONE		= 0,
	AsmMEMORY	= 1,
	AsmRESERVED	= 2,
	AsmDEV		= 5,		/* device registers */
};

enum {
	Wdogms = 200,			/* interval to restart watchdog */
};

/* console for tinyemu */
typedef struct Htif Htif;
struct Htif {
	ulong	tohost[2];
	ulong	fromhost[2];
};

struct Soc {	/* physical addresses, later vmapped to virtual addresses */
	/* interrupt controllers */
	uchar	*clint;
	uchar	*plic;

	/* soc timers */
	uchar	*timer;
	uchar	*rtc;
	uchar	*wdog0;

	/* consoles */
	uchar	*uart;
	uchar	*htif;		/* simple debugging text console (tinyemu) */

	/* other peripherals */
	uchar	*ether[2];
	uchar	*pci;		/* ecam config space */
	uchar	*pcictl;	/* bridge & ctrl regs */
	uchar	*pciess;	/* soft reset, etc. */
	uchar	*sdmmc;
	uchar	*sdiosel;

	uchar	*l2cache;
};

Soc soc;

int	bootmachmode;	/* machine mode at boot time; same on all harts? */
int	havesbihsm;
/*
 * plic contexts are machine-dependent, but are usually hartid*2 + mode.
 * if there's a cut-down management hart at hartid 0 (i.e., it's hobbled),
 * they may instead be 1 + 2*(hartid-1) + mode, as on the icicle.
 */
int	hobbled;	/* flag: hart0 is a puny boot cpu with only 1 context */
uintptr	htifpwrregs;
uintptr	htifregs;
int	idlepause;	/* flag: use pause instead of wfi to idle */
uintptr	memtotal;	/* sum of all banks */
int	mideleg;	/* stuff smuggled in from machine mode */
uintptr	misa;
int	nosbi;
int	nuart;		/* number of uarts in use in uartregs */
uvlong	pagingmode;
uintptr	phyuart;
uvlong	sysepoch;	/* clint->mtime at boot */
uvlong	timebase;
uintptr	uartregs[];	/* not yet used for anything significant */
vlong	uartfreq;

void	(*rvreset)(void);

/* these are register layouts, so must not have bogus padding inserted */

/*
 * CLINT is Core-Local INTerrupt controller
 */

/* Clint arrays are indexed by hart (cpu) id */
struct Clint {
	ulong	msip[4096];		/* non-zero lsb generates sw intr. */
	uvlong	mtimecmp[4095];		/* < mtime generates clock intr. */
	uvlong	mtime;
};

/*
 * PLIC is Platform-Level Interrupt Controller.
 * Any interrupt connected here also shows up as Seie in SIP CSR.
 */

enum {
	Ncontexts = 31*512,
};
enum Cpumodes {
	Machine, Super,		/* order matters, see plic contexts */
	Nctxtmodes,
};

struct Plic {				/* 64 MB */
	ulong	prio[1024];		/* index by intr source */
	ulong	pendbits[1024];		/* 1st 32: bitmap of sources */

	union {
		uchar	_1_[0x200000-0x2000];
		/* index by context, then 1st 6 are bitmap of sources */
		ulong	enabits[Ncontexts][32];
	};
	struct Plictxt {
		ulong	priothresh;
		ulong	claimcompl;	/* claim/complete */
		uchar	_2_[4096 - 2*sizeof(ulong)];
	} context[Ncontexts];		/* index by context */
};

struct Wdog {			/* icicle */
	ulong	refresh;
	ulong	ctl;
	ulong	sts;
	ulong	time;
	ulong	msvp;	/* must be < time */
			/* MVRP: max. value up to which refresh is permitted */
	ulong	trigger;
	ulong	force;
};

struct Ioconf {
	char	*name;		/* e.g., "ether" */
	uintptr	regssize;	/* e.g., PGSZ */
	uchar	**socaddr;	/* soc.ether array; va(s) stored here */
	ushort	irq;		/* 0 means none */
	ushort	baseunit;	/* e.g., 0 */
	ushort	consec;		/* e.g., 2; exception: 0 means 1 */
	uintptr	ioregs;		/* e.g., PAMac0 */
};

struct Membank {
	uintptr	addr;
	uintptr	size;
};
extern Membank membanks[];

struct Sbiret {
	uvlong	status;
	uvlong	error;
};
