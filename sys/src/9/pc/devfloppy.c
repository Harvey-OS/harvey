#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"devtab.h"

/* Intel 82077A (8272A compatible) floppy controller */

typedef	struct Drive		Drive;
typedef	struct Controller	Controller;
typedef struct Type		Type;

#define DPRINT if(floppydebug)kprint
int floppydebug;

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
	Fbusy=		0x10,	/* operation not over */

	/* data register */
	Pdata=		0x3f5,
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

	/* file types */
	Qdir=		0,
	Qdata=		(1<<2),
	Qctl=		(2<<2),
	Qmask=		(3<<2),

	DMAchan=	2,	/* floppy dma channel */
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

/*
 *  floppy types (all MFM encoding)
 */
struct Type
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
Type floppytype[] =
{
 { "3½HD",	T1440kb, 512, 18, 2, 1, 80, 0x1B, 0x54,	0, },
 { "3½DD",	T1440kb, 512,  9, 2, 1, 80, 0x1B, 0x54, 2, },
 { "3½DD",	T720kb,  512,  9, 2, 1, 80, 0x1B, 0x54, 2, },
 { "5¼HD",	T1200kb, 512, 15, 2, 1, 80, 0x2A, 0x50, 0, },
 { "5¼DD",	T1200kb, 512,  9, 2, 2, 40, 0x2A, 0x50, 1, },
 { "5¼DD",	T360kb,  512,  9, 2, 1, 40, 0x2A, 0x50, 2, },
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
	Type	*t;		/* floppy type */
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
struct Controller
{
	QLock;			/* exclusive access to the contoller */

	Drive	*d;		/* the floppy drives */
	Drive	*selected;
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

Controller	fl;

#define MOTORBIT(i)	(1<<((i)+4))

/*
 *  predeclared
 */
static int	floppycmd(void);
static void	floppyeject(Drive*);
static void	floppyintr(Ureg*);
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
static void	floppyformat(Drive*, char*);
static int	cmddone(void*);
void Xdelay(int);

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

static void
fldump(void)
{
	DPRINT("sra %ux srb %ux dor %ux msr %ux dir %ux\n", inb(Psra), inb(Psrb),
		inb(Pdor), inb(Pmsr), inb(Pdir));
}

/*
 *  set floppy drive to its default type
 */
static void
setdef(Drive *dp)
{
	Type *t;

	for(t = floppytype; t < &floppytype[NTYPES]; t++)
		if(dp->dt == t->dt){
			dp->t = t;
			floppydir[NFDIR*dp->dev].length = dp->t->cap;
			break;
		}
}

void
floppyreset(void)
{
	Drive *dp;
	Type *t;
	uchar equip;
	ulong maxtsize;

	/*
	 *  init dependent parameters
	 */
	maxtsize = 0;
	for(t = floppytype; t < &floppytype[NTYPES]; t++){
		t->cap = t->bytes * t->heads * t->sectors * t->tracks;
		t->bcode = b2c[t->bytes/128];
		t->tsize = t->bytes * t->sectors;
		if(maxtsize < t->tsize)
			maxtsize = t->tsize;
	}

	/*
	 *  allocate the drive storage
	 */
	fl.d = xalloc(conf.nfloppy*sizeof(Drive));
	fl.selected = fl.d;

	/*
	 *  stop the motors
	 */
	fl.motor = 0;
	delay(10);
	outb(Pdor, fl.motor | Fintena | Fena);
	delay(10);

	/*
	 *  init drives
	 */
	for(dp = fl.d; dp < &fl.d[conf.nfloppy]; dp++){
		dp->dev = dp - fl.d;
		dp->dt = T1440kb;
		setdef(dp);
		dp->cyl = -1;			/* because we don't know */
		dp->cache = (uchar*)xspanalloc(maxtsize, BY2PG, 64*1024);
		dp->ccyl = -1;
		dp->vers = 1;
	}

	/*
	 *  read nvram for types of floppies 0 & 1
	 */
	equip = nvramread(0x10);
	if(conf.nfloppy > 0){
		fl.d[0].dt = (equip >> 4) & 0xf;
		setdef(&fl.d[0]);
	}
	if(conf.nfloppy > 1){
		fl.d[1].dt = equip & 0xf;
		setdef(&fl.d[1]);
	}

	/*
	 *  first operation will recalibrate
	 */
	fl.confused = 1;
	setvec(Floppyvec, floppyintr);
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
islegal(ulong offset, long n, Drive *dp)
{
	if(offset % dp->t->bytes)
		error(Ebadarg);
	if(n % dp->t->bytes)
		error(Ebadarg);
}

/*
 *  check if the floppy has been replaced under foot.  cause
 *  an error if it has.
 *
 *  a seek and a read clears the condition.  this was determined
 *  experimentally, there has to be a better way.
 *
 *  if the read fails, cycle through the possible floppy
 *  density till one works or we've cycled through all
 *  possibilities for this drive.
 */
static void
changed(Chan *c, Drive *dp)
{
	ulong old;
	Type *start;

	/*
	 *  if floppy has changed or first time through
	 */
	if((inb(Pdir)&Fchange) || dp->vers == 0){
		DPRINT("changed\n");
		fldump();
		dp->vers++;
		setdef(dp);
		start = dp->t;
		dp->confused = 1;	/* make floppyon recal */
		floppyon(dp);
		floppyseek(dp, dp->t->heads*dp->t->tsize);
		while(waserror()){
			while(++dp->t){
				if(dp->t == &floppytype[NTYPES])
					dp->t = floppytype;
				if(dp->dt == dp->t->dt)
					break;
			}
			floppydir[NFDIR*dp->dev].length = dp->t->cap;
			floppyon(dp);
			DPRINT("changed: trying %s\n", dp->t->name);
			fldump();
			if(dp->t == start)
				nexterror();
		}
		floppyxfer(dp, Fread, dp->cache, 0, dp->t->tsize);
		poperror();
	}

	old = c->qid.vers;
	c->qid.vers = dp->vers;
	if(old && old != dp->vers)
		error(Eio);
}

long
floppyread(Chan *c, void *a, long n, ulong offset)
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
		islegal(offset, n, dp);
		aa = a;

		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			nexterror();
		}
		floppyon(dp);
		changed(c, dp);
		for(rv = 0; rv < n; rv += len){
			/*
			 *  truncate xfer at track boundary
			 */
			dp->len = n - rv;
			floppypos(dp, offset+rv);
			cyl = dp->tcyl;
			head = dp->thead;
			len = dp->len;
			sec = dp->tsec;
			nn = dp->t->tsize;

			/*
			 *  read the track
			 */
			if(dp->ccyl!=cyl || dp->chead!=head){
				dp->ccyl = -1;
				i = floppyxfer(dp, Fread, dp->cache,
					(cyl*dp->t->heads+head)*nn, nn);
				if(i != nn){
					if(i == 0)
						break;
					len = 0;
					continue;
				}
				dp->ccyl = cyl;
				dp->chead = head;
			}
			memmove(aa+rv, dp->cache + (sec-1)*dp->t->bytes, len);
		}
		qunlock(&fl);
		poperror();

		break;
	case Qctl:
		return readstr(offset, a, n, dp->t->name);
	default:
		panic("floppyread: bad qid");
	}

	return rv;
}

