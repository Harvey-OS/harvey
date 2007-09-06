#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "packet.h"

static Frag *fragAlloc(Packet*, int n, int pos, Frag *next);
static Frag *fragDup(Packet*, Frag*);
static void fragFree(Frag*);

static Mem *memAlloc(int, int);
static void memFree(Mem*);
static int memHead(Mem *m, uchar *rp, int n);
static int memTail(Mem *m, uchar *wp, int n);

static char EPacketSize[] = "bad packet size";
static char EPacketOffset[] = "bad packet offset";
static char EBadSize[] = "bad size";

static struct {
	Lock lk;
	Packet *packet;
	int npacket;
	Frag *frag;
	int nfrag;
	Mem *bigMem;
	int nbigMem;
	Mem *smallMem;
	int nsmallMem;
} freeList;

#define FRAGSIZE(f) ((f)->wp - (f)->rp)
#define FRAGASIZE(f) ((f)->mem->ep - (f)->mem->bp)

#define NOTFREE(p) assert((p)->size>=0)

Packet *
packetAlloc(void)
{
	Packet *p;
	
	lock(&freeList.lk);
	p = freeList.packet;
	if(p != nil)
		freeList.packet = p->next;
	else
		freeList.npacket++;
	unlock(&freeList.lk);

	if(p == nil)
		p = vtMemBrk(sizeof(Packet));
	else
		assert(p->size == -1);
	p->size = 0;
	p->asize = 0;
	p->first = nil;
	p->last = nil;
	p->next = nil;

	return p;
}

void
packetFree(Packet *p)
{
	Frag *f, *ff;

if(0)fprint(2, "packetFree %p\n", p);

	NOTFREE(p);
	p->size = -1;

	for(f=p->first; f!=nil; f=ff) {
		ff = f->next;
		fragFree(f);
	}
	p->first = nil;
	p->last = nil;

	lock(&freeList.lk);
	p->next = freeList.packet;
	freeList.packet = p;
	unlock(&freeList.lk);
}

Packet *
packetDup(Packet *p, int offset, int n)
{	
	Frag *f, *ff;
	Packet *pp;

	NOTFREE(p);
	if(offset < 0 || n < 0 || offset+n > p->size) {
		vtSetError(EBadSize);
		return nil;
	}

	pp = packetAlloc();
	if(n == 0)
		return pp;

	pp->size = n;

	/* skip offset */
	for(f=p->first; offset >= FRAGSIZE(f); f=f->next)
		offset -= FRAGSIZE(f);
	
	/* first frag */
	ff = fragDup(pp, f);
	ff->rp += offset;
	pp->first = ff;
	n -= FRAGSIZE(ff);
	pp->asize += FRAGASIZE(ff);

	/* the remaining */
	while(n > 0) {
		f = f->next;
		ff->next = fragDup(pp, f);
		ff = ff->next;
		n -= FRAGSIZE(ff);
		pp->asize += FRAGASIZE(ff);
	}
	
	/* fix up last frag: note n <= 0 */
	ff->wp += n;
	ff->next = nil;
	pp->last = ff;

	return pp;
}

Packet *
packetSplit(Packet *p, int n)
{
	Packet *pp;
	Frag *f, *ff;

	NOTFREE(p);
	if(n < 0 || n > p->size) {
		vtSetError(EPacketSize);
		return nil;
	}

	pp = packetAlloc();
	if(n == 0)
		return pp;

	pp->size = n;
	p->size -= n;
	ff = nil;
	for(f=p->first; n > 0 && n >= FRAGSIZE(f); f=f->next) {
		n -= FRAGSIZE(f);
		p->asize -= FRAGASIZE(f);
		pp->asize += FRAGASIZE(f);
		ff = f;
	}

	/* split shared frag */
	if(n > 0) {
		ff = f;
		f = fragDup(pp, ff);
		pp->asize += FRAGASIZE(ff);
		ff->next = nil;
		ff->wp = ff->rp + n;
		f->rp += n;
	}

	pp->first = p->first;
	pp->last = ff;
	p->first = f;
	return pp;
}

