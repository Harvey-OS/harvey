/*
 * Lots still to do:
 *	control over sync/async;
 *	control over connect/disconnect/reselection;
 *	do reset at any time;
 *	bus error recovery;
 *	ever more comments;
 *	accept sync negotiation at any time;
 *	dma;
 *	etc.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#include "ureg.h"

/*
 * Uniprocessors can't (don't need to) lock.
 */
#define lock(l)
#define unlock(l)

enum {
	NCtlr		= 1,
	CtlrID		= 7,
	ClkFreq		= 16,		/* MHz */
	NTarget		= 8,		/* includes controller */
};

enum {					/* registers */
	Rownid		= 0x00,		/* own ID/CDB size register */
	Rcontrol	= 0x01,		/* control register */
	Rtimeout	= 0x02,		/* timeout period register */
	Rcdb		= 0x03,		/* CDB first byte */
	Rlun		= 0x0F,		/* target LUN register */
	Rcmdphase	= 0x10,		/* command phase register */
	Rsync		= 0x11,		/* synchronous transfer register */
	Rtchi		= 0x12,		/* transfer count hi register */
	Rtcmi		= 0x13,		/* transfer count mi register */
	Rtclo		= 0x14,		/* transfer count lo register */
	Rdestid		= 0x15,		/* destination ID */
	Rsrcid		= 0x16,		/* source ID */
	Rstatus		= 0x17,		/* scsi status register */
	Rcmd		= 0x18,		/* command register */
	Rdata		= 0x19,		/* data register */
	Raux		= 0x1F,		/* auxiliary status register */
};

enum {					/* Rownid */
	Eaf		= 0x08,		/* enable advanced features */
	Fs10MHz		= 0x00,		/* divisor for ClkFreq 8-10MHz (2) */
	Fs15MHz		= 0x40,		/* divisor for ClkFreq 12-15MHz (3) */
	Fs16MHz		= 0x80,		/* divisor for ClkFreq 16MHz (4) */
};

enum {					/* Rcontrol */
	Hsp		= 0x01,		/* halt on scsi parity error */
	Idi		= 0x04,		/* intermediate disconnect interrupt */
	Edi		= 0x08,		/* ending disconnect interrupt */
	BurstDma	= 0x20,		/* burst mode dma */
};

enum {					/* Rdestid */
	Dpd		= 0x40,		/* data-phase direction (read) */
};

enum {					/* Rsrcid */
	Siv		= 0x08,		/* source ID is valid */
	Er		= 0x80,		/* enable reselection */
};

enum {					/* Raux */
	Dbr		= 0x01,		/* data buffer ready */
	Cip		= 0x10,		/* command in progress */
	Bsy		= 0x20,		/* busy */
	Lci		= 0x40,		/* last command ignored */
	Intrpend	= 0x80,		/* interrupt pending */
};

enum {					/* commands */
	Creset		= 0x00,		/* reset */
	Catn		= 0x02,		/* assert ATN */
	Cnack		= 0x03,		/* negate ACK */
	CselectATN	= 0x06,		/* select with ATN */
	Cselect		= 0x07,		/* select without ATN */
	CtransferATN	= 0x08,		/* select with ATN and transfer */
	Ctransfer	= 0x09,		/* select without ATN and transfer */
	Ctinfo		= 0x20,		/* transfer info */
	Csbt		= 0x80,		/* single-byte transfer */
};

enum {					/* messages */
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

struct Target {
	QLock;
	Lock	regs;			/* controller */
	Rendez;
	ulong	flags;
	uchar	sync;
	uchar	transfer;

	uchar	cmdphase;
	uchar	lastphase[2];		/* debugging */
	uchar	laststat[2];		/* debugging */

	Target	*active;		/* link on active list */

	ushort	target;			/* SCSI Unit and logical unit */
	ushort	lun;

	uchar	cdb[16];
	int	clen;
	ushort	status;

	struct {			/* DMA things */
		uchar	*dmaaddr;
		ulong	dmalen;
		ulong	dmatx;
		int	rw;
		uchar	*dmaptr;
	};

	ulong	*addr;
	ulong	*data;
	Target	*base;
	ulong	cycle;			/* transfer period in nano-seconds/4 */

