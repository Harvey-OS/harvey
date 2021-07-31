/*
 * fs kernel compatibility hacks for drivers from the cpu/terminal kernel
 */
#include "all.h"
#include "io.h"
#include "mem.h"
#include "../ip/ip.h"	/* for Ether */
#include "etherif.h"	/* for Ether */
#include "compat.h"

enum {
	VectorPIC	= 24,		/* external [A]PIC interrupts */
};

void
free(void *p)		/* there's a struct member named "free".  sigh. */
{
	USED(p);
}

void *
mallocz(ulong sz, int clr)
{
	void *p = malloc(sz);

	if (clr && p != nil)
		memset(p, '\0', sz);
	return p;
}

void
freeb(Block *b)
{
	mbfree(b);
}

/*
 *  free a list of blocks
 */
void
freeblist(Block *b)
{
	Block *next;

	for(; b != 0; b = next){
		next = b->next;
		b->next = 0;
		freeb(b);
	}
}

int
readstr(vlong, void *, int, char *)
{
	return 0;
}

void
addethercard(char *, int (*)(struct Ether *))
{
}

void
kproc(char *name, void (*f)(void), void *arg)
{
	userinit(f, arg, strdup(name));
}

void
intrenable(int irq, void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	setvec(irq+VectorPIC, f, a);
	USED(tbdf, name);
}

int
intrdisable(int irq, void (*f)(Ureg *, void *), void *a, int tbdf, char *name)
{
	USED(irq, f, a, tbdf, name);
	return -1;
}

/*
 * Atomically replace *p with copy of s
 */
void
kstrdup(char **p, char *s)
{
	int n;
	char *t, *prev;
	static Lock l;

	n = strlen(s)+1;
	/* if it's a user, we can wait for memory; if not, something's very wrong */
	if(0 && u){
		t = /* s */malloc(n);
		// setmalloctag(t, getcallerpc(&p));
	}else{
		t = malloc(n);
		if(t == nil)
			panic("kstrdup: no memory");
	}
	memmove(t, s, n);
	prev = *p;
	*p = t;
	free(prev);
}
