#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	buckLook(u8int *score, int type, u8int *data, int n);
static int	writeBucket(ISect *is, u32int buck, IBucket *ib, DBlock *b);
static int	okIBucket(IBucket *ib, ISect *is);
static ISect	*initISect1(ISect *is);

static VtLock	*indexLock;	//ZZZ

static char	indexMagic[] = "venti index";

static char IndexMagic[] = "venti index configuration";

Index*
initIndex(char *name, ISect **sects, int n)
{
	IFile f;
	Index *ix;
	ISect *is;
	u32int last, blockSize, tabSize;
	int i;

	if(n <= 0){
		setErr(EOk, "no index sections to initialize index");
		return nil;
	}
	ix = MKZ(Index);
	if(ix == nil){
		setErr(EOk, "can't initialize index: out of memory");
		freeIndex(ix);
		return nil;
	}

	tabSize = sects[0]->tabSize;
	if(!partIFile(&f, sects[0]->part, sects[0]->tabBase, tabSize))
		return 0;
	if(!parseIndex(&f, ix)){
		freeIFile(&f);
		freeIndex(ix);
		return nil;
	}
	freeIFile(&f);
	if(!nameEq(ix->name, name)){
		setErr(ECorrupt, "mismatched index name: found %s expected %s", ix->name, name);
		return nil;
	}
	ix->sects = sects;
	if(ix->nsects != n){
		setErr(ECorrupt, "mismatched number index sections: found %d expected %d", n, ix->nsects);
		freeIndex(ix);
		return nil;
	}
	last = 0;
	blockSize = ix->blockSize;
	for(i = 0; i < ix->nsects; i++){
		is = sects[i];
		if(!nameEq(ix->name, is->index)
		|| is->blockSize != blockSize
		|| is->tabSize != tabSize
		|| !nameEq(is->name, ix->smap[i].name)
		|| is->start != ix->smap[i].start
		|| is->stop != ix->smap[i].stop
		|| last != is->start
		|| is->start > is->stop){
			setErr(ECorrupt, "inconsistent index sections in %s", ix->name);
			freeIndex(ix);
			return nil;
		}
		last = is->stop;
	}
	ix->tabSize = tabSize;
	ix->buckets = last;

	ix->div = (((u64int)1 << 32) + last - 1) / last;
	last = (((u64int)1 << 32) - 1) / ix->div + 1;
	if(last != ix->buckets){
		setErr(ECorrupt, "inconsistent math for buckets in %s", ix->name);
		freeIndex(ix);
		return nil;
	}

	ix->arenas = MKNZ(Arena*, ix->narenas);
	if(!mapArenas(ix->amap, ix->arenas, ix->narenas, ix->name)){
		freeIndex(ix);
		return nil;
	}
	return ix;
}

int
wbIndex(Index *ix)
{
	Fmt f;
	ZBlock *b;
	int i;

	if(ix->nsects == 0){
		setErr(EOk, "no sections in index %s", ix->name);
		return 0;
	}
	b = allocZBlock(ix->tabSize, 1);
	if(b == nil){
		setErr(EOk, "can't write index configuration: out of memory");
		return 0;
	}
	fmtZBInit(&f, b);
	if(!outputIndex(&f, ix)){
		setErr(EOk, "can't make index configuration: table storage too small %d", ix->tabSize);
		freeZBlock(b);
		return 0;
	}
	for(i = 0; i < ix->nsects; i++){
		if(!writePart(ix->sects[i]->part, ix->sects[i]->tabBase, b->data, ix->tabSize)){
			setErr(EOk, "can't write index: %R");
			freeZBlock(b);
			return 0;
		}
	}
	freeZBlock(b);

	for(i = 0; i < ix->nsects; i++)
		if(!wbISect(ix->sects[i]))
			return 0;

	return 1;
}

/*
 * index: IndexMagic '\n' version '\n' name '\n' blockSize '\n' sections arenas
 * version, blockSize: u32int
 * name: max. ANameSize string
 * sections, arenas: AMap
 */
