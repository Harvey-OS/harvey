#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

/* Intel 82077A (8272A compatible) floppy controller */

typedef	struct Drive		Drive;
typedef	struct Controller	Controller;
typedef struct Type		Type;
typedef struct Device		Device;

/* registers on the 82077 */
struct Device
{
	uchar		sra;	/* (R/O) status register A */		
	uchar		srb;	/* (R/O) status register B */
	uchar		dor;	/* (R/W) digital output register */
	uchar		rsvd3;	/* reserved */
	uchar		msr;	/* (R/O) main status register */
#define dsr	msr
	uchar		fifo;	/* (R/W) data FIFO */
	uchar		rsvd6;	/* reserved */
	uchar		dir;	/* (R/O) digital input register */
#define ccr	dir
	uchar		flpctl;	/* (W/O) control (not in 82077) */
};

/* bits in the registers */
enum
{
	/* status register A */
	Fintpend=	0x80,	/* interrupt pending */

	/* digital output register */
	Fintena=	0x8,	/* enable floppy interrupt */
	Fena=		0x4,	/* 0 == reset controller */

	/* main status register */
	Fready=		0x80,	/* ready to be touched */
	Ffrom=		0x40,	/* data from controller */
	Fbusy=		0x10,	/* operation not over */

	/* data register */
	Frecal=		0x07,	/* recalibrate cmd */
	Fseek=		0x0f,	/* seek cmd */
	Fsense=		0x08,	/* sense cmd */
	Fread=		0x66,	/* read cmd */
	Freadid=	0x4a,	/* read track id */
	Fspec=		0x03,	/* set hold times */
	Fwrite=		0x45,	/* write cmd */
	Fmulti=		0x80,	/* or'd with Fread or Fwrite for multi-head */
	Fdumpreg=	0x0e,	/* dump internal registers */

	/* disk changed register */
	Fchange=	0x80,	/* disk has changed */

	/* floppy control register (not part of 82077) */
	Eject_disc	= 0x80,		/* Eject the floppy */
	Sel_82077	= 0x40,		/* Scsi/82077 select */
	Drv_id		= 0x04,		/* Drive present flag */
	Mid		= 0x03,		/* Media id bits */

	/* status 0 byte */
	Drivemask=	3<<0,
	Seekend=	1<<5,
	Codemask=	(3<<6)|(3<<3),

	/* file types */
	Qdir=		0,
	Qdata=		(1<<2),
	Qctl=		(2<<2),
	Qmask=		(3<<2),
};

/*
 *  floppy types
 */
struct Type
{
	char	*name;
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* number of heads */
	int	steps;		/* steps per cylinder */
	int	tracks;		/* tracks/disk */
	int	gpl;		/* intersector gap length for read/write */	
	int	fgpl;		/* intersector gap length for format */

	/*
	 *  these depend on previous entries and are set filled in
	 *  by floppyinit
	 */
	int	bcode;		/* coded version of bytes for the controller */
	long	cap;		/* drive capacity in bytes */
	long	tsize;		/* track size in bytes */
};
Type floppytype[] =
{
	{ "MF2HD",	512,	18,	2,	1,	80,	0x1B,	0x54, },
	{ "MF1DD",	512,	9,	2,	1,	80,	0x1B,	0x54, },
	{ "MF4HD",	1024,	18,	2,	1,	80,	0x1B,	0x54, },
	{ "F2HD",	512,	15,	2,	1,	80,	0x2A,	0x50, },
	{ "F2DD",	512,	8,	2,	2,	40,	0x2A,	0x50, },
	{ "F1DD",	512,	8,	1,	2,	40,	0x2A,	0x50, },
};
#define NTYPES (sizeof(floppytype)/sizeof(Type))

/*
 *  bytes per sector encoding for the controller.
 *  - index for b2c is is (bytes per sector/128).
 *  - index for c2b is code from b2c
 */
static int b2c[] =
{
[1]	0,
[2]	1,
[4]	2,
[8]	3,
};
static int c2b[] =
{
	128,
	256,
	512,
	1024,
};

/*
 *  a floppy drive
 */
struct Drive
{
	Type	*t;
	int	dev;

	ulong	lasttouched;	/* time last touched */
	int	cyl;		/* current cylinder */
	int	confused;	/* needs to be recalibrated */
	int	vers;

	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	long	len;		/* size of xfer */

