#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
/* compile-time features
   IALLOC use all blocks given to ialloc, otherwise ignore unordered blocks
   MSTATS enable statistics 
   debug enable assertion checking
   longdebug full arena checks at every transaction
*/
#ifdef longdebug
#define debug 1
#endif
#ifdef debug
#include <stdio.h>
#define ASSERT(p) if(!(p))botch(__LINE__);else
static
botch(n)
{
	fprintf(stderr,"bad arena, malloc.c line %d\n" ,n);
	abort();
}
#else
#define ASSERT(p)
#endif

/*	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block
 *	blocks are exact number of words long 
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
*/
#define INT int
#define ALIGN int
#define NALIGN 1
#define WORD sizeof(union store)
#define BLOCK 4096
#define BUSY 1
#define testbusy(p) ((INT)(p)&BUSY)
#define setbusy(p) (union store *)((INT)(p)|BUSY)
#define clearbusy(p) (union store *)((INT)(p)&~BUSY)

union store {
	      union store *ptr;
	      ALIGN dummy[NALIGN];
	      int calloc;	/*calloc clears an array of integers*/
};

/* alloca should have type union store.
 * The funny business gets it initialized without complaint*/
#define addr(a) (union store*)&a
static	char *alloca = (char*)&alloca + BUSY;	/* initial arena */
static	union store *allocb = addr(alloca);	/*arena base*/
static	union store *allocc = addr(alloca);	/*all prev blocks known busy*/
static	union store *allocp = addr(alloca);	/*search ptr*/
static	union store *alloct = addr(alloca);	/*top cell*/
static	union store *allocx;	/*for benefit of realloc*/
extern	char *sbrk();

/* a cache list of frequently-used sizes is maintained. From each
 * cache entry hangs a chain of available blocks 
 * malloc(0) shuts off caching (to keep freed data clean)
*/
#define CACHEMAX 50	/* largest block to be cached (in words) */
#define CACHESIZ 13	/* number of entries (prime) */

static union store *cache[CACHESIZ];
static char *stdmalloc(unsigned);
static draincache(void);
void ialloc(char *, unsigned);
static stdfree(union store *);

#ifdef MSTATS
#define Mstats(e) e
static long nmalloc, nrealloc, nfree;	/* call statistics */
static long walloc, wfree;		/* space statistics */
static long chit, ccoll, cdrain, cavail;	/* cache statistics */
#else
#define Mstats(e)
#endif

void *
malloc(long nbytes)
{
	register union store *p;
	register nw;
	register union store **cp;

	nw = (nbytes+WORD+WORD-1)/WORD;
	Mstats((nmalloc++, walloc += nw));
	if(nw<CACHEMAX && nw>=2) {
		cp = &cache[nw%CACHESIZ];
		p = *cp;
		if(p && nw==clearbusy(p->ptr)-p) {
			ASSERT(testbusy(p->ptr));
			*cp = (++p)->ptr;
			Mstats((chit++, cavail--));
			return (char*)p;
		}
	}
	return stdmalloc(nw);
}

static char *
stdmalloc(unsigned nw)
{
	register union store *p, *q;
	register temp;
	ASSERT(allock(allocp));
	for(; ; ) {	/* done at most thrice */
		p = allocp;
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {
				allocp = p;
				while(!testbusy((q=p->ptr)->ptr)) {
					ASSERT(q>p);
					p->ptr = q->ptr;
				}
				if(q>=p+nw && p+nw>=p)
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if(p <= q) {
				ASSERT(p==allocb&&q==alloct);
				if(++temp>1)
					break;
				ASSERT(allock(allocc));
				p = allocc;
			}
		}
		p = (union store *)sbrk(0);
		temp = ((nw+BLOCK/WORD)/(BLOCK/WORD))*(BLOCK/WORD);
		do {
			if(p+temp > p) {	/*check wrap*/
				q = (union store *)sbrk(temp*WORD);
				if((INT)q != -1)
					goto mextend;
			}
			temp -= (temp-nw)/2;
		} while(temp-nw > 1);
		if(draincache())
			continue;
		return(NULL);
mextend:
		draincache();
		ialloc((char *)q, (unsigned)temp*WORD);
	}
found:
	allocp += nw;
	if(q>allocp) {
		allocx = allocp->ptr;
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
	if(p<=allocc) {
		ASSERT(p==allocc);
		while(testbusy(allocc->ptr)
		     && (q=clearbusy(allocc->ptr))>allocc)
			allocc = q;
	}
	return((char *)(p+1));
}
void
free(void *ap)
{
	register union store *p = (union store *)ap, *q;
	register nw;
	register union store **cp;

	if(p==NULL)
		return;
	--p;
	ASSERT(allock(p));
	ASSERT(testbusy(p->ptr));
	ASSERT(!cached(p));
	nw = clearbusy(p->ptr) - p;
	Mstats((nfree++, wfree += nw));
	ASSERT(nw>0);
	if(nw<CACHEMAX && nw>=2) {
		cp = &cache[nw%CACHESIZ];
		q = *cp;
		if(!q || nw==clearbusy(q->ptr)-q) {
			p[1].ptr = q;
			*cp = p;
			Mstats(cavail++);
			return;
		} else Mstats(q && ccoll++);
	}
	stdfree(p+1);
}

/*	freeing strategy tuned for LIFO allocation
*/
static
stdfree(union store *p)
{
	allocp = --p;
	if(p < allocc)
		allocc = p;
	ASSERT(allock(allocp));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp);
}

