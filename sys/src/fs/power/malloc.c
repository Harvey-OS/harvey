#include "all.h"
#include "mem.h"
#include "io.h"

long	niob;
long	nhiob;
Hiob	*hiob;

struct
{
	Lock;
	ulong	addr;
} palloc;

/*
 * Called to allocate permanent data structures
 * Alignment is in number of bytes. It pertains both to the start and
 * end of the allocated memory.
 */
void*
ialloc(ulong n, int align)
{
	ulong p;
	int m;

	lock(&palloc);
	p = palloc.addr;
	if(p == 0)
		p = (ulong)end & ~KZERO;
	if(align <= 0)
		align = 4;
	if(m = n % align)
		n += align - m;
	if(m = p % align)
		p += align - m;
	palloc.addr = p+n;
	unlock(&palloc);
	if(palloc.addr >= conf.mem)
		panic("ialloc not enough mem");
	memset((void*)(p|KZERO), 0, n);
	return (void*)(p|KZERO);
}

/*
 * allocate rest of mem
 * for io buffers.
 */
#define	HWIDTH	8	/* buffers per hash */
void
iobufinit(void)
{
	long i;
	Iobuf *p, *q;
	Hiob *hp;

	wlock(&mainlock);	/* init */
	wunlock(&mainlock);

	i = conf.mem - ((ulong)ialloc(0, 0) & 0x0fffffffL);
	i -= conf.sparemem;
	niob = i / (sizeof(Iobuf) + RBUFSIZE + sizeof(Hiob)/HWIDTH);
	nhiob = niob / HWIDTH;
	while(!prime(nhiob))
		nhiob++;
	print("	%ld buffers; %ld hashes\n", niob, nhiob);
	hiob = ialloc(nhiob * sizeof(Hiob), 0);
	hp = hiob;
	for(i=0; i<nhiob; i++) {
		lock(hp);
		unlock(hp);
		hp++;
	}
	p = ialloc(niob * sizeof(Iobuf), 0);
	hp = hiob;
	for(i=0; i<niob; i++) {
		p->name = "buf";
		qlock(p);
		qunlock(p);
		if(hp == hiob)
			hp = hiob + nhiob;
		hp--;
		q = hp->link;
		if(q) {
			p->fore = q;
			p->back = q->back;
			q->back = p;
			p->back->fore = p;
		} else {
			hp->link = p;
			p->fore = p;
			p->back = p;
		}
		p->dev = devnone;
		p->addr = -1;
		p->xiobuf = ialloc(RBUFSIZE, RBUFSIZE);
		p->iobuf = (char*)-1;
		p++;
	}
	print("	mem left = %ld\n", conf.mem - ((ulong)ialloc(0, 1) & 0x0fffffffL));
	print("		out of = %ld\n", conf.mem);
}

void*
iobufmap(Iobuf *p)
{
	return p->iobuf = p->xiobuf;
}

void
iobufunmap(Iobuf *p)
{
	p->iobuf = (char*)-1;
}