int
outputIndex(Fmt *f, Index *ix)
{
	if(fmtprint(f, "%s\n%ud\n%s\n%ud\n", IndexMagic, ix->version, ix->name, ix->blockSize) < 0)
		return 0;
	return outputAMap(f, ix->smap, ix->nsects) && outputAMap(f, ix->amap, ix->narenas);
}

int
parseIndex(IFile *f, Index *ix)
{
	AMapN amn;
	u32int v;
	char *s;

	/*
	 * magic
	 */
	s = ifileLine(f);
	if(s == nil || strcmp(s, IndexMagic) != 0){
		setErr(ECorrupt, "bad index magic for %s", f->name);
		return 0;
	}

	/*
	 * version
	 */
	if(!ifileU32Int(f, &v)){
		setErr(ECorrupt, "syntax error: bad version number in %s", f->name);
		return 0;
	}
	ix->version = v;
	if(ix->version != IndexVersion){
		setErr(ECorrupt, "bad version number in %s", f->name);
		return 0;
	}

	/*
	 * name
	 */
	if(!ifileName(f, ix->name)){
		setErr(ECorrupt, "syntax error: bad index name in %s", f->name);
		return 0;
	}

	/*
	 * block size
	 */
	if(!ifileU32Int(f, &v)){
		setErr(ECorrupt, "syntax error: bad version number in %s", f->name);
		return 0;
	}
	ix->blockSize = v;

	if(!parseAMap(f, &amn))
		return 0;
	ix->nsects = amn.n;
	ix->smap = amn.map;

	if(!parseAMap(f, &amn))
		return 0;
	ix->narenas = amn.n;
	ix->amap = amn.map;

	return 1;
}

/*
 * initialize an entirely new index
 */
Index *
newIndex(char *name, ISect **sects, int n)
{
	Index *ix;
	AMap *smap;
	u64int nb;
	u32int div, ub, xb, start, stop, blockSize, tabSize;
	int i, j;

	if(n < 1){
		setErr(EOk, "creating index with no index sections");
		return nil;
	}

	/*
	 * compute the total buckets available in the index,
	 * and the total buckets which are used.
	 */
	nb = 0;
	blockSize = sects[0]->blockSize;
	tabSize = sects[0]->tabSize;
	for(i = 0; i < n; i++){
		if(sects[i]->start != 0 || sects[i]->stop != 0
		|| sects[i]->index[0] != '\0'){
			setErr(EOk, "creating new index using non-empty section %s", sects[i]->name);
			return nil;
		}
		if(blockSize != sects[i]->blockSize){
			setErr(EOk, "mismatched block sizes in index sections");
			return nil;
		}
		if(tabSize != sects[i]->tabSize){
			setErr(EOk, "mismatched config table sizes in index sections");
			return nil;
		}
		nb += sects[i]->blocks;
	}

	/*
	 * check for duplicate names
	 */
	for(i = 0; i < n; i++){
		for(j = i + 1; j < n; j++){
			if(nameEq(sects[i]->name, sects[j]->name)){
				setErr(EOk, "duplicate section name %s for index %s", sects[i]->name, name);
				return nil;
			}
		}
	}

	if(nb >= ((u64int)1 << 32)){
		setErr(EBug, "index too large");
		return nil;
	}
	div = (((u64int)1 << 32) + nb - 1) / nb;
	ub = (((u64int)1 << 32) - 1) / div + 1;
	if(div < 100){
		setErr(EBug, "index divisor too coarse");
		return nil;
	}
	if(ub > nb){
		setErr(EBug, "index initialization math wrong");
		return nil;
	}

	/*
	 * initialize each of the index sections
	 * and the section map table
	 */
	smap = MKNZ(AMap, n);
	if(smap == nil){
		setErr(EOk, "can't create new index: out of memory");
		return nil;
	}
	xb = nb - ub;
	start = 0;
	for(i = 0; i < n; i++){
		stop = start + sects[i]->blocks - xb / n;
		if(i == n - 1)
			stop = ub;
		sects[i]->start = start;
		sects[i]->stop = stop;
		nameCp(sects[i]->index, name);

		smap[i].start = start;
		smap[i].stop = stop;
		nameCp(smap[i].name, sects[i]->name);
		start = stop;
	}

	/*
	 * initialize the index itself
	 */
	ix = MKZ(Index);
	if(ix == nil){
		setErr(EOk, "can't create new index: out of memory");
		free(smap);
		return nil;
	}
	ix->version = IndexVersion;
	nameCp(ix->name, name);
	ix->sects = sects;
	ix->smap = smap;
	ix->nsects = n;
	ix->blockSize = blockSize;
	ix->div = div;
	ix->buckets = ub;
	ix->tabSize = tabSize;
	return ix;
}

