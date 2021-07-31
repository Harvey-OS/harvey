/*
 * Marvell 88SX5040, 5041, 5080, 5081 driver
 * This is a heavily-modified version of a driver written by Coraid, Inc.
 * The original copyright notice appears at the end of this file.
 */
 
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include 	"io.h"
#include	"../port/error.h"

#include	"../port/sd.h"

#define DPRINT	if(0)iprint

enum {
	SrbRing = 32,
	
	/* Addresses of ATA register */
	ARcmd		= 027,
	ARdev		= 026,
	ARerr		= 021,
	ARfea		= 021,
	ARlba2		= 025,
	ARlba1		= 024,
	ARlba0		= 023,
	ARseccnt		= 022,
	ARstat		= 027,
	
	ATAerr	= (1<<0),
	ATAdrq	= (1<<3),
	ATAdf 	= (1<<5),
	ATAdrdy 	= (1<<6),
	ATAbusy 	= (1<<7),
	ATAabort	= (1<<2),
	ATAeIEN	= (1<<1),
	ATAsrst	= (1<<2),
	ATAhob	= (1<<7),

	SFdone = (1<<0),
	SFerror = (1<<1),

	SRBident = 0,
	SRBread,
	SRBwrite,
	SRBsmart,

	SRBnodata = 0,
	SRBdatain,
	SRBdataout,
	
	RQread	= 1,			/* data coming IN from device */
	
	PRDeot	= (1<<15),
	
	/* EDMA interrupt error cause register */

	ePrtDataErr	= (1<<0),
	ePrtPRDErr	= (1<<1),
	eDevErr		= (1<<2),
	eDevDis		= (1<<3),
	eDevCon		= (1<<4),
	eOverrun		= (1<<5),
	eUnderrun	= (1<<6),
	eSelfDis		= (1<<8),
	ePrtCRQBErr	= (1<<9),
	ePrtCRPBErr	= (1<<10),
	ePrtIntErr		= (1<<11),
	eIORdyErr		= (1<<12),

	/* EDMA Command Register */

	eEnEDMA		= (1<<0),
	eDsEDMA 	= (1<<1),
	eAtaRst 		= (1<<2),

	/* Interrupt mask for errors we care about */
	IEM			= (eDevDis | eDevCon | eSelfDis),
	
	Dnull = 0,
	Dnew,
	Dident,
	Dready,
	Derror,
	Dmissing,
	Dunconfig,

	Dext	 	= (1<<0),		/* use ext commands */
	Dpio		= (1<<1),		/* doing pio */
	Dwanted	= (1<<2),		/* someone wants an srb entry */
	Dedma	= (1<<3),		/* device in edma mode */
	Dpiowant	= (1<<4),		/* some wants to use the pio mode */
};

static char* diskstates[] =
{
	"null",
	"new",
	"ident",
	"ready",
	"error",
	"missing",
	"unconfigured",
};

extern SDifc sdmv50xxifc;

typedef struct Arb Arb;
typedef struct Bridge Bridge;
typedef struct Chip Chip;
typedef struct Ctlr Ctlr;
typedef struct Drive Drive;
typedef struct Edma Edma;
typedef struct Prd Prd;
typedef struct Rx Rx;
typedef struct Srb Srb;
typedef struct Tx Tx;

struct Chip	/* pointers to per-Chip mmio */
{
	Arb		*arb;
	Edma	*edma;	/* array of 4 */
};

struct Drive	/* a single disk */
{
	Lock;

	Ctlr		*ctlr;
	SDunit	*unit;
	int		subno;
	char		name[10];

	Bridge	*bridge;
	Edma	*edma;
	Chip		*chip;
	int		chipx;
	
	int		state;
	int		flag;
	uvlong	sectors;

	char		serial[20+1];
	char		firmware[8+1];
	char		model[40+1];

	ushort	info[256];
	
	Srb		*srb[SrbRing-1];
	int		nsrb;
	Prd		*prd;
	Tx		*tx;
	Rx		*rx;
	
	Srb		*srbhead;
	Srb		*srbtail;
};

struct Ctlr		/* a single PCI card */
{
	Lock;

	int		irq;
	int		tbdf;
	SDev		*sdev;
	Pcidev	*pcidev;

	uchar	*mmio;
	Chip		chip[2];
	int		nchip;
	Drive	drive[8];
	int		ndrive;
};

struct Srb		/* request buffer */
{
	Lock;
	Rendez;
	Srb		*next;

	Drive	*drive;
	uvlong	blockno;
	int		count;
	int		req;
	int		flag;
	uchar	*data;
	
	uchar	cmd;
	uchar	lba[6];
	uchar	sectors;
	int		sta;
	int		err;
};

