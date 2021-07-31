#include <u.h>
#include <libc.h>
#include "lock.h"

/*
 *  mutex malloc
 */

enum
{
	MAGIC		= 0xbada110c,
	MAX2SIZE	= 20
};

typedef struct Bucket Bucket;
struct Bucket
{
	int	size;
	int	magic;
	Bucket	*next;
	char	data[1];
};

typedef struct Arena Arena;
struct Arena
{
	Bucket	*btab[MAX2SIZE];	
};
static Arena arena;

static Lock mlock;

#define datoff		((int)((Bucket*)0)->data)
#define nil		((void*)0)

void
paralloc(void)
{
	lockinit(&mlock);
}

void*
malloc(long size)
{
	int pow;
	Bucket *bp;

	lock(&mlock);
	for(pow = 1; pow < MAX2SIZE; pow++) {
		if(size <= (1<<pow))
			goto good;
	}
	unlock(&mlock);

	return nil;
good:
	/* Allocate off this list */
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;
		unlock(&mlock);

		if(bp->magic != 0)
			abort();

		bp->magic = MAGIC;

		memset(bp->data, 0,  size);
		return  bp->data;
	}
	size = sizeof(Bucket)+(1<<pow);
	bp = sbrk(size);
	unlock(&mlock);
	if((int)bp < 0)
		return nil;

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
	lock(&mlock);
	l = &arena.btab[bp->size];
	bp->next = *l;
	*l = bp;
	unlock(&mlock);
}

void*
realloc(void *ptr, long n)
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