static
draincache(void)
{
	register union store **cp = cache+CACHESIZ;
	register union store *q;
	int anyfreed = 0;
	while(--cp>=cache) {
		while(q = *cp) {
			ASSERT(testbusy(q->ptr));
			ASSERT((clearbusy(q->ptr)-q)%CACHESIZ==cp-cache);
			ASSERT(q>=allocb&&q<=alloct);
			stdfree(++q);
			anyfreed++;
			*cp = q->ptr;
		}
	}
	Mstats((cdrain+=anyfreed, cavail=0));
	return anyfreed;
}

/* ialloc(q, nbytes) inserts a block that did not come
 * from malloc into the arena
 *
 * q points to new block
 * r points to last of new block
 * p points to last cell of arena before new block
 * s points to first cell of arena after new block
*/
void
ialloc(char *qq, unsigned nbytes)
{
	register union store *p, *q, *s;
	union store *r;

	q = (union store *)qq;
	r = q + (nbytes/WORD) - 1;
	q->ptr = r;
	if(q > alloct) {
		p = alloct;
		s = allocb;
		alloct = r;
	} else {
#ifdef IALLOC
	/* useful only in small address spaces */
		for(p=allocb; ; p=s) {
			s = clearbusy(p->ptr);
			if(s==allocb)
				break;
			ASSERT(s>p);
			if(s>r) {
				if(p<q)
					break;
				else
					ASSERT(p>r);
			}
		}
		if(allocb > q)
			allocb = q;
		if(allocc > q)
			allocc = q;
		allocp = allocc;
#else
		return;
#endif
	}
	p->ptr = q==p+1? q: setbusy(q);
	r->ptr = s==r+1? s: setbusy(s);
	while(testbusy(allocc->ptr))
		allocc = clearbusy(allocc->ptr);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
*/

void *
realloc(void *pp, long nbytes)
{
	register union store *p = (union store *)pp;
	register union store *s, *t;
	register union store *q;
	register unsigned nw;
	unsigned onw;

	Mstats(nrealloc++);
	if(p==NULL)
		return malloc(nbytes);
	ASSERT(allock(p-1));
	if(testbusy(p[-1].ptr))
		stdfree(p);
	onw = p[-1].ptr - p;
	nw = (nbytes+WORD-1)/WORD;
	q = (union store *)stdmalloc(nw+1);
	if(q==NULL || q==p)
		return((char *)q);
	ASSERT(q<p||q>p[-1].ptr);
	if(nw<onw) {
		Mstats(wfree += onw-nw);
		onw = nw;
	} else Mstats(walloc += nw-onw);
	for(s=p, t=q; onw--!=0; )
		*t++ = *s++;
	ASSERT(clearbusy(q[-1].ptr)-q==nw);
	if(q<p && q+nw>=p)
		(q+(q+nw-p))->ptr = allocx;
	ASSERT(allock(q-1));
	return((char *)q);
}

#ifdef debug
static
allock(union store *q)
{
#ifdef longdebug
	register union store *p, *r;
	register union store **cp;
	int x, y;
	for(cp=cache+CACHESIZ; --cp>=cache; ) {
		if((p= *cp)==0)
			continue;
		x = clearbusy(p->ptr) - p;
		ASSERT(x%CACHESIZ==cp-cache);
		for( ; p; p = p[1].ptr) {
			ASSERT(testbusy(p->ptr));
			ASSERT(clearbusy(p->ptr)-p==x);
		}
	}
	x = 0, y = 0;
	p = allocb;
	for( ; (r=clearbusy(p->ptr)) > p; p=r) {
		if(p==allocc)
			y++;
		ASSERT(y||testbusy(p->ptr));
		if(p==q)
			x++;
	}
	ASSERT(r==allocb);
	ASSERT(x==1||p==q);
	ASSERT(y||p==allocc);
	return(1);
#else
	ASSERT((unsigned)q/WORD*WORD==(unsigned)q);
	ASSERT(q>=allocb&&q<=alloct);
#endif
}
#endif

mstats()
{
#ifdef MSTATS
#ifndef stderr
#include <stdio.h>
#endif
	fprintf(stderr, "Malloc statistics, including overhead bytes\n");
	fprintf(stderr, "Arena: bottom %ld, top %ld\n",
		(long)clearbusy(alloca), (long)alloct);
	fprintf(stderr, "Calls: malloc %ld, realloc %ld, free %ld\n",
		nmalloc, nrealloc, nfree);
	fprintf(stderr, "Bytes: allocated or extended %ld, ",
		walloc*WORD);
	fprintf(stderr, "freed or cut %ld\n", wfree*WORD);
	fprintf(stderr,"Cache: hits %ld, collisions %ld, discards %ld, avail %ld\n",
		chit, ccoll, cdrain, cavail);
#endif
}

#ifdef debug
cached(union store *p)
{
	union store *q = cache[(clearbusy(p->ptr)-p)%CACHESIZ];
	for( ; q; q=q[1].ptr)
		ASSERT(p!=q);
	return 0;
}
#endif
