#include "all.h"
#include "io.h"
#include "mem.h"

typedef struct Ctrl Ctrl;
typedef struct Unit Unit;

static int dbg;
#define DBG	if(dbg)print

enum
{
	MaxCtrl		= 1,
	NTargetID	= 8,
	CtrlID		= 7,

	ScsiOut		= 0x00,
	ScsiIn		= 0x01,

	DmaAlign	= 4,			/* 0x00000004 */
	DmaBank		= 16*1024*1024,		/* 0x01000000 */

	MaxDmaIO	= RBUFSIZE,
};

#define ALIGNED(x, y)	(((((x)|(y)) & (DmaAlign-1)) == 0) &&\
		 (((x)^((x)+(y))) < DmaBank))

struct Unit
{
	QLock;
	Rendez;
	uchar	unaligned;
	uchar	rw;
	uchar	*cmd;
	ulong	cbytes;
	uchar	*data;
	ulong	dbytes;
	ulong	dio;
	void	*buf;
	int	phase;
	int	done;
	uchar	sel;
	Drive;
};

struct Ctrl
{
	QLock;
	SCSIdev	*dev;
	DMAdev	*dma;
	uchar	probe;
	Unit	*run;
	Unit	drive[NTargetID];
	int	ssi;
	int	sss;
};
static Ctrl ctrl[MaxCtrl];

static int
reqsense(Device d)
{
	uchar cmd[6], sense[255];
	int status;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x03;
	cmd[4] = sizeof(sense);
	memset(sense, 0, sizeof(sense));

	if(status = scsiio(d, SCSIread, cmd, sizeof(cmd), sense, sizeof(sense)))
		return status;

	/*
	 * Unit attention. We can handle that.
	 */
	if((sense[2] & 0x0F) == 0x00 || (sense[2] & 0x0F) == 0x06)
		return 0;

	/*
	 * Recovered error. Why bother telling me.
	 */
	if((sense[2] & 0x0F) == 0x01)
		return 0;

	print("scsi reqsense %D: key #%2.2ux code #%2.2ux #%2.2ux\n",
		d, sense[2], sense[12], sense[13]);

	return 0x6002;
}

static void
probe(Ctrl *c, Device d)
{
	uchar cmd[6];
	int i, status;

	/*
	 * we look for drives, we look for drives to make
	 * us work. should check for direct-access device
	 */
	for(i = 0; i < NTargetID; i++) {
		if(i == CtrlID) {
			c->drive[i].status = Dself;
			continue;
		}
		c->drive[i].status = Dready;
	
		memset(cmd, 0, sizeof(cmd));		/* test unit ready */
		d.unit = i;
		status = scsiio(d, SCSIread, cmd, sizeof(cmd), 0, 0);
		if(status == 0)
			continue;
		if((status & 0xFF00) == 0x0200) {
			c->drive[i].status = Dunavail;
			continue;
		}
		if(status == 0x6002 && reqsense(d) == 0)
			continue;
		c->drive[i].status = Dnready;
	}
}

Drive*
scsidrive(Device d)
{
	Ctrl *c;

	if(d.ctrl >= MaxCtrl || d.unit >= NTargetID || d.unit == CtrlID)
		return 0;
	c = &ctrl[d.ctrl];
	if(c->probe == 0){
		probe(c, d);
		c->probe = 1;
	}
	c->drive[d.unit].dev = d;
	return &c->drive[d.unit];
}

enum
{
	ResetIntDis	= 0x40,		/* config register */
	Penable		= 0x10,
	
	Nop		= 0x00,		/* NCR 53C90 commands */
	Flush		= 0x01,
	Reset		= 0x02,
	Busreset	= 0x03,
	Transfer	= 0x10,
	Cmdcomplete	= 0x11,
	Msgaccept	= 0x12,
	Select		= 0x41,
	SelectATN	= 0x42,
	Dma		= 0x80,
	Eresel		= 0x44,
	Dresel		= 0x45,
	ScsiRead	= 0x28,		/* Extended read */
	ScsiWrite	= 0x2a,		/* Extended write */