	uchar	*virtual;	/* track cache */
	uchar	*physical;	/* ... */
	int	ccyl;
	int	chead;

	Rendez	r;		/* waiting here for motor to spin up */
};

/*
 *  controller for 4 floppys
 */
struct Controller
{
	QLock;			/* exclusive access to the contoller */

	Drive	*d;		/* the floppy drives */
	uchar	cmd[14];	/* command */
	int	ncmd;		  /* # command bytes */
	uchar	stat[14];	/* command status */
	int	nstat;		  /* # status bytes */
	int	confused;	/* controler needs to be reset */
	Rendez	r;		/* wait here for command termination */
	int	motor;		/* bit mask of spinning disks */

	uchar	*virtual;	/* kernel buffer for DMA (output) */
	uchar	*physical;	/* ... */

	Rendez	kr;		/* for motor watcher */
};

Controller	fl;

#define MOTORBIT(i)	(1<<((i)+4))

/*
 *  predeclared
 */
static int	floppycmd(void);
static void	floppyeject(Drive*);
static void	floppykproc(void*);
static void	floppyon(Drive*);
static void	floppyoff(Drive*);
static void	floppypos(Drive*,long);
static int	floppyrecal(Drive*);
static int	floppyresult(void);
static void	floppyrevive(void);
static long	floppyseek(Drive*, long);
static int	floppysense(void);
static void	floppywait(void);
static long	floppyxfer(Drive*, int, void*, long, long);
static long	floppythrice(Drive*, int, void*, long, long);
static int	cmddone(void*);

Dirtab floppydir[]={
	"fd0disk",		{Qdata + 0},	0,	0666,
	"fd0ctl",		{Qctl + 0},	0,	0666,
	"fd1disk",		{Qdata + 1},	0,	0666,
	"fd1ctl",		{Qctl + 1},	0,	0666,
	"fd2disk",		{Qdata + 2},	0,	0666,
	"fd2ctl",		{Qctl + 2},	0,	0666,
	"fd3disk",		{Qdata + 3},	0,	0666,
	"fd3ctl",		{Qctl + 3},	0,	0666,
};
#define NFDIR	2	/* directory entries/drive */

/*
 *  grab DMA from SCSI
 */
#define GETDMA\
{\
	qlock(&dmalock);\
	dmaowner = Ofloppy;\
	((Device*)(FDCTLRL))->flpctl = Sel_82077;\
	if(waserror()){\
		((Device*)(FDCTLRL))->flpctl = 0;\
		qunlock(&dmalock);\
		nexterror();\
	}\
}

/*
 *  let SCSI have DMA
 */
#define PUTDMA\
{\
	((Device*)(FDCTLRL))->flpctl = 0;\
	dmaowner = Oscsi;\
	qunlock(&dmalock);\
	poperror();\
}

void
floppyreset(void)
{
	Device *devp = (Device*)(FDCTLRL);
	Drive *dp;
	Type *t;
	ulong p;
	long l;

	/*
	 *  init dependent parameters
	 */
	for(t = floppytype; t < &floppytype[NTYPES]; t++){
		t->cap = t->bytes * t->heads * t->sectors * t->tracks;
		t->bcode = b2c[t->bytes/128];
		t->tsize = t->bytes * t->sectors;
	}

	/*
	 *  allocate the drive storage
	 */
	fl.d = xalloc(conf.nfloppy*sizeof(Drive));

	/*
	 *  allocate a page for dma
	 */
	p = (ulong)xspanalloc(BY2PG, BY2PG, 0);
	fl.virtual = (uchar *)kmappa(p);
	fl.physical = (uchar*)p;

	/*
	 *  reset the chip, leave it off
	 */
	fl.motor = 0;
	devp->flpctl = Sel_82077;
	devp->dor = 0;
	devp->flpctl = 0;

	/*
	 *  init drives
	 */
	for(dp = fl.d; dp < &fl.d[conf.nfloppy]; dp++){
		dp->dev = dp - fl.d;
		dp->t = &floppytype[0];		/* default type */
		floppydir[NFDIR*dp->dev].length = dp->t->cap;
		dp->cyl = -1;		/* because we don't know */
		p = (ulong)xspanalloc(dp->t->tsize, BY2PG, 0);
		dp->virtual = (uchar *)kmappa(p);
		for(l = BY2PG; l < dp->t->tsize; l += BY2PG)
			kmappa(p+l);
		dp->physical = (uchar*)p;
		dp->ccyl = -1;
		dp->vers = 1;
	}

	/*
	 *  first operation will recalibrate
	 */
	fl.confused = 1;
}

