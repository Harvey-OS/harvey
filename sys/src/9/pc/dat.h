typedef struct Conf	Conf;
typedef	struct FController	FController;
typedef	struct FDrive		FDrive;
typedef struct FPsave	FPsave;
typedef struct FType		FType;
typedef struct ISAConf	ISAConf;
typedef struct Label	Label;
typedef struct Lock	Lock;
typedef struct MMU	MMU;
typedef struct Mach	Mach;
typedef struct PCArch	PCArch;
typedef struct PCMmap	PCMmap;
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
	ulong	sr;
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

/*
 *  task state segment.  Plan 9 ignores all the task switching goo and just
 *  uses the tss for esp0 and ss0 on gate's into the kernel, interrupts,
 *  and exceptions.  The rest is completely ignored.
 *
 *  We use one tss per CPU because we'll need to in Brazil.
 *  
 */
typedef struct Tss	Tss;
struct Tss
{
	ulong	backlink;	/* unused */
	ulong	sp0;		/* pl0 stack pointer */
	ulong	ss0;		/* pl0 stack selector */
	ulong	sp1;		/* pl1 stack pointer */
	ulong	ss1;		/* pl1 stack selector */
	ulong	sp2;		/* pl2 stack pointer */
	ulong	ss2;		/* pl2 stack selector */
	ulong	cr3;		/* page table descriptor */
	ulong	eip;		/* instruction pointer */
	ulong	eflags;		/* processor flags */
	ulong	eax;		/* general (hah?) registers */
	ulong 	ecx;
	ulong	edx;
	ulong	ebx;
	ulong	esp;
	ulong	ebp;
	ulong	esi;
	ulong	edi;
	ulong	es;		/* segment selectors */
	ulong	cs;
	ulong	ss;
	ulong	ds;
	ulong	fs;
	ulong	gs;
	ulong	ldt;		/* local descriptor table */
	ulong	iomap;		/* io map base */
};

/*
 *  segment descriptor/gate
 */
struct Segdesc
{
	ulong	d0;
	ulong	d1;
};

struct Mach
{
	/* OFFSETS OF THE FOLLOWING KNOWN BY l.s */
	int	machno;			/* physical id of processor */
	ulong	splpc;			/* pc of last caller to splhi */

	/* ordering from here on irrelevant */
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

	Tss	tss;
	Segdesc	gdt[N386SEG];

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
	int	(*snooze)(ulong, int);
};

extern Mach	*m;
extern User	*u;

ulong boottime;

extern int	flipD[];	/* for flipping bitblt destination polarity */

#define BOOTLINE ((char *)0x80000100) /*  bootline passed by boot program */

extern PCArch *arch;			/* PC architecture */

struct ISAConf {
	char	type[NAMELEN];
	ulong	port;
	ulong	irq;
	int	dma;
	ulong	mem;
	ulong	size;
	uchar	ea[6];
	uchar	bus;
};

/*
 *  maps between ISA memory space and PCMCIA card memory space
 */
struct PCMmap
{
	ulong	ca;		/* card address */
	ulong	cea;		/* card end address */
	ulong	isa;		/* ISA address */
	int	len;		/* length of the ISA area */
	int	attr;		/* attribute memory */
	int	ref;
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

