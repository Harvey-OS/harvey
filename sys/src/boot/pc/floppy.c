#include <u.h>

#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

#define DPRINT if(0)print

typedef	struct Drive	Drive;
typedef	struct Floppy	Floppy;
typedef struct Type	Type;

enum
{
	Pdor=		0x3f2,	/* motor port */
	 Fintena=	 0x4,	/* enable floppy interrupt */
	 Fena=		 0x8,	/* 0 == reset controller */

	Pstatus=	0x3f4,	/* controller main status port */
	 Fready=	 0x80,	/* ready to be touched */
	 Ffrom=		 0x40,	/* data from controller */
	 Fbusy=		 0x10,	/* operation not over */

	Pdata=		0x3f5,	/* controller data port */
	 Frecal=	 0x7,	/* recalibrate cmd */
	 Fseek=		 0xf,	/* seek cmd */
	 Fsense=	 0x8,	/* sense cmd */
	 Fread=		 0x66,	/* read cmd */
	 Fwrite=	 0x47,	/* write cmd */
	 Fmulti=	 0x80,	/* or'd with Fread or Fwrite for multi-head */

	/* digital input register */
	Pdir=		0x3F7,	/* disk changed port (read only) */
	Pdsr=		0x3F7,	/* data rate select port (write only) */
	Fchange=	0x80,	/* disk has changed */

	DMAmode0=	0xb,
	DMAmode1=	0xc,
	DMAaddr=	0x4,
	DMAtop=		0x81,
	DMAinit=	0xa,
	DMAcount=	0x5,

	Maxfloppy=	4,	/* floppies/controller */

	/* sector size encodings */
	S128=		0,
	S256=		1,
	S512=		2,
	S1024=		3,

	/* status 0 byte */
	Drivemask=	3<<0,
	Seekend=	1<<5,
	Codemask=	(3<<6)|(3<<3),
};

#define MOTORBIT(i)	(1<<((i)+4))

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
};
Type floppytype[] =
{
	"MF2HD",	S512,	18,	2,	1,	80,	0x1B,	0x54,
	"MF2DD",	S512,	9,	2,	1,	80,	0x1B,	0x54,
	"F2HD",		S512,	15,	2,	1,	80,	0x2A,	0x50,
	"F2DD",		S512,	8,	2,	1,	40,	0x2A,	0x50,
	"F1DD",		S512,	8,	1,	1,	40,	0x2A,	0x50,
};
static int secbytes[] =
{
	128,
	256,
	512,
	1024
};

/*
 *  a floppy drive
 */
struct Drive
{
	ulong	lasttouched;	/* time last touched */
	int	cyl;		/* current cylinder */
	long	offset;		/* current offset */
	int	confused;	/* needs to be recalibrated (or worse) */
	int	dev;

	Type	*t;

	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	long	len;

};

/*
 *  NEC PD765A controller for 4 floppys
 */
struct Floppy
{
	Lock;

	Drive	d[Maxfloppy];	/* the floppy drives */
	int	rw;		/* true if a read or write in progress */
	int	seek;		/* one bit for each seek in progress */
	uchar	stat[8];	/* status of an operation */
	int	intr;
	int	confused;
	int	motor;
	Drive	*selected;

	int 	cdev;
	uchar	*ccache;		/* cyclinder cache */
	int	ccyl;
	int	chead;
};

Floppy	fl;

static void	floppyalarm(Alarm*);
static int	floppysend(int);
static int	floppyrcv(void);
static int	floppyrdstat(int);
static void	floppypos(Drive*, long);
static void	floppywait(char*);
static int	floppysense(Drive*);
static int	floppyrecal(Drive*);
static void	floppyon(Drive*);
static long	floppyxfer(Drive*, int, void*, long);
static void	floppyintr(Ureg*);
static void	floppyrevive(void);
static void	floppystop(Drive*);

static void
timedsleep(int ms)
{
	ulong end;

	end = m->ticks + 1 + MS2TK(ms);
	while(m->ticks < end)
		;
}

