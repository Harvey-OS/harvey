#include "all.h"
#include "mem.h"
#include "io.h"

long	niob;
long	nhiob;
Hiob	*hiob;

/*
 * Called to allocate permanent data structures.
 * Alignment is in number of bytes. It pertains both to the start and
 * end of the allocated memory.
 */
void*
xialloc(ulong n, int align, ulong pte)
{
	ulong v;
	int m;

	lock(&mconf.iallock);
	if((v = mconf.vfract) == 0)
		v = mconf.vbase;

	if(align <= 0)
		align = 4;
	if(m = n % align)
		n += align - m;
	if(m = v % align)
		v += align - m;
	mconf.vfract = v+n;

	if(mconf.vfract >= mconf.vlimit)
		panic("xialloc not enough mem %lux %lux\n", mconf.vfract, mconf.vlimit);

	if(mconf.vfract > mconf.vbase){
		pte |= PTEVALID|PTEWRITE|PTEKERNEL;
		while(mconf.vbase < mconf.vfract){
			ulong frame = PPN(mconf.vbase);
			if (frame < mconf.npage0)
				frame += PPN(mconf.base0);
			else
				frame += PPN(mconf.base1) - mconf.npage0;
			putpte(mconf.vbase, pte + frame, 1);
			mconf.vbase += BY2PG;
		}
	}

	unlock(&mconf.iallock);
	memset((void*)v, 0, n);
	return (void*)v;
}

void*
ialloc(ulong n, int align)
{
	return xialloc(n, align, PTEMAINMEM);
}

typedef struct Vpage	Vpage;
typedef struct Vpalloc	Vpalloc;

struct Vpage {
	ulong	va;			/* Virtual address for user */
	ushort	ref;			/* Reference count */
	Iobuf*	iobuf;			/* Associated iobuf */
	Vpage	*next;			/* Lru free list */
	Vpage	*prev;
	Vpage	*hash;			/* Iobuf hash chains */
};

struct Vpalloc {
	Lock;
	Vpage	*head;			/* most recently used */
	Vpage	*tail;			/* least recently used */
	int	freecount;		/* how many pages on free list now */
	int	nvpage;			/* total number of vpages */
	Vpage	**hash;

	ulong	hits;
	ulong	misses;
	ulong	reclaims;
};

Vpalloc vpalloc;

static void
cmd_vcache(int argc, char *argv[])
{
	USED(argc, argv);
	print("\t%lud pages, %lud free\n", vpalloc.nvpage, vpalloc.freecount);
	print("\t%lud hits, %lud misses, %lud reclaims\n",
		vpalloc.hits, vpalloc.misses, vpalloc.reclaims);
}

/*
 * allocate rest of mem
 * for io buffers.
 */
#define	HWIDTH	8	/* buffers per hash */
void
iobufinit(void)
{
	long i;
	Iobuf *p, *q;
	Hiob *hp;
	ulong addr;
	Vpage *vp;

	wlock(&mainlock);	/* init */
	wunlock(&mainlock);

	/*
	 * Initialise the physical buffer pool.
	 * First the hash buckets.
	 */
	i = conf.mem - PADDR(ialloc(0, 0));
	i -= conf.sparemem;
	niob = i / (sizeof(Iobuf) + RBUFSIZE + sizeof(Hiob)/HWIDTH);
	nhiob = niob / HWIDTH;
	while(!prime(nhiob))
		nhiob++;
	print("	%ld buffers; %ld hashes\n", niob, nhiob);
	hiob = ialloc(nhiob * sizeof(Hiob), 0);
	hp = hiob;
	for(i=0; i<nhiob; i++) {
		lock(hp);
		unlock(hp);
		hp++;
	}

	/*
	 * Allocate the buffer pool.
	 * We'll initialise it later, once we've carved
	 * up the rest of memory.
	 */
	p = ialloc(niob * sizeof(Iobuf), 0);

	/*
	 * Allocate the spare memory. This is used for all
	 * further iallocs, including the viob array.
	 * After working out nviob from the remaining virtual
	 * space, reset the ialloc arena to be the just allocated
	 * spare memory.
	 */
	addr = (ulong)ialloc(conf.sparemem, PGSIZE);
	vpalloc.nvpage = (mconf.vlimit-(mconf.vbase+conf.sparemem))/RBUFSIZE;
	mconf.vlimit = mconf.vbase;
	mconf.vfract = mconf.vbase = addr;

	/*
	 * Initialise the iob buffer pool.
	 * All physical memory from the top of the sparemem ialloc
	 * arena to conf.mem is available.
	 * niob may be slightly reduced, so recompute.
	 */
	addr = PADDR(mconf.vlimit);
	hp = hiob;
	niob = (conf.mem-addr)/RBUFSIZE;
	for(i=0; i<niob; i++) {
		p->name = "buf";
		qlock(p);
		qunlock(p);
		if(hp == hiob)
			hp = hiob + nhiob;
		hp--;
		q = hp->link;
		if(q) {
			p->fore = q;
			p->back = q->back;
			q->back = p;
			p->back->fore = p;
		} else {
			hp->link = p;
			p->fore = p;
			p->back = p;
		}
		p->dev = devnone;
		p->addr = -1;
		p->xiobuf = (char*)addr;
		addr += RBUFSIZE;
		p->iobuf = (char*)-1;
		p++;
	}

	/*
	 * Initialise virtual buffer maps. Virtual space
	 * starts at the top of the sparemem ialloc arena.
	 */
	lock(&vpalloc);
	vpalloc.head = ialloc(vpalloc.nvpage*sizeof(Vpage), 0);
	vpalloc.hash = ialloc(vpalloc.nvpage*sizeof(Vpage*), 0);
	vp = vpalloc.head;
	for(addr = mconf.vlimit, i = 0; i < vpalloc.nvpage; i++, addr += RBUFSIZE){
		vp->prev = vp-1;
		vp->next = vp+1;
		vp->va = addr;
		vp++;
	}
	vpalloc.tail = vp - 1;
	vpalloc.head->prev = 0;
	vpalloc.tail->next = 0;
	vpalloc.freecount = vpalloc.nvpage;
	unlock(&vpalloc);

	print("	mem left = %ld\n", mconf.vlimit - mconf.vbase);
	print("		out of = %ld\n", conf.mem);

	cmd_install("vcache", "", cmd_vcache);
}

