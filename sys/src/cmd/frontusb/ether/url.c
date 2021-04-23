/*
* realtek rtl8150 10/100 usb ethernet device driver
*
* copy-pasted from shingo watanabe's openbsd url(4) driver
* and bill paul's and shunsuke akiyama's freebsd rue(4) driver
*/
#include <u.h>
#include <libc.h>
#include <thread.h>

#include "usb.h"
#include "dat.h"

enum {
	Timeout = 50,
	Mfl = 60, /* min frame len */
};

enum { /* requests */
	Rqm = 0x05, /* request mem */
	Crm = 1, /* command read mem */
	Cwm = 2, /* command write mem */
};

enum { /* registers */
	Idr0 = 0x120, /* ether addr, load from 93c46 */
	Idr1 = 0x121,
	Idr2 = 0x122,
	Idr3 = 0x123,
	Idr4 = 0x124,
	Idr5 = 0x125,

	Mar0 = 0x126, /* multicast addr */
	Mar1 = 0x127,
	Mar2 = 0x128,
	Mar3 = 0x129,
	Mar4 = 0x12a,
	Mar5 = 0x12b,
	Mar6 = 0x12c,
	Mar7 = 0x12d,

	Cr = 0x12e, /* command */
	Tcr = 0x12f, /* transmit control */
	Rcr = 0x130, /* receive configuration */
	Tsr = 0x132,
	Rsr = 0x133,
	Con0 = 0x135,
	Con1 = 0x136,
	Msr = 0x137, /* media status */
	Phyar = 0x138, /* mii phy addr select */
	Phydr = 0x139, /* mii phy data */
	Phycr = 0x13b, /* mii phy control */
	Gppc = 0x13d,
	Wcr = 0x13e, /* wake count */
	Bmcr = 0x140, /* basic mode control */
	Bmsr = 0x142, /* basic mode status */
	Anar = 0x144, /* an advertisement */
	Anlp = 0x146, /* an link partner ability */
	Aner = 0x148,
	Nwtr = 0x14a, /* nway test */
	Cscr = 0x14c,

	Crc0 = 0x14e,
	Crc1 = 0x150,
	Crc2 = 0x152,
	Crc3 = 0x154,
	Crc4 = 0x156,

	Bm0 = 0x158, /* byte mask */
	Bm1 = 0x160,
	Bm2 = 0x168,
	Bm3 = 0x170,
	Bm4 = 0x178,
	
	Phy1 = 0x180,
	Phy2 = 0x184,
	Tw1 = 0x186
};

enum { /* Cr */
	We = 1 << 5, /* eeprom write enable */
	Sr = 1 << 4, /* software reset */
	Re = 1 << 3, /* ethernet receive enable */
	Te = 1 << 2, /* ethernet transmit enable */
	Ep3ce = 1 << 1, /* enable clr of perf counter */
	Al = 1 << 0 /* auto-load contents of 93c46 */
};

enum { /* Tcr */
	Tr1 = 1 << 7, /* tx retry count */
	Tr0 = 1 << 6,
	Ifg1 = 1 << 4, /* interframe gap time */
	Ifg0 = 1 << 3,
	Nocrc = 1 << 0 /* no crc appended */
};

enum { /* Rcr */
	Tail = 1 << 7,
	Aer = 1 << 6,
	Ar = 1 << 5,
	Am = 1 << 4,
	Ab = 1 << 3,
	Ad = 1 << 2,
	Aam = 1 << 1,
	Aap = 1 << 0
};

enum { /* Msr */
	Tfce = 1 << 7,
	Rfce = 1 << 6,
	Mdx = 1 << 4, /* duplex */
	S100 = 1 << 3, /* speed 100 */
	Lnk = 1 << 2,
	Tpf = 1 << 1,
	Rpf = 1 << 0
};

enum { /* Phyar */
	Phyamsk = 0x1f
};

enum { /* Phycr */
	Phyown = 1 << 6, /* own bit */
	Rwcr = 1 << 5, /* mii mgmt data r/w control */
	Phyoffmsk = 0x1f /* phy register offset */
};

enum { /* Bmcr */
	Spd = 0x2000, /* speed set */
	Bdx = 0x0100 /* duplex */
};

enum { /* Anar */
	Ap = 0x0400 /* pause */
};

enum { /* Anlp */
	Lpp = 0x0400 /* pause */
};

enum { /* eeprom address declarations */
	Ebase = 0x1200,
	Eidr0 = Ebase + 0x02,
	Eidr1 = Ebase + 0x03,
	Eidr2 = Ebase + 0x03,
	Eidr3 = Ebase + 0x03,
	Eidr4 = Ebase + 0x03,
	Eidr5 = Ebase + 0x03,
	Eint = Ebase + 0x17 /* interval */
};

