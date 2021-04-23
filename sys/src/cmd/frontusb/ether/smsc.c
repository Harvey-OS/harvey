/*
 * SMSC LAN95XX
 */

#include <u.h>
#include <libc.h>
#include <thread.h>

#include "usb.h"
#include "dat.h"

enum {
	Doburst		= 1,
	Resettime	= 1000,
	E2pbusytime	= 1000,
	Afcdefault	= 0xF830A1,
	Hsburst		= 24,
	Fsburst		= 129,
	Defbulkdly	= 1000,

	Ethp8021q	= 0x8100,
	MACoffset 	= 1,
	PHYinternal	= 1,
	Rxerror		= 0x8000,
	Txfirst		= 0x2000,
	Txlast		= 0x1000,

	/* USB vendor requests */
	Writereg	= 0xA0,
	Readreg		= 0xA1,

	/* device registers */
	Intsts		= 0x08,
	Txcfg		= 0x10,
		Txon	= 1<<2,
	Hwcfg		= 0x14,
		Bir	= 1<<12,
		Rxdoff	= 3<<9,
		Mef	= 1<<5,
		Lrst	= 1<<3,
		Bce	= 1<<1,
	Pmctrl		= 0x20,
		Phyrst	= 1<<4,
	Ledgpio		= 0x24,
		Ledspd	= 1<<24,
		Ledlnk	= 1<<20,
		Ledfdx	= 1<<16,
	Afccfg		= 0x2C,
	E2pcmd		= 0x30,
		Busy	= 1<<31,
		Timeout	= 1<<10,
		Read	= 0,
	E2pdata		= 0x34,
	Burstcap	= 0x38,
	Intepctl	= 0x68,
		Phyint	= 1<<15,
	Bulkdelay	= 0x6C,
	Maccr		= 0x100,
		Mcpas	= 1<<19,
		Prms	= 1<<18,
		Hpfilt	= 1<<13,
		Txen	= 1<<3,
		Rxen	= 1<<2,
	Addrh		= 0x104,
	Addrl		= 0x108,
	Hashh		= 0x10C,
	Hashl		= 0x110,
	MIIaddr		= 0x114,
		MIIwrite= 1<<1,
		MIIread	= 0<<1,
		MIIbusy	= 1<<0,
	MIIdata		= 0x118,
	Flow		= 0x11C,
	Vlan1		= 0x120,
	Coecr		= 0x130,
		Txcoe	= 1<<16,
		Rxcoemd	= 1<<1,
		Rxcoe	= 1<<0,

	/* MII registers */
	Bmcr		= 0,
		Bmcrreset= 1<<15,
		Speed100= 1<<13,
		Anenable= 1<<12,
		Anrestart= 1<<9,
		Fulldpx	= 1<<8,
	Bmsr		= 1,
	Advertise	= 4,
		Adcsma	= 0x0001,
		Ad10h	= 0x0020,
		Ad10f	= 0x0040,
		Ad100h	= 0x0080,
		Ad100f	= 0x0100,
		Adpause	= 0x0400,
		Adpauseasym= 0x0800,
		Adall	= Ad10h|Ad10f|Ad100h|Ad100f,
	Phyintsrc	= 29,
	Phyintmask	= 30,
		Anegcomp= 1<<6,
		Linkdown= 1<<4,
};

static int
wr(Dev *d, int reg, int val)
{
	int ret;

	ret = usbcmd(d, Rh2d|Rvendor|Rdev, Writereg, 0, reg,
		(uchar*)&val, sizeof(val));
	if(ret < 0)
		fprint(2, "%s: wr(%x, %x): %r", argv0, reg, val);
	return ret;
}

static int
rr(Dev *d, int reg)
{
	int ret, rval;

	ret = usbcmd(d, Rd2h|Rvendor|Rdev, Readreg, 0, reg,
		(uchar*)&rval, sizeof(rval));
	if(ret < 0){
		fprint(2, "%s: rr(%x): %r", argv0, reg);
		return 0;
	}
	return rval;
}

static int
miird(Dev *d, int idx)
{
	while(rr(d, MIIaddr) & MIIbusy)
		;
	wr(d, MIIaddr, PHYinternal<<11 | idx<<6 | MIIread);
	while(rr(d, MIIaddr) & MIIbusy)
		;
	return rr(d, MIIdata);
}

static void
miiwr(Dev *d, int idx, int val)
{
	while(rr(d, MIIaddr) & MIIbusy)
		;
	wr(d, MIIdata, val);
	wr(d, MIIaddr, PHYinternal<<11 | idx<<6 | MIIwrite);
	while(rr(d, MIIaddr) & MIIbusy)
		;
}

