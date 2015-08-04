/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * This allocator takes blocks from a coarser allocator (p->alloc) and
 * uses them as arenas.
 * 
 * An arena is split into a sequence of blocks of variable size.  The
 * blocks begin with a Bhdr that denotes the length (including the Bhdr)
 * of the block.  An arena begins with an Arena header block (Arena,
 * ARENA_MAGIC) and ends with a Bhdr block with magic ARENATAIL_MAGIC and
 * size 0.  Intermediate blocks are either allocated or free.  At the end
 * of each intermediate block is a Btail, which contains information
 * about where the block starts.  This is useful for walking backwards.
 * 
 * Free blocks (Free*) have a magic value of FREE_MAGIC in their Bhdr
 * headers.  They are kept in a binary tree (p->freeroot) traversible by
 * walking ->left and ->right.  Each node of the binary tree is a pointer
 * to a circular doubly-linked list (next, prev) of blocks of identical
 * size.  Blocks are added to this ``tree of lists'' by pooladd(), and
 * removed by pooldel().
 * 
 * When freed, adjacent blocks are coalesced to create larger blocks when
 * possible.
 * 
 * Allocated blocks (Alloc*) have one of two magic values: ALLOC_MAGIC or
 * UNALLOC_MAGIC.  When blocks are released from the pool, they have
 * magic value UNALLOC_MAGIC.  Once the block has been trimmed by trim()
 * and the amount of user-requested data has been recorded in the
 * datasize field of the tail, the magic value is changed to ALLOC_MAGIC.
 * All blocks returned to callers should be of type ALLOC_MAGIC, as
 * should all blocks passed to us by callers.  The amount of data the user
 * asked us for can be found by subtracting the short in tail->datasize 
 * from header->size.  Further, the up to at most four bytes between the
 * end of the user-requested data block and the actual Btail structure are
 * marked with a magic value, which is checked to detect user overflow.
 * 
 * The arenas returned by p->alloc are kept in a doubly-linked list
 * (p->arenalist) running through the arena headers, sorted by descending
 * base address (prev, next).  When a new arena is allocated, we attempt
 * to merge it with its two neighbors via p->merge.
 */

#include <u.h>
#include <libc.h>
#include <pool.h>

typedef struct Alloc	Alloc;
typedef struct Arena	Arena;
typedef struct Bhdr	Bhdr;
typedef struct Btail	Btail;
typedef struct Free	Free;

struct Bhdr {
	uint32_t	magic;
	uint32_t	size;
};
enum {
	NOT_MAGIC = 0xdeadfa11,
	DEAD_MAGIC = 0xdeaddead,
};
#define B2NB(b) ((Bhdr*)((uint8_t*)(b)+(b)->size))

#define SHORT(x) (((x)[0] << 8) | (x)[1])
#define PSHORT(p, x) \
	(((uint8_t*)(p))[0] = ((x)>>8)&0xFF, \
	((uint8_t*)(p))[1] = (x)&0xFF)

enum {
	TAIL_MAGIC0 = 0xBE,
	TAIL_MAGIC1 = 0xEF
};
struct Btail {
	uint8_t	magic0;
	uint8_t	datasize[2];
	uint8_t	magic1;
	uint32_t	size;	/* same as Bhdr->size */
};
#define B2T(b)	((Btail*)((uint8_t*)(b)+(b)->size-sizeof(Btail)))
#define B2PT(b) ((Btail*)((uint8_t*)(b)-sizeof(Btail)))
#define T2HDR(t) ((Bhdr*)((uint8_t*)(t)+sizeof(Btail)-(t)->size))
struct Free {
			Bhdr;
	Free*	left;
	Free*	right;
	Free*	next;
	Free*	prev;
};
enum {
	FREE_MAGIC = 0xBA5EBA11,
};

/*
 * the point of the notused fields is to make 8c differentiate
 * between Bhdr and Allocblk, and between Kempt and Unkempt.
 */
struct Alloc {
			Bhdr;
};
enum {
	ALLOC_MAGIC = 0x0A110C09,
	UNALLOC_MAGIC = 0xCAB00D1E + 1,
};

struct Arena {
			Bhdr;
	Arena*	aup;
	Arena*	down;
	uint32_t	asize;
	uint32_t	pad;	/* to a multiple of 8 bytes */
};
enum {
	ARENA_MAGIC = 0xC0A1E5CE + 1,
	ARENATAIL_MAGIC = 0xEC5E1A0C + 1,
};
#define A2TB(a)	((Bhdr*)((uint8_t*)(a)+(a)->asize-sizeof(Bhdr)))
#define A2B(a)	B2NB(a)

enum {
	ALIGN_MAGIC = 0xA1F1D1C1,
};

enum {
	MINBLOCKSIZE = sizeof(Free)+sizeof(Btail)
};

static uint8_t datamagic[] = { 0xFE, 0xF1, 0xF0, 0xFA };

#define	IntPoison 0xCafeBabe
#define	Poison	(void*)IntPoison

