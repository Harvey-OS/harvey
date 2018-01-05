/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * SMSC LAN95XX
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <usb/usb.h>
#include <usb/usbfs.h>
#include <usb/ether.h>

enum {
	Doburst		= 1,
	Resettime	= 1000,
	E2pbusytime	= 1000,
	Afcdefault	= 0xF830A1,
//	Hsburst		= 37,	/* from original linux driver */
	Hsburst		= 8,
	Fsburst		= 129,
	Defbulkdly	= 0x2000,

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
		(uint8_t*)&val, sizeof(val));
	if(ret < 0)
		deprint(2, "%s: wr(%x, %x): %r", argv0, reg, val);
	return ret;
}

static int
rr(Dev *d, int reg)
{
	int ret, rval;

	ret = usbcmd(d, Rd2h|Rvendor|Rdev, Readreg, 0, reg,
		(uint8_t*)&rval, sizeof(rval));
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
eepromr(Dev *d, int off, uint8_t *buf, int len)
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
getmac(Dev *d, uint8_t buf[])
{
	int i;
	uint8_t ea[Eaddrlen];

	if(eepromr(d, MACoffset, ea, Eaddrlen) < 0)
		return -1;
	for(i = 0; i < Eaddrlen; i++)
		if(ea[i] != 0 && ea[i] != 0xFF){
			memmove(buf, ea, Eaddrlen);
			break;
		}
	return Eaddrlen;
}

static int
smscinit(Ether *ether)
{
	Dev *d;

	if(ether->cid != S95xx)
		return -1;
	d = ether->dev;
	deprint(2, "%s: setting up SMSC95XX\n", argv0);
	if(!doreset(d, Hwcfg, Lrst) || !doreset(d, Pmctrl, Phyrst))
		return -1;
	if(getmac(d, ether->addr) < 0)
		return -1;
	wr(d, Addrl, GET4(ether->addr));
	wr(d, Addrh, GET2(ether->addr+4));
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

	return 0;
}

static int32_t
smscbread(Ether *e, Buf *bp)
{
	uint hd;
	int n, m;
	Buf *rbp;

	rbp = e->Etherops.aux;
	if(rbp->ndata < 4){
		rbp->rp = rbp->data;
		rbp->ndata = read(e->epin->dfd, rbp->rp, Doburst? Hsburst*512:
			Maxpkt);
		if(rbp->ndata < 0)
			return -1;
	}
	if(rbp->ndata < 4){
		werrstr("short frame");
		fprint(2, "smsc short frame %d bytes\n", rbp->ndata);
		return 0;
	}
	hd = GET4(rbp->rp);
	n = hd >> 16;
	m = (n + 4 + 3) & ~3;
	if(n < 6 || m > rbp->ndata){
		werrstr("frame length");
		fprint(2, "smsc length error packet %d buf %d\n", n, rbp->ndata);
		rbp->ndata = 0;
		return 0;
	}
	if(hd & Rxerror){
		fprint(2, "smsc rx error %8.8x\n", hd);
		n = 0;
	}else{
		bp->rp = bp->data + Hdrsize;
		memmove(bp->rp, rbp->rp+4, n);
	}
	bp->ndata = n;
	rbp->rp += m;
	rbp->ndata -= m;
	return n;
}

static int32_t
smscbwrite(Ether *e, Buf *bp)
{
	int n;

	n = bp->ndata & 0x7FF;
	bp->rp -= 8;
	bp->ndata += 8;
	PUT4(bp->rp, n | Txfirst | Txlast);
	PUT4(bp->rp+4, n);
	n = write(e->epout->dfd, bp->rp, bp->ndata);
	return n;
}

static int
smscpromiscuous(Ether *e, int on)
{

#ifdef TODO		/* copied from asix */
	int rxctl;

	deprint(2, "%s: smscpromiscuous %d\n", argv0, on);
	rxctl = getrxctl(e->dev);
	if(on != 0)
		rxctl |= Rxctlprom;
	else
		rxctl &= ~Rxctlprom;
	return wr(e->dev, Cwrxctl, rxctl);
#endif
	return -1;
}

static int
smscmulticast(Ether *e, uint8_t *addr, int on)
{
#ifdef TODO		/* needed for ipv6; copied from asix */
	int rxctl;

	/* BUG: should write multicast filter */
	rxctl = getrxctl(e->dev);
	if(e->nmcasts != 0)
		rxctl |= Rxctlamall;
	else
		rxctl &= ~Rxctlamall;
	deprint(2, "%s: smscmulticast %d\n", argv0, e->nmcasts);
	return wr(e->dev, Cwrxctl, rxctl);
#endif
	return -1;
}

static void
smscfree(Ether *ether)
{
	free(ether->Etherops.aux);
	ether->Etherops.aux = nil;
}

int
smscreset(Ether *ether)
{
	Cinfo *ip;
	Dev *dev;

	dev = ether->dev;
	for(ip = cinfo; ip->vid != 0; ip++)
		if(ip->vid == dev->usb->vid && ip->did == dev->usb->did){
			ether->cid = ip->cid;
			if(smscinit(ether) < 0){
				deprint(2, "%s: smsc init failed: %r\n", argv0);
				return -1;
			}
			deprint(2, "%s: smsc reset done\n", argv0);
			ether->Etherops.name = "smsc";
			if(Doburst){
				ether->Etherops.bufsize = Hsburst*512;
				ether->Etherops.aux = emallocz(sizeof(Buf) +
					ether->Etherops.bufsize - Maxpkt, 1);
			}else{
				ether->Etherops.bufsize = Maxpkt;
				ether->Etherops.aux = emallocz(sizeof(Buf), 1);
			}
			ether->Etherops.free = smscfree;
			ether->Etherops.bread = smscbread;
			ether->Etherops.bwrite = smscbwrite;
			ether->Etherops.promiscuous = smscpromiscuous;
			ether->Etherops.multicast = smscmulticast;
			ether->mbps = 100;	/* BUG */
			return 0;
		}
	return -1;
}
