/*
 * Ultrastor [13]4f SCSI Host Adapter.
 * Originally written by Charles Forsyth, forsyth@minster.york.ac.uk.
 */
#include "all.h"
#include "io.h"
#include "mem.h"

typedef struct Ctlr Ctlr;
typedef struct Mailbox Mailbox;

enum {
	Nmbox		= 8,		/* number of Mailboxes; up to 16? */

	Port		= 0x330,	/* factory default: I/O port */
};

/*
 * Ultrastor mailbox pointers point to one of these structures
 */
struct Mailbox {
	uchar	op;		/* Adapter operation */
	uchar	addr;		/* lun, chan, scsi ID */
	uchar	datap[4];	/* transfer data phys. pointer */
	uchar	datalen[4];	/* transfer data length */
	uchar	cmdlink[4];	/* command link phys. pointer */
	uchar	linkid;		/* SCSI command link ID */
	uchar	nscatter;	/* scatter/gather length (8 bytes per entry) */
	uchar	senselen;	/* length of sense data */
	uchar	cmdlen;		/* length of CDB */
	uchar	cmd[12];	/* SCSI command */
	uchar	adapterstatus;
	uchar	targetstatus;
	uchar	sensep[4];	/* sense data phys. pointer */

	/* the remainder is used by software, not the device */
	Rendez	iodone;
	int	busy;		/* interrupt pending */
	ulong	physaddr;	/* physical address of this Mailbox */
	int	status;		/* status value */
	Mailbox	*next;		/* next Mailbox in free list */

	void	*buf;		/* for copy to > 16Mb */
	void	*data;
	int	copy;
};

/* pack pointer as Intel order bytes (can simply assign) */
#define	PL(buf,p)	(*(ulong*)(buf)=(ulong)(p))

enum {
					/* bits in Mailbox.op[0] */
					/* 3 bit opcode */
	OpAdapter	= 0x01,		/* adapter command, not SCSI command */
	OpTarget	= 0x02,		/* SCSI command for device */
	OpReset		= 0x04,		/* device reset */
					/* 2 bit transfer direction */
	CmdDirection	= 0<<3,		/* transfer direction determined by SCSI command */
	DataIn		= 1<<3,		/* SCSI Data In */
	DataOut		= 2<<3,		/* SCSI Data Out */
	NoTransfer	= 3<<3,
	Nodiscon=	1<<5,		/* disable disconnect */
	Usecache=	1<<6,		/* use adapter cache (if available) */
	Scattered=	1<<7,		/* transfer pointer refers to S/G list */

					/* OpAdapter commands in Mailbox.cmd[0] */
	AdapterInquiry	= 0x01,
	AdapterSelftest	= 0x02,
	AdapterRead	= 0x03,		/* read adapter's buffer */
	AdapterWrite	= 0x04,		/* write to adapter's buffer */

					/* IO port offsets and bits */
	AdapterMask	= 0x00, 	/* local doorbell mask register */
	  OGMIntEnable	= 0x01,
	  SoftResetEnable = 0x40,
	  AdapterReady	= 0xe1,		/* adapter ready for work when all these set */
	AdapterIntr	= 0x01,		/* adapter interrupt/status; set by host, reset by adapter */
	  SignalAdapter	= 0x01,		/* tell adapter to read OGMpointer */
	  BusReset	= 0x20,		/* SCSI Bus Reset */
	  SoftReset	= 0x40,		/* Adapter Soft Reset */
	HostMask	= 0x02, 	/* host doorbell mask register */
	  ICMIntEnable	= 0x01,		/* incoming mail enable */
	  SIntEnable	= 0x80, 	/* enable interrupt from HostIntr reg */
	HostIntr	= 0x03, 	/* host doorbell interrupt/status; set by adapter, reset by host */
	  IntPending	= 0x80,		/* interrupt pending for host */
	  SignalHost	= 0x01,		/* Incoming Mail Interrupt */
	ProductID	= 0x04,		/* two byte product ID */
	Config1		= 0x06,
	Config2		= 0x07,
	OGMpointer	= 0x08,		/* pointer to Outgoing Mail */
	ICMpointer	= 0x0c,		/* pointer to Incoming Mail */
};

#define	OUT(c,v) 	outb(ctlr->io+(c), (v))
#define	IN(c)		inb(ctlr->io+(c))

struct Ctlr {
	Lock;
	QLock;
	int	io;		/* io port */
	int	irq;		/* interrupt vector */
	int	dma;		/* DMA channel */
	int	ownid;		/* adapter's SCSI ID */
	int	ctlrno;
	Mailbox	mbox[Nmbox];
	Mailbox	*free;		/* next free Mailbox */
	Rendez	mboxes;		/* wait for free mbox */
	QLock	mboxq;		/* mutex for mboxes */
	Rendez	ogm;		/* wait for free OGM slot with Ctlr qlocked */
	uchar	sensedata[64];
};