/*
 * Memory-mapped I/O registers in many forms.
 */
struct Bridge	/* memory-mapped per-Drive registers */
{
	ulong	status;
	ulong	serror;
	ulong	sctrl;
	ulong	phyctrl;
	char		fill1[0x2c];
	ulong	ctrl;
	char		fill2[0x34];
	ulong	phymode;
	char		fill3[0x88];	/* pad to 0x100 in length */
};

struct Arb		/* memory-mapped per-Chip registers */
{
	ulong	fill0;
	ulong	rqop;	/* request queue out-pointer */
	ulong	rqip;		/* response queue in pointer */
	ulong	ict;		/* inerrupt caolescing threshold */
	ulong	itt;		/* interrupt timer threshold */
	ulong	ic;		/* interrupt cause */
	ulong	btc;		/* bridges test control */
	ulong	bts;		/* bridges test status */
	ulong	bpc;		/* bridges pin configuration */
	char		fill1[0xdc];
	Bridge	bridge[4];
};

struct Edma	/* memory-mapped per-Drive DMA-related registers */
{
	ulong		config;		/* configuration register */
	ulong		timer;
	ulong		iec;			/* interrupt error cause */
	ulong		iem;			/* interrupt error mask */

	ulong		txbasehi;		/* request queue base address high */
	ulong		txi;			/* request queue in pointer */
	ulong		txo;			/* request queue out pointer */

	ulong		rxbasehi;		/* response queue base address high */
	ulong		rxi;			/* response queue in pointer */
	ulong		rxo;			/* response queue out pointer */
	
	ulong		ctl;			/* command register */
	ulong		testctl;		/* test control */
	ulong		status;
	ulong		iordyto;		/* IORDY timeout */
	char			fill[0xc8];
	ushort		pio;			/* data register */
	char			pad0[2];
	uchar		err;			/* features and error */
	char			pad1[3];
	uchar		seccnt;		/* sector count */
	char			pad2[3];
	uchar		lba0;
	char			pad3[3];
	uchar		lba1;
	char			pad4[3];
	uchar		lba2;
	char			pad5[3];
	uchar		lba3;
	char			pad6[3];
	uchar		cmdstat;		/* cmd/status */
	char			pad7[3];
	uchar		altstat;		/* alternate status */
	char			fill2[0x1edc];	/* pad to 0x2000 bytes */
};

/*
 * Memory structures shared with card.
 */
struct Prd		/* physical region descriptor */
{
	ulong	pa;		/* byte address of physical memory */
	ushort	count;		/* byte count (bit0 must be 0) */
	ushort	flag;
	ulong	zero;			/* high long of 64 bit address */
	ulong	reserved;
};

struct Tx		/* command request block */
{
	ulong	prdpa;		/* physical region descriptor table structures */
	ulong	zero;			/* must be zero (high long of prd address) */
	ushort	flag;			/* control flags */
	ushort	regs[11];
};

struct Rx		/* command response block */
{
	ushort	cid;			/* cID of response */
	uchar	cEdmaSts;		/* EDMA status */
	uchar	cDevSts;		/* status from disk */
	ulong	ts;			/* time stamp */
};

/*
 * Little-endian parsing for drive data.
 */
static ushort
lhgets(void *p)
{
	uchar *a = p;
	return ((ushort) a[1] << 8) | a[0];
}

static ulong
lhgetl(void *p)
{
	uchar *a = p;
	return ((ulong) lhgets(a+2) << 16) | lhgets(a);
}

static uvlong
lhgetv(void *p)
{
	uchar *a = p;
	return ((uvlong) lhgetl(a+4) << 32) | lhgetl(a);
}

static void
idmove(char *p, ushort *a, int n)
{
	char *op;
	int i;
	
	op = p;
	for(i=0; i<n/2; i++){
		*p++ = a[i]>>8;
		*p++ = a[i];
	}
	while(p>op && *--p == ' ')
		*p = 0;
}

/*
 * Request buffers.
 */
struct 
{
	Lock;
	Srb *freechain;
	int nalloc;
} srblist;

static Srb*
allocsrb(void)
{
	Srb *p;
	
	ilock(&srblist);
	if((p = srblist.freechain) == nil){
		srblist.nalloc++;
		iunlock(&srblist);
		p = smalloc(sizeof *p);
	}else{
		srblist.freechain = p->next;
		iunlock(&srblist);
	}
	return p;
}

static void
freesrb(Srb *p)
{
	ilock(&srblist);
	p->next = srblist.freechain;
	srblist.freechain = p;
	iunlock(&srblist);
}

/*
 * Wait for a byte to be a particular value.
 */