#define SNCMP(a, b) strncmp(a, b, sizeof(b)-1)
long
floppywrite(Chan *c, void *a, long n, ulong offset)
{
	Drive *dp;
	long rv, i;
	char *aa = a;
	char ctlmsg[64];

	rv = 0;
	dp = &fl.d[c->qid.path & ~Qmask];
	switch ((int)(c->qid.path & Qmask)) {
	case Qdata:
		islegal(offset, n, dp);
		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			nexterror();
		}
		floppyon(dp);
		changed(c, dp);
		for(rv = 0; rv < n; rv += i){
			floppypos(dp, offset+rv);
			if(dp->tcyl == dp->ccyl)
				dp->ccyl = -1;
			i = floppyxfer(dp, Fwrite, aa+rv, offset+rv, n-rv);
			if(i < 0)
				break;
			if(i == 0)
				error(Eio);
		}
		qunlock(&fl);
		poperror();
		break;
	case Qctl:
		rv = n;
		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			nexterror();
		}
		if(n >= sizeof(ctlmsg))
			n = sizeof(ctlmsg) - 1;
		memmove(ctlmsg, aa, n);
		ctlmsg[n] = 0;
		if(SNCMP(ctlmsg, "eject") == 0){
			floppyeject(dp);
		} else if(SNCMP(ctlmsg, "reset") == 0){
			fl.confused = 1;
			floppyon(dp);
		} else if(SNCMP(ctlmsg, "format") == 0){
			floppyformat(dp, ctlmsg);
		} else if(SNCMP(ctlmsg, "debug") == 0){
			floppydebug = 1;
		}
		poperror();
		qunlock(&fl);
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
				if(TK2SEC(m->ticks - dp->lasttouched) > 5)
					floppyoff(dp);
				qunlock(&fl);
			}
		}
		tsleep(&fl.kr, return0, 0, 1000);
	}
}