#define _B2D(a)	((void*)((uint8_t*)a+sizeof(Bhdr)))
#define _D2B(v)	((Alloc*)((uint8_t*)v-sizeof(Bhdr)))

// static void*	_B2D(void*);
// static void*	_D2B(void*);
static void*	B2D(Pool*, Alloc*);
static Alloc*	D2B(Pool*, void*);
static Arena*	arenamerge(Pool*, Arena*, Arena*);
static void		blockcheck(Pool*, Bhdr*);
static Alloc*	blockmerge(Pool*, Bhdr*, Bhdr*);
static Alloc*	blocksetdsize(Pool*, Alloc*, uint32_t);
static Bhdr*	blocksetsize(Bhdr*, uint32_t);
static uint32_t	bsize2asize(Pool*, uint32_t);
static uint32_t	dsize2bsize(Pool*, uint32_t);
static uint32_t	getdsize(Alloc*);
static Alloc*	trim(Pool*, Alloc*, uint32_t);
static Free*	listadd(Free*, Free*);
static Free**	ltreewalk(Free**, uint32_t);
static void		memmark(void*, int, uint32_t);
static Free*	pooladd(Pool*, Alloc*);
static void*	poolallocl(Pool*, uint32_t);
static void		poolcheckl(Pool*);
static void		poolcheckarena(Pool*, Arena*);
static int		poolcompactl(Pool*);
static Alloc*	pooldel(Pool*, Free*);
static void		pooldumpl(Pool*);
static void		pooldumparena(Pool*, Arena*);
static void		poolfreel(Pool*, void*);
static void		poolnewarena(Pool*, uint32_t);
static void*	poolreallocl(Pool*, void*, uint32_t);
static Free*	treedelete(Free*, Free*);
static Free*	treeinsert(Free*, Free*);
static Free*	treelookupgt(Free*, uint32_t);

/*
 * Debugging
 * 
 * Antagonism causes blocks to always be filled with garbage if their
 * contents are undefined.  This tickles both programs and the library.
 * It's a linear time hit but not so noticeable during nondegenerate use.
 * It would be worth leaving in except that it negates the benefits of the
 * kernel's demand-paging.  The tail magic and end-of-data magic 
 * provide most of the user-visible benefit that antagonism does anyway.
 *
 * Paranoia causes the library to recheck the entire pool on each lock
 * or unlock.  A failed check on unlock means we tripped over ourselves,
 * while a failed check on lock tends to implicate the user.  Paranoia has
 * the potential to slow things down a fair amount for pools with large
 * numbers of allocated blocks.  It completely negates all benefits won
 * by the binary tree.  Turning on paranoia in the kernel makes it painfully
 * slow.
 *
 * Verbosity induces the dumping of the pool via p->print at each lock operation.
 * By default, only one line is logged for each alloc, free, and realloc.
 */

/* the if(!x);else avoids ``dangling else'' problems */
#define antagonism	if(!(p->flags & POOL_ANTAGONISM)){}else
#define paranoia	if(!(p->flags & POOL_PARANOIA)){}else
#define verbosity	if(!(p->flags & POOL_VERBOSITY)){}else

#define DPRINT	if(!(p->flags & POOL_DEBUGGING)){}else p->print
#define LOG		if(!(p->flags & POOL_LOGGING)){}else p->print

/*
 * Tree walking
 */

static void
checklist(Free *t)
{
	Free *q;

	for(q=t->next; q!=t; q=q->next){
		assert(q->size == t->size);
		assert(q->next==nil || q->next->prev==q);
		assert(q->prev==nil || q->prev->next==q);
		assert(q->magic==FREE_MAGIC);
	}
}

static void
checktree(Free *t, int a, int b)
{
	assert(t->magic==FREE_MAGIC);
	assert(a < t->size && t->size < b);
	assert(t->next==nil || t->next->prev==t);
	assert(t->prev==nil || t->prev->next==t);
	checklist(t);
	if(t->left)
		checktree(t->left, a, t->size);
	if(t->right)
		checktree(t->right, t->size, b);
	
}

/* ltreewalk: return address of pointer to node of size == size */
static Free**
ltreewalk(Free **t, uint32_t size)
{
	assert(t != nil /* ltreewalk */);

	for(;;) {
		if(*t == nil)
			return t;

		assert((*t)->magic == FREE_MAGIC);

		if(size == (*t)->size)
			return t;
		if(size < (*t)->size)
			t = &(*t)->left;
		else
			t = &(*t)->right;
	}
}

/* treeinsert: insert node into tree */
static Free*
treeinsert(Free *tree, Free *node)
{
	Free **loc, *repl;

	assert(node != nil /* treeinsert */);

	loc = ltreewalk(&tree, node->size);
	if(*loc == nil) {
		node->left = nil;
		node->right = nil;
	} else {	/* replace existing node */
		repl = *loc;
		node->left = repl->left;
		node->right = repl->right;
	}
	*loc = node;
	return tree;
}