int
floppyinit(void)
{
	Drive *dp;

	delay(10);
	outb(Pdor, Fintena | Fena);
	delay(10);
	for(dp = fl.d; dp < &fl.d[Maxfloppy]; dp++){
		dp->t = &floppytype[0];
		dp->cyl = -1;
		dp->dev = dp - fl.d;
	}
	fl.motor = 0;
	fl.confused = 1;
	fl.ccyl = -1;
	fl.chead = -1;
	fl.cdev = -1;
	fl.ccache = (uchar*)ialloc(18*2*512, 1);

	setvec(Floppyvec, floppyintr);
	alarm(5*1000, floppyalarm, (void *)0);
	return 1;
}

static void
floppyon(Drive *dp)
{
	int alreadyon;
	int tries;

	if(fl.confused)
		floppyrevive();

	alreadyon = fl.motor & MOTORBIT(dp->dev);
	fl.motor |= MOTORBIT(dp->dev);
	outb(Pdor, fl.motor | Fintena | Fena | dp->dev);

	/* get motor going */
	if(!alreadyon)
		timedsleep(750);

	/* get drive to a known cylinder */
	if(dp->confused)
		for(tries = 0; tries < 4; tries++)
			if(floppyrecal(dp) >= 0)
				break;

	dp->lasttouched = m->ticks;
	fl.selected = dp;
}

static void
floppyrevive(void)
{
	Drive *dp;

	/*
	 *  reset the controller if it's confused
	 */
	if(fl.confused){
		/* reset controller and turn all motors off */
		fl.intr = 0;
		splhi();
		outb(Pdor, 0);
		delay(1);
		outb(Pdor, Fintena|Fena);
		spllo();
		for(dp = fl.d; dp < &fl.d[Maxfloppy]; dp++)
			dp->confused = 1;
		fl.motor = 0;
		floppywait("revive");
		fl.confused = 0;
		outb(Pdsr, 0);
	}
}

static void
floppystop(Drive *dp)
{
	fl.motor &= ~MOTORBIT(dp->dev);
	outb(Pdor, fl.motor | Fintena | Fena | dp->dev);
	fl.selected = dp;
}

static void
floppyalarm(Alarm* a)
{
	Drive *dp;

	for(dp = fl.d; dp < &fl.d[Maxfloppy]; dp++){
		if((fl.motor&MOTORBIT(dp->dev)) && TK2SEC(m->ticks - dp->lasttouched) > 5)
			floppystop(dp);
	}

	alarm(5*1000, floppyalarm, 0);
	cancel(a);
}

static int
floppysend(int data)
{
	int tries;
	uchar c;

	for(tries = 0; tries < 100; tries++){
		/*
		 *  see if its ready for data
		 */
		c = inb(Pstatus);
		if((c&(Ffrom|Fready)) != Fready)
			continue;

		/*
		 *  send the data
		 */
		outb(Pdata, data);
		return 0;
	}
	return -1;
}

static int
floppyrcv(void)
{
	int tries;
	uchar c;

	for(tries = 0; tries < 100; tries++){
		/*
		 *  see if its ready for data
		 */
		c = inb(Pstatus);
		if((c&(Ffrom|Fready)) != (Ffrom|Fready))
			continue;

		/*
		 *  get data
		 */
		return inb(Pdata)&0xff;
	}
	DPRINT("floppyrcv returns -1 status = %lux\n", c);
	return -1;
}

static int
floppyrdstat(int n)
{
	int i;
	int c;

	for(i = 0; i < n; i++){
		c = floppyrcv();
		if(c < 0)
			return -1;
		fl.stat[i] = c;
	}
	return 0;
}