void
floppyinit(void)
{
}

Chan*
floppyattach(char *spec)
{
	static int kstarted;

	if(kstarted == 0){
		/*
		 *  watchdog to turn off the motors
		 */
		kstarted = 1;
		kproc("floppy", floppykproc, 0);
	}
	return devattach('f', spec);
}

Chan*
floppyclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
floppywalk(Chan *c, char *name)
{
	return devwalk(c, name, floppydir, conf.nfloppy*NFDIR, devgen);
}

void
floppystat(Chan *c, char *dp)
{
	devstat(c, dp, floppydir, conf.nfloppy*NFDIR, devgen);
}

Chan*
floppyopen(Chan *c, int omode)
{
	return devopen(c, omode, floppydir, conf.nfloppy*NFDIR, devgen);
}

void
floppycreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
floppyclose(Chan *c)
{
	USED(c);
}

void
floppyremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
floppywstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

static void
islegal(Chan *c, long n, Drive *dp)
{
	if(c->offset % dp->t->bytes)
		error(Ebadarg);
	if(n % dp->t->bytes)
		error(Ebadarg);
	if(c->qid.vers!=0 && c->qid.vers!=dp->vers){
		c->qid.vers = dp->vers;
		error(Eio);
	} else
		c->qid.vers = dp->vers;
}

static void
changed(Chan *c, Drive *dp)
{
	ulong old;

	old = c->qid.vers;
	c->qid.vers = dp->vers;
	if(old && old!=dp->vers)
		error(Eio);
}

long
floppyread(Chan *c, void *a, long n)
{
	Drive *dp;
	long rv, i;
	int nn, sec, head, cyl;
	long len;
	uchar *aa;

	if(c->qid.path == CHDIR)
		return devdirread(c, a, n, floppydir, conf.nfloppy*NFDIR, devgen);

	rv = 0;
	dp = &fl.d[c->qid.path & ~Qmask];

	switch ((int)(c->qid.path & Qmask)) {
	case Qdata:
		islegal(c, n, dp);
		aa = a;
		nn = dp->t->tsize;

		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			fl.confused = 1;
			CLRCTL(Cpu_dma);
			nexterror();
		}
		GETDMA;
		floppyon(dp);
		changed(c, dp);
		for(rv = 0; rv < n; rv += len){
			/*
			 *  truncate xfer at track boundary
			 */
			dp->len = n - rv;
			floppypos(dp, c->offset+rv);
			cyl = dp->tcyl;
			head = dp->thead;
			len = dp->len;
			sec = dp->tsec;

			/*
			 *  read the track
			 */
			if(dp->ccyl!=cyl || dp->chead!=head){
				dp->ccyl = -1;
				i = floppythrice(dp, Fread,0,(cyl*dp->t->heads+head)*nn,nn);
				if(i != nn){
					if(i == 0)
						break;
					error(Eio);
				}
				dp->ccyl = cyl;
				dp->chead = head;
			}
			memmove(aa+rv, dp->virtual + (sec-1)*dp->t->bytes, len);
		}
		PUTDMA;
		qunlock(&fl);
		poperror();
		break;
	case Qctl:
		break;
	default:
		panic("floppyread: bad qid");
	}

	return rv;
}

#define SNCMP(a, b) strncmp(a, b, sizeof(b)-1)
long
floppywrite(Chan *c, void *a, long n)
{
	Drive *dp;
	long rv, i;
	char *aa = a;

	rv = 0;
	dp = &fl.d[c->qid.path & ~Qmask];

	switch ((int)(c->qid.path & Qmask)) {
	case Qdata:
		islegal(c, n, dp);
		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			fl.confused = 1;
			CLRCTL(Cpu_dma);
			nexterror();
		}
		GETDMA;
		floppyon(dp);
		changed(c, dp);
		for(rv = 0; rv < n; rv += i){
			floppypos(dp, c->offset+rv);
			if(dp->tcyl == dp->ccyl)
				dp->ccyl = -1;
			i = floppythrice(dp, Fwrite, aa+rv, c->offset+rv,
				n-rv);
			if(i <= 0)
				break;
		}
		PUTDMA;
		qunlock(&fl);
		poperror();
		break;
	case Qctl:
		if(SNCMP(aa, "eject") == 0){
			qlock(&fl);
			GETDMA;
			floppyeject(dp);
			PUTDMA;
			qunlock(&fl);
		} else if(SNCMP(aa, "reset") == 0){
			qlock(&fl);
			GETDMA;
			floppyon(dp);
			PUTDMA;
			qunlock(&fl);
		}
		break;
	default:
		panic("floppywrite: bad qid");
	}

	return rv;
}