/* treedelete: remove node from tree */
static Free*
treedelete(Free *tree, Free *node)
{
	Free **loc, **lsucc, *succ;

	assert(node != nil /* treedelete */);

	loc = ltreewalk(&tree, node->size);
	assert(*loc == node);

	if(node->left == nil)
		*loc = node->right;
	else if(node->right == nil)
		*loc = node->left;
	else {
		/* have two children, use inorder successor as replacement */
		for(lsucc = &node->right; (*lsucc)->left; lsucc = &(*lsucc)->left)
			;
		succ = *lsucc;
		*lsucc = succ->right;
		succ->left = node->left;
		succ->right = node->right;
		*loc = succ;
	}

	node->left = node->right = Poison;
	return tree;
}

/* treelookupgt: find smallest node in tree with size >= size */
static Free*
treelookupgt(Free *t, uint32_t size)
{
	Free *lastgood;	/* last node we saw that was big enough */

	lastgood = nil;
	for(;;) {
		if(t == nil)
			return lastgood;
		if(size == t->size)
			return t;
		if(size < t->size) {
			lastgood = t;
			t = t->left;
		} else
			t = t->right;
	}
}

/* 
 * List maintenance
 */

/* listadd: add a node to a doubly linked list */
static Free*
listadd(Free *list, Free *node)
{
	if(list == nil) {
		node->next = node;
		node->prev = node;
		return node;
	}

	node->prev = list->prev;
	node->next = list;

	node->prev->next = node;
	node->next->prev = node;

	return list;
}

/* listdelete: remove node from a doubly linked list */
static Free*
listdelete(Pool *p, Free *list, Free *node)
{
	if(node->next == node) {	/* singular list */
		node->prev = node->next = Poison;
		return nil;
	}
	if(node->next == nil)
		p->panic(p, "pool->next");
	if(node->prev == nil)
		p->panic(p, "pool->prev");
	node->next->prev = node->prev;
	node->prev->next = node->next;

	if(list == node)
		list = node->next;

	node->prev = node->next = Poison;
	return list;
}

/*
 * Pool maintenance
 */

/* pooladd: add anode to the free pool */
static Free*
pooladd(Pool *p, Alloc *anode)
{
	Free *lst, *olst;
	Free *node;
	Free **parent;

	antagonism {
		memmark(_B2D(anode), 0xF7, anode->size-sizeof(Bhdr)-sizeof(Btail));
	}

	node = (Free*)anode;
	node->magic = FREE_MAGIC;
	parent = ltreewalk((Free **)&p->freeroot, node->size);
	olst = *parent;
	lst = listadd(olst, node);
	if(olst != lst)	/* need to update tree */
		*parent = treeinsert(*parent, lst);
	p->curfree += node->size;
	return node;
}

/* pooldel: remove node from the free pool */
static Alloc*
pooldel(Pool *p, Free *node)
{
	Free *lst, *olst;
	Free **parent;

	parent = ltreewalk((Free **)&p->freeroot, node->size);
	olst = *parent;
	assert(olst != nil /* pooldel */);

	lst = listdelete(p, olst, node);
	if(lst == nil)
		*parent = treedelete(*parent, olst);
	else if(lst != olst)
		*parent = treeinsert(*parent, lst);

	node->left = node->right = Poison;
	p->curfree -= node->size;

	antagonism {
		memmark(_B2D(node), 0xF9, node->size-sizeof(Bhdr)-sizeof(Btail));
	}

	node->magic = UNALLOC_MAGIC;
	return (Alloc*)node;
}

/*
 * Block maintenance 
 */
/* block allocation */
static uint32_t
dsize2bsize(Pool *p, uint32_t sz)
{
	sz += sizeof(Bhdr)+sizeof(Btail);
	if(sz < p->minblock)
		sz = p->minblock;
	if(sz < MINBLOCKSIZE)
		sz = MINBLOCKSIZE;
	sz = (sz+p->quantum-1)&~(p->quantum-1);
	return sz;
}

static uint32_t
bsize2asize(Pool *p, uint32_t sz)
{
	sz += sizeof(Arena)+sizeof(Btail);
	if(sz < p->minarena)
		sz = p->minarena;
	sz = (sz+p->quantum)&~(p->quantum-1);
	return sz;
}

/* blockmerge: merge a and b, known to be adjacent */
/* both are removed from pool if necessary. */
static Alloc*
blockmerge(Pool *pool, Bhdr *a, Bhdr *b)
{
	Btail *t;

	assert(B2NB(a) == b);

	if(a->magic == FREE_MAGIC)
		pooldel(pool, (Free*)a);
	if(b->magic == FREE_MAGIC)
		pooldel(pool, (Free*)b);

	t = B2T(a);
	t->size = IntPoison;
	t->magic0 = (uint8_t)NOT_MAGIC;
	t->magic1 = (uint8_t)NOT_MAGIC;
	PSHORT(t->datasize, NOT_MAGIC);

	a->size += b->size;
	t = B2T(a);
	t->size = a->size;
	PSHORT(t->datasize, 0xFFFF);

	b->size = NOT_MAGIC;
	b->magic = NOT_MAGIC;

	a->magic = UNALLOC_MAGIC;
	return (Alloc*)a;
}

