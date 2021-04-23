/*
 * Asix USB ether adapters
 * I got no documentation for it, thus the bits
 * come from other systems; it's likely this is
 * doing more than needed in some places and
 * less than required in others.
 */
#include <u.h>
#include <libc.h>
#include <thread.h>

#include "usb.h"
#include "dat.h"

enum
{

	/* Asix commands */
	Cswmii		= 0x06,		/* set sw mii */
	Crmii		= 0x07,		/* read mii reg */
	Cwmii		= 0x08,		/* write mii reg */
	Chwmii		= 0x0a,		/* set hw mii */
	Creeprom	= 0x0b,		/* read eeprom */
	Cwdis		= 0x0e,		/* write disable */
	Cwena		= 0x0d,		/* write enable */
	Crrxctl		= 0x0f,		/* read rx ctl */
	Cwrxctl		= 0x10,		/* write rx ctl */
	Cwipg		= 0x12,		/* write ipg */
	Crmac		= 0x13,		/* read mac addr */
	Crphy		= 0x19,		/* read phy id */
	Cwmedium		= 0x1b,		/* write medium mode */
	Crgpio		= 0x1e,		/* read gpio */
	Cwgpio		= 0x1f,		/* write gpios */
	Creset		= 0x20,		/* reset */
	Cwphy		= 0x22,		/* select phy */

	/* reset codes */
	Rclear		= 0x00,
	Rprte		= 0x04,
	Rprl		= 0x08,
	Riprl		= 0x20,
	Rippd		= 0x40,

	Gpiogpo1en	= 0x04,	/* gpio1 enable */,
	Gpiogpo1		= 0x08,	/* gpio1 value */
	Gpiogpo2en	= 0x10,	/* gpio2 enable */
	Gpiogpo2		= 0x20,	/* gpio2 value */
	Gpiorse		= 0x80,	/* gpio reload serial eeprom */

	Pmask		= 0x1F,
	Pembed		= 0x10,			/* embedded phy */

	Mfd		= 0x002,		/* media */
	Mac		= 0x004,
	Mrfc		= 0x010,
	Mtfc		= 0x020,
	Mjfe		= 0x040,
	Mre		= 0x100,
	Mps		= 0x200,
	Mall772		= Mfd|Mrfc|Mtfc|Mps|Mac|Mre,
	Mall178		= Mps|Mfd|Mac|Mrfc|Mtfc|Mjfe|Mre,

	Ipgdflt		= 0x15|0x0c|0x12,	/* default ipg0, 1, 2 */
	Rxctlso		= 0x80,
	Rxctlab		= 0x08,
	Rxctlsep	= 0x04,
	Rxctlamall	= 0x02,			/* all multicast */
	Rxctlprom	= 0x01,			/* promiscuous */

	/* MII */
	Miibmcr			= 0x00,		/* basic mode ctrl reg. */
		Bmcrreset	= 0x8000,	/* reset */
		Bmcranena	= 0x1000,	/* auto neg. enable */
		Bmcrar		= 0x0200,	/* announce restart */

	Miiad			= 0x04,		/* advertise reg. */
		Adcsma		= 0x0001,
		Ad1000f		= 0x0200,
		Ad1000h		= 0x0100,
		Ad10h		= 0x0020,
		Ad10f		= 0x0040,
		Ad100h		= 0x0080,
		Ad100f		= 0x0100,
		Adpause		= 0x0400,
		Adall		= Ad10h|Ad10f|Ad100h|Ad100f,

	Miimctl			= 0x14,		/* marvell ctl */
		Mtxdly		= 0x02,
		Mrxdly		= 0x80,
		Mtxrxdly	= 0x82,

	Miic1000			= 0x09,

};

static uint asixphy;

static int
asixset(Dev *d, int c, int v)
{
	int r;
	int ec;

	r = Rh2d|Rvendor|Rdev;
	ec = usbcmd(d, r, c, v, 0, nil, 0);
	if(ec < 0)
		fprint(2, "%s: asixset %x %x: %r\n", argv0, c, v);
	return ec;
}

