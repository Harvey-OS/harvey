#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

static int	getString(char **s, uchar **p, int *n);
static int	metaBlockUnpack(MetaBlock *mb, uchar *p, int n);
static uchar 	*metaBlockEntry(MetaBlock *mb, int i, int *n);
static int	vacDirUnpack(VacDir *dir, uchar *p, int n);
static int	vacFileAquire(VacFile *vf);
static void	vacFileRelease(VacFile *vf);

void
vacDirCleanup(VacDir *dir)
{
	vtMemFree(dir->elem);
	dir->elem = nil;
	vtMemFree(dir->uid);
	dir->uid = nil;
	vtMemFree(dir->gid);
	dir->gid = nil;
	vtMemFree(dir->mid);
	dir->mid = nil;
}

void
vacDirCopy(VacDir *dst, VacDir *src)
{
	*dst = *src;
	dst->elem = vtStrDup(src->elem);
	dst->uid = vtStrDup(src->uid);
	dst->gid = vtStrDup(src->gid);
	dst->mid = vtStrDup(src->mid);
}


static int
vacDirUnpack(VacDir *dir, uchar *p, int n)
{
	int t, nn;

	memset(dir, 0, sizeof(VacDir));

	/* magic */
	if(n < 4 || vtGetUint32(p) != DirMagic)
		goto Err;
	p += 4;
	n -= 4;

	/* version */
	if(n < 2)
		goto Err;
	dir->version = vtGetUint16(p);
	if(dir->version < 7 || dir->version > 8)
		goto Err;
	p += 2;
	n -= 2;	

	/* elem */
	if(!getString(&dir->elem, &p, &n))
		goto Err;

	/* entry  */
	if(n < 4)
		goto Err;
	dir->entry = vtGetUint32(p);
	p += 4;
	n -= 4;

	/* size is gotten from DirEntry */

	/* qid */
	if(n < 8)
		goto Err;
	dir->qid = vtGetUint64(p);
	p += 8;
	n -= 8;

	/* replacement */
	if(dir->version == 7) {
		if(n < VtScoreSize)
			goto Err;
		p += VtScoreSize;
		n -= VtScoreSize;
	}
	
	/* uid */
	if(!getString(&dir->uid, &p, &n))
		goto Err;

	/* gid */
	if(!getString(&dir->gid, &p, &n))
		goto Err;

	/* mid */
	if(!getString(&dir->mid, &p, &n))
		goto Err;

	if(n < 5*4)
		goto Err;
	dir->mtime = vtGetUint32(p);
	dir->mcount = vtGetUint32(p+4);
	dir->ctime = vtGetUint32(p+8);
	dir->atime = vtGetUint32(p+12);
	dir->mode = vtGetUint32(p+16);
	p += 5*4;
	n -= 5*4;

	/* optional meta data */
	while(n > 0) {
		if(n < 3)
			goto Err;
		t = p[0];
		nn = vtGetUint16(p+1);
		p += 3;
		n -= 3;
		if(n < nn)
			goto Err;
		switch(t) {
		case DirPlan9Entry:
			if(dir->plan9 || nn != 12)
				goto Err;
			dir->plan9 = 1;
			dir->p9path = vtGetUint64(p);
			dir->p9version = vtGetUint32(p+8);
			break;
		}
		p += nn;
		n -= nn;
	}

	return 1;
Err:
	vtSetError(EBadMeta);
	vacDirCleanup(dir);
	return 0;
}

static int
metaBlockUnpack(MetaBlock *mb, uchar *p, int n)
{	
	mb->buf = p;

	if(n == 0) {
		memset(mb, 0, sizeof(MetaBlock));
		return 1;
	}

	if(n < MetaHeaderSize) {
		vtSetError("truncated meta block 1");
		return 0;
	}
	if(vtGetUint32(p) != MetaMagic) {
		vtSetError("bad meta block magic");
		return 0;
	}
	mb->size = vtGetUint16(p+4);
	mb->free = vtGetUint16(p+6);
	mb->maxindex = vtGetUint16(p+8);
	mb->nindex = vtGetUint16(p+10);
	if(mb->size > n) {
		vtSetError("bad meta block size");
		return 0;
	}
	p += MetaHeaderSize;
	n -= MetaHeaderSize;

	USED(p);
	if(n < mb->maxindex*MetaIndexSize) {
 		vtSetError("truncated meta block 2");
		return 0;
	}
	return 1;
}


