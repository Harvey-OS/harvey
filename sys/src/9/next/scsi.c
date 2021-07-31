#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

#define	DPRINT	if(debug)print

int	scsiownid = 7;
int	scsidebugs[8];

typedef struct SCSIdma	SCSIdma;
typedef struct SCSIdev  SCSIdev;

void crankfifo(int);
void Xdelay(int);

enum
{
	DmaFifoOvflw = 16,	/* Amount of extra buffer for crankfifo  overflow */

	Dma 		= 0x80,
	Nop  		= 0x00,
	Flush 		= 0x01,
	Reset 		= 0x02,
	Select 		= 0x41,
	Transfer 	= 0x10,
	Cmdcomplete	= 0x11,
	Msgaccept 	= 0x12
};

static QLock	scsilock;	/* access to device */
static Rendez	scsirendez;	/* sleep/wakeup for requesting process */
static Scsi *	curcmd;		/* currently executing command */
static int	debug;

/* NCR 53C90 device registers */
struct SCSIdev
{
	uchar	countlo;
	uchar	counthi;
	uchar	fifo;
	uchar	cmd;
	union {	
		struct {		/* Registers when READ */
			uchar status;
			uchar intr;
			uchar step;
			uchar fflags;
			uchar config;
			uchar Reserved1;
			uchar Reserved2;
		};
		struct {		/* Registers when WRITTEN */
			uchar destid;
			uchar timeout;
			uchar syncperiod;
			uchar syncoffset;
			uchar XXX;
			uchar clkconf;
			uchar test;	
		};                                                                      	
	};
}; 

/*
 * allocate a scsi buf of any length
 * must be called at ialloc time and never freed
 */
Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;
	ulong l, p;

	b = xalloc(sizeof(Scsibuf));
	p = (ulong)xspanalloc(n + 2*DmaFifoOvflw, BY2PG, 0);
	b->virt = (uchar *)kmappa(p);
	for(l = BY2PG; l < n + 2*DmaFifoOvflw + BY2PG; l += BY2PG)
		kmappa(p+l);
	b->phys = (uchar*)p;
	return b;
}

void
initscsi(void)
{
}

void
resetscsi(void)
{
	SCSIdev *dev = (SCSIdev *)SCSI;

	INITCTL(0x80);
	SETCTL(Int_mask);
	CLRCTL(Cpu_dma);

	dev->cmd = Reset;
	dev->cmd = Nop;
	dev->countlo = 0;
	dev->counthi = 0;
	dev->timeout = 146;
	dev->syncperiod = 0;
	dev->syncoffset = 0;
	dev->config = 0x10|(scsiownid&7);	/* Parity on | Bus id */
	dev->cmd = Dma|Nop;
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
	SCSIdev *dev = (SCSIdev *)SCSI;
	SCSIdma *dma = (SCSIdma *)SCSIDMA;
	ulong n, up;

	debug = scsidebugs[p->target&7];
	DPRINT("scsi %d.%d cmd=0x%2.2ux ", p->target, p->lun, *(p->cmd.ptr));
	qlock(&scsilock);
	qlock(&dmalock);

	p->rflag = rflag;
	p->status = 0;

	CLRCTL(Cpu_dma);

	dma->chainbase = 0;
	dma->chainlimit = 0;
	dma->csr = Dinit | Dcreset | Dclrcint;

	dev->counthi = 0;
	dev->countlo = 0;
	dev->cmd = Dma|Nop;
	dev->cmd = Flush;		/* clear scsi fifo */

	while(p->cmd.ptr < p->cmd.lim)
		dev->fifo = *(p->cmd.ptr)++;

	dev->destid = p->target&7;
	n = p->data.lim - p->data.ptr;
	dev->counthi = n>>8;
	dev->countlo = n;
	dev->cmd = Dma|Nop;
	if(n) {
		up = (ulong)p->b->phys;
		dma->base  = up;
		dma->limit = up + ((n + 0xf)&~0xf);
		if(!rflag)
			dma->limit += DmaFifoOvflw; 	/* NeXT brain damage */
	}

	curcmd = p;
	dev->cmd = Select;

	while(waserror())
		;
	DPRINT("S<");
	sleep(&scsirendez, scsidone, 0);
	poperror();
	qunlock(&dmalock);
	qunlock(&scsilock);
	debug = 0;
	return p->status;
}

