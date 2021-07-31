#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

enum
{
	Rownid		= 0x00,		/* own ID/CDB size register */
	  Eaf		=   0x08,	/* enable advanced features */
	  Fs16MHz	=   0x80,	/* frequency select */
	Rcontrol	= 0x01,		/* control register */
	  Hsp		=   0x01,	/* halt on scsi parity error */
	  Idi		=   0x04,	/* intermediate disconnect interrupt */
	  Edi		=   0x08,	/* ending disconnect interrupt */
	  BurstDma	=   0x20,	/* burst mode dma */
	Rtimeout	= 0x02,		/* timeout period register */
	Rcdb		= 0x03,		/* CDB first byte */
	Rlun		= 0x0F,		/* target LUN register */
	Rcmdphase	= 0x10,		/* command phase register */
	Rsync		= 0x11,		/* synchronous transfer register */
	Rtchi		= 0x12,		/* transfer count hi register */
	Rtcmi		= 0x13,		/* transfer count mi register */
	Rtclo		= 0x14,		/* transfer count lo register */
	Rdestid		= 0x15,		/* destination ID */
	  Dpd		=   0x40,	/* data-phase direction (read) */
	Rsrcid		= 0x16,		/* source ID */
	  Siv		=   0x08,	/* source ID is valid */
	  Er		=   0x80,	/* enable reselection */
	Rstatus		= 0x17,		/* scsi status register */
	Rcmd		= 0x18,		/* command register */
	Rdata		= 0x19,		/* data register */
	Raux		= 0x1F,		/* auxiliary status register */
	  Dbr		=   0x01,	/* data buffer ready */
	  Cip		=   0x10,	/* command in progress */
	  Intrpend	=   0x80,	/* interrupt pending */

	Creset		= 0x00,		/* reset */
	Catn		= 0x02,		/* assert ATN */
	Cnack		= 0x03,		/* negate ACK */
	CselectATN	= 0x06,		/* select with ATN */
	Cselect		= 0x07,		/* select without ATN */
	CtransferATN	= 0x08,		/* select with ATN and transfer */
	Ctransfer	= 0x09,		/* select without ATN and transfer */
	Ctinfo		= 0x20,		/* transfer info */
	Csbt		= 0x80,		/* single-byte transfer */

	Mcomplete	= 0x00,		/* complete */
	Msaveptr	= 0x02,		/* save pointers */
	Mdisconnect	= 0x04,		/* disconnect */
	Mabort		= 0x06,		/* abort */
	Mreject		= 0x07,		/* reject */
	Mnop		= 0x08,		/* nop */
	Mbdr		= 0x0C,		/* bus device reset */
	Midentify	= (0x80|0x40),	/* identify|disconnectok */
};

typedef struct Target Target;
struct Target
{
	QLock;
	Lock	regs;
	Rendez;
	ulong	flags;
	uchar	sync;
	uchar	transfer;
	uchar	cmdphase;

	Target	*active;		/* link on active list */

	int	target;			/* SCSI Unit and logical unit */
	int	lun;
	int	status;

	struct {			/* DMA things */
		ulong	dmaaddr;
		ulong	dmalen;
		ulong	dmatx;
		int	rflag;

		ulong	*dmacnt;
		uchar	*dmafls;
		ulong	dmamap;
	};

	uchar	cdb[16];
	int	clen;

	uchar	*addr;
	uchar	*data;
	Target	*base;

	Target	*ctlr;
};

enum
{
	NCtlr		= 2,
	NTarget		= 8,
	CtlrID		= 0,

	Fbusy		= 0x0001,
	Fidentified	= 0x0002,
	Fdisconnected	= 0x0004,
	Freselect	= 0x0008,
	Fcomplete	= 0x0010,
	Fsyncxfer	= 0x0100,
	Freadxfer	= 0x0200,
	Fdmaxfer	= 0x0400,
	Fretry		= 0x1000,
	Fidentifying	= 0x2000,
};

static Target target[NCtlr][NTarget];

static	void	dmamap(Target*);

static void
issue(Target *ctlr, uchar cmd)
{
	while(*ctlr->addr & Cip)
		;
	*ctlr->addr = Rcmd;
	*ctlr->data = cmd;
}

/*
 * Set up and start a data transfer. This maybe
 * a continuation.
 */
