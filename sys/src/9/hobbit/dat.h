typedef struct Conf Conf;
typedef struct FPsave FPsave;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct Mach Mach;
typedef struct Page Page;
typedef struct PMMU PMMU;
typedef struct Ureg Ureg;
typedef struct User User;
typedef struct Vector Vector;

struct Conf {
	int	nmach;		/* processors */
	int	nproc;		/* processes */
	int	monitor;	/* has display? */
	ulong	npage0;		/* total physical pages of memory, bank 0 */
	ulong	npage1;		/* total physical pages of memory, bank 1 */
	ulong	npage;
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	upages;		/* number of user pages */
	ulong	nimage;		/* number of page cache image headers */
	ulong 	npagetab;	/* number of pte tables */
	ulong	nswap;		/* number of swap blocks */
	int	copymode;	/* 0 is copy on write, 1 is copy on reference */
	ulong	ipif;		/* Ip protocol interfaces */
	ulong	ip;		/* Ip conversations per interface */
	ulong	arp;		/* Arp table size */
	ulong	frag;		/* Ip fragment assemble queue size */
};

/*
 * floating point registers
 */
enum {
	FPinit,
	FPactive,
	FPdirty,
};

struct FPsave {
	int fpstatus;
};

struct Vector {
	ulong	mask;
	int	ctlrno;
	void	(*handler)(int, Ureg*);
	char	*name;
};

struct Label {
	ulong	isp;
	ulong	sp;
	ulong	msp;
	ulong	pc;
	ulong	psw;
};

struct Lock {
	ulong	key;
	ulong	pc;
};

/*
 *  MMU info included in the Proc structure
 */
struct PMMU {
	Page	*mmustb;
	Page	*mmuused;
	Page	*mmufree;
};

#include "../port/portdat.h"

struct Mach {
	int	machno;
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
	int	spinlock;
	int	load;
	int	intr;

	int	stack[1];
};

struct {
	Lock;
	short	machs;
	short	exiting;
} active;

extern Mach *m;
extern Mach *mach0;

#define	MACHP(n)	(n == 0 ? mach0: 0)

#define	NERR	25
#define	NNOTE	5

struct User {
	Proc	*p;
	FPsave	fpsave;			/* address of this is known by db */
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
	ulong	notepsw;
	Ureg	*noteur;
	void	*dbgreg;
};

extern User *u;

/*
 * Fake kmap
 */
typedef void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)(KADDR(p->pa))
#define	kunmap(k)
#define	kmapperm(x)	kmap(x)

#define AOUT_MAGIC	Z_MAGIC
