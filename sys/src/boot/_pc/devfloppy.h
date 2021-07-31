typedef	struct FController FController;
typedef	struct FDrive FDrive;
typedef struct FType FType;

static void floppyintr(Ureg*);
static int floppyon(FDrive*);
static void floppyoff(FDrive*);
static void floppysetdef(FDrive*);

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
	int	offset;		/* current offset */
	int	vers;
	int	maxtries;

	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	long	len;		/* size of xfer */

	uchar	*cache;		/* track cache */
	int	ccyl;
	int	chead;

//	Rendez	r;		/* waiting here for motor to spin up */
	void	*aux;
};

/*
 *  controller for 4 floppys
 */
struct FController
{
//	QLock;			/* exclusive access to the contoller */

	int	ndrive;
	FDrive	*d;		/* the floppy drives */
	FDrive	*selected;
	int	rate;		/* current rate selected */
	uchar	cmd[14];	/* command */
	int	ncmd;		/* # command bytes */
	uchar	stat[14];	/* command status */
	int	nstat;		/* # status bytes */
	int	confused;	/* controler needs to be reset */
//	Rendez	r;		/* wait here for command termination */
	int	motor;		/* bit mask of spinning disks */
//	Rendez	kr;		/* for motor watcher */
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
	Cmdexec=	1<<6,

	/* status 1 byte */
	Overrun=	0x10,
};

/*
 *  types of drive (from PC equipment byte)
 */
enum
{
	Tnone=		0,
	T360kb=		1,
	T1200kb=	2,
	T720kb=		3,
	T1440kb=	4,
};

static void
pcfloppyintr(Ureg *ur, void *a)
{
	USED(a);

	floppyintr(ur);
}

void
floppysetup0(FController *fl)
{
	uchar equip;

	/*
	 * Read nvram for types of floppies 0 & 1.
	 * Always try floppy 0.
	 */
	equip = nvramread(0x10);
	fl->ndrive = 1;

	if(equip & 0xf)
		fl->ndrive++;

	/*
	 * Allocate the drive storage.
	 * There's always one.
	 */
	fl->d = xalloc(fl->ndrive*sizeof(FDrive));
	fl->d[0].dt = (equip >> 4) & 0xf;
	if(fl->d[0].dt == Tnone)
		fl->d[0].dt = T1440kb;

	if(fl->ndrive == 2)
		fl->d[1].dt = equip & 0xf;
}

void
floppysetup1(FController*)
{
//	intrenable(VectorFLOPPY, pcfloppyintr, fl, BUSUNKNOWN);
	setvec(VectorFLOPPY, pcfloppyintr, 0);
}


static vlong pcfloppyseek(FDrive*, vlong);
FController	fl;

vlong
floppyseek(Fs *fs, vlong off)
{
	FDrive *dp;

	dp = &fl.d[fs->dev];
	return pcfloppyseek(dp, off);
}
