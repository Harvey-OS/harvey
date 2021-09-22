/*
 * malloc
 *
 *	Uses Quickfit (see SIGPLAN Notices October 1988)
 *	with allocator from Kernighan & Ritchie
 */

#include <u.h>
#include <libc.h>

enum{
	DEBUG=	0,
	DEBUGX=	1,
};

typedef double Align;
typedef union Header Header;

union Header {
	struct {
		Header*	next;
		uintptr	size;
	} s;
	Align	al;
};
#define	NUNITS(n) (((n)+sizeof(Header)-1)/sizeof(Header) + 1)
#define	BRKSIZE	(8192/sizeof(Header))

#define	NQUICK	((512/sizeof(Header))+1)

typedef struct Alloclist Alloclist;
struct Alloclist {
	Lock	lk;
	Header	h;
};

static	Header	*quicklist[NQUICK+1];
static	Header	misclist;
static	Header	*rover;
static	unsigned tailsize;
static	Header	*tailptr;
static	Header	checkval;

static	int	morecore(uintptr);
static	void	badalloc(void*);

static	int	malloctrace = 0;

void	qmallocdump(void);
void	qfree(void*);

static Lock mainlock;
#define	LOCK	lock(&mainlock)
#define	UNLOCK	unlock(&mainlock)

#define	tailalloc(p,n) ((p)=tailptr, tailsize -= (n), tailptr+=(n),\
			 (p)->s.size=(n), (p)->s.next = &checkval)

void *
qmalloc(uintptr nbytes)
{
	Header *p, *q;
	uint nunits;

	if(DEBUGX && malloctrace>1)
		fprint(2, "malloc %lud\n", nbytes);
	LOCK;
	nunits = NUNITS(nbytes);
	if(nunits <= NQUICK &&
	    (p = quicklist[nunits]) != nil) {
		quicklist[nunits] = p->s.next;
		p->s.next = &checkval;
		UNLOCK;
		return p+1;
	}
	if(nunits > tailsize) {
		/* hard way */
		if((q = rover) != nil)
			do {
				p = q->s.next;
				if(p->s.size >= nunits) {
					if(malloctrace)
						write(2, "%", 1);
					if(p->s.size > nunits) {
						p->s.size -= nunits;
						p += p->s.size;
						p->s.size = nunits;
					} else
						q->s.next = p->s.next;
					p->s.next = &checkval;
					rover = q;
					UNLOCK;
					return (char *)(p+1);
				}
			} while((q = p) != rover);
		if(!morecore(nunits)){
			UNLOCK;
			return nil;
		}
	}
	if(malloctrace)
		write(2, "+", 1);
	tailalloc(p, nunits);
	UNLOCK;
	return p+1;
}

static int
morecore(uintptr nunits)
{
	char *cp;

	if(malloctrace) {
		write(2, ".", 1);
		if(DEBUG){
			static int done;
			if(done == 0) {
				done = 1;
				fprint(2, "\n");
				atexit(qmallocdump);
			}
		}
	}
	if(nunits < BRKSIZE)
		nunits = BRKSIZE;
	cp = sbrk(nunits * sizeof(Header));
	if(cp == (char *)~0)
		return 0;
	if(tailsize != 0) {
		Header *p = tailptr;

		if(p+tailsize == (Header *)cp) {
			/* new block follows tail: combine */
			tailsize += nunits;
			return 1;
		}
		/* there is a gap: must free existing tail space */
		p->s.size = tailsize;
		p->s.next = &checkval;
		tailsize = 0;
		tailptr = nil;
		qfree(p+1);
	}
	tailptr = (Header*)cp;
	tailsize = nunits;
	return 1;
}

void
qfree(void *ap)
{
	Header *p, *q;
	uintptr nunits;

	if(ap == nil)
		return;
	p = (Header*)ap - 1;
	if((nunits = p->s.size) == 0 || p->s.next != &checkval)
		badalloc(ap);
	if(DEBUG && malloctrace>1)
		fprint(2, "free %d at %p\n", nunits, p);
	if(tailptr != nil && p+nunits == tailptr) {
		/* block before tail */
		tailptr = p;
		tailsize += nunits;
		return;
	}
	if(nunits <= NQUICK) {
		Header **fp;
		fp = &quicklist[nunits];
		p->s.next = *fp;
		*fp = p;
		return;
	}
	if((q = rover) == nil) {
		q = &misclist;
		q->s.size = 0;
		q->s.next = q;
	}
	for(; !(p > q && p < q->s.next); q = q->s.next)
		if(q >= q->s.next && (p > q || p < q->s.next))
			break;
	if(p+p->s.size == q->s.next) {
		p->s.size += q->s.next->s.size;
		p->s.next = q->s.next->s.next;
	} else
		p->s.next = q->s.next;
	if(q+q->s.size == p) {
		q->s.size += p->s.size;
		q->s.next = p->s.next;
	} else
		q->s.next = p;
	rover = q;
}

void*
qrealloc(void *ap, uintptr nbytes)
{
	char *dp;
	uintptr nunits, osize;
	Header *p;

	if((p = ap) == nil)
		return qmalloc(nbytes);
	if(nbytes == 0) {
		free(p);
		return nil;
	}
	if((osize = (--p)->s.size) == 0 || p->s.next != &checkval)
		badalloc(ap);
	nunits = NUNITS(nbytes);
	if(p->s.size == nunits)
		return ap;
	if(DEBUG && malloctrace>1)
		fprint(2, "tail %p %ud %p %ud\n", tailptr, tailsize, p, p->s.size);
	if((dp = qmalloc(nbytes)) != nil && dp != ap) {
		osize = (osize-1)*sizeof(Header);
		if(nbytes < osize)
			osize = nbytes;
		memmove(dp, ap, osize);
	}
	free(ap);
	return dp;
}

static void
badalloc(void *ap)
{
	USED(ap);
	write(2, "malloc: corrupt allocation arena\n", 34);
	abort();
}

/*
 * special function for use by valloc
 */
void
_free_mem(void *cp, uintptr nbytes)
{
	uintptr nunits;
	Header *p;

	p = cp;
	nunits = nbytes/sizeof(Header);
	if(nunits > 1) {
		p->s.size = nunits;
		p->s.next = &checkval;
		qfree(p+1);
	}

}

void
qmallocdump(void)
{
	int n;
	Header *q;

	for(n=0; n<=NQUICK; n++) {
		for(q = quicklist[n]; q != nil; q = q->s.next)
			fprint(2, "q%d\t%p\t%ud\n", n, q, q->s.size);
	}
	if((q = rover) != nil)
		do {
			fprint(2, "m%d\t%p\t%ud\n", n, q, q->s.size);
		} while((q = q->s.next) != rover);
}

void*
malloc(uintptr n)
{
	void *p;

	p = qmalloc(n);
	if(p != nil)
		memset(p, 0, n);
	return p;
}

void
free(void *p)
{
	LOCK;
	qfree(p);
	UNLOCK;
}

void*
calloc(uintptr m, uintptr n)
{
	void *p;

	n *= m;
	p = qmalloc(n);
	memset(p, 0, n);
	return p;
}

void
setmalloctag(void*, uintptr)
{
}
