#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "mothra.h"

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
	char	data[1];
};

typedef struct Arena Arena;
struct Arena
{
	int enter;
	int exit;
	Bucket	*btab[MAX2SIZE];	
};
static Arena arena;

#define datoff		((int)((Bucket*)0)->data)
#define nil		((void*)0)

static void*
nlmalloc(long size)
{
	ulong next;
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
		return  bp->data;
	}
	size = sizeof(Bucket)+(1<<pow);
	size += 7;
	size &= ~7;

	if(pow < CUTOFF) {
		n = (CUTOFF-pow)+2;
		bp = sbrk(size*n);
		if((int)bp < 0)
			return nil;

		next = (ulong)bp+size;
		nbp = (Bucket*)next;
		arena.btab[pow] = nbp;
		for(n -= 2; n; n--) {
			next = (ulong)nbp+size;
			nbp->next = (Bucket*)next;
			nbp->size = pow;
			nbp = nbp->next;
		}
		nbp->size = pow;
	}
	else {
		bp = sbrk(size);
		if((int)bp < 0)
			return nil;
	}
		
	bp->size = pow;
	bp->magic = MAGIC;

	return bp->data;
}
void mlock(void){
	static int first = 1;

	if(first){
		first=0;
		switch(fork()){
		case -1:
			fprint(2, "malloc: can't make lock process!\n");
			exits("no lock");
		case 0:
			for(;;){
				rendezvous((ulong)&arena.enter, 1);
				rendezvous((ulong)&arena.exit, 0);
			}
		}
	}
	do;while(rendezvous((ulong)&arena.enter, 0)!=1);
}
void munlock(void){
	rendezvous((ulong)&arena.exit, 0);
}
void *
malloc(long size)
{
	void *v;
	mlock();
	v=nlmalloc(size);
	munlock();
	return v;
}
void*
calloc(long n, long size)
{
	void *p;

	n *= size;
	p = malloc(n);
	if(p)
		memset(p, 0, n);

	return p;
}

static void
nlfree(void *ptr)
{
	Bucket *bp, **l;

	if(ptr == nil)
		return;

	/* Find the start of the structure */
	bp = (Bucket*)((uint)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	bp->magic = 0;
	l = &arena.btab[bp->size];
	bp->next = *l;
	*l = bp;
}

void
free(void *ptr)
{
	mlock();
	nlfree(ptr);
	munlock();
}

static void*
nlrealloc(void *ptr, long n)
{
	void *new;
	uint osize;
	Bucket *bp;

	if(ptr == nil)
		return nlmalloc(n);

	/* Find the start of the structure */
	bp = (Bucket*)((uint)ptr - datoff);

	if(bp->magic != MAGIC)
		abort();

	/* enough space in this bucket */
	osize = 1<<bp->size;
	if(osize >= n && n > osize/2)
		return ptr;

	new = nlmalloc(n);
	if(new == nil)
		return nil;

	memmove(new, ptr, osize < n ? osize : n);
	nlfree(ptr);

	return new;
}

void *
realloc(void *ptr, long size)
{
	void *v;
	mlock();
	v=nlrealloc(ptr, size);
	munlock();
	return v;
}