static uchar *
metaBlockEntry(MetaBlock *mb, int i, int *n)
{
	uchar *p;
	int eo, en;

	if(i < 0 || i >= mb->nindex) {
		vtSetError("bad meta entry index");
		return 0;
	}

	p = mb->buf + MetaHeaderSize + i*MetaIndexSize;
	eo = vtGetUint16(p);
	en = vtGetUint16(p+2);

	if(eo < MetaHeaderSize + mb->maxindex*MetaIndexSize) {
		vtSetError("corrupted entry in meta block");
		return 0;
	}

	if(eo+en > mb->size) {
 		vtSetError("truncated meta block 3");
		return 0;
	}
	
	p = mb->buf + eo;
	
	/* make sure entry looks ok and includes an elem name */
	if(en < 8 || vtGetUint32(p) != DirMagic || en < 8 + vtGetUint16(p+6)) {
		vtSetError("corrupted meta block entry");
		return 0;
	}

	if(n != nil)
		*n = en;
	return p;
}

/* assumes a small amount of checking has been done in metaBlockEntry */
static int
metaEntryCmp(uchar *p, char *s)
{
	int n;

	p += 6;
	n = vtGetUint16(p);
	p += 2;
	while(n > 0) {
		if(*s == 0)
			return -1;
		if(*p < (uchar)*s)
			return -1;
		if(*p > (uchar)*s)
			return 1;
		p++;
		s++;
		n--;
	}
	return *s != 0;
}


static int
dirLookup(Source *meta, char *elem, VacDir *vd)
{
	int i, j, nb, n;
	MetaBlock mb;
	uchar *p;
	Lump *u;
	int b, t, x;

	u = nil;
	nb = (sourceGetSize(meta) + meta->dsize - 1)/meta->dsize;
	for(i=0; i<nb; i++) {
		u = sourceGetLump(meta, i, 1, 1);
		if(u == nil)
			continue;
		if(!metaBlockUnpack(&mb, u->data, u->size))
			goto Err;
		if(0) {
		for(j=0; j<mb.nindex; j++) {
			p = metaBlockEntry(&mb, j, &n);
			if(p == 0)
				goto Err;
			if(metaEntryCmp(p, elem) != 0)
				continue;
			if(!vacDirUnpack(vd, p, n))
				goto Err;
			lumpDecRef(u, 1);
			return 1;
		}
		} else {
		/* binary search within block */
		b = 0;
		t = mb.nindex;
		while(b < t) {
			j = (b+t)>>1;
			p = metaBlockEntry(&mb, j, &n);
			if(p == 0)
				goto Err;
			x = metaEntryCmp(p, elem);

			if(x == 0) {
				if(!vacDirUnpack(vd, p, n))
					goto Err;
				lumpDecRef(u, 1);
				return 1;
			}
		
			if(x < 0)
				b = j+1;
			else /* x > 0 */
				t = j;
		}
		}
		
		lumpDecRef(u, 1);
		u = nil;
	}
	vtSetError("file does not exist");
Err:
	lumpDecRef(u, 1);
	return 0;
}

static VacFile *
vacFileAlloc(VacFS *fs)
{
	VacFile *vf;

	vf = vtMemAllocZ(sizeof(VacFile));
	vf->lk = vtLockAlloc();
	vf->ref = 1;
	vf->fs = fs;
	return vf;
}