	SP_dataout	= 0,		/* SCSI bus phases */
	SP_datain	= 1,
	SP_cmd		= 2,
	SP_status	= 3,
	SP_res1		= 4,
	SP_res2		= 5,
	SP_msgout	= 6,
	SP_msgin	= 7,

	Identify	= 0x80,
	Allowdiscon	= 0x40,

	Msgdisco	= 0x04,
	Msgsavedp	= 0x02,
};

static void
busreset(Ctrl *c)
{
	SCSIdev *dev = c->dev;
	int s;
	uchar intr;

	s = splhi();
	dev->config |= ResetIntDis;
	dev->cmd = Busreset;
	dev->config &= ~ResetIntDis;
	intr = dev->intr;
	USED(intr);
	splx(s);
}

static void
reset(Ctrl *c, int bus)
{
	SCSIdev *dev;
	DMAdev *dma;
	KMap *k;
	uchar intr;

	USED(bus);
	k = kmappa(DMA, PTENOCACHE|PTEIO);
	c->dma = (DMAdev*)k->va;
	k = kmappa(SCSI, PTENOCACHE|PTEIO);
	c->dev = (SCSIdev*)k->va;

	dev = c->dev;
	dma = c->dma;

	dma->csr = Dma_Reset;
	delay(1);
	dma->csr = Int_en;
	dma->count = 0;

	dev->cmd = Reset;
	dev->cmd = Dma|Nop;			/* 2 are necessary for some chips */
	dev->cmd = Dma|Nop;

	dev->clkconf = 4;			/* BUG: 20MHz */
	dev->timeout = 160;			/* BUG: magic */
	dev->syncperiod = 0;
	dev->syncoffset = 0;
	dev->config = Penable|CtrlID;

	intr = dev->intr;
	USED(intr);

	putenab(getenab()|ENABDMA); /**/
}

void
scsiinit(int ctlrno)
{
	int i;
	Ctrl *c;
	ulong pa;

	print("scsi init %d\n", ctlrno);
	if(ctlrno >= MaxCtrl)
		panic("scsiinit: ctlrno %d\n", ctlrno);

	c = &ctrl[ctlrno];
	reset(c, 0);

	/*
	 * allocate a temporary buffer for
	 * unaligned dma
	 */
	for(i = 0; i < NTargetID; i++) {
		do
			pa = (ulong)xialloc(MaxDmaIO, DmaAlign, PTEMAINMEM);
		while(ALIGNED(pa, MaxDmaIO) == 0);
		c->drive[i].buf = (void*)pa;
	}
}

static int
done(Unit *u)
{
	return u->done;
}

void
initdp(SCSIdev *dev)
{
	dev->countlo = 0;
	dev->countmi = 0;
	dev->cmd = Dma|Nop;
}

void
issue(Ctrl *c, Unit *u)
{
	int i;
	SCSIdev *dev;
	DMAdev *dma;

	qlock(c);

	dev = c->dev;
	dma = c->dma;

	c->run = u;				/* This unit is running */
	dev->cmd = Flush;
	dma->csr = Dma_Flush|Int_en;
	initdp(dev);

	if(u->sel == SelectATN)
		dev->fifo = Identify|Allowdiscon;

	for(i = 0; i < u->cbytes; i++)
		dev->fifo = u->cmd[i];

	dev->destid = u-c->drive;
	dev->cmd = u->sel;
	u->sel = 0;
}

