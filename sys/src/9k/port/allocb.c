#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

enum
{
	Hdrspc		= 64,		/* leave room for high-level headers */
	Bdead		= 0x51494F42,	/* "QIOB" */
};

struct
{
	Lock;
	ulong	bytes;
	ulong	limit;

} ialloc;

static Block*
_allocb(int size)
{
	Block *b;
	uchar *p;
	int n;

	n = BLOCKALIGN + ROUNDUP(size+Hdrspc, BLOCKALIGN) + sizeof(Block);
	if((p = malloc(n)) == nil)
		return nil;

	b = (Block*)(p + n - sizeof(Block));	/* block at end of allocated space */
	b->base = p;
	b->next = nil;
	b->list = nil;
	b->free = 0;
	b->flag = 0;

	/* align base and bounds of data */
	b->lim = (uchar*)((uintptr)b & ~((uintptr)BLOCKALIGN-1));

	/* align start of writable data, leaving space below for added headers */
	b->rp = b->lim - ROUNDUP(size, BLOCKALIGN);
	b->wp = b->rp;

	if(b->rp < b->base || b->lim - b->rp < size)
		panic("_allocb");

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
		mallocsummary();
		panic("allocb: no memory for %d bytes", size);
	}

	return b;
}

void
ialloclimit(ulong limit)
{
	ialloc.limit = limit;
}

static Block *
exceeded(char *msg, uint *cntp)
{
	static uint mp;

	if((*cntp)++ % (1<<13) == 0){
		if(mp++ > 1000){
			active.exiting = 1;
			exit(0);
		}
		iprint("iallocb: %s %lud/%lud\n", msg, ialloc.bytes, ialloc.limit);
	}
	return nil;
}

Block*
iallocb(int size)
{
	Block *b;
	static uint m1, m2;

	if(ialloc.bytes > ialloc.limit)
		return exceeded("limited", &m1);
	if((b = _allocb(size)) == nil)
		return exceeded("no memory", &m2);
	b->flag = BINTR;

	ilock(&ialloc);
	ialloc.bytes += b->lim - b->base;
	iunlock(&ialloc);

	return b;
}

void
freeb(Block *b)
{
	uchar *p;

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

	p = b->base;

	/* poison the block in case someone is still holding onto it */
	b->next = (void*)Bdead;
	b->rp = b->wp = b->lim = b->base = (void*)Bdead;

	free(p);
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
	print("ialloc %lud/%lud\n", ialloc.bytes, ialloc.limit);
}
