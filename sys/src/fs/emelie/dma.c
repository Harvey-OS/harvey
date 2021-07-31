#include "all.h"
#include "mem.h"
#include "io.h"

/*
 *  headland chip set for the safari.
 */
typedef struct DMAport	DMAport;
typedef struct DMA	DMA;
typedef struct DMAxfer	DMAxfer;

enum
{
	/*
	 *  the byte registers for DMA0 are all one byte apart
	 */
	Dma0=		0x00,
	Dma0status=	Dma0+0x8,	/* status port */
	Dma0reset=	Dma0+0xD,	/* reset port */

	/*
	 *  the byte registers for DMA1 are all two bytes apart (why?)
	 */
	Dma1=		0xC0,
	Dma1status=	Dma1+2*0x8,	/* status port */
	Dma1reset=	Dma1+2*0xD,	/* reset port */
};

/*
 *  state of a dma transfer
 */
struct DMAxfer
{
	ulong	v;
	ulong	p;

	void	*va;		/* virtual address destination/src */
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

	Lock;
};

struct DMA
{
	DMAport;
	Lock;
	DMAxfer	x[4];
};

DMA dma[2] = {
	{ 0x00, 0x02, 0x04, 0x06,
	  0x01, 0x03, 0x05, 0x07,
	  0x87, 0x83, 0x81, 0x82,
	  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
	{ 0xc0, 0xc6, 0xca, 0xce,
	  0xc4, 0xc8, 0xcc, 0xcf,
	  0x80, 0x8b, 0x89, 0x8a,
	  0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde },
};

/*
 *  DMA must be in the first 16 meg.  This gets called early by main() to
 *  ensure that.
 */
void
dmainit(void)
{
	int i, chan;
	DMA *dp;
	DMAxfer *xp;

	for(i = 0; i < 2; i++){
		dp = &dma[i];
		for(chan = 0; chan < 4; chan++){
			xp = &dp->x[chan];
			xp->v = (ulong)ialloc(BY2PG, BY2PG);
			xp->p = PADDR(xp->v);
		}
	}
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
	DMAxfer *xp;
	ulong pa;
	uchar mode;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;
	xp = &dp->x[chan];

	/*
	 *  if this isn't kernel memory or crossing 64k boundary or above 16 meg
	 *  use the allocated low memory page.
	 */
	pa = PADDR(va);
	if((((ulong)va)&0xF0000000) != KZERO
	|| (pa&0xFFFF0000) != ((pa+len)&0xFFFF0000)
	|| pa > 16*MB){
		if(len > BY2PG)
			len = BY2PG;
		if(!isread)
			memmove((void*)(xp->v), va, len);
		xp->va = va;
		xp->len = len;
		xp->isread = isread;
		pa = xp->p;
	} else
		xp->len = 0;

	/*
	 * this setup must be atomic
	 */
	lock(dp);
	outb(dp->cbp, 0);		/* set count & address to their first byte */
	mode = (isread ? 0x44 : 0x48) | chan;
	outb(dp->mode, mode);		/* single mode dma (give CPU a chance at mem) */
	outb(dp->addr[chan], pa);		/* set address */
	outb(dp->addr[chan], pa>>8);
	outb(dp->page[chan], pa>>16);
	outb(dp->count[chan], len-1);		/* set count */
	outb(dp->count[chan], (len-1)>>8);
	outb(dp->sbm, chan);		/* enable the channel */
	unlock(dp);

	return len;
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

	/*
	 *  disable the channel
	 */
	lock(dp);
	outb(dp->sbm, 4|chan);
	unlock(dp);

	xp = &dp->x[chan];
	if(xp->len == 0)
		return;

	/*
	 *  copy out of temporary page
	 */
	memmove(xp->va, (void*)(xp->v), xp->len);
	xp->len = 0;
}