static Ctlr *ultra14f[MaxScsi];

static void
resetadapter(Ctlr *ctlr, int busreset)
{
	int i;

	if(IN(AdapterMask) & SoftResetEnable){
		OUT(AdapterIntr, SoftReset|busreset);
		for(i=50000; IN(AdapterIntr) & (SoftReset|busreset);)
			if(--i<0) {
				print("ultra14f:%d reset timed out\n", ctlr->ctlrno);
				break;
			}
	}
}

static void
freebox(Ctlr *ctlr, Mailbox *mbox)
{
	lock(ctlr);
	mbox->next = ctlr->free;
	if(mbox->next == 0)
		wakeup(&ctlr->mboxes);
	ctlr->free = mbox;
	unlock(ctlr);
}

static int
boxavail(void *a)
{
	return ((Ctlr*)a)->free != 0;
}

static Mailbox*
allocbox(Ctlr *ctlr)
{
	Mailbox *mbox;

	lock(ctlr);
	for(;;) {
		mbox = ctlr->free;
		if(mbox != 0)
			break;
	
		unlock(ctlr);
		qlock(&ctlr->mboxq);
		sleep(&ctlr->mboxes, boxavail, ctlr);
		qunlock(&ctlr->mboxq);
		lock(ctlr);
	}
	ctlr->free = mbox->next;
	unlock(ctlr);

	return mbox;
}

static int
isready(void *a)
{
	Ctlr *ctlr = (Ctlr*)a;

	return (IN(AdapterIntr) & SignalAdapter) == 0;
}

static int
isdone(void *a)
{
	return ((Mailbox*)a)->busy == 0;
}

static int
ultra14fio(Device d, int rw, uchar *cmd, int clen, void *data, int dlen)
{
	Ctlr *ctlr;
	Mailbox *mbox;
	int status;

	ctlr = ultra14f[d.ctrl];

	if(ctlr->io == 0)
		return 0x0200;

	if(ctlr->ownid == d.unit)
		return 0x0300;

	/*
	 * get a free Mailbox and build an Ultrastor command
	 */
	mbox = allocbox(ctlr);

	mbox->status = 0;
	mbox->op = OpTarget | CmdDirection | Usecache;
	mbox->addr = d.unit | (cmd[1] & 0xE0);

	mbox->cmdlen = clen;
	memmove(mbox->cmd, cmd, clen);

	mbox->copy = 0;
	if(dlen){
		mbox->data = data;
		if(ctlr->dma && DMAOK(data, RBUFSIZE) == 0){
			if(rw == SCSIwrite)
				memmove(mbox->buf, data, dlen);
			data = mbox->buf;
			mbox->copy = 1;
		}
	}
	PL(mbox->datalen, dlen);

	mbox->nscatter = 0;
	PL(mbox->datap, PADDR(data));

	mbox->senselen = 0;
	mbox->adapterstatus = 0;
	mbox->targetstatus = 0;

	/*
	 * send the command to the host adapter
	 */
	for(lock(ctlr); IN(AdapterIntr) & SignalAdapter; lock(ctlr)){
		unlock(ctlr);
		qlock(ctlr);
		sleep(&ctlr->ogm, isready, ctlr);
		qunlock(ctlr);
	}
	mbox->busy = 1;
	outl(ctlr->io+OGMpointer, mbox->physaddr);
	OUT(AdapterIntr, SignalAdapter);
	unlock(ctlr);

	sleep(&mbox->iodone, isdone, mbox);
	status = mbox->status;
	if(status == 0x6000 && rw == SCSIread && mbox->copy)
		memmove(mbox->data, mbox->buf, dlen);
	freebox(ctlr, mbox);
	return status;
}

static void
scsimoan(Ctlr *ctlr, char *msg, Mailbox *mbox)
{
	print("ultra14f:%d: error: %s:", ctlr->ctlrno, msg);
	print("id=%d cmd=%2.2x status=%2.2x adapter=%2.2x",
		mbox->addr, mbox->cmd[0], mbox->targetstatus, mbox->adapterstatus);
	print("\n");
}