static void
floppypos(Drive *dp, long off)
{
	int lsec;
	int cyl;

	lsec = off/secbytes[dp->t->bytes];
	dp->tcyl = lsec/(dp->t->sectors*dp->t->heads);
	dp->tsec = (lsec % dp->t->sectors) + 1;
	dp->thead = (lsec/dp->t->sectors) % dp->t->heads;

	/*
	 *  can't read across cylinder boundaries.
	 *  if so, decrement the bytes to be read.
	 */
	lsec = (off+dp->len)/secbytes[dp->t->bytes];
	cyl = lsec/(dp->t->sectors*dp->t->heads);
	if(cyl != dp->tcyl){
		dp->len -= (lsec % dp->t->sectors)*secbytes[dp->t->bytes];
		dp->len -= ((lsec/dp->t->sectors) % dp->t->heads)*secbytes[dp->t->bytes]
				*dp->t->sectors;
	}

	dp->lasttouched = m->ticks;	
	fl.intr = 0;
}

static void
floppywait(char *cmd)
{
	ulong start;

	for(start = m->ticks; TK2SEC(m->ticks - start) < 1 && fl.intr == 0;)
		;
	if(TK2SEC(m->ticks - start) >= 3)
		print("floppy timed out, cmd=%s\n", cmd);
	fl.intr = 0;
}

static int
floppysense(Drive *dp)
{
	/*
	 *  ask for floppy status
	 */
	if(floppysend(Fsense) < 0){
		fl.confused = 1;
		return -1;
	}
	if(floppyrdstat(2) < 0){
		fl.confused = 1;
		dp->confused = 1;
		return -1;
	}

	/*
	 *  make sure it's the right drive
	 */
	if((fl.stat[0] & Drivemask) != dp->dev){
		DPRINT("sense failed, %lux %lux\n", fl.stat[0], fl.stat[1]);
		dp->confused = 1;
		return -1;
	}
	return 0;
}

static int
floppyrecal(Drive *dp)
{
	fl.intr = 0;
	if(floppysend(Frecal) < 0
	|| floppysend(dp - fl.d) < 0){
		DPRINT("recalibrate rejected\n");
		fl.confused = 0;
		return -1;
	}
	floppywait("recal");

	/*
	 *  get return values
	 */
	if(floppysense(dp) < 0)
		return -1;

	/*
	 *  see what cylinder we got to
	 */
	dp->tcyl = 0;
	dp->cyl = fl.stat[1]/dp->t->steps;
	if(dp->cyl != dp->tcyl){
		DPRINT("recalibrate went to wrong cylinder %d\n", dp->cyl);
		dp->confused = 1;
		return -1;
	}

	dp->confused = 0;
	return 0;
}

long
floppyseek(int dev, long off)
{
	Drive *dp;

	dp = &fl.d[dev];
	floppyon(dp);
	floppypos(dp, off);
	if(dp->cyl == dp->tcyl){
		dp->offset = off;
		return off;
	}

	/*
	 *  tell floppy to seek
	 */
	if(floppysend(Fseek) < 0
	|| floppysend((dp->thead<<2) | dp->dev) < 0
	|| floppysend(dp->tcyl * dp->t->steps) < 0){
		DPRINT("seek cmd failed\n");
		fl.confused = 1;
		return -1;
	}

	/*
	 *  wait for interrupt
	 */
	floppywait("seek");

	/*
	 *  get floppy status
	 */
	if(floppysense(dp) < 0)
		return -1;

	/*
 	 *  see if it worked
	 */
	if((fl.stat[0] & (Codemask|Seekend)) != Seekend){
		DPRINT("seek failed\n");
		dp->confused = 1;
		return -1;
	}

	/*
	 *  see what cylinder we got to
	 */
	dp->cyl = fl.stat[1]/dp->t->steps;
	if(dp->cyl != dp->tcyl){
		DPRINT("seek went to wrong cylinder %d instead of %d\n",
			dp->cyl, dp->tcyl);
		dp->confused = 1;
		return -1;
	}

	dp->offset = off;
	DPRINT("seek to %d succeeded\n", dp->offset);
	return dp->offset;
}

