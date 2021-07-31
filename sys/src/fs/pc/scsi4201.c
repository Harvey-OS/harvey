/*
 * Buslogic BT-7[45]7[SD] EISA SCSI Host Adapter.
 * Also tested on BT-747C.
 * Could do ISA/VLB/PCI models with a little work.
 */
#include "all.h"
#include "io.h"
#include "mem.h"

enum {					/* EISA configuration space */
	IDPORT0		= 0xC80,

	IOPORT1		= 0xC8B,
	IOPORT2		= 0xC8C,
	IOPORT3		= 0xC8D,
};

enum {					/* registers */
	Rcontrol	= 0x00,		/* WO: control register */
	Rstatus		= 0x00,		/* RO: status register */
	Rcpr		= 0x01,		/* WO: command/parameter register */
	Rdatain		= 0x01,		/* RO: data-in register */
	Rinterrupt	= 0x02,		/* RO: interrupt register */
};

enum {					/* Rcontrol */
	Rsbus		= 0x10,		/* SCSI Bus Reset */
	Rint		= 0x20,		/* Interrupt Reset */
	Rsoft		= 0x40,		/* Soft Reset */
	Rhard		= 0x80,		/* Hard Reset */
};

enum {					/* Rstatus */
	Cmdinv		= 0x01,		/* Command Invalid */
	Dirrdy		= 0x04,		/* Data In Register Ready */
	Cprbsy		= 0x08,		/* Command/Parameter-Out Register Busy */
	Hardy		= 0x10,		/* Host Adapter Ready */
	Inreq		= 0x20,		/* Initialisation Required */
	Dfail		= 0x40,		/* Diagnostic Failure */
	Dact		= 0x80,		/* Diagnostic Active */
};

enum {					/* Rcpr */
	Ciem		= 0x81,		/* Initialise Extended Mailbox */
	Cstart		= 0x02,		/* Start Mailbox Command */
	Ceombri		= 0x05,		/* Enable OMBR Interrupt */
};

enum {					/* Rinterrupt */
	Imbl		= 0x01,		/* Incoming Mailbox Loaded */
	Mbor		= 0x02,		/* Mailbox Out Ready */
	Cmdc		= 0x04,		/* Command Complete */
	Rsts		= 0x08,		/* SCSI Reset State */
	Intv		= 0x80,		/* Interrupt Valid */
};

typedef struct {
	uchar	ccb[4];			/* CCB pointer (LSB, ..., MSB) */
	uchar	btstat;			/* BT-7[45]7[SD] status */
	uchar	sdstat;			/* SCSI device status */
	uchar	pad;
	uchar	code;			/* action/completion code */
} Mbox;

enum {					/* mailbox commands */
	Mbfree		= 0x00,		/* Mailbox not in use */

	Mbostart	= 0x01,		/* Start a mailbox command */
	Mboabort	= 0x02,		/* Abort a mailbox command */

	Mbiok		= 0x01,		/* CCB completed without error */
	Mbiabort	= 0x02,		/* CCB aborted at request of host */
	Mbinx		= 0x03,		/* Aborted CCB not found */
	Mbierror	= 0x04,		/* CCB completed with error */
};

typedef struct Ccb Ccb;
typedef struct Ccb {
	uchar	opcode;			/* Operation code */
	uchar	datadir;		/* Data direction control */
	uchar	cdblen;			/* Length of CDB */
	uchar	senselen;		/* Length of sense area */
	uchar	datalen[4];		/* Data length (LSB, ..., MSB) */
	uchar	dataptr[4];		/* Data pointer (LSB, ..., MSB) */
	uchar	reserved[2];
	uchar	btstat;			/* BT-7[45]7[SD] status */
	uchar	sdstat;			/* SCSI device status */
	uchar	targetid;		/* Target ID */
	uchar	luntag;			/* LUN & tag */
	uchar	cdb[12];		/* Command descriptor block */
	uchar	ccbctl;			/* CCB control */
	uchar	linkid;			/* command linking identifier */
	uchar	linkptr[4];		/* Link pointer (LSB, ..., MSB) */
	uchar	senseptr[4];		/* Sense pointer (LSB, ..., MSB) */
	uchar	sense[0xFF];		/* Sense bytes */

	Rendez;
	int	status;			/* completion status */

	Ccb*	ccb;
} Ccb;

enum {					/* opcode */
	OInitiator	= 0x00,		/* initiator CCB */
	Ordl		= 0x03,		/* initiator CCB with residual data length returned */
};

enum {					/* datadir */
	CCBdatain	= 0x08,		/* inbound data transfer, length is checked */
	CCBdataout	= 0x10,		/* outbound data transfer, length is checked */
};

enum {					/* btstat */
	Eok		= 0x00,		/* CCB completed normally with no errors */
};

