/*
 * Adaptec AHA-154[02][BC] Intelligent Host Adapter.
 * Also works for Buslogic BT-747 EISA adapter in non-native mode.
 */
#include "all.h"
#include "io.h"
#include "mem.h"

enum {
	Port		= 0x330,	/* factory defaults: I/O port */
	CtlrID		= 7,		/*	adapter SCSI id */
	Irq		= 11,		/*	interrupt request level */
};

enum {					/* registers */
	Rc		= 0,		/* WO: control */
	Rs		= 0,		/* RO: status */
	Rcdo		= 1,		/* WO: command/data out */
	Rdi		= 1,		/* RO: data in */
	Rif		= 2,		/* RO: interrupt flags */
};

enum {					/* Rc */
	Scrst		= 0x10,		/* SCSI bus reset */
	Irst		= 0x20,		/* interrupt reset */
	Srst		= 0x40,		/* soft reset */
	Hrst		= 0x80,		/* hard reset */
};

enum {					/* Rs */
	Invdcmd		= 0x01,		/* invalid host adapter command */
	Df		= 0x04,		/* data in port full */
	Cdf		= 0x08,		/* command/data port full */
	Idle		= 0x10,		/* SCSI host adapter idle */
	Init		= 0x20,		/* mailbox initialisation required */
	Diagf		= 0x40,		/* internal diagnostic failure */
	Stst		= 0x80,		/* self testing in progress */
};

enum {					/* Rcdo */
	Cnop		= 0x00,		/* no operation */
	Cmbinit		= 0x01,		/* mailbox initialisation */
	Cstart		= 0x02,		/* start SCSI command */
	Cbios		= 0x03,		/* start PC/AT BIOS command */
	Cinquiry	= 0x04,		/* adapter inquiry */
	Cmboie		= 0x05,		/* enable mailbox out available interrupt */
	Cselection	= 0x06,		/* set selection timeout */
	Cbuson		= 0x07,		/* set bus-on time */
	Cbusoff		= 0x08,		/* set bus-off time */
	Ctransfer	= 0x09,		/* set transfer speed */
	Cdevices	= 0x0A,		/* return installed devices */
	Cconfiguration	= 0x0B,		/* return configuration data */
	Ctenable	= 0x0C,		/* enable target mode */
	Csetup		= 0x0D,		/* return setup data */
	Cwbuff		= 0x1A,		/* write adapter channel 2 buffer */
	Crbuff		= 0x1B,		/* read adapter channel 2 buffer */
	Cwfifo		= 0x1C,		/* write adapter FIFO buffer */
	Crfifo		= 0x1D,		/* read adapter FIFO buffer */
	Cecho		= 0x1F,		/* ECHO command data */
	Cdiag		= 0x20,		/* adapter diagnostic */
	Coptions	= 0x21,		/* set host adapter options */
};

enum {					/* Rif */
	Mbif		= 0x01,		/* mailbox in full */
	Mboa		= 0x02,		/* mailbox out available */
	Hacc		= 0x04,		/* host adapter command complete */
	Scrd		= 0x08,		/* SCSI reset detected */
	Ai		= 0x80,		/* any interrupt */
};

typedef struct {
	uchar	cmd;			/* command */
	uchar	msb;			/* CCB pointer MSB */
	uchar	mid;			/* CCB pointer MID */
	uchar	lsb;			/* CCB pointer LSB */
} Mbox;

enum {					/* mailbox commands */
	Mbfree		= 0x00,		/* mailbox is free */

	Mbostart	= 0x01,		/* start SCSI or adapter command */
	Mboabort	= 0x02,		/* abort SCSI or adapter command */

	Mbiok		= 0x01,		/* CCB completed without error */
	Mbiabort	= 0x02,		/* CCB aborted by host */
	Mbinx		= 0x03,		/* aborted CCB not found */
	Mbierror	= 0x04,		/* CCB completed with error */
};

