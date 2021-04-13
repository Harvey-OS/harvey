/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct ACVctl ACVctl;
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
typedef u64 Mpl;
typedef struct Page Page;
typedef struct Pcidev Pcidev;
typedef struct PAMap PAMap;
typedef struct PFPU PFPU;
typedef struct PmcCtr PmcCtr;
typedef struct PmcCtl PmcCtl;
typedef struct PmcWait PmcWait;
typedef struct PMMU PMMU;
typedef struct PNOTIFY PNOTIFY;
typedef u64 PTE;
typedef struct Proc Proc;
typedef struct Sys Sys;
typedef struct Stackframe Stackframe;
typedef u64 uintmem; /* Physical address (hideous) */
typedef struct Ureg Ureg;
typedef struct Vctl Vctl;

/*
 * Conversion for Ureg to gdb reg. This avoids a lot of nonsense
 * in the outside world.
 */
enum regnames {
	GDB_AX,	 /* 0 */
	GDB_BX,	 /* 1 */
	GDB_CX,	 /* 2 */
	GDB_DX,	 /* 3 */
	GDB_SI,	 /* 4 */
	GDB_DI,	 /* 5 */
	GDB_BP,	 /* 6 */
	GDB_SP,	 /* 7 */
	GDB_R8,	 /* 8 */
	GDB_R9,	 /* 9 */
	GDB_R10, /* 10 */
	GDB_R11, /* 11 */
	GDB_R12, /* 12 */
	GDB_R13, /* 13 */
	GDB_R14, /* 14 */
	GDB_R15, /* 15 */
	GDB_PC,	 /* 16 */
	GDB_PS,	 /* 17 */
	GDB_CS,	 /* 18 */
	GDB_SS,	 /* 19 */
	GDB_DS,	 /* 20 */
	GDB_ES,	 /* 21 */
	GDB_FS,	 /* 22 */
	GDB_GS,	 /* 23 */
};

#define DBG_MAX_REG_NUM 24
/* 17 64 bit regs and 5 32 bit regs */
#define GDB_NUMREGBYTES ((17 * 8) + (7 * 4))

#define MAXSYSARG 5 /* for mount(fd, afd, mpt, flag, arg) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC (S_MAGIC)
#define ELF_MAGIC (ELF_MAG)

/*
 *  machine dependent definitions used by ../port/portdat.h
 */

struct Lock {
	u32 key;
	int isilock;
	Mpl pl;
	usize _pc;
	Proc *p;
	Mach *m;
	u64 lockcycles;
};

struct Label {
	usize sp;
	usize pc;
	usize fp;
	usize _pad[13];
};

struct Fxsave {
	u16 fcw;		/* x87 control word */
	u16 fsw;		/* x87 status word */
	u8 ftw;		/* x87 tag word */
	u8 zero;		/* 0 */
	u16 fop;		/* last x87 opcode */
	u64 rip;		/* last x87 instruction pointer */
	u64 rdp;		/* last x87 data pointer */
	u32 mxcsr;		/* MMX control and status */
	u32 mxcsrmask;	/* supported MMX feature bits */
	unsigned char st[128];	/* shared 64-bit media and x87 regs */
	unsigned char xmm[256]; /* 128-bit media regs */
	unsigned char ign[96];	/* reserved, ignored */
} __attribute__((aligned(16)));

/*
 *  FPU stuff in Proc
 */
struct PFPU {
	Fxsave fxsave;
};

/*
 *  MMU stuff in Proc
 */
#define NCOLOR 1
struct PMMU {
	Page *root;
};

/*
 *  things saved in the Proc structure during a notify
 */
struct PNOTIFY {
	//	void	emptiness;
	char emptiness;
};

struct Confmem {
	usize base;
	usize npage;
	usize kbase;
	usize klimit;
};

struct Conf {
	u32 nproc;	   /* processes */
	Confmem mem[4];	   /* physical memory */
	u64 npage;	   /* total physical pages of memory */
	usize upages;	   /* user page pool */
	u32 copymode; /* 0 is copy on write, 1 is copy on reference */
	u32 ialloc;   /* max interrupt time allocation in bytes */
	u32 nimage;   /* number of page cache image headers */
};

