#include "all.h"
#include "io.h"
#include "mem.h"

typedef	struct	Mcsb	Mcsb;
typedef	struct	Mce	Mce;
typedef	struct	Cqe	Cqe;
typedef	struct	Iopb	Iopb;
typedef	struct	Cib	Cib;
typedef	struct	Crb	Crb;
typedef	struct	Csb	Csb;
typedef	struct	Macsi	Macsi;
typedef	struct	Eagle	Eagle;

enum
{
	MaxEagle	= 1,			/* Max. configurable controllers */
	Nriopb		= 20,			/* Number of Receive IOPB's */
	Ntiopb		= 16,			/* Number of Transmit IOPB's */
	Ncqe		= Nriopb+Ntiopb,	/* Number of Command Queue Entries */
	PktBufAlign	= BY2PG,		/* Alignment of I/O packet buffers */
};

#define NEXT(x, l)	(((x)+1)%(l))
#define OFFSETOF(t, m)	((unsigned)&(((t*)0)->m))
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

/*
 * Master Control/Status Block
 */
struct	Mcsb
{
	ushort	msr;			/* Status Register */
	ushort	pad0x0002;
	ushort	mcr;			/* Control Register */
	ushort	pad0x0006;
	ushort	qhp[2];			/* Queue Head Pointer */
	ushort	pad0x000C[2];
};

#define CNA		0x0001		/* msr: Controller Not Available */
#define BOK		0x0002		/* msr: Board OK */
#define STARTQMODE	0x0001		/* mcr: Start Queue Mode */

/*
 * Master Command Entry
 */
struct	Mce
{
	ushort	mcecr;			/* Control Register */
	ushort	iopba;			/* IOPB Address */
	ulong	tag;			/* Command Tag */
	ushort	pad0x0018[2];
};

/*
 * Command Queue Entry
 */
struct	Cqe
{
	ushort	qecr;			/* Control Register */
	ushort	iopba;			/* IOPB Address */
	ulong	tag;			/* Command Tag */
	ushort	wqn;			/* Work Queue Number */
	ushort	pad0x0026;
};

/*
 * IOPB
 */
struct	Iopb
{
	ushort	command;		/* Command Code */
	ushort	options;		/* Command Options */
	ushort	status;			/* Return Status */
	ushort	irqv;			/* Command Complete IRQ and Vector */
	ushort	eirqv;			/* Error IRQ and Vector */
	ushort	vmeoptions;		/* VMEbus Transfer Options */
	union
	{
		struct			/* Initialise Controller */
		{
			ulong	ciboff;	/* Cib Offset */
		};
		struct			/* Receive/Transmit */
		{
			ulong	addr;	/* Transfer Address */
			ulong	mtu;	/* Transfer Size */
			ushort	padrxtx0;
			ushort	ptlf;	/* Packet Type/Length Field */
			uchar	sna[6];	/* Source Node Address */
			ushort	padrxtx1;
			ushort	law1;	/* Lance Descriptor Word 1 */
			ushort	law3;	/* Lance Descriptor Word 3 */
		};
		uchar	padunion[24];
	};
};

/*
 * Controller Initialisation Block
 */
struct	Cib
{
	uchar	pad0x0000;
	uchar	ncqe;			/* Number of Command Queue Entries */
	ushort	options;		/* Special Network Options */
	ushort	pad0x0004;
	uchar	enaddr[6];		/* Physical Node Address */
	uchar	laf[8];			/* Logical Address Filter */
	uchar	pad0x0014[10];
	ushort	irqv;			/* Command Complete IRQ and Vector */
	ushort	eirqv;			/* Error IRQ and Vector */
	ushort	dmabc;			/* DMA Burst Count */
	uchar	pad0x022[8];
};

/*
 * Command Response Block + RIOPB
 */
struct	Crb
{
	ushort	crsw;			/* Command Response Status Word */
	ushort	pad0x0732;
	ulong	tag;			/* Command Tag */
	ushort	wqn;			/* Work Queue Number */
	ushort	pad0x073A[3];
	Iopb;				/* Returned IOPB */
};

/*
 * Configuration Status Block
 */
struct	Csb
{
	uchar	pad0x0764[48];		/* Miscellaneous Junk */
	uchar	enaddr[6];		/* Physical Node Address */
	uchar	pad0x079A[18];		/* More Junk */
};

/*
 * Multiple Active Command Software Interface.
 * This is 2048 bytes of host/controller communications
 * area.
 */
