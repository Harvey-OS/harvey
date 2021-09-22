/*
 * malloc
 *
 *	Uses Quickfit (see SIGPLAN Notices October 1988)
 *	with allocator from Kernighan & Ritchie
 *
 * allocates from KSEG0 space (sys->vmunused to sys->vmend).
 *
 * This is a placeholder.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	Morestats = 0,		/* flag */
};

typedef double Align;
typedef union Header Header;
typedef struct Qlist Qlist;

union Header {
	struct {
		Header*	next;
		uintptr	size;
	} s;
	Align	al;
};

struct Qlist {
	Lock	lk;
	Header*	first;

	uintptr	nalloc;
};

enum {
	Unitsz		= sizeof(Header),
};

#define	NUNITS(n)	(HOWMANY(n, Unitsz) + 1)
#define	NQUICK		((4096/Unitsz)+1)

int mallocok;

static	Qlist	quicklist[NQUICK+1];
static	Header	misclist;
static	Header	*rover;
static	uintptr tailsize;		/* in units of Header */
static	uintptr availnunits;
static	Header	*tailbase;
static	Header	*tailptr;
static	Header	checkval;

static	uintptr	morecore(uintptr);
static	void*	qmalloc(uintptr);
static	void	qfreeinternal(void*);

static	int	qstats[32];

static	Lock		mainlock;

#define	MLOCK		ilock(&mainlock)
#define	MUNLOCK		iunlock(&mainlock)
#define QLOCK(l)	ilock(l)
#define QUNLOCK(l)	iunlock(l)

#define	tailalloc(p, n)	assert(tailptr); \
			((p)=tailptr, tailsize -= (n), tailptr += (n),\
			 (p)->s.size=(n), (p)->s.next = &checkval)

#define ISPOWEROF2(x)	(/* (x) != 0 && */ (((x) & ((x)-1)) == 0))
#define ALIGNHDR(h, a)	(Header*)(((uintptr)(h)+((a)-1)) & ~((uintptr)(a)-1))
#ifndef ALIGNED
#define ALIGNED(h, a)	(((uintptr)(h) & ((a)-1)) == 0)
#endif

static void
qmfreefrag(Header *r, uintptr n, int qsidx)
{
	r->s.size = n;
	r->s.next = &checkval;
	qfreeinternal(r+1);
	qstats[qsidx]++;
}

static void *
qmremblk(Header *p, Header *q, uintptr nunits, int align, uintptr aligned)
{
	uintptr n;
	Header *r;

	q->s.next = p->s.next;
	rover = q;
	qstats[7]++;

	/*
	 * Free any runt in front of the alignment.
	 */
	if(!aligned){
		r = p;
		p = ALIGNHDR(p+1, align) - 1;
		n = p - r;
		p->s.size = r->s.size - n;
		if (n > 0)
			qmfreefrag(r, n, 8);
	}

	/*
	 * Free any residue after the aligned block.
	 */
	if(p->s.size > nunits){
		r = p+nunits;
		qmfreefrag(r, p->s.size - nunits, 9);
		p->s.size = nunits;
	}

	p->s.next = &checkval;
	return p+1;
}

static void *
qmaddcore(Header *q, uintptr align, uintptr nunits)
{
	uintptr naligned, n;
	Header *p, *r;

	if (nunits == 0)		/* sanity */
		nunits = NUNITS(1);
	naligned = NUNITS(align)-1;
	if(tailsize < nunits+naligned){
		/*
		 * There are at least nunits, get enough for alignment.
		 */
		if((n = morecore(naligned)) == 0)
			/* we can't recover */
			panic("qmaddcore: out of memory. pid %d %s",
				up? up->pid: 0, up? up->text: "");
		tailsize += n;
	}
	/*
	 * Save the residue before the aligned allocation
	 * and free it after the tail pointer has been bumped
	 * for the main allocation.
	 */
	n = q - tailptr - 1;
	tailalloc(r, n);
	tailalloc(p, nunits);
	qstats[11]++;
	qfreeinternal(r+1);
	return p;
}

