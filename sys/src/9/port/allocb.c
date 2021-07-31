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
} ialloc;

static Block*
_allocb(int size)
{
	Block *b;
	ulong addr;

	if((b = mallocz(sizeof(Block)+size+Hdrspc, 0)) == nil)
		return nil;

	b->next = nil;
	b->list = nil;
	b->free = 0;
	b->flag = 0;
	b->ref = 0;
	_xinc(&b->ref);

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
		panic("_allocb");
	b->wp = b->rp;

	return b;
}

Block*
allocb(int size)
{
	Block *b;

	/*
	 * Check in a process and wait until successful.
	 * Can still error out of here, though.
	 */
	if(up == nil)
		panic("allocb without up: %#p", getcallerpc(&size));
	if((b = _allocb(size)) == nil){
		xsummary();
		mallocsummary();
		panic("allocb: no memory for %d bytes", size);
	}
	setmalloctag(b, getcallerpc(&size));

	return b;
}

Block*
iallocb(int size)
{
	Block *b;
	static int m1, m2, mp;

	if(ialloc.bytes > conf.ialloc){
		if((m1++%10000)==0){
			if(mp++ > 1000){
				active.exiting = 1;
				exit(0);
			}
			iprint("iallocb: limited %lud/%lud\n",
				ialloc.bytes, conf.ialloc);
		}
		return nil;
	}

	if((b = _allocb(size)) == nil){
		if((m2++%10000)==0){
			if(mp++ > 1000){
				active.exiting = 1;
				exit(0);
			}
			iprint("iallocb: no memory %lud/%lud\n",
				ialloc.bytes, conf.ialloc);
		}
		return nil;
	}
	setmalloctag(b, getcallerpc(&size));
	b->flag = BINTR;

	ilock(&ialloc);
	ialloc.bytes += b->lim - b->base;
	iunlock(&ialloc);

	return b;
}

void
freeb(Block *b)
{
	void *dead = (void*)Bdead;
	long ref;

	if(b == nil || (ref = _xdec(&b->ref)) > 0)
		return;

	if(ref < 0){
		dumpstack();
		panic("freeb: ref %ld; caller pc %#p", ref, getcallerpc(&b));
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
		panic("checkb b %s %#p", msg, b);
	if(b->base == dead || b->lim == dead || b->next == dead
	  || b->rp == dead || b->wp == dead){
		print("checkb: base %#p lim %#p next %#p\n",
			b->base, b->lim, b->next);
		print("checkb: rp %#p wp %#p\n", b->rp, b->wp);
		panic("checkb dead: %s", msg);
	}

	if(b->base > b->lim)
		panic("checkb 0 %s %#p %#p", msg, b->base, b->lim);
	if(b->rp < b->base)
		panic("checkb 1 %s %#p %#p", msg, b->base, b->rp);
	if(b->wp < b->base)
		panic("checkb 2 %s %#p %#p", msg, b->base, b->wp);
	if(b->rp > b->lim)
		panic("checkb 3 %s %#p %#p", msg, b->rp, b->lim);
	if(b->wp > b->lim)
		panic("checkb 4 %s %#p %#p", msg, b->wp, b->lim);
}

void
iallocsummary(void)
{
	print("ialloc %lud/%lud\n", ialloc.bytes, conf.ialloc);
}