int
packetConsume(Packet *p, uchar *buf, int n)
{
	NOTFREE(p);
	if(buf && !packetCopy(p, buf, 0, n))
		return 0;
	return packetTrim(p, n, p->size-n);
}

int
packetTrim(Packet *p, int offset, int n)
{
	Frag *f, *ff;

	NOTFREE(p);
	if(offset < 0 || offset > p->size) {
		vtSetError(EPacketOffset);
		return 0;
	}

	if(n < 0 || offset + n > p->size) {
		vtSetError(EPacketOffset);
		return 0;
	}

	p->size = n;

	/* easy case */
	if(n == 0) {
		for(f=p->first; f != nil; f=ff) {
			ff = f->next;
			fragFree(f);
		}
		p->first = p->last = nil;
		p->asize = 0;
		return 1;
	}
	
	/* free before offset */
	for(f=p->first; offset >= FRAGSIZE(f); f=ff) {
		p->asize -= FRAGASIZE(f);
		offset -= FRAGSIZE(f);
		ff = f->next;
		fragFree(f);
	}

	/* adjust frag */
	f->rp += offset;
	p->first = f;

	/* skip middle */
	for(; n > 0 && n > FRAGSIZE(f); f=f->next)
		n -= FRAGSIZE(f);

	/* adjust end */
	f->wp = f->rp + n;
	p->last = f;
	ff = f->next;
	f->next = nil;

	/* free after */
	for(f=ff; f != nil; f=ff) {
		p->asize -= FRAGASIZE(f);
		ff = f->next;
		fragFree(f);
	}
	return 1;
}

uchar *
packetHeader(Packet *p, int n)
{
	Frag *f;
	Mem *m;

	NOTFREE(p);
	if(n <= 0 || n > MaxFragSize) {
		vtSetError(EPacketSize);
		return 0;
	}

	p->size += n;
	
	/* try and fix in current frag */
	f = p->first;
	if(f != nil) {
		m = f->mem;
		if(n <= f->rp - m->bp)
		if(m->ref == 1 || memHead(m, f->rp, n)) {
			f->rp -= n;
			return f->rp;
		}
	}

	/* add frag to front */
	f = fragAlloc(p, n, PEnd, p->first);
	p->asize += FRAGASIZE(f);
	if(p->first == nil)
		p->last = f;
	p->first = f;
	return f->rp;
}

uchar *
packetTrailer(Packet *p, int n)
{
	Mem *m;
	Frag *f;

	NOTFREE(p);
	if(n <= 0 || n > MaxFragSize) {
		vtSetError(EPacketSize);
		return 0;
	}

	p->size += n;
	
	/* try and fix in current frag */
	if(p->first != nil) {
		f = p->last;
		m = f->mem;
		if(n <= m->ep - f->wp)
		if(m->ref == 1 || memTail(m, f->wp, n)) {
			f->wp += n;
			return f->wp - n;
		}
	}

	/* add frag to end */
	f = fragAlloc(p, n, (p->first == nil)?PMiddle:PFront, nil);
	p->asize += FRAGASIZE(f);
	if(p->first == nil)
		p->first = f;
	else
		p->last->next = f;
	p->last = f;
	return f->rp;
}

int
packetPrefix(Packet *p, uchar *buf, int n)
{
	Frag *f;
	int nn;
	Mem *m;

	NOTFREE(p);
	if(n <= 0)
		return 1;

	p->size += n;

	/* try and fix in current frag */
	f = p->first;
	if(f != nil) {
		m = f->mem;
		nn = f->rp - m->bp;
		if(nn > n)
			nn = n;
		if(m->ref == 1 || memHead(m, f->rp, nn)) {
			f->rp -= nn;
			n -= nn;
			memmove(f->rp, buf+n, nn);
		}
	}

	while(n > 0) {
		nn = n;
		if(nn > MaxFragSize)
			nn = MaxFragSize;
		f = fragAlloc(p, nn, PEnd, p->first);	
		p->asize += FRAGASIZE(f);
		if(p->first == nil)
			p->last = f;
		p->first = f;
		n -= nn;
		memmove(f->rp, buf+n, nn);
	}
	return 1;
}

