#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

#define	NIOBUF		100
#define	HIOB		(NIOBUF/3)

static Iobuf*	hiob[HIOB];		/* hash buckets */
static Iobuf	iobuf[NIOBUF];		/* buffer headers */
static Iobuf*	iohead;
static Iobuf*	iotail;

Iobuf*
getbuf(Xfs *dev, long addr)
{
	Iobuf *p, *h, **l, **f;

	l = &hiob[addr%HIOB];
	for(p = *l; p; p = p->hash) {
		if(p->addr == addr && p->dev == dev) {
			p->busy++;
			return p;
		}
	}
	/* Find a non-busy buffer from the tail */
	for(p = iotail; p && (p->busy > 0); p = p->prev)
		;
	if(!p)
		panic("all buffers busy");
	if(p->dirty){
		xwrite(p);
		p->dirty = 0;
	}

	if( xread(dev, p, addr) < 0)
		return 0;
	/* Delete from hash chain */
	f = &hiob[p->addr%HIOB];
	if( *f == p )
		*f = p->hash;
	else {
		for(h = *f; h ; h = h->hash)
			if( h->hash == p ){
				h->hash = p->hash;
				break;
			}
	}
	/* Fill and hash */
	p->hash = *l;
	*l = p;
	p->addr = addr;
	p->dev = dev;
	p->busy=1;

	return p;
}
void
putbuf(Iobuf *p)
{
	if(p->busy <= 0)
		panic("putbuf");
	p->busy--;

	/* Link onto head for lru */
	if(p == iohead)
		return;
	if( p == iotail ){
		p->prev->next = 0;
		iotail = p->prev;
	}else{
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}

	p->prev = 0;
	p->next = iohead;
	iohead->prev = p;
	iohead = p;
}
void
dirtybuf(Iobuf *p)
{
	if(p->busy <=0)
		panic("dirtybuf");
	p->dirty = 1;
}
void
syncbuf(void)
{
	Iobuf *p;

	for(p=&iobuf[0] ; p<&iobuf[NIOBUF]; p++)
		if( p->dirty ){
			xwrite(p);
			p->dirty = 0;
		}
}
void
purgebuf(Xfs *dev)
{
	Iobuf *p;

	for(p=&iobuf[0]; p<&iobuf[NIOBUF]; p++)
		if(p->dev == dev)
			p->busy = 0;

	/* Blow hash chains */
	memset(hiob, 0, sizeof(hiob));
}
void
iobuf_init(void)
{
	Iobuf *p;

	iohead = iobuf;
	iotail = iobuf+NIOBUF-1;

	for(p = iobuf; p <= iotail; p++) {
		p->next = p+1;
		p->prev = p-1;
		
		p->iobuf = (char *)malloc(EXT2_MAX_BLOCK_SIZE);
		if(p->iobuf == 0)
			panic("iobuf_init");
	}

	iohead->prev = 0;
	iotail->next = 0;
}
int
xread(Xfs *dev, Iobuf *p, long addr)
{
	/*chat("xread %d,%d...", dev->dev, addr);*/

	seek(dev->dev, (vlong)addr*dev->block_size, 0);
	if(read(dev->dev, p->iobuf, dev->block_size) != dev->block_size){
		chat("xread %d, block=%d failed ...", dev->dev, addr);
		errno = Eio;
		return -1;
	}
	/*chat("xread ok...");*/
	return 0;
}
void 
xwrite(Iobuf *p)
{
	Xfs *dev;
	long addr;

	dev = p->dev;
	addr = p->addr;
	/*chat("xwrite %d,%d...", dev->dev, addr);*/

	seek(dev->dev, (vlong)addr*dev->block_size, 0);
	if(write(dev->dev, p->iobuf, dev->block_size) != dev->block_size){
		chat("xwrite %d, block=%d failed ...", dev->dev, addr);
		errno = Eio;
		return;
	}
	/*chat("xwrite ok...");*/
}
void
dumpbuf(void)
{
	Iobuf *p;
	
	for(p = iotail; p ; p = p->prev)
		if( p->busy )
			mchat("\nHi ERROR buf(%x, %d, %d)\n", p, p->addr, p->busy);	
}