static void
interrupt(Ureg *ur, void *arg)
{
	Ctlr *ctlr;
	Mailbox *mbox;
	ulong pa;

	USED(ur);
	ctlr = arg;

	if(ctlr->ogm.p && (IN(AdapterIntr) & SignalAdapter) == 0)
		wakeup(&ctlr->ogm);
	if((IN(HostIntr) & SignalHost) == 0)
		return;	/* no incoming mail */
	pa = inl(ctlr->io+ICMpointer);
	OUT(HostIntr, SignalHost);	/* release ICM slot */
	for(mbox = &ctlr->mbox[0]; mbox->physaddr != pa;)
		if(++mbox >= &ctlr->mbox[Nmbox]) {
			/* these sometimes happen in response to a scsi bus reset; ignore them */
			print("ultra14f:%d: invalid ICM pointer #%lux\n", ctlr->ctlrno, pa);
			return;
		}

	mbox->status = 0x1000 | mbox->adapterstatus;
	switch(mbox->adapterstatus){
	default:
		scsimoan(ctlr, "adapter", mbox);
		if(mbox->adapterstatus >= 0x92) /* ie, scsi command protocol error */
			resetadapter(ctlr, BusReset);
		/* might adapter reset be required in other cases? */
		break;

	case 0x91:	/* selection timed out */
		mbox->status = 0x0200;
		break;

	case 0xA3:
		scsimoan(ctlr, "SCSI bus reset error", mbox);
		break;

	case 0x00:
		mbox->status = 0x6000 | mbox->targetstatus;
		break;
	}
	mbox->busy = 0;
	wakeup(&mbox->iodone);
}

static int irq[4] = {
	15, 14, 11, 10,
};

int (*
ultra14freset(int ctlrno, ISAConf *isa))(Device, int, uchar*, int, void*, int)
{
	Ctlr *ctlr;
	Mailbox *mbox;
	int i, model;

	/*
	 * See if we have any configuration info
	 * and check it is an Ultrastor [13]4f.
	 */
	if(isa->port == 0)
		isa->port = Port;

	if(inb(isa->port+ProductID) != 0x56)
		return 0;
	model = inb(isa->port+ProductID+1);
	switch(model){

	case 0x40:
		model = 14;
		break;

	case 0x41:
		model = 34;
		break;

	default:
		return 0;
	}

	ctlr = ultra14f[ctlrno] = ialloc(sizeof(Ctlr), 0);
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->io = isa->port;

	i = IN(Config1);
	if(isa->irq == 0)
		isa->irq = irq[((i>>4)&03)];
	ctlr->irq = isa->irq;

	if(model == 14)
		ctlr->dma = 5 + ((i>>6)&03);

	i &= 0x03;
	if(i){
		isa->mem = 0xC0000 + i*0x4000;
		isa->size = 0x4000;
	}
	else{
		isa->mem = 0;
		isa->size = 0;
	}
		
	ctlr->ownid = IN(Config2)&07;
	/*print("Ultrastor %dF id %d io 0x%x irq %d dma %d\n",
		model, ctlr->ownid, ctlr->io, ctlr->irq, ctlr->dma);/**/

	/*
	 * set the DMA controller to cascade mode for bus master
	 */
	switch(ctlr->dma){
	case 5:
		outb(0xd6, 0xc1); outb(0xd4, 1); break;
	case 6:
		outb(0xd6, 0xc2); outb(0xd4, 2); break;
	case 7:
		outb(0xd6, 0xc3); outb(0xd4, 3); break;
	}

	resetadapter(ctlr, 0);
	ctlr->free = 0;
	for(i=0; i<Nmbox; i++){
		mbox = &ctlr->mbox[i];
		memset(mbox, 0, sizeof(*mbox));
		mbox->physaddr = PADDR(mbox);

		if(ctlr->dma){
			mbox->buf = ialloc(RBUFSIZE, 0);
			if(DMAOK(mbox->buf, RBUFSIZE) == 0)
				panic("ultra14f:%d: no memory < 16Mb\n", ctlr->ctlrno);
		}

		mbox->next = ctlr->free;
		ctlr->free = mbox;
	}

	setvec(Int0vec+ctlr->irq, interrupt, ctlr);

	/*
	 * issue AdapterInquiry command to check it's alive
	 * -- could check enquiry data
	 */
	OUT(HostMask, 0);	/* mask interrupts */
	mbox = &ctlr->mbox[0];
	mbox->op = OpAdapter | CmdDirection;
	mbox->addr = ctlr->ownid;
	mbox->cmdlen = 1;
	mbox->cmd[0] = AdapterInquiry;
	PL(mbox->datap, PADDR(ctlr->sensedata));
	PL(mbox->datalen, 38);
	while(IN(AdapterIntr) & SignalAdapter)
		;
	outl(ctlr->io+OGMpointer, mbox->physaddr);
	OUT(AdapterIntr, SignalAdapter);
	for(i=100000; (IN(HostIntr)&SignalHost)==0;)
		if(--i < 0){
			print("ultra14f:%d: no response to AdapterInquiry\n", ctlr->ctlrno);
			ctlr->io = 0;
			return 0;
		}
	OUT(HostIntr, SignalHost);

	OUT(HostMask, ICMIntEnable|SIntEnable);

	return ultra14fio;
}
