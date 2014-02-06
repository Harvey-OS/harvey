/* Block allocation */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#define ALIGNUP(a)	ROUND((uintptr)(a), BLOCKALIGN)

enum
{
	Hdrspc		= 64,		/* leave room for high-level headers */
	Bdead		= 0x51494F42,	/* "QIOB" */
	Bmagic		= 0x0910b10c,
};

struct
{
	Lock;
	ulong	bytes;
} ialloc;

/*
 * convert the size of a desired buffer to the size needed
 * to include Block overhead and alignment.
 */
ulong
blocksize(ulong size)
{
	return ALIGNUP(sizeof(Block)) + Hdrspc + ALIGNUP(size);
}

/*
 * convert malloced or non-malloced buffer to a Block.
 * used to build custom Block allocators.
 *
 * buf must be at least blocksize(usable) bytes.
 */
Block *
mem2block(void *buf, ulong usable, int malloced)
{
	Block *b;

	if(buf == nil)
		return nil;

	b = (Block *)buf;
	b->next = nil;
	b->list = nil;
	b->free = 0;
	b->flag = 0;
	b->ref = 0;
	b->magic = Bmagic;
	_xinc(&b->ref);

	/* align start of data portion by rounding up */
	b->base = (uchar*)ALIGNUP((ulong)b + sizeof(Block));

	/* align end of data portion by rounding down */
	b->lim = (uchar*)b + (malloced? msize(b): blocksize(usable));
	b->lim = (uchar*)((ulong)b->lim & ~(BLOCKALIGN-1));

	/* leave sluff at beginning for added headers */
	b->wp = b->rp = b->lim - ALIGNUP(usable);
	if(b->rp < b->base)
		panic("mem2block: b->rp < b->base");
	if(b->lim > (uchar*)b + (malloced? msize(b): blocksize(usable)))
		panic("mem2block: b->lim beyond Block end");
	return b;
}

static Block*
_allocb(int size)
{
	return mem2block(mallocz(blocksize(size), 0), size, 1);
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
		splhi();
		xsummary();
		mallocsummary();
		delay(500);
		panic("allocb: no memory for %d bytes; caller %#p", size,
			getcallerpc(&size));
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

	if(b == nil)
		return;
	if(Bmagic && b->magic != Bmagic)
		panic("freeb: bad magic %#lux in Block %#p; caller pc %#p",
			b->magic, b, getcallerpc(&b));

	if((ref = _xdec(&b->ref)) > 0)
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
	b->magic = 0;

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
	if(Bmagic && b->magic != Bmagic)
		panic("checkb: bad magic %#lux in Block %#p", b->magic, b);
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
