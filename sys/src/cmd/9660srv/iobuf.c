#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

#define	NIOBUF		100
#define	HIOB		(NIOBUF/3)

static Iobuf*	hiob[HIOB];		/* hash buckets */
static Iobuf	iobuf[NIOBUF];		/* buffer headers */
static Iobuf*	iohead;
static Iobuf*	iotail;

Iobuf*
getbuf(Xdata *dev, long addr)
{
	Iobuf *p, *h, **l, **f;

	l = &hiob[addr%HIOB];
	for(p = *l; p; p = p->next) {
		if(p->addr == addr && p->dev == dev) {
			p->busy++;
			return p;
		}
	}

	/* Find a non-busy buffer from the tail */
	for(p = iotail; p && p->busy; p = p->prev)
		;
	if(p == 0)
		panic(0, "all buffers busy");

	/* Delete from hash chain */
	f = &hiob[p->addr%HIOB];
	for(h = *f; h; h = h->next) {
		if(h == p) {
			*f = p->hash;
			break;
		}
		f = &h->hash;
	}

	/* Hash and fill */
	p->hash = *l;
	*l = p;
	p->addr = addr;
	p->dev = dev;
	p->busy++;
	if(waserror()) {
		p->addr = 0;	/* stop caching */
		putbuf(p);
		nexterror();
	}
	xread(p);
	poperror();
	return p;
}

void
putbuf(Iobuf *p)
{
	if(p->busy <= 0)
		panic(0, "putbuf");

	p->busy--;
	if(p == iohead)
		return;

	/* Link onto head for lru */
	if(p->prev) 
		p->prev->next = p->next;
	else
		iohead = p->next;

	if(p->next)
		p->next->prev = p->prev;
	else
		iotail = p->prev;

	p->prev = 0;
	p->next = iohead;
	iohead->prev = p;
	iohead = p;
}

void
purgebuf(Xdata *dev)
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

		p->iobuf = sbrk(Sectorsize);
		if((long)p->iobuf == -1)
			panic(0, "iobuf_init");
	}

	iohead->prev = 0;
	iotail->next = 0;
}