static void
start(Target *tp)
{
	ulong count;
	Target *dev = tp->ctlr;

	/*
	 * Set up the chip for the transfer.
	 * First the LUN and transfer mode.
	 */
	*dev->addr = Rlun;
	*dev->data = tp->lun;
	*dev->data = tp->cmdphase;
	*dev->data = tp->sync;

	/*
	 * Set up transfer count and targetID+direction.
	 * No need to set the address register here, we're
	 * already pointing to the right one.
	 */
	count = tp->dmalen;
	*dev->data = (count>>16) & 0xFF;
	*dev->data = (count>>8) & 0xFF;
	*dev->data = count & 0xFF;

	if(tp->rflag == 0)
		*dev->data = Dpd|tp->target;
	else
		*dev->data = tp->target;

	/*
	 * Load the command into the chip.
	 */
	if(tp->cmdphase < 0x40) {
		*dev->addr = Rownid;
		*dev->data = tp->clen;
		*dev->addr = Rcdb;

		for(count = 0; count < tp->clen; count++)
			*dev->data = tp->cdb[count];
	}
	dmamap(tp);

	/*
	 * Select and transfer.
	 */
	issue(dev, tp->transfer);
}

/*
 * Arbitrate for the bus and start a data transfer.
 * If this is the first time through for the target,
 * i.e. we haven't identified yet, just set up for
 * selection so we can try a synchronous negotiation.
 */
static void
arbitrate(Target *dev)
{
	Target *tp;

	tp = dev->active;

	tp->cmdphase = 0x00;

	if(tp->flags & Fidentified) {
		start(tp);
		return;
	}

	/*
	 * Fretry means that we will try this request again,
	 * but without initialising the command phase, so we will
	 * continue from where we left off.
	 */
	tp->flags |= Fidentifying|Fretry;
	*dev->addr = Rlun;
	*dev->data = tp->lun;
	*dev->data = 0x00;				/* cmdphase */
	*dev->addr = Rdestid;
	*dev->data = tp->target;
	issue(dev, CselectATN);
}

static int
done(void *arg)
{
	return (((Target*)arg)->flags & Fbusy) == 0;
}

static void
scsiregs(Target *ctlr)
{
	int x;

	print("scsi aux status #%.2ux\n", *ctlr->addr & 0xFF);
	for(*ctlr->addr = 0x00, x = 0x00; x < 0x19; x++){
		if((x & 0x07) == 0x07)
			print("#%.2ux: #%.2ux\n", x, *ctlr->data & 0xFF);
		else
			print("#%.2ux: #%.2ux ", x, *ctlr->data & 0xFF);
	}
}

static int
scsiio(int nbus, Scsi *p, int rw)
{
	ulong s;
	int status;
	Target *ctlr, **l, *f, *tp;

	tp = &target[nbus][p->target];
	ctlr = tp->ctlr;

	qlock(tp);
	tp->flags |= Fbusy;
	tp->rflag = rw;
	tp->dmaaddr = (ulong)p->data.base;
	tp->dmalen = p->data.lim - p->data.base;
	tp->dmatx = 0;
	tp->status = 0x6002;
	tp->lun = p->lun;

	tp->clen = p->cmd.lim - p->cmd.base;
	memmove(tp->cdb, p->cmd.base, tp->clen);

	s = splhi();
	lock(&ctlr->regs);

	/*
	 * Link the target onto the end of the
	 * ctlr active list and start the request if
	 * the controller is idle.
	 */
	tp->active = 0;
	l = &ctlr->active;
	for(f = *l; f; f = f->active)
		l = &f->active;
	*l = tp;

	if((ctlr->flags & Fbusy) == 0){
		ctlr->flags |= Fbusy;
		arbitrate(ctlr);
	}

	unlock(&ctlr->regs);
	splx(s);

	while(waserror())
		;

	tsleep(tp, done, tp, 60*5*1000);

	if(!done(tp)) {
		print("scsi%d: Timeout! cmd=#%2.2lux\n", nbus, tp->cdb[0]);
		scsiregs(ctlr);
	}

	status = tp->status;
	p->data.ptr += tp->dmatx;

	qunlock(tp);
	poperror();

	return status;
}

int
scsiexec(Scsi *p, int read)
{
	int rw;

	rw = 1;
	if(read == ScsiIn)
		rw = 0;

	if(p->target == CtlrID)
		return 0x6002;

	p->status = scsiio(p->bus, p, rw);

	return p->status;
}

/*
 * Target request complete.
 * Only called from interrupt level,
 * i.e. ctlr is 'locked'.
 */