enum {
	NPGSZ = 4 /* # of supported  pages sizes in Mach */
};

#include "../port/portdat.h"

/*
 *  CPU stuff in Mach.
 */
struct MCPU {
	u32 cpuinfo[3][4]; /*  CPUID Functions 0, 1, and 5 (n.b.: 2-4 are invalid) */
	int ncpuinfos;		/* number of standard entries */
	int ncpuinfoe;		/* number of extended entries */
	int isintelcpu;		/*  */
};

/*
 *  FPU stuff in Mach.
 */
struct MFPU {
	u16 fcw;	    /* x87 control word */
	u32 mxcsr;	    /* MMX control and status */
	u32 mxcsrmask; /* supported MMX feature bits */
};

struct NIX {
	ICC *icc; /* inter-core call */
	int nixtype;
};

/*
 *  MMU stuff in Mach.
 */
struct MMMU {
	usize cr2;
	Page *pml4;	 /* pml4 for this processor */
	Page pml4kludge; /* NIX KLUDGE: we need a page */
};

/*
 * Inter core calls
 */
enum {
	ICCLNSZ = 128, /* Cache line size for inter core calls */

	ICCOK = 0, /* Return codes: Ok; trap; syscall */
	ICCTRAP,
	ICCSYSCALL
};

struct ICC {
	/* fn is kept in its own cache line */
	alignas(ICCLNSZ) void (*fn)(void);
	int flushtlb;		     /* on the AC, before running fn */
	int rc;			     /* return code from AC to TC */
	char *note;		     /* to be posted in the TC after returning */
	unsigned char data[ICCLNSZ]; /* sent to the AC */
};

/*
 * hw perf counters
 */
struct PmcCtl {
	Ref r;
	u32 _coreno;
	int enab;
	int user;
	int os;
	int nodesc;
	char descstr[KNAMELEN];
	int reset;
};

struct PmcWait {
	Ref r;
	Rendez rend;
	PmcWait *next;
};

struct PmcCtr {
	int stale;
	PmcWait *wq;
	u64 ctr;
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
 */
struct Mach {
	/* WARNING! Known to assembly! */
	usize self; /* %gs:0 still gives us a Mach* */
	u64 splpc; /* pc of last caller to splhi */

	Proc *proc;	   /* current process on this processor */
	usize stack;   /* mach stack, kstack is in proc->kstack */
	usize rathole; /* to save a reg in syscallentry */
	Proc *externup;	   /* Forsyth recommends we replace the global up with this. */
	/* end warning, I think */

	int machno; /* physical id of processor */

	int apicno;
	int online;

	MMMU MMU;

	unsigned char *vsvm;
	void *gdt;
	void *tss;

	u64 ticks; /* of the clock since boot time */
	Label sched;	/* scheduler wakeup */
	Lock alarmlock; /* access to alarm list */
	void *alarm;	/* alarms bound to this clock */
	int inclockintr;

	Proc *readied;	     /* old runproc, only relevant if kernel booted with nosmp (-n append) */
	u64 schedticks; /* next forced context switch, same as above */
	u64 qstart;     /* time when up started running */
	int qexpired;	     /* quantum expired */

	int tlbfault;
	int tlbpurge;
	int pfault;
	int cs;
	int syscall;
	int intr;
	int mmuflush; /* make current proc flush it's mmu state */
	int ilockdepth;
	Perf perf;  /* performance counters */
	int inidle; /* profiling */
	int lastintr;

	Lock apictimerlock;
	u64 cyclefreq; /* Frequency of user readable cycle counter */
	i64 cpuhz;
	int cpumhz;
	u64 rdtsc;

	Lock pmclock;
	PmcCtr pmc[PmcMaxCtrs];

	MFPU FPU;
	MCPU CPU;

	NIX NIX;