static void
floppykproc(void *a)
{
	Drive *dp;

	USED(a);

	while(waserror())
		;
	for(;;){
		for(dp = fl.d; dp < &fl.d[conf.nfloppy]; dp++){
			if((fl.motor&MOTORBIT(dp->dev))
			&& TK2SEC(m->ticks - dp->lasttouched) > 5
			&& canqlock(&fl)){
				GETDMA;
				if(TK2SEC(m->ticks - dp->lasttouched) > 5)
					floppyoff(dp);
				qunlock(&fl);
				PUTDMA;
			}
		}
		tsleep(&fl.kr, return0, 0, 5*1000);
	}
}

/*
 *  start a floppy drive's motor.
 */
static void
floppyon(Drive *dp)
{
	Device *devp = (Device*)(FDCTLRL);
	int alreadyon;
	int tries;

	for(tries = 0; tries < 4; tries++){
		if(fl.confused)
			floppyrevive();
	
		/* start motor and select drive */
		alreadyon = fl.motor & MOTORBIT(dp->dev);
		fl.motor |= MOTORBIT(dp->dev);
		devp->dor = fl.motor | Fintena | Fena | dp->dev;
		if(!alreadyon)
			tsleep(&dp->r, return0, 0, 750);
	
		/* get drive to a known cylinder */
		if(dp->confused)
			if(floppyrecal(dp) >= 0)
				break;
		else
			break;
	}
	dp->lasttouched = m->ticks;
}

/*
 *  stop the floppy if it hasn't been used in 5 seconds
 */
static void
floppyoff(Drive *dp)
{
	Device *devp = (Device*)(FDCTLRL);

	fl.motor &= ~MOTORBIT(dp->dev);
	devp->dor = fl.motor | Fintena | Fena | dp->dev;
}

/*
 *  eject disk
 */
static void
floppyeject(Drive *dp)
{
	Device *devp = (Device*)(FDCTLRL);

	dp->vers++;
	floppyon(dp);
	delay(1);
	devp->flpctl = Eject_disc|Sel_82077;
	delay(1);
	devp->flpctl = Sel_82077;
	delay(1);
	floppyoff(dp);
}

/*
 *  send a command to the floppy
 */
static int
floppycmd(void)
{
	Device *devp = (Device*)(FDCTLRL);
	int i;
	int tries;

	for(i = 0; i < fl.ncmd; i++){
		for(tries = 0; ; tries++){
			if(tries > 1000){
print("cmd %ux can't be sent (%d %ux)\n", fl.cmd[0], i, devp->msr);
				fl.confused = 1;
				return -1;
			}
			if((devp->msr&(Ffrom|Fready)) == Fready)
				break;
		}
		devp->fifo = fl.cmd[i];
	}
	return 0;
}

/*
 *  get a command result from the floppy
 *
 *  when the controller goes ready waiting for a command
 *  (instead of sending results), we're done
 * 
 */
static int
floppyresult(void)
{
	Device *devp = (Device*)(FDCTLRL);
	int i, s;
	int tries;

	for(i = 0; i < sizeof(fl.stat); i++){
		for(tries = 0; ; tries++){
			if(tries > 1000){
				fl.confused = 1;
				return -1;
			}
			s = devp->msr&(Ffrom|Fready);
			if(s == Fready){
				fl.nstat = i;
				return i;
			}
			if(s == (Ffrom|Fready))
				break;
		}
		fl.stat[i] = devp->fifo;
	}
	fl.nstat = i;
	return i;
}

/*
 *  calculate physical address of a logical byte offset into the disk
 *
 *  truncate dp->length if it crosses a track boundary
 */