ISect*
initISect(Part *part)
{
	ISect *is;
	ZBlock *b;
	int ok;

	b = allocZBlock(HeadSize, 0);
	if(b == nil || !readPart(part, PartBlank, b->data, HeadSize)){
		setErr(EAdmin, "can't read index section header: %R");
		return nil;
	}

	is = MKZ(ISect);
	if(is == nil){
		freeZBlock(b);
		return nil;
	}
	is->part = part;
	ok = unpackISect(is, b->data);
	freeZBlock(b);
	if(!ok){
		setErr(ECorrupt, "corrupted index section header: %R");
		freeISect(is);
		return nil;
	}

	if(is->version != ISectVersion){
		setErr(EAdmin, "unknown index section version %d", is->version);
		freeISect(is);
		return nil;
	}

	return initISect1(is);
}

ISect*
newISect(Part *part, char *name, u32int blockSize, u32int tabSize)
{
	ISect *is;
	u32int tabBase;

	is = MKZ(ISect);
	if(is == nil)
		return nil;

	nameCp(is->name, name);
	is->version = ISectVersion;
	is->part = part;
	is->blockSize = blockSize;
	is->start = 0;
	is->stop = 0;
	tabBase = (PartBlank + HeadSize + blockSize - 1) & ~(blockSize - 1);
	is->blockBase = (tabBase + tabSize + blockSize - 1) & ~(blockSize - 1);
	is->blocks = is->part->size / blockSize - is->blockBase / blockSize;

	is = initISect1(is);
	if(is == nil)
		return nil;

	return is;
}

/*
 * initialize the computed paramaters for an index
 */
static ISect*
initISect1(ISect *is)
{
	u64int v;

	is->buckMax = (is->blockSize - IBucketSize) / IEntrySize;
	is->blockLog = u64log2(is->blockSize);
	if(is->blockSize != (1 << is->blockLog)){
		setErr(ECorrupt, "illegal non-power-of-2 bucket size %d\n", is->blockSize);
		freeISect(is);
		return nil;
	}
	partBlockSize(is->part, is->blockSize);
	is->tabBase = (PartBlank + HeadSize + is->blockSize - 1) & ~(is->blockSize - 1);
	if(is->tabBase >= is->blockBase){
		setErr(ECorrupt, "index section config table overlaps bucket storage");
		freeISect(is);
		return nil;
	}
	is->tabSize = is->blockBase - is->tabBase;
	v = is->part->size & ~(u64int)(is->blockSize - 1);
	if(is->blockBase + (u64int)is->blocks * is->blockSize != v){
		setErr(ECorrupt, "invalid blocks in index section %s", is->name);
//ZZZZZZZZZ
//		freeISect(is);
//		return nil;
	}

	if(is->stop - is->start > is->blocks){
		setErr(ECorrupt, "index section overflows available space");
		freeISect(is);
		return nil;
	}
	if(is->start > is->stop){
		setErr(ECorrupt, "invalid index section range");
		freeISect(is);
		return nil;
	}

if(indexLock == nil)indexLock = vtLockAlloc();
	return is;
}

int
wbISect(ISect *is)
{
	ZBlock *b;

	b = allocZBlock(HeadSize, 1);
	if(b == nil)
//ZZZ set error?
		return 0;

	if(!packISect(is, b->data)){
		setErr(ECorrupt, "can't make index section header: %R");
		freeZBlock(b);
		return 0;
	}
	if(!writePart(is->part, PartBlank, b->data, HeadSize)){
		setErr(EAdmin, "can't write index section header: %R");
		freeZBlock(b);
		return 0;
	}
	freeZBlock(b);

	return 1;
}