static void
complete(Target *ctlr)
{
	Target *tp;

	tp = ctlr->active;
	tp->flags &= (Fretry|Fidentified|Fbusy);

	/*
	 * Retry (continue) the operation if necessary
	 * (used for initial identify), otherwise unlink from
	 * the ctlr active list.
	 */
	if(tp->flags & Fretry){
		tp->flags &= ~Fretry;
		start(tp);
		return;
	}

	ctlr->active = tp->active;

	/*
	 * All done with this target, wakeup scsiio.
	 * If there are other targets queued, start
	 * one up.
	 */
	tp->flags &= Fidentified;
	*tp->dmafls = 0;

	wakeup(tp);

	if(ctlr->active){
		arbitrate(ctlr);
		return;
	}
	ctlr->flags &= ~Fbusy;
}

/*
 * A target has disconnected.
 * Unlink it from the active list and
 * start another if possible.
 * Only called from interrupt level,
 * i.e. ctlr is 'locked'.
 */
static void
disconnect(Target *ctlr)
{
	Target *tp;

	tp = ctlr->active;
	tp->flags |= Fdisconnected;
	ctlr->active = tp->active;

	if(ctlr->active)
		arbitrate(ctlr);
	else
		ctlr->flags &= ~Fbusy;
}

/*
 * A target reselected, put it back
 * on the ctlr active list. We mark the ctlr
 * busy, it's the responsibility of the caller
 * to restart the transfer.
 * Only called from interrupt level,
 * i.e. ctlr is 'locked'.
 */
static void
reselect(Target *ctlr, uchar id)
{
	Target *tp;

	tp = &ctlr->base[id];
	tp->active = ctlr->active;
	ctlr->active = tp;
	ctlr->flags |= Fbusy;
	tp->flags &= ~Fdisconnected;
}

static int
identify(Target *tp, uchar status)
{
	int i, n;
	Target *dev;
	static uchar msgout[6] = {
		0x00, 0x01, 0x03, 0x01, 0x3F, 0x0C
	};
	uchar msgin[16];

	dev = tp->ctlr;

	switch(status){

	case 0x11:				/* select complete */
		/*
		 * This is what WD calls an 'extended interrupt',
		 * the next interrupt (0x8E) is already in the
		 * pipe, we don't have to do anything.
		 */
		break;

	case 0x8E:				/* service required - MSGOUT */
		/*
		 * Send a synchronous data transfer request.
		 * The target may act strangely here and not respond
		 * correctly.
		 */
		tp->sync = 0x00;		/* asynchronous transfer mode */
		tp->flags |= Fidentified;
		*dev->addr = Rtchi;
		*dev->data = 0x00;
		*dev->data = 0x00;
		*dev->data = sizeof(msgout);
		issue(dev, Ctinfo);
		msgout[0] = Midentify|tp->lun;
		*dev->addr = Rdata;
		n = 0;
		while(n < sizeof(msgout)) {
			/*
			 * Wait until the chip says ready.
			 * If an interrupt is indicated, something
			 * went wrong, so get out of here and let
			 * the interrupt happen.
			 */
			if(*dev->addr & Intrpend)
				return 0;
			if(*dev->addr & Dbr){
				*dev->data = msgout[n];
				n++;
			}
		}
		break;

	case 0x1F:				/* transfer complete - MSGIN */
	case 0x4F:				/* unexpected MSGIN */
		/*
		 * Collect the response. The first 3 bytes
		 * of an extended message are:
		 *  0. extended message code	(0x01)
		 *  1. extended message length	(0x03)
		 *  2. sync. tranfer req. code	(0x01)
		 * Only other defined code with length >1 is
		 * 'modify data pointer', which has length 7;
		 * we better not receive anything much longer...
		 * Adjust the number of bytes to collect once we see
		 * how long the response is (byte 2).
		 * If we collect anything other than an extended message
		 * or sync. transfer req. response, toss it
		 * and assume async.
		 */
		*dev->addr = Rtchi;
		*dev->data = 0x00;
		*dev->data = 0x00;
		*dev->data = 5;
		issue(dev, Ctinfo);
		*dev->addr = Rdata;
		for(i = sizeof(msgin)-1, n = 0; n < i; n++){
			while((*dev->addr & Dbr) == 0){
				if(*dev->addr & Intrpend)
					return 0;
			}
			msgin[n] = *dev->data;
			if(n == 1)
				i = msgin[1] + 2;
		}

		if(msgin[2] == 0x01 && (n = msgin[3]) && n != 0xFF){
			if(n < 0x40)
				n = 0x02;
			else if(n < 0x80)
				n = 0x04;
			else
				n = 0;
			n |= msgin[4]<<4;
			tp->sync = n;
		}
		break;

	case 0x4E:
		/*
		 * The target went to message-out phase, so send
		 * it a NOP to keep it happy.
		 * There seems to be no reason for this delay, other than
		 * that we just end up busy without an interrupt if it
		 * isn't there.
		 * I wish I understood this.
		 */
		delay(10);
		*dev->addr = Rstatus;
		n = *dev->data;
		USED(n);

		*dev->addr = Rtchi;
		*dev->data = 0x00;
		*dev->data = 0x00;
		*dev->data = 1;
		issue(dev, Ctinfo);
		*dev->addr = Rdata;
		while((*dev->addr & Dbr) == 0){
			if(*dev->addr & Intrpend)
				return 0;
		}
		*dev->data = Mnop;
		break;

	case 0x20:					/* transfer info paused */
		issue(dev, Cnack);
		break;

	case 0x1A:
	case 0x8A:
		/*
		 * Fretry should be set, so we will
		 * pick up from here.
		 */
		tp->cmdphase = 0x30;		/* command phase has begun */
		complete(dev);
		break;

	default:
		return 1;
	}
	return 0;
}

