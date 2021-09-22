#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"floppy.h"

/* Intel 82077A (8272A compatible) floppy controller */

/* This module expects the following functions to be defined
 * elsewhere: 
 * 
 * inb()
 * outb()
 * floppyexec()
 * floppyeject() 
 * floppysetup0()
 * floppysetup1()
 * dmainit()
 * dmasetup()
 * dmaend()
 * 
 * On DMA systems, floppyexec() should be an empty function; 
 * on non-DMA systems, dmaend() should be an empty function; 
 * dmasetup() may enforce maximum transfer sizes. 
 */

enum {
	/* file types */
	Qdir=		0, 
	Qdata=		(1<<2),
	Qctl=		(2<<2),
	Qmask=		(3<<2),

	DMAchan=	2,	/* floppy dma channel */
};

#define DPRINT if(floppydebug)print
int floppydebug = 0;

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

FType floppytype[] =
{
 { "3½HD",	T1440kb, 512, 18, 2, 1, 80, 0x1B, 0x54,	0, },
 { "3½DD",	T1440kb, 512,  9, 2, 1, 80, 0x1B, 0x54, 2, },
 { "3½DD",	T720kb,  512,  9, 2, 1, 80, 0x1B, 0x54, 2, },
 { "5¼HD",	T1200kb, 512, 15, 2, 1, 80, 0x2A, 0x50, 0, },
 { "5¼DD",	T1200kb, 512,  9, 2, 2, 40, 0x2A, 0x50, 1, },
 { "ATT3B1",	T1200kb, 512,  8, 2, 2, 48, 0x2A, 0x50, 1, },
 { "5¼DD",	T360kb,  512,  9, 2, 1, 40, 0x2A, 0x50, 2, },
};

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

FController	fl;

#define MOTORBIT(i)	(1<<((i)+4))

/*
 *  predeclared
 */
static int	cmddone(void*);
static void	floppyformat(FDrive*, Cmdbuf*);
static void	floppykproc(void*);
static void	floppypos(FDrive*,long);
static int	floppyrecal(FDrive*);
static int	floppyresult(void);
static void	floppyrevive(void);
static long	floppyseek(FDrive*, long);
static int	floppysense(void);
static void	floppywait(int);
static long	floppyxfer(FDrive*, int, void*, long, long);

Dirtab floppydir[]={
	".",		{Qdir, 0, QTDIR},	0,	0550,
	"fd0disk",		{Qdata + 0},	0,	0660,
	"fd0ctl",		{Qctl + 0},	0,	0660,
	"fd1disk",		{Qdata + 1},	0,	0660,
	"fd1ctl",		{Qctl + 1},	0,	0660,
	"fd2disk",		{Qdata + 2},	0,	0660,
	"fd2ctl",		{Qctl + 2},	0,	0660,
	"fd3disk",		{Qdata + 3},	0,	0660,
	"fd3ctl",		{Qctl + 3},	0,	0660,
};
#define NFDIR	2	/* directory entries/drive */

enum
{
	CMdebug,
	CMnodebug,
	CMeject,
	CMformat,
	CMreset,
};

static Cmdtab floppyctlmsg[] =
{
	CMdebug,	"debug",	1,
	CMnodebug,	"nodebug", 1,
	CMeject,	"eject",	1,
	CMformat,	"format",	0,
	CMreset,	"reset",	1,
};

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
floppysetdef(FDrive *dp)
{
	FType *t;

	for(t = floppytype; t < &floppytype[nelem(floppytype)]; t++)
		if(dp->dt == t->dt){
			dp->t = t;
			floppydir[1+NFDIR*dp->dev].length = dp->t->cap;
			break;
		}
}

