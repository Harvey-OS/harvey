#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include 	"io.h"

/*
 *  headland chip set for the safari.
 */
typedef struct DMAport	DMAport;
typedef struct DMA	DMA;

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

/*
 * setup a dma transfer.
 * needs to change if the address is over 16meg
 *
 *  we assume BIOS has set up the command register before we
 *  are booted.
 */
long
dmasetup(int chan, void *va, long len, int isread)
{
	DMA *dp;
	ulong pa;
	uchar mode;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	pa = PADDR(va);

	/*
	 * this setup must be atomic
	 */
	ilock(dp);
	mode = (isread ? 0x44 : 0x48) | chan;
	EISAOUTB(dp->mode, mode);		/* single mode dma (give CPU a chance at mem) */
	EISAOUTB(dp->page[chan], pa>>16);
	EISAOUTB(dp->cbp, 0);		/* set count & address to their first byte */
	EISAOUTB(dp->addr[chan], pa>>dp->shift);		/* set address */
	EISAOUTB(dp->addr[chan], pa>>(8+dp->shift));
	EISAOUTB(dp->count[chan], (len>>dp->shift)-1);		/* set count */
	EISAOUTB(dp->count[chan], ((len>>dp->shift)-1)>>8);
	EISAOUTB(dp->sbm, chan);		/* enable the channel */
	iunlock(dp);

	return len;
}

/*
 *  this must be called after a dma has been completed.
 */
void
dmaend(int chan)
{
	DMA *dp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	/*
	 *  disable the channel
	 */
	ilock(dp);
	EISAOUTB(dp->sbm, 4|chan);
	iunlock(dp);
}