static void
saveptr(Target *tp)
{
	Target *dev;
	ulong count, tx;

	*tp->dmafls = 0;

	dev = tp->ctlr;
	/*
	 * The transfer count register contains the
	 * number of bytes NOT transferred.
	 */
	*dev->addr = Rtchi;
	count =  (*dev->data & 0xFF)<<16;
	count += (*dev->data & 0xFF)<<8;
	count += (*dev->data & 0xFF);

	*dev->addr = Rtchi;
	*dev->data = 0;
	*dev->data = 0;
	*dev->data = 0;

	tx = tp->dmalen - count;
	tp->dmaaddr += tx;
	tp->dmalen = count;
	tp->dmatx += tx;
}

int
scsistatus(Target *ctlr)
{
	ulong status;

	*ctlr->addr = Rlun;
	status = *ctlr->data & 0xFF;		/* target status */
	status |= *ctlr->data<<8;		/* command phase */

	return status;
}

static void
scsiprocessint(int ctlrno, Target *ctlr)
{
	Target *tp;
	uchar aux, status, sid, phase, x;

	tp = ctlr->active;

	while(*ctlr->addr & 0x20)		/* Wait for busy to clear */
		;

	aux = *ctlr->addr;
	if((aux&0x80) == 0) {
		print("scsi%d: bogus int #%2.2lux\n", ctlrno, *ctlr->addr);
		return;
	}
	if(aux & 0x02)
		print("scsi%d: parity error (ignored)\n", ctlrno);
	if(aux & 0x10)
		print("scsi%d: cip #%2.2lux\n", ctlrno, aux);

	*ctlr->addr = Rcmdphase;
	phase = *ctlr->data;
	*ctlr->addr = Rsrcid;
	sid = *ctlr->data;
	*ctlr->addr = Rstatus;
	status = *ctlr->data;

	if((aux&0x40) && status != 0x81)
		print("lci but not reconnect #%2.2lux\n", status);

	switch(status) {
	case 0x42:				/* timeout */
		tp->flags &= ~Fretry;
		scsistatus(ctlr);		/* Must read lun register */
		complete(ctlr);
		return;

	case 0x16:				/* select-and-transfer complete */
		saveptr(tp);
		tp->status = scsistatus(ctlr);	/* target status */
		complete(ctlr);
		return;

	case 0x21:				/* save data pointers */
		saveptr(tp);
		issue(ctlr, tp->transfer);
		return;

	case 0x4B:				/* unexpected status phase */
		saveptr(tp);
		*ctlr->addr = Rcmdphase;
		if(phase <= 0x3c)
			*ctlr->data = 0x41;	/* resume no data */
		else
			*ctlr->data = 0x46;	/* collect status */
		issue(ctlr, tp->transfer);
		return;

	case 0x85:				/* disconnect */
		if(tp->flags & Fidentifying){
			tp->cmdphase = 0x00;	/* complete retry */
			complete(ctlr);
			return;
		}
		disconnect(ctlr);
		return;

	case 0x81:
		*ctlr->addr = Rdata;
		x = *ctlr->data;		/* the identify message */
		USED(x);
		if((sid & Siv) == 0) {		/* source ID valid */
			print("scsi%d: invalid src id\n", ctlrno);
			break;
		}
		reselect(ctlr, sid & 0x07);
		ctlr->active->cmdphase = 0x45;	/* reselected by target */
		start(ctlr->active);
		return;

	default:
		if(tp == 0) {
			print("scsi%d: bogus sr #%2.2lux phase #%2.2lux\n",
					ctlrno, status, phase);
			return;
		}
		if((tp->flags & Fidentifying) && identify(tp, status) == 0)
			return;
		break;
	}
	print("scsi%d:\n", ctlrno);
	if(tp) print("unit %d cmd #%2.2lux flags #%4.4lux phase #%2.2lux\n",
			tp->target, tp->cdb[0], tp->flags, tp->cmdphase);
	print("scsi status #%.2ux phase #%2.2lux\n", status, phase);
	print("aux  status #%2.2ux was #%2.2lux\n", *ctlr->addr, aux);
	for(*ctlr->addr = 0x00, x = 0x00; x < 0x19; x++){
		if((x & 0x07) == 0x07)
			print("#%.2ux: #%2.2ux\n", x, *ctlr->data);
		else
			print("#%.2ux: #%2.2ux ", x, *ctlr->data);
	}
	panic("\nscsiintr");
}

