#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"pool.h"

enum
{
	Hdrspc		= 64,		/* leave room for high-level headers */
	Bdead		= 0x51494F42,	/* "QIOB" */
};

struct
{
	Lock;
	ulong	bytes;
} ialloc;

/*
 *  allocate blocks, round the data base up to a multiple of BLOCKALIGN.
 */
Block*
allocb(int size)
{
	Block *b;
	ulong addr;
	int n;

	n = sizeof(Block) + size;
	b = malloc(n+Hdrspc);
	if(b == 0)
		exhausted("Blocks");
	memset(b, 0, sizeof(Block));

	/* align start of data portion by rounding up */
	addr = (ulong)b;
	addr = ROUND(addr + sizeof(Block), BLOCKALIGN);
	b->base = (uchar*)addr;

	/* align end of data portion by rounding down */
	b->lim = ((uchar*)b) + msize(b);
	addr = (ulong)(b->lim);
	addr = addr & ~(BLOCKALIGN-1);
	b->lim = (uchar*)addr;

	/* leave sluff at beginning for added headers */
	b->rp = b->lim - ROUND(size, BLOCKALIGN);
	if(b->rp < b->base)
		panic("allocb");
	b->wp = b->rp;
	setmalloctag(b, getcallerpc(&size));

	return b;
}

/*
 *  interrupt time allocation
 */
Block*
iallocb(int size)
{
	Block *b;
	ulong addr;
	int n;

	if(ialloc.bytes > conf.ialloc){
		print("iallocb: limited %lud/%lud\n",
			ialloc.bytes, conf.ialloc);
		return 0;
	}

	n = sizeof(Block) + size;
	b = malloc(n+Hdrspc);
	if(b == 0){
		print("iallocb: no memory %lud/%lud\n",
			ialloc.bytes, conf.ialloc);
		return nil;
	}
	memset(b, 0, sizeof(Block));

	/* align start of data portion by rounding up */
	addr = (ulong)b;
	addr = ROUND(addr + sizeof(Block), BLOCKALIGN);
	b->base = (uchar*)addr;

	/* align end of data portion by rounding down */
	b->lim = ((uchar*)b) + msize(b);
	addr = (ulong)(b->lim);
	addr = addr & ~(BLOCKALIGN-1);
	b->lim = (uchar*)addr;

	/* leave sluff at beginning for added headers */
	b->rp = b->lim - ROUND(size, BLOCKALIGN);
	if(b->rp < b->base)
		panic("allocb");
	b->wp = b->rp;

	b->flag = BINTR;

	ilock(&ialloc);
	ialloc.bytes += b->lim - b->base;
	iunlock(&ialloc);
	setmalloctag(b, getcallerpc(&size));

	return b;
}

void
freeb(Block *b)
{
	void *dead = (void*)Bdead;

	if(b == nil)
		return;

	/*
	 * drivers which perform non cache coherent DMA manage their own buffer
	 * pool of uncached buffers and provide their own free routine.
	 */
	if(b->free) {
		b->free(b);
		return;
	}
	if(b->flag & BINTR) {
		ilock(&ialloc);
		ialloc.bytes -= b->lim - b->base;
		iunlock(&ialloc);
	}

	/* poison the block in case someone is still holding onto it */
	b->next = dead;
	b->rp = dead;
	b->wp = dead;
	b->lim = dead;
	b->base = dead;

	free(b);
}

void
checkb(Block *b, char *msg)
{
	void *dead = (void*)Bdead;

	if(b == dead)
		panic("checkb b %s %lux", msg, b);
	if(b->base == dead || b->lim == dead || b->next == dead
	  || b->rp == dead || b->wp == dead){
		print("checkb: base 0x%8.8luX lim 0x%8.8luX next 0x%8.8luX\n",
			b->base, b->lim, b->next);
		print("checkb: rp 0x%8.8luX wp 0x%8.8luX\n", b->rp, b->wp);
		panic("checkb dead: %s\n", msg);
	}

	if(b->base > b->lim)
		panic("checkb 0 %s %lux %lux", msg, b->base, b->lim);
	if(b->rp < b->base)
		panic("checkb 1 %s %lux %lux", msg, b->base, b->rp);
	if(b->wp < b->base)
		panic("checkb 2 %s %lux %lux", msg, b->base, b->wp);
	if(b->rp > b->lim)
		panic("checkb 3 %s %lux %lux", msg, b->rp, b->lim);
	if(b->wp > b->lim)
		panic("checkb 4 %s %lux %lux", msg, b->wp, b->lim);

}

void
iallocsummary(void)
{
	print("ialloc %lud/%lud\n", ialloc.bytes, conf.ialloc);
}
