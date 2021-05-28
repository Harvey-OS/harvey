/*
 * Buddy allocator for physical memory allocation.
 * One per ACPI affinity domain, to color pages depending on their
 * NUMA location.
 *
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "acpi.h"

#define ISPOWEROF2(x)	(((x) != 0) && !((x) & ((x)-1)))
#define UNO		((uintmem)1)

enum {
	BKmin		= 12,			/* Minimum lg2 */
	BKmax		= 30,			/* Maximum lg2 */

	Ndoms = 16,				/* Max # of domains */

	Used = 0,
	Avail = 1,
};


#define INDEX(b, v)	((uint)(((v))/(b)->bminsz))
#define BLOCK(b, i)	((i)-INDEX((b),(b)->memory))

typedef struct Buddy Buddy;
struct Buddy {
	short	tag;		/* Used or Avail */
	short	kval;
	uint	next;
	uint	prev;
	void	*p;
};

/*
 * Bals should allocate using its base address as 0.
 * For now, all of them refer to the entire memory and we record
 * the base and size for each one.
 */
typedef struct Bal Bal;
struct Bal {
	uintmem	base;
	u64int	size;
	usize	nfree;
	usize	nblocks;
	int	kmin;		/* Minimum lg2 */
	int	kmax;		/* Maximum lg2 */
	uintmem	bminsz;		/* minimum block sz */
	uintmem memory;
	uint	kspan;

	Buddy* blocks;
	Buddy* avail;
};

static Bal bal[Ndoms];
static int ndoms;
static Lock budlock;

char*
seprintphysstats(char *s,  char *e)
{
	Bal *b;
	int i;

	lock(&budlock);
	for(i = 0; i < Ndoms; i++){
		b = &bal[i];
		if(b->size > 0)
			s = seprint(s, e, "%uld/%uld %ulldK color %d blocks avail\n",
				b->nfree, b->nblocks, b->bminsz/KiB, i);
	}
	unlock(&budlock);
	return s;
}

static void
xphysfree(Bal *b, uintmem data, u64int size)
{
	uint i;
	Buddy *l, *p;
	Buddy *blocks, *avail;

	DBG("physfree\n");

	/*
	 * Knuth's Algorithm S (Buddy System Liberation).
	 */
	blocks = b->blocks;
	avail = b->avail;

	if(data == 0 /*|| !ALIGNED(data, b->bminsz)*/)
		return;
	i = INDEX(b,data);

	lock(&budlock);
S1:
	/*
	 * Find buddy.
	 */
	l = &blocks[BLOCK(b,i)];
	l->p = nil;
	DBG("\tbsl: BLOCK(b,i) %d index %ulld kval %d\n",
		BLOCK(b,i), BLOCK(b,i)/((1<<l->kval)/b->bminsz), l->kval);
	if((BLOCK(b,i)/((1<<l->kval)/b->bminsz)) & 1)	/* simpler test? */
		p = l - (1<<l->kval)/b->bminsz;
	else
		p = l + (1<<l->kval)/(b->bminsz);
	DBG("\tbsl: l @ %ld buddy @ %ld\n", l - blocks, p - blocks);

	/*
	 * Is buddy available?
	 * Can't merge if:
	 *	this is the largest block;
	 *	buddy isn't free;
	 *	buddy has been subsequently split again.
	 */
	if(l->kval == b->kmax || p->tag == Used || (p->tag == Avail && p->kval != l->kval)){
		/*
		 * Put on list.
		 */
		l->tag = Avail;
		l->next = avail[l->kval].next;
		l->prev = 0;
		if(l->next != 0)
			blocks[BLOCK(b,l->next)].prev = i;
		avail[l->kval].next = i;

		b->nfree += size/b->bminsz;

		unlock(&budlock);
		DBG("bsl: free @ i %d BLOCK(b,i) %d kval %d next %d %s\n",
			i, BLOCK(b,i), l->kval, l->next, l->tag?"avail":"used");
		return;
	}

	/*
	 * Combine with buddy.
	 * This removes block P from the avail list.
	 */
	if(p->prev != 0)
		blocks[BLOCK(b,p->prev)].next = p->next;
	else
		avail[p->kval].next = p->next;
	if(p->next != 0)
		blocks[BLOCK(b,p->next)].prev = p->prev;
	p->next = p->prev = 0;
	p->tag = Used;

	/*
	 * Now can try to merge this larger block.
	k++;
	 */
	DBG("\tbsl: l @ %ld p @ %ld\n", l - blocks, p - blocks);
	if(p < l)
		l = p;
	i = l - blocks + INDEX(b,b->memory);
	l->kval++;
	DBG("bsl: merge @ i %d BLOCK(b,i) %d kval %d next %d tag %s\n",
		i, BLOCK(b,i), l->kval, l->next, l->tag?"avail":"used");
	goto S1;
}

