#include <alef.h>

enum
{
	MAGIC		= 0xbada110c,
	MAX2SIZE	= 32,
	CUTOFF		= 12,
};

aggr Bucket
{
	int	size;
	int	magic;
	Bucket	*next;
	char	data[1];
};

aggr Arena
{
	Lock;
	Bucket	*btab[MAX2SIZE];	
};

intern Arena arena;

#define datoff		((int)&((Bucket*)0)->data)

void*
malloc(uint size)
{
	uint next;
	int pow, n;
	Bucket *bp, *nbp;

	for(pow = 1; pow < MAX2SIZE; pow++) {
		if(size <= (1<<pow))
			goto good;
	}

	return nil;
good:
	/* Allocate off this list */
	arena.lock();
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;
		arena.unlock();

		if(bp->magic != 0)
			abort();

		bp->magic = MAGIC;

		memset(bp->data, 0,  size);
		return  bp->data;
	}

	size = sizeof(Bucket)+(1<<pow);
	size += 3;
	size &= ~3;

	rescue{
		arena.unlock();
		return nil;
	};

	if(pow < CUTOFF) {
		n = (CUTOFF-pow)+2;
		bp = sbrk(size*n);
		if((int)bp < 0)
			raise;

		next = (uint)bp+size;
		nbp = (Bucket*)next;
		arena.btab[pow] = nbp;
		for(n -= 2; n; n--) {
			next = (uint)nbp+size;
			nbp->next = (Bucket*)next;
			nbp->size = pow;
			nbp = nbp->next;
		}
		nbp->size = pow;
	}
	else {
		bp = sbrk(size);
		if((int)bp < 0)
			raise;
	}
	arena.unlock();
		
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
	bp = (Bucket*)((uint)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	bp->magic = 0;
	arena.lock();
	l = &arena.btab[bp->size];
	bp->next = *l;
	*l = bp;
	arena.unlock();
}

void*
realloc(void *ptr, uint n)
{
	void *new;
	uint osize;
	Bucket *bp;

	if(ptr == nil)
		return malloc(n);

	/* Find the start of the structure */
	bp = (Bucket*)((uint)ptr - datoff);

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
