/*
 * Dummy driver for PCF8584 IIC-bus Controller.
 * Master-only mode and no interrupts as all we
 * really want is to get the ether address from the
 * EEPROM and set the LEDs.
 * It'll need a rewrite for the keyboard and finger-mouse.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

enum {
	Busfree		= 0x01,		/* bus free */
	Nak		= 0x08,		/* nak */
	Buserr		= 0x10,		/* bus error */
	Pin		= 0x80,		/*  */

	Ack		= 0x01,		/* send ack after each byte */
	Sto		= 0x02,		/* stop */
	Sta		= 0x04,		/* start */
	Eni		= 0x08,		/* enable external interrupt */
	OwnAddrReg	= 0x00,		/* own address register select */
	IntrVecReg	= 0x10,		/* interrupt vector register select */
	ClockReg	= 0x20,		/* clock register select */
	Eso		= 0x40,		/* enable serial output */

	Ecl8MHz		= 0x18,		/* 8MHz external clock */
	Scl100KHz	= 0x00,		/* 100KHz serial clock */

	Write		= 0x00,
	Read		= 0x01,

	PCF8584		= 0x10,		/* my address */
	PCF8574		= 0x40,		/* led address */
	PCF8582		= 0xA4,		/* eeprom address */
};

typedef struct {
	QLock;
	int	eeprom;
	Iic	*iic;
} Ctlr;
static Ctlr ctlr;

struct Eeprom eeprom;

/*
 * Microsecond delay.
 */
static void
mdelay(int count)
{
	if(count == 0)
		count = 2;
	count *= 25;
	while(count-- > 0)
		;
}

#define PUTREG(x)	((x), mdelay(0))

static void
release(void)
{
	Iic *iic = ctlr.iic;

	/*
	 * Stop the transfer and wait for the bus to
	 * be released.
	 */
	mdelay(0);
	PUTREG(iic->ctl = Pin|Eso|Sto|Ack);
	while((iic->ctl & Busfree) == 0)
		mdelay(0);
}

static int
iicack(void)
{
	Iic *iic = ctlr.iic;

	while(iic->ctl & Pin)
		mdelay(0);
	mdelay(0);
	if(iic->ctl & (Nak|Buserr)){
		release();
		return 1;
	}
	mdelay(0);
	return 0;
}

static int
master(uchar slave)
{
	Iic *iic = ctlr.iic;

	/*
	 * Load the slave-device address + write into the data register,
	 * then set the control register for a single transfer (Ack not
	 * set).
	 * Pin must be set (why?).
	 */
	PUTREG(iic->data = slave|Write);
	PUTREG(iic->ctl = Pin|Eso|Sta);
	return iicack();
}

enum {
	Qled		= PCF8574,
	Qeeprom		= PCF8582,
};

static Dirtab iicdir[] = {
	"led",		{ Qled, 0 },	0,	0666,
	"eeprom",	{ Qeeprom, 0 },	0,	0666,
};
#define	NIICDEV	(sizeof(iicdir)/sizeof(iicdir[0]))

static int
eepromread(void)
{
	Iic *iic = ctlr.iic;
	int i;
	uchar *p;

	if(master(Qeeprom))
		return 1;

	/*
	 * Write the starting offset into the EEPROM.
	 */
	PUTREG(iic->data = 0);
	if(iicack())
		return 1;

	/*
	 * Select the EEPROM for reading. For some reason we must
	 * read the ctl register then delay a while.
	 * The reason might be that we have to flush the data we
	 * wrote above from the data register, it doesn't go away
	 * automatically.
	 */
	PUTREG(iic->data = Qeeprom|Read);
	i = iic->ctl;
	mdelay(100);

	/*
	 * Set the control register for a multiple-byte transfer
	 * (Ack set). We get some junk back, then the data.
	 * Is the junk some start-of-data acknowledgement from
	 * the slave?
	 */
	PUTREG(iic->ctl = Pin|Eso|Sta|Ack);
	if(iicack())
		return 1;

	i = iic->data;
	mdelay(0);

	/*
	 * Read the EEPROM data. We read 1 byte less than we want
	 * because we have to tell the slave when to terminate.
	 */
	p = (uchar*)&eeprom;
	i = 0;
	do{
		if(iicack())
			return 1;
		*p++ = iic->data;
	}while(i++ < sizeof(Eeprom)-1);

	/*
	 * Take Ack away so the slave will stop sending, causing Nak
	 * on the bus. We get one more byte of data.
	 * We can't use iicack() here as it thinks Nak is an error.
	 */
	PUTREG(iic->ctl = 0xC0);
	while(iic->ctl & Pin)
		;

	mdelay(0);
	*p = iic->data;

	release();
	return 0;
}