enum {					/* sdstat */
	Sgood		= 0x00,		/* good status */
	Scheck		= 0x02,		/* check status */
	Sbusy		= 0x08,		/* LUN busy */
};

enum {					/* luntag */
	TagEnable	= 0x20,		/* Tag enable */
	SQTag		= 0x00,		/* Simple Queue Tag */
	HQTag		= 0x40,		/* Head of Queue Tag */
	OQTag		= 0x80,		/* Ordered Queue Tag */
};

enum {					/* CCB control */
	NoDisc		= 0x08,		/* No disconnect */
	NoUnd		= 0x10,		/* No underrrun error report */
	NoData		= 0x20,		/* No data transfer */
	NoStat		= 0x40,		/* No CCB status if zero */
	NoIntr		= 0x80,		/* No Interrupts */
};

enum {
	NCcb		= 32,		/* number of Ccb's */
	NMbox		= NCcb+1,	/* number of Mbox's */
};

typedef struct {
	ulong	port;			/* I/O port */
	ulong	id;			/* adapter SCSI id */
	int	ctlrno;

	uchar	cmd[6];			/* adapter command out */
	uchar	cmdlen;			/* adapter command out length */
	uchar	data[256];		/* adapter command data in */
	uchar	datalen;		/* adapter command data in length */

	Mbox	mb[NMbox+NMbox];	/* mailbox out + mailbox in */
	Lock	mboxlock;
	Mbox*	mbox;			/* current mailbox out index into mb */
	Mbox*	mbix;			/* current mailbox in index into mb */

	Lock	ccblock;
	QLock	ccbq;
	Rendez	ccbr;
	Ccb*	ccb;

	Lock	cachelock;
	Ccb*	cache[NTarget];
} Ctlr;

static Ctlr *bus4201[MaxScsi];

/*
 * Issue a command to the controller. The command and its length is
 * contained in ctlr->cmd and ctlr->cmdlen. If any data is to be
 * returned, ctlr->datalen should be non-zero, and the returned data
 * will be placed in ctlr->data.
 * If we see Cmdc set, bail out, we'll process the invalid command
 * at interrupt time.
 */
static void
issue(Ctlr *ctlr)
{
	int len;
	ulong port;

	port = ctlr->port;
	if(ctlr->cmd[0] != Cstart && ctlr->cmd[0] != Ceombri){
		while((inb(port+Rstatus) & Hardy) == 0)
			;
	}
	outb(port+Rcpr, ctlr->cmd[0]);

	len = 1;
	while(len < ctlr->cmdlen){
		if((inb(port+Rstatus) & Cprbsy) == 0){
			outb(port+Rcpr, ctlr->cmd[len]);
			len++;
		}
		if(inb(port+Rinterrupt) & Cmdc)
			return;
	}

	if(ctlr->datalen){
		len = 0;
		while(len < ctlr->datalen){
			if(inb(port+Rstatus) & Dirrdy){
				ctlr->data[len] = inb(ctlr->port+Rdatain);
				len++;
			}
			if(inb(port+Rinterrupt) & Cmdc)
				return;
		}
	}
}

static void
ccbfree(Ctlr *ctlr, Ccb *ccb)
{
	lock(&ctlr->ccblock);
	ccb->ccb = ctlr->ccb;
	if(ccb->ccb == 0)
		wakeup(&ctlr->ccbr);
	ctlr->ccb = ccb;
	unlock(&ctlr->ccblock);
}

static int
ccbavailable(void *a)
{
	return ((Ctlr*)a)->ccb != 0;
}

static Ccb*
ccballoc(Ctlr *ctlr)
{
	Ccb *ccb;

	lock(&ctlr->ccblock);
	while((ccb = ctlr->ccb) == 0){
		unlock(&ctlr->ccblock);
		qlock(&ctlr->ccbq);
		sleep(&ctlr->ccbr, ccbavailable, ctlr);
		qunlock(&ctlr->ccbq);
		lock(&ctlr->ccblock);
	}
	ctlr->ccb = ccb->ccb;
	unlock(&ctlr->ccblock);

	return ccb;
}

static int
done(void *arg)
{
	return ((Ccb*)arg)->status;
}

