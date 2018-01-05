/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Index, mapping scores to log positions.
 *
 * The index is made up of some number of index sections, each of
 * which is typically stored on a different disk.  The blocks in all the
 * index sections are logically numbered, with each index section
 * responsible for a range of blocks.  Blocks are typically 8kB.
 *
 * The N index blocks are treated as a giant hash table.  The top 32 bits
 * of score are used as the key for a lookup.  Each index block holds
 * one hash bucket, which is responsible for ceil(2^32 / N) of the key space.
 *
 * The index is sized so that a particular bucket is extraordinarily
 * unlikely to overflow: assuming compressed data blocks are 4kB
 * on disk, and assuming each block has a 40 byte index entry,
 * the index data will be 1% of the total data.  Since scores are essentially
 * random, all buckets should be about the same fullness.
 * A factor of 5 gives us a wide comfort boundary to account for
 * random variation.  So the index disk space should be 5% of the arena disk space.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	initindex1(Index*);
static ISect	*initisect1(ISect *is);

#define KEY(k,d)	((d) ? (k)>>(32-(d)) : 0)

static char IndexMagic[] = "venti index configuration";

Index*
initindex(char *name, ISect **sects, int n)
{
	IFile f;
	Index *ix;
	ISect *is;
	uint32_t last, blocksize, tabsize;
	int i;

	if(n <= 0){
fprint(2, "bad n\n");
		seterr(EOk, "no index sections to initialize index");
		return nil;
	}
	ix = MKZ(Index);
	if(ix == nil){
fprint(2, "no mem\n");
		seterr(EOk, "can't initialize index: out of memory");
		freeindex(ix);
		return nil;
	}

	tabsize = sects[0]->tabsize;
	if(partifile(&f, sects[0]->part, sects[0]->tabbase, tabsize) < 0)
		return nil;
	if(parseindex(&f, ix) < 0){
		freeifile(&f);
		freeindex(ix);
		return nil;
	}
	freeifile(&f);
	if(namecmp(ix->name, name) != 0){
		seterr(ECorrupt, "mismatched index name: found %s expected %s", ix->name, name);
		return nil;
	}
	if(ix->nsects != n){
		seterr(ECorrupt, "mismatched number index sections: found %d expected %d", n, ix->nsects);
		freeindex(ix);
		return nil;
	}
	ix->sects = sects;
	last = 0;
	blocksize = ix->blocksize;
	for(i = 0; i < ix->nsects; i++){
		is = sects[i];
		if(namecmp(ix->name, is->index) != 0
		|| is->blocksize != blocksize
		|| is->tabsize != tabsize
		|| namecmp(is->name, ix->smap[i].name) != 0
		|| is->start != ix->smap[i].start
		|| is->stop != ix->smap[i].stop
		|| last != is->start
		|| is->start > is->stop){
			seterr(ECorrupt, "inconsistent index sections in %s", ix->name);
			freeindex(ix);
			return nil;
		}
		last = is->stop;
	}
	ix->tabsize = tabsize;
	ix->buckets = last;

	if(initindex1(ix) < 0){
		freeindex(ix);
		return nil;
	}

	ix->arenas = MKNZ(Arena*, ix->narenas);
	if(maparenas(ix->amap, ix->arenas, ix->narenas, ix->name) < 0){
		freeindex(ix);
		return nil;
	}

	return ix;
}

static int
initindex1(Index *ix)
{
	uint32_t buckets;

	ix->div = (((uint64_t)1 << 32) + ix->buckets - 1) / ix->buckets;
	buckets = (((uint64_t)1 << 32) - 1) / ix->div + 1;
	if(buckets != ix->buckets){
		seterr(ECorrupt, "inconsistent math for divisor and buckets in %s", ix->name);
		return -1;
	}

	return 0;
}

int
wbindex(Index *ix)
{
	Fmt f;
	ZBlock *b;
	int i;

	if(ix->nsects == 0){
		seterr(EOk, "no sections in index %s", ix->name);
		return -1;
	}
	b = alloczblock(ix->tabsize, 1, ix->blocksize);
	if(b == nil){
		seterr(EOk, "can't write index configuration: out of memory");
		return -1;
	}
	fmtzbinit(&f, b);
	if(outputindex(&f, ix) < 0){
		seterr(EOk, "can't make index configuration: table storage too small %d", ix->tabsize);
		freezblock(b);
		return -1;
	}
	for(i = 0; i < ix->nsects; i++){
		if(writepart(ix->sects[i]->part, ix->sects[i]->tabbase, b->data, ix->tabsize) < 0
		|| flushpart(ix->sects[i]->part) < 0){
			seterr(EOk, "can't write index: %r");
			freezblock(b);
			return -1;
		}
	}
	freezblock(b);

	for(i = 0; i < ix->nsects; i++)
		if(wbisect(ix->sects[i]) < 0)
			return -1;

	return 0;
}