	Target	*ctlr;
};

enum {
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

static void
mdelay(int ms)
{
	extern Eeprom eeprom;

	ms *= ROUNDUP(eeprom.clock, 2)/2;
	while(ms-- > 0)
		;
}

static void
scsiregs(Target *ctlr, uchar aux, uchar status)
{
	Target *tp;
	int i;

	print("scsi aux status #%2.2ux\n", aux);
	if(aux & Bsy){
		if((aux & Cip) == 0){
			*ctlr->addr = Rcmd;
			print("#%2.2ux: #%2.2ux\n", Rcmd, *ctlr->data & 0xFF);
		}
		return;
	}

	print("scsi intr status #%2.2ux\n", status);
	for(*ctlr->addr = 0x00, i = 0x00; i < 0x19; i++){
		if((i & 0x07) == 0x07)
			print("#%2.2ux: #%2.2ux\n", i, *ctlr->data & 0xFF);
		else
			print("#%2.2ux: #%2.2ux ", i, *ctlr->data & 0xFF);
	}

	if((tp = ctlr->active) == 0)
		tp = ctlr->base;
	print("target%d: dmaaddr=#%lux, dmalen=%lud, dmatx=%lud\n",
		tp->target, tp->dmaaddr, tp->dmalen, tp->dmatx);
	print("dmaptr=#%lux (%lud transferred)\n",
		tp->dmaptr, tp->dmaptr-tp->dmaaddr);
	print("lastphase[0]=#%2.2ux, lastphase[1]=#%2.2ux\n",
		tp->lastphase[0], tp->lastphase[1]);
	print("laststat[0]=#%2.2ux, laststat[1]=#%2.2ux\n",
		tp->laststat[0], tp->laststat[1]);
}

static void
scsipanic(char *s, Target *ctlr, uchar aux, uchar status)
{
	mdelay(1000);
	scsiregs(ctlr, aux, status);
	panic("scsipanic: %s\n", s);
}

static void
issue(Target *ctlr, uchar cmd)
{
	/*
	 * Mustn't touch the command register within 7us of
	 * accessing the status register. We've probably wasted
	 * a couple of us getting here, so 7 should be enough.
	 */
	mdelay(7);

	while(*ctlr->addr & Cip)
		;
	*ctlr->addr = Rcmd;
	*ctlr->data = cmd;
}

/*
 * Set up and start a data transfer.
 * We don't load the command phase here as
 * we may be continuing a stopped operation.
 */
static void
start(Target *tp)
{
	Target *ctlr = tp->ctlr;
	ulong count;

	/*
	 * Set up the chip for the transfer.
	 * First the LUN, command phase and transfer mode.
	 */
	*ctlr->addr = Rlun;
	*ctlr->data = tp->lun;
	*ctlr->data = tp->cmdphase;
	*ctlr->data = tp->sync;

	/*
	 * Set up transfer count and targetID+direction.
	 * No need to set the address register here, we're
	 * already pointing to the right one.
	 */
	count = tp->dmalen;
	*ctlr->data = (count>>16) & 0xFF;
	*ctlr->data = (count>>8) & 0xFF;
	*ctlr->data = count & 0xFF;

	if(tp->rw == ScsiIn)
		*ctlr->data = Dpd|tp->target;
	else
		*ctlr->data = tp->target;

	/*
	 * Load the command into the chip if necessary.
	 * Testing on 0x40 may not be completely correct.
	 */
	if(tp->cmdphase < 0x40){
		*ctlr->addr = Rownid;
		*ctlr->data = count;
		*ctlr->addr = Rcdb;

		for(count = 0; count < tp->clen; count++)
			*ctlr->data = tp->cdb[count];
	}

	/*
	 * Select and transfer.
	 */
	issue(ctlr, tp->transfer);
}

/*
 * Arbitrate for the bus and start a data transfer.
 * If this is the first time through for the target,
 * i.e. we haven't identified yet, just set up for
 * selection so we can try a synchronous negotiation.
 */
static void
arbitrate(Target *ctlr)
{
	Target *tp = ctlr->active;

	tp->cmdphase = 0x00;

	if(tp->flags & Fidentified){
		start(tp);
		return;
	}

	/*
	 * Fretry means that we will try this request again,
	 * but without initialising the command phase, so we will
	 * continue from where we left off.
	 */
	tp->flags |= Fidentifying|Fretry;
	*ctlr->addr = Rlun;
	*ctlr->data = tp->lun;
	*ctlr->data = 0x00;				/* cmdphase */
	*ctlr->addr = Rdestid;
	*ctlr->data = tp->target;
	issue(ctlr, CselectATN);
}

static int
done(void *arg)
{
	return (((Target*)arg)->flags & Fbusy) == 0;
}

int
scsiio(int bus, Scsi *p, int rw)
{
	Target *ctlr, *tp, **l, *f;
	ulong s;
	ushort status;

	/*
	 * Wait for the target to become free,
	 * then set it up. Bug - rflag should be set in
	 * the caller.
	 */
	tp = &target[bus][p->target];
	ctlr = tp->ctlr;

	qlock(tp);
	if(waserror()){
		qunlock(tp);
		nexterror();
	}

	tp->flags |= Fbusy;
	tp->rw = rw;
	tp->dmaaddr = tp->dmaptr = p->data.base;
	tp->dmalen = p->data.lim - p->data.base;
	tp->dmatx = 0;
	tp->status = 0x6002;

	tp->lastphase[0] = tp->lastphase[1] = 0xFF;		/* debugging */
	tp->laststat[0] = tp->laststat[1] = 0xFF;		/* debugging */

	tp->clen = p->cmd.lim - p->cmd.base;
	memmove(tp->cdb, p->cmd.base, tp->clen);

	/*
	 * Link the target onto the end of the
	 * ctlr active list and start the request if
	 * the controller is idle.
	 */
	s = splhi();
	lock(&ctlr->regs);

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

	/*
	 * Wait for the request to complete
	 * and return the status.
	 */
	sleep(tp, done, tp);

	status = tp->status;
	p->data.ptr += tp->dmatx;

	poperror();
	qunlock(tp);
	return status;
}

int
scsiexec(Scsi *p, int rw)
{
	if(p->target == CtlrID)
		return 0x6002;

	return p->status = scsiio(0, p, rw);
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

	if(ctlr->active){
		arbitrate(ctlr);
		return;
	}
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
	Target *ctlr = tp->ctlr;
	int i, n;
	static uchar msgout[6] = {
		0x00, 0x01, 0x03, 0x01, 0x3F, 0x0A
	};
	uchar msgin[16];

	switch(status){

	case 0x11:					/* select with ATN complete */
		/*
		 * This is what WD calls an 'extended interrupt',
		 * the next interrupt (0x8E) is already in the
		 * pipe, we don't have to do anything.
		 */
		break;

	case 0x8E:					/* service required - MSGOUT */
		/*
		 * Send a synchronous data transfer request.
		 * The target may act strangely here and not respond
		 * correctly.
		 */
		tp->sync = 0x00;			/* asynchronous transfer mode */
		tp->flags |= Fidentified;

		msgout[0] = Midentify|tp->lun;

		/*
		 * Send our minimum transfer period, 2 cycles.
		 */
		msgout[4] = 2*ctlr->cycle;

		*ctlr->addr = Rtchi;
		*ctlr->data = 0x00;
		*ctlr->data = 0x00;
		*ctlr->data = sizeof(msgout);
		issue(ctlr, Ctinfo);
		*ctlr->addr = Rdata;
		n = 0;
		while(n < sizeof(msgout)){
			/*
			 * Wait until the chip says ready.
			 * If an interrupt is indicated, something
			 * went wrong, so get out of here and let
			 * the interrupt happen.
			 */
			if(*ctlr->addr & Intrpend)
				return 0;
			if(*ctlr->addr & Dbr){
				*ctlr->data = msgout[n];
				n++;
			}
		}
		break;

	case 0x1F:					/* transfer complete - MSGIN */
	case 0x4F:					/* unexpected MSGIN */
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
		*ctlr->addr = Rtchi;
		*ctlr->data = 0x00;
		*ctlr->data = 0x00;
		*ctlr->data = 5;
		issue(ctlr, Ctinfo);
		*ctlr->addr = Rdata;
		for(i = sizeof(msgin)-1, n = 0; n < i; n++){
			while((*ctlr->addr & Dbr) == 0){
				if(*ctlr->addr & Intrpend)
					return 0;
			}
			msgin[n] = *ctlr->data;
			if(n == 1)
				i = msgin[1] + 2;
		}

		/*
		 * Check if we collected a valid sync response.
		 * If so, save the values.
		 * Transfer period can't be less than 2 cycles
		 * or greater than 8.
		 */
		if(msgin[2] == 0x01 && (n = msgin[3]) && n != 0xFF){
			n /= ctlr->cycle;
			if(n < 2)
				n = 2;
			if(n >= 8)
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
		mdelay(10);
		*ctlr->addr = Rstatus;
		n = *ctlr->data;

		*ctlr->addr = Rtchi;
		*ctlr->data = 0x00;
		*ctlr->data = 0x00;
		*ctlr->data = 1;
		issue(ctlr, Ctinfo);
		*ctlr->addr = Rdata;
		while((*ctlr->addr & Dbr) == 0){
			if(*ctlr->addr & Intrpend)
				return 0;
		}
		*ctlr->data = Mnop;
		break;

	case 0x20:					/* transfer info paused */
		issue(ctlr, Cnack);
		break;

	case 0x1A:
	case 0x8A:
		/*
		 * Fretry should be set, so we will
		 * pick up from here.
		 */
		tp->cmdphase = 0x30;			/* command phase has begun */
		complete(ctlr);
		break;

	default:
		return 1;
	}
	return 0;
}

static void
saveptr(Target *tp)
{
	Target *ctlr = tp->ctlr;
	ulong count, tx;

	/*
	 * The transfer count register contains the
	 * number of bytes NOT transferred.
	 */
	*ctlr->addr = Rtchi;
	count = (*ctlr->data & 0xFF)<<16;
	count += (*ctlr->data & 0xFF)<<8;
	count += (*ctlr->data & 0xFF);

	tx = tp->dmalen - count;
	tp->dmaaddr += tx;
	tp->dmalen = count;
	tp->dmatx += tx;

	tp->dmaptr = tp->dmaaddr;
}

static int
scsistatus(Target *ctlr)
{
	ulong status;

	/*
	 * The target status comes back in the Rlun
	 * register. Returned status is combined
	 * with the final command phase, which
	 * should be 0x60.
	 */
	*ctlr->addr = Rlun;
	status = *ctlr->data & 0xFF;			/* target status */
	status |= *ctlr->data<<8;			/* command phase */

	return status;
}

void
scsiintr(int ctlrno, Ureg *ur)
{
	Target *ctlr = &target[ctlrno][CtlrID];
	Target *tp = ctlr->active;
	uchar aux, cmd, lun, cmdphase, srcid, status, data;

	USED(ur);

	lock(&ctlr->lock);

	/*
	 * Wait for Bsy to go away. If it doesn't, we're
	 * dead.
	 * Check we have a valid interrupt, bail out if not.
	 */
	while((aux = *ctlr->addr) & Bsy)
		;

	/*
	 * Read all the registers we need before reading the status
	 * register, so they don't change under foot if the chip
	 * decides to post another interrupt after that.
	 */
	*ctlr->addr = Rcmd;
	cmd = *ctlr->data;
	*ctlr->addr = Rlun;
	lun = *ctlr->data;
	cmdphase = *ctlr->data;
	*ctlr->addr = Rsrcid;
	srcid = *ctlr->data;

	*ctlr->addr = Rstatus;
	status = *ctlr->data;

	if((aux & Intrpend) == 0){
		print("stray #%lux\n", ur->user);
		scsipanic("stray", ctlr, aux, status);
		return;
	}

	/*
	 * Check for last-command-ignored, which can
	 * happen if the command is issued just prior to
	 * or concurrent with a pending interrupt.
	 *
	 * Should be smarter here...
	 */
	if(aux  & Lci)
		scsipanic("Lci", ctlr, aux, status);

	switch(status){

	case 0x42:					/* timeout */
		tp->flags &= ~Fretry;
		scsistatus(ctlr);			/* Must read lun register */
		complete(ctlr);
		break;

	case 0x16:					/* select-and-transfer complete */
		saveptr(tp);
		tp->status = scsistatus(ctlr);		/* target status */
		complete(ctlr);
		break;

	case 0x21:					/* save data pointers */
		saveptr(tp);
		issue(ctlr, tp->transfer);
		break;

	case 0x4B:					/* unexpected status phase */
		saveptr(tp);
		*ctlr->addr = Rcmdphase;
		if(cmdphase <= 0x3C)
			*ctlr->data = 0x41;		/* resume no data */
		else
			*ctlr->data = 0x46;		/* collect status */
		*ctlr->addr = Rtchi;
		*ctlr->data = 0;
		*ctlr->data = 0;
		*ctlr->data = 0;
		issue(ctlr, tp->transfer);
		break;

	case 0x85:					/* disconnect */
		if(tp->flags & Fidentifying){
			tp->cmdphase = 0x00;		/* complete retry */
			complete(ctlr);
		}
		else
			disconnect(ctlr);
		break;

	case 0x81:					/* reselect */
		*ctlr->addr = Rdata;
		data = *ctlr->data;			/* the identify message */
		if(srcid & Siv){			/* source ID valid */
			reselect(ctlr, srcid & 0x07);
			tp = ctlr->active;
			tp->cmdphase = 0x45;		/* reselected by target */
			start(tp);
			break;
		}
		scsipanic("reselect", ctlr, aux, status);
		break;

	default:
		if((tp->flags & Fidentifying) == 0 || identify(tp, status))
			scsipanic("unknown phase", ctlr, aux, status);
		break;
	}

	/*
	 * Debugging - save the last successful
	 * interrupt status.
	 */
	tp->lastphase[1] = tp->lastphase[0];
	tp->lastphase[0] = cmdphase;
	tp->laststat[1] = tp->laststat[0];
	tp->laststat[0] = status;

	unlock(&ctlr->lock);
}

static void
scsidmaintr(int ctlrno, Ureg *ur)
{
	Target *tp = target[ctlrno][CtlrID].active;
	ulong *status;
	uchar *p;

	status = &IRQADDR->status1;
	p = tp->dmaptr;
	if(tp->rw == ScsiIn){
		while(*status & 0x01)
			*p++ = *SCSIPDMA;
	}
	else {
		while(*status & 0x01)
			*SCSIPDMA = *p++;
	}
	tp->dmaptr = p;
	if(tp->dmaptr > (tp->dmaaddr+tp->dmalen))
		panic("scsidmaintr #%lux %lud %lud, #%lux\n",
			tp->dmaaddr, tp->dmatx, tp->dmalen, tp->dmaptr);
}

static void
ctlrreset(Target *ctlr)
{
	Target *tp;
	int i;
	uchar status;

	/*
	 * If there's a pending interrupt
	 * read the status register to
	 * clear the interrupt.
	 */
	if(*ctlr->addr & Intrpend){
		*ctlr->addr = Rstatus;
		status = *ctlr->data;
		USED(status);
	}

	/*
	 * Give the chip a soft reset. First set up the
	 * reset control bits in Rownid.
	 * While we're at it, work out the internal data transfer
	 * clock cycle in nanosecs:
	 *   1 cycle = (clock-divisor*1000)/2*ClkFreq
	 * We'll use cycle time later for sync negotiations, so
	 * put in 4ns increments.
	 * Reset will cause an interrupt, wait for it and
	 * clear it.
	 */
	*ctlr->addr = Rownid;
	if(ClkFreq >= 8 && ClkFreq <= 10){
		*ctlr->data = Fs10MHz|Eaf|CtlrID;
		ctlr->cycle = (2*1000)/(2*ClkFreq);
	}
	else if(ClkFreq >= 12 && ClkFreq <= 15){
		*ctlr->data = Fs15MHz|Eaf|CtlrID;
		ctlr->cycle = (3*1000)/(2*ClkFreq);
	}
	else {
		*ctlr->data = Fs16MHz|Eaf|CtlrID;
		ctlr->cycle = (4*1000)/(2*ClkFreq);
	}
	ctlr->cycle = (ctlr->cycle+3)/4;

	issue(ctlr, Creset);

	while((*ctlr->addr & Intrpend) == 0)
		delay(250);
	*ctlr->addr = Rstatus;
	status = *ctlr->data;
	USED(status);

	/*
	 * Freselect means enable the chip to respond to reselects.
	 * Timeout register value is
	 *   (timeout-period-in-ms * input-clk-freq-in-MHz)/80
	 */
	ctlr->flags = Freselect|Fidentified;

	*ctlr->addr = Rcontrol;
	*ctlr->data = BurstDma|Edi|Idi|Hsp;
	*ctlr->addr = Rtimeout;
	*ctlr->data = (200*ClkFreq)/80;			/* 200ms */

	*ctlr->addr = Rsrcid;
	if(ctlr->flags & Freselect)
		*ctlr->data = Er;
	else
		*ctlr->data = 0x00;

	/*
	 * Initialise all the targets for this controller.
	 */
	for(i = 0; i < NTarget; i++){
		if(i == CtlrID)
			continue;

		tp = ctlr->base+i;
		tp->lun = 0;
		tp->target = i;
		tp->ctlr = ctlr;

		if(ctlr->flags & Freselect){
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
 * Called at boot time, initialise the software and hardware.
 * This is where the fiction of multiple controllers breaks down.
 */
static Vector vector[2] = {
	{ IRQSCSI, 0, scsiintr, "scsi0", },
	{ IRQSCSIDMA, 0, scsidmaintr, "scsidma", },
};

void
resetscsi(void)
{
	Target *ctlr = &target[0][CtlrID];

	setvector(&vector[0]);
	setvector(&vector[1]);

	/*
	 * Take reset off the scsi bus.
	 */
	*IOCTLADDR &= ~SCSIENABLE;
	mdelay(25);
	*IOCTLADDR |= SCSIENABLE;
	mdelay(250);

	/*
	 * Set up the software controller struct,
	 * and call the hardware reset.
	 */
	ctlr->addr = SCSIADDR;
	ctlr->data = SCSIDATA;
	ctlr->base = target[0];

	ctlrreset(ctlr);
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
