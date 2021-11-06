#include "all.h"
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
	void *p = mallocalign(n, align, 0, 0);

	if (p == nil)
		panic("ialloc: out of memory");
	memset(p, 0, n);
	return p;
}

void
prbanks(void)
{
	Mbank *mbp;

	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++)
		print("bank[%ld]: base 0x%8.8lux, limit 0x%8.8lux (%.0fMB)\n",
			mbp - mconf.bank, mbp->base, mbp->limit,
			(mbp->limit - mbp->base)/(double)MB);
}

static void
cmd_memory(int, char *[])
{
	prbanks();
}

enum { HWIDTH = 8 };		/* buffers per hash */

/*
 * allocate rest of mem
 * for io buffers.
 */
void
iobufinit(void)
{
	long m;
	int i;
	char *xiop;
	Iobuf *p, *q;
	Hiob *hp;
	Mbank *mbp;

	wlock(&mainlock);	/* init */
	wunlock(&mainlock);

	prbanks();
	m = 0;
	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++)
		m += mbp->limit - mbp->base;

	niob = m / (sizeof(Iobuf) + RBUFSIZE + sizeof(Hiob)/HWIDTH);
	nhiob = niob / HWIDTH;
	while(!prime(nhiob))
		nhiob++;
	print("\t%ld buffers; %ld hashes\n", niob, nhiob);
	hiob = ialloc(nhiob * sizeof(Hiob), 0);
	hp = hiob;
	for(i=0; i<nhiob; i++) {
		lock(hp);
		unlock(hp);
		hp++;
	}
	p = ialloc(niob * sizeof(Iobuf), 0);
	xiop = ialloc(niob * RBUFSIZE, 0);
	hp = hiob;
	for(i=0; i < niob; i++) {
//		p->name = "buf";
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
//		p->xiobuf = ialloc(RBUFSIZE, RBUFSIZE);
		p->xiobuf = xiop;
		p->iobuf = (char*)-1;
		p++;
		xiop += RBUFSIZE;
	}

	/*
	 * Make sure that no more of bank[0] can be used.
	 */
	mconf.bank[0].base = mconf.bank[0].limit;

	i = 0;
	for(mbp = mconf.bank; mbp < &mconf.bank[mconf.nbank]; mbp++)
		i += mbp->limit - mbp->base;
	print("\tmem left = %,d, out of %,ld\n", i, conf.mem);
	/* paranoia: add this command as late as is easy */
	cmd_install("memory", "-- print ranges of memory banks", cmd_memory);
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
