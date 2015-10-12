/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum {
	Data=		0x60,		/* data port */

	Status=		0x64,		/* status port */
		Inready=	0x01,		/*  input character ready */
		Outbusy=	0x02,		/*  output busy */
		Sysflag=	0x04,		/*  system flag */
		Cmddata=	0x08,		/*  cmd==0, data==1 */
		Inhibit=	0x10,		/*  keyboard/mouse inhibited */
		Minready=	0x20,		/*  mouse character ready */
		Rtimeout=	0x40,		/*  general timeout */
		Parity=	0x80,

	Cmd=		0x64,		/* command port (write only) */

};


enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cauxdis=	(1<<5),		/* mouse disable */
	Ckeybdis=	(1<<4),		/* keyb disable */
	Csf=		(1<<2),		/* system flag */
	Cauxint=	(1<<1),		/* mouse interrupt enable */
	Ckeybint=	(1<<0),		/* keyb interrupt enable */
};

static Queue *keybq;
static Queue *mouseq;


static int nokeyb = 1;

static Lock i8042lock;
static uint8_t ccc;

/*
 *  wait for output no longer busy
 */
static int
outready(void)
{
	int tries;

	for(tries = 0; (inb(Status) & Outbusy); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

/*
 *  wait for input
 */
static int
inready(void)
{
	int tries;

	for(tries = 0; !(inb(Status) & Inready); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

void
i8042systemreset(void)
{
	uint16_t *s = KADDR(0x472);
	int i, x;

	if(nokeyb)
		return;

	*s = 0x1234;		/* BIOS warm-boot flag */

	/* newer reset the machine command */
	outready();
	outb(Cmd, 0xFE);
	outready();

	/* Pulse it by hand (old somewhat reliable) */
	x = 0xDF;
	for(i = 0; i < 5; i++){
		x ^= 1;
		outready();
		outb(Cmd, 0xD1);
		outready();
		outb(Data, x);	/* toggle reset */
		delay(100);
	}
}

int
mousecmd(int cmd)
{
	unsigned int c;
	int tries;
	static int badkbd;

	if(badkbd)
		return -1;
	c = 0;
	tries = 0;

	ilock(&i8042lock);
	do{
		if(tries++ > 2)
			break;
		if(outready() < 0)
			break;
		outb(Cmd, 0xD4);
		if(outready() < 0)
			break;
		outb(Data, cmd);
		if(outready() < 0)
			break;
		if(inready() < 0)
			break;
		c = inb(Data);
	} while(c == 0xFE || c == 0);
	iunlock(&i8042lock);

	if(c != 0xFA){
		print("mousecmd: %2.2ux returned to the %2.2ux command\n", c, cmd);
		badkbd = 1;	/* don't keep trying; there might not be one */
		return -1;
	}
	return 0;
}

static int
mousecmds(uint8_t *cmd, int ncmd)
{
	int i;

	ilock(&i8042lock);
	for(i=0; i<ncmd; i++){
		if(outready() == -1)
			break;
		outb(Cmd, 0xD4);
		if(outready() == -1)
			break;
		outb(Data, cmd[i]);
	}
	iunlock(&i8042lock);
	return i;
}

static void
i8042intr(Ureg* u, void* v)
{
	uint8_t stat, data;

	ilock(&i8042lock);
	stat = inb(Status);
	if((stat&Inready) == 0){
		iunlock(&i8042lock);
		return;
	}
	data = inb(Data);
	iunlock(&i8042lock);

	if(stat & Minready){
		if(mouseq != nil)
			qiwrite(mouseq, &data, 1);
	} else {
		if(keybq != nil)
			qiwrite(keybq, &data, 1);
	}
}

static int
outbyte(int port, int c)
{
	if(outready() == -1) {
		return -1;
	}
	outb(port, c);
	if(outready() == -1) {
		return -1;
	}
	return 0;
}

static int32_t
mouserwrite(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return mousecmds(vbuf, len);
}

static int32_t
mouseread(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return qread(mouseq, vbuf, len);
}

void
mouseenable(void)
{
	mouseq = qopen(32, 0, 0, 0);
	if(mouseq == nil)
		panic("mouseenable");
	qnoblock(mouseq, 1);

	ccc &= ~Cauxdis;
	ccc |= Cauxint;

	ilock(&i8042lock);
	if(outready() == -1)
		iprint("mouseenable: failed 0\n");
	outb(Cmd, 0x60);	/* write control register */
	if(outready() == -1)
		iprint("mouseenable: failed 1\n");
	outb(Data, ccc);
	if(outready() == -1)
		iprint("mouseenable: failed 2\n");
	outb(Cmd, 0xA8);	/* auxilliary device enable */
	if(outready() == -1){
		iprint("mouseenable: failed 3\n");
		iunlock(&i8042lock);
		return;
	}
	iunlock(&i8042lock);

	intrenable(IrqAUX, i8042intr, 0, BUSUNKNOWN, "mouse");
	addarchfile("ps2mouse", 0666, mouseread, mouserwrite);
}

void
keybinit(void)
{
	int c, try;

	/* wait for a quiescent controller */
	ilock(&i8042lock);

	try = 1000;
	while(try-- > 0 && (c = inb(Status)) & (Outbusy | Inready)) {
		if(c & Inready)
			inb(Data);
		delay(1);
	}
	if (try <= 0) {
		iunlock(&i8042lock);
		print("keybinit failed 0\n");
		return;
	}

	/* get current controller command byte */
	outb(Cmd, 0x20);
	if(inready() == -1){
		iunlock(&i8042lock);
		print("keybinit failed 1\n");
		ccc = 0;
	} else
		ccc = inb(Data);

	/* enable keyb xfers and interrupts */
	ccc &= ~(Ckeybdis);
	ccc |= Csf | Ckeybint | Cscs1;
	if(outready() == -1) {
		iunlock(&i8042lock);
		print("keybinit failed 2\n");
		return;
	}

	if (outbyte(Cmd, 0x60) == -1){
		iunlock(&i8042lock);
		print("keybinit failed 3\n");
		return;
	}
	if (outbyte(Data, ccc) == -1){
		iunlock(&i8042lock);
		print("keybinit failed 4\n");
		return;
	}

	nokeyb = 0;

	iunlock(&i8042lock);
}

static int32_t
keybread(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return qread(keybq, vbuf, len);
}

void
keybenable(void)
{
	keybq = qopen(32, 0, 0, 0);
	if(keybq == nil)
		panic("keybinit");
	qnoblock(keybq, 1);

	ioalloc(Data, 1, 0, "keyb");
	ioalloc(Cmd, 1, 0, "keyb");

	intrenable(IrqKBD, i8042intr, 0, BUSUNKNOWN, "keyb");

	addarchfile("ps2keyb", 0666, keybread, nil);

}
