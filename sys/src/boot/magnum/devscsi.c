#include	"all.h"

#define	DPRINT	if(scsidebug)print

#define	HMEG	(512*1024)

static Scsi	staticcmd;
#define	DATASIZE	512
static uchar	*datablk;

static int	ownid = 7;

void *
dmaalloc(ulong n)
{
	uchar *x;
	long m;

	m = n % 64;
	if(m != 0)
		n += 64-m;
	if(n > HMEG)
		panic("dmaalloc");
	x = ialloc(0, 0);
	m = (ulong)x % 64;
	if(m != 0)
		x = ialloc(64-m, 0);
	m = HMEG - ((ulong)x % HMEG);
	if(n > m)
		ialloc(m, 0);
	return ialloc(n, 0);
}


Scsi *
scsicmd(int dev, int cmdbyte, long size)
{
	Scsi *cmd = &staticcmd;

	if(size > DATASIZE)
		panic("scsicmd %d", size);
	cmd->target = dev>>3;
	cmd->lun = dev&7;
	cmd->cmd.base = cmd->cmdblk;
	cmd->cmd.ptr = cmd->cmd.base;
	memset(cmd->cmdblk, 0, sizeof cmd->cmdblk);
	cmd->cmdblk[0] = cmdbyte;
	switch(cmdbyte>>5){
	case 0:
		cmd->cmd.lim = &cmd->cmdblk[6]; break;
	case 1:
		cmd->cmd.lim = &cmd->cmdblk[10]; break;
	default:
		cmd->cmd.lim = &cmd->cmdblk[12]; break;
	}
	switch(cmdbyte){
	case 0x00:	/* test unit ready */
		break;
	case 0x03:	/* read sense data */
		cmd->cmdblk[4] = size;
		break;
	case 0x25:	/* read capacity */
		break;
	}
	cmd->data.base = datablk;
	cmd->data.lim = cmd->data.base + size;
	cmd->data.ptr = cmd->data.base;
	cmd->save = cmd->data.base;
	return cmd;
}

int
scsiready(int dev)
{
	Scsi *cmd = scsicmd(dev, 0x00, 0);
	int status;

	status = scsiexec(cmd, 0);
	if ((status&0xff00) != 0x6000)
		return -1;
	return status&0xff;
}

int
scsisense(int dev, void *p)
{
	Scsi *cmd = scsicmd(dev, 0x03, 18);
	int status;

	status = scsiexec(cmd, 1);
	memmove(p, cmd->data.base, 18);
	if ((status&0xff00) != 0x6000)
		return -1;
	return status&0xff;
}

int
scsicap(int dev, void *p)
{
	Scsi *cmd = scsicmd(dev, 0x25, 8);
	int status;

	status = scsiexec(cmd, 1);
	memmove(p, cmd->data.base, 8);
	if ((status&0xff00) != 0x6000)
		return -1;
	return status&0xff;
}

/*
 *	NCR 53C94 commands
 */

enum {
	Dma = 0x80,	/* flag */
	Nop  = 0x00,
	Flush = 0x01,
	Reset = 0x02,
	Busreset = 0x03,
	Select = 0x41,
	Transfer = 0x10,
	Cmdcomplete = 0x11,
	Msgaccept = 0x12
};

int	scsidebug;

static Scsi *	curcmd;		/* currently executing command */

#define	WB(a, e)	(((a) = (e)), wbflush())

static int
scsidone(void)
{
	return curcmd == 0;
}

int
scsiexec(Scsi *p, int rflag)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	long n;

	DPRINT("scsi %d.%d cmd=0x%2.2ux ", p->target, p->lun, *(p->cmd.ptr));
	p->rflag = rflag;
	p->status = 0;
	WB(dma->block, 0);		/* disable dma */
	WB(dma->mode, (1<<31)|Clearerror); /* purge dma fifo */
	WB(dma->mode, 0);
	WB(dev->counthi, 0);
	WB(dev->countlo, 0);
	WB(dev->cmd, Dma|Nop);
	WB(dev->cmd, Flush);		/* clear scsi fifo */
	while (p->cmd.ptr < p->cmd.lim)
		WB(dev->fifo, *(p->cmd.ptr)++);
	WB(dev->destid, p->target&7);
	n = p->data.lim - p->data.ptr;
	WB(dev->counthi, n>>8);
	WB(dev->countlo, n);
	WB(dev->cmd, Dma|Nop);
	if(n){
		WB(dma->mode, rflag ? Tomemory|Enable : Enable);
		WB(dma->laddr, (ulong)p->data.ptr);
		WB(dma->block, (n+63)/64);
	}
	curcmd = p;
	WB(dev->cmd, Select);
	DPRINT("S<");
	while(!scsidone())
		/* noop */;
	if(rflag && p->data.ptr>p->data.base)
		dcflush(p->data.base, p->data.ptr-p->data.base);
	DPRINT(">\n");
	return p->status;
}

