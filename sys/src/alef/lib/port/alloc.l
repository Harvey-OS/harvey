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
	Bucket*	next;
	byte	data[1];
};

aggr Arena
{
	Lock;
	Bucket*	btab;	
};

intern Arena arena[MAX2SIZE];

#define datoff		((int)((Bucket*)0)->data)

void*
ALEFalloc(uint size, int clr)
{
	uint next;
	int pow, n;
	Arena *a;
	Bucket *bp, *nbp;

	for(pow = 1; pow < MAX2SIZE; pow++) {
		if(size <= (1<<pow))
			goto good;
	}

	check 0, "no memory";

good:
	/* Allocate off this list */
	a = &arena[pow];
	a->lock();
	bp = a->btab;
	if(bp) {
		a->btab = bp->next;
		a->unlock();

		check bp->magic == 0, "corrupted arena";
		bp->magic = MAGIC;

		if(clr)
			memset(bp->data, 0,  size);

		return  bp->data;
	}

	size = sizeof(Bucket)+(1<<pow);
	size += 3;
	size &= ~3;

	rescue{
		a->unlock();
		check 0, "no memory";
	};

	if(pow < CUTOFF) {
		n = (CUTOFF-pow)+2;
		bp = sbrk(size*n);
		if(bp == (void*)-1)
			raise;

		nbp = bp;
		while(--n) {
			next = (uint)nbp+size;
			nbp = (Bucket*)next;
			nbp->size = pow;
			nbp->next = a->btab;
			a->btab = nbp;
		}
	}
	else {
		bp = sbrk(size);
		if(bp == (void*)-1)
			raise;
	}
	a->unlock();
		
	bp->size = pow;
	bp->magic = MAGIC;

	return bp->data;
}

void
free(void *ptr)
{
	Arena *a;
	Bucket *bp;

	if(ptr == nil)
		return;

	/* Find the start of the structure */
	bp = (Bucket*)((uint)ptr - datoff);

	if(bp->magic != MAGIC)
		check 0, "corrupted arena";

	bp->magic = 0;
	a = &arena[bp->size];
	a->lock();
	bp->next = a->btab;
	a->btab = bp;
	a->unlock();
}

void*
realloc(void *ptr, uint n)
{
	void *new;
	uint osize;
	Bucket *bp;

	if(ptr == nil)
		return ALEFalloc(n, 1);

	/* Find the start of the structure */
	bp = (Bucket*)((uint)ptr - datoff);

	if(bp->magic != MAGIC)
		check 0, "corrupted arena";

	/* enough space in this bucket */
	osize = 1<<bp->size;
	if(osize >= n)
		return ptr;

	new = ALEFalloc(n, 1);

	memmove(new, ptr, osize);
	free(ptr);

	return new;
}
