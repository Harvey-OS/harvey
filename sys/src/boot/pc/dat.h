typedef struct Conf	Conf;
typedef struct FPsave	FPsave;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct MMU	MMU;
typedef struct Mach	Mach;
typedef struct Page	Page;
typedef struct PMMU	PMMU;
typedef struct Segdesc	Segdesc;
typedef struct Ureg	Ureg;
typedef struct User	User;

#define	MACHP(n)	(n==0? &mach0 : *(Mach**)0)

extern	Mach	mach0;
extern  void	(*kprofp)(ulong);

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	I_MAGIC

struct Lock
{
	ulong	key;
	ulong	pc;
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

struct	FPsave	/* ??? needs to be fixed ??? */
{
	long	status;
	char	reg[66];
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	ulong	npgrp;		/* process groups */
	ulong	npage0;		/* total physical pages in bank0 */
	ulong	npage1;		/* total physical pages in bank1 */
	ulong	npage;		/* total physical pages of memory */
	ulong	nseg;		/* number of segments */
	ulong	nimage;		/* number of page cache image headers */
	ulong 	npagetab;	/* number of pte tables */
	ulong	nswap;		/* number of swap pages */
	ulong	nalarm;		/* alarms */
	ulong	nchan;		/* channels */
	ulong	nenv;		/* distinct environment values */
	ulong	nenvchar;	/* environment text storage */
	ulong	npgenv;		/* environment files per process group */
	ulong	nmtab;		/* mounted-upon channels per process group */
	ulong	nmount;		/* mounts */
	ulong	nmntdev;	/* mounted devices (devmnt.c) */
	ulong	nmntbuf;	/* buffers for devmnt.c messages */
	ulong	nmnthdr;	/* headers for devmnt.c messages */
	ulong	nstream;	/* streams */
	ulong	nqueue;		/* stream queues */
	ulong	nblock;		/* stream blocks */
	ulong	nsrv;		/* public servers (devsrv.c) */
	ulong	nbitmap;	/* bitmap structs (devbit.c) */
	ulong	nbitbyte;	/* bytes of bitmap data (devbit.c) */
	ulong	nfont;		/* font structs (devbit.c) */
	ulong	nnoifc;		/* number of nonet interfaces */
	ulong	nnoconv;	/* number of nonet conversations/ifc */
	ulong	nurp;		/* max urp conversations */
	ulong	nasync;		/* number of async protocol modules */
	ulong	npipe;		/* number of pipes */
	ulong	maxialloc;	/* maximum bytes used by ialloc */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ipif;		/* Ip protocol interfaces */
	ulong	ip;		/* Ip conversations per interface */
	ulong	arp;		/* Arp table size */
	ulong	frag;		/* Ip fragment assemble queue size */
	ulong	cntrlp;		/* panic on ^P */
	ulong	nfloppy;
	ulong	nhard;
};

/*
 *  MMU stuff in proc
 */
#define MAXMMU	4
#define MAXSMMU	1
struct PMMU
{
	int	mmuvalid;
	Page	*mmu[MAXMMU+MAXSMMU];	/* bottom level page tables */
	ulong	mmue[MAXMMU+MAXSMMU];	/* top level pointers to mmup pages */
};

#include "portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */

struct Mach
{
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc of last caller to splhi */
	int	mmask;			/* 1<<m->machno */
	ulong	ticks;			/* of the clock since boot time */
	Proc	*proc;			/* current process on this processor */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	fpstate;		/* state of fp registers on machine */

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	spinlock;
	int	intr;

	int	stack[1];
};

/*
 * Fake kmap
 */
typedef void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kunmap(k)

#define	NERR	15
#define	NNOTE	5
struct User
{
	Proc	*p;
	int	nerrlab;
	Label	errlab[NERR];
	char	error[ERRLEN];
	FPsave	fpsave;			/* address of this is known by vdb */
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
	ushort	svvo;
	ushort	svsr;
};

/*
 *  segment descriptor/gate
 */
struct Segdesc
{
	ulong	d0;
	ulong	d1;
};


struct
{
	Lock;
	short	machs;
	short	exiting;
}active;

extern Mach	*m;
extern User	*u;

extern int	flipD[];
extern int	have9part;

/*
 *  bootline passed by boot program
 */
#define BOOTLINE ((char *)0x80000100)
