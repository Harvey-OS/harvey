#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

uchar	scsiownid = 7;
int	scsidebugs[8];

static int	dmatype;

Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;
	ulong pa, va;
	int i;

	b = xalloc(sizeof(Scsibuf));

	/*
	 * Allocate space in host memory for the io buffer.
	 * NOTE: dma mustn't cross a 16Mb boundary.
	 */
	i = (n+(BY2PG-1))/BY2PG;
	pa = PADDR(xspanalloc(i*BY2PG, BY2PG, 16*1024*1024));
	va = kmapdma(pa, i*BY2PG);

	b->virt = (void*)va; 
	b->phys = (void*)pa;

	return b;
}

enum {
	ResetIntDis	= 0x40,		/* config register */
	Penable		= 0x10,

	Dma		= 0x80,		/* NCR 53C90 commands */
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

struct {
	DMAdev *dma;
	SCSIdev *scsi;
} ioaddr;

static void
scsibusreset(void)
{
	SCSIdev *dev = ioaddr.scsi;
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

void
resetscsi(void)
{
	SCSIdev *dev;
	DMAdev *dma;
	KMap *k;
	uchar intr;

	k = kmappas(SBUSSPACE, DMA, PTENOCACHE|PTEIO);
	ioaddr.dma = (DMAdev*)VA(k);
	k = kmappas(SBUSSPACE, SCSI, PTENOCACHE|PTEIO);
	ioaddr.scsi = (SCSIdev*)VA(k);

	dev = ioaddr.scsi;
	dma = ioaddr.dma;

	dma->csr = Dma_Reset;
	delay(1);
	dma->csr = Int_en;
	dma->count = 0;

	dmatype = (dma->csr>>28) & 0xF;

	dev->conf2 = 0;
	dev->conf2 = 0x0A;
	delay(1);
	if((dev->conf2 & 0x0F) == 0x0A){
		dev->conf3 = 0;
		dev->conf3 = 0x05;
		delay(1);
		if(dev->conf3 == 0x05)
			print("scsi type ESP236\n");
		else
			print("scsi type ESP100A\n");
	}
	else
		print("scsi type NCR53C90\n");

	dev->cmd = Reset;
	dev->cmd = Dma|Nop;			/* 2 are necessary for some chips */
	dev->cmd = Dma|Nop;

	dev->clkconf = 4;			/* BUG: 20MHz */
	dev->timeout = 160;			/* BUG: magic */
	dev->syncperiod = 0;
	dev->syncoffset = 0;
	dev->config = Penable|(scsiownid&7);

	intr = dev->intr;
	USED(intr);

	/* Enable DMA */
	dma->csr = Int_en|En_dma;
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
	SCSIdev *dev = ioaddr.scsi;
	DMAdev *dma = ioaddr.dma;
	long n;
	ulong pa;

retry: 
	qlock(&scsilock);

	p->rflag = rflag;
	p->status = 0;

	dma->csr = Dma_Flush|Int_en;

	/* This is a hack, using the RMMU to find the PA of the buffer */
	pa = getflushprobe(((ulong)p->data.ptr&0xFFFFF000)|0<<8);
	if (pa == 0) 
		panic("scsiexec");
	dma->addr = (pa & 0xFFFFFF00) << 4;

	dev->countlo = 0;
	dev->countmi = 0;
	dev->cmd = Dma|Nop;
	dev->cmd = Flush;			/* clear scsi fifo */

	while (p->cmd.ptr < p->cmd.lim)
		dev->fifo = *(p->cmd.ptr)++;

	dev->destid = p->target&7;
	n = p->data.lim - p->data.ptr;
	dev->countlo = n;
	dev->countmi = n>>8;
	dev->cmd = Dma|Nop;

	dma->csr = Int_en;

	curcmd = p;
	dev->cmd = Select;

	while(waserror())
		;
	sleep(&scsirendez, scsidone, 0);
	poperror();
	qunlock(&scsilock);

	return p->status;
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
	SCSIdev *dev = ioaddr.scsi;
	DMAdev *dma = ioaddr.dma;
	Scsi *p = curcmd;
	int status, step, intr, n;
	ulong m, csr;

	csr = dma->csr;

	status = dev->status;
	step = dev->step;
	intr = dev->intr;

	intr |= ((step&7)<<8);

	if(p == 0 || (intr & 0x80)) {	/* SCSI bus reset */
print("SCSI bus reset\n");
		dev->cmd = Nop;
		goto Done;
	}

	switch(p->status>>8){
	case 0x00:		/* Select was issued */
		switch(intr){
		default:
			scsimoan("bad case", status, intr, csr);
			print("cmd = #%2.2ux\n", dev->cmd);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x020:	/* arbitration complete, selection timed out */
			goto Done;
		case 0x218:	/* selection complete, no command phase */
			p->status = 0x1000;
			scsimoan("no cmd phase", status, intr, csr);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x318:	/* command phase ended prematurely */
			n = (p->cmd.lim - p->cmd.base) - (dev->fflags & 0x1f);
			p->status = (0x30+n)<<8;
			scsimoan("short cmd phase", status, intr, csr);
			resetscsi();
			scsibusreset();
			goto Done;
		case 0x418:	/* select sequence complete */
			p->status = 0x4100;
			if((status & 0x07) == p->rflag){
				dma->csr = p->rflag ? En_dma|Write|Int_en : En_dma|Int_en;
				dev->cmd = Dma|Transfer;
				return;
			}else if((status & 0x07) != 3){
				scsimoan("weird phase after cmd",
					status, intr, csr);
				goto Done;
			}
			/* else fall through */
		}

	case 0x41:	/* data transfer, if any, is finished */
		p->status = 0x4600;
		p->data.ptr = p->data.lim - ((dev->countmi<<8)|dev->countlo);
		if((status & 0x07) != 3){
			scsimoan("weird phase after xfr",
				status, intr, csr);
			goto Done;
		}

		if(p->rflag){
			switch(dmatype){

			case 8:
				dma->csr = Drain|Int_en;
				/*FALLTHROUGH*/

			case 9:
				while(dma->csr & Pack_cnt)
					;
				break;

			default:
				break;
			}
		}

		dev->cmd = Cmdcomplete;
		return;

	case 0x46:	/* Cmdcomplete was issued */
		p->status = 0x6000|dev->fifo;
		m = dev->fifo;
		USED(m);
		dev->cmd = Msgaccept;
		return;

	case 0x60:	/* Msgaccept was issued */
		goto Done;
	}
Done:
	curcmd = 0;
	wakeup(&scsirendez);
}

#ifdef notdef
void
scsidump(void)
{
	SCSIdev *dev = ioaddr.scsi;
	DMAdev *dma = ioaddr.dma;

	print("\nscsi:\n");
	print("	countlo=0x%2.2ux\n", dev->countlo);
	print("	countmi=0x%2.2ux\n", dev->countmi);
	print("	cmd    =0x%2.2ux\n", dev->cmd);
	print("	status =0x%2.2ux\n", dev->status);
	print("	intr   =0x%2.2ux\n", dev->intr);
	print("	step   =0x%2.2ux\n", dev->step);
	print("	fflags =0x%2.2ux\n", dev->fflags);
	print("	config =0x%2.2ux\n", dev->config);
	print("dma:\n");
	print("	csr    =0x%8.8ux\n", dma->csr);
	print("	addr   =0x%8.8ux\n", dma->addr);
	print("	count  =0x%4.4ux\n", dma->count);
	print("	diag   =0x%8.8ux\n", dma->diag);
}
#endif