int
scsiio(Device d, int rw, uchar *cmd, int cbytes, void *data, int dbytes)
{
	Ctrl *c;
	Unit *u;
	int status;

	if(d.ctrl >= MaxCtrl)
		panic("scsiio: d.ctrl = %d\n", d.ctrl);

	c = &ctrl[d.ctrl];
	u = &c->drive[d.unit];

retry:
	qlock(u);

	u->unaligned = 0;
	u->rw = (rw == SCSIread) ? ScsiIn: ScsiOut;
	u->cmd = cmd;
	u->cbytes = cbytes;
	u->data = data;
	u->dbytes = dbytes;
	u->dio = 0;
	u->phase = 0;
	u->done = 0;
	
	switch(cmd[0]) {
/* This will be fixed sometime, various people report the intr state
   machine has problems dealing with some drives.
	case ScsiRead:
	case ScsiWrite:
		u->sel = SelectATN;
		break;
*/
	default:
		u->sel = Select;
	}

	if(ALIGNED((ulong)data, dbytes) == 0){
		if(u->rw == ScsiOut)
			memmove(u->buf, u->data, u->dbytes);

		u->data = u->buf;
		u->unaligned = 1;
	}
	if(u->rw == ScsiOut && u->dbytes)
		wbackcache((ulong)u->data, u->dbytes);

	issue(c, u);

	if(u->fflag == 0) {
		dofilter(&u->rate);
		dofilter(&u->work);
		u->fflag = 1;
	}
	u->work.count++;
	u->rate.count += dbytes;

	sleep(u, done, u);

	status = u->phase;
	if(status == 0x6000)
		status = 0;

	if(status == 0 && u->rw == ScsiIn && u->dio){
		invalcache((ulong)u->data, u->dio);
		if(u->unaligned)
			memmove(data, u->buf, u->dio);
	}

	qunlock(u);
	qunlock(c);

	if((u->phase == 0x6002) && reqsense(d)){
		print("scsiio: retry I/O on %D\n", d);
		goto retry;
	}
	return status;
}

static void
setdma(Ctrl *c, Unit *u)
{
	SCSIdev *dev;
	DMAdev *dma;

	dev = c->dev;
	dma = c->dma;

	DBG("setdma: %d\n", u->dbytes);

	dev->countlo = u->dbytes;
	dev->countmi = u->dbytes>>8;
	dev->cmd = Dma|Nop;

	dma->addr = (ulong)u->data;
	dma->csr = Int_en;

	if(u->rw == ScsiIn)
		dma->csr = En_dma|Write|Int_en;
	else
		dma->csr = En_dma|Int_en;
}

static void
scsimoan(char *msg, int status, int intr, int dmacsr)
{
	print("scsiintr: %s:", msg);
	print(" status=%2.2ux step/intr=%3.3ux", status, intr);
	print(" dma=%8.8ux\n", dmacsr);
}

static uchar
getmsg(SCSIdev *dev)
{
	uchar msg;

	/* Wait for the message to turn up */
	while(!(dev->fflags&0x1f))
		;

	msg = dev->fifo;
	DBG("msg #%2.2ux\n", msg);
	return msg;
}

static void
drain(Unit *u, DMAdev *dma, int csr)
{
	USED(u);

	if(1/*u->rw == ScsiIn*/){
		switch((csr>>28) & 0xF){
		case 8:
			dma->csr = Drain|Int_en;
			/*FALLTHROUGH*/

		case 9:
			while(dma->csr & Pack_cnt)
				;
			break;
		}
	}
	dma->csr = Dma_Flush|Int_en;
}