static int
satawait(uchar *p, uchar mask, uchar v, int ms)
{
	int i;

//	DPRINT("satawait %p %#x %#x %d...", p, mask, v, ms);
//	DPRINT("!%#x...", *p);
	for(i=0; i<ms && (*p & mask) != v; i++){
		if(i%1000 == 0)
			DPRINT("!%#x", *p);
		microdelay(1000);
	}
	return (*p & mask) == v;
}

/*
 * Drive initialization
 */
static int
configdrive(Ctlr *ctlr, Drive *d, SDunit *unit)
{
	int i;
	ulong *r;
	
	DPRINT("%s: configdrive\n", unit->name);
	d->unit = unit;
	d->ctlr = ctlr;
	d->chipx = unit->subno%4;
	d->chip = &ctlr->chip[unit->subno/4];
	d->bridge = &d->chip->arb->bridge[d->chipx];
	d->edma = &d->chip->edma[d->chipx];

	if(d->tx == nil){
		d->tx = mallocalign(32*sizeof(Tx), 1024, 0, 0);
		d->rx = mallocalign(32*sizeof(Rx), 256, 0, 0);
		d->prd = mallocalign(32*sizeof(Prd), 32, 0, 0);
		if(d->tx == nil || d->rx == nil || d->prd == nil){
			iprint("%s: out of memory allocating ring buffers\n",
				unit->name);
			free(d->tx);
			d->tx = nil;
			free(d->rx);
			d->rx = nil;
			free(d->prd);
			d->prd = nil;
			d->state = Dunconfig;
			return 0;
		}
		for(i=0; i<32; i++)
			d->tx[i].prdpa = PADDR(&d->prd[i]);
		coherence();
	}
	
	/* leave disk interrupts turned off until we use it ... */
	d->edma->iem = 0;
	
	/* ... but enable them on the controller */
	r = (ulong*)(d->ctlr->mmio + 0x1D64);
	if(d->unit->subno < 4)
		*r |= 3 << (d->chipx*2);
	else
		*r |= 3 << (d->chipx*2+9);

	return 1;
}

static int
enabledrive(Drive *d)
{
	Edma *edma;
	
	DPRINT("%s: enabledrive\n", d->unit->name);

	if((d->bridge->status & 0xF) != 0x3){	/* Det */
		DPRINT("%s: not present\n", d->unit->name);
		d->state = Dmissing;
		return 0;
	}
	edma = d->edma;
	if(satawait(&edma->cmdstat, ATAbusy, 0, 10*1000) == 0){
		print("%s: busy timeout\n", d->unit->name);
		d->state = Dmissing;
		return 0;
	}

	edma->iec = 0;
	d->chip->arb->ic &= ~(0x101 << d->chipx);
	edma->config = 0x11F;
	edma->txi = PADDR(d->tx);
	edma->txo = (ulong)d->tx & 0x3E0;
	edma->rxi = (ulong)d->rx & 0xF8;
	edma->rxo = PADDR(d->rx);
	edma->ctl |= 1;		/* enable dma */

	DPRINT("%s: enable interrupts\n", d->unit->name);
	if(d->bridge->status = 0x113)
		d->state = Dnew;
	d->edma->iem = IEM;
	return 1;
}

static void
disabledrive(Drive *d)
{
	int i;
	ulong *r;

	DPRINT("%s: disabledrive\n", d->unit->name);

	if(d->tx == nil)	/* never enabled */
		return;

	d->edma->ctl = 0;
	d->edma->iem = 0;

	r = (ulong*)(d->ctlr->mmio + 0x1D64);
	i = d->chipx;
	if(d->chipx < 4)
		*r &= ~(3 << (i*2));
	else
		*r |= ~(3 << (i*2+9));
}

static int
setudmamode(Drive *d, uchar mode)
{
	Edma *edma;
	
	DPRINT("%s: setudmamode %d\n", d->unit->name, mode);

	edma = d->edma;
	if(satawait(&edma->cmdstat, ATAerr|ATAdrq|ATAdf|ATAdrdy|ATAbusy, ATAdrdy, 15*1000) == 0){
		iprint("%s: cmdstat 0x%.2ux ready timeout\n",
			d->unit->name, edma->cmdstat);
		return 0;
	}
	edma->altstat = ATAeIEN;
	edma->err = 3;
	edma->seccnt = 0x40 | mode;
	edma->cmdstat = 0xEF;
	microdelay(1);
	if(satawait(&edma->cmdstat, ATAbusy, 0, 15*1000) == 0){
		iprint("%s: cmdstat 0x%.2ux busy timeout\n", 
			d->unit->name, edma->cmdstat);
		return 0;
	}
	return 1;
}