void
freeISect(ISect *is)
{
	if(is == nil)
		return;
	free(is);
}

void
freeIndex(Index *ix)
{
	int i;

	if(ix == nil)
		return;
	free(ix->amap);
	free(ix->arenas);
	for(i = 0; i < ix->nsects; i++)
		freeISect(ix->sects[i]);
	free(ix->sects);
	free(ix->smap);
	free(ix);
}

/*
 * write a clump to an available arena in the index
 * and return the address of the clump within the index.
ZZZ question: should this distinguish between an arena
filling up and real errors writing the clump?
 */
u64int
writeIClump(Index *ix, Clump *c, u8int *clbuf)
{
	u64int a;
	int i;

	for(i = ix->mapAlloc; i < ix->narenas; i++){
		a = writeAClump(ix->arenas[i], c, clbuf);
		if(a != TWID64)
			return a + ix->amap[i].start;
	}

	setErr(EAdmin, "no space left in arenas");
	return 0;
}

/*
 * convert an arena index to an relative address address
 */
Arena*
amapItoA(Index *ix, u64int a, u64int *aa)
{
	int r, l, m;

	l = 1;
	r = ix->narenas - 1;
	while(l <= r){
		m = (r + l) / 2;
		if(ix->amap[m].start <= a)
			l = m + 1;
		else
			r = m - 1;
	}
	l--;

	if(a > ix->amap[l].stop){
		setErr(ECrash, "unmapped address passed to amapItoA");
		return nil;
	}

	if(ix->arenas[l] == nil){
		setErr(ECrash, "unmapped arena selected in amapItoA");
		return nil;
	}
	*aa = a - ix->amap[l].start;
	return ix->arenas[l];
}

int
iAddrEq(IAddr *ia1, IAddr *ia2)
{
	return ia1->type == ia2->type
		&& ia1->size == ia2->size
		&& ia1->blocks == ia2->blocks
		&& ia1->addr == ia2->addr;
}

/*
 * lookup the the score int the partition
 *
 * nothing needs to be explicitly locked:
 * only static parts of ix are used, and
 * the bucket is locked by the DBlock lock.
 */
int
loadIEntry(Index *ix, u8int *score, int type, IEntry *ie)
{
	ISect *is;
	DBlock *b;
	IBucket ib;
	u32int buck;
	int h, ok;

	buck = hashBits(score, 32) / ix->div;
	ok = 0;
	for(;;){
		vtLock(stats.lock);
		stats.indexReads++;
		vtUnlock(stats.lock);
		is = findISect(ix, buck);
		if(is == nil){
			setErr(EAdmin, "bad math in loadIEntry");
			return 0;
		}
		buck -= is->start;
		b = getDBlock(is->part, is->blockBase + ((u64int)buck << is->blockLog), 1);
		if(b == nil)
			break;

		unpackIBucket(&ib, b->data);
		if(!okIBucket(&ib, is))
			break;

		h = buckLook(score, type, ib.data, ib.n);
		if(h & 1){
			h ^= 1;
			unpackIEntry(ie, &ib.data[h]);
			ok = 1;
			break;
		}

		break;
	}
	putDBlock(b);
	return ok;
}

/*
 * insert or update an index entry into the appropriate bucket
 */