static void*
qmallocalign(uintptr nbytes, uintptr align, vlong offset, uintptr span)
{
	Qlist *qlist;
	uintptr aligned;
	Header **pp, *p, *q;
	uintptr naligned, nunits, n;

	if (!mallocok) {
		mallocinit();
		print("qmallocalign(%llud): called before mallocinit; doing so\n",
			(uvlong)nbytes);
	}
	if(nbytes == 0 || offset != 0 || span != 0) {
		print("qmallocalign: bad args %lld %lld %lld\n",
			(vlong)nbytes, offset, (vlong)span);
		return nil;
	}

	if(!ISPOWEROF2(align))
		panic("qmallocalign");

	if(align <= sizeof(Align))
		return qmalloc(nbytes);

	qstats[5]++;
	nunits = NUNITS(nbytes);
	if(nunits <= NQUICK){
		/*
		 * Look for a conveniently aligned block
		 * on one of the quicklists.
		 */
		qlist = &quicklist[nunits];
		QLOCK(&qlist->lk);
		pp = &qlist->first;
		for(p = *pp; p != nil; p = p->s.next){
			if(ALIGNED(p+1, align)){
				*pp = p->s.next;
				p->s.next = &checkval;
				QUNLOCK(&qlist->lk);
				qstats[6]++;
				return p+1;
			}
			pp = &p->s.next;
		}
		QUNLOCK(&qlist->lk);
	}

	MLOCK;
	if(nunits > tailsize) {
		/* hard way */
		if((q = rover) != nil)
			do {
				p = q->s.next;
				if(p->s.size < nunits)
					continue;
				aligned = ALIGNED(p+1, align);
				naligned = NUNITS(align)-1;
				if(!aligned && p->s.size < nunits+naligned)
					continue;

				/*
				 * This block is big enough, remove it
				 * from the list.
				 */
				p = qmremblk(p, q, nunits, align, aligned);
				MUNLOCK;
				return p;
			} while((q = p) != rover);

		/* nothing fits, so add more core */
		if((n = morecore(nunits)) == 0){
			MUNLOCK;
			/* we can't recover */
			panic("qmallocalign: out of memory. pid %d %s",
				up? up->pid: 0, up? up->text: "");
		}
		tailsize += n;
	}

	q = ALIGNHDR(tailptr+1, align);
	if(q == tailptr+1){
		tailalloc(p, nunits);
		qstats[10]++;
	}else
		p = qmaddcore(q, align, nunits);
	MUNLOCK;
	return p+1;
}

static void*
qmalloc(uintptr nbytes)
{
	Qlist *qlist;
	Header *p, *q;
	uintptr nunits, n;

	if(nbytes == 0) {
		print("qmalloc: nbytes == 0\n");
		delay(500);
		return nil;
	}
	if (!mallocok) {
		mallocinit();
		print("qmalloc: called before mallocinit; doing so\n");
	}

	qstats[0]++;			/* heavily trafficked */
	nunits = NUNITS(nbytes);
	if(nunits <= NQUICK){
		qlist = &quicklist[nunits];
		QLOCK(&qlist->lk);
		if((p = qlist->first) != nil){
			qlist->first = p->s.next;
			qlist->nalloc++;
			qstats[1]++;	/* heavily trafficked ~85% */
			QUNLOCK(&qlist->lk);
			p->s.next = &checkval;
			return p+1;
		}
		QUNLOCK(&qlist->lk);
	}

	/* uncommon path ~15% */
	MLOCK;
	if(nunits > tailsize) {
		/* hard way */
		if((q = rover) != nil)
			do {
				p = q->s.next;
				if(p->s.size >= nunits) {
					if(p->s.size > nunits) {
						p->s.size -= nunits;
						p += p->s.size;
						p->s.size = nunits;
					} else
						q->s.next = p->s.next;
					p->s.next = &checkval;
					rover = q;
					qstats[2]++;
					MUNLOCK;
					return p+1;
				}
			} while((q = p) != rover);

		/* nothing fits, so add more core */
		if((n = morecore(nunits)) == 0){
			MUNLOCK;
			/* we can't recover */
			panic("qmalloc: out of memory. pid %d %s",
				up? up->pid: 0, up? up->text: "");
		}
		tailsize += n;
	}
	qstats[3]++;			/* moderately trafficked (~15%) */
	tailalloc(p, nunits);
	MUNLOCK;
	return p+1;
}

static void
ckcorrupt(char *func, Header *p)
{
	if(p->s.size == 0)
		panic("%s: corrupt allocation arena: p->s.size 0, p %#p",
			func, p);
	if(p->s.next != &checkval)
		panic("%s: corrupt allocation arena: bad p->s.next %#p, p %#p",
			func, p->s.next, p);
}

