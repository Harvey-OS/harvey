typedef struct Conf	Conf;
typedef struct Confmem	Confmem;
typedef struct FPsave	FPsave;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct Notsave	Notsave;
typedef struct Page	Page;
typedef struct PCArch	PCArch;
typedef struct PCB	PCB;
typedef struct Pcidev	Pcidev;
typedef struct PMMU	PMMU;
typedef struct Proc	Proc;
typedef struct Sys	Sys;
typedef struct Ureg	Ureg;
typedef struct Vctl	Vctl;
typedef vlong		Tval;

#define MAXSYSARG	6	/* for mount(fd, mpt, flag, arg, srv) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	L_MAGIC

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
	ulong	pid;
	ushort	isilock;
};

struct Label
{
	ulong	sp;
	ulong	pc;
};

/*
 * Proc.fpstate
 */
enum
{
	/* floating point states */
	FPinit,
	FPactive,
	FPinactive,

	/* bit or'd with the state */
	FPillegal= 0x100,
};

struct	FPsave
{
	long	fpreg[2*32];
	long	dummy;		/* lower bits of FPCR, useless */
	long	fpstatus;
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
	Confmem	mem[2];
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	monitor;		/* has display? */
	ulong	ialloc;		/* bytes available for interrupt time allocation */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
};

/*
 *  mmu goo in the Proc structure
 */
struct PMMU
{
	Page	*mmutop;	/* 1st level table */
	Page	*mmulvl2;	/* 2nd level table */
	Page	*mmufree;	/* unused page table pages */
	Page	*mmuused;	/* used page table pages, except for mmustk */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct Notsave
{
	ulong	UNUSED;
};

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */
/*
 * Fake kmap
 */
typedef	void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kunmap(k)

/*
 *	Process Control Block, used by PALcode
 */
struct PCB {
	uvlong	ksp;
	uvlong	usp;
	uvlong	ptbr;
	ulong	asn;
	ulong	pcc;
	uvlong	unique;
	ulong	fen;
	ulong	dummy;
	uvlong	rsrv1;
	uvlong	rsrv2;
};

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc that called splhi() */
	Proc	*proc;			/* current process on this processor */

	/* ordering from here on irrelevant */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	inclockintr;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;	/* next forced context switch */

	vlong	cpuhz;			/* hwrpb->cfreq */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */
	ulong	pcclast;
	uvlong	fastclock;
	Perf	perf;			/* performance counters */

	int	tlbfault;		/* only used by devproc; no access to tlb */
	int	tlbpurge;		/* ... */
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	flushmmu;		/* make current proc flush it's mmu state */
	int	ilockdepth;

	ulong	spuriousintr;
	int	lastintr;

	PCB;

	/* MUST BE LAST */
	int	stack[1];
};

struct
{
	Lock;
	short	machs;
	short	exiting;
	short	ispanic;
}active;

/*
 *	Implementation-dependant functions (outside of Alpha architecture proper).
 *	Called PCArch because that's what mkdevc calls it (for the PC).
 */
struct PCArch
{
	char*	id;
	int	(*ident)(void);

	void	(*coreinit)(void);		/* set up core logic, PCI mappings etc */
	void	(*corehello)(void);		/* identify core logic to user */
	void	(*coredetach)(void);		/* restore core logic before return to console */
	void	*(*pcicfg)(int, int);		/* map and point to PCI cfg space */
	void	*(*pcimem)(int, int);		/* map and point to PCI memory space */
	int	(*intrenable)(Vctl*);
	int	(*intrvecno)(int);
	int	(*intrdisable)(int);

	int	(*_inb)(int);
	ushort	(*_ins)(int);
	ulong	(*_inl)(int);
	void	(*_outb)(int, int);
	void	(*_outs)(int, ushort);
	void	(*_outl)(int, ulong);
	void	(*_insb)(int, void*, int);
	void	(*_inss)(int, void*, int);
	void	(*_insl)(int, void*, int);
	void	(*_outsb)(int, void*, int);
	void	(*_outss)(int, void*, int);
	void	(*_outsl)(int, void*, int);
};

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

extern PCArch	*arch;

#define	MACHP(n)	((Mach *)((int)&mach0+n*BY2PG))
extern Mach		mach0;

extern register Mach	*m;
extern register Proc	*up;

/*
 *  hardware info about a device
 */
typedef struct {
	ulong	port;	
	int	size;
} Devport;

struct DevConf
{
	ulong	intnum;	/* interrupt number */
	char	*type;	/* card type, malloced */
	int	nports;	/* Number of ports */
	Devport	*ports;	/* The ports themselves */
};

extern FPsave initfp;
