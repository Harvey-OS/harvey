typedef struct Conf	Conf;
typedef struct FFrame	FFrame;
typedef struct FPsave	FPsave;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Mach	Mach;
typedef struct Page	Page;
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

struct Lock
{
	char	key;
	ulong	pc;
};

struct Label
{
	ulong	sp;
	ulong	pc;
	ushort	sr;
};

/*
 * floating point registers
 */
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
	union{
	char	reg[3*4+8*12];
	struct{
		ulong	fpcr;
		ulong	fpsr;
		ulong	fpiar;
		struct{
			ulong	d[3];
		}dreg[8];
	};
	};
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
	int	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	portispaged;	/* ??? */
	ulong	ipif;		/* Ip protocol interfaces */
	ulong	ip;		/* Ip conversations per interface */
	ulong	arp;		/* Arp table size */
	ulong	frag;		/* Ip fragment assemble queue size */
	int	nfloppy;
};

/*
 *  MMU stuff in proc
 */
struct PMMU
{
	Page	*mmufree;	/* unused page table pages */
	Page	*mmuused;	/* used page table pages */
	ulong	mmuptr;		/* first free entry in cur */
	ulong	mmuA[2];	/* first 2 entries of A table */
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
	int	fpstate;		/* state of fp registers on machine */

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	intr;
	Proc	*lproc;

	int	stack[1];
};

/*
 * Fake kmap
 */
typedef void		KMap;
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)(p->pa|KZERO)
#define	kmapperm(p)	kmap(p)
#define	kunmap(k)

#define	NERR	20
#define	NNOTE	5
struct User
{
	Proc	*p;
	FPsave	fpsave;			/* offset known by db */
	int	scallnr;		/* sys call number - known by db */
	Sargs	s;			/* offset known by db */
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
	ushort	svvo;
	ushort	svsr;
};

enum
{
	Scsidevice,
	Scsidma,
};

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;


extern Mach	*m;
extern User	*u;
