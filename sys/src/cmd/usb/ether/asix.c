/*
 * Asix USB ether adapters
 * I got no documentation for it, thus the bits
 * come from other systems; it's likely this is
 * doing more than needed in some places and
 * less than required in others.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ether.h"

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

static int
asixset(Dev *d, int c, int v)
{
	int r;
	int ec;

	r = Rh2d|Rvendor|Rdev;
	ec = usbcmd(d, r, c, v, 0, nil, 0);
	if(ec < 0)
		deprint(2, "%s: asixset %x %x: %r\n", argv0, c, v);
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
		deprint(2, "%s: asixget %x: %r\n", argv0, c);
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
	deprint(2, "%s: phy addr %#ux\n", argv0, buf[1]);
	return buf[1];
}

static int
getrxctl(Dev *d)
{
	uchar buf[2];
	int r;

	memset(buf, 0, sizeof(buf));
	if(asixget(d, Crrxctl, buf, sizeof(buf)) < 0)
		return -1;
	r = GET2(buf);
	deprint(2, "%s: rxctl %#x\n", argv0, r);
	return r;
}

static int
getmac(Dev *d, uchar buf[])
{
	if(asixget(d, Crmac, buf, Eaddrlen) < 0)
		return -1;
	return Eaddrlen;
}

static int
miiread(Dev *d, int phy, int reg)
{
	int r;
	uchar v[2];

	r = Rd2h|Rvendor|Rdev;
	if(usbcmd(d, r, Crmii, phy, reg, v, 2) < 0){
		dprint(2, "%s: miiwrite: %r\n", argv0);
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
		deprint(2, "%s: miiwrite: %#x %#x %r\n", argv0, reg, val);
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
		deprint(2, "%s: eepromread %d: %r\n", argv0, i);
	ec = GET2(buf);
	deprint(2, "%s: eeprom %#x = %#x\n", argv0, i, ec);
	if(ec == 0xFFFF)
		ec = -1;
	return ec;
}

/*
 * No doc. we are doing what Linux does as closely
 * as we can.
 */
static int
ctlrinit(Ether *ether)
{
	Dev *d;
	int i;
	int bmcr;
	int gpio;
	int ee17;
	int rc;

	d = ether->dev;
	switch(ether->cid){
	default:
		fprint(2, "%s: card known but not implemented\n", argv0);
		return -1;

	case A88178:
		deprint(2, "%s: setting up A88178\n", argv0);
		gpio = getgpio(d);
		if(gpio < 0)
			return -1;
		deprint(2, "%s: gpio sts %#x\n", argv0, gpio);
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
		if(getmac(d, ether->addr) < 0)
			return -1;
		ether->phy = getphy(d);
		if(ee17 < 0 || (ee17 & 0x7) == 0){
			miiwrite(d, ether->phy, Miimctl, Mtxrxdly);
			sleep(60);
		}
		miiwrite(d, ether->phy, Miibmcr, Bmcrreset|Bmcranena);
		miiwrite(d, ether->phy, Miiad, Adall|Adcsma|Adpause);
		miiwrite(d, ether->phy, Miic1000, Ad1000f);
		bmcr = miiread(d, ether->phy, Miibmcr);
		if((bmcr & Bmcranena) != 0){
			bmcr |= Bmcrar;
			miiwrite(d, ether->phy, Miibmcr, bmcr);
		}
		asixset(d, Cwmedium, Mall178);
		asixset(d, Cwrxctl, Rxctlso|Rxctlab);
		break;

	case A88772:
		deprint(2, "%s: setting up A88772\n", argv0);
		if(asixset(d, Cwgpio, Gpiorse|Gpiogpo2|Gpiogpo2en) < 0)
			return -1;
		ether->phy = getphy(d);
		dprint(2, "%s: phy %#x\n", argv0, ether->phy);
		if((ether->phy & Pmask) == Pembed){
			/* embedded 10/100 ethernet */
			rc = asixset(d, Cwphy, 1);
		}else
			rc = asixset(d, Cwphy, 0);
		if(rc < 0)
			return -1;
		if(asixset(d, Creset, Rippd|Rprl) < 0)
			return -1;
		sleep(150);
		if((ether->phy & Pmask) == Pembed)
			rc = asixset(d, Creset, Riprl);
		else
			rc = asixset(d, Creset, Rprte);
		if(rc < 0)
			return -1;
		sleep(150);
		rc = getrxctl(d);
		deprint(2, "%s: rxctl is %#x\n", argv0, rc);
		if(asixset(d, Cwrxctl, 0) < 0)
			return -1;
		if(getmac(d, ether->addr) < 0)
			return -1;


		if(asixset(d, Creset, Rprl) < 0)
			return -1;
		sleep(150);
		if(asixset(d, Creset, Riprl|Rprl) < 0)
			return -1;
		sleep(150);

		miiwrite(d, ether->phy, Miibmcr, Bmcrreset);
		miiwrite(d, ether->phy, Miiad, Adall|Adcsma);
		bmcr = miiread(d, ether->phy, Miibmcr);
		if((bmcr & Bmcranena) != 0){
			bmcr |= Bmcrar;
			miiwrite(d, ether->phy, Miibmcr, bmcr);
		}
		if(asixset(d, Cwmedium, Mall772) < 0)
			return -1;
		if(asixset(d, Cwipg, Ipgdflt) < 0)
			return -1;
		if(asixset(d, Cwrxctl, Rxctlso|Rxctlab) < 0)
			return -1;
		deprint(2, "%s: final rxctl: %#x\n", argv0, getrxctl(d));
		break;
	}

	if(etherdebug){
		fprint(2, "%s: ether: phy %#x addr ", argv0, ether->phy);
		for(i = 0; i < sizeof(ether->addr); i++)
			fprint(2, "%02x", ether->addr[i]);
		fprint(2, "\n");
	}
	return 0;
}