/*
 *  start a floppy drive's motor.
 */
static void
floppyon(Drive *dp)
{
	int alreadyon;
	int tries;

	if(fl.confused)
		floppyrevive();

	/* start motor and select drive */
	alreadyon = fl.motor & MOTORBIT(dp->dev);
	fl.motor |= MOTORBIT(dp->dev);
	outb(Pdor, fl.motor | Fintena | Fena | dp->dev);
	if(!alreadyon){
		/* wait for drive to spin up */
		tsleep(&dp->r, return0, 0, 750);

		/* clear any pending interrupts */
		setvec(Floppyvec, floppyintr);
		floppysense();
	}

	/* set transfer rate */
	if(fl.rate != dp->t->rate){
		fl.rate = dp->t->rate;
		outb(Pdsr, fl.rate);
	}

	/* get drive to a known cylinder */
	if(dp->confused)
		for(tries = 0; tries < 4; tries++)
			if(floppyrecal(dp) >= 0)
				break;
	dp->lasttouched = m->ticks;
	fl.selected = dp;
}

/*
 *  stop the floppy if it hasn't been used in 5 seconds
 */
static void
floppyoff(Drive *dp)
{
	fl.motor &= ~MOTORBIT(dp->dev);
	outb(Pdor, fl.motor | Fintena | Fena | dp->dev);
	fl.selected = dp;
}

/*
 *  eject disk ( unknown on safari )
 */
static void
floppyeject(Drive *dp)
{
	floppyon(dp);
	dp->vers++;
	floppyoff(dp);
}

/*
 *  send a command to the floppy
 */