/*
 * index: IndexMagic '\n' version '\n' name '\n' blocksize '\n' [V2: bitblocks '\n'] sections arenas
 * version, blocksize: uint32_t
 * name: max. ANameSize string
 * sections, arenas: AMap
 */
int
outputindex(Fmt *f, Index *ix)
{
	if(fmtprint(f, "%s\n%u\n%s\n%u\n", IndexMagic, ix->version, ix->name, ix->blocksize) < 0
	|| outputamap(f, ix->smap, ix->nsects) < 0
	|| outputamap(f, ix->amap, ix->narenas) < 0)
		return -1;
	return 0;
}

int
parseindex(IFile *f, Index *ix)
{
	AMapN amn;
	uint32_t v;
	char *s;

	/*
	 * magic
	 */
	s = ifileline(f);
	if(s == nil || strcmp(s, IndexMagic) != 0){
		seterr(ECorrupt, "bad index magic for %s", f->name);
		return -1;
	}

	/*
	 * version
	 */
	if(ifileuint32_t(f, &v) < 0){
		seterr(ECorrupt, "syntax error: bad version number in %s", f->name);
		return -1;
	}
	ix->version = v;
	if(ix->version != IndexVersion){
		seterr(ECorrupt, "bad version number in %s", f->name);
		return -1;
	}

	/*
	 * name
	 */
	if(ifilename(f, ix->name) < 0){
		seterr(ECorrupt, "syntax error: bad index name in %s", f->name);
		return -1;
	}

	/*
	 * block size
	 */
	if(ifileuint32_t(f, &v) < 0){
		seterr(ECorrupt, "syntax error: bad block size number in %s", f->name);
		return -1;
	}
	ix->blocksize = v;

	if(parseamap(f, &amn) < 0)
		return -1;
	ix->nsects = amn.n;
	ix->smap = amn.map;

	if(parseamap(f, &amn) < 0)
		return -1;
	ix->narenas = amn.n;
	ix->amap = amn.map;

	return 0;
}

/*
 * initialize an entirely new index
 */
