#include <u.h>

#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

#include	"fs.h"
#include	"devfloppy.h"


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

#define DPRINT if(0)print

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
static void	floppyformat(FDrive*, char*);
static void	floppykproc(void*);
static void	floppypos(FDrive*,long);
static int	floppyrecal(FDrive*);
static int	floppyresult(void);
static void	floppyrevive(void);
static vlong	pcfloppyseek(FDrive*, vlong);
static int	floppysense(void);
static void	floppywait(int);
static long	floppyxfer(FDrive*, int, void*, long, long);

static void
fldump(void)
{
	DPRINT("sra %ux srb %ux dor %ux msr %ux dir %ux\n", inb(Psra), inb(Psrb),
		inb(Pdor), inb(Pmsr), inb(Pdir));
}

static void
floppyalarm(Alarm* a)
{
	FDrive *dp;

	for(dp = fl.d; dp < &fl.d[fl.ndrive]; dp++){
		if((fl.motor&MOTORBIT(dp->dev)) && TK2SEC(m->ticks - dp->lasttouched) > 5)
			floppyoff(dp);
	}

	alarm(5*1000, floppyalarm, 0);
	cancel(a);
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
			break;
		}
}

static void
_floppydetach(void)
{
	/*
	 *  stop the motors
	 */
	fl.motor = 0;
	delay(10);
	outb(Pdor, fl.motor | Fintena | Fena);
	delay(10);
}

int
floppyinit(void)
{
	FDrive *dp;
	FType *t;
	ulong maxtsize;
	int mask;

	dmainit(DMAchan);

	floppysetup0(&fl);

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

	fl.selected = fl.d;

	floppydetach = _floppydetach;
	floppydetach();

	/*
	 *  init drives
	 */
	mask = 0;
	for(dp = fl.d; dp < &fl.d[fl.ndrive]; dp++){
		dp->dev = dp - fl.d;
		if(dp->dt == Tnone)
			continue;
		mask |= 1<<dp->dev;
		floppysetdef(dp);
		dp->cyl = -1;			/* because we don't know */
		dp->cache = (uchar*)xspanalloc(maxtsize, BY2PG, 64*1024);
		dp->ccyl = -1;
		dp->vers = 0;
		dp->maxtries = 5;
	}

	/*
	 *  first operation will recalibrate
	 */
	fl.confused = 1;

	floppysetup1(&fl);

	/* to turn the motor off when inactive */
	alarm(5*1000, floppyalarm, 0);

	return mask;
}

void
floppyinitdev(int i, char *name)
{
	if(i >= fl.ndrive)
		panic("floppyinitdev");
	sprint(name, "fd%d", i);
}

void
floppyprintdevs(int i)
{
	if(i >= fl.ndrive)
		panic("floppyprintdevs");
	print(" fd%d", i);
}

int
floppyboot(int dev, char *file, Boot *b)
{
	Fs *fs;

	if(strncmp(file, "dos!", 4) == 0)
		file += 4;
	else if(strchr(file, '!') || strcmp(file, "")==0) {
		print("syntax is fd0!file\n");
		return -1;
	}

	fs = floppygetfspart(dev, "dos", 1);
	if(fs == nil)
		return -1;

	return fsboot(fs, file, b);
}

