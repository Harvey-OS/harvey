typedef struct Conf	Conf;
typedef struct FPsave	FPsave;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct MMU	MMU;
typedef struct Mach	Mach;
typedef struct PCArch	PCArch;
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

struct	FPsave
{
	ushort	control;
	ushort	r1;
	ushort	status;
	ushort	r2;
	ushort	tag;
	ushort	r3;
	ulong	pc;
	ushort	selector;
	ushort	r4;
	ulong	operand;
	ushort	oselector;
	ushort	r5;
	uchar	regs[80];	/* floating point registers */
};

struct Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	ulong	monitor;	/* has monitor? */
	ulong	npage0;		/* total physical pages of memory */
	ulong	npage1;		/* total physical pages of memory */
	ulong	topofmem;	/* highest physical address + 1 */
	ulong	npage;		/* total physical pages of memory */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap pages */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	arp;		/* Arp table size */
	ulong	nfloppy;	/* number of floppy drives */
	ulong	nhard;		/* number of hard drives */
	ulong	ldepth;		/* screen depth */
	ulong	maxx;		/* screen width */
	ulong	maxy;		/* screen length */
};

/*
 *  MMU stuff in proc
 */
#define MAXMMU	4
#define MAXSMMU	1
struct PMMU
{
	Page	*mmutop;	/* 1st level table */
	Page	*mmufree;	/* unused page table pages */
	Page	*mmuused;	/* used page table pages */
};

#include "../port/portdat.h"

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

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
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
	FPsave	fpsave;			/* address of this is known by vdb */
	int	scallnr;		/* sys call number - known by db */
	Sargs	s;			/* address of this is known by db */
	int	nerrlab;
	Label	errlab[NERR];
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
	ulong	svcs;		/* cs before a notify */
	ulong	svss;		/* ss before a notify */
	ulong	svflags;		/* flags before a notify */
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

/*
 *  routines for things outside the PC model, like power management
 */
struct PCArch
{
	char	*id;
	void	(*reset)(void);		/* this should be in the model */
	int	(*cpuspeed)(int);	/* 0 = low, 1 = high */
	void	(*buzz)(int, int);	/* make a noise */
	void	(*lights)(int);		/* turn lights or icons on/off */
	int	(*serialpower)(int);	/* 1 == on, 0 == off */
	int	(*modempower)(int);	/* 1 == on, 0 == off */
	int	(*extvga)(int);		/* 1 == external, 0 == internal */
};

extern Mach	*m;
extern User	*u;

extern int	flipD[];	/* for flipping bitblt destination polarity */

#define BOOTLINE ((char *)0x80000100) /*  bootline passed by boot program */

extern PCArch *arch;			/* PC architecture */
