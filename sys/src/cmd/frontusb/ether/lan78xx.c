/*
 * Microchip (ex SMSC) LAN78XX
 *	 Also used as ethernet core in LAN7515 usb hub + ethernet
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
	Hsburst		= 32,
	Defbulkdly	= 1000,
	Rxfifosize	= (12*1024),
	Txfifosize	= (12*1024),

	MACoffset 	= 1,
	PHYinternal	= 1,
	Rxerror		= 0x00400000,
	Txfcs		= 1<<22,

	/* USB vendor requests */
	Writereg	= 0xA0,
	Readreg		= 0xA1,

	/* device registers */
	Idrev		= 0x00,
	Intsts		= 0x0C,
	Hwcfg		= 0x10,
		Led0en	= 1<<20,
		Led1en	= 1<<21,
		Mef	= 1<<4,
		Lrst	= 1<<1,
	Pmctrl		= 0x14,
		Ready	= 1<<7,
		Phyrst	= 1<<4,
	Gpiocfg0	= 0x18,
	Gpiocfg1	= 0x1C,
	E2pcmd		= 0x40,
		Busy	= 1<<31,
		Timeout	= 1<<10,
		Loaded	= 1<<9,
		Read	= 0,
	E2pdata		= 0x44,
	Burstcap	= 0x90,
	Intepctl	= 0x98,
		Phyint	= 1<<17,
	Bulkdelay	= 0x94,
	Rfectl		= 0xB0,
		Rxcoe	= 0xF<<11,
		Ab		= 1<<10,
		Am		= 1<<9,
		Au		= 1<<8,
		Dpf		= 1<<1,
	Usbcfg0		= 0x80,
		Bir	= 1<<6,
		Bce	= 1<<5,
	Usbcfg1		= 0x84,
	Rxfifoctl		= 0xC0,
		Rxen	= 1<<31,	
	Txfifoctl		= 0xC4,
		Txen	= 1<<31,
	Rxfifo		= 0xC8,
	Txfifo		= 0xCc,
	Fctflow		= 0xD0,
	Maccr		= 0x100,
		Add		= 1<<12,
		Asd		= 1<<11,
	Macrx		= 0x104,
		Macfcs	= 1<<4,
		Macrxen	= 1<<0,
	Mactx		= 0x108,
		Mactxen	= 1<<0,
	Addrh		= 0x118,
	Addrl		= 0x11C,
	MIIaddr		= 0x120,
		MIIwrite= 1<<1,
		MIIread	= 0<<1,
		MIIbusy	= 1<<0,
	MIIdata		= 0x124,
	Flow		= 0x10C,
	Addrfilth	= 0x400,
		Afvalid	= 1<<31,
	Addrfiltl	= 0x404,

	/* MII registers */
	Bmcr		= 0,
		Bmcrreset= 1<<15,
		Speed100= 1<<13,
		Anenable= 1<<12,
		Anrestart= 1<<9,
		Fulldpx	= 1<<8,
		Speed1000= 1<<6,
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
	Lpa		= 5,
	Ctrl1000	= 9,
		Ad1000h = 0x0400,
		Ad1000f = 0x0200,
	Ledmodes	= 29,
		Led0shift = 0,
		Led1shift = 4,
		Linkact = 0x0,
		Link1000 = 0x1,
	Phyintmask	= 25,
		Anegcomp= 1<<10,
		Linkchg = 1<<13,
};

static int burstcap = Hsburst, bulkdelay = Defbulkdly;

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
	wr(d, MIIaddr, PHYinternal<<11 | idx<<6 | MIIread | MIIbusy);
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
	wr(d, MIIaddr, PHYinternal<<11 | idx<<6 | MIIwrite | MIIbusy);
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
	miiwr(d, Ctrl1000, Ad1000f);
	miiwr(d, Phyintmask, 0);
	miiwr(d, Ledmodes, (Linkact<<Led1shift) | (Link1000<<Led0shift));
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
lan78xxreceive(Dev *ep)
{
	Block *b;
	uint hd;
	int n;

	n = Doburst? burstcap*512: Maxpkt;
	b = allocb(n);
	if((n = read(ep->dfd, b->wp, n)) < 10){
		freeb(b);
		return -1;
	}
	b->wp += n;
	while(BLEN(b) >= 10){
		hd = GET4(b->rp);
		b->rp += 10;
		n = hd & 0x3FFF;
		if(n > BLEN(b))
			break;
		if((hd & Rxerror) == 0){
			if(n == BLEN(b)){
				etheriq(b);
				return 0;
			}
			etheriq(copyblock(b, n));
		}
		b->rp = (uchar*)(((uintptr)b->rp + n + 3)&~3);
	}
	freeb(b);
	return 0;
}

