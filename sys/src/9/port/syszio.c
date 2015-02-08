/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * Experiment on zero-copy
 *
 * Each address in a Zio slot implies a reference
 * counter for that buffer. Provided the address,
 * we must be able to get to the counter.
 * We can use shared segments with fixed message sizes per
 * segment, so we can do arithmetic to locate the counter.
 * We could also use per-page reference counters, and perhaps
 * accept any user pointer.
 * If the kernel supplies the buffers, it must allocate them
 * from a place available for the user, perhaps a heap segment
 * or something like that.
 */

enum
{
	Maxatomic = 64*KiB
};

typedef struct ZMap ZMap;
typedef struct Map Map;

struct Map {
	Map* next;
	int free;
	uintptr	addr;
	uvlong	size;
};

struct ZMap {
	Map*	map;
	Lock;
};

static int inited;

static void zmapfree(ZMap* rmap, uintptr addr);
static uintptr zmapalloc(ZMap* rmap, usize size);

static void
zioinit(void)
{
	if(inited)
		return;
	inited++;
	fmtinstall('Z', ziofmt);
}

int
ziofmt(Fmt *f)
{
	Kzio *io;

	io = va_arg(f->args, Kzio*);
	return fmtprint(f, "%#p[%#ulx]", io->data, io->size);
}

static void
dumpzmap(ZMap *map)
{
	Map *mp;
	for(mp = map->map; mp != nil; mp = mp->next)
		print("\tmap %#ullx[%#ullx] %c\n", mp->addr, mp->size,
			mp->free ? 'f' : 'a');
}

/*
 * No locks!
 */
void
dumpzseg(Segment *s)
{
	Zseg *zs;
	ZMap *map;
	int i;

	if(DBGFLG == 0)
		return;

	zs = &s->zseg;
	print("zseg %#ullx type %#ux map %#p naddr %d end %d\n",
		s->base, s->type, zs->map, zs->naddr, zs->end);
	if(zs->addr != nil)
		for(i = 0; i < zs->end; i++)
			print("\taddr %#ullx\n", zs->addr[i]);
	map = zs->map;
	if(map == nil)
		return;
	dumpzmap(map);
}

/*
 * Called from putseg, when the segment is being destroyed.
 */
void
freezseg(Segment *s)
{
	Zseg *zs;
	ZMap *zp;
	Map *mp;

	DBG("freezseg: ");
	dumpzseg(s);
	zs = &s->zseg;
	zp = zs->map;
	if(zp == nil)
		return;
	while(zp->map != nil){
		mp = zp->map;
		zp->map = mp->next;
		free(mp);
	}
	free(zp);
}

/*
 * Grow the pool of addresses in s's zseg, s is qlocked
 */
void
zgrow(Segment *s)
{
	enum{Incr = 32};
	Zseg *zs;

	zioinit();
	zs = &s->zseg;
	zs->naddr += Incr;
	zs->addr = realloc(zs->addr, zs->naddr*sizeof(uintptr));
	if(zs->addr == nil)
		panic("zgrow: no memory");
}

/*
 * Find an address in s's zseg; s is qlocked
 */
uintptr
zgetaddr(Segment *s)
{
	Zseg *zs;
	uintptr va;

	zs = &s->zseg;
	if(zs->end == 0)
		return 0ULL;
	va = zs->addr[0];
	zs->end--;
	if(zs->end > 0)
		zs->addr[0] = zs->addr[zs->end];
	DBG("zgetaddr: %#ullx\n", va);
	dumpzseg(s);
	return va;
}

/*
 * add an address to s's zseg; s is qlocked.
 * wakeup any reader if it's waiting.
 */
int
zputaddr(Segment *s, uintptr va)
{
	Zseg *zs;

	zs = &s->zseg;
	if((s->type&SG_ZIO) == 0)
		return -1;
	if((s->type&SG_KZIO) != 0){
		DBG("zputaddr: zmapfree %#ullx\n", va);
		zmapfree(s->zseg.map, va);
		dumpzseg(s);
		return 0;
	}
	if(zs->end == zs->naddr)
		zgrow(s);
	zs->addr[zs->end++] = va;
	if(zs->end == 1)
		wakeup(&zs->rr);	/* in case anyone was waiting */
	DBG("zputaddr %#ullx\n", va);
	dumpzseg(s);
	return 0;
}