void
scsireset(void)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	int s;

	if(datablk == 0){
		datablk = dmaalloc(DATASIZE);
	}
	s = splhi();
	WB(dev->cmd, Nop);
	WB(dev->cmd, Reset);
	WB(dev->cmd, Nop);
	WB(dev->countlo, 0);
	WB(dev->counthi, 0);
	WB(dev->timeout, 146);
	WB(dev->syncperiod, 0);
	WB(dev->syncoffset, 0);
	WB(dev->config, 0x10|(ownid&7));
	WB(dev->config2, 0);
	WB(dev->config3, 0);
	WB(dev->cmd, Dma|Nop);
	WB(dma->block, 0);
	WB(dma->mode, (1<<31)|Clearerror);
	WB(dma->mode, 0);
	WB(dev->cmd, Nop);
	WB(dev->cmd, Busreset);
	WB(dev->cmd, Nop);
	splx(s);
}

static void
scsimoan(char *msg, int status, int intr, int dmastat)
{
	print("scsiintr: %s:", msg);
	print(" status=%2.2ux step/intr=%3.3ux", status, intr);
	print(" dma=%8.8ux cmd status=%4.4ux\n",
		dmastat, curcmd ? curcmd->status : 0xffff);
}

void
scsiintr(void)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	Scsi *p = curcmd;
	int status, step, intr, dmastat, n;
	ulong m;

	status = dev->status;
	step = dev->step;
	intr = ((step&7)<<8) | dev->intr;
	dmastat = dma->mode;

	DPRINT("\n\t[ status=%2.2ux step/intr=%3.3ux", status, intr);
	DPRINT(" dma=%8.8ux cmd status=%4.4ux ]",
		dmastat, p ? p->status : 0xffff);

	if(!(status & 0x80)){	/* spurious interrupt */
		scsimoan("spurious", status, intr, dmastat);	/* stop your whining */
		return;
	}
	if(p == 0 || (intr & 0x80)){	/* SCSI bus reset */
		WB(dev->cmd, Nop);
		goto Done;
	}
	switch(p->status>>8){
	case 0x00:		/* Select was issued */
		switch(intr){
		default:
			print("bad case\n");
			goto Done;
		case 0x020:	/* arbitration complete, selection timed out */
			goto Done;
		case 0x218:	/* selection complete, no command phase */
			p->status = 0x1000;
			goto Done;
		case 0x318:	/* command phase ended prematurely */
			n = (p->cmd.lim - p->cmd.base) - (dev->fflags & 0x1f);
			p->status = (0x30+n)<<8;
			goto Done;
		case 0x418:	/* select sequence complete */
			p->status = 0x4100;
			if((status & 0x07) == p->rflag){
				DPRINT(" Dma|Transfer");
				WB(dev->cmd, Dma|Transfer);
				return;
			}else if((status & 0x07) != 3)
				goto Done;
			/* else fall through */
		}

	case 0x41:	/* data transfer, if any, is finished */
		p->status = 0x4600;
		p->data.ptr = p->data.lim - ((dev->counthi<<8)|dev->countlo);
		if((status & 0x07) != 3)
			goto Done;
		if(p->rflag && !(dmastat & Empty)){
			n = 32 - (dmastat & 0xff);
			DPRINT(" pad %d", n);
			while(--n >= 0)
				WB(dma->fifo, 0xffff);
			WB(dma->mode, 1<<31);
		}
		WB(dma->block, 0);
		DPRINT(" Cmdcomplete");
		WB(dev->cmd, Cmdcomplete);
		return;

	case 0x46:	/* Cmdcomplete was issued */
		p->status = 0x6000|dev->fifo;
		m = dev->fifo;
		DPRINT(" end status=0x%2.2ux msg=0x%2.2ux",
			p->status, m);
		DPRINT(" Msgaccept");
		WB(dev->cmd, Msgaccept);
		return;

	case 0x60:	/* Msgaccept was issued */
		goto Done;
	}
Done:
	DPRINT(" Done");
	curcmd = 0;
}
