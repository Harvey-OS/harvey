#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

enum
{
	Slop = 100,	/* room to grow with reallocation */
};

void
Buffer.sizecache(Buffer *b, uint n)
{
	if(n <= b->cmax)
		return;
	b->cmax = n+Slop;
	b->c = runerealloc(b->c, b->cmax);
}

/*
 * Move cache so b->cq <= q0 < b->cq+b->cnc.
 * If at very end, q0 will fall on end of cache block.
 */

void
Buffer.flush(Buffer *b)
{
	if(b->cdirty || b->cnc==0){
		if(b->cnc == 0)
			b->delblock(b->cbi);
		else
			disk->write(&b->bl[b->cbi], b->c, b->cnc);
		b->cdirty = FALSE;
	}
}

void
Buffer.setcache(Buffer *b, uint q0)
{
	Block **blp, *bl;
	uint i, q;

	check q0 <= b->nc;
	/*
	 * flush and reload if q0 is not in cache.
	 */
	if(b->nc == 0 || (b->cq<=q0 && q0<b->cq+b->cnc))
		return;
	/*
	 * if q0 is at end of file and end of cache, continue to grow this block
	 */
	if(q0==b->nc && q0==b->cq+b->cnc && b->cnc<Maxblock)
		return;
	b->flush();
	/* find block */
	if(q0 < b->cq){
		q = 0;
		i = 0;
	}else{
		q = b->cq;
		i = b->cbi;
	}
	blp = &b->bl[i];
	while(q+(*blp)->n <= q0 && q+(*blp)->n < b->nc){
		q += (*blp)->n;
		i++;
		blp++;
		if(i >= b->nbl)
			error("block not found");
	}
	bl = *blp;
	/* remember position */
	b->cbi = i;
	b->cq = q;
	b->sizecache(bl->n);
	b->cnc = bl->n;
	/*read block*/
	disk->read(bl, b->c, b->cnc);
}

void
Buffer.addblock(Buffer *b, uint i, uint n)
{
	check i <= b->nbl;

	b->bl = realloc(b->bl, (b->nbl+1)*sizeof b->bl[0]);
	if(i < b->nbl)
		memmove(b->bl+i+1, b->bl+i, (b->nbl-i)*sizeof(Block*));
	b->bl[i] = disk->new(n);
	b->nbl++;
}

void
Buffer.delblock(Buffer *b, uint i)
{
	check i < b->nbl;

	b->nbl--;
	if(i < b->nbl)
		memmove(b->bl+i, b->bl+i+1, (b->nbl-i)*sizeof(Block*));
	b->bl = realloc(b->bl, b->nbl*sizeof b->bl[0]);
}

void
Buffer.insert(Buffer *b, uint q0, Rune *s, uint n)
{
	uint i, m, t, off;

	check q0<=b->nc;

	while(n > 0){
		b->setcache(q0);
		off = q0-b->cq;
		if(b->cnc+n <= Maxblock){
			/* Everything fits in one block. */
			t = b->cnc+n;
			m = n;
			if(b->bl == nil){	/* allocate */
				check b->cnc == 0;
				b->addblock(0, t);
				b->cbi = 0;
			}
			b->sizecache(t);
			runemove(b->c+off+m, b->c+off, b->cnc-off);
			runemove(b->c+off, s, m);
			b->cnc = t;
			goto Tail;
		}
		/*
		 * We must make a new block.  If q0 is at
		 * the very beginning or end of this block,
		 * just make a new block and fill it.
		 */
		if(q0==b->cq || q0==b->cq+b->cnc){
			if(b->cdirty)
				b->flush();
			m = min(n, Maxblock);
			if(b->bl == nil){	/* allocate */
				check b->cnc == 0;
				i = 0;
			}else{
				i = b->cbi;
				if(q0 > b->cq)
					i++;
			}
			b->addblock(i, m);
			b->sizecache(m);
			runemove(b->c, s, m);
			b->cq = q0;
			b->cbi = i;
			b->cnc = m;
			goto Tail;
		}
		/*
		 * Split the block; cut off the right side and
		 * let go of it.
		 */
		m = b->cnc-off;
		if(m > 0){
			i = b->cbi+1;
			b->addblock(i, m);
			disk->write(&b->bl[i], b->c+off, m);
			b->cnc -= m;
		}
		/*
		 * Now at end of block.  Take as much input
		 * as possible and tack it on end of block.
		 */
		m = min(n, Maxblock-b->cnc);
		b->sizecache(b->cnc+m);
		runemove(b->c+b->cnc, s, m);
		b->cnc += m;
  Tail:
		b->nc += m;
		q0 += m;
		s += m;
		n -= m;
		b->cdirty = TRUE;
	}
}

void
Buffer.delete(Buffer *b, uint q0, uint q1)
{
	uint m, n, off;

	check q0<=q1 && q0<=b->nc && q1<=b->nc;

	while(q1 > q0){
		b->setcache(q0);
		off = q0-b->cq;
		if(q1 > b->cq+b->cnc)
			n = b->cnc - off;
		else
			n = q1-q0;
		m = b->cnc - (off+n);
		if(m > 0)
			runemove(b->c+off, b->c+off+n, m);
		b->cnc -= n;
		b->cdirty = TRUE;
		q1 -= n;
		b->nc -= n;
	}
}

uint
Buffer.load(Buffer *b, uint q0, int fd, int *nulls)
{
	byte *p;
	Rune *r;
	int l, m, n, nb, nr;
	uint q1;

	check q0<=b->nc;
	p = malloc((Maxblock+UTFmax+1)*sizeof p[0]);
	r = runemalloc(Maxblock);
	m = 0;
	n = 1;
	q1 = q0;
	/*
	 * At top of loop, may have m bytes left over from
	 * last pass, possibly representing a partial rune.
	 */
	while(n > 0){
		n = read(fd, p+m, Maxblock);
		if(n < 0){
			warning(nil, "read error in Buffer.load");
			break;
		}
		m += n;
		p[m] = 0;
		l = m;
		if(n > 0)
			l -= UTFmax;
		cvttorunes(p, l, r, &nb, &nr, nulls);
		runemove(p, p+nb, m-nb);
		m -= nb;
		b->insert(q1, r, nr);
		q1 += nr;
	}
	free(p);
	free(r);
	return q1-q0;
}

void
Buffer.read(Buffer *b, uint q0, Rune *s, uint n)
{
	uint m;

	check q0<=b->nc && q0+n<=b->nc;

	while(n > 0){
		b->setcache(q0);
		m = min(n, b->cnc-(q0-b->cq));
		runemove(s, b->c+(q0-b->cq), m);
		q0 += m;
		s += m;
		n -= m;
	}
}

void
Buffer.reset(Buffer *b)
{
	int i;

	b->nc = 0;
	b->cnc = 0;
	b->cq = 0;
	b->cdirty = 0;
	b->cbi = 0;
	/* delete backwards to avoid n² behavior */
	for(i=b->nbl-1; --i>=0; )
		b->delblock(i);
}

void
Buffer.close(Buffer *b)
{
	b->reset();
	free(b->c);
	b->c = nil;
	b->cnc = 0;
	free(b->bl);
	b->bl = nil;
	b->nbl = 0;
}