#define HASH(pa)	((((ulong)(pa))/RBUFSIZE) % vpalloc.nvpage)

Vpage*
lookvpage(Iobuf *p)
{
	Vpage *v;

	lock(&vpalloc);
	for(v = vpalloc.hash[HASH(p->xiobuf)]; v; v = v->hash){
		if(v->iobuf != p)
			continue;
		if(v->ref){
			cmd_vcache(0, 0);
			panic("lookvpage %d\n", v->ref);
		}
		if(v->prev) 
			v->prev->next = v->next;
		else
			vpalloc.head = v->next;

		if(v->next)
			v->next->prev = v->prev;
		else
			vpalloc.tail = v->prev;
		v->ref++;
		vpalloc.freecount--;
		vpalloc.hits++;
		unlock(&vpalloc);
		return v;	
	}
	unlock(&vpalloc);
	return 0;
}

void*
iobufmap(Iobuf *p)
{
	Vpage *v, **l, *f;

	/*
	 * Look in the cache.
	 */
	if(v = lookvpage(p)){
		p->iobuf = (char*)v->va;
		return p->iobuf;
	}

	/*
	 * Not found in the cache. Pull one off
	 * the free list.
	 */
	lock(&vpalloc);
	vpalloc.misses++;
	if(vpalloc.freecount == 0)
		panic("vpalloc.freecount == 0\n");

	v = vpalloc.head;
	if(vpalloc.head = v->next)
		vpalloc.head->prev = 0;
	else
		vpalloc.tail = 0;
	vpalloc.freecount--;

	/*
	 * Remove it from the cache if necessary.
	 */
	if(v->iobuf){
		vpalloc.reclaims++;
		l = &vpalloc.hash[HASH(v->iobuf->xiobuf)];
		for(f = *l; f; f = f->hash){
			if(f == v){
				*l = v->hash;
				v->iobuf = 0;
				break;
			}
			l = &f->hash;
		}

		/* Write back before we change the mapping */
		wbackcache((ulong)v->va, RBUFSIZE);

		if(v->iobuf)
			panic("vpage reclaim\n");
	}

	/*
	 * Set up the new vpage.
	 * Put it in the cache.
	 */
	v->ref++;
	v->iobuf = p;
	{
		ulong frame = PPN((ulong)p->xiobuf);
		if (frame < mconf.npage0)
			frame += PPN(mconf.base0);
		else
			frame += PPN(mconf.base1) - mconf.npage0;
		putpte(v->va, frame|PTEVALID|PTEKERNEL|PTEWRITE|PTEMAINMEM, 1);
	}

	l = &vpalloc.hash[HASH(p->xiobuf)];
	v->hash = *l;
	*l = v;

	unlock(&vpalloc);

	p->iobuf = (char*)v->va;
	return p->iobuf;
}

void
iobufunmap(Iobuf *p)
{
	Vpage *v;

	/*
	 * Put the buffer on the tail of the free list.
	 * Leave it in the cache.
	 */
	lock(&vpalloc);
	for(v = vpalloc.hash[HASH(p->xiobuf)]; v; v = v->hash){
		if(v->iobuf == p)
			break;
	}
	
	if(v == 0 || v->ref != 1)
		panic("iobufunmap %lux\n", v);
	if(vpalloc.tail){
		v->prev = vpalloc.tail;
		vpalloc.tail->next = v;
	}
	else{
		vpalloc.head = v;
		v->prev = 0;
	}
	vpalloc.tail = v;
	vpalloc.freecount++;
	v->next = 0;
	v->ref = 0;
	unlock(&vpalloc);

	p->iobuf = (char*)-1;
}