struct	Macsi
{
	Mcsb;				/* Master Control/Status Block */
	Mce	mce;			/* Master Command Entry */
	Cqe	cqe[Ncqe];		/* Command Queue */
	Iopb	riopb[Nriopb];		/* Receive IOPB's */
	Iopb	tiopb[Ntiopb];		/* Transmit IOPB's */
	Cib	cib;			/* Controller Initialisation Block */
	uchar	hus[1812		/* Host Usable Space */
		- (Ncqe)*sizeof(Cqe)
		- (Nriopb)*sizeof(Iopb)
		- (Ntiopb)*sizeof(Iopb)
		- sizeof(Cib)];
	Crb	crb;			/* Command Response Block */
	Csb	csb;			/* Configuration Status Block */
	uchar	pad0x07AC[84];		/* Controller Statistics Block + pad */
};

/*
 * Software Controller
 */
struct	Eagle
{
	Lock	cqel;
	uchar	nextcqe;
	uchar	re;			/* first rcv IOPB belonging to Eagle */
	uchar	rc;			/* first rcv IOPB belonging to CPU */
	Rendez	rr;
	uchar	te;			/* first tx IOPB belonging to Eagle */
	uchar	tc;			/* first tx IOPB belonging to CPU */
	Rendez	tr;
	Vmedevice* vme;			/* hardware info */
	ulong	vmeaddr;		/* vme2sysmap of this Eagle */
	Enpkt*	rpkt[Nriopb];
	Enpkt*	tpkt[Ntiopb];
	Ifc	ifc;
};

static	Eagle	eagle[MaxEagle];

static
void
qrxiopb(Eagle *e)
{
	Macsi *m;
	Cqe *cqe;

	m = e->vme->address;
	invalcache(e->rpkt[e->rc], sizeof(Enpkt));
	lock(&e->cqel);
	cqe = &m->cqe[e->nextcqe];
	cqe->wqn = 0x02;
	cqe->iopba = OFFSETOF(Macsi, riopb[e->rc]);
	cqe->tag = e->rc;
	e->rc = NEXT(e->rc, Nriopb);
	e->nextcqe = NEXT(e->nextcqe, Ncqe);
	cqe->qecr = 0x01;
	unlock(&e->cqel);
}

static
int
anyrxiopb(Eagle *e)
{
	return e->rc != e->re;
}

static
void
eagleinput(void)
{
	Eagle *e;
	Macsi *m;
	Iopb *iopb;
	int l, type;
	Enpkt *p;
	Ifc *ifc;

	e = getarg();
	ifc = &e->ifc;
	m = e->vme->address;

	print("eagle input %d: %E %I\n", e->vme->ctlr, e->ifc.ea, e->ifc.ipa);	
	for(l = 0; l < Nriopb; l++)
		qrxiopb(e);

loop:
	while(anyrxiopb(e) == 0)
		sleep(&e->rr, anyrxiopb, e);
	iopb = &m->riopb[e->rc];
	if(iopb->status == 0){
		l = iopb->ptlf - 4;
		vmeflush(e->vme->bus, iopb->addr, l);
		p = e->rpkt[e->rc];
		invalcache(p, l);
		type = nhgets(p->type);
		switch(type){
		case Arptype:
			arpreceive(p, l, ifc);
			break;
		case Iptype:
			ipreceive(p, l, ifc);
			break;
		}
		ifc->rxpkt++;
		ifc->work.count++;
		ifc->rate.count += l;
	}
	else
		ifc->rcverr++;
out:
	qrxiopb(e);
	goto loop;
}

static
void
qtxiopb(Eagle *e, int mtu)
{
	Macsi *m;
	Cqe *cqe;

	m = e->vme->address;
	m->tiopb[e->te].mtu = mtu;
	wbackcache(e->tpkt[e->te], mtu);
	invalcache(e->tpkt[e->te], mtu);
	lock(&e->cqel);
	cqe = &m->cqe[e->nextcqe];
	cqe->wqn = 0x03;
	cqe->iopba = OFFSETOF(Macsi, tiopb[e->te]);
	cqe->tag = e->te;
	e->te = NEXT(e->te, Ntiopb);
	e->nextcqe = NEXT(e->nextcqe, Ncqe);
	cqe->qecr = 0x01;
	unlock(&e->cqel);
}

static
int
anytxiopb(Eagle *e)
{
	return NEXT(e->te, Ntiopb) != e->tc;
}