static void
floppyreset(void)
{
	FDrive *dp;
	FType *t;
	ulong maxtsize;
	
	floppysetup0(&fl);
	if(fl.ndrive == 0)
		return;

	/*
	 *  init dependent parameters
	 */
	maxtsize = 0;
	for(t = floppytype; t < &floppytype[nelem(floppytype)]; t++){
		t->cap = t->bytes * t->heads * t->sectors * t->tracks;
		t->bcode = b2c[t->bytes/128];
		t->tsize = t->bytes * t->sectors;
		if(maxtsize < t->tsize)
			maxtsize = t->tsize;
	}

	/*
	 * Should check if this fails. Can do so
	 * if there is no space <= 16MB for the DMA
	 * bounce buffer.
	 */
	dmainit(DMAchan, maxtsize);

	/*
	 *  allocate the drive storage
	 */
	fl.d = xalloc(fl.ndrive*sizeof(FDrive));
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
	for(dp = fl.d; dp < &fl.d[fl.ndrive]; dp++){
		dp->dev = dp - fl.d;
		dp->dt = T1440kb;
		floppysetdef(dp);
		dp->cyl = -1;			/* because we don't know */
		dp->cache = (uchar*)xspanalloc(maxtsize, BY2PG, 64*1024);
		dp->ccyl = -1;
		dp->vers = 0;
	}

	/*
	 *  first operation will recalibrate
	 */
	fl.confused = 1;

	floppysetup1(&fl);
}

static Chan*
floppyattach(char *spec)
{
	static int kstarted;

	if(fl.ndrive == 0)
		error(Enodev);

	if(kstarted == 0){
		/*
		 *  watchdog to turn off the motors
		 */
		kstarted = 1;
		kproc("floppy", floppykproc, 0);
	}
	return devattach('f', spec);
}

static Walkqid*
floppywalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, floppydir, 1+fl.ndrive*NFDIR, devgen);
}

static int
floppystat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, floppydir, 1+fl.ndrive*NFDIR, devgen);
}

static Chan*
floppyopen(Chan *c, int omode)
{
	return devopen(c, omode, floppydir, 1+fl.ndrive*NFDIR, devgen);
}

static void
floppyclose(Chan *)
{
}

static void
islegal(ulong offset, long n, FDrive *dp)
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
changed(Chan *c, FDrive *dp)
{
	ulong old;
	FType *start;

	/*
	 *  if floppy has changed or first time through
	 */
	if((inb(Pdir)&Fchange) || dp->vers == 0){
		DPRINT("changed\n");
		fldump();
		dp->vers++;
		start = dp->t;
		dp->maxtries = 3;	/* limit it when we're probing */

		/* floppyon will fail if there's a controller but no drive */
		dp->confused = 1;	/* make floppyon recal */
		if(floppyon(dp) < 0)
			error(Eio);

		/* seek to the first track */
		floppyseek(dp, dp->t->heads*dp->t->tsize);
		while(waserror()){
			/*
			 *  if first attempt doesn't reset changed bit, there's
			 *  no floppy there
			 */
			if(inb(Pdir)&Fchange)
				nexterror();

			while(++dp->t){
				if(dp->t == &floppytype[nelem(floppytype)])
					dp->t = floppytype;
				if(dp->dt == dp->t->dt)
					break;
			}
			floppydir[1+NFDIR*dp->dev].length = dp->t->cap;

			/* floppyon will fail if there's a controller but no drive */
			if(floppyon(dp) < 0)
				error(Eio);

			DPRINT("changed: trying %s\n", dp->t->name);
			fldump();
			if(dp->t == start)
				nexterror();
		}

		/* if the read succeeds, we've got the density right */
		floppyxfer(dp, Fread, dp->cache, 0, dp->t->tsize);
		poperror();
		dp->maxtries = 20;
	}

	old = c->qid.vers;
	c->qid.vers = dp->vers;
	if(old && old != dp->vers)
		error(Eio);
}

static int
readtrack(FDrive *dp, int cyl, int head)
{
	int i, nn, sofar;
	ulong pos;

	nn = dp->t->tsize;
	if(dp->ccyl==cyl && dp->chead==head)
		return nn;
	pos = (cyl*dp->t->heads+head) * nn;
	for(sofar = 0; sofar < nn; sofar += i){
		dp->ccyl = -1;
		i = floppyxfer(dp, Fread, dp->cache + sofar, pos + sofar, nn - sofar);
		if(i <= 0)
			return -1;
	}
	dp->ccyl = cyl;
	dp->chead = head;
	return nn;
}