static void
floppypos(Drive *dp, long off)
{
	int lsec;
	int ltrack;
	int end;

	lsec = off/dp->t->bytes;
	ltrack = lsec/dp->t->sectors;
	dp->tcyl = ltrack/dp->t->heads;
	dp->tsec = (lsec % dp->t->sectors) + 1;
	dp->thead = (lsec/dp->t->sectors) % dp->t->heads;

	/*
	 *  can't read across track boundaries.
	 *  if so, decrement the bytes to be read.
	 */
	end = (ltrack+1)*dp->t->sectors*dp->t->bytes;
	if(off+dp->len > end)
		dp->len = end - off;
}

/*
 *  get the interrupt cause from the floppy.
 */
static int
floppysense(void)
{
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Fsense;
	if(floppycmd() < 0)
		return -1;
	if(floppyresult() < 2){
print("can't read sense response\n");
		fl.confused = 1;
		return -1;
	}
	return 0;
}

static int
cmddone(void *a)
{
	USED(a);
	return fl.ncmd == 0;
}

/*
 *  wait for a floppy interrupt
 */
static void
floppywait(void)
{
	tsleep(&fl.r, cmddone, 0, 2000);
}

/*
 *  we've lost the floppy position, go to cylinder 0.
 */
static int
floppyrecal(Drive *dp)
{
	Device *devp = (Device*)(FDCTLRL);
	int type;

	dp->ccyl = -1;

	type = devp->flpctl & Mid;
	switch(type){
	case 0:
		print("no diskette %d\n", dp->dev);
		return -1;
	case 1:
		devp->dsr = 3;
		devp->ccr = 3;
		dp->t = &floppytype[2];
		return -1;
	case 2:
		dp->t = &floppytype[0];
		break;
	case 3:
		devp->dsr = 0;
		devp->ccr = 0;
		dp->t = &floppytype[1];
		break;
	}
	
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Frecal;
	fl.cmd[fl.ncmd++] = dp->dev;
	if(floppycmd() < 0)
		return -1;
	floppywait();
	if(fl.nstat < 2){
		fl.confused = 1;
		return -1;
	}
	if((fl.stat[0] & (Codemask|Seekend)) != Seekend){
		dp->confused = 1;
		return -1;
	}
	dp->cyl = fl.stat[1]/dp->t->steps;
	if(dp->cyl != 0){
		print("recalibrate went to wrong cylinder %d\n", dp->cyl);
		dp->confused = 1;
		return -1;
	}

	dp->confused = 0;
	return 0;
}

/*
 *  if the controller or a specific drive is in a confused state,
 *  reset it and get back to a kown state
 */
void
floppyrevive(void)
{
	Device *devp = (Device*)(FDCTLRL);
	Drive *dp;

	/*
	 *  reset the controller if it's confused
	 */
	if(fl.confused){
		/* reset controller and turn all motors off */
		splhi();
		fl.cmd[0] = 0;
		devp->dor = 0;
		delay(1);
		devp->dor = Fintena|Fena;
		spllo();
		for(dp = fl.d; dp < &fl.d[conf.nfloppy]; dp++)
			dp->confused = 1;
		fl.motor = 0;
		floppywait();
		fl.confused = 0;
		devp->dsr = 0;
		devp->ccr = 0;
	}
}

/*
 *  seek to the target cylinder
 *
 *	interrupt, no results
 */
static long
floppyseek(Drive *dp, long off)
{
	floppypos(dp, off);
	if(dp->cyl == dp->tcyl)
		return dp->cyl;

/* print("seeking tcyl %d, thead %d\n", dp->tcyl, dp->thead); /**/
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Fseek;
	fl.cmd[fl.ncmd++] = (dp->thead<<2) | dp->dev;
	fl.cmd[fl.ncmd++] = dp->tcyl * dp->t->steps;
	if(floppycmd() < 0){
		print("seek cmd failed\n");
		return -1;
	}
	floppywait();
	if(fl.nstat < 2){
		fl.confused = 1;
		return -1;
	}
	if((fl.stat[0] & (Codemask|Seekend)) != Seekend){
		print("seek failed\n");
		dp->confused = 1;
		return -1;
	}

	dp->cyl = dp->tcyl;
	return dp->cyl;
}

/*
 *  since floppies are so flakey, try 3 times before giving up
 */
static long
floppythrice(Drive *dp, int cmd, void *a, long off, long n)
{
	int tries;
	long rv;

	for(tries = 0; ; tries++){
		if(waserror()){
			if(strcmp(u->error, Eintr)==0 || tries > 3)
				nexterror();
		} else {
			rv = floppyxfer(dp, cmd, a, off, n);
			poperror();
			return rv;
		}
	}
}