static
void
eagleoutput(void)
{
	Eagle *e;
	Enpkt *p;
	Msgbuf *mb;
	int n;

	e = getarg();
	print("eagle output %d\n", e->vme->ctlr);

loop:
	mb = recv(e->ifc.reply, 0);
	if(mb == 0)
		goto loop;
	memmove(((Enpkt*)(mb->data))->s, e->ifc.ea, Easize);
	n = mb->count;
	if(n > ETHERMAXTU){
		print("eagle: pkt too big\n");
		mbfree(mb);
		goto loop;
	}
	while(anytxiopb(e) == 0)
		sleep(&e->tr, anytxiopb, e);
	p = e->tpkt[e->te];
	memmove(p, mb->data, n);
	if(n < ETHERMINTU){
		memset((char*)p+n, 0, ETHERMINTU-n);
		n = ETHERMINTU;
	}
	qtxiopb(e, n);
	e->ifc.work.count++;
	e->ifc.rate.count += n;
	e->ifc.txpkt++;
	mbfree(mb);
	goto loop;
}

int
eagleinit(Vmedevice *vp)
{
	Eagle *e;
	Macsi *m;
	ushort crsw;
	int l, x;
	uchar *p;

	print("eagle init %d\n", vp->ctlr);
	if(vp->ctlr >= MaxEagle)
		return -1;
	e = &eagle[vp->ctlr];
	if(e->vme)
		return -1;
	m = vp->address;

	/*
	 * does the board exist?
	if(probe(&m->mcr, sizeof(m->mcr)))
		return -1;
	 */

	/*
	 * Set the control register to a known state and
	 * check that the status says the board is available
	 * and passed power-on diagnostics.
	 * If it's not OK, then we could try resetting it
	 * here.
	 */
	m->mcr = 0;
	if((m->msr & (BOK|CNA)) != BOK)
		return -1;
	/*
	 * Set the Controller Initialisation Block.
	 */
	memset(&m->cib, 0, sizeof(Cib));
	m->cib.ncqe = Ncqe;
	m->cib.options = 0x0001;			/* IEEE 802.3 */
	memmove(m->cib.enaddr, m->csb.enaddr, 6);
	m->cib.irqv = (vp->irq<<8)|vp->vector;
	m->cib.eirqv = (vp->irq<<8)|vp->vector;

	/*
	 * Set the IOPB for the MCE. We can use
	 * one of the command queue IOPB's for the
	 * moment, until we turn on queueing.
	 */
	memset(&m->riopb[0], 0, sizeof(Iopb));
	m->riopb[0].command = 0x41;			/* Initialise Controller */
	m->riopb[0].ciboff = OFFSETOF(Macsi, cib);

	/*
	 * Set up the MCE, and start the init.
	 */
	memset(&m->mce, 0, sizeof(Mce));
	m->mce.iopba = OFFSETOF(Macsi, riopb[0]);
	m->mce.mcecr = 0x01;				/* GO */

	/*
	 * Wait for Command Response Block to
	 * become valid.
	 * Free the controller, check for error
	 * or exception.
	 */
	while(((crsw = m->crb.crsw) & 0x01) == 0)	/* Crb valid */
		;
	m->crb.crsw &= ~0x01;				/* free controller */
	if(crsw & 0x0C)					/* error or exception */
		return -1;

	/*
	 * Do we need to initialise the work queues?
	 * The manual says so, in many places, EXCEPT
	 * in the description of the 'INITIALIZE WORK
	 * QUEUE' command, where it says they are automatically
	 * initialised by the 'INITIALIZE CONTROLLER'
	 * command.
	 */

	/*
	 * Allocate space for I/O packets. These need to be
	 * suitably aligned on cache boundaries. PktBufAlign
	 * should be at least LINESIZE, but aligning on page
	 * boundaries seems to give better performance.
	 * Map the space to the VME bus.
	 */
	l = ROUNDUP(sizeof(Enpkt), PktBufAlign);
	p = ialloc(l*(Nriopb+Ntiopb), PktBufAlign);
	e->vmeaddr = vme2sysmap(vp->bus, (ulong)p, l*(Nriopb+Ntiopb));

	/*
	 * Initialise the unchanging parts of the IOPB's,
	 * and the I/O packet pointers.
	 */
	memset(m->cqe, 0, sizeof(m->cqe));
	memset(m->riopb, 0, sizeof(m->riopb));
	for(x = 0; x < Nriopb; x++){
		m->riopb[x].command = 0x60;		/* Receive */
		m->riopb[x].options = 0x05;		/* DMA|IENABLE */
		m->riopb[x].irqv = (vp->irq<<8)|vp->vector;
		m->riopb[x].eirqv = (vp->irq<<8)|vp->vector;
		m->riopb[x].vmeoptions = 0x160B;	/* Block mode, 32b */
		m->riopb[x].addr = e->vmeaddr + l*x;
		m->riopb[x].mtu = sizeof(Enpkt);
		e->rpkt[x] = (Enpkt*)p;
		p += l;
	}
	memset(m->tiopb, 0, sizeof(m->tiopb));
	for(x = 0; x < Ntiopb; x++){
		m->tiopb[x].command = 0x50;		/* Transmit */
		m->tiopb[x].options = 0x05;		/* DMA|IENABLE */
		m->tiopb[x].irqv = (vp->irq<<8)|vp->vector;
		m->tiopb[x].eirqv = (vp->irq<<8)|vp->vector;
		m->tiopb[x].vmeoptions = 0x060B;	/* Block mode, 32b */
		m->tiopb[x].addr = e->vmeaddr + l*(Nriopb+x);
		e->tpkt[x] = (Enpkt*)p;
		p += l;
	}

	/*
	 * Hardware data structures all set up, start the
	 * controller scanning the command queue.
	 * This command will generate an interrupt, after which
	 * we shall post some receive buffers.
	 */
	m->mcr = STARTQMODE;

	/*
	 * Cross-link the Eagle and Vmedevice
	 * structures to show we're happy the 
	 * controller is usable.
	 */
	e->vme = vp;
	vp->private = e;

	memmove(e->ifc.ea, m->csb.enaddr, 6);

	return 0;
}