typedef struct {
	uchar	op;			/* command control block operation code */
	uchar	ctl;			/* address and direction control */
	uchar	cmdlen;			/* SCSI command length */
	uchar	reqlen;			/* request sense allocation length */
	uchar	datalen[3];		/* data length (MSB, MID, LSB) */
	uchar	dataptr[3];		/* data pointer (MSB, MID, LSB) */
	uchar	linkptr[3];		/* link pointer (MSB, MID, LSB) */
	uchar	linkid;			/* command linking identifier */
	uchar	hastat;			/* host adapter status */
	uchar	tarstat;		/* target device status */
	uchar	reserved[2];
	uchar	cs[12+0xFF];		/* SCSI command and sense bytes */
} Ccb;

enum {					/* op */
	OInitiator	= 0x00,		/* initiator CCB */
	OTarget		= 0x01,		/* target CCB */
	Osg		= 0x02,		/* initiator CCB with scatter/gather */
	Ordl		= 0x03,		/* initiator CCB, residual data length returned */
	Osgrdl		= 0x04,		/* initiator CCB, both of the above */
	Obdr		= 0x81,		/* bus device reset */
};

enum {					/* ctl */
	CCBdatain	= 0x08,		/* inbound data transfer, length is checked */
	CCBdataout	= 0x10,		/* outbound data transfer, length is checked */
};

enum {					/* hastat */
	Eok		= 0x00,		/* no host adapter detected error */
	Etimeout	= 0x11,		/* selection timeout */
	Elength		= 0x12,		/* data over/under run */
	Ebusfree	= 0x13,		/* unexpected bus free */
	Ebusphase	= 0x14,		/* target bus phase sequence error */
	Eopcode		= 0x16,		/* invalid CCB opcode */
	Elink		= 0x17,		/* linked CCB does not have same LUN */
	Edirection	= 0x18,		/* invalid target direction received from host */
	Eduplicate	= 0x19,		/* duplicate CCB received in target mode */
	Esegment	= 0x1A,		/* invalid CCB or segment list parameter */
};

enum {					/* tarstat */
	Sgood		= 0x00,		/* good status */
	Scheck		= 0x02,		/* check status */
	Sbusy		= 0x08,		/* LUN busy */
};

typedef struct Target Target;
typedef struct Ctlr Ctlr;

struct Target {
	QLock;
	Rendez;

	Ccb	ccb;
	ulong	paddr;			/* physical address of ccb */
	uchar	*sense;			/* address of returned sense data */

	int	done;
	Target	*active;		/* link on active list */

	void	*buf;			/* for copy to > 16Mb */
	void	*data;
	int	copy;
};

struct Ctlr {
	Lock;

	ulong	port;			/* I/O port */
	ulong	id;			/* adapter SCSI id */
	int	ctlrno;
	int	dma;

	uchar	cmd[5];			/* adapter command out */
	uchar	cmdlen;			/* adapter command out length */
	uchar	data[256];		/* adapter command data in */
	uchar	datalen;		/* adapter command data in length */

	Mbox	mb[NTarget+NTarget];	/* mailbox out + mailbox in */

	Mbox	*mbox;			/* current mailbox out index into mb */
	Mbox	*mbix;			/* current mailbox in index into mb */

	Target	target[NTarget];
	Target	*active;		/* link on active list */
};

static Ctlr *aha1542[MaxScsi];

/*
 * Issue a command to the controller. The command and its length is
 * contained in ctlr->cmd and ctlr->cmdlen. If any data is to be
 * returned, ctlr->datalen should be non-0, and the returned data will
 * be placed in ctlr->data.
 * If we see Hacc set, bail out, we'll process
 * the invalid command at interrupt time.
 */