int
packetAppend(Packet *p, uchar *buf, int n)
{
	Frag *f;
	int nn;
	Mem *m;

	NOTFREE(p);
	if(n <= 0)
		return 1;

	p->size += n;
	/* try and fix in current frag */
	if(p->first != nil) {
		f = p->last;
		m = f->mem;
		nn = m->ep - f->wp;
		if(nn > n)
			nn = n;
		if(m->ref == 1 || memTail(m, f->wp, nn)) {
			memmove(f->wp, buf, nn);
			f->wp += nn;
			buf += nn;
			n -= nn;
		}
	}
	
	while(n > 0) {
		nn = n;
		if(nn > MaxFragSize)
			nn = MaxFragSize;
		f = fragAlloc(p, nn, (p->first == nil)?PMiddle:PFront, nil);
		p->asize += FRAGASIZE(f);
		if(p->first == nil)
			p->first = f;
		else
			p->last->next = f;
		p->last = f;
		memmove(f->rp, buf, nn);
		buf += nn;
		n -= nn;
	}
	return 1;
}

int
packetConcat(Packet *p, Packet *pp)
{
	NOTFREE(p);
	NOTFREE(pp);
	if(pp->size == 0)
		return 1;
	p->size += pp->size;
	p->asize += pp->asize;

	if(p->first != nil)
		p->last->next = pp->first;
	else
		p->first = pp->first;
	p->last = pp->last;
	pp->size = 0;
	pp->asize = 0;
	pp->first = nil;
	pp->last = nil;
	return 1;
}

uchar *
packetPeek(Packet *p, uchar *buf, int offset, int n)
{
	Frag *f;
	int nn;
	uchar *b;

	NOTFREE(p);
	if(n == 0)
		return buf;

	if(offset < 0 || offset >= p->size) {
		vtSetError(EPacketOffset);
		return 0;
	}

	if(n < 0 || offset + n > p->size) {
		vtSetError(EPacketSize);
		return 0;
	}
	
	/* skip up to offset */
	for(f=p->first; offset >= FRAGSIZE(f); f=f->next)
		offset -= FRAGSIZE(f);

	/* easy case */
	if(offset + n <= FRAGSIZE(f))
		return f->rp + offset;

	for(b=buf; n>0; n -= nn) {
		nn = FRAGSIZE(f) - offset;
		if(nn > n)
			nn = n;
		memmove(b, f->rp+offset, nn);
		offset = 0;
		f = f->next;
		b += nn;
	}

	return buf;
}

int
packetCopy(Packet *p, uchar *buf, int offset, int n)
{
	uchar *b;

	NOTFREE(p);
	b = packetPeek(p, buf, offset, n);
	if(b == nil)
		return 0;
	if(b != buf)
		memmove(buf, b, n);
	return 1;
}

int
packetFragments(Packet *p, IOchunk *io, int nio, int offset)
{
	Frag *f;
	int size;
	IOchunk *eio;

	NOTFREE(p);
	if(p->size == 0 || nio <= 0)
		return 0;
	
	if(offset < 0 || offset > p->size) {
		vtSetError(EPacketOffset);
		return -1;
	}

	for(f=p->first; offset >= FRAGSIZE(f); f=f->next)
		offset -= FRAGSIZE(f);

	size = 0;
	eio = io + nio;
	for(; f != nil && io < eio; f=f->next) {
		io->addr = f->rp + offset;
		io->len = f->wp - (f->rp + offset);	
		offset = 0;
		size += io->len;
		io++;
	}

	return size;
}

