#include "all.h"
#include "mem.h"
#include "io.h"

long	niob;
long	nhiob;
Hiob	*hiob;

/*
 * Called to allocate permanent data structures
 * Alignment is in number of bytes. It pertains both to the start and
 * end of the allocated memory.
 */
void*
ialloc(ulong n, int align)
{
	Mbank *mbp;
	ulong p;
	int m;

	ilock(&mconf);
	for(mbp = &mconf.bank[0]; mbp < &mconf.bank[mconf.nbank]; mbp++){
		p = mbp->base;

		if(align <= 0)
			align = 4;
		if(m = n % align)
			n += align - m;
		if(m = p % align)
			p += align - m;

		if(p+n > mbp->limit)
			continue;

		mbp->base = p+n;
		iunlock(&mconf);

		memset((void*)(p|KZERO), 0, n);
		return (void*)(p|KZERO);
	}

	iunlock(&mconf);
	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++)
		print("bank[%ld]: base 0x%8.8lux, limit 0x%8.8lux\n",
			mbp - mconf.bank, mbp->base, mbp->limit);
	panic("ialloc(0x%lux, 0x%lx): out of memory\n", n, align);
	return 0;
}

/*
 * allocate rest of mem
 * for io buffers.
 */
#define	HWIDTH	8	/* buffers per hash */
void
iobufinit(void)
{
	long m;
	int i;
	Iobuf *p, *q;
	Hiob *hp;
	Mbank *mbp;

	wlock(&mainlock);	/* init */
	wunlock(&mainlock);

	m = 0;
	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++) {
		m += mbp->limit - mbp->base;
	}
	m -= conf.sparemem;

	niob = m / (sizeof(Iobuf) + RBUFSIZE + sizeof(Hiob)/HWIDTH);
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

	/*
	 * Make sure that no more of bank[0] can be used:
	 * 'check' will do an ialloc(0, 1) to find the base of
	 * sparemem.
	 */
	mconf.bank[0].base = mconf.bank[0].limit+1;

	i = 0;
	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++)
		i += mbp->limit - mbp->base;
	print("	mem left = %d\n", i);
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
