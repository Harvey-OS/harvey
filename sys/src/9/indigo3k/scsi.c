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
int	scsiownid = 0x08|5; /* enable advanced features */

Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;

	b = xalloc(sizeof(Scsibuf));
	b->virt = b->phys = pxalloc(n);
	return b;
}

enum
{
	Eox=	1<<31		/* end of transmission */
};

typedef struct Sdesc	Sdesc;

struct Sdesc {
	ulong	count;
	ulong	addr;		/* eox / addr:28 */
	ulong	next;
};

#define	NMAP	64		/* max no. of pages in dma transfer */

typedef struct Scsictl {
	uchar	fill0[0x88];	/* 0000 */
	ulong	bc;		/* 0088 byte count */
	ulong	cbdp;		/* 008c current buffer descriptor pointer */
	ulong	nbdp;		/* 0090 next buffer descriptor pointer */
	ulong	ctrl;		/* 0094 control register */
	ulong	ptr;		/* 0098 pointer register */
	ulong	fifo;		/* 009c fifo data register */
	uchar	fill1[0x82];	/* 00a0 */
	uchar	asr;		/* 0122 wd33c93 address/status register */
	uchar	fill2[0x03];	/* 0123 */
	uchar	data;		/* 0126 wd33c93 data register */
} Scsictl;

#define	DEV	IO(Scsictl, HPC_0_ID_ADDR)

#define	PUT(a,d)	(dev->asr=(a), dev->data=(d))
#define	GET(a)		(dev->asr=(a), dev->data)

enum Scsi_ctl {
	Sreset=	0x01,		/* reset SCSI bus */
	Sflush=	0x02,		/* flush dma */
	Sread=	0x10,		/* transfer data to memory */
	Sdma=	0x80		/* start dma */
};

enum SBIC_regs {
	Own_id=0x00, Control=0x01, CDB=0x03, Target_LUN=0x0f,
	Cmd_phase=0x10, Tc_hi=0x12,
	Dest_id=0x15, Src_id=0x16, SCSI_Status=0x17,
	Cmd=0x18, Data=0x19,
};

enum Commands {
	Reset = 0x00,
	Assert_ATN = 0x02,
	Negate_ACK = 0x03,
	Select_with_ATN = 0x06,
	Select_with_ATN_and_Xfr = 0x08,
	Select_and_Xfr = 0x09,
	Transfer_Info = 0x20,
	SBT = 0x80,		/* modifier for single-byte transfer */
};

enum Aux_status {
	INT=0x80, LCI=0x40, BSY=0x20, CIP=0x10,
#undef PE
	PE=0x02, DBR=0x01,
};

static QLock	scsilock;
static Rendez	scsirendez;
static Sdesc *	dmamap;
static long	debug, scsirflag, scsibusy;
static ulong	datap, datalim;
static Sdesc *	curmap;

static void
nop(void)
{}

void
resetscsi(void)
{
	int i;

	dmamap = pxspanalloc(NMAP*sizeof(Sdesc), BY2PG, 128*1024*1024);
	for(i=0; i<NMAP; i++){
		dmamap[i].count = 0;
		dmamap[i].addr = Eox;
		dmamap[i].next = PADDR(&dmamap[i+1]);
	}
	dmamap[NMAP-1].next = 0;
}

void
initscsi(void)
{
	Scsictl *dev = DEV;
	int s;

	s = splhi();
	*IO(uchar, LIO_0_MASK) &= ~LIO_SCSI;
	splx(s);
	dev->ctrl = Sreset;
	Xdelay(25);
	dev->ctrl = 0;
	s = splhi();
	*IO(uchar, LIO_0_MASK) |= LIO_SCSI;
	splx(s);
	while(*IO(uchar, LIO_0_ISR) & LIO_SCSI)
		nop();
	scsiownid &= 0x0f; /* possibly advanced features */
	scsiownid |= 0x80; /* 16MHz */
	PUT(Own_id, scsiownid);
	PUT(Cmd, Reset);
}

static int
scsidone(void *arg)
{
	USED(arg);
	return (scsibusy == 0);
}

int
scsiexec(Scsi *p, int rflag)
{
	Scsictl *dev = DEV;
	Sdesc *dp;
	long k, n, addr;

	debug = scsidebugs[p->target&7];
	DPRINT("scsi %d.%d %2.2ux ", p->target, p->lun, *(p->cmd.ptr));
	qlock(&scsilock);
	if(waserror()){
		qunlock(&scsilock);
		DPRINT(" error return\n");
		nexterror();
	}
	scsirflag = rflag;
	p->rflag = rflag;
	datap = PADDR(p->data.base);
	datalim = PADDR(p->data.lim);
	n = datalim - datap;
	addr = datap;
	curmap = dp = dmamap;
	do{
		k = BY2PG - addr%BY2PG;
		if(k > n)
			k = n;
		dp->count = k;
		dp->addr = addr;
		n -= k;
		addr += k;
		++dp;
	}while(n > 0);
	--dp;
	dp->addr |= Eox;
	if ((scsiownid & 0x08) && rflag)
		PUT(Dest_id, 0x40|p->target);
	else
		PUT(Dest_id, p->target);
	PUT(Target_LUN, p->lun);
	n = p->data.lim - p->data.base;
	PUT(Tc_hi, n>>16);
	dev->data = n>>8;
	dev->data = n;
	if (scsiownid & 0x08) {
		n = p->cmd.lim - p->cmd.ptr;
		DPRINT("len=%d ", n);
		PUT(Own_id, n);
	}
	PUT(CDB, *(p->cmd.ptr)++);
	while (p->cmd.ptr < p->cmd.lim)
		dev->data = *(p->cmd.ptr)++;
	scsibusy = 1;
	if(curmap->count){
		dev->nbdp = PADDR(curmap);
		dev->ctrl = scsirflag ? (Sdma|Sread) : Sdma;
	}
	PUT(Cmd, Select_and_Xfr);
	/*PUT(Cmd, Select_with_ATN_and_Xfr);*/
	DPRINT("S<");
	sleep(&scsirendez, scsidone, 0);
	DPRINT(">\n");
	p->data.ptr = KADDR1(datap);
	p->status = GET(Target_LUN);
	p->status |= dev->data<<8;
	poperror();
	qunlock(&scsilock);
	debug = 0;
	return p->status;
}