Index *
newindex(char *name, ISect **sects, int n)
{
	Index *ix;
	AMap *smap;
	uint64_t nb;
	uint32_t div, ub, xb, start, stop, blocksize, tabsize;
	int i, j;

	if(n < 1){
		seterr(EOk, "creating index with no index sections");
		return nil;
	}

	/*
	 * compute the total buckets available in the index,
	 * and the total buckets which are used.
	 */
	nb = 0;
	blocksize = sects[0]->blocksize;
	tabsize = sects[0]->tabsize;
	for(i = 0; i < n; i++){
		/*
		 * allow index, start, and stop to be set if index is correct
		 * and start and stop are what we would have picked.
		 * this allows calling fmtindex to reformat the index after
		 * replacing a bad index section with a freshly formatted one.
		 * start and stop are checked below.
		 */
		if(sects[i]->index[0] != '\0' && strcmp(sects[i]->index, name) != 0){
			seterr(EOk, "creating new index using non-empty section %s", sects[i]->name);
			return nil;
		}
		if(blocksize != sects[i]->blocksize){
			seterr(EOk, "mismatched block sizes in index sections");
			return nil;
		}
		if(tabsize != sects[i]->tabsize){
			seterr(EOk, "mismatched config table sizes in index sections");
			return nil;
		}
		nb += sects[i]->blocks;
	}

	/*
	 * check for duplicate names
	 */
	for(i = 0; i < n; i++){
		for(j = i + 1; j < n; j++){
			if(namecmp(sects[i]->name, sects[j]->name) == 0){
				seterr(EOk, "duplicate section name %s for index %s", sects[i]->name, name);
				return nil;
			}
		}
	}

	if(nb >= ((uint64_t)1 << 32)){
		fprint(2, "%s: index is 2^32 blocks or more; ignoring some of it\n",
			argv0);
		nb = ((uint64_t)1 << 32) - 1;
	}

	div = (((uint64_t)1 << 32) + nb - 1) / nb;
	if(div < 100){
		fprint(2, "%s: index divisor %d too coarse; "
			"index larger than needed, ignoring some of it\n",
			argv0, div);
		div = 100;
		nb = (((uint64_t)1 << 32) - 1) / (100 - 1);
	}
	ub = (((uint64_t)1 << 32) - 1) / div + 1;
	if(ub > nb){
		seterr(EBug, "index initialization math wrong");
		return nil;
	}
	xb = nb - ub;

	/*
	 * initialize each of the index sections
	 * and the section map table
	 */
	smap = MKNZ(AMap, n);
	if(smap == nil){
		seterr(EOk, "can't create new index: out of memory");
		return nil;
	}
	start = 0;
	for(i = 0; i < n; i++){
		stop = start + sects[i]->blocks - xb / n;
		if(i == n - 1)
			stop = ub;

		if(sects[i]->start != 0 || sects[i]->stop != 0)
		if(sects[i]->start != start || sects[i]->stop != stop){
			seterr(EOk, "creating new index using non-empty section %s", sects[i]->name);
			return nil;
		}

		sects[i]->start = start;
		sects[i]->stop = stop;
		namecp(sects[i]->index, name);

		smap[i].start = start;
		smap[i].stop = stop;
		namecp(smap[i].name, sects[i]->name);
		start = stop;
	}

	/*
	 * initialize the index itself
	 */
	ix = MKZ(Index);
	if(ix == nil){
		seterr(EOk, "can't create new index: out of memory");
		free(smap);
		return nil;
	}
	ix->version = IndexVersion;
	namecp(ix->name, name);
	ix->sects = sects;
	ix->smap = smap;
	ix->nsects = n;
	ix->blocksize = blocksize;
	ix->buckets = ub;
	ix->tabsize = tabsize;
	ix->div = div;

	if(initindex1(ix) < 0){
		free(smap);
		return nil;
	}

	return ix;
}

ISect*
initisect(Part *part)
{
	ISect *is;
	ZBlock *b;
	int ok;

	b = alloczblock(HeadSize, 0, 0);
	if(b == nil || readpart(part, PartBlank, b->data, HeadSize) < 0){
		seterr(EAdmin, "can't read index section header: %r");
		return nil;
	}

	is = MKZ(ISect);
	if(is == nil){
		freezblock(b);
		return nil;
	}
	is->part = part;
	ok = unpackisect(is, b->data);
	freezblock(b);
	if(ok < 0){
		seterr(ECorrupt, "corrupted index section header: %r");
		freeisect(is);
		return nil;
	}

	if(is->version != ISectVersion1 && is->version != ISectVersion2){
		seterr(EAdmin, "unknown index section version %d", is->version);
		freeisect(is);
		return nil;
	}

	return initisect1(is);
}

ISect*
newisect(Part *part, uint32_t vers, char *name, uint32_t blocksize,
	 uint32_t tabsize)
{
	ISect *is;
	uint32_t tabbase;

	is = MKZ(ISect);
	if(is == nil)
		return nil;

	namecp(is->name, name);
	is->version = vers;
	is->part = part;
	is->blocksize = blocksize;
	is->start = 0;
	is->stop = 0;
	tabbase = (PartBlank + HeadSize + blocksize - 1) & ~(blocksize - 1);
	is->blockbase = (tabbase + tabsize + blocksize - 1) & ~(blocksize - 1);
	is->blocks = is->part->size / blocksize - is->blockbase / blocksize;
	is->bucketmagic = 0;
	if(is->version == ISectVersion2){
		do{
			is->bucketmagic = fastrand();
		}while(is->bucketmagic==0);
	}
	is = initisect1(is);
	if(is == nil)
		return nil;

	return is;
}

/*
 * initialize the computed parameters for an index
 */