void*
alloczio(Segment *s, long len)
{
	Zseg *zs;
	uintptr va;

	zs = &s->zseg;
	va = zmapalloc(zs->map, len);
	if(va == 0ULL)
		error("kernel zero copy segment exhausted");
	return UINT2PTR(va);
}

/*
 * Locate the kernel segment for zero copy here,
 * return it unlocked with a reference added.
 */
Segment*
getzkseg(void)
{
	Segment *s;
	int i;

	qlock(&up->seglock);
	for(i = 0; i < NSEG; i++){
		s = up->seg[i];
		if(s != nil && (s->type&SG_KZIO) != 0){
			incref(s);
			qunlock(&up->seglock);
			DBG("getzkseg: %#p\n", s);
			return s;
		}
	}
	qunlock(&up->seglock);
	DBG("getzkseg: nil\n");
	return nil;
}

/*
 * This is the counterpart of devzread in some sense,
 * it reads in the traditional way from io[].
 */
long
readzio(Kzio *io, int nio, void *a, long count)
{
	long tot, nr;
	char *p;

	p = a;
	tot = 0;
	while(nio-- > 0){
		if(tot < count){
			nr = io->size;
			if(tot + nr > count)
				nr = count - tot;
			DBG("readzio: copy %#p %Z\n", p+tot, io);
			memmove(p+tot, io->data, nr);
			tot += nr;
		}
		qlock(&io->seg->lk);
		zputaddr(io->seg, PTR2UINT(io->data));
		qunlock(&io->seg->lk);
		putseg(io->seg);
		io->seg = nil;
		io++;
	}
	return tot;
}

int
devzread(Chan *c, Kzio io[], int nio, usize tot, vlong offset)
{
	Segment *s;

	DBG("devzread %#p[%d]\n", io, nio);

	s = getzkseg();
	if(s == nil)
		error("no kernel segment for zero-copy");
	if(tot > Maxatomic)
		tot = Maxatomic;
	io[0].data = alloczio(s, tot);
	io[0].seg = s;
	if(waserror()){
		zputaddr(s, PTR2UINT(io[0].data));
		putseg(s);
		nexterror();
	}
	io[0].size = c->dev->read(c, io[0].data, tot, offset);
	poperror();
	return 1;
}

int
devzwrite(Chan *c, Kzio io[], int nio, vlong offset)
{
	int i, j;
	long tot;
	Block *bp;

	DBG("devzwrite %#p[%d]\n", io, nio);

	tot = 0;
	for(i = 0; i < nio; i++)
		tot += io[i].size;
	bp = nil;
	if(waserror()){
		if(bp != nil)
			freeb(bp);
		nexterror();
	}
	if(nio == 1)
		tot = c->dev->write(c, io[0].data, io[0].size, offset);
	else{
		bp = allocb(tot);
		if(bp == nil)
			error(Enomem);
		for(i = 0; i < nio; i++){
			DBG("devzwrite: copy %#p %Z\n", bp->wp, &io[i]);
			memmove(bp->wp, io[i].data, io[i].size);
			bp->wp += io[i].size;
			qlock(&io[i].seg->lk);
			if(zputaddr(io[i].seg, PTR2UINT(io[i].data)) < 0)
				panic("devzwrite: not a shared data segment");
			qunlock(&io[i].seg->lk);
		}
		tot = c->dev->bwrite(c, bp, offset);
	}
	j = 0;
	for(i = 0; i < nio; i++){
		io[i].data = nil; /* safety */
		io[i].seg = nil;
		putseg(io[i].seg);
		if(tot > 0)
			if(tot >= io[i].size)
				tot -= io[i].size;
			else
				io[i].size = tot;
		else{
			j = i;
			io[i].size = 0;
		}
		io[i].data = nil; /* safety */
		putseg(io[i].seg);
		io[i].seg = nil;
	}
	nio = j;
	poperror();
	return nio;
}