static void
issue(Ctlr *ctlr)
{
	int len;

	len = 0;
	while(len < ctlr->cmdlen){
		if((inb(ctlr->port+Rs) & Cdf) == 0){
			outb(ctlr->port+Rcdo, ctlr->cmd[len]);
			len++;
		}

		if(inb(ctlr->port+Rif) & Hacc)
			return;
	}

	if(ctlr->datalen){
		len = 0;
		while(len < ctlr->datalen){
			if(inb(ctlr->port+Rs) & Df){
				ctlr->data[len] = inb(ctlr->port+Rdi);
				len++;
			}

			if(inb(ctlr->port+Rif) & Hacc)
				return;
		}
	}
}

static int
done(void *arg)
{
	return ((Target*)arg)->done;
}

static int
aha1542io(Device d, int rw, uchar *cmd, int clen, void *data, int dlen)
{
	Ctlr *ctlr;
	Target *tp;
	ulong x;
	int status;

	ctlr = aha1542[d.ctrl];
	if(ctlr->port == 0)
		return 0x0200;
	if(ctlr->id == d.unit)
		return 0x0300;

	/*
	 * Wait for the target to become free,
	 * then set it up. The Adaptec will allow us to
	 * queue multiple transactions per target, but
	 * gives no guarantee about ordering, so we just
	 * allow one per target.
	 */
	tp = &ctlr->target[d.unit];
	qlock(tp);

	/*
	 * If this is a request-sense and we have valid sense data
	 * from the last command, return it immediately.
	 * A pox on these weird enum names and the WD33C93A status
	 * codes.
	 */
	if(cmd[0] == 0x03 && tp->sense){
		memmove(data, tp->sense, 8+tp->sense[7]);
		tp->sense = 0;

		qunlock(tp);
		return 0x6000;
	}
	tp->sense = 0;

	/*
	 * Fill in the ccb. 
	 */
	tp->ccb.op = Ordl;
	tp->ccb.ctl = (d.unit<<5)|((cmd[1]>>5) & 0x07);

	tp->ccb.cmdlen = clen;
	memmove(tp->ccb.cs, cmd, clen);

	tp->ccb.reqlen = 0xFF;

	tp->ccb.datalen[0] = (dlen>>16) & 0xFF;
	tp->ccb.datalen[1] = (dlen>>8) & 0xFF;
	tp->ccb.datalen[2] = dlen;
	
	tp->copy = 0;
	if(dlen){
		tp->data = data;
		if(ctlr->dma && DMAOK(data, RBUFSIZE) == 0){
			if(rw == SCSIwrite)
				memmove(tp->buf, data, dlen);
			data = tp->buf;
			tp->copy = 1;
		}

		if(rw == SCSIread)
			tp->ccb.ctl |= CCBdatain;
		else
			tp->ccb.ctl |= CCBdataout;
	}
	else
		tp->ccb.ctl |= CCBdataout|CCBdatain;

	x = PADDR(data);
	tp->ccb.dataptr[0] = (x>>16) & 0xFF;
	tp->ccb.dataptr[1] = (x>>8) & 0xFF;
	tp->ccb.dataptr[2] = x;

	tp->ccb.linkptr[0] = tp->ccb.linkptr[1] = tp->ccb.linkptr[2] = 0;
	tp->ccb.linkid = 0;

	tp->ccb.hastat = tp->ccb.tarstat = 0;
	tp->ccb.reserved[0] = tp->ccb.reserved[1] = 0;

	/*
	 * Link the target onto the beginning of the
	 * ctlr active list and start the request.
	 * The interrupt routine has to be able to take
	 * requests off the queue in any order.
	 */
	x = splhi();
	tp->done = 0;
	tp->active = ctlr->active;
	ctlr->active = tp;

	ctlr->mbox->msb = (tp->paddr>>16) & 0xFF;
	ctlr->mbox->mid = (tp->paddr>>8) & 0xFF;
	ctlr->mbox->lsb = tp->paddr & 0xFF;
	ctlr->mbox->cmd = Mbostart;

	ctlr->cmd[0] = Cstart;
	ctlr->cmdlen = 1;
	ctlr->datalen = 0;
	issue(ctlr);

	ctlr->mbox++;
	if(ctlr->mbox >= &ctlr->mb[NTarget])
		ctlr->mbox = ctlr->mb;
	splx(x);

	/*
	 * Wait for the request to complete
	 * and return the status.
	 */
	sleep(tp, done, tp);
	if((status = (tp->ccb.hastat<<8)) == 0)
		status = 0x6000;
	status |= tp->ccb.tarstat;

	if(status == 0x6000 && rw == SCSIread && tp->copy)
		memmove(tp->data, tp->buf, dlen);

	/*
	 * If the command returned sense data, keep a note
	 * of where it is for a subsequent request-sense command.
	 */
	if(tp->ccb.tarstat == Scheck && tp->ccb.hastat == Eok)
		tp->sense = &tp->ccb.cs[tp->ccb.cmdlen];

	qunlock(tp);
	return status;
}