static long
floppyread(Chan *c, void *a, long n, vlong off)
{
	FDrive *dp;
	long rv;
	int sec, head, cyl;
	long len;
	uchar *aa;
	ulong offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, floppydir, 1+fl.ndrive*NFDIR, devgen);

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
			 *  all xfers come out of the track cache
			 */
			dp->len = n - rv;
			floppypos(dp, offset+rv);
			cyl = dp->tcyl;
			head = dp->thead;
			len = dp->len;
			sec = dp->tsec;
			if(readtrack(dp, cyl, head) < 0)
				break;
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

static long
floppywrite(Chan *c, void *a, long n, vlong off)
{
	FDrive *dp;
	long rv, i;
	char *aa = a;
	Cmdbuf *cb;
	Cmdtab *ct;
	ulong offset = off;

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
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		qlock(&fl);
		if(waserror()){
			qunlock(&fl);
			nexterror();
		}
		ct = lookupcmd(cb, floppyctlmsg, nelem(floppyctlmsg));
		switch(ct->index){
		case CMeject:
			floppyeject(dp);
			break;
		case CMformat:
			floppyformat(dp, cb);
			break;
		case CMreset:
			fl.confused = 1;
			floppyon(dp);
			break;
		case CMdebug:
			floppydebug = 1;
			break;
		case CMnodebug:
			floppydebug = 0;
			break;
		}
		poperror();
		qunlock(&fl);
		poperror();
		free(cb);
		break;
	default:
		panic("floppywrite: bad qid");
	}

	return rv;
}

static void
floppykproc(void *)
{
	FDrive *dp;

	while(waserror())
		;
	for(;;){
		for(dp = fl.d; dp < &fl.d[fl.ndrive]; dp++){
			if((fl.motor&MOTORBIT(dp->dev))
			&& TK2SEC(m->ticks - dp->lasttouched) > 5
			&& canqlock(&fl)){
				if(TK2SEC(m->ticks - dp->lasttouched) > 5)
					floppyoff(dp);
				qunlock(&fl);
			}
		}
		tsleep(&up->sleep, return0, 0, 1000);
	}
}

/*
 *  start a floppy drive's motor.
 */
static int
floppyon(FDrive *dp)
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
		tsleep(&up->sleep, return0, 0, 750);

		/* clear any pending interrupts */
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

	/* return -1 if this didn't work */
	if(dp->confused)
		return -1;
	return 0;
}

/*
 *  stop the floppy if it hasn't been used in 5 seconds
 */
static void
floppyoff(FDrive *dp)
{
	fl.motor &= ~MOTORBIT(dp->dev);
	outb(Pdor, fl.motor | Fintena | Fena | dp->dev);
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
			if((inb(Pmsr)&(Ffrom|Fready)) == Fready)
				break;
			if(tries > 1000){
				DPRINT("cmd %ux can't be sent (%d)\n", fl.cmd[0], i);
				fldump();

				/* empty fifo, might have been a bad command */
				floppyresult();
				return -1;
			}
			microdelay(8);	/* for machine independence */
		}
		outb(Pfdata, fl.cmd[i]);
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
			s = inb(Pmsr)&(Ffrom|Fready);
			if(s == Fready){
				fl.nstat = i;
				return fl.nstat;
			}
			if(s == (Ffrom|Fready))
				break;
			if(tries > 1000){
				DPRINT("floppyresult: %d stats\n", i);
				fldump();
				fl.confused = 1;
				return -1;
			}
			microdelay(8);	/* for machine independence */
		}
		fl.stat[i] = inb(Pfdata);
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
floppypos(FDrive *dp, long off)
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
cmddone(void *)
{
	return fl.ncmd == 0;
}

/*
 *  Wait for a floppy interrupt.  If none occurs in 5 seconds, we
 *  may have missed one.  This only happens on some portables which
 *  do power management behind our backs.  Call the interrupt
 *  routine to try to clear any conditions.
 */
