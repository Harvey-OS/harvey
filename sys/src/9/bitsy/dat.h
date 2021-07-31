typedef struct Cisdat 		Cisdat;
typedef struct Conf		Conf;
typedef struct Confmem	Confmem;
typedef struct FPU		FPU;
typedef struct FPenv		FPenv;
typedef struct FPsave		FPsave;
typedef struct DevConf		DevConf;
typedef struct Label		Label;
typedef struct Lock		Lock;
typedef struct MMU		MMU;
typedef struct Mach		Mach;
typedef struct Notsave		Notsave;
typedef struct Page		Page;
typedef struct PCMmap		PCMmap;
typedef struct PCMslot		PCMslot;
typedef struct PCMconftab	PCMconftab;
typedef struct PhysUart		PhysUart;
typedef struct PMMU		PMMU;
typedef struct Proc		Proc;
typedef struct Uart		Uart;
typedef struct Ureg		Ureg;
typedef struct Vctl		Vctl;
typedef long		Tval;

#pragma incomplete Ureg

typedef void IntrHandler(Ureg*, void*);

#define MAXSYSARG	5	/* for mount(fd, mpt, flag, arg, srv) */

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	(E_MAGIC)

struct Lock
{
	ulong	key;
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

/*
 * FPsave.status
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
};
struct	FPsave
{
	ulong	status;
	ulong	control;
	ulong	regs[8][3];	/* emulated fp */	
};

struct Confmem
{
	ulong	base;
	ulong	npage;
	ulong	limit;
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
	int	monitor;
	ulong	ialloc;		/* bytes available for interrupt time allocation */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
	ulong	hz;		/* processor cycle freq */
	ulong	mhz;
};

/*
 *  MMU stuff in proc
 */
enum
{
	NCOLOR=	1,	/* 1 level cache, don't worry about VCE's */
	Nmeg=	32,	/* maximum size of user space */
};

struct PMMU
{
	Page	*l1page[Nmeg];	/* this's process' level 1 entries */
	ulong	l1table[Nmeg];	/* ... */
	Page	*mmufree;	/* free mmu pages */
};

/*
 *  things saved in the Proc structure during a notify
 */
struct Notsave
{
	int dummy;
};

#include "../port/portdat.h"

struct Mach
{
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc of last caller to splhi */

	Proc	*proc;			/* current process */
	ulong	mmupid;			/* process id currently in mmu & cache */

	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	int	inclockintr;

	Proc*	readied;		/* for runproc */
	ulong	schedticks;	/* next forced context switch */

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

	int	flushmmu;		/* make current proc flush it's mmu state */
	Proc	*pid2proc[31];		/* what proc holds what pid */
	int	lastpid;		/* highest assigned pid slot */

	int	cpumhz;			/* speed of cpu */
	vlong	cpuhz;			/* ... */
	uvlong	cyclefreq;		/* Frequency of user readable cycle counter */

	/* save areas for exceptions */
	ulong	sfiq[5];
	ulong	sirq[5];
	ulong	sund[5];
	ulong	sabt[5];

	int	stack[1];
};

/*
 * Fake kmap since we direct map dram
 */
typedef void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)((p)->pa)
#define	kunmap(k)

struct
{
	Lock;
	int	machs;			/* bitmap of active CPUs */
	int	exiting;		/* shutdown */
	int	ispanic;		/* shutdown in response to a panic */
}active;

#define	MACHP(n)	((Mach *)(MACHADDR+(n)*BY2PG))

extern Mach	*m;
extern Proc	*up;

enum
{
	OneMeg=	1024*1024,
};

/*
 * PCMCIA structures known by both port/cis.c and the pcmcia driver
 */

/*
 * Map between ISA memory space and PCMCIA card memory space.
 */
struct PCMmap {
	ulong	ca;			/* card address */
	ulong	cea;			/* card end address */
	ulong	isa;			/* local virtual address */
	int	len;			/* length of the ISA area */
	int	attr;			/* attribute memory */
};

/*
 *  a PCMCIA configuration entry
 */
struct PCMconftab
{
	int	index;
	ushort	irqs;		/* legal irqs */
	uchar	irqtype;
	uchar	bit16;		/* true for 16 bit access */
	struct {
		ulong	start;
		ulong	len;
	} io[16];
	int	nio;
	uchar	vpp1;
	uchar	vpp2;
	uchar	memwait;
	ulong	maxwait;
	ulong	readywait;
	ulong	otherwait;
};

/*
 *  PCMCIA card slot
 */
struct PCMslot
{
	RWlock;

	Ref	ref;

	long	memlen;		/* memory length */
	uchar	slotno;		/* slot number */
	void	*regs;		/* i/o registers */
	void	*mem;		/* memory */
	void	*attr;		/* attribute memory */

	/* status */
	uchar	occupied;	/* card in the slot */
	uchar	configed;	/* card configured */
	uchar	inserted;	/* card just inserted */

	Dev		*dev;	/* set in ctlwrite `configure' */

	/* cis info */
	int	cisread;	/* set when the cis has been read */
	char	verstr[512];	/* version string */
	int	ncfg;		/* number of configurations */
	struct {
		ushort	cpresent;	/* config registers present */
		ulong	caddr;		/* relative address of config registers */
	} cfg[8];
	int	nctab;		/* number of config table entries */
	PCMconftab	ctab[8];
	PCMconftab	*def;		/* default conftab */

	/* maps are fixed */
	PCMmap memmap;
	PCMmap attrmap;
};

/*
 *  hardware info about a device
 */
typedef struct {
	ulong	port;	
	int	size;
} Devport;

struct DevConf
{
	RWlock;			/* write: configure/unconfigure/suspend; read: normal access */
	ulong	mem;		/* mapped memory address */
	Devport	*ports;		/* ports[0]: mapped i/o regs, access size */
	int	nports;		/* always 1 for the bitsy */
	int	itype;		/* type of interrupt */
	ulong	intnum;		/* interrupt number */
	char	*type;		/* card type, mallocated */
};