static void
identifydrive(Drive *d)
{
	int i;
	ushort *id;
	Edma *edma;
	SDunit *unit;
	
	DPRINT("%s: identifydrive\n", d->unit->name);

	if(setudmamode(d, 5) == 0)	/* do all SATA support 5? */
		goto Error;

	id = d->info;
	memset(d->info, 0, sizeof d->info);
	edma = d->edma;
	if(satawait(&edma->cmdstat, 0xE9, 0x40, 15*1000) == 0)
		goto Error;

	edma->altstat = ATAeIEN;	/* no interrupts */
	edma->cmdstat = 0xEC;
	microdelay(1);
	if(satawait(&edma->cmdstat, ATAbusy, 0, 15*1000) == 0)
		goto Error;
	for(i=0; i<256; i++)
		id[i] = edma->pio;
	if(edma->cmdstat & (ATAerr|ATAdf))
		goto Error;
	i = lhgets(id+83) | lhgets(id+86);
	if(i & (1<<10)){
		d->flag |= Dext;
		d->sectors = lhgetv(id+100);
	}else{
		d->flag &= ~Dext;
		d->sectors = lhgetl(id+60);
	}
	idmove(d->serial, id+10, 20);
	idmove(d->firmware, id+23, 8);
	idmove(d->model, id+27, 40);
	
	unit = d->unit;
	memset(unit->inquiry, 0, sizeof unit->inquiry);
	unit->inquiry[2] = 2;
	unit->inquiry[3] = 2;
	unit->inquiry[4] = sizeof(unit->inquiry)-4;
	idmove((char*)unit->inquiry+8, id+27, 40);

	if(enabledrive(d))
		d->state = Dready;
	else
		d->state = Derror;
	return;

Error:
	DPRINT("error...");
	d->state = Derror;
}

static void abortallsrb(Drive*);

static void
updatedrive(Drive *d, ulong cause)
{
	int x;
	Edma *edma;
	
	if(cause == 0)
		return;

	DPRINT("%s: updatedrive %#lux\n", d->unit->name, cause);

	edma = d->edma;
	if(cause & eDevDis){
		d->state = Dmissing;
		edma->ctl |= eAtaRst;
		microdelay(25);
		edma->ctl &= ~eAtaRst;
		microdelay(25);
	}
	if(cause & eDevCon){
		d->bridge->sctrl = (d->bridge->sctrl & ~0xF) | 1;
		d->state = Dnew;
	}
	if(cause & eSelfDis)
		d->state = Derror;
	edma->iec = 0;
	d->sectors = 0;
	d->unit->sectors = 0;
	abortallsrb(d);
	x = edma->cmdstat;
	USED(x);
}

/*
 * Requests
 */
static Srb*
srbrw(int req, Drive *d, uchar *data, uint sectors, uvlong lba)
{
	int i;
	Srb *srb;
	static uchar cmd[2][2] = { 0xC8, 0x25, 0xCA, 0x35 };

	switch(req){
	case SRBread:
	case SRBwrite:
		break;
	default:
		return nil;
	}
	
	srb = allocsrb();
	srb->req = req;
	srb->drive = d;
	srb->blockno = lba;
	srb->sectors = sectors;
	srb->count = sectors*512;
	srb->flag = 0;
	srb->data = data;

	for(i=0; i<6; i++)
		srb->lba[i] = lba >> (8*i);
	srb->cmd = cmd[srb->req!=SRBread][(d->flag&Dext)!=0];
	return srb;
}

static uintptr
advance(uintptr pa, int shift)
{
	int n, mask;
	
	mask = 0x1F<<shift;
	n = (pa & mask) + (1<<shift);
	return (pa & ~mask) | (n & mask);
}

#define CMD(r, v) (((r)<<8) | ((v)&0xFF))
static void
atarequest(ushort *cmd, Srb *srb, int ext)
{
	*cmd++ = CMD(ARseccnt, 0);
	*cmd++ = CMD(ARseccnt, srb->sectors);
	*cmd++ = CMD(ARfea, 0);
	if(ext){
		*cmd++ = CMD(ARlba0, srb->lba[3]);
		*cmd++ = CMD(ARlba0, srb->lba[0]);
		*cmd++ = CMD(ARlba1, srb->lba[4]);
		*cmd++ = CMD(ARlba1, srb->lba[1]);
		*cmd++ = CMD(ARlba2, srb->lba[5]);
		*cmd++ = CMD(ARlba2, srb->lba[2]);
		*cmd++ = CMD(ARdev, 0xE0);
	}else{
		*cmd++ = CMD(ARlba0, srb->lba[0]);
		*cmd++ = CMD(ARlba1, srb->lba[1]);
		*cmd++ = CMD(ARlba2, srb->lba[2]);
		*cmd++ = CMD(ARdev, srb->lba[3] | 0xE0);
	}
	*cmd++ = CMD(ARcmd, srb->cmd) | (1<<15);
	USED(cmd);
}