static long
asixbread(Ether *e, Buf *bp)
{
	ulong nr;
	ulong hd;
	Buf *rbp;

	rbp = e->aux;
	if(rbp == nil || rbp->ndata < 4){
		rbp->rp = rbp->data;
		rbp->ndata = read(e->epin->dfd, rbp->rp, sizeof(bp->data));
		if(rbp->ndata < 0)
			return -1;
	}
	if(rbp->ndata < 4){
		werrstr("short frame");
		deprint(2, "%s: asixbread got %d bytes\n", argv0, rbp->ndata);
		rbp->ndata = 0;
		return 0;
	}
	hd = GET4(rbp->rp);
	nr = hd & 0xFFFF;
	hd = (hd>>16) & 0xFFFF;
	if(nr != (~hd & 0xFFFF)){
		if(0)deprint(2, "%s: asixread: bad header %#ulx %#ulx\n",
			argv0, nr, (~hd & 0xFFFF));
		werrstr("bad usb packet header");
		rbp->ndata = 0;
		return 0;
	}
	rbp->rp += 4;
	if(nr < 6 || nr > Epktlen){
		if(nr < 6)
			werrstr("short frame");
		else
			werrstr("long frame");
		deprint(2, "%s: asixbread %r (%ld)\n", argv0, nr);
		rbp->ndata = 0;
		return 0;
	}
	bp->rp = bp->data + Hdrsize;
	memmove(bp->rp, rbp->rp, nr);
	bp->ndata = nr;
	rbp->rp += 4 + nr;
	rbp->ndata -= (4 + nr);
	return bp->ndata;
}

static long
asixbwrite(Ether *e, Buf *bp)
{
	ulong len;
	long n;

	deprint(2, "%s: asixbwrite %d bytes\n", argv0, bp->ndata);
	assert(bp->rp - bp->data >= Hdrsize);
	bp->ndata &= 0xFFFF;
	len = (0xFFFF0000 & ~(bp->ndata<<16))  | bp->ndata;
	bp->rp -= 4;
	PUT4(bp->rp, len);
	bp->ndata += 4;
	if((bp->ndata % e->epout->maxpkt) == 0){
		PUT4(bp->rp+bp->ndata, 0xFFFF0000);
		bp->ndata += 4;
	}
	n = write(e->epout->dfd, bp->rp, bp->ndata);
	deprint(2, "%s: asixbwrite wrote %ld bytes\n", argv0, n);
	if(n <= 0)
		return n;
	return n;
}

static int
asixpromiscuous(Ether *e, int on)
{
	int rxctl;

	deprint(2, "%s: aixprompiscuous %d\n", argv0, on);
	rxctl = getrxctl(e->dev);
	if(on != 0)
		rxctl |= Rxctlprom;
	else
		rxctl &= ~Rxctlprom;
	return asixset(e->dev, Cwrxctl, rxctl);
}

static int
asixmulticast(Ether *e, uchar *addr, int on)
{
	int rxctl;

	USED(addr);
	USED(on);
	/* BUG: should write multicast filter */
	rxctl = getrxctl(e->dev);
	if(e->nmcasts != 0)
		rxctl |= Rxctlamall;
	else
		rxctl &= ~Rxctlamall;
	deprint(2, "%s: asixmulticast %d\n", argv0, e->nmcasts);
	return asixset(e->dev, Cwrxctl, rxctl);
}

static void
asixfree(Ether *ether)
{
	deprint(2, "%s: aixfree %#p\n", argv0, ether);
	free(ether->aux);
	ether->aux = nil;
}

int
asixreset(Ether *ether)
{
	Cinfo *ip;
	Dev *dev;

	dev = ether->dev;
	for(ip = cinfo; ip->vid != 0; ip++)
		if(ip->vid == dev->usb->vid && ip->did == dev->usb->did){
			ether->cid = ip->cid;
			if(ctlrinit(ether) < 0){
				deprint(2, "%s: init failed: %r\n", argv0);
				return -1;
			}
			deprint(2, "%s: asix reset done\n", argv0);
			ether->aux = emallocz(sizeof(Buf), 1);
			ether->bread = asixbread;
			ether->bwrite = asixbwrite;
			ether->free = asixfree;
			ether->promiscuous = asixpromiscuous;
			ether->multicast = asixmulticast;
			ether->mbps = 100;	/* BUG */
			return 0;
		}
	return -1;
}
