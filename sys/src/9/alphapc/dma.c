#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"io.h"

typedef struct DMAport	DMAport;
typedef struct DMA	DMA;
typedef struct DMAxfer	DMAxfer;

/*
 *  state of a dma transfer
 */
struct DMAxfer
{
	ulong	bpa;		/* bounce buffer physical address */
	void*	bva;		/* bounce buffer virtual address */
	int	blen;		/* bounce buffer length */
	void*	va;		/* virtual address destination/src */
	long	len;		/* bytes to be transferred */
	int	isread;
};

/*
 *  the dma controllers.  the first half of this structure specifies
 *  the I/O ports used by the DMA controllers.
 */
struct DMAport
{
	uchar	addr[4];	/* current address (4 channels) */
	uchar	count[4];	/* current count (4 channels) */
	uchar	page[4];	/* page registers (4 channels) */
	uchar	cmd;		/* command status register */
	uchar	req;		/* request registers */
	uchar	sbm;		/* single bit mask register */
	uchar	mode;		/* mode register */
	uchar	cbp;		/* clear byte pointer */
	uchar	mc;		/* master clear */
	uchar	cmask;		/* clear mask register */
	uchar	wam;		/* write all mask register bit */
};

struct DMA
{
	DMAport;
	int	shift;
	Lock;
	DMAxfer	x[4];
};

DMA dma[2] = {
	{ 0x00, 0x02, 0x04, 0x06,
	  0x01, 0x03, 0x05, 0x07,
	  0x87, 0x83, 0x81, 0x82,
	  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	 0 },

	{ 0xc0, 0xc4, 0xc8, 0xcc,
	  0xc2, 0xc6, 0xca, 0xce,
	  0x8f, 0x8b, 0x89, 0x8a,
	  0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
	 1 },
};

extern int i8237dma;
static void* i8237bva[2];
static int i8237used;

/*
 *  DMA must be in the first 16MB.  This gets called early by the
 *  initialisation routines of any devices which require DMA to ensure
 *  the allocated bounce buffers are below the 16MB limit.
 */
void
_i8237alloc(void)
{
	void* bva;

	if(i8237dma <= 0)
		return;
	if(i8237dma > 2)
		i8237dma = 2;

	bva = xspanalloc(64*1024*i8237dma, BY2PG, 64*1024);
	if(bva == nil || PADDR(bva)+64*1024*i8237dma > 16*MB){
		/*
		 * This will panic with the current
		 * implementation of xspanalloc().
		if(bva != nil)
			xfree(bva);
		 */
		return;
	}

	i8237bva[0] = bva;
	if(i8237dma == 2)
		i8237bva[1] = ((uchar*)i8237bva[0])+64*1024;
}

static void
dmastatus(DMA *dp, int chan, char c)
{
	int a, l, s;

	ilock(dp);
	outb(dp->cbp, 0);
	a = inb(dp->addr[chan]);
	a |= inb(dp->addr[chan])<<8;
	a |= inb(dp->page[chan])<<16;
	a |= inb(0x400|dp->page[chan])<<24;
	outb(dp->cbp, 0);
	l = inb(dp->count[chan]);
	l |= inb(dp->count[chan])<<8;
	s = inb(dp->cmd);
	iunlock(dp);
	print("%c: addr %uX len %uX stat %uX\n", c, a, l, s);
}

int
dmainit(int chan, int maxtransfer)
{
	DMA *dp;
	DMAxfer *xp;
	static int once;

	if(once == 0){
		if(ioalloc(0x00, 0x10, 0, "dma") < 0
		|| ioalloc(0x80, 0x10, 0, "dma") < 0
		|| ioalloc(0xd0, 0x10, 0, "dma") < 0)
			panic("dmainit");
		outb(dma[0].mc, 0);
		outb(dma[1].mc, 0);
		outb(dma[0].cmask, 0);
		outb(dma[1].cmask, 0);
		outb(dma[1].mode, 0xC0);
		once = 1;
	}

	if(maxtransfer > 64*1024)
		maxtransfer = 64*1024;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;
	xp = &dp->x[chan];
	if(xp->bva != nil){
		if(xp->blen < maxtransfer)
			return 1;
		return 0;
	}
//dmastatus(dp, chan, 'I');

	if(i8237used >= i8237dma || i8237bva[i8237used] == nil){
		print("no i8237 DMA bounce buffer < 16MB\n");
		return 1;
	}
	xp->bva = i8237bva[i8237used++];
	xp->bpa = PADDR(xp->bva);
	xp->blen = maxtransfer;
	xp->len = 0;
	xp->isread = 0;

	return 0;
}