void
scsiintr(int ctrlno)
{
	Ctrl *c;
	Unit *u;
	SCSIdev *dev;
	DMAdev *dma;
	ulong csr;
	int status, step, intr;
	int phase, id, target, tfer;
uchar msg;

	if(ctrlno >= MaxCtrl)
		panic("scsiintr: ctrlno = %d\n", ctrlno);

	c = &ctrl[ctrlno];
	u = c->run;
	dev = c->dev;
	dma = c->dma;

	csr = dma->csr;

	status = dev->status;
	step = dev->step;
	intr = dev->intr;

	intr |= ((step&7)<<8);

	if((status & 0x80) == 0)		/* spurious interrupt */
		return;

	if(intr & 0x80){			/* bus reset */
		dev->cmd = Nop;
		goto buggery;
	}

	if(dbg) {	
		print("cmd = #%2.2ux phase=#%2.2ux ", dev->cmd, u->phase>>8);
		scsimoan("debug", status, intr, csr);
	}

	phase = status & 0x07;

	switch(u->phase>>8){
	case 0x00:				/* select issued */
		switch(intr){
		default:
			scsimoan("bad case", status, intr, csr);
			print("cmd = #%2.2ux\n", dev->cmd);
			reset(c, 0);
			busreset(c);
			goto buggery;

		case 0x020:			/* timed out */
			dev->cmd = Flush;
			u->phase = 0x0100;
			goto buggery;

		case 0x218:			/* complete, no cmd phase */
		case 0x318:			/* short cmd phase */
			scsimoan("cmd phase", status, intr, csr);
			reset(c, 0);
			u->phase = 0x6002;
			goto buggery;

		case 0x418:			/* select complete */
			u->phase = 0x4100;
			switch(phase) {
			case SP_datain:
			case SP_dataout:
				setdma(c, c->run);
				dev->cmd = Dma|Transfer;
				return;

			case SP_msgin:
				dev->cmd = Transfer;
				msg = getmsg(dev);
				if(msg != Msgdisco) {
					print("message #%2.2ux\n", msg);
					scsimoan("not disconnect", status, intr, csr);
					delay(20000);
					reset(c, 0);
					busreset(c);
					goto buggery;
				}
				dev->cmd = Msgaccept;
				return;

			case SP_status:
				u->phase = 0x4600;
				dev->cmd = Cmdcomplete;
				return;
			}
		}
		scsimoan("weird phase after cmd", status, intr, csr);
		goto buggery;

	case 0x41:		/* data transfer, done or diconnect */
		switch(intr) {
		case 0x420:			/* Disconnect */
			dev->cmd = Eresel;
			return;

		case 0x40c:			/* Reconnect */
			id = getmsg(dev);
			getmsg(dev);
			dev->cmd = Msgaccept;
			id &= ~(1<<CtrlID);
			for(target = 0; target < 8; target++)
				if(id & (1<<target))
					break;

			dev->destid = target;
			c->run = &c->drive[target];
			return;

		case 0x408:			/* xfer/reconnect complete */
			return;

		case 0x410:			/* Bus service required */
			switch(phase) {
			default:
				scsimoan("bad service phase", status, intr, csr);
				break;
			case SP_status:
				break;
			case SP_datain:
			case SP_dataout:
				setdma(c, c->run);
				dev->cmd = Dma|Transfer;
				return;
			case SP_msgin:
				dev->cmd = Transfer;
				id = getmsg(dev);
				dev->cmd = Msgaccept;
				switch(id) {
				case Msgsavedp:
					drain(u, dma, csr);
					tfer = dev->countlo;
					tfer |= dev->countmi<<8;
					u->data += (u->dbytes-tfer);
					u->dio += u->dbytes-tfer;
					u->dbytes = tfer;
					initdp(dev);
					return;
				case Msgdisco:
					dev->cmd = Eresel;
					return;
				}
				print("msg #%2.2ux", id);
				scsimoan("bad msg", status, intr, csr);
				return;
			}
			/* FALLTHROUGH */

		default:
			u->phase = 0x4600;
			tfer = dev->countlo;
			tfer |= dev->countmi<<8;
			initdp(dev);
			u->dio += (u->dbytes-tfer);
			u->dbytes -= tfer;
			drain(u, dma, csr);
		}
		if(phase != SP_status){
			scsimoan("weird phase after xfr", status, intr, csr);
			goto buggery;
		}
		dev->cmd = Cmdcomplete;
		return;

	case 0x46:				/* Cmdcomplete issued */
		switch(phase) {
		default:
			scsimoan("bad status phase", status, intr, csr);
			break;
		case SP_msgin:
			if((dev->fflags&0x1f) != 2)
				scsimoan("werird fifo", status, intr, csr);
			u->phase = 0x6000|getmsg(dev);
			if(getmsg(dev) != 0x00)
				scsimoan("expected command complete", status, intr, csr);
			dev->cmd = Msgaccept;
			break;
		}
		return;

	case 0x60:				/* Msgaccept was issued */
		if(intr != 0x420)
			scsimoan("bad termination state", status, intr, csr);
		u->done = 1;
		wakeup(u);
		return;
	}
buggery:
	u->done = 1;
	wakeup(u);
}