static void
qfreeinternal(void* ap)
{
	Qlist *qlist;
	Header *p, *q;
	uintptr nunits;

	if(ap == nil)
		return;
	qstats[16]++;			/* heavily trafficked */

	if (isuseraddr(ap))
		panic("free of user address %#p; pc=%#p", ap, getcallerpc(&ap));
	p = (Header*)ap - 1;
	ckcorrupt("qfreeinternal", p);
	nunits = p->s.size;
	if(tailptr != nil && p+nunits == tailptr) {	/* ~5% */
		/* block before tail */
		tailptr = p;
		tailsize += nunits;
		qstats[17]++;
		return;
	}
	if(nunits <= NQUICK) {
		qlist = &quicklist[nunits];
		QLOCK(&qlist->lk);
		p->s.next = qlist->first;
		qlist->first = p;
		QUNLOCK(&qlist->lk);
		qstats[18]++;		/* heavily trafficked */
		return;
	}

	/* uncommon path ~5% */
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
		qstats[19]++;		/* virtually never reached */
	} else
		p->s.next = q->s.next;
	if(q+q->s.size == p) {
		q->s.size += p->s.size;
		q->s.next = p->s.next;
		qstats[20]++;		/* virtually never reached */
	} else
		q->s.next = p;
	rover = q;
}

static int
mallocreadfmt(char* s, char* e)
{
	char *p;
	Header *q;
	int i, n, t;
	Qlist *qlist;

	p = s;
	p = seprint(p, e, "%llud/%llud kernel malloc\n",
		(vlong)(tailptr-tailbase)*sizeof(Header),
		(vlong)(tailsize+availnunits + tailptr-tailbase)*sizeof(Header));
	p = seprint(p, e, "0/0 kernel draw\n"); // keep scripts happy

	t = 0;
	for(i = 0; i <= NQUICK; i++) {
		n = 0;
		qlist = &quicklist[i];
		QLOCK(&qlist->lk);
		for(q = qlist->first; q != nil; q = q->s.next){
			if(Morestats && q->s.size != i)
				p = seprint(p, e, "q%d\t%#p\t%llud\n",
					i, q, (vlong)q->s.size);
			n++;
		}
		QUNLOCK(&qlist->lk);

		if(Morestats && n != 0)
			p = seprint(p, e, "q%d %d\n", i, n);
		t += n * i*sizeof(Header);
	}
	p = seprint(p, e, "quick: %ud bytes total\n", t);

	MLOCK;
	if((q = rover) != nil){
		i = t = 0;
		do {
			t += q->s.size;
			i++;
			if (Morestats)
				p = seprint(p, e, "m%lld\t%#p\n",
					(vlong)q->s.size, q);
		} while((q = q->s.next) != rover);

		p = seprint(p, e, "rover: %d blocks %llud bytes total\n",
			i, (vlong)t*sizeof(Header));
	}

	for(i = 0; i < nelem(qstats); i++)
		if(Morestats && qstats[i])
			p = seprint(p, e, "qstats[%d] %ud\n", i, qstats[i]);
	*p = '\0';
	MUNLOCK;
	return p - s;
}

long
mallocreadsummary(Chan*, void *a, long n, long offset)
{
	int bytes;
	char *alloc;

	if (n < 0)
		return n;
	alloc = malloc(4*READSTR);
	if(waserror()){
		free(alloc);
		nexterror();
	}
	bytes = mallocreadfmt(alloc, alloc+4*READSTR);
	n = readstr(offset, a, MIN(bytes, n), alloc);
	poperror();
	free(alloc);

	return n;
}

void
mallocsummary(void)
{
	Header *q;
	int i, n, t;
	Qlist *qlist;

	t = 0;
	for(i = 0; i <= NQUICK; i++) {
		n = 0;
		qlist = &quicklist[i];
		QLOCK(&qlist->lk);
		for(q = qlist->first; q != nil; q = q->s.next){
			if(q->s.size != i)
				DBG("q%d\t%#p\t%llud\n", i, q, (uvlong)q->s.size);
			n++;
		}
		QUNLOCK(&qlist->lk);

		t += n * i*sizeof(Header);
	}
	print("quick: %ud bytes total\n", t);

	MLOCK;
	if((q = rover) != nil){
		i = t = 0;
		do {
			t += q->s.size;
			i++;
		} while((q = q->s.next) != rover);
	}
	MUNLOCK;

	if(i != 0)
		print("rover: %d blocks %llud bytes total\n",
			i, (vlong)t*sizeof(Header));
	print("total allocated %llud, %llud remaining\n",
		(vlong)(tailptr-tailbase)*sizeof(Header),
		(vlong)(tailsize+availnunits)*sizeof(Header));

	for(i = 0; i < nelem(qstats); i++)
		if(qstats[i])
			print("qstats[%d] %ud\n", i, qstats[i]);
}