static void
lan78xxtransmit(Dev *ep, Block *b)
{
	int n = BLEN(b) & 0xFFFFF;
	b->rp -= 8;
	PUT4(b->rp, n | Txfcs);
	PUT4(b->rp+4, n);
	write(ep->dfd, b->rp, BLEN(b));
	freeb(b);
}

static int
lan78xxpromiscuous(Dev *d, int on)
{
	int rxctl;

	rxctl = rr(d, Rfectl);
	if(on)
		rxctl |= Am|Au;
	else {
		rxctl &= ~Au;
		if(nmulti == 0)
			rxctl &= ~Am;
	}
	return wr(d, Rfectl, rxctl);
}

static int
lan78xxmulticast(Dev *d, uchar *, int)
{
	int rxctl;

	rxctl = rr(d, Rfectl);
	if(nmulti != 0)
		rxctl |= Am;
	else
		rxctl &= ~Am;
	return wr(d, Rfectl, rxctl);
}

int
lan78xxinit(Dev *d)
{
	u32int a;
	int i;

	if(!doreset(d, Hwcfg, Lrst) || !doreset(d, Pmctrl, Phyrst))
		return -1;
	for(i = 0; i < Resettime/10; i++){
		 if(rr(d, Pmctrl) & Ready)
			break;
		sleep(10);
	}
	if((rr(d, Pmctrl) & Ready) == 0){
		fprint(2, "%s: device not ready after reset\n", argv0);
		return -1;
	}

	if(!setmac)
		if(eepromr(d, MACoffset, macaddr, Eaddrlen) < 0)
			fprint(2, "%s: can't read etheraddr from EEPROM\n", argv0);

	a = GET4(macaddr);
	wr(d, Addrl, a);
	wr(d, Addrfiltl, a);
	a = GET2(macaddr+4);
	wr(d, Addrh, a);
	wr(d, Addrfilth, a|Afvalid);

	wr(d, Usbcfg0, rr(d, Usbcfg0) | Bir);
	if(Doburst){
		wr(d, Hwcfg, rr(d, Hwcfg)|Mef);
		wr(d, Usbcfg0, rr(d, Usbcfg0)|Bce);
		wr(d, Burstcap, burstcap);
		wr(d, Bulkdelay, bulkdelay);
	}else{
		wr(d, Hwcfg, rr(d, Hwcfg)&~Mef);
		wr(d, Usbcfg0, rr(d, Usbcfg0)&~Bce);
		wr(d, Burstcap, 0);
		wr(d, Bulkdelay, 0);
	}
	wr(d, Rxfifo, (Rxfifosize-512)/512);
	wr(d, Txfifo, (Txfifosize-512)/512);
	wr(d, Intsts, ~0);
	wr(d, Hwcfg, rr(d, Hwcfg) | Led0en|Led1en);
	wr(d, Flow, 0);
	wr(d, Fctflow, 0);
	wr(d, Rfectl, (rr(d, Rfectl) & ~Rxcoe) | Ab|Dpf); /* TODO could offload checksums? */

	phyinit(d);

	wr(d, Maccr, rr(d,Maccr)|Add|Asd);

	wr(d, Intepctl, rr(d, Intepctl)|Phyint);
	wr(d, Mactx, Mactxen);
	wr(d, Macrx, rr(d, Macrx) | Macfcs|Macrxen);
	wr(d, Txfifoctl, Txen);
	wr(d, Rxfifoctl, Rxen);

	eptransmit = lan78xxtransmit;
	epreceive = lan78xxreceive;
	eppromiscuous = lan78xxpromiscuous;
	epmulticast = lan78xxmulticast;

	return 0;
}