static void
startsrb(Drive *d, Srb *srb)
{
	int i;
	Edma *edma;
	Prd *prd;
	Tx *tx;
	
	if(d->nsrb >= nelem(d->srb)){
		srb->next = nil;
		if(d->srbhead)
			d->srbtail->next = srb;
		else
			d->srbhead = srb;
		d->srbtail = srb;
		return;
	}
	
	d->nsrb++;
	for(i=0; i<nelem(d->srb); i++)
		if(d->srb[i] == nil)
			break;
	if(i == nelem(d->srb))
		panic("sdmv50xx: no free srbs");
	d->srb[i] = srb;
	edma = d->edma;
	tx = (Tx*)KADDR(edma->txi);
	tx->flag = (i<<1) | (srb->req == SRBread);
	prd = KADDR(tx->prdpa);
	prd->pa = PADDR(srb->data);
	prd->count = srb->count;
	prd->flag = PRDeot;
	atarequest(tx->regs, srb, d->flag&Dext);
	coherence();
	edma->txi = advance(edma->txi, 5);
}

static void
completesrb(Drive *d)
{
	Edma *edma;
	Rx *rx;
	Srb *srb;
	
	edma = d->edma;
	if((edma->ctl & eEnEDMA) == 0)
		return;
	
	while((edma->rxo & (0x1F<<3)) != (edma->rxi & (0x1F<<3))){
		rx = (Rx*)KADDR(edma->rxo);
		if(srb = d->srb[rx->cid]){
			d->srb[rx->cid] = nil;
			d->nsrb--;
			if(rx->cDevSts & (ATAerr|ATAdf))
				srb->flag |= SFerror;
			srb->flag |= SFdone;
			srb->sta = rx->cDevSts;
			wakeup(srb);
		}else
			iprint("srb missing\n");
		edma->rxo = advance(edma->rxo, 3);
		if(srb = d->srbhead){
			d->srbhead = srb->next;
			startsrb(d, srb);
		}
	}
}
			
static void
abortallsrb(Drive *d)
{
	int i;
	Srb *srb;

	for(i=0; i<nelem(d->srb); i++){
		if(srb = d->srb[i]){
			d->srb[i] = nil;
			d->nsrb--;
			srb->flag |= SFerror|SFdone;
			wakeup(srb);
		}
	}
	while(srb = d->srbhead){
		d->srbhead = srb->next;
		srb->flag |= SFerror|SFdone;
		wakeup(srb);
	}	
}

static int
srbdone(void *v)
{
	Srb *srb;
	
	srb = v;
	return srb->flag & SFdone;
}

/*
 * Interrupts
 */
static void
mv50interrupt(Ureg*, void *a)
{
	int i;
	ulong cause;
	Ctlr *ctlr;
	Drive *drive;
	
	ctlr = a;
	ilock(ctlr);
	cause = *(ulong*)(ctlr->mmio + 0x1D60);
	DPRINT("sd%c: mv50interrupt: 0x%lux\n", ctlr->sdev->idno, cause);
	for(i=0; i<ctlr->ndrive; i++){
		if(cause & (3<<(i*2+i/4))){
			drive = &ctlr->drive[i];
			if(drive->edma == nil)
				continue;		/* not ready yet */
			ilock(drive);
			updatedrive(drive, drive->edma->iec);
			while(ctlr->chip[i/4].arb->ic & (0x0101 << (i%4))){
				ctlr->chip[i/4].arb->ic = ~(0x101 << (i%4));
				completesrb(drive);
			}
			iunlock(drive);
		}
	}
	iunlock(ctlr);
}

/*
 * Device discovery
 */