/* blocksetsize: set the total size of a block, fixing tail pointers */
static Bhdr*
blocksetsize(Bhdr *b, uint32_t bsize)
{
	Btail *t;

	assert(b->magic != FREE_MAGIC /* blocksetsize */);

	b->size = bsize;
	t = B2T(b);
	t->size = b->size;
	t->magic0 = TAIL_MAGIC0;
	t->magic1 = TAIL_MAGIC1;
	return b;
}

/* getdsize: return the requested data size for an allocated block */
static uint32_t
getdsize(Alloc *b)
{
	Btail *t;
	t = B2T(b);
	return b->size - SHORT(t->datasize);
}

/* blocksetdsize: set the user data size of a block */
static Alloc*
blocksetdsize(Pool *p, Alloc *b, uint32_t dsize)
{
	Btail *t;
	uint8_t *q, *eq;

	assert(b->size >= dsize2bsize(p, dsize));
	assert(b->size - dsize < 0x10000);

	t = B2T(b);
	PSHORT(t->datasize, b->size - dsize);

	q=(uint8_t*)_B2D(b)+dsize;
	eq = (uint8_t*)t;
	if(eq > q+4)
		eq = q+4;
	for(; q<eq; q++)
		*q = datamagic[((uint32_t)(uintptr)q)%nelem(datamagic)];

	return b;
}

/* trim: trim a block down to what is needed to hold dsize bytes of user data */
static Alloc*
trim(Pool *p, Alloc *b, uint32_t dsize)
{
	uint32_t extra, bsize;
	Alloc *frag;

	bsize = dsize2bsize(p, dsize);
	extra = b->size - bsize;
	if(b->size - dsize >= 0x10000 ||
	  (extra >= bsize>>2 && extra >= MINBLOCKSIZE && extra >= p->minblock)) {
		blocksetsize(b, bsize);
		frag = (Alloc*) B2NB(b);

		antagonism {
			memmark(frag, 0xF1, extra);
		}

		frag->magic = UNALLOC_MAGIC;
		blocksetsize(frag, extra);
		pooladd(p, frag);
	}

	b->magic = ALLOC_MAGIC;
	blocksetdsize(p, b, dsize);
	return b;
}

static Alloc*
freefromfront(Pool *p, Alloc *b, uint32_t skip)
{
	Alloc *bb;

	skip = skip&~(p->quantum-1);
	if(skip >= 0x1000 || (skip >= b->size>>2 && skip >= MINBLOCKSIZE && skip >= p->minblock)){
		bb = (Alloc*)((uint8_t*)b+skip);
		blocksetsize(bb, b->size-skip);
		bb->magic = UNALLOC_MAGIC;
		blocksetsize(b, skip);
		b->magic = UNALLOC_MAGIC;
		pooladd(p, b);
		return bb;
	}
	return b;	
}

/*
 * Arena maintenance
 */

/* arenasetsize: set arena size, updating tail */
static void
arenasetsize(Arena *a, uint32_t asize)
{
	Bhdr *atail;

	a->asize = asize;
	atail = A2TB(a);
	atail->magic = ARENATAIL_MAGIC;
	atail->size = 0;
}

/* poolnewarena: allocate new arena */
static void
poolnewarena(Pool *p, uint32_t asize)
{
	Arena *a;
	Arena *ap, *lastap;
	Alloc *b;

	LOG(p, "newarena %lud\n", asize);
	if(p->cursize+asize > p->maxsize) {
		if(poolcompactl(p) == 0){
			LOG(p, "pool too big: %lud+%lud > %lud\n",
				p->cursize, asize, p->maxsize);
			werrstr("memory pool too large");
		}
		return;
	}

	if((a = p->alloc(asize)) == nil) {
		/* assume errstr set by p->alloc */
		return;
	}

	p->cursize += asize;

	/* arena hdr */
	a->magic = ARENA_MAGIC;
	blocksetsize(a, sizeof(Arena));
	arenasetsize(a, asize);
	blockcheck(p, a);

	/* create one large block in arena */
	b = (Alloc*)A2B(a);
	b->magic = UNALLOC_MAGIC;
	blocksetsize(b, (uint8_t*)A2TB(a)-(uint8_t*)b);
	blockcheck(p, b);
	pooladd(p, b);
	blockcheck(p, b);

	/* sort arena into descending sorted arena list */
	for(lastap=nil, ap=p->arenalist; ap > a; lastap=ap, ap=ap->down)
		;

	if(a->down = ap)	/* assign = */
		a->down->aup = a;

	if(a->aup = lastap)	/* assign = */
		a->aup->down = a;
	else
		p->arenalist = a;

	/* merge with surrounding arenas if possible */
	/* must do a with up before down with a (think about it) */
	if(a->aup)
		arenamerge(p, a, a->aup);
	if(a->down)
		arenamerge(p, a->down, a);
}

