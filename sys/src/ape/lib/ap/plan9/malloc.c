#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define HOWMANY(x, y)	(((x)+((y)-1)) / (y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y)) * (y))

enum
{
	MAGIC		= 0xbada110c,
#ifdef _BITS64
	MAX2SIZE	= 63,
#else
	MAX2SIZE	= 32,
#endif
	CUTOFF		= 12,
};

typedef struct Bucket Bucket;
struct Bucket
{
	int	size;		/* lg2(actual size) */
	int	magic;
	Bucket	*next;
#ifndef _BITS64
	int	pad;
#endif
	char	data[1];
};

typedef struct Arena Arena;
struct Arena
{
	Bucket	*btab[MAX2SIZE];
};
static Arena arena;

#define datoff		((uintptr_t)((Bucket*)0)->data)
#define nil		((void*)0)

extern	void	*sbrk(uintptr_t);

static unsigned long long
nextpow2(unsigned long long n)
{
	int pow;

	for(pow = 1; pow < MAX2SIZE; pow++)
		if(n <= (1ULL << pow))
			break;
	return pow;
}

/*
 * not sure why this helps, but it prevents poolfreel from
 * dying later after large allocations, at least on 64-bit systems.
 */
static int firstalloc = 1;

#define initialalloc()	if (firstalloc) { firstalloc = 0; malloc(8); }

void*
malloc(size_t size)
{
	int pow, n;
	Bucket *bp, *nbp;

	initialalloc();
	pow = nextpow2(size);
	if (pow == 0)
		return nil;

	/* Allocate off this list, if any available */
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;

		if(bp->magic != 0)
			abort();

		bp->magic = MAGIC;
		return bp->data;
	}

	/* quantum of allocation */
	size = ROUNDUP(sizeof(Bucket) + (1ULL << pow), sizeof(long long));
	if(pow < CUTOFF) {
		n = (CUTOFF-pow) + 2;
		bp = sbrk(size*n);	/* allocate small array for list */
		if((intptr_t)bp == -1)
			return nil;

		nbp = (Bucket*)((uintptr_t)bp + size);
		arena.btab[pow] = nbp;	/* list skips first quantum (bp) */
		for(n -= 2; n > 0; n--) {
			nbp->size = pow;
			nbp = nbp->next = (Bucket*)((uintptr_t)nbp + size);
		}
		nbp->size = pow;
	} else {
		bp = sbrk(size);
		if((intptr_t)bp == -1)
			return nil;
	}
	bp->size = pow;
	bp->magic = MAGIC;
	return bp->data;
}

void
free(void *ptr)
{
	Bucket *bp, **l;

	if(ptr == nil)
		return;

	/* Find the start of the structure */
	bp = (Bucket*)((uintptr_t)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	bp->magic = 0;
	l = &arena.btab[bp->size];
	bp->next = *l;
	*l = bp;
}

void*
realloc(void *ptr, size_t n)
{
	void *new;
	uintptr_t osize;
	Bucket *bp;

	if(ptr == nil)
		return malloc(n);

	/* Find the start of the structure */
	bp = (Bucket*)((uintptr_t)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	/* enough space in this bucket? */
	osize = 1ULL << bp->size;
	if(osize >= n)
		return ptr;

	new = malloc(n);
	if(new == nil)
		return nil;

	memmove(new, ptr, osize);
	free(ptr);

	return new;
}
