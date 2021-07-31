/*
 * Ultrastor [13]4f SCSI Host Adapter.
 * Originally written by Charles Forsyth, forsyth@minster.york.ac.uk.
 * Needs work:
 *	handle multiple controllers;
 *	set options from user-level;
 *	split out to handle adaptec too;
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#include "ureg.h"

typedef struct Ctlr Ctlr;
typedef struct Mailbox Mailbox;
typedef struct Scatter Scatter;

enum {
	Nctl		= 1,		/* only support one */
	Nmbox		= 8,		/* number of Mailboxes; up to 16? */
	Nscatter 	= 33,		/* s/g list limit fixed by Ultrastor controller */

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
	Scatter	sglist[Nscatter];
	Rendez	iodone;
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
	Lock;
	QLock;
	int	io;		/* io port */
	int	irq;		/* interrupt vector */
	int	dma;		/* DMA channel */
	int	ownid;		/* adapter's SCSI ID */
	Mailbox	mbox[Nmbox];
	Mailbox	*free;		/* next free Mailbox */
	Rendez	mboxes;		/* wait for free mbox */
	QLock	mboxq;		/* mutex for mboxes */
	Rendez	ogm;		/* wait for free OGM slot with Ctlr qlocked */
};

static	Ctlr	ultra[Nctl];

int	scsidebugs[8] = {0};
int	scsiownid = 7;

static void
prmbox(Mailbox *m)
{
	int i;

	print("scsi %lx: op=%2.2x dir=%x cmd=%d [", m->physaddr, m->op&07, (m->op>>3)&3, m->cmdlen);
	for(i=0; i<m->cmdlen; i++)
		print(" %2.2x", m->cmd[i]);
	print("] id=%d lun=%d chan=%d", m->addr&7, (m->addr>>5)&7, (m->addr>>3)&3);
	print(" dlen=%lx dptr=%lx\n", *(long*)m->datalen, *(long*)m->datap);
	print(" astat=%2.2x dstat=%2.2x sc=%d [", m->adapterstatus, m->targetstatus, m->nscatter);
	for(i=0; i<m->nscatter; i++)
		print(" %lx,%ld", m->sglist[i].start, m->sglist[i].len);
	print("]\n");
	/*for(i=0; i<10; i++)
		print(" %2.2x", m->sensedata[i]);*/
	print("\n");
}

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
freebox(Ctlr *ctlr, Mailbox *m)
{
	lock(ctlr);
	if((m->next = ctlr->free) == 0)
		wakeup(&ctlr->mboxes);
	ctlr->free = m;
	unlock(ctlr);
}

static int
boxavail(void *a)
{
	return ((Ctlr*)a)->free != 0;
}