void
free(void* ap)
{
	MLOCK;
	qfreeinternal(ap);
	MUNLOCK;
}

void*
mallocz(uintptr size, int clr)
{
	void *v;

	if((v = qmalloc(size)) != nil && clr)
		memset(v, 0, size);
	return v;
}

void*
malloc(uintptr size)
{
	return mallocz(size, 1);
}

void*
mallocalign(uintptr nbytes, uintptr align, vlong offset, uintptr span)
{
	void *v;

	if(span != 0 && align <= span){
		if(nbytes > span) {
			print("mallocalign: nbytes %llud > span %llud\n",
				(uvlong)nbytes, (uvlong)span);
			return nil;
		}
		align = span;
		span = 0;
	}

	/*
	 * Should this memset or should it be left to the caller?
	 * kernel allocation always zeroes.
	 */
	if((v = qmallocalign(nbytes, align, offset, span)) != nil)
		memset(v, 0, nbytes);
	return v;
}

void*
smalloc(uintptr size)
{
	int first = 1;
	void *v;

	while((v = malloc(size)) == nil) {
		if (first) {
			if (up == nil)
				panic("smalloc %lld: nil up", (vlong)size);
			print("smalloc %lld stuck retrying; proc %s pid %d\n",
				(vlong)size, up->text, up->pid);
		}
		tsleep(&up->sleep, return0, 0, 100);
		first = 0;
	}
	return v;
}

void*
realloc(void* ap, uintptr size)
{
	void *v;
	Header *p;
	uintptr osize, nunits, ounits;
	vlong delta;

	/*
	 * Easy stuff:
	 * free and return nil if size is 0 (implementation-defined behaviour);
	 * behave like malloc if ap is nil;
	 * check for arena corruption;
	 * do nothing if units are the same.
	 */
	if (ap != nil && isuseraddr(ap))
		panic("realloc of user address %#p; pc=%#p",
			ap, getcallerpc(&ap));
	if(size == 0){
		free(ap);
		return nil;
	}
	if(ap == nil)
		return malloc(size);	/* kernel allocations always zero */

	p = (Header*)ap - 1;
	ckcorrupt("realloc", p);
	ounits = p->s.size;
	if((nunits = NUNITS(size)) == ounits)
		return ap;

	/*
	 * Slightly harder:
	 * if this allocation abuts the tail, try to just adjust the tailptr.
	 */
	MLOCK;
	if(tailptr != nil && p+ounits == tailptr){
		delta = nunits-ounits;
		if(delta < 0 || tailsize >= delta){
			p->s.size = nunits;
			tailsize -= delta;
			tailptr += delta;
			MUNLOCK;
			return ap;
		}
	}
	MUNLOCK;

	/*
	 * Worth doing if it's a small reduction?
	 * Do it anyway if <= NQUICK?
	if(ounits - nunits < 2)
		return ap;
	 */

	/*
	 * Too hard (or can't be bothered): allocate, copy and free.
	 * What does the standard say for failure here?
	 */
	v = qmalloc(size);
	if(v != nil){
		osize = (ounits-1)*sizeof(Header);
		if(size < osize)
			osize = size;	/* shrinking: copy only new size */
		memmove(v, ap, osize);
		/* growing: zero new bytes; kernel allocations always zero */
		if (size > osize)
			memset((char *)v + osize, 0, size - osize);
		free(ap);
	}
	return v;
}

void
setmalloctag(void*, uintptr)
{
}

void
mallocinit(void)
{
	if(tailptr != nil)
		return;

	tailptr = tailbase = (Header *)sys->vmunused;
	availnunits = HOWMANY(sys->vmend - sys->vmunused, Unitsz);
if(0)	print("malloc: base %#p end %#p nunits %llud\n",	
		tailbase, sys->vmend, (uvlong)availnunits);
	mallocok = 1;
}

static uintptr
morecore(uintptr nunits)
{
	/*
	 * First (simple) cut.
	 * Pump it up when you don't really need it.
	 * Pump it up until you can feel it.
	 */
	if(nunits < NUNITS(128*KB))
		nunits = NUNITS(128*KB);
	if(nunits > availnunits)
		nunits = availnunits;
	availnunits -= nunits;

	return nunits;
}