void
packetStats(void)
{
	Packet *p;
	Frag *f;
	Mem *m;

	int np, nf, nsm, nbm;

	lock(&freeList.lk);
	np = 0;
	for(p=freeList.packet; p; p=p->next)
		np++;
	nf = 0;
	for(f=freeList.frag; f; f=f->next)
		nf++;
	nsm = 0;
	for(m=freeList.smallMem; m; m=m->next)
		nsm++;
	nbm = 0;
	for(m=freeList.bigMem; m; m=m->next)
		nbm++;
	
	fprint(2, "packet: %d/%d frag: %d/%d small mem: %d/%d big mem: %d/%d\n",
		np, freeList.npacket,
		nf, freeList.nfrag,
		nsm, freeList.nsmallMem,
		nbm, freeList.nbigMem);

	unlock(&freeList.lk);
}


int
packetSize(Packet *p)
{
	NOTFREE(p);
	if(0) {
		Frag *f;
		int size = 0;
	
		for(f=p->first; f; f=f->next)
			size += FRAGSIZE(f);
		if(size != p->size)
			fprint(2, "packetSize %d %d\n", size, p->size);
		assert(size == p->size);
	}
	return p->size;
}

int
packetAllocatedSize(Packet *p)
{
	NOTFREE(p);
	if(0) {
		Frag *f;
		int asize = 0;
	
		for(f=p->first; f; f=f->next)
			asize += FRAGASIZE(f);
		if(asize != p->asize)
			fprint(2, "packetAllocatedSize %d %d\n", asize, p->asize);
		assert(asize == p->asize);
	}
	return p->asize;
}

void
packetSha1(Packet *p, uchar sha1[VtScoreSize])
{
	Frag *f;
	VtSha1 *s;
	int size;

	NOTFREE(p);
	s = vtSha1Alloc();
	size = p->size;
	for(f=p->first; f; f=f->next) {
		vtSha1Update(s, f->rp, FRAGSIZE(f));
		size -= FRAGSIZE(f);
	}
	assert(size == 0);
	vtSha1Final(s, sha1);
	vtSha1Free(s);
}

int
packetCmp(Packet *pkt0, Packet *pkt1)
{
	Frag *f0, *f1;
	int n0, n1, x;

	NOTFREE(pkt0);
	NOTFREE(pkt1);
	f0 = pkt0->first;
	f1 = pkt1->first;

	if(f0 == nil)
		return (f1 == nil)?0:-1;
	if(f1 == nil)
		return 1;
	n0 = FRAGSIZE(f0);
	n1 = FRAGSIZE(f1);

	for(;;) {
		if(n0 < n1) {
			x = memcmp(f0->wp - n0, f1->wp - n1, n0);
			if(x != 0)
				return x;
			n1 -= n0;
			f0 = f0->next;
			if(f0 == nil)
				return -1;
			n0 = FRAGSIZE(f0);
		} else if (n0 > n1) {
			x = memcmp(f0->wp - n0, f1->wp - n1, n1);
			if(x != 0)
				return x;
			n0 -= n1;
			f1 = f1->next;
			if(f1 == nil)
				return 1;
			n1 = FRAGSIZE(f1);
		} else { /* n0 == n1 */
			x = memcmp(f0->wp - n0, f1->wp - n1, n0);
			if(x != 0)
				return x;
			f0 = f0->next;
			f1 = f1->next;
			if(f0 == nil)
				return (f1 == nil)?0:-1;
			if(f1 == nil)
				return 1;
			n0 = FRAGSIZE(f0);
			n1 = FRAGSIZE(f1);
		}
	}
}
	

static Frag *
fragAlloc(Packet *p, int n, int pos, Frag *next)
{
	Frag *f, *ef;
	Mem *m;

	/* look for local frag */
	f = &p->local[0];
	ef = &p->local[NLocalFrag];
	for(;f<ef; f++) {
		if(f->state == FragLocalFree) {
			f->state = FragLocalAlloc;
			goto Found;
		}
	}
	lock(&freeList.lk);	
	f = freeList.frag;
	if(f != nil)
		freeList.frag = f->next;
	else
		freeList.nfrag++;
	unlock(&freeList.lk);

	if(f == nil) {
		f = vtMemBrk(sizeof(Frag));
		f->state = FragGlobal;
	}

Found:
	if(n == 0)
		return f;

	if(pos == PEnd && next == nil)
		pos = PMiddle;
	m = memAlloc(n, pos);
	f->mem = m;
	f->rp = m->rp;
	f->wp = m->wp;
	f->next = next;

	return f;
}