void
eagleintr(Vmedevice *vme)
{
	Eagle *e;
	Macsi *m;
	Iopb *iopb;

	if(vme->ctlr >= MaxEagle || vme->private == 0)
		panic("eagle%d: spurious intr\n", vme->ctlr);
	e = vme->private;
	m = vme->address;
	switch(m->crb.wqn){

	case 0x02:					/* Receive WQ */
		/*
		 * Save any useful info from the RIOPB in
		 * the real IOPB.
		 * The manual doesn't say where the received
		 * packet length is stored, it's in the mtu
		 * field. We save it in the ptlf field of the
		 * real IOPB for the input process to find.
		 */
		iopb = &m->riopb[m->crb.tag];
		if(iopb->status = m->crb.status){
			iopb->law1 = m->crb.law1;
			iopb->law3 = m->crb.law3;
		}
		iopb->ptlf = m->crb.mtu;
		e->re = NEXT(e->re, Nriopb);
		wakeup(&e->rr);
		break;

	case 0x03:					/* Transmit WQ */
		/*
		 * Nothing much to do,
		 * just bump the queue.
		 */
		e->tc = NEXT(e->tc, Ntiopb);
		wakeup(&e->tr);
		break;

	case 0x00:					/* Miscellaneous WQ */
		if(m->crb.crsw & 0x20)			/* Queue Mode Started */
			break;
		/*FALLTHROUGH*/

	default:
		print("eagle%d: spurious wq: tag #%lux\n", vme->ctlr, m->crb.tag);
		break;
	}
	if((m->crb.crsw & 0x0C) || m->crb.status){
		print("eagle%d: status: wq %d, crsw #%ux, status #%ux\n",
			vme->ctlr, m->crb.wqn,  m->crb.crsw, m->crb.status);
		print("eagle%d: lance: law1 #%ux, law3 #%ux\n",
			vme->ctlr, m->crb.law1, m->crb.law3);
	}
	m->crb.crsw &= ~0x01;				/* free controller */
}

static
void
stats(Eagle *e)
{
	Ifc *ifc;

	print("eagle stats %d\n", e-eagle);
	ifc = &e->ifc;
	print("	work = %F pkts\n", (Filta){&ifc->work, 1});
	print("	rate = %F tBps\n", (Filta){&ifc->rate, 1000});
	print("	err  =    %3ld rc %3ld sum\n", ifc->rcverr, ifc->sumerr);
}

static
void
cmd_state(int argc, char *argv[])
{
	Eagle *e;

	USED(argc);
	USED(argv);

	for(e = &eagle[0]; e < &eagle[MaxEagle]; e++){
		if(e->vme)
			stats(e);
	}
}

void
eaglestart(void)
{
	Eagle *e;
	Ifc *ifc;

	for(e = &eagle[0]; e < &eagle[MaxEagle]; e++){
		if(e->vme){
			ifc = &e->ifc;
			lock(ifc);
			if(ifc->reply == 0){
				dofilter(&ifc->work);
				dofilter(&ifc->rate);
				ifc->reply = newqueue(Nqueue);
			}
			getipa(ifc);
			unlock(ifc);
			userinit(eagleinput, e, "egi");
			userinit(eagleoutput, e, "ego");
			cmd_install("state", "-- eagle stats", cmd_state);

			ifc->next = enets;
			enets = ifc;
		}
	}
	arpstart();
}