void
floppyprintbootdevs(int dev)
{
	print(" fd%d", dev);
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
static int
changed(FDrive *dp)
{
	FType *start;

	/*
	 *  if floppy has changed or first time through
	 */
	if((inb(Pdir)&Fchange) || dp->vers == 0){
		DPRINT("changed\n");
		fldump();
		dp->vers++;
		floppysetdef(dp);
		dp->maxtries = 3;
		start = dp->t;

		/* flopppyon fails if there's no drive */
		dp->confused = 1;	/* make floppyon recal */
		if(floppyon(dp) < 0)
			return -1;

		pcfloppyseek(dp, dp->t->heads*dp->t->tsize);

		while(floppyxfer(dp, Fread, dp->cache, 0, dp->t->tsize) <= 0){

			/*
			 *  if the xfer attempt doesn't clear the changed bit,
			 *  there's no floppy in the drive
			 */
			if(inb(Pdir)&Fchange)
				return -1;

			while(++dp->t){
				if(dp->t == &floppytype[nelem(floppytype)])
					dp->t = floppytype;
				if(dp->dt == dp->t->dt)
					break;
			}

			/* flopppyon fails if there's no drive */
			if(floppyon(dp) < 0)
				return -1;

			DPRINT("changed: trying %s\n", dp->t->name);
			fldump();
			if(dp->t == start)
				return -1;
		}
	}

	return 0;
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

long
floppyread(Fs *fs, void *a, long n)
{
	FDrive *dp;
	long rv, offset;
	int sec, head, cyl;
	long len;
	uchar *aa;

	aa = a;
	dp = &fl.d[fs->dev];

	offset = dp->offset;
	floppyon(dp);
	if(changed(dp))
		return -1;

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
	dp->offset = offset+rv;

	return rv;
}

void*
floppygetfspart(int i, char *name, int chatty)
{
	static Fs fs;

	if(strcmp(name, "dos") != 0){
		if(chatty)
			print("unknown partition fd%d!%s (use fd%d!dos)\n", i, name, i);
		return nil;
	}

	fs.dev = i;
	fs.diskread = floppyread;
	fs.diskseek = floppyseek;

	/* sometimes we get spurious errors and doing it again works */
	if(dosinit(&fs) < 0 && dosinit(&fs) < 0){
		if(chatty)
			print("fd%d!%s does not contain a FAT file system\n", i, name);
		return nil;
	}
	return &fs;
}

static int
return0(void*)
{
	return 0;
}

static void
timedsleep(int (*f)(void*), void* arg, int ms)
{
	int s;
	ulong end;

	end = m->ticks + 1 + MS2TK(ms);
	while(m->ticks < end && !(*f)(arg)){
		s = spllo();
		delay(10);
		splx(s);
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
	dp->lasttouched = m->ticks;
	alreadyon = fl.motor & MOTORBIT(dp->dev);
	if(!alreadyon){
		fl.motor |= MOTORBIT(dp->dev);
		outb(Pdor, fl.motor | Fintena | Fena | dp->dev);
		/* wait for drive to spin up */
		timedsleep(return0, 0, 750);

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
			microdelay(1);
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
			microdelay(1);
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
floppywait(int slow)
{
	timedsleep(cmddone, 0, slow ? 5000 : 1000);
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
 *  reset it and get back to a kown state
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
static vlong
pcfloppyseek(FDrive *dp, vlong off)
{
	floppypos(dp, off);
	if(dp->cyl == dp->tcyl){
		dp->offset = off;
		return off;
	}
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
	dp->offset = off;
	DPRINT("seek to %d succeeded\n", dp->offset);

	return dp->offset;
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
	for(tries = 0; tries < dp->maxtries; tries++){

		dp->len = n;
		if(pcfloppyseek(dp, off) < 0){
			DPRINT("xfer: seek failed\n");
			dp->confused = 1;
			continue;
		}

		/*
		 *  set up the dma (dp->len may be trimmed)
		 */
		dp->len = dmasetup(DMAchan, a, dp->len, cmd==Fread);
		if(dp->len < 0){
	buggery:
			dmaend(DMAchan);
			continue;
		}
	
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
			goto buggery;

		/*
		 *  give bus to DMA, floppyintr() will read result
		 */
		floppywait(0);
		dmaend(DMAchan);

		/*
		 *  check for errors
		 */
		if(fl.nstat < 7){
			DPRINT("xfer: confused\n");
			fl.confused = 1;
			continue;
		}
		if((fl.stat[0] & Codemask)!=0 || fl.stat[1] || fl.stat[2]){
			DPRINT("xfer: failed %ux %ux %ux\n", fl.stat[0],
				fl.stat[1], fl.stat[2]);
			DPRINT("offset %lud len %ld\n", off, dp->len);
			if((fl.stat[0]&Codemask)==Cmdexec && fl.stat[1]==Overrun){
				DPRINT("DMA overrun: retry\n");
			} else
				dp->confused = 1;
			continue;
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
			continue;
		}
	
		dp->lasttouched = m->ticks;
		dp->maxtries = 20;
		return dp->len;
	}

	return -1;
}

/*
void
floppymemwrite(void)
{
	int i;
	int n;
	uchar *a;
	FDrive *dp;

	dp = &fl.d[0];
	a = (uchar*)0x80000000;
	n = 0;
	while(n < 1440*1024){
		i = floppyxfer(dp, Fwrite, a+n, n, 1440*1024-n);
		if(i <= 0)
			break;
		n += i;
	}
	print("floppymemwrite wrote %d bytes\n", n);
splhi(); for(;;);
}
*/

static void
floppyintr(Ureg *ur)
{
	USED(ur);
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
}