static SDev*
mv50pnp(void)
{
	int i, nunit;
	uchar *base;
	ulong io;
	void *mem;
	Ctlr *ctlr;
	Pcidev *p;
	SDev *head, *tail, *sdev;

	DPRINT("mv50pnp\n");

	p = nil;
	head = nil;
	tail = nil;
	while((p = pcimatch(p, 0x11AB, 0)) != nil){
		switch(p->did){
		case 0x5041:
			nunit = 4;
			break;
		case 0x5081:
			nunit = 8;
			break;
		default:
			continue;
		}
		if((sdev = malloc(sizeof(SDev))) == nil)
			continue;
		if((ctlr = malloc(sizeof(Ctlr))) == nil){
			free(sdev);
			continue;
		}
		io = p->mem[0].bar & ~0x0F;
		mem = vmap(io, p->mem[0].size);
		if(mem == 0){
			print("sdmv50xx: address 0x%luX in use\n", io);
			free(sdev);
			free(ctlr);
			continue;
		}
		sdev->ifc = &sdmv50xxifc;
		sdev->ctlr = ctlr;
		sdev->nunit = nunit;
		sdev->idno = 'E';
		ctlr->sdev = sdev;
		ctlr->irq = p->intl;
		ctlr->tbdf = p->tbdf;
		ctlr->pcidev = p;
		ctlr->mmio = mem;
		ctlr->nchip = (nunit+3)/4;
		ctlr->ndrive = nunit;
		for(i=0; i<ctlr->nchip; i++){
			base = ctlr->mmio+0x20000+0x10000*i;
			ctlr->chip[i].arb = (Arb*)base;
			ctlr->chip[i].edma = (Edma*)(base + 0x2000);
		}
		if(head)
			tail->next = sdev;
		else
			head = sdev;
		tail = sdev;
	}
	return head;
}

/*
 * Enable the controller.  Each disk has its own interrupt mask,
 * and those get enabled as the disks are brought online.
 */
static int
mv50enable(SDev *sdev)
{
	char name[32];
	Ctlr *ctlr;

	DPRINT("sd%c: enable\n", sdev->idno);

	ctlr = sdev->ctlr;
	snprint(name, sizeof name, "%s (%s)", sdev->name, sdev->ifc->name);
	intrenable(ctlr->irq, mv50interrupt, ctlr, ctlr->tbdf, name);
	return 1;
}

/*
 * Disable the controller.
 */
static int
mv50disable(SDev *sdev)
{
	char name[32];
	int i;
	Ctlr *ctlr;
	Drive *drive;
	
	DPRINT("sd%c: disable\n", sdev->idno);

	ctlr = sdev->ctlr;
	ilock(ctlr);
	for(i=0; i<ctlr->sdev->nunit; i++){
		drive = &ctlr->drive[i];
		ilock(drive);
		disabledrive(drive);
		iunlock(drive);
	}
	iunlock(ctlr);
	snprint(name, sizeof name, "%s (%s)", sdev->name, sdev->ifc->name);
	intrdisable(ctlr->irq, mv50interrupt, ctlr, ctlr->tbdf, name);
	return 0;
}

/*
 * Clean up all disk structures.  Already disabled.
 * Could keep count of number of allocated controllers
 * and free the srblist when it drops to zero.
 */
static void
mv50clear(SDev *sdev)
{
	int i;
	Ctlr *ctlr;
	Drive *d;

	DPRINT("sd%c: clear\n", sdev->idno);

	ctlr = sdev->ctlr;
	for(i=0; i<ctlr->ndrive; i++){
		d = &ctlr->drive[i];
		free(d->tx);
		free(d->rx);
		free(d->prd);
	}
	free(ctlr);
}

/*
 * Check that there is a disk or at least a hot swap bay in the drive.
 */
static int
mv50verify(SDunit *unit)
{
	Ctlr *ctlr;
	Drive *drive;

	DPRINT("%s: verify\n", unit->name);

	/*
	 * First access of unit.
	 */

	ctlr = unit->dev->ctlr;
	drive = &ctlr->drive[unit->subno];
	ilock(ctlr);
	ilock(drive);

	if(!configdrive(ctlr, drive, unit) || !enabledrive(drive)){
		iunlock(drive);
		iunlock(ctlr);
		return 0;
	}
	/*
	 * Need to reset the drive before the first call to 
	 * identifydrive, or else the satawait in setudma will 
	 * freeze the machine when accessing edma->cmdstat.
	 * I do not understand this.		-rsc
	 */
	updatedrive(drive, eDevDis);

	iunlock(drive);
	iunlock(ctlr);

	return 1;
}

/*
 * Check whether the disk is online.
 */
static int
mv50online(SDunit *unit)
{
	Ctlr *ctlr;
	Drive *drive;

	ctlr = unit->dev->ctlr;
	drive = &ctlr->drive[unit->subno];
	ilock(drive);
	if(drive->state == Dready){
		unit->sectors = drive->sectors;
		unit->secsize = 512;
		iunlock(drive);
		return 1;
	}

	DPRINT("%s: online %s\n", unit->name, diskstates[drive->state]);

	if(drive->state == Dnew){
		identifydrive(drive);
		if(drive->state == Dready){
			unit->sectors = drive->sectors;
			unit->secsize = 512;
			iunlock(drive);
			return 2;	/* media changed */
		}
	}
	iunlock(drive);
	return 0;
}

