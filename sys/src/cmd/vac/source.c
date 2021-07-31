#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

static int	sizeToDepth(uvlong s, int psize, int dsize);

static int
sizeToDepth(uvlong s, int psize, int dsize)
{
	int np;
	int d;
	
	/* determine pointer depth */
	np = psize/VtScoreSize;
	s = (s + dsize - 1)/dsize;
	for(d = 0; s > 1; d++)
		s = (s + np - 1)/np;
	return d;
}

/* assumes u is lock? */
Source *
sourceAlloc(Cache *c, Lump *u, int entry, int readOnly)
{
	Source *r;
	int psize, dsize;
	uvlong size;
	int depth;
	VtDirEntry2 *d;

	if(u->size < (entry+1)*VtDirEntrySize2) {
fprint(2, "bad bad block size: %d\n", u->size);
		vtSetError(ENoDir);
		return nil;
	}
	
	d = (VtDirEntry2*)(u->data + entry*VtDirEntrySize2);
	if(!(d->flag & VtDirEntryActive)) {
		vtSetError(ENoDir);
		return nil;
	}
	
	psize = vtGetUint16(d->psize);
	if(psize < 512 || psize > VtMaxLumpSize) {
fprint(2, "bad psize: %d\n", psize);
		vtSetError(EBadDir);
		return nil;
	}

	dsize = vtGetUint16(d->dsize);
	if(dsize < 512 || dsize > VtMaxLumpSize) {
fprint(2, "bad dsize: %d\n", dsize);
		vtSetError(EBadDir);
		return nil;
	}
//fprint(2, "psize = %d dsize = %d\n", psize, dsize);	
	size = vtGetUint48(d->size);

	depth = (d->flag & VtDirEntryDepthMask) >> VtDirEntryDepthShift;

	/* HACK for backwards compatiblity - should go away at some point */
	if(depth == 0) {
if(size > dsize) fprint(2, "depth == 0! size = %ulld\n", size);
		depth = sizeToDepth(size, psize, dsize);
	}

	if(depth < sizeToDepth(size, psize, dsize)) {
fprint(2, "bad depth: %d %llud\n", depth, size);
		vtSetError(EBadDir);
		return nil;
	}

	r = vtMemAllocZ(sizeof(Source));
	r->lk = vtLockAlloc();
	r->cache = c;
	r->readOnly = readOnly;
	r->lump = lumpIncRef(u);
	r->entry = entry;
	r->gen = vtGetUint32(d->gen);
	r->psize = psize;
	r->dsize = dsize;
	r->dir = (d->flag & VtDirEntryDir) != 0;
	r->depth = depth;
	r->size2 = size;
	r->epb = r->dsize/VtDirEntrySize;

	return r;
}

Source *
sourceOpen(Source *r, ulong entry, int readOnly)
{
	ulong bn;
	Lump *u;

if(0)fprint(2, "sourceOpen: %V:%d: %lud\n", r->lump->score, r->entry, entry);
	if(r->readOnly && !readOnly) {
		vtSetError(EReadOnly);
		return nil;
	}

	bn = entry/r->epb;

	u = sourceGetLump(r, bn, readOnly, 0);
	if(u == nil)
		return nil;

	r = sourceAlloc(r->cache, u, entry%r->epb, readOnly);
	lumpDecRef(u, 0);
	return r;
}

Source *
sourceCreate(Source *r, int psize, int dsize, int isdir)
{
	Source *rr;
	int i, j;
	ulong entry;
	Lump *u;
	ulong bn;
	VtDirEntry2 *dir;
	ulong gen;

	if(r->readOnly) {
		vtSetError(EReadOnly);
		return nil;
	}

	/*
	 * first look at a random block to see if we can find an empty entry
	 */
	entry = sourceGetDirSize(r);
	entry = r->epb*lnrand(entry/r->epb+1);
	/*
	 * need to loop since multiple threads could be trying to allocate
	 */
	for(;;) {
		bn = entry/r->epb;
		sourceSetDepth(r, (uvlong)(bn+1)*r->dsize);
		u = sourceGetLump(r, bn, 0, 1);
		if(u == nil)
			return nil;
		for(i=entry%r->epb; i<r->epb; i++) {
			dir = (VtDirEntry*)(u->data + i*VtDirEntrySize);
			gen = vtGetUint32(dir->gen);
			if((dir->flag&VtDirEntryActive) == 0 && gen != ~0)
				goto Found;
		}
		lumpDecRef(u, 1);
		entry = sourceGetDirSize(r);
	}
Found:
	/* found an entry */
for(j=4; j<VtDirEntrySize; j++)
if(u->data[i*VtDirEntrySize + j] != 0) {
fprint(2, "nonzero dir entry!! %V %d %d %x %d\n", u->score, i, j, dir->flag, vtGetUint16(dir->psize));
break;
}
	memset(dir, 0, VtDirEntrySize);
	vtPutUint32(dir->gen, gen);
	vtPutUint16(dir->psize, psize);
	vtPutUint16(dir->dsize, dsize);
	dir->flag |= VtDirEntryActive;
	if(isdir)
		dir->flag |= VtDirEntryDir;
	memmove(dir->score, vtZeroScore, VtScoreSize);
	sourceSetDirSize(r, bn*r->epb + i + 1);
	rr = sourceAlloc(r->cache, u, i, 0);
	
	lumpDecRef(u, 1);
	return rr;
}

