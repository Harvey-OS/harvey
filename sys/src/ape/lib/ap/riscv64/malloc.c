/*
 * V7 unix malloc, adapted to the modern world
 */

/*
 *	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of)
 *	the next following block
 *	blocks are exact number of words long
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
 */

#include <u.h>
#include <libc.h>

#ifdef debug
#define ASSERT(p) if(!(p))botch("p");else

void
botch(char *s)
{
	unlock(&malloclck);
	print("assertion botched: %s\n", s);
	abort();
}
#else
#define ASSERT(p)
#endif

#define INT	uintptr
#define ALIGN	vlong
#define NALIGN	1
#define WORD	sizeof(union store)
#define BLOCK	1024			/* a multiple of WORD */

#define BUSY 1
#define NULL 0

#define testbusy(p)	((INT)(p)&BUSY)
#define setbusy(p)	(union store *)((INT)(p)|BUSY)
#define clearbusy(p)	(union store *)((INT)(p)&~BUSY)

typedef union store Store;
union store {
	Store	*ptr;
	ALIGN	dummy[NALIGN];
	int	calloc;		/*calloc clears an array of integers*/
};

static Store allocs[2];	/*initial arena*/
static Store *allocp;	/*search ptr*/
static Store *alloct;	/*arena top*/
static Store *allocx;	/*for benefit of realloc*/
static Lock malloclck;

void *
malloc(uintptr nbytes)
{
	uintptr nw, temp;
	Store *p, *q;

	lock(&malloclck);
	if (allocs[0].ptr == 0) {	/* first time */
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
	}
	nw = (nbytes + WORD + WORD - 1) / WORD;
	ASSERT(allocp >= allocs && allocp <= alloct);
	ASSERT(allock());
	for (p = allocp; ; ) {
		for (temp = 0; ; ) {
			if (!testbusy(p->ptr)) {
				while (!testbusy((q = p->ptr)->ptr)) {
					ASSERT(q > p && q < alloct);
					p->ptr = q->ptr;
				}
				if (q >= p + nw && p + nw >= p)
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if (p > q) {
				ASSERT(p <= alloct);
			} else if (q != alloct || p != allocs) {
				ASSERT(q == alloct && p == allocs);
				unlock(&malloclck);
				return NULL;
			} else if (++temp > 1)
				break;
		}
		temp = ((nw + BLOCK/WORD) / (BLOCK/WORD)) * (BLOCK/WORD);
		q = (Store *)sbrk(0);
		if (q + temp < q) {
			unlock(&malloclck);
			return NULL;
		}
		q = (Store *)sbrk(temp * WORD);
		if ((INT)q == -1) {
			unlock(&malloclck);
			return NULL;
		}
		ASSERT(q > alloct);
		alloct->ptr = q;
		if (q != alloct + 1)
			alloct->ptr = setbusy(alloct->ptr);
		alloct = q->ptr = q + temp - 1;
		alloct->ptr = setbusy(allocs);
	}
found:
	allocp = p + nw;
	ASSERT(allocp <= alloct);
	if (q > allocp) {
		allocx = allocp->ptr;
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
	unlock(&malloclck);
	return p + 1;
}

void *
mallocz(uintptr size, int clr)
{
	void *p;

	p = malloc(size);
	if (p && clr)
		memset(p, 0, size);
	return p;
}

/*
 *	freeing strategy tuned for LIFO allocation
 */
void
free(void *ap)
{
	Store *p = (Store *)ap;

	if (p == 0)
		return;
	lock(&malloclck);
	ASSERT(p > clearbusy(allocs[1].ptr) && p <= alloct);
	ASSERT(allock());
	allocp = --p;
	ASSERT(testbusy(p->ptr));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
	unlock(&malloclck);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
 */

void *
realloc(void *ap, uintptr nbytes)
{
	Store *p, *q;
	uintptr nw, onw;

	p = ap;
	if (nbytes == 0) {
		free(p);
		return nil;
	}
	if (p == nil)
		return malloc(nbytes);

	if (testbusy(p[-1].ptr))
		free(p);

	onw = p[-1].ptr - p;
	q = (Store *)malloc(nbytes);
	if (q == NULL || q == p)
		return q;

	lock(&malloclck);
	memmove(q, p, nbytes);
	nw = (nbytes + WORD - 1) / WORD;
	if (q < p && q + nw >= p)
		(q + (q + nw - p))->ptr = allocx;
	unlock(&malloclck);
	return q;
}

#ifdef debug
void
allock(void)
{
#ifdef longdebug
	Store *p;
	int	x;

	x = 0;
	for (p = &allocs[0]; clearbusy(p->ptr) > p; p = clearbusy(p->ptr))
		if (p == allocp)
			x++;
	ASSERT(p == alloct);
	return x == 1 | p == allocp;
#else
	return 1;
#endif
}
#endif

void*
calloc(uintptr n, uintptr szelem)
{
	void *v;

	if (v = mallocz(n * szelem, 1))
		setmalloctag(v, getcallerpc(&n));
	return v;
}

void
setmalloctag(void *, uintptr)
{
}

void
setrealloctag(void *, uintptr)
{
}

uintptr
getmalloctag(void *)
{
	return ~0;
}

uintptr
getrealloctag(void *)
{
	return ~0;
}