/*
 * Register dumps
 */
typedef struct Regs Regs;
struct Regs
{
	ulong offset;
	char *name;
};

static Regs regsctlr[] =
{
	0x0C28, "pci serr# mask",
	0x1D40, "pci err addr low",
	0x1D44, "pci err addr hi",
	0x1D48, "pci err attr",
	0x1D50, "pci err cmd",
	0x1D58, "pci intr cause",
	0x1D5C, "pci mask cause",
	0x1D60, "device micr",
	0x1D64, "device mimr",
};

static Regs regsarb[] =
{
	0x0004,	"arb rqop",
	0x0008,	"arb rqip",
	0x000C,	"arb ict",
	0x0010,	"arb itt",
	0x0014,	"arb ic",
	0x0018,	"arb btc",
	0x001C,	"arb bts",
	0x0020,	"arb bpc",
};

static Regs regsbridge[] =
{
	0x0000,	"bridge status",
	0x0004,	"bridge serror",
	0x0008,	"bridge sctrl",
	0x000C,	"bridge phyctrl",
	0x003C,	"bridge ctrl",
	0x0074,	"bridge phymode",
};

static Regs regsedma[] =
{
	0x0000,	"edma config",
	0x0004,	"edma timer",
	0x0008,	"edma iec",
	0x000C,	"edma iem",
	0x0010,	"edma txbasehi",
	0x0014,	"edma txi",
	0x0018,	"edma txo",
	0x001C,	"edma rxbasehi",
	0x0020,	"edma rxi",
	0x0024,	"edma rxo",
	0x0028,	"edma c",
	0x002C,	"edma tc",
	0x0030,	"edma status",
	0x0034,	"edma iordyto",
/*	0x0100,	"edma pio",
	0x0104,	"edma err",
	0x0108,	"edma sectors",
	0x010C,	"edma lba0",
	0x0110,	"edma lba1",
	0x0114,	"edma lba2",
	0x0118,	"edma lba3",
	0x011C,	"edma cmdstat",
	0x0120,	"edma altstat",
*/
};

static char*
rdregs(char *p, char *e, void *base, Regs *r, int n, char *prefix)
{
	int i;
	
	for(i=0; i<n; i++)
		p = seprint(p, e, "%s%s%-19s %.8ux\n", 
			prefix ? prefix : "", prefix ? ": " : "",
			r[i].name, *(u32int*)((uchar*)base+r[i].offset));
	return p;
}

static char*
rdinfo(char *p, char *e, ushort *info)
{
	int i;
	
	p = seprint(p, e, "info");
	for(i=0; i<256; i++){
		p = seprint(p, e, "%s%.4ux%s", 
			i%8==0 ? "\t" : "",
			info[i], 
			i%8==7 ? "\n" : "");
	}
	return p;
}

static int
mv50rctl(SDunit *unit, char *p, int l)
{
	char *e, *op;
	Ctlr *ctlr;
	Drive *drive;
	
	if((ctlr = unit->dev->ctlr) == nil)
		return 0;
	drive = &ctlr->drive[unit->subno];
	
	e = p+l;
	op = p;
	if(drive->state == Dready){
		p = seprint(p, e, "model    %s\n", drive->model);
		p = seprint(p, e, "serial   %s\n", drive->serial);
		p = seprint(p, e, "firmware %s\n", drive->firmware);
	}else
		p = seprint(p, e, "no disk present\n");
	p = seprint(p, e, "geometry %llud 512\n", drive->sectors);
	p = rdinfo(p, e, drive->info);
	
	p = rdregs(p, e, drive->chip->arb, regsarb, nelem(regsarb), nil);
	p = rdregs(p, e, drive->bridge, regsbridge, nelem(regsbridge), nil);
	p = rdregs(p, e, drive->edma, regsedma, nelem(regsedma), nil);

	return p-op;
}

static int
mv50wctl(SDunit *unit, Cmdbuf *cb)
{
	Ctlr *ctlr;
	Drive *drive;
	
	USED(unit);
	if(strcmp(cb->f[0], "reset") == 0){
		ctlr = unit->dev->ctlr;
		drive = &ctlr->drive[unit->subno];
		ilock(drive);
		updatedrive(drive, eDevDis);
		iunlock(drive);
		return 0;
	}
	cmderror(cb, Ebadctl);
	return -1;
}

