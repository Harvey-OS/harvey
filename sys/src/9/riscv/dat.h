/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Conf Conf;
typedef struct Confmem Confmem;
typedef struct Fxsave Fxsave;
typedef struct ICC ICC;
typedef struct ICCparms ICCparms;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct MCPU MCPU;
typedef struct MFPU MFPU;
typedef struct MMMU MMMU;
typedef struct NIX NIX;
typedef struct Mach Mach;
typedef uint64_t Mpl;
typedef struct Page Page;
typedef struct PFPU PFPU;
typedef struct PmcCtr PmcCtr;
typedef struct PmcCtl PmcCtl;
typedef struct PmcWait PmcWait;
typedef struct PMMU PMMU;
typedef struct PNOTIFY PNOTIFY;
typedef uint64_t PTE;
typedef struct Proc Proc;
typedef struct Sys Sys;
typedef struct Stackframe Stackframe;
typedef uint64_t uintmem;				/* Physical address (hideous) */
typedef struct Ureg Ureg;
typedef struct Vctl Vctl;

/*
 * Conversion for Ureg to gdb reg. This avoids a lot of nonsense
 * in the outside world. TODO. 
 */
enum regnames {
	GDB_IP,
};
#define DBG_MAX_REG_NUM		(0)
#define GDB_NUMREGBYTES		(0)

#define MAXSYSARG	6	/* for mount(fd, afd, mpt, flag, arg, mntname) */

/*
 *  parameters for sysproc.c
 */
#define ELF_MAGIC	(ELF_MAG)

/*
 *  machine dependent definitions used by ../port/portdat.h
 */


/* Crap. */
struct ISAConf {
	int _;
};

/* End Crap. */

struct Lock
{
	uint32_t	key;
	int	isilock;
	Mpl	pl;
	uintptr_t	_pc;
	Proc*	p;
	Mach*	m;
	uint64_t	lockcycles;
};

struct Label
{
	/* order known to assembly */
	uintptr_t	sp;
	uintptr_t	pc;
	uintptr_t       fp;
	uintptr_t	x[32];
};

struct Fxsave {
	int _;
};

/*
 *  FPU stuff in Proc
 */
struct PFPU {
	int	fpustate;
	unsigned char	fxsave[sizeof(Fxsave)+15];
	void*	fpusave;
};

/*
 *  MMU stuff in Proc
 */
#define NCOLOR 1
struct PMMU
{
	Page*	mmuptp[4];		/* page table pages for each level */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct PNOTIFY
{
//	void	emptiness;
	char	emptiness;
};

struct Confmem
{
	uintptr_t	base;
	usize	npage;
	uintptr_t	kbase;
	uintptr_t	klimit;
};

struct Conf
{
	uint32_t	nproc;		/* processes */
	Confmem	mem[4];		/* physical memory */
	uint64_t	npage;		/* total physical pages of memory */
	usize	upages;		/* user page pool */
	uint32_t	copymode;	/* 0 is copy on write, 1 is copy on reference */
	uint32_t	ialloc;		/* max interrupt time allocation in bytes */
	uint32_t	nimage;		/* number of page cache image headers */
};

enum
{
	NPGSZ = 4	/* # of supported  pages sizes in Mach */
};

#include "../port/portdat.h"

/*
 *  CPU stuff in Mach.
 */
struct MCPU
{
	int _;
};

/*
 *  FPU stuff in Mach.
 */
struct MFPU
{
	int _;
};

struct NIX
{
	ICC*	icc;			/* inter-core call */
	int	nixtype;
};

/*
 *  MMU stuff in Mach.
 */
struct MMMU
{
	uintptr_t badaddr;
	Page*   root;			/* root for this processor */
	PTE*	pmap;			/* unused as of yet */

	Page	nixkludge;		/* NIX KLUDGE: we need a page */
};

/*
 * Inter core calls
 */
enum
{
	ICCLNSZ =	128,	/* Cache line size for inter core calls */


	ICCOK = 0,		/* Return codes: Ok; trap; syscall */
	ICCTRAP,
	ICCSYSCALL
};

struct ICC
{
	/* fn is kept in its own cache line */
	union{
		void	(*fn)(void);
		unsigned char	_ln1_[ICCLNSZ];
	};
	int	flushtlb;	/* on the AC, before running fn */
	int	rc;		/* return code from AC to TC */
	char*	note;		/* to be posted in the TC after returning */
	unsigned char	data[ICCLNSZ];	/* sent to the AC */
};

/*
 * hw perf counters
 */
struct PmcCtl {
	Ref r;
	uint32_t _coreno;
	int enab;
	int user;
	int os;
	int nodesc;
	char descstr[KNAMELEN];
	int reset;
};

struct PmcWait{
	Ref r;
	Rendez rend;
	PmcWait*	next;
};

struct PmcCtr{
	int stale;
	PmcWait *wq;
	uint64_t ctr;
	int ctrset;
	PmcCtl PmcCtl;
	int ctlset;
};

enum {
	PmcMaxCtrs = 4,
	PmcIgn = 0,
	PmcGet = 1,
	PmcSet = 2,
};

/*
 * Per processor information.
 *
 * The offsets of the first few elements may be known
 * to low-level assembly code, so do not re-order:
 *	self	- machp()
 *	splpc	- splhi, spllo, splx
 *	proc	- syscallentry
 *	stack	- acsyscall
 *	externup - externup()
 *
 * riscv uses tp for mach.
 */
struct Mach
{
	/* WARNING! Known to assembly! */
	uintptr_t	self;			/* %gs:0 still gives us a Mach* */
	uint64_t	splpc;			/* pc of last caller to splhi */