static Frag *
fragDup(Packet *p, Frag *f)
{
	Frag *ff;
	Mem *m;

	m = f->mem;	

	/*
	 * m->rp && m->wp can be out of date when ref == 1
	 * also, potentially reclaims space from previous frags
	 */
	if(m->ref == 1) {
		m->rp = f->rp;
		m->wp = f->wp;
	}

	ff = fragAlloc(p, 0, 0, nil);
	*ff = *f;
	lock(&m->lk);
	m->ref++;
	unlock(&m->lk);
	return ff;
}


static void
fragFree(Frag *f)
{
	memFree(f->mem);

	if(f->state == FragLocalAlloc) {
		f->state = FragLocalFree;
		return;
	}

	lock(&freeList.lk);
	f->next = freeList.frag;
	freeList.frag = f;
	unlock(&freeList.lk);	
}

static Mem *
memAlloc(int n, int pos)
{
	Mem *m;
	int nn;

	if(n < 0 || n > MaxFragSize) {
		vtSetError(EPacketSize);
		return 0;
	}
	if(n <= SmallMemSize) {
		lock(&freeList.lk);
		m = freeList.smallMem;
		if(m != nil)
			freeList.smallMem = m->next;
		else
			freeList.nsmallMem++;
		unlock(&freeList.lk);
		nn = SmallMemSize;
	} else {
		lock(&freeList.lk);
		m = freeList.bigMem;
		if(m != nil)
			freeList.bigMem = m->next;
		else
			freeList.nbigMem++;
		unlock(&freeList.lk);
		nn = BigMemSize;
	}

	if(m == nil) {
		m = vtMemBrk(sizeof(Mem));
		m->bp = vtMemBrk(nn);
		m->ep = m->bp + nn;
	}
	assert(m->ref == 0);	
	m->ref = 1;

	switch(pos) {
	default:
		assert(0);
	case PFront:
		m->rp = m->bp;
		break;
	case PMiddle:
		/* leave a little bit at end */
		m->rp = m->ep - n - 32;
		break;
	case PEnd:
		m->rp = m->ep - n;
		break; 
	}
	/* check we did not blow it */
	if(m->rp < m->bp)
		m->rp = m->bp;
	m->wp = m->rp + n;
	assert(m->rp >= m->bp && m->wp <= m->ep);
	return m;
}

static void
memFree(Mem *m)
{
	lock(&m->lk);
	m->ref--;
	if(m->ref > 0) {
		unlock(&m->lk);
		return;
	}
	unlock(&m->lk);
	assert(m->ref == 0);

	switch(m->ep - m->bp) {
	default:
		assert(0);
	case SmallMemSize:
		lock(&freeList.lk);
		m->next = freeList.smallMem;
		freeList.smallMem = m;
		unlock(&freeList.lk);
		break;
	case BigMemSize:
		lock(&freeList.lk);
		m->next = freeList.bigMem;
		freeList.bigMem = m;
		unlock(&freeList.lk);
		break;
	}
}

static int
memHead(Mem *m, uchar *rp, int n)
{
	lock(&m->lk);
	if(m->rp != rp) {
		unlock(&m->lk);
		return 0;
	}
	m->rp -= n;
	unlock(&m->lk);
	return 1;
}

static int
memTail(Mem *m, uchar *wp, int n)
{
	lock(&m->lk);
	if(m->wp != wp) {
		unlock(&m->lk);
		return 0;
	}
	m->wp += n;
	unlock(&m->lk);
	return 1;
}
