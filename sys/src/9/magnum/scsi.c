#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

#define	DPRINT	if(debug)kprint


int	scsidebugs[8];
int	scsiownid = 7;

Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;
	void	*x;

	if(n > 512*1024)
		panic("scsialloc too big: %d", n);

	b = xalloc(sizeof(Scsibuf));
	x = xspanalloc(n, 64, 512*1024);

	b->phys = x;
	b->virt = x;

	return b;
}

/*
 *	NCR 53C94 commands
 */

enum
{
	Dma		= 0x80,
	Nop		= 0x00,
	Flush		= 0x01,
	Reset		= 0x02,
	Busreset	= 0x03,
	Select		= 0x41,
	Transfer	= 0x10,
	Cmdcomplete	= 0x11,
	Msgaccept	= 0x12
};

static QLock	scsilock;	/* access to device */
static Rendez	scsirendez;	/* sleep/wakeup for requesting process */
static Scsi *	curcmd;		/* currently executing command */
static int	debug;

#define	WB(a, e)	(((a) = (e)), wbflush())

void
resetscsi(void)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	int s;

	extern void (*pscsiintr)(void);

	if(pscsiintr == 0)
		pscsiintr = scsiintr;

	s = splhi();
	WB(dev->cmd, Nop);
	WB(dev->cmd, Reset);
	WB(dev->cmd, Nop);
	WB(dev->countlo, 0);
	WB(dev->counthi, 0);
	WB(dev->timeout, 146);
	WB(dev->syncperiod, 0);
	WB(dev->syncoffset, 0);
	WB(dev->config, 0x10|(scsiownid&7));
	WB(dev->config2, 0);
	WB(dev->config3, 0);
	WB(dev->cmd, Dma|Nop);
	WB(dma->block, 0);
	WB(dma->mode, (1<<31)|Clearerror);
	WB(dma->mode, 0);
	splx(s);
}

void
initscsi(void)
{
}

static int
scsidone(void *arg)
{
	USED(arg);
	return (curcmd == 0);
}

int
scsiexec(Scsi *p, int rflag)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	long n;

	debug = scsidebugs[p->target&7];
	DPRINT("scsi %d.%d cmd=0x%2.2ux ", p->target, p->lun, *(p->cmd.ptr));
	qlock(&scsilock);
	if(rflag && debug)
		memset(p->data.ptr, 0x5a, p->data.lim - p->data.ptr);
	if(waserror()){
		DPRINT("scsi %d.%d: error return\n", p->target, p->lun);
		qunlock(&scsilock);
		nexterror();
	}
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
	sleep(&scsirendez, scsidone, 0);
	if(rflag && p->data.ptr>p->data.base)
		dcflush(p->data.base, p->data.ptr-p->data.base);
	DPRINT(">\n");
	poperror();
	qunlock(&scsilock);
	debug = 0;
	return p->status;
}


void
scsibusreset(void)
{
	SCSIdev *dev = SCSI;
	int s;

	s = splhi();
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
/*		scsimoan("spurious", status, intr, dmastat); */
		return;
	}
	if(intr & 0x80){	/* SCSI bus reset */
		if(p)
			scsimoan("SCSI bus reset", status, intr, dmastat);
		WB(dev->cmd, Nop);
		goto Done;
	}
	if(p == 0){		/* ``can't happen'' */
		scsimoan("no command", status, intr, dmastat);
		WB(dev->cmd, Nop);
		goto Done;
	}
	switch(p->status>>8){
	case 0x00:		/* Select was issued */
		switch(intr){
		default:
			scsimoan("bad case", status, intr, dmastat);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x020:	/* arbitration complete, selection timed out */
			goto Done;
		case 0x218:	/* selection complete, no command phase */
			p->status = 0x1000;
			scsimoan("no cmd phase", status, intr, dmastat);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x318:	/* command phase ended prematurely */
			n = (p->cmd.lim - p->cmd.base) - (dev->fflags & 0x1f);
			p->status = (0x30+n)<<8;
			scsimoan("short cmd phase", status, intr, dmastat);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x418:	/* select sequence complete */
			p->status = 0x4100;
			if((status & 0x07) == p->rflag){
				DPRINT(" Dma|Transfer");
				WB(dev->cmd, Dma|Transfer);
				return;
			}else if((status & 0x07) != 3){
				scsimoan("weird phase after cmd",
					status, intr, dmastat);
				goto Done;
			}
			/* else fall through */
		}

	case 0x41:	/* data transfer, if any, is finished */
		p->status = 0x4600;
		p->data.ptr = p->data.lim - ((dev->counthi<<8)|dev->countlo);
		if((status & 0x07) != 3){
			scsimoan("weird phase after xfr",
				status, intr, dmastat);
			goto Done;
		}
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
	wakeup(&scsirendez);
}

/*void
scsidump(void)
{
	SCSIdev *dev = SCSI;
	DMAdev *dma = DMA1;
	int s;

	s = splhi();
	kprint("\nscsi:\n");
	kprint("	countlo=0x%2.2ux\n", dev->countlo);
	kprint("	counthi=0x%2.2ux\n", dev->counthi);
	kprint("	cmd    =0x%2.2ux\n", dev->cmd);
	kprint("	status =0x%2.2ux\n", dev->status);
	kprint("	intr   =0x%2.2ux\n", dev->intr);
	kprint("	step   =0x%2.2ux\n", dev->step);
	kprint("	fflags =0x%2.2ux\n", dev->countlo);
	kprint("	config =0x%2.2ux\n", dev->config);
	kprint("	config2=0x%2.2ux\n", dev->config2);
	kprint("	config3=0x%2.2ux\n", dev->config3);
	kprint("dma:\n");
	kprint("	laddr  =0x%8.8ux\n", dma->laddr);
	kprint("	mode   =0x%8.8ux\n", dma->mode);
	kprint("	block  =0x%4.4ux\n", dma->block);
	kprint("	caddr  =0x%8.8ux\n", dma->caddr);
	splx(s);
}*/