static void
interrupt(Ureg *ur, void *arg)
{
	Ctlr *ctlr;
	uchar rif, rs;
	Target *tp, **l;
	ulong paddr;

	USED(ur);
	ctlr = arg;

	/*
	 * Save and clear the interrupt(s). The only
	 * interrupts expected are Hacc, which we ignore,
	 * and Mbif which means something completed.
	 */
	rif = inb(ctlr->port+Rif);
	rs = inb(ctlr->port+Rs);
	outb(ctlr->port+Rc, Irst);
	if(rif & ~(Ai|Hacc|Mbif))
		print("aha1542:%d: interrupt #%2.2ux\n", ctlr->ctlrno, rif);
	if((rif & Hacc) && (rs & Invdcmd))
		print("aha1542:%d: invdcmd #%2.2ux, len %d\n",
			ctlr->ctlrno, ctlr->cmd[0], ctlr->cmdlen);

	/*
	 * Look for something in the mail.
	 * If there is, try to find the recipient from the
	 * ccb address, take it off the active list and
	 * wakeup whoever.
	 */
	while(ctlr->mbix->cmd){
		paddr = (ctlr->mbix->msb<<16)|(ctlr->mbix->mid<<8)|ctlr->mbix->lsb;
		l = &ctlr->active;
		for(tp = *l; tp; tp = tp->active){
			if(tp->paddr == paddr)
				break;
			l = &tp->active;
		}
		if(tp == 0)
			panic("aha1542:%d: no target for ccb #%lux\n", ctlr->ctlrno, paddr);
		*l = tp->active;

		ctlr->mbix->cmd = 0;

		tp->done = 1;
		wakeup(tp);

		ctlr->mbix++;
		if(ctlr->mbix >= &ctlr->mb[NTarget+NTarget])
			ctlr->mbix = &ctlr->mb[NTarget];
	}
}

static void
reset(Ctlr *ctlr)
{
	ulong paddr;
	int i;

	/*
	 * Initialise the software controller and set the board
	 * scanning the mailboxes.
	 * Need code here to tidy things up if we're
	 * resetting after being active.
	 */
	for(i = 0; i < NTarget; i++){
		ctlr->target[i].paddr = PADDR(&ctlr->target[i].ccb);
		ctlr->mbox = ctlr->mb;
		ctlr->mbix = &ctlr->mb[NTarget];

		if(ctlr->dma){
			ctlr->target[i].buf = ialloc(RBUFSIZE, 0);
			if(DMAOK(ctlr->target[i].buf, RBUFSIZE) == 0)
				panic("aha1542:%d: no memory < 16Mb\n", ctlr->ctlrno);
		}
	}

	ctlr->cmd[0] = Cmbinit;
	paddr = PADDR(ctlr->mb);
	ctlr->cmd[1] = NTarget;
	ctlr->cmd[2] = (paddr>>16) & 0xFF;
	ctlr->cmd[3] = (paddr>>8) & 0xFF;
	ctlr->cmd[4] = paddr & 0xFF;
	ctlr->cmdlen = 5;
	ctlr->datalen = 0;
	issue(ctlr);
}

