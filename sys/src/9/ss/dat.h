typedef struct Conf	Conf;
typedef struct Ctx	Ctx;
typedef struct FFrame	FFrame;
typedef struct FPsave	FPsave;
typedef struct KMap	KMap;
typedef struct Lance	Lance;
typedef struct Lancemem	Lancemem;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Pmeg	Pmeg;
typedef struct PMMU	PMMU;
typedef struct Mach	Mach;
typedef struct Ureg	Ureg;
typedef struct List	List;
typedef struct User	User;



#define	MACHP(n)	(n==0? &mach0 : *(Mach**)0)

extern	Mach	mach0;
extern  void	(*kprofp)(ulong);

/*
 *  parameters for sysproc.c
 */
#define AOUT_MAGIC	K_MAGIC

/*
 *  machine dependent definitions used by ../port/dat.h
 */

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
 * floating point registers
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
};

#define	NFPQ	4	/* just a guess */

struct	FPsave
{
	long	fsr;
	long	fpreg[32];
	struct{
		ulong	a;	/* address */
		ulong	i;	/* instruction */
	}q[NFPQ];
	long	fppc;		/* for recursive traps only */
};

struct Conf
{
	int	nmach;		/* processors */
	int	nproc;		/* processes */
	ulong	monitor;	/* graphics monitor id; 0 for none */
	char	ss2;		/* is a sparcstation 2 */
	char	ss2cachebug;	/* has sparcstation2 cache bug */
	int	ncontext;	/* in mmu */
	int	npmeg;
	int	vacsize;	/* size of virtual address cache, in bytes */
	int	vaclinesize;	/* size of cache line */
	ulong	npage0;		/* total physical pages of memory, bank 0 */
	ulong	npage1;		/* total physical pages of memory, bank 1 */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	npage;		/* total physical pages of memory */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap blocks */
	ulong	upages;		/* number of user pages */
	int	copymode;	/* 0 is copy on write, 1 is copy on reference */
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
	Ctx	*ctxonmach[MAXMACH];
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
	int	mmask;			/* 1<<m->machno */
	ulong	ticks;			/* of the clock since boot time */
	Proc	*proc;			/* current process on this processor */
	Proc	*lproc;			/* last process on this processor */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	fpstate;		/* state of fp registers on machine */

	Ctx	*cctx;			/* current context */
	List	*clist;			/* lru list of all contexts */
	Pmeg	*pmeg;			/* array of allocatable pmegs */
	int	pfirst;			/* index of first allocatable pmeg */
	List	*plist;			/* lru list of non-kernel Pmeg's */
	int	needpmeg;		/* a process needs a pmeg */

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	fpunsafe;		/* FP unsaved */
	int	fptrap;			/* FP trap occurred while unsafe */
	int	intr;

	int	stack[1];
};

#define	NERR	15
#define	NNOTE	5
struct User
{
	Proc	*p;
					/* &fpsave must be 4 mod 8 */
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
	ulong	svpsr;
	Note	note[NNOTE];
	short	nnote;
	short	notified;		/* sysnoted is due */
	Note	lastnote;
	int	(*notify)(void*, char*);
	void	*ureg;
	void	*dbgreg;		/* User registers for debugging in proc */
	ulong	svr7;
};

/*
 *  LANCE CSR3 (bus control bits)
 */
#define BSWP	0x4
#define ACON	0x2
#define BCON	0x1

/*
 *  system dependent lance stuff
 *  filled by lancesetup() 
 */
struct Lance
{
	ushort	lognrrb;	/* log2 number of receive ring buffers */
	ushort	logntrb;	/* log2 number of xmit ring buffers */
	ushort	nrrb;		/* number of receive ring buffers */
	ushort	ntrb;		/* number of xmit ring buffers */
	ushort	*rap;		/* lance address register */
	ushort	*rdp;		/* lance data register */
	ushort	busctl;		/* bus control bits */
	uchar	ea[6];		/* our ether addr */
	int	sep;		/* separation between shorts in lance ram
				    as seen by host */
	ushort	*lanceram;	/* start of lance ram as seen by host */
	Lancemem *lm;		/* start of lance ram as seen by lance */
	Etherpkt *rp;		/* receive buffers (host address) */
	Etherpkt *tp;		/* transmit buffers (host address) */
	Etherpkt *lrp;		/* receive buffers (lance address) */
	Etherpkt *ltp;		/* transmit buffers (lance address) */
};

extern register Mach	*m;		/* R6 */
extern register User	*u;		/* R5 */

extern	uchar	*intrreg;
extern	FPsave	*initfpp;

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;