void
scsiintr(int ctlrno)
{
	Target *ctlr;

	ctlr = &target[ctlrno][CtlrID];

	lock(&ctlr->regs);
	scsiprocessint(ctlrno, ctlr);
	unlock(&ctlr->regs);
}

void
scsiicltr(int ctlrnr, uchar *data, uchar *addr, ulong *cnt, uchar *fls, ulong map)
{
	int i;
	uchar status;
	Target *ctlr, *tp;

	ctlr = &target[ctlrnr][CtlrID];

	ctlr->data = data;
	ctlr->addr = addr;
	ctlr->base = target[ctlrnr];

	if(*ctlr->addr & Intrpend) {
		*ctlr->addr = Rstatus;
		status = *ctlr->data;
		USED(status);
	}

	/*
	 * Give the chip a soft reset. This will
	 * cause an interrupt. Wait for it and clear it.
	 */
	*ctlr->addr = Rownid;
	*ctlr->data = Fs16MHz|Eaf|CtlrID;
	*ctlr->addr = Rcmd;
	*ctlr->data = Creset;

	while((*ctlr->addr & Intrpend) == 0)
		delay(250);

	*ctlr->addr = Rstatus;
	status = *ctlr->data;
	USED(status);

	/*
	 * Freselect means enable the chip to
	 * respond to reselects.
	 */
	ctlr->flags = Freselect|Fidentified;

	*ctlr->addr = Rcontrol;
	*ctlr->data = BurstDma|Edi|Idi|Hsp;
	*ctlr->addr = Rtimeout;
	*ctlr->data = 40;				/* 200ms */

	*ctlr->addr = Rsrcid;
	if(ctlr->flags & Freselect)
		*ctlr->data = Er;
	else
		*ctlr->data = 0x00;

	for(i = 0; i < NTarget; i++){
		if(i == CtlrID)
			continue;

		tp = &target[ctlrnr][i];
		tp->lun = 0;
		tp->target = i;
		tp->ctlr = ctlr;
		tp->dmacnt = cnt;
		tp->dmafls = fls;
		tp->dmamap = map;

		/* Set so scsibus one does not try to perform
		 * synchronous negotiation.
		 */
		if(ctlrnr == 0 && (ctlr->flags & Freselect)){
			tp->flags = 0;
			tp->transfer = CtransferATN;
		}
		else {
			tp->flags = Fidentified;
			tp->transfer = Ctransfer;
		}
	}
}

/*
 * Called at boot time.
 * Initialise the hardware.
 * There will be an interrupt in reponse
 * to the reset.
 */
void
resetscsi(void)
{
	uchar m3;

	/*
	 * Reset both scsi busses
	 */
	m3 = MODEREG->promenet;
	m3 &= ~((1<<6)|(1<<7));
	MODEREG->promenet = m3 | (1<<6) | (1<<7);
	delay(500);
	MODEREG->promenet = m3;
	delay(100);

	scsiicltr(0, SCSI0DATA, SCSI0ADDR, SCSI0CNT, SCSI0FLS, 0x1fc0);
	scsiicltr(1, SCSI1DATA, SCSI1ADDR, SCSI1CNT, SCSI1FLS, 0x1f80);
}

static void
dmamap(Target *tp)
{
	ulong y, len, addr, index, off;

	*tp->dmafls = 1;

	len  = tp->dmalen;
	if(len == 0)
		return;

	addr = tp->dmaaddr;
	index = tp->dmamap;
	for(y = addr+len+BY2PG; addr < y; addr += BY2PG){
		*WRITEMAP = (index<<16)|(addr>>12)&0xFFFF;
		index++;
	}
	off = tp->dmaaddr & (BY2PG-1);
	if(tp->rflag == 0)
		off |= 1<<15;

	*tp->dmacnt = off;
}

/*
 * Known to devscsi.c.
 */
int scsidebugs[8];
int scsiownid = CtlrID;

void
initscsi(void)
{
}