static ISect*
initisect1(ISect *is)
{
	uint64_t v;

	is->buckmax = (is->blocksize - IBucketSize) / IEntrySize;
	is->blocklog = u64log2(is->blocksize);
	if(is->blocksize != (1 << is->blocklog)){
		seterr(ECorrupt, "illegal non-power-of-2 bucket size %d\n", is->blocksize);
		freeisect(is);
		return nil;
	}
	partblocksize(is->part, is->blocksize);
	is->tabbase = (PartBlank + HeadSize + is->blocksize - 1) & ~(is->blocksize - 1);
	if(is->tabbase >= is->blockbase){
		seterr(ECorrupt, "index section config table overlaps bucket storage");
		freeisect(is);
		return nil;
	}
	is->tabsize = is->blockbase - is->tabbase;
	v = is->part->size & ~(uint64_t)(is->blocksize - 1);
	if(is->blockbase + (uint64_t)is->blocks * is->blocksize != v){
		seterr(ECorrupt, "invalid blocks in index section %s", is->name);
		/* ZZZ what to do?
		freeisect(is);
		return nil;
		*/
	}

	if(is->stop - is->start > is->blocks){
		seterr(ECorrupt, "index section overflows available space");
		freeisect(is);
		return nil;
	}
	if(is->start > is->stop){
		seterr(ECorrupt, "invalid index section range");
		freeisect(is);
		return nil;
	}

	return is;
}

int
wbisect(ISect *is)
{
	ZBlock *b;

	b = alloczblock(HeadSize, 1, 0);
	if(b == nil){
		/* ZZZ set error? */
		return -1;
	}

	if(packisect(is, b->data) < 0){
		seterr(ECorrupt, "can't make index section header: %r");
		freezblock(b);
		return -1;
	}
	if(writepart(is->part, PartBlank, b->data, HeadSize) < 0 || flushpart(is->part) < 0){
		seterr(EAdmin, "can't write index section header: %r");
		freezblock(b);
		return -1;
	}
	freezblock(b);

	return 0;
}

void
freeisect(ISect *is)
{
	if(is == nil)
		return;
	free(is);
}

void
freeindex(Index *ix)
{
	int i;

	if(ix == nil)
		return;
	free(ix->amap);
	free(ix->arenas);
	if(ix->sects)
		for(i = 0; i < ix->nsects; i++)
			freeisect(ix->sects[i]);
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
uint64_t
writeiclump(Index *ix, Clump *c, uint8_t *clbuf)
{
	uint64_t a;
	int i;
	IAddr ia;
	AState as;

	trace(TraceLump, "writeiclump enter");
	qlock(&ix->writing);
	for(i = ix->mapalloc; i < ix->narenas; i++){
		a = writeaclump(ix->arenas[i], c, clbuf);
		if(a != TWID64){
			ix->mapalloc = i;
			ia.addr = ix->amap[i].start + a;
			ia.type = c->info.type;
			ia.size = c->info.uncsize;
			ia.blocks = (c->info.size + ClumpSize + (1<<ABlockLog) - 1) >> ABlockLog;
			as.arena = ix->arenas[i];
			as.aa = ia.addr;
			as.stats = as.arena->memstats;
			insertscore(c->info.score, &ia, IEDirty, &as);
			qunlock(&ix->writing);
			trace(TraceLump, "writeiclump exit");
			return ia.addr;
		}
	}
	qunlock(&ix->writing);

	seterr(EAdmin, "no space left in arenas");
	trace(TraceLump, "writeiclump failed");
	return TWID64;
}

/*
 * convert an arena index to an relative arena address
 */
Arena*
amapitoa(Index *ix, uint64_t a, uint64_t *aa)
{
	int i, r, l, m;

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
for(i=0; i<ix->narenas; i++)
	print("arena %d: %llux - %llux\n", i, ix->amap[i].start, ix->amap[i].stop);
print("want arena %d for %llux\n", l, a);
		seterr(ECrash, "unmapped address passed to amapitoa");
		return nil;
	}

	if(ix->arenas[l] == nil){
		seterr(ECrash, "unmapped arena selected in amapitoa");
		return nil;
	}
	*aa = a - ix->amap[l].start;
	return ix->arenas[l];
}

/*
 * convert an arena index to the bounds of the containing arena group.
 */
Arena*
amapitoag(Index *ix, uint64_t a, uint64_t *gstart, uint64_t *glimit,
	  int *g)
{
	uint64_t aa;
	Arena *arena;

	arena = amapitoa(ix, a, &aa);
	if(arena == nil)
		return nil;
	if(arenatog(arena, aa, gstart, glimit, g) < 0)
		return nil;
	*gstart += a - aa;
	*glimit += a - aa;
	return arena;
}

int
iaddrcmp(IAddr *ia1, IAddr *ia2)
{
	return ia1->type != ia2->type
		|| ia1->size != ia2->size
		|| ia1->blocks != ia2->blocks
		|| ia1->addr != ia2->addr;
}

