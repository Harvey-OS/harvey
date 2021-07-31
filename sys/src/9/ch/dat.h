typedef struct Conf	Conf;
typedef struct FPsave	FPsave;
typedef struct KMap	KMap;
typedef struct Lance	Lance;
typedef struct Lancemem	Lancemem;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct MMU	MMU;
typedef struct Notsave	Notsave;
typedef struct PMMU	PMMU;
typedef struct Softtlb	Softtlb;
typedef struct Ureg	Ureg;
typedef struct Proc	Proc;
/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	V_MAGIC || magic==M_MAGIC

/*
 *  machine dependent definitions used by ../port/dat.h
 */

struct Lock
{
	ulong	key;			/* semaphore (non-zero = locked) */
	ulong	sr;
	ulong	pc;
	Proc	*p;
	ushort	isilock;
};

struct Label
{
	ulong	sp;
	ulong	pc;
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	ulong	npage0;		/* total physical pages of memory */
	ulong	npage1;		/* total physical pages of memory */
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	int	nswppo;		/* max # of pageouts per segment pass */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ipif;		/* Ip protocol interfaces */
	ulong	ip;		/* Ip conversations per interface */
	ulong	arp;		/* Arp table size */
	ulong	frag;		/* Ip fragment assemble queue size */
	ulong	debugger;	/* use processor 1 as a kernel debugger */
	ulong	ialloc;		/* bytes available for interrupt time allocation */
	ulong	pipeqsize;	/* size in bytes of pipe queues */
};

/*
 * floating point registers
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
};

struct	FPsave
{
	long	fpreg[32];
	long	fpstatus;
};

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
	ulong	svstatus;
};

#include "../port/portdat.h"

/* First FOUR members offsets known by l.s */
struct Mach
{
	/* the following are all known by l.s and cannot be moved */
	int	machno;			/* physical id of processor FIRST */
	Softtlb*stb;			/* Software tlb simulation SECOND */
	Proc*	proc;			/* process on this processor THIRD */
	ulong	splpc;			/* pc that called splhi() FOURTH */
	ulong	tlbfault;		/* # of tlb faults FIFTH */
	ulong	tlbpurge;		/* MUST BE SIXTH */

	/* the following is safe to move */
	ulong	ticks;			/* of the clock since boot time */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void*	alarm;			/* alarms bound to this clock */
	int	lastpid;		/* last pid allocated on this machine */
	Proc*	pidproc[NTLBPID];	/* proc that owns tlbpid on this mach */
	Page*	ufreeme;		/* address of upage of exited process */
	Ureg*	ur;
	KMap*	kactive;		/* active on this machine */
	int	knext;
	uchar	ktlbx[NTLB];		/* tlb index used for kmap */
	uchar	ktlbnext;
	int	speed;			/* cpu speed */
	ulong	delayloop;		/* for the delay() routine */
	int	nrdy;
	ulong	fairness;		/* for runproc */
	ulong	lastcyclecount;
	uvlong	fastclock;
	vlong	intrts;			/* time stamp of last interrupt */
	int	flushmmu;

	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	ledval;			/* value last written to LED */

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
#define PPN(x)		((x)>>6)

struct Softtlb
{
	ulong	virt;
	ulong	phys0;
	ulong	phys1;
};

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;

extern KMap kpte[];
extern register Mach	*m;
extern register Proc	*up;