VacFile *
vacFileRoot(VacFS *fs, uchar *score)
{
	Lump *u, *v;
	Source *r0, *r1, *r2;
	MetaBlock mb;
	uchar *q;
	int nn;
	VacFile *root = nil;

	r0 = nil;
	r1 = nil;
	r2 = nil;
	v = nil;

	u = cacheGetLump(fs->cache, score, VtDirType, fs->bsize);
	if(u == nil)
		goto Err;
	if(!fs->readOnly) {
		v = cacheAllocLump(fs->cache, VtDirType, fs->bsize, 1);
		if(v == nil) {
			vtUnlock(u->lk);
			goto Err;
		}
		v->gen = u->gen;
		v->size = u->size;
		v->state = LumpActive;
		memmove(v->data, u->data, v->size);
		lumpDecRef(u, 1);
		u = v;
		v = nil;
	}
	vtUnlock(u->lk);
	r0 = sourceAlloc(fs->cache, u, 0, fs->readOnly);
	if(r0 == nil)
		goto Err;
	r1 = sourceAlloc(fs->cache, u, 1, fs->readOnly);
	if(r1 == nil)
		goto Err;
	r2 = sourceAlloc(fs->cache, u, 2, fs->readOnly);
	if(r2 == nil)
		goto Err;
	lumpDecRef(u, 0);

	root = vacFileAlloc(fs);
	root->up = root;
	root->source = r0;
	r0 = nil; USED(r0);
	root->msource = r1;
	r1 = nil; USED(r1);

	u = sourceGetLump(r2, 0, 1, 0);
	if(u == nil)
		goto Err;

	if(!metaBlockUnpack(&mb, u->data, u->size))
		goto Err;

	q = metaBlockEntry(&mb, 0, &nn);
	if(q == nil)
		goto Err;
	if(!vacDirUnpack(&root->dir, q, nn))
		goto Err;
	lumpDecRef(u, 0);
	sourceFree(r2);

	return root;
Err:
	lumpDecRef(u, 0);
	lumpDecRef(v, 0);
	if(r0)
		sourceFree(r0);
	if(r1)
		sourceFree(r1);
	if(r2)
		sourceFree(r2);
	if(root)
		vacFileDecRef(root);

	return nil;
}


VacFile *
vacFileWalk(VacFile *vf, char *elem)
{
	VacFile *nvf = nil;

	vtLock(vf->lk);
	if(elem[0] == 0) {
		vtSetError("illegal path element");
		goto Err;
	}

	if(vf->removed) {
		vtSetError(ERemoved);
		goto Err;
	}

	if(!(vf->dir.mode & ModeDir)) {
		vtSetError("not a directory");
		goto Err;
	}

	if(strcmp(elem, ".") == 0) {
		vtUnlock(vf->lk);
		return vacFileIncRef(vf);
	}

	if(strcmp(elem, "..") == 0) {
		vtUnlock(vf->lk);
		return vacFileIncRef(vf->up);
	}

	for(nvf = vf->down; nvf; nvf=nvf->next) {
		vtLock(nvf->lk);
		if(strcmp(elem, nvf->dir.elem) == 0 && !nvf->removed) {
			nvf = vacFileIncRef(nvf);
			vtUnlock(nvf->lk);
			vtUnlock(vf->lk);
			return nvf;
		}
		vtUnlock(nvf->lk);
	}

	nvf = vacFileAlloc(vf->fs);
	if(!dirLookup(vf->msource, elem, &nvf->dir))
		goto Err;
	nvf->source = sourceOpen(vf->source, nvf->dir.entry, vf->fs->readOnly);
	if(nvf->source == nil)
		goto Err;
	if(nvf->dir.mode & ModeDir) {
		nvf->msource = sourceOpen(vf->source, nvf->dir.entry+1, vf->fs->readOnly);
		if(nvf->msource == nil)
			goto Err;
	}
	nvf->next = vf->down;
	vf->down = nvf;
	nvf->up = vf;
	vtUnlock(vf->lk);
	vacFileIncRef(vf);
//fprint(2, "opening %s\n", nvf->dir.elem);
	return nvf;
Err:
	vtUnlock(vf->lk);
	if(nvf) {
		vacDirCleanup(&nvf->dir);
		sourceFree(nvf->source);
		sourceFree(nvf->msource);
		vtLockFree(nvf->lk);
		vtMemFree(nvf);
	}
	return nil;
}