void
sourceRemove(Source *r)
{
	lumpFreeEntry(r->lump, r->entry);
	sourceFree(r);
}

int
sourceSetDepth(Source *r, uvlong size)
{
	Lump *u, *v;
	VtDirEntry2 *dir;
	int depth;

	if(r->readOnly){
		vtSetError(EReadOnly);
		return 0;
	}

	depth = sizeToDepth(size, r->psize, r->dsize);

	assert(depth >= 0);

	if(depth > VtPointerDepth) {
		vtSetError(ETooBig);
		return 0;
	}

	vtLock(r->lk);
	
	u = r->lump;
	vtLock(u->lk);
	dir = (VtDirEntry2*)(u->data + r->entry*VtDirEntrySize2);
	while(r->depth < depth) {
		v = cacheAllocLump(r->cache, VtPointerType0+r->depth, r->psize, r->dir);
		if(v == nil) {
			vtUnlock(u->lk);
			vtUnlock(r->lk);
			return 0;
		}
		memmove(v->data, dir->score, VtScoreSize);
		memmove(dir->score, v->score, VtScoreSize);
		r->depth++;
		dir->flag &= ~VtDirEntryDepthMask;
		dir->flag |= r->depth << VtDirEntryDepthShift;
		vtUnlock(v->lk);
	}
	vtUnlock(u->lk);
	vtUnlock(r->lk);
	return 1;
}

int
sourceGetVtDirEntry(Source *r, VtDirEntry *dir)
{
	Lump *u;
	memset(dir, 0, sizeof(*dir));
	u = r->lump;
	vtLock(u->lk);
	memmove(dir, u->data + r->entry*VtDirEntrySize, VtDirEntrySize);
	vtUnlock(u->lk);
	return 1;
}

uvlong
sourceGetSize(Source *r)
{
	uvlong size;

	vtLock(r->lk);
	size = r->size2;
	vtUnlock(r->lk);

	return size;
}


int
sourceSetSize(Source *r, uvlong size)
{
	Lump *u;
	VtDirEntry2 *dir;
	int depth;

	if(r->readOnly) {
		vtSetError(EReadOnly);
		return 0;
	}

	if(size > VtMaxFileSize) {
		vtSetError(ETooBig);
		return 0;
	}

	vtLock(r->lk);
	depth = sizeToDepth(size, r->psize, r->dsize);
	if(size < r->size2) {
		vtUnlock(r->lk);
		return 1;
	}
	if(depth > r->depth) {
		vtSetError(EBadDir);
		vtUnlock(r->lk);
		return 0;
	}
	
	u = r->lump;
	vtLock(u->lk);
	dir = (VtDirEntry2*)(u->data + r->entry*VtDirEntrySize2);
	vtPutUint48(dir->size, size);
	r->size2 = size;
	vtUnlock(u->lk);
	vtUnlock(r->lk);
	return 1;
}

int
sourceSetDirSize(Source *r, ulong ds)
{
	uvlong size;

	size = (uvlong)r->dsize*(ds/r->epb);
	size += VtDirEntrySize*(ds%r->epb);
	return sourceSetSize(r, size);
}

ulong
sourceGetDirSize(Source *r)
{
	ulong ds;
	uvlong size;

	size = sourceGetSize(r);
	ds = r->epb*(size/r->dsize);
	ds += (size%r->dsize)/VtDirEntrySize;
	return ds;
}

Lump *
sourceWalk(Source *r, ulong block, int readOnly, int *off)
{
	int depth;
	int i, np;
	Lump *u, *v;
	int elem[VtPointerDepth+1];
	ulong b;

	if(r->readOnly && !readOnly) {
		vtSetError(EReadOnly);
		return nil;
	}

	vtLock(r->lk);
	np = r->psize/VtScoreSize;
	b = block;
	for(i=0; i<r->depth; i++) {
		elem[i] = b % np;
		b /= np;
	}
	if(b != 0) {
		vtUnlock(r->lk);
		vtSetError(EBadOffset);
		return nil;
	}
	elem[i] = r->entry;
	u = lumpIncRef(r->lump);
	depth = r->depth;
	*off = elem[0];
	vtUnlock(r->lk);

	for(i=depth; i>0; i--) {
		v = lumpWalk(u, elem[i], VtPointerType0+i-1, r->psize, readOnly, 0);
		lumpDecRef(u, 0);
		if(v == nil)
			return nil;
		u = v;
	}

	return u;
}

Lump *
sourceGetLump(Source *r, ulong block, int readOnly, int lock)
{
	int type, off;
	Lump *u, *v;

	if(r->readOnly && !readOnly) {
		vtSetError(EReadOnly);
		return nil;
	}
if(0)fprint(2, "sourceGetLump: %V:%d %lud\n", r->lump->score, r->entry, block);
	u = sourceWalk(r, block, readOnly, &off);
	if(u == nil)
		return nil;
	if(r->dir)
		type = VtDirType;
	else
		type = VtDataType;
	v = lumpWalk(u, off, type, r->dsize, readOnly, lock);
	lumpDecRef(u, 0);
	return v;
}

void
sourceFree(Source *k)
{
	if(k == nil)
		return;
	lumpDecRef(k->lump, 0);
	vtLockFree(k->lk);
	memset(k, ~0, sizeof(*k));
	vtMemFree(k);
}