static long
floppyxfer(Drive *dp, int cmd, void *a, long off, long n)
{
	Device *devp = (Device*)(FDCTLRL);
	SCSIdma *dma = (SCSIdma *)SCSIDMA;
	long offset;
	ulong up;

	if(off >= dp->t->cap)
		return 0;
	if(off + n > dp->t->cap)
		n = dp->t->cap - off;

	/*
	 *  calculate new position and seek to it (dp->len may be trimmed)
	 */
	dp->len = n;
	if(floppyseek(dp, off) < 0)
		error(Eio);
	
/*print("tcyl %d, thead %d, tsec %d, addr %lux, buf %lux, n %d\n",
	dp->tcyl, dp->thead, dp->tsec, a, fl.physical, dp->len);/**/

	/*
	 *  turn off any previous dma, set up new one
	 */
	dma->csr = Dinit | Dcreset;
	if(cmd == Fread){
		up = (ulong)dp->physical;
	} else {
		memmove(fl.virtual, a, dp->len);
		up = (ulong)fl.physical;
	}
	dma->base = up;
	if(cmd == Fread){
		dma->limit = up + dp->len;
		SETCTL(Dmadir);
		dma->csr = Dseten | Dsetread | Dinit;
	} else {
		dma->limit = up + dp->len + 16;
		CLRCTL(Dmadir);
		dma->csr = Dseten | Dinit;
	}

	/*
	 *  start operation
	 */
/*	cmd = cmd | (dp->t->heads > 1 ? Fmulti : 0);/**/
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = cmd;
	fl.cmd[fl.ncmd++] = (dp->thead<<2) | dp->dev;
	fl.cmd[fl.ncmd++] = dp->tcyl * dp->t->steps;
	fl.cmd[fl.ncmd++] = dp->thead;
	fl.cmd[fl.ncmd++] = dp->tsec;
	fl.cmd[fl.ncmd++] = dp->t->bcode;
	fl.cmd[fl.ncmd++] = dp->tsec + dp->len/dp->t->bytes - 1;
	fl.cmd[fl.ncmd++] = dp->t->gpl;
	fl.cmd[fl.ncmd++] = 0xFF;
	splhi();
	if(floppycmd() < 0){
		spllo();
		print("xfer cmd failed\n");
		error(Eio);
	}

	/*
	 *  give bus to DMA, floppyintr() will read result
	 */
	SETCTL(Cpu_dma);
	spllo();
	floppywait();

	/*
	 *  check for errors
	 */
	if(fl.nstat < 7){
		print("xfer result failed %lux\n", devp->msr);
		fl.confused = 1;
		error(Eio);
	}
	if((fl.stat[0] & Codemask)!=0 || fl.stat[1] || fl.stat[2]){
		if(fl.stat[1] != 0x80){
			print("xfer failed %lux %lux %lux\n", fl.stat[0],
				fl.stat[1], fl.stat[2]);
			print("offset %lud len %d\n", off, dp->len);
			dp->confused = 1;
			error(Eio);
		} else
			fl.stat[5]++;
	}

	/*
	 *  check for correct cylinder
	 */
	offset = (fl.stat[3]/dp->t->steps) * dp->t->heads + fl.stat[4];
	offset = offset*dp->t->sectors + fl.stat[5] - 1;
	offset = offset * c2b[fl.stat[6]];
	if(offset != off+dp->len){
		print("new offset %d instead of %d\n", offset, off+dp->len);
		dp->confused = 1;
		error(Eio);
	}

	dp->lasttouched = m->ticks;
	return dp->len;
}

void
floppyintr(void)
{
	SCSIdma *dma = (SCSIdma *)SCSIDMA;
	Device *devp = (Device*)(FDCTLRL);

	switch(fl.cmd[0]&~Fmulti){
	case Fread:
	case Fwrite:
		if(dma->base != dma->limit)
			crankfifo(dma->limit - dma->base);
		CLRCTL(Cpu_dma);
		floppyresult();
		break;
	case Freadid:
		floppyresult();
		break;
	case Fseek:
	case Frecal:
	default:
		if(dmaowner != Ofloppy)
			devp->flpctl = Sel_82077;
		floppysense();	/* to clear interrupt */
		if(dmaowner != Ofloppy)
			devp->flpctl = 0;
		break;
	}
	fl.ncmd = 0;
	wakeup(&fl.r);
}