int (*
aha1542reset(int ctlrno, ISAConf *isa))(Device, int, uchar*, int, void*, int)
{
	Ctlr *ctlr;
	ulong s;
	uchar rif, rs;

	/*
	 * See if we have any configuration info.
	 * Only one controller for now.
	 */
	if(isa->port == 0)
		isa->port = Port;

	/*
	 * Attempt to hard-reset the board and reset
	 * the SCSI bus. If the board state doesn't settle to
	 * idle with mailbox initialisation required, either
	 * it isn't an Adaptec or it's broken.
	 * The 154[02]C has an extra I/O port we could use for
	 * identification, but that wouldn't work on the B.
	 */
	outb(isa->port+Rc, Hrst|Scrst);
	delay(5000);
	if(inb(isa->port+Rs) != (Init|Idle))
		return 0;

	ctlr = aha1542[ctlrno] = ialloc(sizeof(Ctlr), 0);
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->port = isa->port;
	ctlr->ctlrno = ctlrno;

	/*
	 * Get the DMA and IRQ info from the board. This will
	 * cause an interrupt which we hope doesn't cause any
	 * trouble because we don't know which one it is yet.
	 * We have to do this to get the DMA info as it won't
	 * be set up if the board has the BIOS disabled.
	 */
	s = splhi();
	ctlr->cmd[0] = Cconfiguration;
	ctlr->cmdlen = 1;
	ctlr->datalen = 3;
	issue(ctlr);
	while(((rif = inb(ctlr->port+Rif)) & Hacc) == 0)
		;
	
	rs = inb(ctlr->port+Rs);
	outb(ctlr->port+Rc, Irst);
	if((rif & Hacc) && (rs & Invdcmd)){
		splx(s);
		return 0;
	}
	splx(s);

	switch(ctlr->dma = ctlr->data[0]){	/* DMA Arbitration Priority */

	case 0x80:				/* Channel 7 */
		outb(0xD6, 0xC3);
		outb(0xD4, 0x03);
		break;

	case 0x40:				/* Channel 6 */
		outb(0xD6, 0xC2);
		outb(0xD4, 0x02);
		break;

	case 0x20:				/* Channel 5 */
		outb(0xD6, 0xC1);
		outb(0xD4, 0x01);
		break;

	case 0x01:				/* Channel 0 */
		outb(0x0B, 0xC0);
		outb(0x0A, 0x00);
		break;

	default:
		/*
		 * This might be an EISA card (e.g. Buslogic 747)
		 * which doesn't have ISA DMA compatibility set
		 * so no DMA channel will show.
		 * Carry on regardless. However, we have to set ctlr->dma
		 * to make sure that accesses aboce 16Mb will be double-
		 * buffered properly.
		print("aha1542:%d: invalid DMA priority #%2.2ux\n",
			ctlr->ctlrno, ctlr->data[0]);
		 */
		ctlr->dma = -1;
		break;
	}

	switch(ctlr->data[1]){			/* Interrupt Channel */

	case 0x40:
		ctlr->data[1] = 15;
		break;

	case 0x20:
		ctlr->data[1] = 14;
		break;

	case 0x08:
		ctlr->data[1] = 12;
		break;

	case 0x04:
		ctlr->data[1] = 11;
		break;

	case 0x02:
		ctlr->data[1] = 10;
		break;

	case 0x01:
		ctlr->data[1] = 9;
		break;

	default:
		print("aha1542:%d: invalid irq #%2.2ux\n", ctlrno, ctlr->data[1]);
		return 0;
	}

	isa->irq = ctlr->data[1];
	setvec(Int0vec+isa->irq, interrupt, ctlr);
	ctlr->id = ctlr->data[2] & 0x07;
	reset(ctlr);

	return aha1542io;
}