VacFile *
vacFileOpen(VacFS *fs, char *path)
{
	VacFile *vf, *nvf;
	char *p, elem[VtMaxStringSize];
	int n;

	vf = fs->root;
	vacFileIncRef(vf);
	while(*path != 0) {
		for(p = path; *p && *p != '/'; p++)
			;
		n = p - path;
		if(n > 0) {
			if(n > VtMaxStringSize) {
				vtSetError("path element too long");
				goto Err;
			}
			memmove(elem, path, n);
			elem[n] = 0;
			nvf = vacFileWalk(vf, elem);
			if(nvf == nil)
				goto Err;
			vacFileDecRef(vf);
			vf = nvf;
		}
		if(*p == '/')
			p++;
		path = p;
	}
	return vf;
Err:
	vacFileDecRef(vf);
	return nil;
}

int
vacFileRead(VacFile *vf, void *buf, int cnt, vlong offset)
{
	Source *s;
	uvlong size;
	ulong bn;
	int off, dsize, n, nn;
	Lump *u;
	uchar *b;

if(0)fprint(2, "vacFileRead: %s %d, %lld\n", vf->dir.elem, cnt, offset);

	if(!vacFileAquire(vf))
		return -1;

	s = vf->source;

	dsize = s->dsize;
	size = sourceGetSize(s);

	if(offset < 0) {
		vtSetError(EBadOffset);
		goto Err;
	}

	if(offset >= size)
		offset = size;

	if(cnt > size-offset)
		cnt = size-offset;
	bn = offset/dsize;
	off = offset%dsize;
	b = buf;
	while(cnt > 0) {
		u = sourceGetLump(s, bn, 1, 0);
		if(u == nil)
			goto Err;
		n = cnt;
		if(n > dsize-off)
			n = dsize-off;
		nn = u->size-off;
		if(nn > n)
			nn = n;
		memmove(b, u->data+off, nn);
		memset(b+nn, 0, nn-n);
		off = 0;
		bn++;
		cnt -= n;
		b += n;
		lumpDecRef(u, 0);
	}
	vacFileRelease(vf);
	return b-(uchar*)buf;
Err:
	vacFileRelease(vf);
	return -1;
}

int
vacFileWrite(VacFile *vf, void *buf, int cnt, vlong offset, char *user)
{
	Source *s;
	ulong bn;
	int off, dsize, n;
	Lump *u;
	uchar *b;

	USED(user);

	if(!vacFileAquire(vf))
		return -1;

	if(vf->fs->readOnly) {
		vtSetError(EReadOnly);
		goto Err;
	}

	if(vf->dir.mode & ModeDir) {
		vtSetError(ENotFile);
		goto Err;
	}
if(0)fprint(2, "vacFileWrite: %s %d, %lld\n", vf->dir.elem, cnt, offset);

	s = vf->source;
	dsize = s->dsize;

	if(offset < 0) {
		vtSetError(EBadOffset);
		goto Err;
	}

	bn = offset/dsize;
	off = offset%dsize;
	b = buf;
	while(cnt > 0) {
		n = cnt;
		if(n > dsize-off)
			n = dsize-off;
		if(!sourceSetDepth(s, offset+n))
			goto Err;
		u = sourceGetLump(s, bn, 0, 0);
		if(u == nil)
			goto Err;
		if(u->size < dsize) {
			vtSetError("runt block");
			lumpDecRef(u, 0);
			goto Err;
		}
		memmove(u->data+off, b, n);
		off = 0;
		cnt -= n;
		b += n;
		offset += n;
		bn++;
		lumpDecRef(u, 0);
		if(!sourceSetSize(s, offset))
			goto Err;
	}
	vacFileRelease(vf);
	return b-(uchar*)buf;
Err:
	vacFileRelease(vf);
	return -1;
}

int
vacFileGetDir(VacFile *vf, VacDir *dir)
{
	if(!vacFileAquire(vf))
		return 0;
	vtLock(vf->lk);
	vacDirCopy(dir, &vf->dir);
	dir->size = sourceGetSize(vf->source);
	vtUnlock(vf->lk);
	vacFileRelease(vf);
	return 1;
}