static char*
mv50rtopctl(SDev *sdev, char *p, char *e)
{
	char name[10];
	Ctlr *ctlr;
	
	ctlr = sdev->ctlr;
	if(ctlr == nil)
		return p;

	snprint(name, sizeof name, "sd%c", sdev->idno);
	p = rdregs(p, e, ctlr->mmio, regsctlr, nelem(regsctlr), name);
	/* info for first disk */
	p = rdregs(p, e, ctlr->chip[0].arb, regsarb, nelem(regsarb), name);
	p = rdregs(p, e, &ctlr->chip[0].arb->bridge[0], regsbridge, nelem(regsbridge), name);
	p = rdregs(p, e, &ctlr->chip[0].edma[0], regsedma, nelem(regsedma), name);
	
	return p;
}

static int
mv50rio(SDreq *r)
{
	int count, max, n, status;
	uchar *cmd, *data;
	uvlong lba;
	Ctlr *ctlr;
	Drive *drive;
	SDunit *unit;
	Srb *srb;
	
	unit = r->unit;
	ctlr = unit->dev->ctlr;
	drive = &ctlr->drive[unit->subno];
	cmd = r->cmd;
	
	if((status = sdfakescsi(r, drive->info, sizeof drive->info)) != SDnostatus){
		/* XXX check for SDcheck here */
		r->status = status;
		return status;
	}

	switch(cmd[0]){
	case 0x28:	/* read */
	case 0x2A:	/* write */
		break;
	default:
		print("sdmv50xx: bad cmd 0x%.2ux\n", cmd[0]);
		r->status = SDcheck;
		return SDcheck;
	}
	
	lba = (cmd[2]<<24)|(cmd[3]<<16)|(cmd[4]<<8)|cmd[5];
	count = (cmd[7]<<8)|cmd[8];
	if(r->data == nil)
		return SDok;
	if(r->dlen < count*unit->secsize)
		count = r->dlen/unit->secsize;
	
	/* 
	 * Could arrange here to have an Srb always outstanding:
	 *
	 *	lsrb = nil;
	 *	while(count > 0 || lsrb != nil){
	 *		srb = nil;
	 *		if(count > 0){
	 *			srb = issue next srb;
	 *		}
	 *		if(lsrb){
	 *			sleep on lsrb and handle it
	 *		}
	 *	}
	 *
	 * On the disks I tried, this didn't help.  If anything,
	 * it's a little slower.		-rsc
	 */
	data = r->data;
	while(count > 0){
		/*
		 * Max is 128 sectors (64kB) because prd->count is 16 bits.
		 */
		max = 128;
		n = count;
		if(n > max)
			n = max;
		srb = srbrw(cmd[0]==0x28 ? SRBread : SRBwrite, drive, data, n, lba);
		ilock(drive);
		startsrb(drive, srb);
		iunlock(drive);

		/*
		 * Cannot let user interrupt the DMA.
		 */
		while(waserror())
			;
		tsleep(srb, srbdone, srb, 60*1000);
		poperror();
		
		if(!(srb->flag & SFdone)){
			ilock(drive);
			if(!(srb->flag & SFdone)){
				/*
				 * DMA didn't finish but we have to let go of
				 * the data buffer.  Reset the drive to (try to) keep it
				 * from using the buffer after we're gone.
				 */
				iprint("%s: i/o timeout\n", unit->name);
				updatedrive(drive, eDevDis);
				enabledrive(drive);
				freesrb(srb);
				iunlock(drive);
				error("i/o timeout");
			}
			iunlock(drive);
		}

		if(srb->flag & SFerror){
			freesrb(srb);
			error("i/o error");
		}
		freesrb(srb);
		count -= n;
		lba += n;
		data += n*unit->secsize;
	}
	r->rlen = data - (uchar*)r->data;
	return SDok;	
}

SDifc sdmv50xxifc = {
	"mv50xx",				/* name */

	mv50pnp,			/* pnp */
	nil,				/* legacy */
	mv50enable,		/* enable */
	mv50disable,		/* disable */

	mv50verify,			/* verify */
	mv50online,			/* online */
	mv50rio,				/* rio */
	mv50rctl,			/* rctl */
	mv50wctl,			/* wctl */

	scsibio,			/* bio */
	nil,			/* probe */
	mv50clear,			/* clear */
	mv50rtopctl,			/* rtopctl */
};

/*
 * The original driver on which this one is based came with the 
 * following notice:
 *
 * Copyright 2005
 * Coraid, Inc.
 *
 * This software is provided `as-is,' without any express or implied
 * warranty.  In no event will the author be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1.  The origin of this software must not be misrepresented; you must
 * not claim that you wrote the original software.  If you use this
 * software in a product, an acknowledgment in the product documentation
 * would be appreciated but is not required.
 * 
 * 2.  Altered source versions must be plainly marked as such, and must
 * not be misrepresented as being the original software.
 * 
 * 3.  This notice may not be removed or altered from any source
 * distribution.
 */