enum { /* receive header */
	Bcm = 0x0fff, /* rx bytes count mask */
	Vpm = 0x1000, /* valid packet mask */
	Rpm = 0x2000, /* runt packet mask */
	Ppm = 0x4000, /* physical match packet mask */
	Mpm = 0x8000 /* multicast packet mask */
};

static int mem(Dev *, int, int, uchar *, int);
static int csr8r(Dev *, int);
static int csr16r(Dev *, int);
static int csr8w(Dev *, int, int);
static int csr16w(Dev *, int, int);
static int csr32w(Dev *, int, int);
static void reset(Dev *);
int urlinit(Dev *);

static int
mem(Dev *d, int cmd, int off, uchar *buf, int len)
{
	int r, rc;

	if(d == nil)
		return 0;
	r = Rvendor | Rdev;
	if(cmd == Crm)
		r |= Rd2h;
	else
		r |= Rh2d;
	rc = usbcmd(d, r, Rqm, off, 0, buf, len);
	if(rc < 0) {
		fprint(2, "%s: mem(%d, %#.4x) failed\n",
			argv0, cmd, off);
	}
	return rc;
}

static int
csr8r(Dev *d, int reg)
{
	uchar v;

	v = 0;
	if(mem(d, Crm, reg, &v, sizeof v) < 0)
		return 0;
	return v;
}

static int
csr16r(Dev *d, int reg)
{
	uchar v[2];

	PUT2(v, 0);
	if(mem(d, Crm, reg, v, sizeof v) < 0)
		return 0;
	return GET2(v);
}

static int
csr8w(Dev *d, int reg, int val)
{
	uchar v;

	v = val;
	if(mem(d, Cwm, reg, &v, sizeof v) < 0)
		return -1;
	return 0;
}

static int
csr16w(Dev *d, int reg, int val)
{
	uchar v[2];

	PUT2(v, val);
	if(mem(d, Cwm, reg, v, sizeof v) < 0)
		return -1;
	return 0;
}

static int
csr32w(Dev *d, int reg, int val)
{
	uchar v[4];

	PUT4(v, val);
	if(mem(d, Cwm, reg, v, sizeof v) < 0)
		return -1;
	return 0;
}

static void
reset(Dev *d)
{
	int i, r;

	r = csr8r(d, Cr) | Sr;
	csr8w(d, Cr, r);

	for(i = 0; i < Timeout; i++) {
		if((csr8r(d, Cr) & Sr) == 0)
			break;
		sleep(10);
	}
	if(i >= Timeout)
		fprint(2, "%s: reset failed\n", argv0);

	sleep(100);
}

static int
urlreceive(Dev *ep)
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
	n -= 4;
	b->wp += n;
	hd = GET2(b->wp);
	if((hd & Vpm) == 0)
		freeb(b);
	else
		etheriq(b);
	return 0;
}

static void
urltransmit(Dev *ep, Block *b)
{
	int n;

	n = BLEN(b);
	if(n < Mfl){
		memset(b->wp, 0, Mfl-n);
		b->wp += (Mfl-n);
	}
	write(ep->dfd, b->rp, BLEN(b));
	freeb(b);
}

static int
urlpromiscuous(Dev *d, int on)
{
	int r;

	r = csr16r(d, Rcr);
	if(on)
		r |= Aam|Aap;
	else {
		r &= ~Aap;
		if(nmulti == 0)
			r &= ~Aam;
	}
	return csr16w(d, Rcr, r);
}

static int
urlmulticast(Dev *d, uchar*, int)
{
	int r;

	r = csr16r(d, Rcr);
	if(nmulti)
		r |= Aam;
	else {
		if(nprom == 0)
			r &= ~Aam;
	}
	return csr16w(d, Rcr, r);
}

int
urlinit(Dev *d)
{
	int i, r;

	reset(d);
	if(mem(d, Crm, Idr0, macaddr, sizeof macaddr) < 0)
		return -1;

	reset(d);
	for(i = 0; i < sizeof macaddr; i++)
		csr8w(d, Idr0+i, macaddr[i]);

	csr8w(d, Tcr, Tr1|Tr0|Ifg1|Ifg0);
	csr16w(d, Rcr, Tail|Ad|Ab);

	r = csr16r(d, Rcr) & ~(Am | Aam | Aap);
	csr16w(d, Rcr, r);
	csr32w(d, Mar0, 0);
	csr32w(d, Mar4, 0);

	csr8w(d, Cr, Te|Re);

	epreceive = urlreceive;
	eptransmit = urltransmit;
	eppromiscuous = urlpromiscuous;
	epmulticast = urlmulticast;

	return 0;
}
