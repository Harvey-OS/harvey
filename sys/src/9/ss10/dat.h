typedef struct Conf	Conf;
typedef struct Ctx	Ctx;
typedef	struct FController	FController;
typedef	struct FDrive		FDrive;
typedef struct FFrame	FFrame;
typedef struct FPsave	FPsave;
typedef struct FType		FType;
typedef struct KMap	KMap;
typedef struct Lance	Lance;
typedef struct Lancemem	Lancemem;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct Page	Page; 
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
	ulong	sr;
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

#define	NFPQ	4	/* From TMS390Z50 User Docs p. 66 (4.6.3) */

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
	char	extras;		/* has extra non-contiguous banks of memory */
	int	ncontext;	/* in mmu */
	int	ctxalign;
	ulong	npage0;		/* total physical pages of memory, bank 0 */
	ulong	npage1;		/* total physical pages of memory, bank 1 */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	npage;		/* total physical pages of memory */
	ulong	nimage;		/* number of page cache image headers */
	ulong	nswap;		/* number of swap blocks */
	ulong	upages;		/* number of user pages */
	int	copymode;	/* 0 is copy on write, 1 is copy on reference */
	int	nfloppy;
};


/*
 *  mmu goo in the Proc structure
 */
struct PMMU
{
	Ctx	*ctx;		/* context table entry */
	ulong	root;		/* root page table pointer (MMU format) */
	Page	*mmufree;	/* unused page table pages */
	Page	*mmuused;	/* used page table pages */
	ulong	mmuptr;		/* first free entry in cur */
	int	pidonmach[MAXMACH];
};

#include "../port/portdat.h"

/*
 *  machine dependent definitions not used by ../port/dat.h
 */
#define	KMap		void
#define	VA(k)		((ulong)(k))
#define	kmap(p)		(KMap*)((p)->pa|KZERO)
#define	kunmap(k)

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc of last caller to splhi */

	/* ordering from here on irrelevant */
	int	mmask;			/* 1<<m->machno */
	ulong	ticks;			/* of the clock since boot time */
	Proc	*proc;			/* current process on this processor */
	Proc	*lproc;			/* last process on this processor */
	Label	sched;			/* scheduler wakeup */
	Lock	alarmlock;		/* access to alarm list */
	void	*alarm;			/* alarms bound to this clock */
	int	fpstate;		/* state of fp registers on machine */

	Ctx	*cctx;			/* current context */
	Ctx	*ctxlru;		/* LRU list of available contexts */
	ulong	*upt;			/* pointer to User page table entry */
	ulong	*contexts;		/* hardware context table */
	char	pidhere[NTLBPID];	/* is this pid possibly in this mmu? */
	int	lastpid;		/* last pid allocated on this machine */
	Proc	*pidproc[NTLBPID];	/* process that owns this tlbpid on this mach */

	int	tlbfault;
	int	tlbpurge;
	int	pfault;
	int	cs;
	int	syscall;
	int	load;
	int	fpunsafe;		/* FP unsaved */
	int	fptrap;			/* FP trap occurred while unsafe */
	int	intr;

	/* MUST BE LAST */
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
	Note	note[NNOTE];
	short	nnote;
	short	notified;		/* sysnoted is due */
	Note	lastnote;
	int	(*notify)(void*, char*);
	void	*ureg;
	void	*dbgreg;		/* User registers for debugging in proc */
};

/*
 * The ECC and VSIMM control registers live at 0xF.0000.0000.
 */
struct Ecc {
	ulong	emer;	/* ECC Memory Enable Register */
	ulong	mdr;		/* Campus-2 Memory Delay Register */
	ulong 	efsr;		/* ECC Fault Status Register */
	ulong	vcr;		/* Campus-2 Video Configuration Register */
	ulong	efar0;	/* ECC Fault Address Register 0 */
	ulong	efar1;	/* ECC Fault Address Register 1 */
	ulong	diag;		/* ECC Diagnostic Register */
};

/* Four of these structures live at 0xF.F140.{0,1,2,3}000, corresponding
 * to the four CPUs in a multiprocessor machine.  For this port, we only need
 * the lowest one.
 */
struct Intrreg {
	ulong	pend;		/* Pending (latched) interrupt register */
	ulong	clr_pend;		/* Write 1 to clear an interrupt in pend */
	ulong	set_pend;		/* Write 1 to post an interrupt to pend */
};
	