static int
bus4201io(Device d, int rw, uchar *cmd, int clen, void *data, int dlen)
{
	Ctlr *ctlr;
	Ccb *ccb;
	Mbox *mb;
	ulong p, targetid;
	int status;

	if((ctlr = bus4201[d.ctrl]) == 0 || ctlr->port == 0)
		return 0x0200;
	targetid = d.unit & 0x07;
	if(ctlr->id == targetid)
		return 0x0300;

	/*
	 * Ctlr->cache holds the last completed Ccb for this target.
	 * If this command is a request-sense and we have valid sense data
	 * from the last completed Ccb, return it immediately.
	 * A pox on these weird enum names and the WD33C93A status
	 * codes.
	 */
	lock(&ctlr->cachelock);
	if(ccb = ctlr->cache[targetid]){
		ctlr->cache[targetid] = 0;
		if(cmd[0] == 0x03 && ccb->status == 0x6000|Scheck){
			memmove(data, ccb->sense, 8+ccb->sense[7]);
			unlock(&ctlr->cachelock);
			return 0x6000;
		}
	}
	unlock(&ctlr->cachelock);
	if(ccb == 0)
		ccb = ccballoc(ctlr);

	/*
	 * Fill in the ccb. 
	 */
	ccb->opcode = Ordl;

	if(dlen == 0)
		ccb->datadir = CCBdataout|CCBdatain;
	else if(rw == SCSIread)
		ccb->datadir = CCBdatain;
	else
		ccb->datadir = CCBdataout;

	ccb->cdblen = clen;
	memmove(ccb->cdb, cmd, clen);

	ccb->datalen[0] = dlen & 0xFF;
	ccb->datalen[1] = (dlen>>8) & 0xFF;
	ccb->datalen[2] = (dlen>>16) & 0xFF;
	ccb->datalen[3] = (dlen>>24) & 0xFF;
	p = PADDR(data);
	ccb->dataptr[0] = p & 0xFF;
	ccb->dataptr[1] = (p>>8) & 0xFF;
	ccb->dataptr[2] = (p>>16) & 0xFF;
	ccb->dataptr[3] = (p>>24) & 0xFF;

	ccb->btstat = ccb->sdstat = 0;
	ccb->targetid = targetid;
	ccb->luntag = (cmd[1]>>5) & 0x07;
	ccb->ccbctl = NoStat;

	ccb->status = 0;

	/*
	 * There's one more mbox than there there is
	 * ccb so the next one is always free otherwise
	 * we'd queue in ccballoc().
	 */
	lock(&ctlr->mboxlock);
	mb = ctlr->mbox;
	p = PADDR(ccb);
	mb->ccb[0] = p & 0xFF;
	mb->ccb[1] = (p>>8) & 0xFF;
	mb->ccb[2] = (p>>16) & 0xFF;
	mb->ccb[3] = (p>>24) & 0xFF;
	mb->code = Mbostart;
	ctlr->mbox++;
	if(ctlr->mbox >= &ctlr->mb[NMbox])
		ctlr->mbox = ctlr->mb;

	/*
	 * This command does not require Hardy
	 * and doesn't generate a Cmdc interrupt.
	 */
	outb(ctlr->port+Rcpr, Cstart);
	unlock(&ctlr->mboxlock);

	/*
	 * Wait for the request to complete
	 * and return the status.
	 * If there was a check-condition, save the
	 * ccb for a possible request-sense command.
	 */
	sleep(ccb, done, ccb);
	status = ccb->status;

	if(status == (0x6000|Scheck)){
		lock(&ctlr->cachelock);
		if(ctlr->cache[targetid])
			ccbfree(ctlr, ctlr->cache[targetid]);
		ctlr->cache[targetid] = ccb;
		unlock(&ctlr->cachelock);

		return status;
	}
	ccbfree(ctlr, ccb);

	return status;
}

static void
interrupt(Ureg *ur, void *arg)
{
	Ctlr *ctlr;
	ulong port;
	uchar rintr, rstat;
	Mbox *mbox;
	Ccb *ccb;

	USED(ur);
	ctlr = arg;

	/*
	 * Save and clear the interrupt(s). The only
	 * interrupts expected are Cmdc, which we ignore,
	 * and Imbl which means something completed.
	 */
	port = ctlr->port;
	rintr = inb(port+Rinterrupt);
	rstat = inb(port+Rstatus);
	outb(port+Rcontrol, Rint);
	if((rintr & ~(Cmdc|Imbl)) != Intv)
		print("bus4201:%d: interrupt 0x%2.2ux\n", ctlr->ctlrno, rintr);
	if((rintr & Cmdc) && (rstat & Cmdinv))
		print("bus4201:%d: cmdinv 0x%2.2ux, len %d\n",
			ctlr->ctlrno, ctlr->cmd[0], ctlr->cmdlen);

	/*
	 * Look for something in the mail.
	 * If there is, save the status, free the mailbox
	 * and wakeup whoever.
	 */
	for(mbox = ctlr->mbix; mbox->code; mbox = ctlr->mbix){
		ccb = (Ccb*)(KZERO
			   |(mbox->ccb[3]<<24)
			   |(mbox->ccb[2]<<16)
			   |(mbox->ccb[1]<<8)
			   |mbox->ccb[0]);
		if((ccb->status = (mbox->btstat<<8)) == 0)
			ccb->status = 0x6000;
		ccb->status |= mbox->sdstat;
		mbox->code = 0;
		wakeup(ccb);

		ctlr->mbix++;
		if(ctlr->mbix >= &ctlr->mb[NMbox+NMbox])
			ctlr->mbix = &ctlr->mb[NMbox];
	}
}