/* blockresize: grow a block to encompass space past its end, possibly by */
/* trimming it into two different blocks. */
static void
blockgrow(Pool *p, Bhdr *b, uint32_t nsize)
{
	if(b->magic == FREE_MAGIC) {
		Alloc *a;
		Bhdr *bnxt;
		a = pooldel(p, (Free*)b);
		blockcheck(p, a);
		blocksetsize(a, nsize);
		blockcheck(p, a);
		bnxt = B2NB(a);
		if(bnxt->magic == FREE_MAGIC)
			a = blockmerge(p, a, bnxt);
		blockcheck(p, a);
		pooladd(p, a);
	} else {
		Alloc *a;
		uint32_t dsize;

		a = (Alloc*)b;
		dsize = getdsize(a);
		blocksetsize(a, nsize);
		trim(p, a, dsize);
	}
}

/* arenamerge: attempt to coalesce to arenas that might be adjacent */
static Arena*
arenamerge(Pool *p, Arena *bot, Arena *top)
{
	Bhdr *bbot, *btop;
	Btail *t;

	blockcheck(p, bot);
	blockcheck(p, top);
	assert(bot->aup == top && top > bot);

	if(p->merge == nil || p->merge(bot, top) == 0)
		return nil;

	/* remove top from list */
	if(bot->aup = top->aup)	/* assign = */
		bot->aup->down = bot;
	else
		p->arenalist = bot;
	
	/* save ptrs to last block in bot, first block in top */
	t = B2PT(A2TB(bot));
	bbot = T2HDR(t);
	btop = A2B(top);
	blockcheck(p, bbot);
	blockcheck(p, btop);

	/* grow bottom arena to encompass top */
	arenasetsize(bot, top->asize + ((uint8_t*)top - (uint8_t*)bot));

	/* grow bottom block to encompass space between arenas */
	blockgrow(p, bbot, (uint8_t*)btop-(uint8_t*)bbot);
	blockcheck(p, bbot);
	return bot;
}