int
storeIEntry(Index *ix, IEntry *ie)
{
	ISect *is;
	DBlock *b;
	IBucket ib;
	u32int buck;
	int h, ok;

	buck = hashBits(ie->score, 32) / ix->div;
	ok = 0;
	for(;;){
		vtLock(stats.lock);
		stats.indexWReads++;
		vtUnlock(stats.lock);
		is = findISect(ix, buck);
		if(is == nil){
			setErr(EAdmin, "bad math in storeIEntry");
			return 0;
		}
		buck -= is->start;
		b = getDBlock(is->part, is->blockBase + ((u64int)buck << is->blockLog), 1);
		if(b == nil)
			break;

		unpackIBucket(&ib, b->data);
		if(!okIBucket(&ib, is))
			break;

		h = buckLook(ie->score, ie->ia.type, ib.data, ib.n);
		if(h & 1){
			h ^= 1;
			packIEntry(ie, &ib.data[h]);
			ok = writeBucket(is, buck, &ib, b);
			break;
		}

		if(ib.n < is->buckMax){
			memmove(&ib.data[h + IEntrySize], &ib.data[h], ib.n * IEntrySize - h);
			ib.n++;

			packIEntry(ie, &ib.data[h]);
			ok = writeBucket(is, buck, &ib, b);
			break;
		}

		break;
	}

	putDBlock(b);
	return ok;
}

static int
writeBucket(ISect *is, u32int buck, IBucket *ib, DBlock *b)
{
	if(buck >= is->blocks)
		setErr(EAdmin, "index write out of bounds: %d >= %d\n",
				buck, is->blocks);
	vtLock(stats.lock);
	stats.indexWrites++;
	vtUnlock(stats.lock);
	packIBucket(ib, b->data);
	return writePart(is->part, is->blockBase + ((u64int)buck << is->blockLog), b->data, is->blockSize);
}

/*
 * find the number of the index section holding score
 */
int
indexSect(Index *ix, u8int *score)
{
	u32int buck;
	int r, l, m;

	buck = hashBits(score, 32) / ix->div;
	l = 1;
	r = ix->nsects - 1;
	while(l <= r){
		m = (r + l) >> 1;
		if(ix->sects[m]->start <= buck)
			l = m + 1;
		else
			r = m - 1;
	}
	return l - 1;
}

/*
 * find the index section which holds buck
 */
ISect*
findISect(Index *ix, u32int buck)
{
	ISect *is;
	int r, l, m;

	l = 1;
	r = ix->nsects - 1;
	while(l <= r){
		m = (r + l) >> 1;
		if(ix->sects[m]->start <= buck)
			l = m + 1;
		else
			r = m - 1;
	}
	is = ix->sects[l - 1];
	if(is->start <= buck && is->stop > buck)
		return is;
	return nil;
}

static int
okIBucket(IBucket *ib, ISect *is)
{
	if(ib->n <= is->buckMax && (ib->next == 0 || ib->next >= is->start && ib->next < is->stop))
		return 1;

	setErr(EICorrupt, "corrupted disk index bucket: n=%ud max=%ud, next=%lud range=[%lud,%lud)",
		ib->n, is->buckMax, ib->next, is->start, is->stop);
	return 0;
}

/*
 * look for score within data;
 * return 1 | byte index of matching index,
 * or 0 | index of least element > score
 */
static int
buckLook(u8int *score, int type, u8int *data, int n)
{
	int i, r, l, m, h, c, cc;

	l = 0;
	r = n - 1;
	while(l <= r){
		m = (r + l) >> 1;
		h = m * IEntrySize;
		for(i = 0; i < VtScoreSize; i++){
			c = score[i];
			cc = data[h + i];
			if(c != cc){
				if(c > cc)
					l = m + 1;
				else
					r = m - 1;
				goto cont;
			}
		}
		cc = data[h + IEntryTypeOff];
		if(type != cc){
			if(type > cc)
				l = m + 1;
			else
				r = m - 1;
			goto cont;
		}
		return h | 1;
	cont:;
	}

	return l * IEntrySize;
}

/*
 * compare two IEntries; consistent with buckLook
 */
int
ientryCmp(void *vie1, void *vie2)
{
	u8int *ie1, *ie2;
	int i, v1, v2;

	ie1 = vie1;
	ie2 = vie2;
	for(i = 0; i < VtScoreSize; i++){
		v1 = ie1[i];
		v2 = ie2[i];
		if(v1 != v2){
			if(v1 < v2)
				return -1;
			return 1;
		}
	}
	v1 = ie1[IEntryTypeOff];
	v2 = ie2[IEntryTypeOff];
	if(v1 != v2){
		if(v1 < v2)
			return -1;
		return 1;
	}
	return 0;
}