static struct {
	ulong	port;
	ulong	irq;
	ulong	mem;
	ulong	id;
} slotinfo[MaxEISA];

static ulong port[8] = {
	0x330, 0x334, 0x230, 0x234, 0x130, 0x134, 0x000, 0x000,
};
static ulong irq[8] = {
	9, 10, 11, 12, 14, 15, 0, 0,
};
static ulong mem[4] = {
	0xDC000, 0xD8000, 0xCC000, 0xC8000,
};

int (*
bus4201reset(int ctlrno, ISAConf *isa))(Device, int, uchar*, int, void*, int)
{
	static int already;
	int slot;
	ulong p, x;
	Ctlr *ctlr;
	Ccb *ccb;

	/*
	 * First time through, check if this is an EISA machine.
	 * If not, nothing to do. If it is, run through the slots
	 * looking for appropriate cards and saving the
	 * configuration info.
	 */
	if(already == 0){
		already = 1;
		if(strncmp((char*)(KZERO|0xFFFD9), "EISA", 4))
			return 0;

		for(slot = 1; slot < MaxEISA; slot++){
			p = slot*0x1000;
			x = inl(p+IDPORT0);
			if(x != 0x0142B30A && x != 0x0242B30A)
				continue;

			x = inb(p+IOPORT2);
			slotinfo[slot].port = port[x & 0x07];
			slotinfo[slot].irq = irq[(x>>5) & 0x07];
			slotinfo[slot].mem = mem[(x>>3) & 0x03];

			x = inb(p+IOPORT1);
			slotinfo[slot].id = x & 0x07;
		}
	}

	/*
	 * Look through the found adapters for one that matches
	 * the given port address (if any). The possibilties are:
	 * 1) 0;
	 * 2) a slot address;
	 * 3) a slot+base address;
	 * 4) a base address.
	 */
	if(isa->port == 0){
		for(slot = 1; slot < MaxEISA; slot++){
			if(slotinfo[slot].port)
				break;
		}
	}
	else if(isa->port >= 0x1000){
		if((slot = (isa->port>>16)) < MaxEISA && slotinfo[slot].port){
			if((x = (isa->port & 0xFFF)) && x != slotinfo[slot].port)
				slot = 0;
		}
	}
	else for(slot = 1; slot < MaxEISA; slot++){
		if(isa->port == slotinfo[slot].port)
			break;
	}
	if(slot >= MaxEISA || slotinfo[slot].port == 0)
		return 0;

	p = slot*0x1000 + slotinfo[slot].port;
	slotinfo[slot].port = 0;

	/*
	 * Attempt to hard-reset the board and reset
	 * the SCSI bus. If the board state doesn't settle to
	 * idle with mailbox initialisation required, either
	 * it isn't what we think it is or it's broken.
	 */
	outb(p+Rcontrol, Rhard|Rsbus);
	delay(500);
	if(inb(p+Rstatus) != (Inreq|Hardy))
		return 0;

	ctlr = bus4201[ctlrno] = ialloc(sizeof(Ctlr), 0);
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->port = p;
	ctlr->id = slotinfo[slot].id;
	ctlr->ctlrno = ctlrno;

	ccb = ialloc(sizeof(Ccb)*NCcb, 0);
	for(x = 0; x < NCcb; x++){
		ccb->senselen = sizeof(ccb->sense);
		p = PADDR(ccb->sense);
		ccb->senseptr[0] = p & 0xFF;
		ccb->senseptr[1] = (p>>8) & 0xFF;
		ccb->senseptr[2] = (p>>16) & 0xFF;
		ccb->senseptr[3] = (p>>24) & 0xFF;

		ccb->ccb = ctlr->ccb;
		ctlr->ccb = ccb++;
	}

	isa->port = ctlr->port;
	isa->irq = slotinfo[slot].irq;
	isa->mem = slotinfo[slot].mem;
	isa->size = 0x4000;
	setvec(Int0vec+isa->irq, interrupt, ctlr);

	/*
	 * Initialise the software controller and set the board
	 * scanning the mailboxes.
	 */
	ctlr->mbox = ctlr->mb;
	ctlr->mbix = &ctlr->mb[NMbox];

	ctlr->cmd[0] = Ciem;
	ctlr->cmd[1] = NMbox;
	p = PADDR(ctlr->mb);
	ctlr->cmd[2] = p & 0xFF;
	ctlr->cmd[3] = (p>>8) & 0xFF;
	ctlr->cmd[4] = (p>>16) & 0xFF;
	ctlr->cmd[5] = (p>>24) & 0xFF;
	ctlr->cmdlen = 6;
	ctlr->datalen = 0;
	issue(ctlr);

	return bus4201io;
}
