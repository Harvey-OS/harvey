typedef struct Conf	Conf;
typedef struct FFrame	FFrame;
typedef struct FPsave	FPsave;
typedef struct KMap	KMap;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct MMU	MMU;
typedef struct Mach	Mach;
typedef struct PMMU	PMMU;
typedef struct Portpage	Portpage;
typedef struct Ureg	Ureg;
typedef struct User	User;

#define	MACHP(n)	(n==0? &mach0 : *(Mach**)0)

extern	Mach	mach0;
extern  void	(*kprofp)(ulong);

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	A_MAGIC

/*
 *  machine dependent definitions used by ../port/dat.h
 */

struct Lock
{
	char	key;
	ulong	pc;
};

enum
{
	FPinit,
	FPactive,
	FPdirty,
};

struct	FPsave
{
	uchar	type;
	uchar	size;
	short	reserved;
	char	junk[212];	/* 68881: sizes 24, 180; 68882: 56, 212 */
	/* this offset known in db */
	char	reg[3*4+8*12];
};

struct Label
{
	ulong	sp;
	ulong	pc;
	ushort	sr;
};

/*
 *  MMU info included in the Proc structure
 */
struct PMMU
{
	int	pmmu_dummy;
};

struct Conf
{
	int	nmach;		/* processors */
	int	nproc;		/* processes */
	int	monitor;	/* has display */
	ulong	npage0;		/* total physical pages of memory, bank 0 */
	ulong	npage1;		/* total physical pages of memory, bank 1 */
	ulong	npage;
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	upages;		/* user page pool */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap blocks */
	int	nfsyschan;	/* number of filsys open channels */
	int	nisdn;		/* number of isdn interfaces */
	int	nlapd;		/* number of dragnet protocol modules */
	int	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	portispaged;	/* 1 if extended I/O port addresses */
	int	nconc;		/* number of datakit concentrators */
};

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */
struct KMap
{
	KMap	*next;
	ulong	pa;
	ulong	va;
};
#define	VA(k)	((k)->va)

struct Mach
{
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc of last caller to splhi */
	ulong	ticks;			/* of the clock since boot time */
	Proc	*proc;			/* current process on this processor */
	Proc	*lproc;			/* last process on this processor */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	fpstate;		/* state of fp registers on machine */

	int	tlbpurge;
	int	tlbfault;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;

	int	stack[1];
};

/*
 *  gnot bus ports
 */
#define PORTSIZE	64
#define PORTSHIFT	6
#define PORTSELECT	PORT[32]

struct Portpage
{
	union{
		Lock;
		QLock;
	};
	int	 select;
};

#define	NERR	25
#define	NNOTE	5
struct User
{
	Proc	*p;
	FPsave	fpsave;			/* address of this is known by vdb */
	int	scallnr;		/* sys call number - known by db */
	Sargs	s;			/* address of this is known by db */
	uchar	balusave[64];		/* #include botch */
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
	void	*dbgreg;
	ushort	svsr;
	ushort	svvo;
};

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;

extern Mach	*m;
extern User	*u;