static void
kernzio(Kzio *io)
{
	Segment *s;
	void *data;
	Kzio uio;

	s = getzkseg();
	if(s == nil)
		error("can't use zero copy in this segment");
	uio = *io;
	data = alloczio(s, io->size);
	memmove(data, io->data, io->size);
	io->data = data;
	DBG("kernzio: copy %Z %Z\n", io, &uio);
	putseg(io->seg);
	io->seg = s;
}

/*
 * Zero copy I/O.
 * I/O is performed using an array of Zio structures.
 * Each one points to a shared buffer address indicating a length.
 * Each entry indicating a length and using nil as the address
 * is asking the system to allocate memory as needed (mread only).
 */
static int
ziorw(int fd, Zio *io, int nio, usize count, vlong offset, int iswrite)
{
	int i, n, isprw;
	Kzio *kio, skio[16];
	Chan *c;
	usize tot;

	if(nio <= 0 || nio > 512)
		error("wrong io[] size");
	zioinit();
	kio = nil;

	io = validaddr(io, sizeof io[0] * nio, 1);
	DBG("ziorw %d io%#p[%d] %uld %lld\n", fd, io, nio, count, offset);
	if(DBGFLG)
		for(i = 0; i < nio; i++)
			print("\tio%#p[%d] = %Z %s\n",
				io, i, (Kzio*)&io[i], iswrite?"w":"r");

	if(iswrite)
		c = fdtochan(fd, OWRITE, 1, 1);
	else
		c = fdtochan(fd, OREAD, 1, 1);
	isprw = offset != -1LL;
	if(isprw)
		offset = c->offset;
	if(waserror()){
		cclose(c);
		if(kio != nil){
			for(i = 0; i < nio; i++)
				if(kio[i].seg != nil)
					putseg(kio[i].seg);
			if(kio != skio)
				free(kio);
		}
		nexterror();
	}
	if(nio < nelem(skio))
		kio = skio;
	else
		kio = smalloc(sizeof kio[0] * nio);
	for(i = 0; i < nio; i++){
		kio[i].Zio = io[i];
		if(iswrite){
			kio[i].seg = seg(up, PTR2UINT(io[i].data), 1);
			if(kio[i].seg == nil)
				error("invalid address in zio");
			incref(kio[i].seg);
			qunlock(&kio[i].seg->lk);
			validaddr(kio[i].data, kio[i].size, 1);
			if((kio[i].seg->type&SG_ZIO) == 0){
				/*
				 * It's not a segment where we can report
				 * addresses to anyone once they are free.
				 * So, allocate space in the kernel
				 * and copy the user data there.
				 */
				kernzio(&kio[i]);
			}
			assert(kio[i].seg->type&SG_ZIO);
		}else{
			kio[i].data = nil;
			kio[i].seg = nil;
		}
	}

	if(c->dev->zread == nil){
		DBG("installing devzread for %s\n", c->dev->name);
		c->dev->zread = devzread;
	}
	if(c->dev->zwrite == nil){
		DBG("installing devzwrite for %s\n", c->dev->name);
		c->dev->zwrite = devzwrite;
	}
	if(iswrite)
		n = c->dev->zwrite(c, kio, nio, offset);
	else
		n = c->dev->zread(c, kio, nio, count, offset);
	tot = 0;
	for(i = 0; i < n; i++){
		io[i] = kio[i].Zio;
		tot += kio[i].size;
	}
	if(!isprw){
		/* unlike in syswrite, we update offsets at the end */
		lock(c);
		c->devoffset += tot;
		c->offset += tot;
		unlock(c);
	}
	poperror();
	cclose(c);
	if(kio != skio)
		free(kio);
	return n;
}

void
sysziopread(Ar0 *ar0, va_list list)
{
	int fd, nio;
	long count;
	vlong offset;
	Zio *io;

	/*
	 * int zpread(int fd, Zio *io[], int nio, usize count, vlong offset);
	 */
	fd = va_arg(list, int);
	io = va_arg(list, Zio*);
	nio = va_arg(list, int);
	count = va_arg(list, usize);
	offset = va_arg(list, vlong);
	ar0->i = ziorw(fd, io, nio, count, offset, 0);
}