void
iicreset(void)
{
	Iic *iic = IICADDR;

	/*
	 * The first access after a reset must be a write
	 * in order to set the appropriate interface mode.
	 * We write our own address and the access should
	 * put the chip in 68K mode.
	 */
	PUTREG(iic->data = PCF8584);

	/*
	 * Select the clock register and set it for
	 * 8MHz external clock and 100KHz serial clock.
	 */
	PUTREG(iic->ctl = ClockReg);
	PUTREG(iic->data = Ecl8MHz|Scl100KHz);

	/*
	 * Enable the serial bus and check for it to
	 * be free.
	 * If all is ok, read the eeprom now as the ethernet
	 * code will need it.
	 * Must ensure that this code runs before ether reset.
	 */
	PUTREG(iic->ctl = Eso);
	if(iic->ctl & Busfree){
		ctlr.iic = iic;
		if(eepromread()){
			print("iic: can't read eeprom\n");
			return;
		}
		ctlr.eeprom = 1;
	}
}

static int
ledputbyte(uchar c)
{
	Iic *iic = ctlr.iic;

	if(master(Qled))
		return 1;

	/*
	 * Write the data.
	 */
	PUTREG(iic->data = c);
	if(iicack())
		return 1;

	release();
	return 0;
}

void
iicinit(void)
{
}

Chan *
iicattach(char *spec)
{
	if(ctlr.iic == 0)
		error(Eio);
	return devattach('2', spec);
}

Chan *
iicclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
iicwalk(Chan *c, char *name)
{
	return devwalk(c, name, iicdir, NIICDEV, devgen);
}

void	 
iicstat(Chan *c, char *dp)
{
	devstat(c, dp, iicdir, NIICDEV, devgen);
}

Chan*
iicopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(c->qid.path){

	case Qled:
		if(omode != OWRITE)
			error(Eperm);
		break;

	case Qeeprom:
		if(omode != OREAD)
			error(Eperm);
		break;
	}
	return devopen(c, omode, iicdir, NIICDEV, devgen);
}

void	 
iiccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
iicclose(Chan *c)
{
	USED(c);
}

long	 
iicread(Chan *c, void *buf, long n, ulong offset)
{
	USED(offset);

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, iicdir, NIICDEV, devgen);

	switch(c->qid.path){

	case Qeeprom:
		if(ctlr.eeprom == 0)
			error(Eio);
		if(offset > sizeof(Eeprom))
			return 0;
		if(n > sizeof(Eeprom) - offset)
			n = sizeof(Eeprom) - offset;
		memmove(buf, ((uchar*)&eeprom)+offset, n);
		return n;
	}
	error(Eio);
	return -1;
}

long	 
iicwrite(Chan *c, void *buf, long n, ulong offset)
{
	long i;
	uchar *p;

	USED(offset);
	switch(c->qid.path){

	case Qled:
		qlock(&ctlr);
		for(p = buf, i = n; i; i--, p++){
			if(ledputbyte(*p)){
				qunlock(&ctlr);
				error(Eio);
			}
		}
		qunlock(&ctlr);
		return n;
	}
	error(Eio);
	return -1;
}

void	 
iicremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
iicwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
