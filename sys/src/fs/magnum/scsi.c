/*
 * it doesn't get more basic than this...
 */
#include "all.h"
#include "io.h"

typedef struct Ctrl Ctrl;

enum
{
	MaxCtrl		= 1,
	NTargetID	= 8,
	CtrlID		= 7,

	DmaAlign	= 64,			/* 0x00000040 */
	DmaBank		= 512*1024,		/* 0x00080000 */

	MaxDmaIO	= RBUFSIZE,
};

#define ALIGNED(x, y)	(((((x)|(y)) & (DmaAlign-1)) == 0) &&\
		 (((x)^((x)+(y))) < DmaBank))
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

struct Ctrl
{
	QLock;
	Rendez;
	SCSIdev	*dev;
	DMAdev	*dma;
	uchar	probe;
	uchar	unaligned;
	uchar	rw;
	void	*cmd;
	ulong	cbytes;
	void	*data;
	ulong	dbytes;
	void	*buf;
	int	phase;
	int	done;
	Drive	drive[NTargetID];
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
	if(status = scsiio(d, SCSIread, cmd, sizeof(cmd), sense, sizeof(sense)))
		return status;
	if((sense[2] & 0x0F) == 0x00 || (sense[2] & 0x0F) == 0x06)
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
	 * us work
	 * should check for direct-access device
	 */
	for(i = 0; i < NTargetID; i++){
		if(i == CtrlID){
			c->drive[i].status = Dself;
			continue;
		}
		c->drive[i].status = Dready;
	
		memset(cmd, 0, sizeof(cmd));		/* test unit ready */
		d.unit = i;
		if((status = scsiio(d, SCSIread, cmd, sizeof(cmd), 0, 0)) == 0)
			continue;
		if((status & 0xFF00) == 0x0200){
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

	if(d.ctrl < 0 || d.ctrl >= MaxCtrl || d.unit >= NTargetID || d.unit == CtrlID)
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
	Nop		= 0x00,			/* NCR 53C94 commands */
	Flush		= 0x01,
	Reset		= 0x02,
	Busreset	= 0x03,
	Select		= 0x41,
	Transfer	= 0x10,
	Cmdcomplete	= 0x11,
	Msgaccept	= 0x12,
	Dma		= 0x80,			/* flag */

	ScsiOut		= 0x00,
	ScsiIn		= 0x01,
};

#define	WB(a, e)	(((a) = (e)), wbflush())

static void
busreset(Ctrl *c)
{
	SCSIdev *dev = c->dev;

	WB(dev->cmd, Busreset);
	WB(dev->cmd, Nop);
}

static void
reset(Ctrl *c, int bus)
{
	SCSIdev *dev = c->dev;
	DMAdev *dma = c->dma;

	WB(dev->cmd, Nop);
	WB(dev->cmd, Reset);
	WB(dev->cmd, Nop);
	WB(dev->countlo, 0);
	WB(dev->counthi, 0);
	WB(dev->timeout, 0x99);
	WB(dev->clock, 5);
	WB(dev->config1, 0x10|(CtrlID));
	WB(dev->config2, 0x40);
	WB(dev->cmd, Dma|Nop);
	WB(dma->block, 0);
	WB(dma->mode, (1<<31)|Clearerror);
	WB(dma->mode, 0);
	if (bus)
		busreset(c);
}

void
scsiinit(int ctrlno)
{
	Ctrl *c;

	if(ctrlno >= MaxCtrl)
		panic("scsiintr: ctrlno = %d\n", ctrlno);
	c = &ctrl[ctrlno];
	c->dev = SCSI;
	c->dma = DMA1;
	reset(c, 0);

	/*
	 * allocate a temporary buffer for
	 * unaligned dma
	 */
	do
		c->buf = ialloc(MaxDmaIO, DmaAlign);
	while(ALIGNED((ulong)c->buf, MaxDmaIO) == 0);
}

static int
done(Ctrl *c)
{
	return c->done;
}
	
int
scsiio(Device d, int rw, uchar *cmd, int cbytes, void *data, int dbytes)
{
	Ctrl *c;
	SCSIdev *dev;
	DMAdev *dma;
	Drive *dr;
	int i, status;
	uchar *p;

	if(d.ctrl < 0 || d.ctrl >= MaxCtrl)
		panic("scsiio: d.ctrl = %d\n", d.ctrl);
	c = &ctrl[d.ctrl];
	dev = c->dev;
	dma = c->dma;
  retry:
	qlock(c);

	WB(dma->block, 0);			/* disable dma */
	WB(dma->mode, (1<<31)|Clearerror);	/* purge dma fifo */
	WB(dma->mode, 0);
	WB(dev->counthi, 0);
	WB(dev->countlo, 0);
	WB(dev->cmd, Dma|Nop);
	WB(dev->cmd, Flush);			/* clear scsi fifo */

	c->unaligned = 0;
	c->rw = (rw == SCSIread) ? ScsiIn: ScsiOut;
	c->cmd = cmd;
	c->cbytes = cbytes;
	c->data = data;
	c->dbytes = dbytes;

	for(p = cmd, i = cbytes; i; i--, p++)
		WB(dev->fifo, *p);
	WB(dev->destid, d.unit);
	WB(dev->counthi, dbytes>>8);
	WB(dev->countlo, dbytes);
	WB(dev->cmd, Dma|Nop);
	if(dbytes){
		/*
		 * dma controller only looks at bits 27:6 of the address.
		 *
		 * need to check here for validity of dma transfer,
		 * and do it to a temporary buffer if not ok.
		 */
		if(ALIGNED((ulong)data, dbytes) == 0){
			if(rw == SCSIwrite)
				memmove(c->buf, data, dbytes);
			data = c->buf;
			c->unaligned = 1;
		}
		WB(dma->mode, rw == SCSIread ? Tomemory|Enable: Enable);
		WB(dma->laddr, (ulong)data);
		WB(dma->block, HOWMANY(dbytes, DmaAlign));
	}
	c->phase = 0;
	c->done = 0;
	WB(dev->cmd, Select);

	dr = &c->drive[d.unit];
	if(dr->fflag == 0) {
		dofilter(&dr->rate);
		dofilter(&dr->work);
		dr->fflag = 1;
	}
	dr->work.count++;
	dr->rate.count += dbytes;

	sleep(c, done, c);
	status = (c->phase == 0x6000) ? 0: c->phase;
	if(status == 0 && rw == SCSIread && c->dbytes) {
		invaldcache(data, c->dbytes);
		if(c->unaligned)
			memmove(c->data, c->buf, c->dbytes);
	}
	qunlock(c);
	if((c->phase == 0x6002) && reqsense(d) == 0){
		print("scsiio: retry I/O on %D\n", d);
		goto retry;
	}
	return status;
}

void
scsiintr(int ctrlno)
{
	SCSIdev *dev;
	DMAdev *dma;
	Ctrl *c;
	int status, intr, mode, n;

	if(ctrlno >= MaxCtrl)
		panic("scsiintr: ctrlno = %d\n", ctrlno);
	c = &ctrl[ctrlno];
	dev = c->dev;
	dma = c->dma;
	status = dev->status;
	intr = (dev->step & 0x07)<<8;
	intr |= dev->intr;
	mode = dma->mode;

	if((status & 0x80) == 0)		/* spurious interrupt */
		return;
	if(intr & 0x80){			/* bus reset */
		WB(dev->cmd, Nop);
		goto buggery;
	}

	switch(c->phase>>8){

	case 0x00:				/* select issued */
		switch(intr){

		default:
			panic("scsiintr: select #%ux\n", intr);
			goto buggery;

		case 0x020:			/* timed out */
			WB(dev->cmd, Flush);
			c->phase = 0x0100;
			goto buggery;

		case 0x218:			/* complete, no cmd phase */
		case 0x318:			/* short cmd phase */
			print("scsiintr: select: intr #%ux status #%ux mode #%ux fflags #%ux\n",
				intr, status, mode, dev->fflags);
			reset(c, 1);
			c->phase = 0x6002;
			goto buggery;

		case 0x418:			/* select complete */
			c->phase = 0x4100;
			if((status & 0x07) == c->rw){
				WB(dev->cmd, Dma|Transfer);
				return;
			}
			else if((status & 0x07) != 3){
				panic("scsiintr: weird cmd phase\n");
				goto buggery;
			}
			/*FALLTHROUGH*/
		}

	case 0x41:				/* data transfer, if any, done */
		c->phase = 0x4600;
		c->dbytes -= ((dev->counthi<<8)|dev->countlo);
		if((status & 0x07) != 3){
			panic("scsiintr: weird data phase #%ux\n", status);
			goto buggery;
		}
		if((c->rw == ScsiIn) && (mode & Empty) == 0){
			n = 32 - (mode & 0xff);
			while(--n >= 0)
				WB(dma->fifo, 0xffff);
			WB(dma->mode, 1<<31);
		}
		WB(dma->block, 0);
		WB(dev->cmd, Cmdcomplete);
		return;

	case 0x46:				/* Cmdcomplete issued */
		c->phase = 0x6000|dev->fifo;
		WB(dev->cmd, Msgaccept);
		return;

	case 0x60:				/* Msgaccept was issued */
		goto buggery;
	}

buggery:
	c->done = 1;
	wakeup(c);
}
