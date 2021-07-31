typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct KMap	KMap;
typedef struct Lance	Lance;
typedef struct Lancemem	Lancemem;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct MMU	MMU;
typedef struct Notsave	Notsave;
typedef struct Pcidev	Pcidev;
typedef struct PMMU	PMMU;
typedef struct Softtlb	Softtlb;
typedef struct Ureg	Ureg;
typedef struct Proc	Proc;
typedef uvlong		Tval;

#pragma incomplete Pcidev

#define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */

/*
 *  parameters for sysproc.c and rebootcmd.c
 */
#define AOUT_MAGIC	V_MAGIC || magic==M_MAGIC
/* r3k or r4k boot images */
#define BOOT_MAGIC	(0x160<<16) || magic == ((0x160<<16)|3)

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
	ushort	isilock;
};

struct Label
{
	ulong	sp;
	ulong	pc;
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
	Confmem	mem[1];
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ialloc;		/* bytes available for interrupt-time allocation */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
	int	nuart;		/* number of uart devices */
};

/*
 * floating point registers
 */
enum
{
	/* floating point state */
	FPinit,
	FPactive,
	FPinactive,
	FPemu,

	/* bit meaning floating point illegal */
	FPillegal= 0x100,
};

enum {
	Nfpregs		= 32,		/* floats; half as many doubles */
};

/*
 * emulated floating point (mips32r2 with ieee fp regs)
 * fpstate is separate, kept in Proc
 */
struct FPsave
{
	/* /dev/proc expects the registers to be first in FPsave */
	ulong	reg[Nfpregs];		/* the canonical bits */
	union {
		ulong	fpstatus;	/* both are fcr31 */
		ulong	fpcontrol;
	};

	int	fpdelayexec;		/* executing delay slot of branch */
	uintptr	fpdelaypc;		/* pc to resume at after */
	ulong	fpdelaysts;	/* save across user-mode delay-slot execution */

	/* stuck-fault detection */
	uintptr	fppc;			/* addr of last fault */
	int	fpcnt;			/* how many consecutive at that addr */
};

int fpemudebug;

/*
 *  mmu goo in the Proc structure
 */
struct PMMU
{
	int	pidonmach[MAXMACH];
};

/*
 *  things saved in the Proc structure during a notify
 */
struct Notsave
{
	ulong	nonempty;
};

#include "../port/portdat.h"

/* First FIVE members' offsets known by l.s */
struct Mach
{
	/* the following are all known by l.s and cannot be moved */
	int	machno;			/* physical id of processor FIRST */
	Softtlb*stb;			/* Software tlb simulation SECOND */
	Proc*	proc;			/* process on this processor THIRD */
	ulong	splpc;			/* pc that called splhi() FOURTH */
	ulong	tlbfault;		/* # of tlb faults FIFTH */
	ulong	ktlbfault;
	ulong	utlbfault;

	/* the following is safe to move */
	ulong	tlbpurge;
	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	void*	alarm;			/* alarms bound to this clock */
	int	lastpid;		/* last pid allocated on this machine */
	Proc*	pidproc[NTLBPID];	/* proc that owns tlbpid on this mach */
	KMap*	kactive;		/* active on this machine */
	int	knext;
	uchar	ktlbx[NTLB];		/* tlb index used for kmap */
	uchar	ktlbnext;
	int	speed;			/* cpu speed */
	ulong	delayloop;		/* for the delay() routine */
	ulong	fairness;		/* for runproc */
	int	flushmmu;
	int	inclockintr;
	int	ilockdepth;
	Perf	perf;			/* performance counters */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	/* for per-processor timers */
	ulong	lastcount;
	uvlong	fastticks;
	ulong	hz;
	ulong	maxperiod;
	ulong	minperiod;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;		/* next forced context switch */

	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	hashcoll;		/* soft-tlb hash collisions */
	int	paststartup;		/* for putktlb */

	int	stack[1];
};

struct KMap
{
	Ref;
	ulong	virt;
	ulong	phys0;
	ulong	phys1;
	KMap*	next;
	KMap*	konmach[MAXMACH];
	Page*	pg;
	ulong	pc;			/* of caller to kmap() */
};

#define	VA(k)		((k)->virt)
#define PPN(x)		((ulong)(x)>>6)

/* offsets known by l.s */
struct Softtlb
{
	ulong	virt;
	ulong	phys0;
	ulong	phys1;
};

struct
{
	Lock;
	long	machs;		/* bitmap of processors */
	short	exiting;
	int	ispanic;
}active;

extern KMap kpte[];
extern register Mach	*m;
extern register Proc	*up;

extern FPsave initfp;

extern	int normalprint;