static int
scatter(Scatter *list, void *va, ulong nb)
{
	char *p = (char *)va;
	Scatter *sp = list;
	int limit = Nscatter;

	for(; nb != 0; sp++){
		if(--limit < 0)
			panic("Ultrastor I/O too scattered");
		sp->start = PADDR(p);
		sp->len = BY2PG - (sp->start&(BY2PG-1));	/* the rest of that page */
		if(sp->len > nb)
			sp->len = nb;
		nb -= sp->len;
		p += sp->len;
	}
	return sp - list;
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
ultra14fexec(Scsi *p, int rflag)
{
	Ctlr *ctlr = &ultra[0];
	Mailbox *m;
	long n;
	uchar *cp;

	if (p == 0 || ctlr->io == 0)
		return 0x0200;	/* device not available */

	/*
	 * get a free Mailbox and build an Ultrastor command
	 */
	while(lock(ctlr), (m = ctlr->free) == 0){
		unlock(ctlr);
		qlock(&ctlr->mboxq);
		if(waserror()){
			qunlock(&ctlr->mboxq);
			nexterror();
		}
		sleep(&ctlr->mboxes, boxavail, ctlr);
		poperror();
		qunlock(&ctlr->mboxq);
	}
	ctlr->free = m->next;
	unlock(ctlr);
	p->rflag = rflag;
	m->p9status = 0;
	m->op = OpTarget | CmdDirection | Usecache;	/* BUG? is it safe always to Usecache? */
	m->addr = p->target | (p->lun<<5);
	if(p->cmd.lim - p->cmd.ptr > sizeof(m->cmd))
		panic("scsiexec");
	m->cmdlen = p->cmd.lim - p->cmd.ptr;
	for(cp = m->cmd; p->cmd.ptr < p->cmd.lim;)
		*cp++ = *p->cmd.ptr++;
	n = p->data.lim - p->data.ptr;
	if(n){
		PL(m->datap, PADDR(m->sglist));
		m->op |= Scattered;
		m->nscatter = scatter(m->sglist, p->data.ptr, n);
	} else {
		m->nscatter = 0; /* controller will not allow s/g when n==0 */
		PL(m->datap, 0);
	}
	PL(m->datalen, n);
	m->adapterstatus = 0;
	m->targetstatus = 0;

	/*
	 * send the command to the host adapter
	 */
	while(lock(ctlr), IN(AdapterIntr) & SignalAdapter) {	/* adapter busy: infrequent? */
		static int fred;
		if(fred == 0){
			print("ultrastor busy\n");
			fred = 1;
		}
		unlock(ctlr);
		qlock(ctlr);
		if(waserror()) {
			freebox(ctlr, m);
			qunlock(ctlr);
			nexterror();
		}
		sleep(&ctlr->ogm, isready, ctlr);
		poperror();
		qunlock(ctlr);
	}
	m->busy = 1;
	outl(ctlr->io+OGMpointer, m->physaddr);
	OUT(AdapterIntr, SignalAdapter);
	unlock(ctlr);

	/*
	 * wait for reply
	 */
	while(waserror())
		;	/* could send MSCP abort request to adapter */
	while(m->busy)
		sleep(&m->iodone, isdone, m);
	poperror();
	p->status = m->p9status;
	if(scsidebugs[2])
		prmbox(m);
	if(p->status == 0x6000)
		p->data.ptr = p->data.lim;	/* can only assume no residue */
	freebox(ctlr, m);
	return p->status;
}

static void
scsimoan(char *msg, Mailbox *m)
{
	/*int i;*/

	print("SCSI error: %s:", msg);
	print("id=%d cmd=%2.2x status=%2.2x adapter=%2.2x", m->addr&7, m->cmd[0], m->targetstatus, m->adapterstatus);
	/*print(" sense:");
	for(i=0; i<10; i++)
		print(" %2.2x", m->sensedata[i]);*/
	print("\n");
}

static void
interrupt(Ureg *ur, void *a)
{
	Ctlr *ctlr = &ultra[0];
	Mailbox *m;
	ulong pa;

	USED(ur, a);
	if(ctlr->ogm.p && (IN(AdapterIntr) & SignalAdapter) == 0)
		wakeup(&ctlr->ogm);
	if((IN(HostIntr) & SignalHost) == 0)
		return;	/* no incoming mail */
	pa = inl(ctlr->io+ICMpointer);
	OUT(HostIntr, SignalHost);	/* release ICM slot */
	for(m = &ctlr->mbox[0]; m->physaddr != pa;)
		if(++m >= &ctlr->mbox[Nmbox]) {
			/* these sometimes happen in response to a scsi bus reset; ignore them */
			print("ultrastor: invalid ICM pointer #%lux\n", pa);
			return;
		}
	if(scsidebugs[1])
		prmbox(m);

	m->p9status = 0x1000 | m->adapterstatus;
	switch(m->adapterstatus){
	default:
		scsimoan("adapter", m);
		if(m->adapterstatus >= 0x92) /* ie, scsi command protocol error */
			resetadapter(BusReset);
		/* might adapter reset be required in other cases? */
		break;

	case 0x91:	/* selection timed out */
		m->p9status = 0x0200;
		if(scsidebugs[0])
			scsimoan("timeout", m);
		break;

	case 0xA3:
		scsimoan("SCSI bus reset error", m);
		break;

	case 0x00:
		m->p9status = 0x6000 | m->targetstatus;
		if(m->targetstatus && scsidebugs[0])
			scsimoan("device", m);
		break;
	}
	m->busy = 0;
	wakeup(&m->iodone);
}

static ISAConf ultra14f = {
	"ultra14f",
	Port,
	Irq,
	0,
	0,
};

static int irq[4] = {
	15, 14, 11, 10,
};

int (*
ultra14freset(void))(Scsi*, int)
{
	ISAConf *isa = &ultra14f;
	Ctlr *ctlr = &ultra[0];
	Mailbox *m;
	int i, model;

	/*
	 * See if we have any configuration info
	 * and check it is an Ultrastor [13]4f.
	 * Only one controller for now.
	 */
	if(isaconfig("scsi", 0, &ultra14f) == 0)
		return 0;
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->io = isa->port;

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
		m = &ctlr->mbox[i];
		memset(m, 0, sizeof(*m));
		m->physaddr = PADDR(m);
		PL(m->sensep, 0);	/* DON'T collect sense data automatically: messes up scuzz? */
		m->senselen = 0;
		m->next = ctlr->free;
		ctlr->free = m;
	}

	setvec(Int0vec+ctlr->irq, interrupt, 0);

	/*
	 * issue AdapterInquiry command to check it's alive
	 * -- could check enquiry data
	 */
	OUT(HostMask, 0);	/* mask interrupts */
	m = &ctlr->mbox[0];
	m->op = OpAdapter | CmdDirection;
	m->addr = ctlr->ownid;
	m->cmdlen = 1;
	m->cmd[0] = AdapterInquiry;
	PL(m->datap, PADDR(m->sensedata));
	PL(m->datalen, 38);
	while(IN(AdapterIntr) & SignalAdapter)
		;
	outl(ctlr->io+OGMpointer, m->physaddr);
	OUT(AdapterIntr, SignalAdapter);
	for(i=100000; (IN(HostIntr)&SignalHost)==0;)
		if(--i < 0){
			print("No response to UltraStor Inquiry\n");
			ctlr->io = 0;
			return 0;
		}
	OUT(HostIntr, SignalHost);

	OUT(HostMask, ICMIntEnable|SIntEnable);
	scsiownid = ctlr->ownid;

	print("scsi%d: %s: port %lux irq %d, dma = %d\n",
		0, isa->type, ctlr->io, ctlr->irq, ctlr->dma);

	return ultra14fexec;
}
