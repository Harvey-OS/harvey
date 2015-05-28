/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum
{
	Nhole		= 128,
	Magichole	= 0x484F4C45,			/* HOLE */
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;

struct Hole
{
	uint32_t	addr;
	uint32_t	size;
	uint32_t	top;
	Hole*	link;
};

struct Xhdr
{
	uint32_t	size;
	uint32_t	magix;
	char	data[];
};

struct Xalloc
{
	Lock;
	Hole	hole[Nhole];
	Hole*	flist;
	Hole*	table;
};

static Xalloc	xlists;

void*
xallocz(uint32_t size, int zero)
{
	Xhdr *p;
	Hole *h, **l;

	/* add room for magix & size overhead, round up to nearest vlong */
	size += BY2V + offsetof(Xhdr, data[0]);
	size &= ~(BY2V-1);

	ilock(&xlists);
	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		if(h->size >= size) {
			p = (Xhdr*)KADDR(h->addr);
			h->addr += size;
			h->size -= size;
			if(h->size == 0) {
				*l = h->link;
				h->link = xlists.flist;
				xlists.flist = h;
			}
			iunlock(&xlists);
			if(zero)
				memset(p, 0, size);
			p->magix = Magichole;
			p->size = size;
			return p->data;
		}
		l = &h->link;
	}
	iunlock(&xlists);
	return nil;
}

void*
xalloc(uint32_t size)
{
	return xallocz(size, 1);
}

void
xhole(uintmem addr, uint32_t size)
{
	uint32_t top;
	Hole *h, *c, **l;

	if(size == 0)
		return;

	top = addr + size;
	ilock(&xlists);
	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		if(h->top == addr) {
			h->size += size;
			h->top = h->addr+h->size;
			c = h->link;
			if(c && h->top == c->addr) {
				h->top += c->size;
				h->size += c->size;
				h->link = c->link;
				c->link = xlists.flist;
				xlists.flist = c;
			}
			iunlock(&xlists);
			return;
		}
		if(h->addr > addr)
			break;
		l = &h->link;
	}
	if(h && top == h->addr) {
		h->addr -= size;
		h->size += size;
		iunlock(&xlists);
		return;
	}

	if(xlists.flist == nil) {
		iunlock(&xlists);
		print("xfree: no free holes, leaked %lud bytes\n", size);
		return;
	}

	h = xlists.flist;
	xlists.flist = h->link;
	h->addr = addr;
	h->top = top;
	h->size = size;
	h->link = *l;
	*l = h;
	iunlock(&xlists);
}

void
xsummary(void)
{
	int i;
	Hole *h;

	i = 0;
	for(h = xlists.flist; h; h = h->link)
		i++;

	print("%d holes free", i);
	i = 0;
	for(h = xlists.table; h; h = h->link) {
		if (0) {
			print("addr %#.8lux top %#.8lux size %lud\n",
				h->addr, h->top, h->size);
			delay(10);
		}
		i += h->size;
		if (h == h->link) {
			print("xsummary: infinite loop broken\n");
			break;
		}
	}
	print(" %d bytes free\n", i);
}

void*
xspanalloc(uint32_t size, int align, uint32_t span)
{
	uint64_t a, v, t;
	a = (uint64_t)xalloc(size+align+span);
	if(a == 0)
		panic("xspanalloc: %lud %d %lux", size, align, span);

	if(span > 2) {
		v = (a + span) & ~(span-1);
		t = v - a;
		if(t > 0)
			xhole(PADDR(UINT2PTR(a)), t);
		t = a + span - v;
		if(t > 0)
			xhole(PADDR(UINT2PTR(v+size+align)), t);
	}
	else
		v = a;

	if(align > 1)
		v = (v + align) & ~(align-1);

	return (void*)v;
}

#ifdef WTF
void
xinit(void)
{
	int i, n, upages, kpages;
	//uint32_t maxpages;
	Confmem *m;
	Pallocmem *pm;
	Hole *h, *eh;

	eh = &xlists.hole[Nhole-1];
	for(h = xlists.hole; h < eh; h++)
		h->link = h+1;

	xlists.flist = xlists.hole;

	upages = conf.upages;
	kpages = conf.npage - upages;
	pm = Palloc.mem;
	for(i=0; i<nelem(conf.mem); i++){
		m = &conf.mem[i];
		n = m->npage;
		if(n > kpages)
			n = kpages;
		/* don't try to use non-KADDR-able memory for kernel */
		//maxpages = cankaddr(m->base)/BIGPGSZ;
		//if(n > maxpages)
		//	n = maxpages;
		/* first give to kernel */
		if(n > 0){
			m->kbase = (uint64_t)KADDR(m->base);
			m->klimit = (uint64_t)KADDR(m->base+n*BIGPGSZ);
			xhole(m->base, n*BIGPGSZ);
			kpages -= n;
		}
		/* if anything left over, give to user */
		if(n < m->npage){
			if(pm >= Palloc.mem+nelem(Palloc.mem)){
				print("xinit: losing %lud pages\n", m->npage-n);
				continue;
			}
			pm->base = m->base+n*BIGPGSZ;
			pm->npage = m->npage - n;
			pm++;
		}
	}
//	xsummary();			/* call it from main if desired */
}
#endif

void
xfree(void *p)
{
	Xhdr *x;

	x = (Xhdr*)((uint64_t)p - offsetof(Xhdr, data[0]));
	if(x->magix != Magichole) {
		xsummary();
		panic("xfree(%#p) %#ux != %#lux", p, Magichole, x->magix);
	}
	xhole(PADDR(UINT2PTR((uint64_t)x)), x->size);
}

int
xmerge(void *vp, void *vq)
{
	Xhdr *p, *q;

	p = (Xhdr*)(((uint64_t)vp - offsetof(Xhdr, data[0])));
	q = (Xhdr*)(((uint64_t)vq - offsetof(Xhdr, data[0])));
	if(p->magix != Magichole || q->magix != Magichole) {
		int i;
		uint32_t *wd;
		void *badp;

		xsummary();
		badp = (p->magix != Magichole? p: q);
		wd = (uint32_t *)badp - 12;
		for (i = 24; i-- > 0; ) {
			print("%#p: %lux", wd, *wd);
			if (wd == badp)
				print(" <-");
			print("\n");
			wd++;
		}
		panic("xmerge(%#p, %#p) bad magic %#lux, %#lux",
			vp, vq, p->magix, q->magix);
	}
	if((unsigned char*)p+p->size == (unsigned char*)q) {
		p->size += q->size;
		return 1;
	}
	return 0;
}
