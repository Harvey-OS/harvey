/*
 * Ultrastor [13]4f SCSI Host Adapter.
 * Originally written by Charles Forsyth, forsyth@minster.york.ac.uk.
 * Needs work:
 *	handle multiple controllers;
 *	set options from user-level;
 *	split out to handle adaptec too;
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct Ctlr Ctlr;
typedef struct Mailbox Mailbox;
typedef struct Scatter Scatter;

enum {
	Nctl		= 1,		/* only support one */
	Nmbox		= 8,		/* number of Mailboxes; up to 16? */
	Nscatter 	= 33,		/* s/g list limit fixed by Ultrastor controller */
	Timeout		= 10,		/* seconds to wait for things to complete */

	Port		= 0x330,	/* factory defaults: I/O port */
	CtlrID		= 7,		/*	adapter SCSI id */
	Irq		= 14,		/*	interrupt request level */
};

struct Scatter {
	ulong	start;	/* physical address; Intel byte order */
	ulong	len;	/* length in bytes; Intel byte order */
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
	uchar	sensedata[64];	/* enough for AdapterInquiry data too */
	int	busy;		/* interrupt pending */
	ulong	physaddr;	/* physical address of this Mailbox */
	int	p9status;	/* p9 status value */
	Mailbox	*next;		/* next Mailbox in free list */
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

#define	OUT(c,v) outb(ctlr->io+(c), (v))
#define	IN(c)	inb(ctlr->io+(c))

struct Ctlr {
	int	io;		/* io port */
	int	irq;		/* interrupt vector */
	int	dma;		/* DMA channel */
	int	ownid;		/* adapter's SCSI ID */
	Mailbox	mbox[Nmbox];
	Mailbox	*free;		/* next free Mailbox */
};

static	Ctlr	ultra[Nctl];

static void
resetadapter(int busreset)
{
	Ctlr *ctlr = &ultra[0];
	int i;

	if(IN(AdapterMask) & SoftResetEnable){
		OUT(AdapterIntr, SoftReset|busreset);
		for(i=50000; IN(AdapterIntr) & (SoftReset|busreset);)
			if(--i<0) {
				print("Ultrastor reset timed out\n");
				break;
			}
	}
}

static void
freebox(Ctlr *ctlr, Mailbox *mbox)
{
	mbox->next = ctlr->free;
	ctlr->free = mbox;
}

static void
ultra14fwait(Ctlr *ctlr, Mailbox *mbox)
{
	ulong start;
	int x;
	static void interrupt(Ureg*, void*);

	x = spllo();
	start = m->ticks;
	while(TK2SEC(m->ticks - start) < Timeout && mbox->busy)
		;
	if(TK2SEC(m->ticks - start) >= Timeout){
		print("ultra14fwait timed out\n");
		interrupt(0, ctlr);
	}
	splx(x);
}

static int
ultra14fexec(Scsi *p, int rflag)
{
	Ctlr *ctlr = &ultra[0];
	Mailbox *mbox;
	long n;
	uchar *cp;
	ulong s;

	if (p == 0 || ctlr->io == 0)
		return 0x6001;	/* device not available */

	/*
	 * get a free Mailbox and build an Ultrastor command
	 */
	if((mbox = ctlr->free) == 0)
		return 0x6001;
	ctlr->free = mbox->next;
	p->rflag = rflag;
	mbox->p9status = 0;
	mbox->op = OpTarget | CmdDirection | Usecache;	/* BUG? is it safe always to Usecache? */
	mbox->addr = p->target | (p->lun<<5);
	if(p->cmd.lim - p->cmd.ptr > sizeof(mbox->cmd))
		panic("ultra14fexec");
	mbox->cmdlen = p->cmd.lim - p->cmd.ptr;
	for(cp = mbox->cmd; p->cmd.ptr < p->cmd.lim;)
		*cp++ = *p->cmd.ptr++;
	n = p->data.lim - p->data.ptr;
	mbox->nscatter = 0;
	PL(mbox->datap, PADDR(p->data.ptr));
	PL(mbox->datalen, n);
	mbox->adapterstatus = 0;
	mbox->targetstatus = 0;

	/*
	 * send the command to the host adapter
	 */
	s = splhi();
	if(IN(AdapterIntr) & SignalAdapter)
		panic("ultrastor busy");
	mbox->busy = 1;
	outl(ctlr->io+OGMpointer, mbox->physaddr);
	OUT(AdapterIntr, SignalAdapter);

	/*
	 * wait for reply
	 */
	splx(s);
	ultra14fwait(ctlr, mbox);

	p->status = mbox->p9status;
	if(p->status == 0x6000)
		p->data.ptr = p->data.lim;	/* can only assume no residue */
	freebox(ctlr, mbox);
	return p->status;
}

static void
scsimoan(char *msg, Mailbox *mbox)
{
	/*int i;*/

	print("SCSI error: %s:", msg);
	print("id=%d cmd=%2.2x status=%2.2x adapter=%2.2x",
		mbox->addr&7, mbox->cmd[0], mbox->targetstatus, mbox->adapterstatus);
	/*print(" sense:");
	for(i=0; i<10; i++)
		print(" %2.2x", mbox->sensedata[i]);*/
	print("\n");
}

static void
interrupt(Ureg*, void *arg)
{
	Ctlr *ctlr;
	Mailbox *mbox;
	ulong pa;

	ctlr = arg;

	if((IN(HostIntr) & SignalHost) == 0)
		return;	/* no incoming mail */
	pa = inl(ctlr->io+ICMpointer);
	OUT(HostIntr, SignalHost);	/* release ICM slot */
	for(mbox = &ctlr->mbox[0]; mbox->physaddr != pa;)
		if(++mbox >= &ctlr->mbox[Nmbox]) {
			/* these sometimes happen in response to a scsi bus reset; ignore them */
			print("ultrastor: invalid ICM pointer #%lux\n", pa);
			return;
		}

	mbox->p9status = 0x1000 | mbox->adapterstatus;
	switch(mbox->adapterstatus){
	default:
		scsimoan("adapter", mbox);
		if(mbox->adapterstatus >= 0x92) /* ie, scsi command protocol error */
			resetadapter(BusReset);
		/* might adapter reset be required in other cases? */
		break;

	case 0x91:	/* selection timed out */
		mbox->p9status = 0x20;
		break;

	case 0xA3:
		scsimoan("SCSI bus reset error", mbox);
		break;

	case 0x00:
		mbox->p9status = 0x6000 | mbox->targetstatus;
		break;
	}
	mbox->busy = 0;
}

static int irq[4] = {
	15, 14, 11, 10,
};

int (*
ultra14freset(void))(Scsi*, int)
{
	Ctlr *ctlr = &ultra[0];
	Mailbox *mbox;
	int i, model;

	/*
	 * Check it is an Ultrastor [13]4f.
	 * Only one controller for now.
	 * For the moment assume the factory default settings.
	 */
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->io = Port;

	if(IN(ProductID) != 0x56)
		return 0;
	model = IN(ProductID+1);
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

	i = IN(Config1);
	ctlr->irq = irq[((i>>4)&03)];
	if(model == 14)
		ctlr->dma = 5 + ((i>>6)&03);
	ctlr->ownid = IN(Config2)&07;
	/*print("Ultrastor %dF id %d io 0x%x irq %d dma %d\n",
		model, ctlr->ownid, ctlr->io, ctlr->irq, ctlr->dma);*/

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

	resetadapter(0);
	ctlr->free = 0;
	for(i=0; i<Nmbox; i++){
		mbox = &ctlr->mbox[i];
		memset(mbox, 0, sizeof(*mbox));
		mbox->physaddr = PADDR(mbox);
		PL(mbox->sensep, 0);	/* DON'T collect sense data automatically: messes up scuzz? */
		mbox->senselen = 0;
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
	PL(mbox->datap, PADDR(mbox->sensedata));
	PL(mbox->datalen, 38);
	while(IN(AdapterIntr) & SignalAdapter)
		;
	outl(ctlr->io+OGMpointer, mbox->physaddr);
	OUT(AdapterIntr, SignalAdapter);
	for(i=100000; (IN(HostIntr)&SignalHost)==0;)
		if(--i < 0){
			print("No response to UltraStor Inquiry\n");
			ctlr->io = 0;
			return 0;
		}
	OUT(HostIntr, SignalHost);

	OUT(HostMask, ICMIntEnable|SIntEnable);

	return ultra14fexec;
}
