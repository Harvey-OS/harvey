#include <u.h>
#include <libc.h>
#include "dns.h"

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

static int parallel;
static int parfd[2];
static char token[1];
#define LOCK if(parallel) read(parfd[0], token, 1)
#define UNLOCK if(parallel) write(parfd[1], token, 1)

#define datoff		((int)((Bucket*)0)->data)
#define nil		((void*)0)

/*
 *  turn on parallel alloc
 */
void
paralloc(void)
{
	parallel = 1;
	if(pipe(parfd) < 0)
		abort();
	UNLOCK;
}

void*
malloc(long size)
{
	int pow;
	Bucket *bp;

	LOCK;
	for(pow = 1; pow < MAX2SIZE; pow++) {
		if(size <= (1<<pow))
			goto good;
	}
	UNLOCK;

	return nil;
good:
	/* Allocate off this list */
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;
		UNLOCK;

		if(bp->magic != 0)
			abort();

		bp->magic = MAGIC;

		memset(bp->data, 0,  size);
		return  bp->data;
	}
	size = sizeof(Bucket)+(1<<pow);
	bp = sbrk(size);
	UNLOCK;
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
	LOCK;
	l = &arena.btab[bp->size];
	bp->next = *l;
	*l = bp;
	UNLOCK;
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
