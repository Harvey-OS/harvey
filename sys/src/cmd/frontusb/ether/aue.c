/*
* admtek pegasus driver copy-pasted from bill paul's
* openbsd/freebsd aue(4) driver, with help
* from the datasheet and petko manolov's linux
* drivers/net/usb/pegasus.c driver
*/
#include <u.h>
#include <libc.h>
#include <thread.h>

#include "usb.h"
#include "dat.h"

enum {
	Readreg = 0xf0,
	Writereg = 0xf1,

	Ctl0 = 0x00,
	C0incrxcrc = 1 << 0,
	C0allmulti = 1 << 1,
	C0stopbackoff = 1 << 2,
	C0rxstatappend = 1 << 3,
	C0wakeonen = 1 << 4,
	C0rxpauseen = 1 << 5,
	C0rxen = 1 << 6,
	C0txen = 1 << 7,

	Ctl1 = 0x01,
	C1homelan = 1 << 2,
	C1resetmac = 1 << 3,
	C1speedsel = 1 << 4,
	C1duplex = 1 << 5,
	C1delayhome = 1 << 6,

	Ctl2 = 0x02,
	C2ep3clr = 1 << 0,
	C2rxbadpkt = 1 << 1,
	C2prom = 1 << 2,
	C2loopback = 1 << 3,
	C2eepromwren = 1 << 4,
	C2eepromload = 1 << 5,

	Par = 0x10,

	Eereg = 0x20,
	Eedata = 0x21,

	Eectl = 0x23,
	Eectlwr = 1 << 0,
	Eectlrd = 1 << 1,
	Eectldn = 1 << 2,

	Phyaddr = 0x25,
	Phydata = 0x26,

	Phyctl = 0x28,
	Phyctlphyreg = 0x1f,
	Phyctlwr = 1 << 5,
	Phyctlrd = 1 << 6,
	Phyctldn = 1 << 7,

	Gpio0 = 0x7e,
	Gpio1 = 0x7f,
	Gpioin0 = 1 << 0,
	Gpioout0 = 1 << 1,
	Gpiosel0 = 1 << 2,
	Gpioin1 = 1 << 3,
	Gpioout1 = 1 << 4,
	Gpiosel1 = 1 << 5,

	Rxerror = 0x1e << 16,

	Timeout = 1000
};

static int csr8r(Dev *, int);
static int csr16r(Dev *, int);
static int csr8w(Dev *, int, int);
static int eeprom16r(Dev *, int);
static void reset(Dev *);

static int
csr8r(Dev *d, int reg)
{
	int rc;
	uchar v;

	rc = usbcmd(d, Rd2h|Rvendor|Rdev, Readreg,
		0, reg, &v, sizeof v);
	if(rc < 0) {
		fprint(2, "%s: csr8r(%#x): %r\n",
			argv0, reg);
		return 0;
	}
	return v;
}

static int
csr16r(Dev *d, int reg)
{
	int rc;
	uchar v[2];

	rc = usbcmd(d, Rd2h|Rvendor|Rdev, Readreg,
		0, reg, v, sizeof v);
	if(rc < 0) {
		fprint(2, "%s: csr16r(%#x): %r\n",
			argv0, reg);
		return 0;
	}
	return GET2(v);
}

static int
csr8w(Dev *d, int reg, int val)
{
	int rc;
	uchar v;
	
	v = val;
	rc = usbcmd(d, Rh2d|Rvendor|Rdev, Writereg,
		val&0xff, reg, &v, sizeof v);
	if(rc < 0) {
		fprint(2, "%s: csr8w(%#x, %#x): %r\n",
			argv0, reg, val);
	}
	return rc;
}

static int
eeprom16r(Dev *d, int off)
{
	int i;

	csr8w(d, Eereg, off);
	csr8w(d, Eectl, Eectlrd);
	for(i = 0; i < Timeout; i++) {
		if(csr8r(d, Eectl) & Eectldn)
			break;
	}
	if(i >= Timeout) {
		fprint(2, "%s: EEPROM read timed out\n",
			argv0);
	}
	return csr16r(d, Eedata);
}

static void
reset(Dev *d)
{
	int i;

	csr8w(d, Ctl1, csr8r(d, Ctl1)|C1resetmac);
	for(i = 0; i < Timeout; i++) {
		if(!(csr8r(d, Ctl1) & C1resetmac))
			break;
	}
	if(i >= Timeout)
		fprint(2, "%s: reset failed\n", argv0);
	csr8w(d, Gpio0, Gpiosel0|Gpiosel1);
	csr8w(d, Gpio0, Gpiosel0|Gpiosel1|Gpioout0);
	sleep(10);
}

static int
auereceive(Dev *ep)
{
	Block *b;
	uint hd;
	int n;

	b = allocb(Maxpkt+4);
	if((n = read(ep->dfd, b->wp, b->lim - b->base)) < 0){
		freeb(b);
		return -1;
	}
	if(n < 4){
		freeb(b);
		return 0;
	}
	b->wp += n-4;
	hd = GET4(b->wp);
	n = hd & 0xfff;
	if((hd & Rxerror) != 0 || n > BLEN(b)){
		freeb(b);
		return 0;
	}
	b->wp = b->rp + n;
	etheriq(b);
	return 0;
}

static void
auetransmit(Dev *ep, Block *b)
{
	int n;

	n = BLEN(b);
	b->rp -= 2;
	PUT2(b->rp, n);
	write(ep->dfd, b->rp, BLEN(b));
	freeb(b);
}

static int
auepromiscuous(Dev *d, int on)
{
	int r;

	r = csr8r(d, Ctl2);
	if(on)
		r |= C2prom;
	else
		r &= ~C2prom;
	return csr8w(d, Ctl2, r);
}

static int
auemulticast(Dev *d, uchar*, int)
{
	int r;

	r = csr8r(d, Ctl0);
	if(nmulti)
		r |= C0allmulti;
	else
		r &= ~C0allmulti;
	return csr8w(d, Ctl0, r);
}

int
aueinit(Dev *d)
{
	int i, v;
	uchar *p;

	reset(d);
	for(i = 0, p = macaddr; i < 3; i++, p += 2) {
		v = eeprom16r(d, i);
		PUT2(p, v);
	}
	for(i = 0; i < sizeof macaddr; i++)
		csr8w(d, Par+i, macaddr[i]);
	csr8w(d, Ctl2, csr8r(d, Ctl2)&~C2prom);
	csr8w(d, Ctl0, C0rxstatappend|C0rxen);
	csr8w(d, Ctl0, csr8r(d, Ctl0)|C0txen);
	csr8w(d, Ctl2, csr8r(d, Ctl2)|C2ep3clr);

	epreceive = auereceive;
	eptransmit = auetransmit;
	eppromiscuous = auepromiscuous;
	epmulticast = auemulticast;

	return 0;
}