/* This structure lives at 0xF.F141.000.  sys_pend shows pending system-
 * wide interrupts.  sys_m is the interrupt mask; write 1 to the sys_mclear
 * and sys_mset pseudo-registers to alter the mask.  itr indicates which 
 * processor should receive (unmasked) system-wide interrupts. 
 */
struct Sysintrreg {
	ulong	sys_pend;
	ulong	sys_m;
	ulong	sys_mclear;
	ulong	sys_mset;
	ulong	itr;
};

/*
 * IOMMU registers
 */
struct Iommu {
	ulong	ctl;
	ulong	base;
	ulong	res1;
	ulong	res2;
	ulong	res3;
	ulong	tlbflush;
	ulong	addrflush;
};

/* 
 * MBus and SBus error registers and arbitration enables
 */
struct Busctl {
	ulong	m2safsr;
	ulong	m2safar;
	ulong	arben;
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

extern	struct Intrreg	*intrreg;
extern	struct Sysintrreg	*sysintrreg;
extern	FPsave	*initfpp;

struct
{
	Lock;
	short	machs;
	short	exiting;
}active;

/*
 *  floppy types (all MFM encoding)
 */
struct FType
{
	char	*name;
	int	dt;		/* compatible drive type */
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* number of heads */
	int	steps;		/* steps per cylinder */
	int	tracks;		/* tracks/disk */
	int	gpl;		/* intersector gap length for read/write */	
	int	fgpl;		/* intersector gap length for format */
	int	rate;		/* rate code */

	/*
	 *  these depend on previous entries and are set filled in
	 *  by floppyinit
	 */
	int	bcode;		/* coded version of bytes for the controller */
	long	cap;		/* drive capacity in bytes */
	long	tsize;		/* track size in bytes */
};
/*
 *  a floppy drive
 */
struct FDrive
{
	FType	*t;		/* floppy type */
	int	dt;		/* drive type */
	int	dev;

	ulong	lasttouched;	/* time last touched */
	int	cyl;		/* current arm position */
	int	confused;	/* needs to be recalibrated */
	int	vers;

	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	long	len;		/* size of xfer */

	uchar	*cache;	/* track cache */
	int	ccyl;
	int	chead;

	Rendez	r;		/* waiting here for motor to spin up */
};

/*
 *  controller for 4 floppys
 */
struct FController
{
	QLock;			/* exclusive access to the contoller */

	FDrive	*d;		/* the floppy drives */
	FDrive	*selected;
	int	rate;		/* current rate selected */
	uchar	cmd[14];	/* command */
	int	ncmd;		  /* # command bytes */
	uchar	stat[14];	/* command status */
	int	nstat;		  /* # status bytes */
	int	confused;	/* controler needs to be reset */
	Rendez	r;		/* wait here for command termination */
	int	motor;		/* bit mask of spinning disks */
	Rendez	kr;		/* for motor watcher */
};

/* bits in the registers */
enum
{
	/* status registers a & b */
	Psra=		0x3f0,
	Psrb=		0x3f1,

	/* digital output register */
	Pdor=		0x3f2,
	Fintena=	0x8,	/* enable floppy interrupt */
	Fena=		0x4,	/* 0 == reset controller */

	/* main status register */
	Pmsr=		0x3f4,
	Fready=		0x80,	/* ready to be touched */
	Ffrom=		0x40,	/* data from controller */
	Fnondma=	0x20, 	/* non-DMA transfer */
	Ffloppybusy=	0x10,	/* operation not over */

	/* data register */
	Pfdata=		0x3f5,
	Frecal=		0x07,	/* recalibrate cmd */
	Fseek=		0x0f,	/* seek cmd */
	Fsense=		0x08,	/* sense cmd */
	Fread=		0x66,	/* read cmd */
	Freadid=	0x4a,	/* read track id */
	Fspec=		0x03,	/* set hold times */
	Fwrite=		0x45,	/* write cmd */
	Fformat=	0x4d,	/* format cmd */
	Fmulti=		0x80,	/* or'd with Fread or Fwrite for multi-head */
	Fdumpreg=	0x0e,	/* dump internal registers */

	/* digital input register */
	Pdir=		0x3F7,	/* disk changed port (read only) */
	Pdsr=		0x3F7,	/* data rate select port (write only) */
	Fchange=	0x80,	/* disk has changed */

	/* status 0 byte */
	Drivemask=	3<<0,
	Seekend=	1<<5,
	Codemask=	(3<<6)|(3<<3),
};