static int
floppycmd(void)
{
	int i;
	int tries;

	fl.nstat = 0;
	for(i = 0; i < fl.ncmd; i++){
		for(tries = 0; ; tries++){
			if(tries > 1000){
				DPRINT("cmd %ux can't be sent (%d)\n", fl.cmd[0], i);
				fldump();

				/* empty fifo, might have been a bad command */
				floppyresult();
				return -1;
			}
			if((inb(Pmsr)&(Ffrom|Fready)) == Fready)
				break;
		}
		outb(Pdata, fl.cmd[i]);
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
	int i, s;
	int tries;

	/* get the result of the operation */
	for(i = 0; i < sizeof(fl.stat); i++){
		/* wait for status byte */
		for(tries = 0; ; tries++){
			if(tries > 1000){
				DPRINT("floppyresult: %d stats\n", i);
				fldump();
				fl.confused = 1;
				return -1;
			}
			s = inb(Pmsr)&(Ffrom|Fready);
			if(s == Fready){
				fl.nstat = i;
				return fl.nstat;
			}
			if(s == (Ffrom|Fready))
				break;
		}
		fl.stat[i] = inb(Pdata);
	}
	fl.nstat = sizeof(fl.stat);
	return fl.nstat;
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
		DPRINT("can't read sense response\n");
		fldump();
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
 *  Wait for a floppy interrupt.  If none occurs in 5 seconds, we
 *  may have missed one.  This only happens on some portables which
 *  do power management behind our backs.  Call the interrupt
 *  routine to try to clear any conditions.
 */
static void
floppywait(void)
{
	tsleep(&fl.r, cmddone, 0, 5000);
	if(!cmddone(0)){
		floppyintr(0);
		fl.confused = 1;
	}
}

/*
 *  we've lost the floppy position, go to cylinder 0.
 */
static int
floppyrecal(Drive *dp)
{
	dp->ccyl = -1;
	dp->cyl = -1;

	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Frecal;
	fl.cmd[fl.ncmd++] = dp->dev;
	if(floppycmd() < 0)
		return -1;
	floppywait();
	if(fl.nstat < 2){
		DPRINT("recalibrate: confused %ux\n", inb(Pmsr));
		fl.confused = 1;
		return -1;
	}
	if((fl.stat[0] & (Codemask|Seekend)) != Seekend){
		DPRINT("recalibrate: failed\n");
		dp->confused = 1;
		return -1;
	}
	dp->cyl = fl.stat[1];
	if(dp->cyl != 0){
		DPRINT("recalibrate: wrong cylinder %d\n", dp->cyl);
		dp->cyl = -1;
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
	Drive *dp;

	/*
	 *  reset the controller if it's confused
	 */
	if(fl.confused){
		DPRINT("floppyrevive in\n");
		fldump();

		/* reset controller and turn all motors off */
		splhi();
		fl.ncmd = 1;
		fl.cmd[0] = 0;
		outb(Pdor, 0);
		delay(10);
		outb(Pdor, Fintena|Fena);
		delay(10);
		spllo();
		fl.motor = 0;
		fl.confused = 0;
		floppywait();

		/* mark all drives in an unknown state */
		for(dp = fl.d; dp < &fl.d[conf.nfloppy]; dp++)
			dp->confused = 1;

		/* set rate to a known value */
		outb(Pdsr, 0);
		fl.rate = 0;

		DPRINT("floppyrevive out\n");
		fldump();
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
		return dp->tcyl;
	dp->cyl = -1;

	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Fseek;
	fl.cmd[fl.ncmd++] = (dp->thead<<2) | dp->dev;
	fl.cmd[fl.ncmd++] = dp->tcyl * dp->t->steps;
	if(floppycmd() < 0)
		return -1;
	floppywait();
	if(fl.nstat < 2){
		DPRINT("seek: confused\n");
		fl.confused = 1;
		return -1;
	}
	if((fl.stat[0] & (Codemask|Seekend)) != Seekend){
		DPRINT("seek: failed\n");
		dp->confused = 1;
		return -1;
	}

	dp->cyl = dp->tcyl;
	return dp->tcyl;
}

/*
 *  read or write to floppy.  try up to three times.
 */
static long
floppyxfer(Drive *dp, int cmd, void *a, long off, long n)
{
	long offset;
	int tries;

	if(off >= dp->t->cap)
		return 0;
	if(off + n > dp->t->cap)
		n = dp->t->cap - off;

	/* retry on error 3 times */
	tries = 0;
	while(waserror()){
		DPRINT("floppyxfer: retrying\n");
		if(tries++ >= 3)
			nexterror();
	}

	dp->len = n;
	if(floppyseek(dp, off) < 0){
		DPRINT("xfer: seek failed\n");
		dp->confused = 1;
		error(Eio);
	}

	/*
	 *  set up the dma (dp->len may be trimmed)
	 */
	if(waserror()){
		dmaend(DMAchan);
		nexterror();
	}
	dp->len = dmasetup(DMAchan, a, dp->len, cmd==Fread);

	/*
	 *  start operation
	 */
	cmd = cmd | (dp->t->heads > 1 ? Fmulti : 0);
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = cmd;
	fl.cmd[fl.ncmd++] = (dp->thead<<2) | dp->dev;
	fl.cmd[fl.ncmd++] = dp->tcyl;
	fl.cmd[fl.ncmd++] = dp->thead;
	fl.cmd[fl.ncmd++] = dp->tsec;
	fl.cmd[fl.ncmd++] = dp->t->bcode;
	fl.cmd[fl.ncmd++] = dp->t->sectors;
	fl.cmd[fl.ncmd++] = dp->t->gpl;
	fl.cmd[fl.ncmd++] = 0xFF;
	if(floppycmd() < 0)
		error(Eio);

	/*
	 *  give bus to DMA, floppyintr() will read result
	 */
	floppywait();
	dmaend(DMAchan);
	poperror();

	/*
	 *  check for errors
	 */
	if(fl.nstat < 7){
		DPRINT("xfer: confused\n");
		fl.confused = 1;
		error(Eio);
	}
	if((fl.stat[0] & Codemask)!=0 || fl.stat[1] || fl.stat[2]){
		DPRINT("xfer: failed %lux %lux %lux\n", fl.stat[0],
			fl.stat[1], fl.stat[2]);
		DPRINT("offset %lud len %d\n", off, dp->len);
		dp->confused = 1;
		error(Eio);
	}

	/*
	 *  check for correct cylinder
	 */
	offset = fl.stat[3] * dp->t->heads + fl.stat[4];
	offset = offset*dp->t->sectors + fl.stat[5] - 1;
	offset = offset * c2b[fl.stat[6]];
	if(offset != off+dp->len){
		DPRINT("xfer: ends on wrong cyl\n");
		dp->confused = 1;
		error(Eio);
	}
	poperror();

	dp->lasttouched = m->ticks;
	return dp->len;
}

/*
 *  format a track
 */
static void
floppyformat(Drive *dp, char *params)
{
 	int cyl, h, sec;
	ulong track;
	uchar *buf, *bp;
	Type *t;
	char *f[3];

	/*
	 *  set the type
	 */
	if(getfields(params, f, 3, ' ') > 1){
		for(t = floppytype; t < &floppytype[NTYPES]; t++)
			if(strcmp(f[1], t->name)==0 && t->dt==dp->dt){
				dp->t = t;
				floppydir[NFDIR*dp->dev].length = dp->t->cap;
				break;
			}
	} else {
		setdef(dp);
		t = dp->t;
	}

	/*
	 *  buffer for per track info
	 */
	buf = smalloc(t->sectors*4);
	if(waserror()){
		free(buf);
		nexterror();
	}

	/* force a recalibrate to cylinder 0 */
	dp->confused = 1;
	if(!waserror()){
		floppyon(dp);
		poperror();
	}

	/*
	 *  format a track at time
	 */
	for(track = 0; track < t->tracks*t->heads; track++){
		cyl = track/t->heads;
		h = track % t->heads;

		/*
		 *  seek to track, ignore errors
		 */
		floppyseek(dp, track*t->tsize);
		dp->cyl = cyl;
		dp->confused = 0;

		/*
		 *  set up the dma (dp->len may be trimmed)
		 */
		bp = buf;
		for(sec = 1; sec <= t->sectors; sec++){
			*bp++ = cyl;
			*bp++ = h;
			*bp++ = sec;
			*bp++ = t->bcode;
		}
		if(waserror()){
			dmaend(DMAchan);
			nexterror();
		}
		dmasetup(DMAchan, buf, bp-buf, 0);

		/*
		 *  start operation
		 */
		fl.ncmd = 0;
		fl.cmd[fl.ncmd++] = Fformat;
		fl.cmd[fl.ncmd++] = (h<<2) | dp->dev;
		fl.cmd[fl.ncmd++] = t->bcode;
		fl.cmd[fl.ncmd++] = t->sectors;
		fl.cmd[fl.ncmd++] = t->fgpl;
		fl.cmd[fl.ncmd++] = 0x5a;
		if(floppycmd() < 0)
			error(Eio);

		/*
		 *  give bus to DMA, floppyintr() will read result
		 */
		floppywait();
		dmaend(DMAchan);
		poperror();

		/*
		 *  check for errors
		 */
		if(fl.nstat < 7){
			DPRINT("format: confused\n");
			fl.confused = 1;
			error(Eio);
		}
		if((fl.stat[0]&Codemask)!=0 || fl.stat[1]|| fl.stat[2]){
			DPRINT("format: failed %lux %lux %lux\n",
				fl.stat[0], fl.stat[1], fl.stat[2]);
			dp->confused = 1;
			error(Eio);
		}
	}
	free(buf);
	dp->confused = 1;
	poperror();
}

static void
floppyintr(Ureg *ur)
{
	USED(ur);
	switch(fl.cmd[0]&~Fmulti){
	case Fread:
	case Fwrite:
	case Fformat:
		floppyresult();
		break;
	case Fseek:
	case Frecal:
	default:
		floppysense();	/* to clear interrupt */
		break;
	}
	fl.ncmd = 0;
	wakeup(&fl.r);
}