	Proc*	proc;			/* current process on this processor */
	uintptr_t	stack;		/* mach stack, kstack is in proc->kstack */
	uintptr_t	rathole;	/* to save a reg in syscallentry */
	Proc*	externup;		/* Forsyth recommends we replace the global up with this. */
	/* end warning, I think */

	int	machno;			/* physical id of processor */

	int	online;

	MMMU MMU;

	uint64_t	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	int	inclockintr;

	Proc*	readied;		/* old runproc, only relevant if kernel booted with nosmp (-n append) */
	uint64_t   schedticks;		/* next forced context switch, same as above */
	uint64_t	qstart;			/* time when up started running */
	int	qexpired;		/* quantum expired */

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	intr;
	int	mmuflush;		/* make current proc flush it's mmu state */
	int	ilockdepth;
	Perf	perf;			/* performance counters */
	int	inidle;			/* profiling */
	int	lastintr;

	uint64_t	cyclefreq;		/* Frequency of user readable cycle counter */
	int64_t	cpuhz;
	int	cpumhz;
	uint64_t	rdtsc;

	Lock	pmclock;
	PmcCtr	pmc[PmcMaxCtrs];

	MFPU FPU;
	MCPU CPU;

	NIX NIX;

	/* for restoring pre-AMP scheduler */
	Sched *sch;
	int load;
};

struct Stackframe
{
	Stackframe *next;
	uintptr_t pc;
};

/*
 * firmware sets up the virtual memory of the kernel. What to do after that? I don't know.
 */
struct Sys {
	unsigned char	machstk[MACHSTKSZ];

	Page    root;

	union {
		Mach	mach;
		unsigned char	machpage[MACHSZ];
	};

	union {
		struct {
			uint64_t	pmstart;	/* physical memory */
			uint64_t	pmoccupied;	/* how much is occupied */
			uint64_t	pmend;		/* total span */

			uintptr_t	vmstart;	/* base address for malloc */
			uintptr_t	vmunused;	/* 1st unused va */
			uintptr_t	vmunmapped;	/* 1st unmapped va */
			uintptr_t	vmend;		/* 1st unusable va */
			uint64_t	epoch;		/* crude time synchronisation */

			int		nc[NIXROLES];		/* number of online processors */
			int		nmach;
			int		load;
			uint64_t	ticks;			/* of the clock since boot time */
		};
		unsigned char	syspage[4*KiB];
	};

	union {
		Mach*	machptr[MACHMAX];
		unsigned char	ptrpage[4*KiB];
	};

	uint64_t	cyclefreq;		/* Frequency of user readable cycle counter (mach 0) */

	uint	pgszlg2[NPGSZ];		/* per Mach or per Sys? */
	uint	pgszmask[NPGSZ];	/* Per sys -aki */
	uint	pgsz[NPGSZ];
	int	npgsz;

	unsigned char	_57344_[2][4*KiB];		/* unused */
};

extern Sys *sys;
#define MACHP(x) (sys->machptr[(x)])
/*
 * KMap
 */
typedef void KMap;
extern KMap* kmap(Page*);

#define kunmap(k)
#define VA(k)		PTR2UINT(k)

struct
{
	Lock l;
	int	nonline;			/* # of active CPUs */
	int nbooting;			/* # of CPUs waiting for the bTC to go */
	int	exiting;			/* shutdown */
	int	ispanic;			/* shutdown in response to a panic */
	int	thunderbirdsarego;	/* lets the added processors continue */
}active;

/*
 * The Mach structures must be available via the per-processor
 * MMU information array machptr, mainly for disambiguation and access to
 * the clock which is only maintained by the bootstrap processor (0).
 */

extern uintptr_t kseg0;

extern char*rolename[];
extern void *kseg2;



/*
 * Horrid.
 */
// HARVEY: TODO: bring this back, it's actually nice. Or do something better.
// Talk to Ron before you condemn it.

#ifdef _DBGC_
#define DBGFLG		(dbgflg[_DBGC_])
#else
#define DBGFLG		(0)
#endif /* _DBGC_ */

#define DBG(...)	if(!DBGFLG){}else dbgprint(__VA_ARGS__)

extern char dbgflg[256];

#define dbgprint	print		/* for now */

