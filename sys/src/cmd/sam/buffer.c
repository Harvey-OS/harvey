#include "sam.h"

int	incache(Buffer*, Posn, Posn);

Buffer *
Bopen(Discdesc *dd)
{
	Buffer *b;

	b = emalloc(sizeof(Buffer));
	b->disc = Dopen(dd);
	Strinit(&b->cache);
	return b;
}

void
Bclose(Buffer *b)
{
	Dclose(b->disc);
	Strclose(&b->cache);
	free(b);
}

int
Bread(Buffer *b, Rune *addr, int n, Posn p0)
{
	int m;

	if(b->c2>b->disc->nrunes || b->c1>b->disc->nrunes)
		panic("bread cache");
	if(p0 < 0)
		panic("Bread p0<0");
	if(p0+n > b->nrunes){
		n = b->nrunes-p0;
		if(n < 0)
			panic("Bread<0");
	}
	if(!incache(b, p0, p0+n)){
		Bflush(b);
		if(n>=BLOCKSIZE/2)
			return Dread(b->disc, addr, n, p0);
		else{
			Posn minp;
			if(b->nrunes-p0>BLOCKSIZE/2)
				m = BLOCKSIZE/2;
			else
				m = b->nrunes-p0;
			if(m<n)
				m = n;
			minp = p0-BLOCKSIZE/2;
			if(minp<0)
				minp = 0;
			m += p0-minp;
			Strinsure(&b->cache, m);
			if(Dread(b->disc, b->cache.s, m, minp)!=m)
				panic("Bread");
			b->cache.n = m;
			b->c1 = minp;
			b->c2 = minp+m;
			b->dirty = FALSE;
		}
	}
	memmove(addr, &b->cache.s[p0-b->c1], n*RUNESIZE);
	return n;
}

void
Binsert(Buffer *b, String *s, Posn p0)
{
	if(b->c2>b->disc->nrunes || b->c1>b->disc->nrunes)
		panic("binsert cache");
	if(p0<0)
		panic("Binsert p0<0");
	if(s->n == 0)
		return;
	if(incache(b, p0, p0) && b->cache.n+s->n<=STRSIZE){
		Strinsert(&b->cache, s, p0-b->c1);
		b->dirty = TRUE;
		if(b->cache.n > BLOCKSIZE*2){
			b->nrunes += s->n;
			Bflush(b);
			/* try to leave some cache around p0 */
			if(p0 >= b->c1+BLOCKSIZE){
				/* first BLOCKSIZE can go */
				Strdelete(&b->cache, 0, BLOCKSIZE);
				b->c1 += BLOCKSIZE;
			}else if(p0 <= b->c2-BLOCKSIZE){
				/* last BLOCKSIZE can go */
				b->cache.n -= BLOCKSIZE;
				b->c2 -= BLOCKSIZE;
			}else{
				/* too hard; negate the cache and pick up next time */
				Strzero(&b->cache);
				b->c1 = b->c2 = 0;
			}
			return;
		}
	}else{
		Bflush(b);
		if(s->n >= BLOCKSIZE/2){
			b->cache.n = 0;
			b->c1 = b->c2 = 0;
			Dinsert(b->disc, s->s, s->n, p0);
		}else{
			int m;
			Posn minp;
			if(b->nrunes-p0 > BLOCKSIZE/2)
				m = BLOCKSIZE/2;
			else
				m = b->nrunes-p0;
			minp = p0-BLOCKSIZE/2;
			if(minp < 0)
				minp = 0;
			m += p0-minp;
			Strinsure(&b->cache, m);
			if(Dread(b->disc, b->cache.s, m, minp)!=m)
				panic("Bread");
			b->cache.n = m;
			b->c1 = minp;
			b->c2 = minp+m;
			Strinsert(&b->cache, s, p0-b->c1);
			b->dirty = TRUE;
		}
	}
	b->nrunes += s->n;
}

void
Bdelete(Buffer *b, Posn p1, Posn p2)
{
	if(p1<0 || p2<0)
		panic("Bdelete p<0");
	if(b->c2>b->disc->nrunes || b->c1>b->disc->nrunes)
		panic("bdelete cache");
	if(p1 == p2)
		return;
	if(incache(b, p1, p2)){
		Strdelete(&b->cache, p1-b->c1, p2-b->c1);
		b->dirty = TRUE;
	}else{
		Bflush(b);
		Ddelete(b->disc, p1, p2);
		b->cache.n = 0;
		b->c1 = b->c2 = 0;
	}
	b->nrunes -= p2-p1;
}

void
Bflush(Buffer *b)
{
	if(b->dirty){
		Dreplace(b->disc, b->c1, b->c2, b->cache.s, b->cache.n);
		b->c2 = b->c1+b->cache.n;
		b->dirty = FALSE;
		if(b->nrunes != b->disc->nrunes)
			panic("Bflush");
	}
}

void
Bclean(Buffer *b)
{
	if(b->dirty){
		Bflush(b);
		b->c1 = b->c2 = 0;
		Strzero(&b->cache);
	}
}

/*int hits, misses; /**/

int
incache(Buffer *b, Posn p1, Posn p2)
{
	/*if(b->c1<=p1 && p2<=b->c1+b->cache.n)hits++; else misses++;/**/
	return b->c1<=p1 && p2<=b->c1+b->cache.n;
}