static void
floppywait(int slow)
{
	tsleep(&fl.r, cmddone, 0, slow ? 5000 : 1000);
	if(!cmddone(0)){
		floppyintr(0);
		fl.confused = 1;
	}
}

/*
 *  we've lost the floppy position, go to cylinder 0.
 */
static int
floppyrecal(FDrive *dp)
{
	dp->ccyl = -1;
	dp->cyl = -1;

	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = Frecal;
	fl.cmd[fl.ncmd++] = dp->dev;
	if(floppycmd() < 0)
		return -1;
	floppywait(1);
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
 *  reset it and get back to a known state
 */
static void
floppyrevive(void)
{
	FDrive *dp;

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
		floppywait(0);

		/* mark all drives in an unknown state */
		for(dp = fl.d; dp < &fl.d[fl.ndrive]; dp++)
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
floppyseek(FDrive *dp, long off)
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
	floppywait(1);
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
floppyxfer(FDrive *dp, int cmd, void *a, long off, long n)
{
	long offset;
	int tries;

	if(off >= dp->t->cap)
		return 0;
	if(off + n > dp->t->cap)
		n = dp->t->cap - off;

	/* retry on error (until it gets ridiculous) */
	tries = 0;
	while(waserror()){
		if(tries++ >= dp->maxtries)
			nexterror();
		DPRINT("floppyxfer: retrying\n");
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
	if(dp->len < 0)
		error(Eio);

	/*
	 *  start operation
	 */
	fl.ncmd = 0;
	fl.cmd[fl.ncmd++] = cmd | (dp->t->heads > 1 ? Fmulti : 0);
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

	/* Poll ready bits and transfer data */
	floppyexec((char*)a, dp->len, cmd==Fread);

	/*
	 *  give bus to DMA, floppyintr() will read result
	 */
	floppywait(0);
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
		DPRINT("xfer: failed %ux %ux %ux\n", fl.stat[0],
			fl.stat[1], fl.stat[2]);
		DPRINT("offset %lud len %ld\n", off, dp->len);
		if((fl.stat[0]&Codemask)==Cmdexec && fl.stat[1]==Overrun){
			DPRINT("DMA overrun: retry\n");
		} else
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
floppyformat(FDrive *dp, Cmdbuf *cb)
{
 	int cyl, h, sec;
	ulong track;
	uchar *buf, *bp;
	FType *t;

	/*
	 *  set the type
	 */
	if(cb->nf == 2){
		for(t = floppytype; t < &floppytype[nelem(floppytype)]; t++){
			if(strcmp(cb->f[1], t->name)==0 && t->dt==dp->dt){
				dp->t = t;
				floppydir[1+NFDIR*dp->dev].length = dp->t->cap;
				break;
			}
		}
		if(t >= &floppytype[nelem(floppytype)])
			error(Ebadarg);
	} else if(cb->nf == 1){
		floppysetdef(dp);
		t = dp->t;
	} else {
		cmderror(cb, "invalid floppy format command");
		SET(t);
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
		if(dmasetup(DMAchan, buf, bp-buf, 0) < 0)
			error(Eio);

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

		/* Poll ready bits and transfer data */
		floppyexec((char *)buf, bp-buf, 0);

		/*
		 *  give bus to DMA, floppyintr() will read result
		 */
		floppywait(1);
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
			DPRINT("format: failed %ux %ux %ux\n",
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
floppyintr(Ureg *)
{
	switch(fl.cmd[0]&~Fmulti){
	case Fread:
	case Fwrite:
	case Fformat:
	case Fdumpreg: 
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

Dev floppydevtab = {
	'f',
	"floppy",

	floppyreset,
	devinit,
	devshutdown,
	floppyattach,
	floppywalk,
	floppystat,
	floppyopen,
	devcreate,
	floppyclose,
	floppyread,
	devbread,
	floppywrite,
	devbwrite,
	devremove,
	devwstat,
};