uvlong
vacFileGetId(VacFile *vf)
{
	return vf->dir.qid;
}

int
vacFileIsDir(VacFile *vf)
{
	return (vf->dir.mode & ModeDir) != 0;
}

int
vacFileGetMode(VacFile *vf, ulong *mode)
{
	if(!vacFileAquire(vf))
		return 0;
	vtLock(vf->lk);
	*mode = vf->dir.mode;
	vtUnlock(vf->lk);
	vacFileRelease(vf);
	return 1;
}

int
vacFileGetSize(VacFile *vf, uvlong *size)
{
	if(!vacFileAquire(vf))
		return 0;
	*size = sourceGetSize(vf->source);
	vacFileRelease(vf);
	return 1;
}

int
vacFileGetPointerSize(VacFile *vf)
{
	return vf->source->psize;
}

int
vacFileGetPointerDepth(VacFile *vf)
{
	return vf->source->depth;
}

int
vacFileGetDataSize(VacFile *vf)
{
	return vf->source->dsize;
}

VacFile *
vacFileIncRef(VacFile *vf)
{
	vtLock(vf->lk);
	assert(vf->ref > 0);
	vf->ref++;
	vtUnlock(vf->lk);
	return vf;
}

int
vacFileRemove(VacFile *vf)
{	
	if(!vacFileAquire(vf))
		return 0;
	
	vtLock(vf->lk);
	vf->removed = 1;
	vtUnlock(vf->lk);

	vacFileRelease(vf);

	return 1;
}

void
vacFileUnlink(VacFile *vf)
{
	VacFile *p, *q, **qq;

	assert(vf->down == nil);
	p = vf->up;
	assert(p != nil);

	/* check if root */
	if(p == vf)
		return;

	vtLock(p->lk);
	qq = &p->down;
	for(q = *qq; q; qq=&q->next,q=*qq)
		if(q == vf)
			break;
	*qq = vf->next;
	vtUnlock(p->lk);

	vacFileDecRef(p);
}

void
vacFileDecRef(VacFile *vf)
{
	vtLock(vf->lk);
	vf->ref--;
	if(vf->ref > 0) {
		vtUnlock(vf->lk);
		return;
	}
	vtUnlock(vf->lk);
	assert(vf->ref == 0);
	vacFileUnlink(vf);
	vtLockFree(vf->lk);
	sourceFree(vf->source);	
	sourceFree(vf->msource);
	vacDirCleanup(&vf->dir);
	
	vtMemFree(vf);
}

int
vacFileGetVtDirEntry(VacFile *vf, VtDirEntry *dir)
{
	int res;

	if(!vacFileAquire(vf))
		return 0;
	res = sourceGetVtDirEntry(vf->source, dir);
	vacFileRelease(vf);
	return res;
}

int
vacFileGetBlockScore(VacFile *vf, ulong bn, uchar score[VtScoreSize])
{
	Lump *u;
	int ret, off;
	Source *r;

	r = vf->source;

	u = sourceWalk(r, bn, 1, &off);
	if(u == nil)
		return 0;
	
	ret = lumpGetScore(u, off, score);
	lumpDecRef(u, 0);
	return ret;
}


static VacDirEnum *
vacDirEnumAlloc(VacFile *vf)
{
	VacDirEnum *ds;

	if(!(vf->dir.mode & ModeDir)) {
		vtSetError(ENotDir);
		vacFileDecRef(vf);
		return nil;
	}

	ds = vtMemAllocZ(sizeof(VacDirEnum));
	ds->file = vf;
	
	return ds;
}

VacDirEnum *
vacDirEnumOpen(VacFS *fs, char *path)
{
	VacFile *vf;

	vf = vacFileOpen(fs, path);
	if(vf == nil)
		return nil;

	return vacDirEnumAlloc(vf);
}

