typedef struct Conf	Conf;
typedef struct FPsave	FPsave;
typedef struct KMap	KMap;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct PMMU	PMMU;
typedef struct Softtlb	Softtlb;
typedef struct Ureg	Ureg;
typedef struct User	User;

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	V_MAGIC
#define ENTRYOFFSET	(-4)

/*
 *  machine dependent definitions used by ../port/dat.h
 */

struct Lock
{
	ulong	key;			/* semaphore (non-zero = locked) */
	ulong	pc;
	void	*upa;
	ulong	sr;
};

struct Label
{
	ulong	sp;
	ulong	pc;
};

/*
 * FPsave.fpstatus
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

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	ulong	monitor;	/* has display? */
	ulong	npage0;		/* total physical pages of memory */
	ulong	npage1;		/* total physical pages of memory */
	ulong	npage;		/* total physical pages of memory */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ipif;		/* Ip protocol interfaces */
	ulong	ip;		/* Ip conversations per interface */
	ulong	arp;		/* Arp table size */
	ulong	frag;		/* Ip fragment assemble queue size */
};

/*
 *  mmu goo in the Proc structure
 */
struct PMMU
{
	int	pidonmach[MAXMACH];
};

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */

struct KMap
{
	ulong	virt;
	ulong	phys0;
	ulong	phys1;
	KMap *	next;
};

#define	VA(k)		((k)->virt)
#define PPN(x)		((x)>>6)

struct Softtlb
{
	ulong	virt;
	ulong	phys0;
	ulong	phys1;
};

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor */
	Softtlb *stb;			/* Software tlb simulation */
	KMap *	ktb;			/* KMap */
	ulong	splpc;			/* pc that called splhi() */
	int	tlbfault;
	int	tlbpurge;

	/* ordering from here on irrelevant */
	ulong	ticks;			/* of the clock since boot time */
	Proc	*proc;			/* current process on this processor */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	char	pidhere[NTLBPID];	/* is this pid possibly in this mmu? */
	int	lastpid;		/* last pid allocated on this machine */
	Proc	*pidproc[NTLBPID];	/* process that owns this tlbpid on this mach */
	Page	*ufreeme;		/* address of upage of exited process */

	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	int	nettime;

	/* MUST BE LAST */
	int	stack[1];
};

#define	NERR	20
#define	NNOTE	5
struct User
{
	Proc	*p;
	FPsave	fpsave;			/* address of this is known by db */
	int	scallnr;		/* sys call number - known by db */
	Sargs	s;			/* address of this is known by db */
	Label	errlab[NERR];
	int	nerrlab;
	char	error[ERRLEN];
	char	elem[NAMELEN];		/* last name element from namec */
	Chan	*slash;
	Chan	*dot;
	/*
	 * Rest of structure controlled by devproc.c and friends.
	 * lock(&p->debug) to modify.
	 */
	Note	note[NNOTE];
	short	nnote;
	short	notified;		/* sysnoted is due */
	Note	lastnote;
	int	(*notify)(void*, char*);
	void	*ureg;
	void	*dbgreg;		/* User registers for debugging in proc */
};

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;
#define	MACHP(n)	((Mach *)(MACHADDR+n*BY2PG))

extern char		sysname[];
extern register	Mach	*m;
extern register	User	*u;

extern KMap ktlb[];

/*
 * codes for reversed screen bitblts
 */
extern	int	flipS[];
extern	int	flipD[];