void
physfree(uintmem data, u64int size)
{
	Bal *b;
	int i;

	for(i = 0; i < Ndoms; i++){
		b = &bal[i];
		if(b->base <= data && data < b->base + b->size){
			xphysfree(b, data, size);
			return;
		}
	}
	panic("physfree: no bal");
}

static void*
xphystag(Bal *b, uintmem data)
{
	uint i;
	Buddy *blocks;

	DBG("phystag\n");

	blocks = b->blocks;

	if(data == 0 /*|| !ALIGNED(data, b->bminsz)*/)
		return nil;
	i = INDEX(b,data);
	return blocks[BLOCK(b,i)].p;
}

void*
phystag(uintmem data)
{
	Bal *b;
	int i;

	for(i = 0; i < Ndoms; i++){
		b = &bal[i];
		if(b->base <= data && data < b->base + b->size)
			return xphystag(b, data);
	}
	return nil;
}

static uchar lg2table[256] = {
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};

static int
lg2floor(u64int w)
{
	u64int hi, lo;

	if((lo = (w>>48)) != 0){
		if((hi = (lo>>8)) != 0)
			return 56+lg2table[hi];
		return 48+lg2table[lo];
	}
	if((lo = (w>>32)) != 0){
		if((hi = (lo>>8)) != 0)
			return 40+lg2table[hi];
		return 32+lg2table[lo];
	}
	if((lo = (w>>16)) != 0){
		if((hi = (lo>>8)) != 0)
			return 24+lg2table[hi];
		return 16+lg2table[lo];
	}
	if((hi = (w>>8)) != 0)
		return 8+lg2table[hi];
	return lg2table[w];
}

static uintmem
xphysalloc(Bal *b, u64int size, void *tag)
{
	uint i, j, k;
	Buddy *l, *p;
	Buddy *avail, *blocks;
	uintmem m;

	DBG("physalloc\n");
	assert(b->size > 0);

	avail = b->avail;
	blocks = b->blocks;

	/*
	 * Knuth's Algorithm R (Buddy System Reservation).
	 */
	if(size < b->bminsz)
		size = b->bminsz;

	/*
	 * Find block.
	 */
	if(!ISPOWEROF2(size))
		return 0;
	k = lg2floor(size);

	lock(&budlock);
	for(j = k; j <= b->kmax; j++){
		if(avail[j].next != 0)
			break;
	}
	DBG("bsr: size %#llud k %d j %d\n", size, k, j);
	if(j > b->kmax){
		unlock(&budlock);
		return 0;
	}

	/*
	 * Remove from list.
	 */
	i = avail[j].next;
	l = &blocks[BLOCK(b,i)];
	DBG("bsr: block @ i %d BLOCK(b,i) %d kval %d next %d %s\n",
		i, BLOCK(b,i), l->kval, l->next, l->tag?"avail":"used");
	avail[j].next = l->next;
	if(avail[j].next != 0)
		blocks[BLOCK(b,avail[j].next)].prev = 0;
	l->prev = l->next = 0;
	l->tag = Used;
	l->kval = k;

	/*
	 * Split required?
	 */
	while(j > k){
		/*
		 * Split.
		 */
		j--;
		p = &blocks[BLOCK(b,i) + (UNO<<j)/(b->bminsz)];
		p->tag = Avail;
		p->kval = j;
		p->next = avail[j].next;
		p->prev = 0;
		if(p->next != 0)
			blocks[BLOCK(b,p->next)].prev = i + (UNO<<j)/(b->bminsz);
		avail[j].next = i + (UNO<<j)/(b->bminsz);
		DBG("bsr: split @ i %d BLOCK(b,i) %ld j %d next %d (%d) %s\n",
			i, p - blocks, j, p->next, BLOCK(b,p->next),
			p->tag?"avail":"used");
	}
	b->nfree -= size/b->bminsz;
	unlock(&budlock);

	m = b->memory + b->bminsz*BLOCK(b,i);
	assert(m >= b->base && m < b->base + b->size);
	blocks[BLOCK(b,i)].p = tag;

	return m;
}

uintmem
physalloc(u64int size, int *colorp, void *tag)
{
	int i, color;
	uintmem m;

	m = 0;

	color = *colorp;
	if(color >= 0){
		color %= ndoms;
		if(bal[color].kmin > 0){
			*colorp = color;
			m = xphysalloc(&bal[color], size, tag);
		}
	}
	if(m == 0)
		for(i = 0; i < ndoms; i++)
			if(bal[i].kmin > 0)
				if((m = xphysalloc(&bal[i], size, tag)) != 0){
					*colorp = i;
					return m;
				}
	return m;
}

