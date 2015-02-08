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

THIS FILE IS NOT USED FOR NIX.
I'm leaving it here, and making sure it does not compile. -nemo

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
	uintptr	addr;
	ulong	size;
	uintptr	top;
	Hole*	link;
};

struct Xhdr
{
	ulong	size;
	ulong	magix;
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

extern void* xalloc(ulong);
extern int xmerge(void*, void*);
extern void xhole(uintptr, ulong);
extern void xsummary(void);

void
xinit(void)
{
	int i, n, upages, kpages;
	ulong maxkpa;
	Confmem *m;
	Pallocmem *pm;
	Hole *h, *eh;

	eh = &xlists.hole[Nhole-1];
	for(h = xlists.hole; h < eh; h++)
		h->link = h+1;

	xlists.flist = xlists.hole;

	upages = conf.upages;
	kpages = conf.npage - upages;
	pm = palloc.mem;
	maxkpa = -KZERO;
	for(i=0; i<nelem(conf.mem); i++){
		m = &conf.mem[i];
		n = m->npage;
		if(n > kpages)
			n = kpages;
		if(m->base >= maxkpa)
			n = 0;
		else if(n > 0 && m->base+n*PGSZ >= maxkpa)
			n = (maxkpa - m->base)/PGSZ;
		/* first give to kernel */
		if(n > 0){
			m->kbase = PTR2UINT(KADDR(m->base));
			m->klimit = PTR2UINT(KADDR(m->base+n*PGSZ));
			xhole(m->base, n*PGSZ);
			kpages -= n;
		}
		/* if anything left over, give to user */
		if(n < m->npage){
			if(pm >= palloc.mem+nelem(palloc.mem)){
				print("xinit: losing %lud pages\n", m->npage-n);
				continue;
			}
			pm->base = m->base+n*PGSZ;
			pm->npage = m->npage - n;
			pm++;
		}
	}
	xsummary();
}

void*
xspanalloc(ulong size, int align, ulong span)
{
	uintptr a, v, t;

	a = PTR2UINT(xalloc(size+align+span));
	if(a == 0)
		panic("xspanalloc: %lud %d %lux\n", size, align, span);

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

void*
xallocz(ulong size, int zero)
{
	Xhdr *p;
	Hole *h, **l;

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
				memset(p->data, 0, size);
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
xalloc(ulong size)
{
	return xallocz(size, 1);
}

void
xfree(void *p)
{
	Xhdr *x;

	x = UINT2PTR((PTR2UINT(p) - offsetof(Xhdr, data[0])));
	if(x->magix != Magichole) {
		xsummary();
		panic("xfree(%#p) %#ux != %#lux", p, Magichole, x->magix);
	}
	xhole(PADDR(x), x->size);
}

int
xmerge(void *vp, void *vq)
{
	Xhdr *p, *q;

	p = UINT2PTR((PTR2UINT(vp) - offsetof(Xhdr, data[0])));
	q = UINT2PTR((PTR2UINT(vq) - offsetof(Xhdr, data[0])));
	if(p->magix != Magichole || q->magix != Magichole) {
		xsummary();
		panic("xmerge(%#p, %#p) bad magic %#lux, %#lux\n",
			vp, vq, p->magix, q->magix);
	}
	if((uchar*)p+p->size == (uchar*)q) {
		p->size += q->size;
		return 1;
	}
	return 0;
}

void
xhole(uintptr addr, ulong size)
{
	uintptr top;
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

	print("%d holes free\n", i);
	i = 0;
	for(h = xlists.table; h; h = h->link) {
		print("%.8p %.8p %lud\n", h->addr, h->top, h->size);
		i += h->size;
	}
	print("%d bytes free\n", i);
}