static int
asixget(Dev *d, int c, uchar *buf, int l)
{
	int r;
	int ec;

	r = Rd2h|Rvendor|Rdev;
	ec = usbcmd(d, r, c, 0, 0, buf, l);
	if(ec < 0)
		fprint(2, "%s: asixget %x: %r\n", argv0, c);
	return ec;
}

static int
getgpio(Dev *d)
{
	uchar c;

	if(asixget(d, Crgpio, &c, 1) < 0)
		return -1;
	return c;
}

static int
getphy(Dev *d)
{
	uchar buf[2];

	if(asixget(d, Crphy, buf, sizeof(buf)) < 0)
		return -1;
	return buf[1];
}

static int
getrxctl(Dev *d)
{
	uchar buf[2];

	memset(buf, 0, sizeof(buf));
	if(asixget(d, Crrxctl, buf, sizeof(buf)) < 0)
		return -1;
	return GET2(buf);
}

static int
miiread(Dev *d, int phy, int reg)
{
	int r;
	uchar v[2];

	r = Rd2h|Rvendor|Rdev;
	if(usbcmd(d, r, Crmii, phy, reg, v, 2) < 0){
		fprint(2, "%s: miiwrite: %r\n", argv0);
		return -1;
	}
	r = GET2(v);
	if(r == 0xFFFF)
		return -1;
	return r;
}


static int
miiwrite(Dev *d, int phy, int reg, int val)
{
	int r;
	uchar v[2];

	if(asixset(d, Cswmii, 0) < 0)
		return -1;
	r = Rh2d|Rvendor|Rdev;
	PUT2(v, val);
	if(usbcmd(d, r, Cwmii, phy, reg, v, 2) < 0){
		fprint(2, "%s: miiwrite: %#x %#x %r\n", argv0, reg, val);
		return -1;
	}
	if(asixset(d, Chwmii, 0) < 0)
		return -1;
	return 0;
}

static int
eepromread(Dev *d, int i)
{
	int r;
	int ec;
	uchar buf[2];

	r = Rd2h|Rvendor|Rdev;
	ec = usbcmd(d, r, Creeprom, i, 0, buf, sizeof(buf));
	if(ec < 0)
		fprint(2, "%s: eepromread %d: %r\n", argv0, i);
	ec = GET2(buf);
	if(ec == 0xFFFF)
		ec = -1;
	return ec;
}

static int
asixreceive(Dev *ep)
{
	Block *b;
	uint hd;
	int n;

	b = allocb(Maxpkt+4);
	if((n = read(ep->dfd, b->wp, b->lim - b->base)) < 0){
		freeb(b);
		return -1;
	}
	b->wp += n;
	while(BLEN(b) >= 4){
		hd = GET4(b->rp);
		b->rp += 4;
		n = hd & 0xFFFF;
		hd = (hd>>16) ^ 0xFFFF;
		if((n != hd) || (n > BLEN(b)))
			break;
		if(n == BLEN(b)){
			etheriq(b);
			return 0;
		}
		etheriq(copyblock(b, n));
		b->rp += n;
	}
	freeb(b);
	return 0;
}

static void
asixtransmit(Dev *ep, Block *b)
{
	uint hd;
	int n;

	n = BLEN(b);
	hd = n | (n<<16)^0xFFFF0000;
	b->rp -= 4;
	PUT4(b->rp, hd);
	n += 4;
	if((n % ep->maxpkt) == 0){
		PUT4(b->wp, 0xFFFF0000);
		b->wp += 4;
	}
	write(ep->dfd, b->rp, BLEN(b));
	freeb(b);
}

static int
asixpromiscuous(Dev *d, int on)
{
	int rxctl;

	rxctl = getrxctl(d);
	if(on)
		rxctl |= Rxctlprom;
	else
		rxctl &= ~Rxctlprom;
	return asixset(d, Cwrxctl, rxctl);
}

static int
asixmulticast(Dev *d, uchar*, int)
{
	int rxctl;

	rxctl = getrxctl(d);
	if(nmulti != 0)
		rxctl |= Rxctlamall;
	else
		rxctl &= ~Rxctlamall;
	return asixset(d, Cwrxctl, rxctl);
}