	/* for restoring pre-AMP scheduler */
	Sched *sch;
	int load;
};
static_assert(sizeof(Mach) <= PGSZ, "Mach is too big");

struct Stackframe {
	Stackframe *next;
	usize pc;
};

/*
 * This is the low memory map, between 1MiB and 2MiB.
 *
 * It is located there to allow fundamental data structures to be
 * created and used before knowing where free memory begins
 * (e.g. there may be modules located after the kernel BSS end).
 * The layout is known in the bootstrap code in entry.S
 *
 * It is logically two parts: the per processor data structures
 * for the bootstrap processor (stack, Mach, vsvm, and page tables),
 * and the global information about the system (syspage, ptrpage).
 * Some of the elements must be aligned on page boundaries, hence
 * the unions.
 */
struct Sys {
	alignas(4096) unsigned char machstk[MACHSTKSZ];

	PTE ipml4[PTSZ / sizeof(PTE)];		 // Only used very early in boot
	PTE epml4[PTSZ / sizeof(PTE)];		 // Only used for ...
	PTE epml3[PTSZ / sizeof(PTE)];		 // ...BSP initialization...
	PTE epml2[PTSZ / sizeof(PTE)][4];	 // ...and AP early boot.
	PTE pml4[PTSZ / sizeof(PTE)];		 // Real PML4
	PTE pml3[((128 + 64) * PTSZ) / sizeof(PTE)];

	unsigned char vsvmpage[4 * KiB];

	alignas(4096) Mach mach;

	alignas(4096) Mach *machptr[MACHMAX];

	u64 pmstart; /* physical memory */
	u64 pmend;	  /* total span */

	u64 epoch; /* crude time synchronisation */

	int nc[NIXROLES]; /* number of online processors */
	int nmach;
	int load;
	u64 ticks; /* of the clock since boot time */

	u64 cyclefreq; /* Frequency of user readable cycle counter (mach 0) */

	u32 pgszlg2[NPGSZ];  /* per Mach or per Sys? */
	u32 pgszmask[NPGSZ]; /* Per sys -aki */
	u32 pgsz[NPGSZ];
	int npgsz;
};
static_assert(sizeof(Sys) <= (1 * MiB - 1 * KiB), "Sys is too big");

extern Sys *const sys;
#define MACHP(x) (sys->machptr[(x)])

/*
 * The Physical Address Map.  This describes the physical address
 * space layout of the machine, also taking into account where the
 * kernel is loaded, multiboot modules, etc.  Unused regions do not
 * appear in the map.
 */
#define PHYSADDRSIZE (1ULL << 46)

struct PAMap {
	u64 addr;
	usize size;
	int type;
	PAMap *next;
};

enum {
	PamNONE = 0,
	PamMEMORY,
	PamRESERVED,
	PamACPI,
	PamPRESERVE,
	PamUNUSABLE,
	PamDEV,
	PamMODULE,
	PamKTEXT,
	PamKRDONLY,
	PamKRDWR,
};

extern PAMap *pamap;

/*
 * KMap
 */
typedef void KMap;
extern KMap *kmap(Page *);

#define kunmap(k)
#define VA(k) PTR2UINT(k)

struct {
	Lock l;
	int nonline;	       /* # of active CPUs */
	int nbooting;	       /* # of CPUs waiting for the bTC to go */
	int exiting;	       /* shutdown */
	int ispanic;	       /* shutdown in response to a panic */
	int thunderbirdsarego; /* lets the added processors continue */
} active;

/*
 *  a parsed plan9.ini line
 */
#define NISAOPT 8

struct ISAConf {
	char *type;
	usize port;
	int irq;
	u32 dma;
	usize mem;
	usize size;
	u32 freq;

	int nopt;
	char *opt[NISAOPT];
};

/*
 * The Mach structures must be available via the per-processor
 * MMU information array machptr, mainly for disambiguation and access to
 * the clock which is only maintained by the bootstrap processor (0).
 */

extern char *rolename[];

/*
 * Horrid.
 */
// HARVEY: TODO: bring this back, it's actually nice. Or do something better.
// Talk to Ron before you condemn it.

#ifdef _DBGC_
#define DBGFLG (dbgflg[_DBGC_])
#else
#define DBGFLG (0)
#endif /* _DBGC_ */

#define DBG(...)                               \
	do {                                   \
		if(DBGFLG)                     \
			dbgprint(__VA_ARGS__); \
	} while(0)

extern char dbgflg[256];

#define dbgprint print /* for now */
