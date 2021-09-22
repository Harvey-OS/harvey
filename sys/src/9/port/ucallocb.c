/*
 * allocate Blocks from uncached memory
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

enum
{
	Hdrspc		= 64,		/* leave room for high-level headers */
	Bdead		= 0x51494F42,	/* "QIOB" */
};

struct
{
	Lock;
	ulong	bytes;
} ucialloc;

static Block*
_ucallocb(int size)
{
	Block *b;

	if((b = ucalloc(sizeof(Block)+size+Hdrspc)) == nil)
		return nil;

	b->next = nil;
	b->list = nil;
	b->free = 0;
	b->flag = 0;
	b->ref = 1;			/* avoid ainc(&b->ref); */

	/* align start of data portion by rounding up */
	b->base = (uchar*)(ROUND((ulong)b + sizeof(Block), BLOCKALIGN));

	/* align end of data portion by rounding down */
	b->lim = (uchar*)b + msize(b);
	b->lim = (uchar*)((ulong)b->lim & ~(BLOCKALIGN-1));

	/* leave sluff at beginning for added headers */
	b->wp = b->rp = b->lim - ROUND(size, BLOCKALIGN);
	if(b->rp < b->base)
		panic("_ucallocb: b->rp < b->base");
	return b;
}

Block*
ucallocb(int size)
{
	Block *b;

	if((b = _ucallocb(size)) == nil)
		panic("ucallocb: no memory for %d bytes", size);
	setmalloctag(b, getcallerpc(&size));
	return b;
}

Block*
uciallocb(int size)
{
	Block *b;
	static int m1, m2, mp;

	if(0 && ucialloc.bytes > conf.ialloc){
		if((m1++%10000)==0){
			if(mp++ > 1000){
				active.exiting = 1;
				exit(0);
			}
			iprint("uciallocb: limited %lud/%lud\n",
				ucialloc.bytes, conf.ialloc);
		}
		return nil;
	}

	if((b = _ucallocb(size)) == nil){
		if(0 && (m2++%10000)==0){
			if(mp++ > 1000){
				active.exiting = 1;
				exit(0);
			}
			iprint("uciallocb: no memory %lud/%lud\n",
				ucialloc.bytes, conf.ialloc);
		}
		return nil;
	}
	setmalloctag(b, getcallerpc(&size));
	b->flag = BINTR;

	ilock(&ucialloc);
	ucialloc.bytes += b->lim - b->base;
	iunlock(&ucialloc);

	return b;
}

void
ucfreeb(Block *b)
{
	void *dead = (void*)Bdead;
	long ref;

	if(b == nil || (ref = adec(&b->ref)) > 0)
		return;

	if(ref < 0){
		dumpstack();
		panic("ucfreeb: ref %ld; caller pc %#p", ref, getcallerpc(&b));
	}

	/*
	 * drivers which perform non cache coherent DMA manage their own buffer
	 * pool of uncached buffers and provide their own free routine.
	 */
	if(b->free) {
		b->free(b);
		return;
	}
	if(b->flag & BINTR) {
		ilock(&ucialloc);
		ucialloc.bytes -= b->lim - b->base;
		iunlock(&ucialloc);
	}

	/* poison the block in case someone is still holding onto it */
	b->next = dead;
	b->rp = dead;
	b->wp = dead;
	b->lim = dead;
	b->base = dead;

	ucfree(b);
}