/* dumpblock: print block's vital stats */
static void
dumpblock(Pool *p, Bhdr *b)
{
	uint32_t *dp;
	uint32_t dsize;
	uint8_t *cp;

	dp = (uint32_t*)b;
	p->print(p, "pool %s block %p\nhdr %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux\n",
		p->name, b, dp[0], dp[1], dp[2], dp[3], dp[4], dp[5], dp[6]);

	dp = (uint32_t*)B2T(b);
	p->print(p, "tail %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux | %.8lux %.8lux\n",
		dp[-6], dp[-5], dp[-4], dp[-3], dp[-2], dp[-1], dp[0], dp[1]);

	if(b->magic == ALLOC_MAGIC){
		dsize = getdsize((Alloc*)b);
		if(dsize >= b->size)	/* user data size corrupt */
			return;

		cp = (uint8_t*)_B2D(b)+dsize;
		p->print(p, "user data ");
		p->print(p, "%.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux",
			cp[-8], cp[-7], cp[-6], cp[-5], cp[-4], cp[-3], cp[-2], cp[-1]);
		p->print(p, " | %.2ux %.2ux %.2ux %.2ux  %.2ux %.2ux %.2ux %.2ux\n",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
	}
}

static void
printblock(Pool *p, Bhdr *b, char *msg)
{
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
}

static void
panicblock(Pool *p, Bhdr *b, char *msg)
{
	p->print(p, "%s\n", msg);
	dumpblock(p, b);
	p->panic(p, "pool panic");
}

/* blockcheck: ensure a block consistent with our expectations */
/* should only be called when holding pool lock */
static void
blockcheck(Pool *p, Bhdr *b)
{
	Alloc *a;
	Btail *t;
	int i, n;
	uint8_t *q, *bq, *eq;
	uint32_t dsize;

	switch(b->magic) {
	default:
		panicblock(p, b, "bad magic");
	case FREE_MAGIC:
	case UNALLOC_MAGIC:
	 	t = B2T(b);
		if(t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		break;
	case DEAD_MAGIC:
	 	t = B2T(b);
		if(t->magic0 != TAIL_MAGIC0 || t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");
		n = getdsize((Alloc*)b);
		q = _B2D(b);
		q += 8;
		for(i=8; i<n; i++)
			if(*q++ != 0xDA)
				panicblock(p, b, "dangling pointer write");
		break;
	case ARENA_MAGIC:
		b = A2TB((Arena*)b);
		if(b->magic != ARENATAIL_MAGIC)
			panicblock(p, b, "bad arena size");
		/* fall through */
	case ARENATAIL_MAGIC:
		if(b->size != 0)
			panicblock(p, b, "bad arena tail size");
		break;
	case ALLOC_MAGIC:
		a = (Alloc*)b;
	 	t = B2T(b);
		dsize = getdsize(a);
		bq = (uint8_t*)_B2D(a)+dsize;
		eq = (uint8_t*)t;

		if(t->magic0 != TAIL_MAGIC0){
			/* if someone wrote exactly one byte over and it was a NUL, we sometimes only complain. */
			if((p->flags & POOL_TOLERANCE) && bq == eq && t->magic0 == 0)
				printblock(p, b, "mem user overflow (magic0)");
			else
				panicblock(p, b, "corrupt tail magic0");
		}

		if(t->magic1 != TAIL_MAGIC1)
			panicblock(p, b, "corrupt tail magic1");
		if(T2HDR(t) != b)
			panicblock(p, b, "corrupt tail ptr");

		if(dsize2bsize(p, dsize)  > a->size)
			panicblock(p, b, "too much block data");

		if(eq > bq+4)
			eq = bq+4;
		for(q=bq; q<eq; q++){
			if(*q != datamagic[((uintptr)q)%nelem(datamagic)]){
				if(q == bq && *q == 0 && (p->flags & POOL_TOLERANCE)){
					printblock(p, b, "mem user overflow");
					continue;
				}
				panicblock(p, b, "mem user overflow");
			}
		}
		break;
	}
}

/*
 * compact an arena by shifting all the free blocks to the end.
 * assumes pool lock is held.
 */
enum {
	FLOATING_MAGIC = 0xCBCBCBCB,	/* temporarily neither allocated nor in the free tree */
};

static int
arenacompact(Pool *p, Arena *a)
{
	Bhdr *b, *wb, *eb, *nxt;
	int compacted;

	if(p->move == nil)
		p->panic(p, "don't call me when pool->move is nil\n");

	poolcheckarena(p, a);
	eb = A2TB(a);
	compacted = 0;
	for(b=wb=A2B(a); b && b < eb; b=nxt) {
		nxt = B2NB(b);
		switch(b->magic) {
		case FREE_MAGIC:
			pooldel(p, (Free*)b);
			b->magic = FLOATING_MAGIC;
			break;
		case ALLOC_MAGIC:
			if(wb != b) {
				memmove(wb, b, b->size);
				p->move(_B2D(b), _B2D(wb));
				compacted = 1;
			}
			wb = B2NB(wb);
			break;
		}
	}

	/*
	 * the only free data is now at the end of the arena, pointed
	 * at by wb.  all we need to do is set its size and get out.
	 */
	if(wb < eb) {
		wb->magic = UNALLOC_MAGIC;
		blocksetsize(wb, (uint8_t*)eb-(uint8_t*)wb);
		pooladd(p, (Alloc*)wb);
	}

	return compacted;		
}

/*
 * compact a pool by compacting each individual arena.
 * 'twould be nice to shift blocks from one arena to the
 * next but it's a pain to code.
 */
static int
poolcompactl(Pool *pool)
{
	Arena *a;
	int compacted;

	if(pool->move == nil || pool->lastcompact == pool->nfree)
		return 0;

	pool->lastcompact = pool->nfree;
	compacted = 0;
	for(a=pool->arenalist; a; a=a->down)
		compacted |= arenacompact(pool, a);
	return compacted;
}

/*
static int
poolcompactl(Pool*)
{
	return 0;
}
*/

/*
 * Actual allocators
 */

/*
static void*
_B2D(void *a)
{
	return (uint8_t*)a+sizeof(Bhdr);
}
*/

static void*
B2D(Pool *p, Alloc *a)
{
	if(a->magic != ALLOC_MAGIC)
		p->panic(p, "B2D called on unworthy block");
	return _B2D(a);
}

/*
static void*
_D2B(void *v)
{
	Alloc *a;
	a = (Alloc*)((uint8_t*)v-sizeof(Bhdr));
	return a;
}
*/

static Alloc*
D2B(Pool *p, void *v)
{
	Alloc *a;
	uint32_t *u;

	if((uintptr)v&(sizeof(uint32_t)-1))
		v = (char*)v - ((uintptr)v&(sizeof(uint32_t)-1));
	u = v;
	while(u[-1] == ALIGN_MAGIC)
		u--;
	a = _D2B(u);
	if(a->magic != ALLOC_MAGIC)
		p->panic(p, "D2B called on non-block %p (double-free?)", v);
	return a;
}

/* poolallocl: attempt to allocate block to hold dsize user bytes; assumes lock held */
static void*
poolallocl(Pool *p, uint32_t dsize)
{
	uint32_t bsize;
	Free *fb;
	Alloc *ab;

	if(dsize >= 0x80000000UL){	/* for sanity, overflow */
		werrstr("invalid allocation size");
		return nil;
	}

	bsize = dsize2bsize(p, dsize);

	fb = treelookupgt(p->freeroot, bsize);
	if(fb == nil) {
		poolnewarena(p, bsize2asize(p, bsize));
		if((fb = treelookupgt(p->freeroot, bsize)) == nil) {
			/* assume poolnewarena failed and set %r */
			return nil;
		}
	}

	ab = trim(p, pooldel(p, fb), dsize);
	p->curalloc += ab->size;
	antagonism {
		memset(B2D(p, ab), 0xDF, dsize);
	}
	return B2D(p, ab);
}

/* poolreallocl: attempt to grow v to ndsize bytes; assumes lock held */
static void*
poolreallocl(Pool *p, void *v, uint32_t ndsize)
{
	Alloc *a;
	Bhdr *left, *right, *newb;
	Btail *t;
	uint32_t nbsize;
	uint32_t odsize;
	uint32_t obsize;
	void *nv;

	if(v == nil)	/* for ANSI */
		return poolallocl(p, ndsize);
	if(ndsize == 0) {
		poolfreel(p, v);
		return nil;
	}
	a = D2B(p, v);
	blockcheck(p, a);
	odsize = getdsize(a);
	obsize = a->size;

	/* can reuse the same block? */
	nbsize = dsize2bsize(p, ndsize);
	if(nbsize <= a->size) {
	Returnblock:
		if(v != _B2D(a))
			memmove(_B2D(a), v, odsize);
		a = trim(p, a, ndsize);
		p->curalloc -= obsize;
		p->curalloc += a->size;
		v = B2D(p, a);
		return v;
	}

	/* can merge with surrounding blocks? */
	right = B2NB(a);
	if(right->magic == FREE_MAGIC && a->size+right->size >= nbsize) {
		a = blockmerge(p, a, right);
		goto Returnblock;
	}

	t = B2PT(a);
	left = T2HDR(t);
	if(left->magic == FREE_MAGIC && left->size+a->size >= nbsize) {
		a = blockmerge(p, left, a);
		goto Returnblock;
	}

	if(left->magic == FREE_MAGIC && right->magic == FREE_MAGIC
	&& left->size+a->size+right->size >= nbsize) {
		a = blockmerge(p, blockmerge(p, left, a), right);
		goto Returnblock;
	}

	if((nv = poolallocl(p, ndsize)) == nil)
		return nil;

	/* maybe the new block is next to us; if so, merge */
	left = T2HDR(B2PT(a));
	right = B2NB(a);
	newb = D2B(p, nv);
	if(left == newb || right == newb) {
		if(left == newb || left->magic == FREE_MAGIC)
			a = blockmerge(p, left, a);
		if(right == newb || right->magic == FREE_MAGIC)
			a = blockmerge(p, a, right);
		assert(a->size >= nbsize);
		goto Returnblock;
	}

	/* enough cleverness */
	memmove(nv, v, odsize);
	antagonism { 
		memset((char*)nv+odsize, 0xDE, ndsize-odsize);
	}
	poolfreel(p, v);
	return nv;
}

static void*
alignptr(void *v, uint32_t align, long offset)
{
	char *c;
	uint32_t off;

	c = v;
	if(align){
		off = (uintptr)c%align;
		if(off != offset){
			c += offset - off;
			if(off > offset)
				c += align;
		}
	}
	return c;
}

/* poolallocalignl: allocate as described below; assumes pool locked */
static void*
poolallocalignl(Pool *p, uint32_t dsize, uint32_t align, long offset,
		uint32_t span)
{
	uint32_t asize;
	void *v;
	char *c;
	uint32_t *u;
	int skip;
	Alloc *b;

	/*
	 * allocate block
	 * 	dsize bytes
	 *	addr == offset (modulo align)
	 *	does not cross span-byte block boundary
	 *
	 * to satisfy alignment, just allocate an extra
	 * align bytes and then shift appropriately.
	 * 
	 * to satisfy span, try once and see if we're
	 * lucky.  the second time, allocate 2x asize
	 * so that we definitely get one not crossing
	 * the boundary.
	 */
	if(align){
		if(offset < 0)
			offset = align - ((-offset)%align);
		else
			offset %= align;
	}
	asize = dsize+align;
	v = poolallocl(p, asize);
	if(v == nil)
		return nil;
	if(span && (uintptr)v/span != ((uintptr)v+asize)/span){
		/* try again */
		poolfreel(p, v);
		v = poolallocl(p, 2*asize);
		if(v == nil)
			return nil;
	}

	/*
	 * figure out what pointer we want to return
	 */
	c = alignptr(v, align, offset);
	if(span && (uintptr)c/span != (uintptr)(c+dsize-1)/span){
		c += span - (uintptr)c%span;
		c = alignptr(c, align, offset);
		if((uintptr)c/span != (uintptr)(c+dsize-1)/span){
			poolfreel(p, v);
			werrstr("cannot satisfy dsize %lud span %lud with align %lud+%ld", dsize, span, align, offset);
			return nil;
		}
	}
	skip = c - (char*)v;

	/*
	 * free up the skip bytes before that pointer
	 * or mark it as unavailable.
	 */
	b = _D2B(v);
	b = freefromfront(p, b, skip);
	v = _B2D(b);
	skip = c - (char*)v;
	if(c > (char*)v){
		u = v;
		while(c >= (char*)u+sizeof(uint32_t))
			*u++ = ALIGN_MAGIC;
	}
	trim(p, b, skip+dsize);
	assert(D2B(p, c) == b);
	antagonism { 
		memset(c, 0xDD, dsize);
	}
	return c;
}

/* poolfree: free block obtained from poolalloc; assumes lock held */
static void
poolfreel(Pool *p, void *v)
{
	Alloc *ab;
	Bhdr *back, *fwd;

	if(v == nil)	/* for ANSI */
		return;

	ab = D2B(p, v);
	blockcheck(p, ab);

	if(p->flags&POOL_NOREUSE){
		int n;

		ab->magic = DEAD_MAGIC;
		n = getdsize(ab)-8;
		if(n > 0)
			memset((uint8_t*)v+8, 0xDA, n);
		return;	
	}

	p->nfree++;
	p->curalloc -= ab->size;
	back = T2HDR(B2PT(ab));
	if(back->magic == FREE_MAGIC)
		ab = blockmerge(p, back, ab);

	fwd = B2NB(ab);
	if(fwd->magic == FREE_MAGIC)
		ab = blockmerge(p, ab, fwd);

	pooladd(p, ab);
}

void*
poolalloc(Pool *p, uint32_t n)
{
	void *v;

	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	v = poolallocl(p, n);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolalloc %p %lud = %p\n", p, n, v);
	p->unlock(p);
	return v;
}

void*
poolallocalign(Pool *p, uint32_t n, uint32_t align, int32_t offset,
	       uint32_t span)
{
	void *v;

	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	v = poolallocalignl(p, n, align, offset, span);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolalignspanalloc %p %lud %lud %lud %ld = %p\n", p, n, align, span, offset, v);
	p->unlock(p);
	return v;
}

int
poolcompact(Pool *p)
{
	int rv;

	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	rv = poolcompactl(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	LOG(p, "poolcompact %p\n", p);
	p->unlock(p);
	return rv;
}

void*
poolrealloc(Pool *p, void *v, uint32_t n)
{
	void *nv;

	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	nv = poolreallocl(p, v, n);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolrealloc %p %p %ld = %p\n", p, v, n, nv);
	p->unlock(p);
	return nv;
}

void
poolfree(Pool *p, void *v)
{
	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	poolfreel(p, v);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolfree %p %p\n", p, v);
	p->unlock(p);
}

/*
 * Return the real size of a block, and let the user use it. 
 */
uint32_t
poolmsize(Pool *p, void *v)
{
	Alloc *b;
	uint32_t dsize;

	p->lock(p);
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(v == nil)	/* consistency with other braindead ANSI-ness */
		dsize = 0;
	else {
		b = D2B(p, v);
		dsize = (b->size&~(p->quantum-1)) - sizeof(Bhdr) - sizeof(Btail);
		assert(dsize >= getdsize(b));
		blocksetdsize(p, b, dsize);
	}
	paranoia {
		poolcheckl(p);
	}
	verbosity {
		pooldumpl(p);
	}
	if(p->logstack && (p->flags & POOL_LOGGING)) p->logstack(p);
	LOG(p, "poolmsize %p %p = %ld\n", p, v, dsize);
	p->unlock(p);
	return dsize;
}

/*
 * Debugging 
 */

static void
poolcheckarena(Pool *p, Arena *a)
{
	Bhdr *b;
	Bhdr *atail;

	atail = A2TB(a);
	for(b=a; b->magic != ARENATAIL_MAGIC && b<atail; b=B2NB(b))
		blockcheck(p, b);
	blockcheck(p, b);
	if(b != atail)
		p->panic(p, "found wrong tail");
}

static void
poolcheckl(Pool *p)
{
	Arena *a;

	for(a=p->arenalist; a; a=a->down)
		poolcheckarena(p, a);
	if(p->freeroot)
		checktree(p->freeroot, 0, 1<<30);
}

void
poolcheck(Pool *p)
{
	p->lock(p);
	poolcheckl(p);
	p->unlock(p);
}

void
poolblockcheck(Pool *p, void *v)
{
	if(v == nil)
		return;

	p->lock(p);
	blockcheck(p, D2B(p, v));
	p->unlock(p);
}

static void
pooldumpl(Pool *p)
{
	Arena *a;

	p->print(p, "pool %p %s\n", p, p->name);
	for(a=p->arenalist; a; a=a->down)
		pooldumparena(p, a);
}

void
pooldump(Pool *p)
{
	p->lock(p);
	pooldumpl(p);
	p->unlock(p);
}

static void
pooldumparena(Pool *p, Arena *a)
{
	Bhdr *b;

	for(b=a; b->magic != ARENATAIL_MAGIC; b=B2NB(b))
		p->print(p, "(%p %.8lux %lud)", b, b->magic, b->size);
	p->print(p, "\n");
}

/*
 * mark the memory in such a way that we know who marked it
 * (via the signature) and we know where the marking started.
 */
static void
memmark(void *v, int sig, uint32_t size)
{
	uint8_t *p, *ep;
	uint32_t *lp, *elp;
	lp = v;
	elp = lp+size/4;
	while(lp < elp) {
		*lp = (sig<<24) ^ ((uintptr)lp-(uintptr)v);
		lp++;
	}
	p = (uint8_t*)lp;
	ep = (uint8_t*)v+size;
	while(p<ep)
		*p++ = sig;
}
