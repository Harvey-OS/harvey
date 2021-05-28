#include <stdlib.h>
#include <string.h>

typedef unsigned int	uint;
typedef long long intptr_t;

enum
{
	MAGIC		= 0xbada110c,
	MAX2SIZE	= 32,
	CUTOFF		= 12,
};

typedef struct Bucket Bucket;
struct Bucket
{
	int	size;
	int	magic;
	Bucket	*next;
	int pad;
};

typedef struct Arena Arena;
struct Arena
{
	Bucket	*btab[MAX2SIZE];	
};
static Arena arena;

#define datoff	((sizeof(struct Bucket)+7)&~7)
#define nil		((void*)0)

extern	void	*sbrk(unsigned long);

void*
malloc(size_t size)
{
	intptr_t next;
	int pow, n;
	Bucket *bp, *nbp;

	for(pow = 1; pow < MAX2SIZE; pow++) {
		if(size <= (1<<pow))
			goto good;
	}

	return nil;
good:
	/* Allocate off this list */
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;

		if(bp->magic != 0)
			abort();

		bp->magic = MAGIC;
		return  bp + 1;
	}
	size = sizeof(Bucket)+(1<<pow);
	size += 7;
	size &= ~7;

	if(pow < CUTOFF) {
		n = (CUTOFF-pow)+2;
		bp = sbrk(size*n);
		if((intptr_t)bp == -1)
			return nil;

		next = (intptr_t)bp+size;
		nbp = (Bucket*)next;
		arena.btab[pow] = nbp;
		for(n -= 2; n; n--) {
			next = (intptr_t)nbp+size;
			nbp->next = (Bucket*)next;
			nbp->size = pow;
			nbp = nbp->next;
		}
		nbp->size = pow;
	}
	else {
		bp = sbrk(size);
		if((intptr_t)bp == -1)
			return nil;
	}
		
	bp->size = pow;
	bp->magic = MAGIC;

	return bp + 1;
}

void
free(void *ptr)
{
	Bucket *bp, **l;

	if(ptr == nil)
		return;

	/* Find the start of the structure */
	bp = (Bucket*)((intptr_t)ptr - datoff);

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
	uint osize;
	Bucket *bp;

	if(ptr == nil)
		return malloc(n);

	/* Find the start of the structure */
	bp = (Bucket*)((intptr_t)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	/* enough space in this bucket */
	osize = 1<<bp->size;
	if(osize >= n)
		return ptr;

	new = malloc(n);
	if(new == nil)
		return nil;

	memmove(new, ptr, osize);
	free(ptr);

	return new;
}