static void
dump(Bal *b)
{
	uint bi, i, k;
	Buddy *blocks;

	blocks = b->blocks;
	for(i = 0; i < (UNO<<(b->kmax-b->kmin+1)); i++){
		if(blocks[i].tag == Used)
			continue;
		print("blocks[%d]: size %d prev %d next %d\n",
			i, 1<<b->blocks[i].kval, blocks[i].prev, blocks[i].next);
		//i += (1<<blocks[i].kval)/b->bminsz-1;
	}

	for(k = 0; k <= b->kmax; k++){
		print("a[%d]:", k);
		for(bi = b->avail[k].next; bi != 0; bi = blocks[BLOCK(b,bi)].next){
			print(" %d", bi);
		}
		print("\n");
	}
}

void
physallocdump(void)
{
	int n;

	for(n = 0; n < Ndoms; n++)
		if(bal[n].size > 0)
			print("physalloc color=%d base=%#ullx size=%#ullx\n",
				n, bal[n].base, bal[n].size);
}

static int
plop(Bal *b, uintmem a, int k, int type)
{
	uint i;
	Buddy *l;


	DBG("plop(a %#p k %d type %d)\n", a, k, type);

	i = INDEX(b,a);
	l = &b->blocks[BLOCK(b,i)];

	l->kval = k;
	xphysfree(b, a, 1<<k);

	return 1;
}

static int
iimbchunk(Bal *b, uintmem a, uintmem e, int type)
{
	int k;
	uint s;

	a = ROUNDUP(a, b->bminsz);
	e = ROUNDDN(e, b->bminsz);
	DBG("iimbchunk: start a %#P e %#P\n", a, e);

	b->nblocks += (e-a)/b->bminsz;

	for(k = b->kmin, s = b->bminsz; a+s < e && k < b->kmax; s <<= 1, k += 1){
		if(a & s){
			plop(b, a, k, type);
			a += s;
		}
	}
	DBG("done1 a %#P e %#P s %#ux %d\n", a, e, s, k);

	while(a+s <= e){
		plop(b, a, k, type);
		a += s;
	}
	DBG("done2 a %#P e %#P s %#ux %d\n", a, e, s, k);

	for(k -= 1, s >>= 1; a < e; s >>= 1, k -= 1){
		if(a+s <= e){
			plop(b, a, k, type);
			a += s;
		}
	}
	DBG("done3 a %#P e %#P s %#ux %d\n", a, e, s, k);

	return 0;
}

/*
 * Called from umeminit to initialize user memory allocators.
 */
void
physinit(uintmem a, u64int size)
{
	uintmem dtsz;
	Bal *b;
	int i, dom;
	uintmem addr, len;

	DBG("physinit %#ullx %#ullx\n", a, size);

	for(addr = a; addr < a+size; addr += len){
		dom = 0;
		len = acpimblocksize(addr, &dom);
		/* len can be zero if there's no acpi information about addr */
		if(len == 0 || addr + len > a + size)
			len = a + size - addr;
		/*
		 * Each block belongs to a different domain (ie. cpu/mem socket)
		 * We must create a buddy allocator for each block, so we could
		 * allocate memory from different domains.
		 *
		 * This code assumes that a domain may be extended later and
		 * that there is no interleaving of domains. Ok by now.
		 */
		DBG("physmem block dom %d addr %#ullx size %#ullx\n",
			dom, addr, len);
		if(dom < 0 || dom >= Ndoms){
			print("physinit: invalid dom %d\n", dom);
			dom = 0;
		}
		b = &bal[dom];
		if(dom >= ndoms)
			ndoms = dom+1;
		if(b->kmin == 0){
			b->base = addr;
			b->size = len;
			b->kmin = BKmin;
			b->kmax = BKmax;
			b->bminsz = (UNO<<b->kmin);
			b->memory = sys->pmstart;
			b->kspan = lg2floor(sys->pmend);
			if(!ISPOWEROF2(sys->pmend))
				b->kspan++;
			dtsz = sizeof(Buddy)*(UNO<<(b->kspan-b->kmin+1));
			DBG("kspan %ud (arrysz = %llud)\n", b->kspan, dtsz);
			b->blocks = malloc(dtsz);
			if(b->blocks == nil)
				panic("physinit: no blocks");
			memset(b->blocks, 0, dtsz);
			b->avail = malloc(sizeof(Buddy)*(b->kmax+1));
			if(b->avail == nil)
				panic("physinit: no avail");
			memset(b->avail, 0, sizeof(Buddy)*(b->kmax+1));
		}else{
			if(addr < b->base)
				panic("physinit: decreasing base");
			if(b->base+b->size < addr + len)
				b->size = (addr-b->base) + len;
			for(i = 0; i < Ndoms; i++)
				if(bal[i].kmin && &bal[i] != b)
				if(bal[i].base < b->base + b->size &&
				   bal[i].base + bal[i].size > b->base + b->size)
					panic("physinit: doms overlap");
		}
		assert(addr >= b->base && addr+len <= b->base + b->size);

		iimbchunk(b, addr, addr+len, 0);
	}


}