void
sysziopwrite(Ar0 *ar0, va_list list)
{
	int fd, nio;
	vlong offset;
	Zio *io;

	/*
	 * int zpwrite(int fd, Zio *io[], int nio, vlong offset);
	 */
	fd = va_arg(list, int);
	io = va_arg(list, Zio*);
	nio = va_arg(list, int);
	offset = va_arg(list, vlong);
	ar0->i = ziorw(fd, io, nio, 0, offset, 1);
}

void
sysziofree(Ar0 *, va_list list)
{
	Zio *io;
	int nio, i;
	Segment *s;

	/*
	 * zfree(Zio io[], int nio);
	 */
	io = va_arg(list, Zio*);
	nio = va_arg(list, int);
	io = validaddr(io, sizeof io[0] * nio, 1);
	for(i = 0; i < nio; i++){
		s = seg(up, PTR2UINT(io[i].data), 1);
		if(s == nil)
			error("invalid address in zio");
		if((s->type&SG_ZIO) == 0){
			qunlock(&s->lk);
			error("segment is not a zero-copy segment");
		}
		zputaddr(s, PTR2UINT(io[i].data));
		qunlock(&s->lk);
		io[i].data = nil;
		io[i].size = 0;
	}
}

/*
 * This must go, but for now, we use Zmaps
 * to allocate messages within the shared kernel segment.
 * This is a simple first fist with a single fragment list.
 */

void
newzmap(Segment *s)
{
	ZMap *zp;
	Map *mp;

	zioinit();
	if((s->type&SG_KZIO) == 0)
		panic("newzmap but not SG_KZIO");
	if(s->zseg.map != nil)
		panic("newzmap: already allocated");
	zp = smalloc(sizeof(ZMap));
	s->zseg.map = zp;
	mp = smalloc(sizeof(Map));
	mp->free = 1;
	mp->addr = s->base;
	mp->size = s->top - s->base;
	zp->map = mp;
	if(DBGFLG > 1){
		DBG("newzmap:\n");
		dumpzmap(zp);
	}
}

static void
zmapfree(ZMap* rmap, uintptr addr)
{
	Map *mp, *prev, *next;

	lock(rmap);
	if(waserror()){
		unlock(rmap);
		nexterror();
	}
	prev = nil;
	for(mp = rmap->map; mp != nil; mp = mp->next){
		if(mp->addr <= addr)
			break;
		prev = mp;
	}
	if(mp == nil)
		panic("zmapfree: no map");
	if(mp->free == 1)
		panic("zmapfree: already free");
	if(prev != nil && prev->free && prev->addr + prev->size == addr){
		prev->size += mp->size;
		prev->next = mp->next;
		free(mp);
		mp = prev;
	}
	next = mp->next;
	if(next != nil && next->free && mp->addr + mp->size == next->addr){
		mp->size += next->size;
		mp->next = next->next;
		mp->free = 1;
		free(next);
	}
	poperror();
	unlock(rmap);
	if(DBGFLG > 1){
		DBG("zmapfree %#ullx:\n", addr);
		dumpzmap(rmap);
	}
}

static uintptr
zmapalloc(ZMap* rmap, usize size)
{
	Map *mp, *nmp;

	lock(rmap);
	if(waserror()){
		unlock(rmap);
		nexterror();
	}
	for(mp = rmap->map; mp->free == 0 || mp->size < size; mp = mp->next)
		;
	if(mp == nil){
		poperror();
		unlock(rmap);
		return 0ULL;
	}
	if(mp->free == 0)
		panic("zmapalloc: not free");
	if(mp->size > size){
		nmp = smalloc(sizeof *nmp);
		*nmp = *mp;
		nmp->addr += size;
		nmp->size -= size;
		nmp->free = 1;
		mp->size = size;
		mp->next = nmp;
	}
	mp->free = 0;
	poperror();
	unlock(rmap);
	if(DBGFLG > 1){
		DBG("zmapalloc %#ullx:\n", mp->addr);
		dumpzmap(rmap);
	}
	return mp->addr;
}