static long
floppyxfer(Drive *dp, int cmd, void *a, long n)
{
	ulong addr;
	long offset;

	addr = (ulong)a;

	/*
	 *  dma can't cross 64 k boundaries
	 */
	if((addr & 0xffff0000) != ((addr+n) & 0xffff0000))
		n -= (addr+n)&0xffff;

	dp->len = n;
	if(floppyseek(dp->dev, dp->offset) < 0){
		DPRINT("xfer seek failed\n");
		return -1;
	}

	DPRINT("floppy %d tcyl %d, thead %d, tsec %d, addr %lux, n %d\n",
		dp->dev, dp->tcyl, dp->thead, dp->tsec, addr, n);/**/

	/*
	 *  set up the dma
	 */
	outb(DMAmode1, cmd==Fread ? 0x46 : 0x4a);
	outb(DMAmode0, cmd==Fread ? 0x46 : 0x4a);
	outb(DMAaddr, addr);
	outb(DMAaddr, addr>>8);
	outb(DMAtop, addr>>16);
	outb(DMAcount, n-1);
	outb(DMAcount, (n-1)>>8);
	outb(DMAinit, 2);

	/*
	 *  tell floppy to go
	 */
	cmd = cmd | (dp->t->heads > 1 ? Fmulti : 0);
	if(floppysend(cmd) < 0
	|| floppysend((dp->thead<<2) | dp->dev) < 0
	|| floppysend(dp->tcyl * dp->t->steps) < 0
	|| floppysend(dp->thead) < 0
	|| floppysend(dp->tsec) < 0
	|| floppysend(dp->t->bytes) < 0
	|| floppysend(dp->t->sectors) < 0
	|| floppysend(dp->t->gpl) < 0
	|| floppysend(0xFF) < 0){
		DPRINT("xfer cmd failed\n");
		fl.confused = 1;
		return -1;
	}

	floppywait("xfer");

	/*
	 *  get status
 	 */
	if(floppyrdstat(7) < 0){
		DPRINT("xfer status failed\n");
		fl.confused = 1;
		return -1;
	}

	if((fl.stat[0] & Codemask)!=0 || fl.stat[1] || fl.stat[2]){
		DPRINT("xfer failed %lux %lux %lux\n", fl.stat[0],
			fl.stat[1], fl.stat[2]);
		dp->confused = 1;
		return -1;
	}

	offset = (fl.stat[3]/dp->t->steps) * dp->t->heads + fl.stat[4];
	offset = offset*dp->t->sectors + fl.stat[5] - 1;
	offset = offset * secbytes[fl.stat[6]];
	if(offset != dp->offset+n){
		DPRINT("new offset %d instead of %d\n", offset, dp->offset+dp->len);
		dp->confused = 1;
		return -1;/**/
	}
	dp->offset += dp->len;
	return dp->len;
}

long
floppyread(int dev, void *a, long n)
{
	Drive *dp;
	long rv, i, nn, offset, sec;
	uchar *aa;
	int tries;

	dp = &fl.d[dev];

	dp->len = n;
	floppypos(dp, dp->offset);
	offset = dp->offset;
	sec = dp->tsec + dp->t->sectors*dp->thead;
	n = dp->len;
	if(fl.ccyl==dp->tcyl && fl.cdev==dev)
		goto out;

	fl.ccyl = -1;
	fl.cdev = dev;
	aa = fl.ccache;
	nn = secbytes[dp->t->bytes]*dp->t->sectors*dp->t->heads;
	dp->offset = dp->tcyl*nn;
	for(rv = 0; rv < nn; rv += i){
		i = 0;
		for(tries = 0; tries < 2; tries++){
			i = floppyxfer(dp, Fread, aa+rv, nn-rv);
			if(i > 0)
				break;
		}
		if(tries == 2)
			break;
	}
	if(rv != nn){
		dp->confused = 1;
		return -1;
	}
	fl.ccyl = dp->tcyl;
out:
	memmove(a, fl.ccache + secbytes[dp->t->bytes]*(sec-1), n);
	dp->offset = offset + n;
	return n;
}

static void
floppyintr(Ureg *ur)
{
	USED(ur);
	fl.intr = 1;
}