int
a88178init(Dev *d)
{
	int bmcr;
	int gpio;
	int ee17;

	gpio = getgpio(d);
	if(gpio < 0)
		return -1;
	asixset(d, Cwena, 0);
	ee17 = eepromread(d, 0x0017);
	asixset(d, Cwdis, 0);
	asixset(d, Cwgpio, Gpiorse|Gpiogpo1|Gpiogpo1en);
	if((ee17 >> 8) != 1){
		asixset(d, Cwgpio, 0x003c);
		asixset(d, Cwgpio, 0x001c);
		asixset(d, Cwgpio, 0x003c);
	}else{
		asixset(d, Cwgpio, Gpiogpo1en);
		asixset(d, Cwgpio, Gpiogpo1|Gpiogpo1en);
	}
	asixset(d, Creset, Rclear);
	sleep(150);
	asixset(d, Creset, Rippd|Rprl);
	sleep(150);
	asixset(d, Cwrxctl, 0);
	if(asixget(d, Crmac, macaddr, Eaddrlen) < 0)
		return -1;
	asixphy = getphy(d);
	if(ee17 < 0 || (ee17 & 0x7) == 0){
		miiwrite(d, asixphy, Miimctl, Mtxrxdly);
		sleep(60);
	}
	miiwrite(d, asixphy, Miibmcr, Bmcrreset|Bmcranena);
	miiwrite(d, asixphy, Miiad, Adall|Adcsma|Adpause);
	miiwrite(d, asixphy, Miic1000, Ad1000f);
	bmcr = miiread(d, asixphy, Miibmcr);
	if((bmcr & Bmcranena) != 0){
		bmcr |= Bmcrar;
		miiwrite(d, asixphy, Miibmcr, bmcr);
	}
	asixset(d, Cwmedium, Mall178);
	asixset(d, Cwrxctl, Rxctlso|Rxctlab);

	epreceive = asixreceive;
	eptransmit = asixtransmit;
	eppromiscuous = asixpromiscuous;
	epmulticast = asixmulticast;

	return 0;
}

int
a88772init(Dev *d)
{
	int bmcr;
	int rc;

	if(asixset(d, Cwgpio, Gpiorse|Gpiogpo2|Gpiogpo2en) < 0)
		return -1;
	asixphy = getphy(d);
	if((asixphy & Pmask) == Pembed){
		/* embedded 10/100 ethernet */
		rc = asixset(d, Cwphy, 1);
	}else
		rc = asixset(d, Cwphy, 0);
	if(rc < 0)
		return -1;
	if(asixset(d, Creset, Rippd|Rprl) < 0)
		return -1;
	sleep(150);
	if((asixphy & Pmask) == Pembed)
		rc = asixset(d, Creset, Riprl);
	else
		rc = asixset(d, Creset, Rprte);
	if(rc < 0)
		return -1;
	sleep(150);
	getrxctl(d);
	if(asixset(d, Cwrxctl, 0) < 0)
		return -1;
	if(asixget(d, Crmac, macaddr, Eaddrlen) < 0)
		return -1;
	if(asixset(d, Creset, Rprl) < 0)
		return -1;
	sleep(150);
	if(asixset(d, Creset, Riprl|Rprl) < 0)
		return -1;
	sleep(150);

	miiwrite(d, asixphy, Miibmcr, Bmcrreset);
	miiwrite(d, asixphy, Miiad, Adall|Adcsma);
	bmcr = miiread(d, asixphy, Miibmcr);
	if((bmcr & Bmcranena) != 0){
		bmcr |= Bmcrar;
		miiwrite(d, asixphy, Miibmcr, bmcr);
	}
	if(asixset(d, Cwmedium, Mall772) < 0)
		return -1;
	if(asixset(d, Cwipg, Ipgdflt) < 0)
		return -1;
	if(asixset(d, Cwrxctl, Rxctlso|Rxctlab) < 0)
		return -1;

	epreceive = asixreceive;
	eptransmit = asixtransmit;
	eppromiscuous = asixpromiscuous;
	epmulticast = asixmulticast;

	return 0;
}