static void
scsirun(void)
{
	wakeup(&scsirendez);
	scsibusy = 0;
}

static void
scsireset0(void)
{
	Scsictl *dev = DEV;

/*	PUT(Control, 0x29);	/* burst DMA, halt on parity error */
	PUT(Control, 0x28);	/* burst DMA, allow parity errors */
	PUT(Control+1, (100*16+79)/80);	/* 100 msec at 16MHz */
	PUT(Src_id, 0x80);	/* enable reselection */
	dev->ctrl = 0;		/* abort any dma in progress */
	scsirun();
	/*qunlock(&scsilock);*/
}

static void
flushdma(int n)
{
	Scsictl *dev = DEV;
	Sdesc *dp;
	uchar *p;
	int fcnt = 0, xcnt;
	ulong fdata;

	SET(fdata);
	if(n < 0){
		n = GET(Tc_hi);
		n = (n<<8)|dev->data;
		n = (n<<8)|dev->data;
	}
	datap = datalim - n;
	dp = curmap;
	xcnt = datap - (dp->addr&0x0fffffff);
	if(scsirflag && xcnt){
		/* fflags:4, :5, fcnt:7, :2, chptr:6, :2, mptr:4, :2 */
		fcnt = (dev->ptr>>16)&0x7f;
		if(0 < fcnt && fcnt < 4){
			/* ``software workaround for DDM bug required; a flush
			 * in this case when the HPC is ready to flush some
			 * other fifos to memory can result in a 2nd HPC access
			 * following the partial word store of the SCSI fifo
			 * bytes, and this triggers a DDM bug with the
			 * rev B DDM chips.''
			 */
			fdata = dev->fifo;
			dev->ctrl = 0;	/* stop dma (no flush) */
		}else{
			fcnt = 512;	/* pessimistic */
			dev->ctrl |= Sflush;
			while((dev->ctrl & Sdma) && --fcnt > 0) ;
			if(fcnt == 0)
				panic("scsi flushdma");
			fcnt = 0;
		}
	}else
		dev->ctrl = 0;

	if(xcnt >= dp->count){
		xcnt -= dp->count;
		dp++;
	}
	if(xcnt > 0){
		dp += xcnt / BY2PG;
		xcnt %= BY2PG;
		dp->count -= xcnt;
		dp->addr += xcnt;
	}
	curmap = dp;

	if(!scsirflag)
		return;
	p = KADDR1(datap);
	switch(fcnt){
	case 3:
		p[-3] = fdata>>24;
		p[-2] = fdata>>16;
		p[-1] = fdata>>8;
		break;
	case 2:
		p[-2] = fdata>>24;
		p[-1] = fdata>>16;
		break;
	case 1:
		p[-1] = fdata>>24;
		break;
	}
}

void
scsiintr(void)
{
	Scsictl *dev = DEV;
	int status, phase;
	long n;

	status = GET(SCSI_Status);
	DPRINT("I%2.2x ", status);
	switch(status){
	case 0x00:			/* reset by command or power-up */
	case 0x01:			/* reset by command or power-up */
		scsireset0();
		break;
	case 0x21:			/* Save Data Pointers message received */
		flushdma(-1);
		PUT(Cmd_phase, 0x41);
		PUT(Cmd, Select_and_Xfr);
		break;
	case 0x16:			/* select-and-transfer completed */
		flushdma(-1);
	case 0x42:			/* timeout during select */
		scsirun();
		break;
	case 0x4b:			/* unexpected status phase */
		flushdma(-1);
		datalim = datap;
		dev->asr = Target_LUN;
		kprint("lun/status 0x%ux\n", dev->data);
		phase = dev->data;
		kprint("phase 0x%ux\n", phase);
		PUT(Tc_hi, 0);
		dev->data = 0;
		dev->data = 0;
		switch(phase){
		case 0x50:
		case 0x60:
			break;
		default:
			phase = 0x46;
			break;
		}
		PUT(Cmd_phase, phase);
		PUT(Cmd, Select_and_Xfr);
		break;
	default:
		kprint("scsintr 0x%ux\n", status);
		dev->asr = Target_LUN;
		kprint("lun/status 0x%ux\n", dev->data);
		kprint("phase 0x%ux\n", dev->data);
		switch (status&0xf0) {
		case 0x00:
		case 0x10:
		case 0x20:
		case 0x40:
		case 0x80:
			if(status & 0x08){
				n = GET(Tc_hi);
				n = (n<<8)|dev->data;
				n = (n<<8)|dev->data;
				kprint("count 0x%.6ux", n);
				flushdma(n);
			}
			scsirun();
			break;
		default:
			panic("scsi status 0x%2.2ux", status);
		}
		kprint("resetting...");
		PUT(Own_id, scsiownid);
		PUT(Cmd, Reset);
		break;
	}
}
