/*
 * bcm2835 dma controller
 *
 * simplest to use only channels 0-6
 *	channels 7-14 have reduced functionality
 *	channel 15 is at a weird address
 *	channels 0 and 15 have an "external 128 bit 8 word read FIFO"
 *	  for memory to memory transfers
 *
 * Experiments show that only channels 2-5,11-12 work with mmc
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define DMAREGS	(VIRTIO+0x7000)

#define DBG	if(Dbg)

enum {
	Nchan		= 7,		/* number of dma channels */
	Regsize		= 0x100,	/* size of regs for each chan */
	Cbalign		= 32,		/* control block byte alignment */
	Dbg		= 0,
	
	/* registers for each dma controller */
	Cs		= 0x00>>2,
	Conblkad	= 0x04>>2,
	Ti		= 0x08>>2,
	Sourcead	= 0x0c>>2,
	Destad		= 0x10>>2,
	Txfrlen		= 0x14>>2,
	Stride		= 0x18>>2,
	Nextconbk	= 0x1c>>2,
	Debug		= 0x20>>2,

	/* collective registers */
	Intstatus	= 0xfe0>>2,
	Enable		= 0xff0>>2,

	/* Cs */
	Reset		= 1<<31,
	Abort		= 1<<30,
	Error		= 1<<8,
	Waitwrite	= 1<<6,
	Waitdreq	= 1<<5,
	Paused		= 1<<4,
	Dreq		= 1<<3,
	Int		= 1<<2,
	End		= 1<<1,
	Active		= 1<<0,

	/* Ti */
	Permapshift= 16,
	Srcignore	= 1<<11,
	Srcdreq		= 1<<10,
	Srcwidth128	= 1<<9,
	Srcinc		= 1<<8,
	Destignore	= 1<<7,
	Destdreq	= 1<<6,
	Destwidth128	= 1<<5,
	Destinc		= 1<<4,
	Waitresp	= 1<<3,
	Tdmode		= 1<<1,
	Inten		= 1<<0,

	/* Debug */
	Lite		= 1<<28,
	Clrerrors	= 7<<0,
};

typedef struct Ctlr Ctlr;
typedef struct Cb Cb;

struct Ctlr {
	u32int	*regs;
	Cb	*cb;
	Rendez	r;
	int	dmadone;
};

struct Cb {
	u32int	ti;
	u32int	sourcead;
	u32int	destad;
	u32int	txfrlen;
	u32int	stride;
	u32int	nextconbk;
	u32int	reserved[2];
};

static Ctlr dma[Nchan];
static u32int *dmaregs = (u32int*)DMAREGS;

static void
dump(char *msg, uchar *p, int n)
{
	print("%s", msg);
	while(n-- > 0)
		print(" %2.2x", *p++);
	print("\n");
}

static void
dumpdregs(char *msg, u32int *r)
{
	int i;

	print("%s: %#p =", msg, r);
	for(i = 0; i < 9; i++)
		print(" %8.8uX", r[i]);
	print("\n");
}

static int
dmadone(void *a)
{
	return ((Ctlr*)a)->dmadone;
}

static void
dmainterrupt(Ureg*, void *a)
{
	Ctlr *ctlr;

	ctlr = a;
	ctlr->regs[Cs] = Int;
	ctlr->dmadone = 1;
	wakeup(&ctlr->r);
}

void
dmastart(int chan, int dev, int dir, void *src, void *dst, int len)
{
	Ctlr *ctlr;
	Cb *cb;
	int ti;

	ctlr = &dma[chan];
	if(ctlr->regs == nil){
		ctlr->regs = (u32int*)(DMAREGS + chan*Regsize);
		ctlr->cb = xspanalloc(sizeof(Cb), Cbalign, 0);
		assert(ctlr->cb != nil);
		dmaregs[Enable] |= 1 << chan;
		ctlr->regs[Cs] = Reset;
		while(ctlr->regs[Cs] & Reset)
			;
		intrenable(IRQDMA(chan), dmainterrupt, ctlr, 0, "dma");
	}
	cb = ctlr->cb;
	ti = 0;
	switch(dir){
	case DmaD2M:
		cachedwbinvse(dst, len);
		ti = Srcdreq | Destinc;
		cb->sourcead = DMAIO(src);
		cb->destad = DMAADDR(dst);
		break;
	case DmaM2D:
		cachedwbse(src, len);
		ti = Destdreq | Srcinc;
		cb->sourcead = DMAADDR(src);
		cb->destad = DMAIO(dst);
		break;
	case DmaM2M:
		cachedwbse(src, len);
		cachedwbinvse(dst, len);
		ti = Srcinc | Destinc;
		cb->sourcead = DMAADDR(src);
		cb->destad = DMAADDR(dst);
		break;
	}
	cb->ti = ti | dev << Permapshift | Inten;
	cb->txfrlen = len;
	cb->stride = 0;
	cb->nextconbk = 0;
	cachedwbse(cb, sizeof(Cb));
	ctlr->regs[Cs] = 0;
	microdelay(1);
	ctlr->regs[Conblkad] = DMAADDR(cb);
	DBG print("dma start: %ux %ux %ux %ux %ux %ux\n",
		cb->ti, cb->sourcead, cb->destad, cb->txfrlen,
		cb->stride, cb->nextconbk);
	DBG print("intstatus %ux\n", dmaregs[Intstatus]);
	dmaregs[Intstatus] = 0;
	ctlr->regs[Cs] = Int;
	microdelay(1);
	coherence();
	DBG dumpdregs("before Active", ctlr->regs);
	ctlr->regs[Cs] = Active;
	DBG dumpdregs("after Active", ctlr->regs);
}

int
dmawait(int chan)
{
	Ctlr *ctlr;
	u32int *r;
	int s;

	ctlr = &dma[chan];
	tsleep(&ctlr->r, dmadone, ctlr, 3000);
	ctlr->dmadone = 0;
	r = ctlr->regs;
	DBG dumpdregs("after sleep", r);
	s = r[Cs];
	if((s & (Active|End|Error)) != End){
		print("dma chan %d %s Cs %ux Debug %ux\n", chan,
			(s&End)? "error" : "timeout", s, r[Debug]);
		r[Cs] = Reset;
		r[Debug] = Clrerrors;
		return -1;
	}
	r[Cs] = Int|End;
	return 0;
}