VacDirEnum *
vacFileDirEnum(VacFile *vf)
{
	return vacDirEnumAlloc(vacFileIncRef(vf));
}

static int
dirEntrySize(Source *s, ulong elem, uvlong *size)
{
	Lump *u;
	ulong bn;
	VtDirEntry2 *d;

	bn = elem/(s->dsize/VtDirEntrySize2);
	elem -= bn*(s->dsize/VtDirEntrySize2);

	u = sourceGetLump(s, bn, 1, 1);
	if(u == nil)
		goto Err;
	if(u->size < (elem+1)*VtDirEntrySize2) {
		vtSetError(ENoDir);
		goto Err;
	}
	
	d = (VtDirEntry2*)(u->data + elem*VtDirEntrySize2);
	if(!(d->flag & VtDirEntryActive)) {
		vtSetError(ENoDir);
		goto Err;
	}

	*size = vtGetUint48(d->size);
	lumpDecRef(u, 1);
	return 1;	

Err:
	lumpDecRef(u, 1);
	return 0;
}

int
vacDirEnumRead(VacDirEnum *ds, VacDir *dir, int n)
{
	ulong nb;
	int nn, i;
	uchar *q;
	Source *meta, *source;
	MetaBlock mb;
	Lump *u;

	i = 0;
	source = ds->file->source;
	meta = ds->file->msource;
	nb = (sourceGetSize(meta) + meta->dsize - 1)/meta->dsize;

	if(ds->block >= nb)
		return 0;
	u = sourceGetLump(meta, ds->block, 1, 1);
	if(u == nil)
		goto Err;
	if(!metaBlockUnpack(&mb, u->data, u->size))
		goto Err;

	for(i=0; i<n; i++) {
		while(ds->index >= mb.nindex) {
			lumpDecRef(u, 1);
			ds->index = 0;
			ds->block++;
			if(ds->block >= nb)
				return i;
			u = sourceGetLump(meta, ds->block, 1, 1);
			if(u == nil)
				goto Err;
			if(!metaBlockUnpack(&mb, u->data, u->size))
				goto Err;
		}
		q = metaBlockEntry(&mb, ds->index, &nn);
		if(q == nil)
			goto Err;
		if(dir != nil) {
			if(!vacDirUnpack(&dir[i], q, nn))
				goto Err;
			if(!(dir[i].mode & ModeDir))
			if(!dirEntrySize(source, dir[i].entry, &dir[i].size))
				goto Err;
		}
		ds->index++;
	}
	lumpDecRef(u, 1);
	return i;
Err:
	lumpDecRef(u, 1);
	n = i;
	for(i=0; i<n ; i++)
		vacDirCleanup(&dir[i]);
	return -1;
}

void
vacDirEnumFree(VacDirEnum *ds)
{
	if(ds == nil)
		return;
	vacFileDecRef(ds->file);
	vtMemFree(ds);
}


static int
getString(char **s, uchar **p, int *n)
{
	int nn;

	if(*n < 2)
		return 0;
	
	nn = vtGetUint16(*p);
	*p += 2;
	*n -= 2;
	if(nn > *n)
		return 0;
	*s = vtMemAlloc(nn+1);
	memmove(*s, *p, nn);
	(*s)[nn] = 0;
	*p += nn;
	*n -= nn;
	return 1;
}

static int
vacFileAquire(VacFile *vf)
{
	vtLock(vf->lk);
	if(vf->removed) {
		vtUnlock(vf->lk);
		vtSetError(ERemoved);
		return 0;
	}
	assert(vf->used >= 0);
	vf->used++;
	vtUnlock(vf->lk);
	return 1;
}

static void
vacFileRelease(VacFile *vf)
{
	vtLock(vf->lk);
	vf->used--;
	if(vf->used > 0) {
		vtUnlock(vf->lk);
		return;
	}
	assert(vf->used == 0);
	if(vf->removed) {
		sourceRemove(vf->source);
		vf->source = nil;
		if(vf->msource) {
			sourceRemove(vf->msource);
			vf->msource = nil;
		}
	}
	vtUnlock(vf->lk);
}