void
xdmastatus(int chan)
{
	DMA *dp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	dmastatus(dp, chan, 'X');
}

/*
 *  setup a dma transfer.  if the destination is not in kernel
 *  memory, allocate a page for the transfer.
 *
 *  we assume BIOS has set up the command register before we
 *  are booted.
 *
 *  return the updated transfer length (we can't transfer across 64k
 *  boundaries)
 */
long
dmasetup(int chan, void *va, long len, int isread)
{
	DMA *dp;
	ulong pa;
	uchar mode;
	DMAxfer *xp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;
	xp = &dp->x[chan];
//print("va%lux+", va);
#define tryPCI
#ifndef PCIWADDR
#define PCIWADDR(va)	PADDR(va)
#endif /* PCIWADDR */
#ifdef notdef

	/*
	 *  if this isn't kernel memory or crossing 64k boundary or above 16 meg
	 *  use the bounce buffer.
	 */
	pa = PADDR(va);
	if((((ulong)va)&0xF0000000) != KZERO
	|| (pa&0xFFFF0000) != ((pa+len)&0xFFFF0000)
	|| pa >= 16*MB) {
		if(xp->bva == nil)
			return -1;
		if(len > xp->blen)
			len = xp->blen;
		if(!isread)
			memmove(xp->bva, va, len);
		xp->va = va;
		xp->len = len;
		xp->isread = isread;
		pa = xp->bpa;
	}
	else
		xp->len = 0;
#endif /* notdef */
#ifdef tryISA
	pa = ISAWADDR(va);
#endif /* tryISA */
#ifdef tryPCI
	pa = PCIWADDR(va);
	if((((ulong)va)&0xF0000000) != KZERO){
		if(xp->bva == nil)
			return -1;
		if(len > xp->blen)
			len = xp->blen;
		if(!isread)
			memmove(xp->bva, va, len);
		xp->va = va;
		xp->len = len;
		xp->isread = isread;
		pa = PCIWADDR(xp->bva);
	}
	else
		xp->len = 0;
#endif /* tryPCI */

	/*
	 * this setup must be atomic
	 */
	mode = (isread ? 0x44 : 0x48) | chan;
	ilock(dp);
	outb(dp->cbp, 0);		/* set count & address to their first byte */
	outb(dp->mode, mode);	/* single mode dma (give CPU a chance at mem) */
	outb(dp->addr[chan], pa>>dp->shift);		/* set address */
	outb(dp->addr[chan], pa>>(8+dp->shift));
	outb(dp->page[chan], pa>>16);
#ifdef tryPCI
	outb(0x400|dp->page[chan], pa>>24);
#endif /* tryPCI */
	outb(dp->cbp, 0);		/* set count & address to their first byte */
	outb(dp->count[chan], (len>>dp->shift)-1);		/* set count */
	outb(dp->count[chan], ((len>>dp->shift)-1)>>8);
	outb(dp->sbm, chan);		/* enable the channel */
	iunlock(dp);
//dmastatus(dp, chan, 'S');

	return len;
}

int
dmadone(int chan)
{
	DMA *dp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	return inb(dp->cmd) & (1<<chan);
}

/*
 *  this must be called after a dma has been completed.
 *
 *  if a page has been allocated for the dma,
 *  copy the data into the actual destination
 *  and free the page.
 */
void
dmaend(int chan)
{
	DMA *dp;
	DMAxfer *xp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

//dmastatus(dp, chan, 'E');
	/*
	 *  disable the channel
	 */
	ilock(dp);
	outb(dp->sbm, 4|chan);
	iunlock(dp);

	xp = &dp->x[chan];
	if(xp->len == 0 || !xp->isread)
		return;

	/*
	 *  copy out of temporary page
	 */
	memmove(xp->va, xp->bva, xp->len);
	xp->len = 0;
}

/*
int
dmacount(int chan)
{
	int     retval;
	DMA     *dp;
 
	dp = &dma[(chan>>2)&1];
	outb(dp->cbp, 0);
	retval = inb(dp->count[chan]);
	retval |= inb(dp->count[chan]) << 8;
	return((retval<<dp->shift)+1);
}
 */