static int
eepromr(Dev *d, int off, uchar *buf, int len)
{
	int i, v;

	for(i = 0; i < E2pbusytime; i++)
		if((rr(d, E2pcmd) & Busy) == 0)
			break;
	if(i == E2pbusytime)
		return -1;
	for(i = 0; i < len; i++){
		wr(d, E2pcmd, Busy|Read|(i+off));
		while((v = rr(d, E2pcmd) & (Busy|Timeout)) == Busy)
			;
		if(v & Timeout)
			return -1;
		buf[i] = rr(d, E2pdata);
	}
	return 0;
}

static void
phyinit(Dev *d)
{
	int i;

	miiwr(d, Bmcr, Bmcrreset|Anenable);
	for(i = 0; i < Resettime/10; i++){
		if((miird(d, Bmcr) & Bmcrreset) == 0)
			break;
		sleep(10);
	}
	miiwr(d, Advertise, Adcsma|Adall|Adpause|Adpauseasym);
//	miiwr(d, Advertise, Adcsma|Ad10f|Ad10h|Adpause|Adpauseasym);
	miird(d, Phyintsrc);
	miiwr(d, Phyintmask, Anegcomp|Linkdown);
	miiwr(d, Bmcr, miird(d, Bmcr)|Anenable|Anrestart);
}


static int
doreset(Dev *d, int reg, int bit)
{
	int i;

	if(wr(d, reg, bit) < 0)
		return -1;
	for(i = 0; i < Resettime/10; i++){
		 if((rr(d, reg) & bit) == 0)
			return 1;
		sleep(10);
	}
	return 0;
}

static int
smscreceive(Dev *ep)
{
	Block *b;
	uint hd;
	int n;

	n = Doburst? Hsburst*512 : Maxpkt+4;
	b = allocb(n);
	if((n = read(ep->dfd, b->wp, n)) < 0){
		freeb(b);
		return -1;
	}
	b->wp += n;
	while(BLEN(b) >= 4){
		hd = GET4(b->rp);
		b->rp += 4;
		n = hd >> 16;
		if(n > BLEN(b))
			break;
		if((hd & Rxerror) == 0){
			if(n == BLEN(b)){
				etheriq(b);
				return 0;
			}
			etheriq(copyblock(b, n));
		}
		b->rp += (n + 3) & ~3;
	}
	freeb(b);
	return 0;
}

static void
smsctransmit(Dev *ep, Block *b)
{
	int n;

	n = BLEN(b);
	b->rp -= 8;
	PUT4(b->rp, n | Txfirst | Txlast);
	PUT4(b->rp+4, n);
	write(ep->dfd, b->rp, BLEN(b));
	freeb(b);
}

static int
smscpromiscuous(Dev *d, int on)
{
	int rxctl;

	rxctl = rr(d, Maccr);
	if(on)
		rxctl |= Prms;
	else
		rxctl &= ~Prms;
	return wr(d, Maccr, rxctl);
}

static int
smscmulticast(Dev *d, uchar *, int)
{
	int rxctl;

	rxctl = rr(d, Maccr);
	if(nmulti != 0)
		rxctl |= Mcpas;
	else
		rxctl &= ~Mcpas;
	return wr(d, Maccr, rxctl);
}

int
smscinit(Dev *d)
{
	if(!doreset(d, Hwcfg, Lrst) || !doreset(d, Pmctrl, Phyrst))
		return -1;
	if(!setmac)
		if(eepromr(d, MACoffset, macaddr, Eaddrlen) < 0)
			return -1;
	wr(d, Addrl, GET4(macaddr));
	wr(d, Addrh, GET2(macaddr+4));
	if(Doburst){
		wr(d, Hwcfg, (rr(d,Hwcfg)&~Rxdoff)|Bir|Mef|Bce);
		wr(d, Burstcap, Hsburst);
	}else{
		wr(d, Hwcfg, (rr(d,Hwcfg)&~(Rxdoff|Mef|Bce))|Bir);
		wr(d, Burstcap, 0);
	}
	wr(d, Bulkdelay, Defbulkdly);
	wr(d, Intsts, ~0);
	wr(d, Ledgpio, Ledspd|Ledlnk|Ledfdx);
	wr(d, Flow, 0);
	wr(d, Afccfg, Afcdefault);
	wr(d, Vlan1, Ethp8021q);
	wr(d, Coecr, rr(d,Coecr)&~(Txcoe|Rxcoe)); /* TODO could offload checksums? */

	wr(d, Hashh, 0);
	wr(d, Hashl, 0);
	wr(d, Maccr, rr(d,Maccr)&~(Prms|Mcpas|Hpfilt));

	phyinit(d);

	wr(d, Intepctl, rr(d, Intepctl)|Phyint);
	wr(d, Maccr, rr(d, Maccr)|Txen|Rxen);
	wr(d, Txcfg, Txon);

	eptransmit = smsctransmit;
	epreceive = smscreceive;
	eppromiscuous = smscpromiscuous;
	epmulticast = smscmulticast;

	return 0;
}