/*
 * lookup the score in the partition
 *
 * nothing needs to be explicitly locked:
 * only static parts of ix are used, and
 * the bucket is locked by the DBlock lock.
 */
int
loadientry(Index *ix, uint8_t *score, int type, IEntry *ie)
{
	ISect *is;
	DBlock *b;
	IBucket ib;
	uint32_t buck;
	int h, ok;

	ok = -1;

	trace(TraceLump, "loadientry enter");

	/*
	qlock(&stats.lock);
	stats.indexreads++;
	qunlock(&stats.lock);
	*/

	if(!inbloomfilter(mainindex->bloom, score)){
		trace(TraceLump, "loadientry bloomhit");
		return -1;
	}

	trace(TraceLump, "loadientry loadibucket");
	b = loadibucket(ix, score, &is, &buck, &ib);
	trace(TraceLump, "loadientry loadedibucket");
	if(b == nil)
		return -1;

	if(okibucket(&ib, is) < 0){
		trace(TraceLump, "loadientry badbucket");
		goto out;
	}

	h = bucklook(score, type, ib.data, ib.n);
	if(h & 1){
		h ^= 1;
		trace(TraceLump, "loadientry found");
		unpackientry(ie, &ib.data[h]);
		ok = 0;
		goto out;
	}
	trace(TraceLump, "loadientry notfound");
	addstat(StatBloomFalseMiss, 1);
out:
	putdblock(b);
	trace(TraceLump, "loadientry exit");
	return ok;
}

int
okibucket(IBucket *ib, ISect *is)
{
	if(ib->n <= is->buckmax)
		return 0;

	seterr(EICorrupt, "corrupted disk index bucket: n=%u max=%u, range=[%lu,%lu)",
		ib->n, is->buckmax, is->start, is->stop);
	return -1;
}

/*
 * look for score within data;
 * return 1 | byte index of matching index,
 * or 0 | index of least element > score
 */
int
bucklook(uint8_t *score, int otype, uint8_t *data, int n)
{
	int i, r, l, m, h, c, cc, type;

	if(otype == -1)
		type = -1;
	else
		type = vttodisktype(otype);
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
		if(type != cc && type != -1){
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
 * compare two IEntries; consistent with bucklook
 */
int
ientrycmp(const void *vie1, const void *vie2)
{
	uint8_t *ie1, *ie2;
	int i, v1, v2;

	ie1 = (uint8_t*)vie1;
	ie2 = (uint8_t*)vie2;
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

/*
 * find the number of the index section holding bucket #buck
 */
int
indexsect0(Index *ix, uint32_t buck)
{
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
	return l - 1;
}

/*
 * load the index block at bucket #buck
 */
static DBlock*
loadibucket0(Index *ix, uint32_t buck, ISect **pis, uint32_t *pbuck,
	     IBucket *ib, int mode)
{
	ISect *is;
	DBlock *b;

	is = ix->sects[indexsect0(ix, buck)];
	if(buck < is->start || is->stop <= buck){
		seterr(EAdmin, "index lookup out of range: %u not found in index\n", buck);
		return nil;
	}

	buck -= is->start;
	if((b = getdblock(is->part, is->blockbase + ((uint64_t)buck << is->blocklog), mode)) == nil)
		return nil;

	if(pis)
		*pis = is;
	if(pbuck)
		*pbuck = buck;
	if(ib)
		unpackibucket(ib, b->data, is->bucketmagic);
	return b;
}

/*
 * find the number of the index section holding score
 */
int
indexsect1(Index *ix, uint8_t *score)
{
	return indexsect0(ix, hashbits(score, 32) / ix->div);
}

/*
 * load the index block responsible for score.
 */
static DBlock*
loadibucket1(Index *ix, uint8_t *score, ISect **pis, uint32_t *pbuck,
	     IBucket *ib)
{
	return loadibucket0(ix, hashbits(score, 32)/ix->div, pis, pbuck, ib, OREAD);
}

int
indexsect(Index *ix, uint8_t *score)
{
	return indexsect1(ix, score);
}

DBlock*
loadibucket(Index *ix, uint8_t *score, ISect **pis, uint32_t *pbuck,
	    IBucket *ib)
{
	return loadibucket1(ix, score, pis, pbuck, ib);
}