void
scsiintr(int type)
{
	SCSIdev *dev = (SCSIdev *)SCSI;
	SCSIdma *dma = (SCSIdma *)SCSIDMA;
	Scsi *p = curcmd;
	int status, step, intr, n;
	ulong m, csr;

	USED(type);
	csr = dma->csr;

	CLRCTL(Cpu_dma);		/* Access the registers */

	/* Read cause */
	status = dev->status;
	step = dev->step;
	intr = dev->intr;

	intr |= ((step&7)<<8);

	DPRINT("\n\t[ status=%2.2ux step/intr=%3.3ux", status, intr);
	DPRINT(" dma=%8.8ux cmd status=%4.4ux ]", csr, p ? p->status : 0xffff);

	if(p == 0 || (intr & 0x80)) {	/* SCSI bus reset */
		dev->cmd = Nop;
		goto Done;
	}

	switch(p->status>>8){
	case 0x00:		/* Select was issued */
		switch(intr){
		default:
			print("devscsi: bad case\n");
			goto Done;
		case 0x020:		/* arbitration complete, selection timed out */
			goto Done;
		case 0x218:		/* selection complete, no command phase */
			p->status = 0x1000;
			goto Done;
		case 0x318:		/* command phase ended prematurely */
			n = (p->cmd.lim - p->cmd.base) - (dev->fflags & 0x1f);
			p->status = (0x30+n)<<8;
			goto Done;
		case 0x418:		/* select sequence complete */
			p->status = 0x4100;
			if((status & 0x07) == p->rflag){
				DPRINT(" Dma|Transfer ");
				dma->csr = Dcreset|Dinit;
				dma->csr = 0;
				if(p->rflag) {
					SETCTL(Dmadir);
					dma->csr = Dseten|Dsetread|Dinit;
				}
				else {
					CLRCTL(Dmadir);
					dma->csr = Dseten|Dinit;
				}
				dev->cmd = Dma|Transfer;
				SETCTL(Cpu_dma);
				return;
			}
			else if((status & 0x07) != 3)
				goto Done;
			/* else fall through */
		}

	case 0x41:	/* data transfer, if any, is finished */
		p->status = 0x4600;
		p->data.ptr = p->data.lim - ((dev->counthi<<8)|dev->countlo);
		if((status & 0x07) != 3)
			goto Done;
		crankfifo(dma->limit - dma->base);
		dma->csr = Dcreset;
		DPRINT(" Cmdcomplete");
		dev->cmd = Cmdcomplete;
		return;

	case 0x46:	/* Cmdcomplete was issued */
		p->status = 0x6000|dev->fifo;
		m = dev->fifo;
		DPRINT(" end status=0x%2.2ux msg=0x%2.2ux", p->status, m);
		DPRINT(" Msgaccept");
		dev->cmd = Msgaccept;
		return;

	case 0x60:	/* Msgaccept was issued */
		goto Done;
	}
Done:
	DPRINT(" Done");
	curcmd = 0;
	wakeup(&scsirendez);
}

void
crankfifo(int n)
{
	if(n <= 0)
		return;

	n = n>>2;
	SETCTL(Cpu_dma);
	while(n--) {
		CLRCTL(Fifofl);
		SETCTL(Fifofl);
		CLRCTL(Fifofl);
	}
	CLRCTL(Cpu_dma);
}

void
Xdelay(int time)
{
	time *= 100;
	while(time--)
		;	
}
