#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct MetaChunk MetaChunk;

struct MetaChunk {
	ushort offset;
	ushort size;
	ushort index;
};

static void usage(void);
static void setBit(uchar *bmap, ulong addr);
static int getBit(uchar *bmap, ulong addr);
static int readLabel(Label *l, u32int addr);
static void error(char*, ...);
static void warn(char *fmt, ...);
static void chkEpoch(u32int epoch);
static void chkFree(void);
static int readLabel(Label *l, u32int addr);
static void chkDir(char *name, Source *source, Source *meta);

#pragma	varargck	argpos	error	1
#pragma	varargck	argpos	warn	1

uchar *amap;		/* bitmap: has been visited at all */
uchar *emap;		/* bitmap: see condition (ii) below */
uchar *vmap;		/* bitmap: has been visited in this epoch */
uchar *xmap;		/* bitmap: see condition (iii) below */

Fs *fs;
Cache *cache;
int nblocks;
int bsize;
int badactive;
int dumpblocks;	/* write lost blocks into /tmp/lost */
int fast;		/* don't check that all the venti blocks are there */
u32int hint;	/* a guess at where chkEpoch might look to find the next root */

void
main(int argc, char *argv[])
{
	int csize = 1000;
	VtSession *z;
	char *host = nil;
	u32int e;
	Source *r, *mr;
	Block *b;
	Super super;

	ARGBEGIN{
	default:
		usage();
	case 'c':
		csize = atoi(ARGF());
		break;
	case 'd':
		dumpblocks = 1;
		break;
	case 'f':
		fast = 1;
		break;
	case 'h':
		host = ARGF();
		break;
	}ARGEND;

	if(argc != 1)
		usage();

	vtAttach();

	fmtinstall('L', labelFmt);
	fmtinstall('V', scoreFmt);
	fmtinstall('R', vtErrFmt);

	/*
	 * Connect to Venti.
	 */
	z = vtDial(host, 0);
	if(z == nil){
		if(!fast)
			vtFatal("could not connect to server: %s", vtGetError());
	}else if(!vtConnect(z, 0))
		vtFatal("vtConnect: %s", vtGetError());

	/*
	 * Initialize file system.
	 */
	fs = fsOpen(argv[0], z, csize, OReadOnly);
	if(fs == nil)
		vtFatal("could not open file system: %R");
	cache = fs->cache;
	nblocks = cacheLocalSize(cache, PartData);
	bsize = fs->blockSize;

	b = superGet(cache, &super);
	if(b == nil)
		vtFatal("could not load super block: %R");
	blockPut(b);

	hint = super.active;

	/*
	 * Run checks.
	 */
	amap = vtMemAllocZ(nblocks/8 + 1);
	emap = vtMemAllocZ(nblocks/8 + 1);
	vmap = vtMemAllocZ(nblocks/8 + 1);
	xmap = vtMemAllocZ(nblocks/8 + 1);
	for(e=fs->ehi; e >= fs->elo; e--){
		memset(emap, 0, nblocks/8+1);
		memset(xmap, 0, nblocks/8+1);
		chkEpoch(e);
	}
	chkFree();
	vtMemFree(amap);
	vtMemFree(emap);
	vtMemFree(vmap);
	vtMemFree(xmap);

	sourceLock(fs->source, OReadOnly);
	r = sourceOpen(fs->source, 0, OReadOnly);
	mr = sourceOpen(fs->source, 1, OReadOnly);
	sourceUnlock(fs->source);
	chkDir("", r, mr);

	sourceClose(r);
	sourceClose(mr);

	fsClose(fs);

	exits(0);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-c cachesize] [-h host] file\n", argv0);
	exits("usage");
}

/*
 * When b points at bb, need to check:
 *
 * (i) b.e in [bb.e, bb.eClose)
 * (ii) if b.e==bb.e,  then no other b' in e points at bb.
 * (iii) if !(b.state&Copied) and b.e==bb.e then no other b' points at bb.
 * (iv) if b is active then no other active b' points at bb.
 * (v) if b is a past life of b' then only one of b and b' is active (too hard to check)
 *
 * Does not walk onto Venti.
 */

static int
walk(Block *b, uchar score[VtScoreSize], int type, u32int tag, u32int epoch)
{
	Block *bb;
	u32int addr;
	int i, ret;
	u32int ep;
	Entry e;

	if(fast && globalToLocal(score) == NilBlock)
		return 1;

	bb = cacheGlobal(cache, score, type, tag, OReadOnly);
	if(bb == nil){
		error("could not load block %V type %d tag %ux: %R", score, type, tag);
		return 0;
	}

	ret = 0;
	addr = globalToLocal(score);
	if(addr == NilBlock){
		ret = 1;
		goto Exit;
	}

	/* (i) */
	if(b->l.epoch < bb->l.epoch || bb->l.epochClose <= b->l.epoch){
		error("walk: block %#ux [%ud, %ud) points at %#ux [%ud, %ud)\n",
			b->addr, b->l.epoch, b->l.epochClose,
			bb->addr, bb->l.epoch, bb->l.epochClose);
		goto Exit;
	}

	/* (ii) */
	if(b->l.epoch == epoch && bb->l.epoch == epoch){
		if(getBit(emap, addr)){
			error("walk: epoch join detected: addr %#ux %L\n", bb->addr, &bb->l);
			goto Exit;
		}
		setBit(emap, addr);
	}

	/* (iii) */
	if(!(b->l.state&BsCopied) && b->l.epoch == bb->l.epoch){
		if(getBit(xmap, addr)){
			error("walk: copy join detected; addr %#ux %L\n", bb->addr, &bb->l);
			goto Exit;
		}
		setBit(xmap, addr);
	}

	/* (iv) */
	if(epoch == fs->ehi){
		/* since epoch==fs->ehi is first, amap is same as ``have seen active'' */
		if(getBit(amap, addr)){
			error("walk: active join detected: addr %#ux %L\n", bb->addr, &bb->l);
			goto Exit;
		}
	}

	if(getBit(vmap, addr)){
		ret = 1;
		goto Exit;
	}

	setBit(vmap, addr);
	setBit(amap, addr);

	b = nil;		/* make sure no more refs to parent */
	USED(b);

	switch(type){
	default:
		/* pointer block */
		for(i=0; i<bsize/VtScoreSize; i++)
			if(!walk(bb, bb->data + i*VtScoreSize, type-1, tag, epoch))
				print("# clrp %#ux %d\n", bb->addr, i);
		break;
	case BtData:
		break;
	case BtDir:
		for(i=0; i<bsize/VtEntrySize; i++){
			if(!entryUnpack(&e, bb->data, i)){
				error("walk: could not unpack entry: %ux[%d]: %R", addr, i);
				print("# clre %#ux %d\n", bb->addr, i);
				continue;
			}
			if(!(e.flags & VtEntryActive))
				continue;
//fprint(2, "%x[%d] tag=%x snap=%d score=%V\n", addr, i, e.tag, e.snap, e.score);
			ep = epoch;
			if(e.snap != 0){
				if(e.snap >= epoch){
					error("bad snap in entry: %ux[%d] snap = %ud: epoch = %ud",
						addr, i, e.snap, epoch);
					print("# clre %#ux %d\n", bb->addr, i);
					continue;
				}
				continue;
			}
			if(e.flags & VtEntryLocal){
				if(e.tag < UserTag)
				if(e.tag != RootTag || tag != RootTag || i != 1){
					error("bad tag in entry: %ux[%d] tag = %ux", addr, i, e.tag);
					print("# clre %#ux %d\n", bb->addr, i);
					continue;
				}
			}else{
				if(e.tag != 0){
					error("bad tag in entry: %ux[%d] tag = %ux", addr, i, e.tag);
					print("# clre %#ux %d\n", bb->addr, i);
					continue;
				}
			}
			if(!walk(bb, e.score, entryType(&e), e.tag, ep))
				print("# clre %#ux %d\n", bb->addr, i);
		}
		break;
	}

	ret = 1;

Exit:
	blockPut(bb);
	return ret;
}

static void
chkEpoch(u32int epoch)
{
	u32int a;
	Label l;
	Entry e;
	Block *b;

	print("chkEpoch %ud\n", epoch);
	
	/* find root block */
	for(a=0; a<nblocks; a++){
		if(!readLabel(&l, (a+hint)%nblocks)){
			error("could not read label: addr %ux %d %ux %ux: %R", a, l.type, l.state, l.state);
			continue;
		}
		if(l.tag == RootTag && l.epoch == epoch)
			break;
	}

	if(a == nblocks){
		print("chkEpoch: could not find root block for epoch: %ud\n", epoch);
		return;
	}

	a = (a+hint)%nblocks;
	b = cacheLocalData(cache, a, BtDir, RootTag, OReadOnly, 0);
	if(b == nil){
		error("could not read root block %ux: %R\n", a);
		return;
	}

	/* no one should point at the root blocks */
	setBit(amap, a);
	setBit(emap, a);
	setBit(vmap, a);
	setBit(xmap, a);

	/*
	 * First entry is the rest of the file system.
	 * Second entry is link to previous epoch root,
	 * just a convenience to help the search.
	 */
	if(!entryUnpack(&e, b->data, 0)){
		error("could not unpack root block %ux: %R", a);
		blockPut(b);
		return;
	}
	walk(b, e.score, BtDir, e.tag, epoch);
	if(entryUnpack(&e, b->data, 1))
		hint = globalToLocal(e.score);
	blockPut(b);
}

/*
 * We've just walked the whole write buffer.  Notice blocks that
 * aren't marked available but that we didn't visit.  They are lost.
 */
static void
chkFree(void)
{
	char buf[64];
	int fd;
	u32int a;
	Block *b;
	Label l;
	u32int nfree;
	u32int nlost;

	nfree = 0;
	nlost = 0;
	/* find root block */
	for(a=0; a<nblocks; a++){
		if(!readLabel(&l, a)){
			error("could not read label: addr %ux %d %d: %R",
				a, l.type, l.state);
			continue;
		}
		if(getBit(amap, a))
			continue;
		if(l.state == BsFree || l.epochClose <= fs->elo){
			nfree++;
			setBit(amap, a);
			continue;
		}
		nlost++;
		warn("unreachable block: addr %ux type %d tag %ux state %s epoch %ud close %ud",
			a, l.type, l.tag, bsStr(l.state), l.epoch, l.epochClose);
		print("# bfree %#ux\n", a);
		if(dumpblocks){
			sprint(buf, "/tmp/lost.%ux", a);
			if((fd = create(buf, OWRITE, 0666)) < 0){
				fprint(2, "create %s: %r\n", buf);
				goto nodump;
			}
			if((b = cacheLocal(cache, PartData, a, OReadOnly)) == nil){
				close(fd);
				fprint(2, "load block %ux: %R\n", a);
				goto nodump;
			}
			if(write(fd, b->data, bsize) != bsize)
				fprint(2, "writiting %s: %r\n", buf);
			close(fd);
			blockPut(b);
		}
	    nodump:
		setBit(amap, a);
	}
	fprint(2, "\tused=%ud free space = %ud(%f%%) lost=%ud\n",
		nblocks-nfree-nlost, nblocks, 100.*nfree/nblocks, nlost);
}

static Source *
openSource(Source *s, char *name, uchar *bm, u32int offset, u32int gen, int dir)
{	
	Source *r;

	if(getBit(bm, offset)){
		warn("multiple references to source: %s -> %d", name, offset);
		print("# clri %s\n", name);
		return nil;
	}
	setBit(bm, offset);

	r = sourceOpen(s, offset, OReadOnly);
	if(r == nil){
		warn("could not open source: %s -> %d: %R", name, offset);
		print("# clri %s\n", name);
		return nil;
	}

	if(r->gen != gen){
		warn("source has been removed: %s -> %d", name, offset);
		print("# clri %s\n", name);
		goto Err;
	}

	if(r->dir != dir){
		warn("dir mismatch: %s -> %d", name, offset);
		print("# clri %s\n", name);
		goto Err;
	}
	return r;
Err:
	sourceClose(r);
	return nil;
}

static int
offsetCmp(void *s0, void *s1)
{
	MetaChunk *mc0, *mc1;

	mc0 = s0;
	mc1 = s1;
	if(mc0->offset < mc1->offset)
		return -1;
	if(mc0->offset > mc1->offset)
		return 1;
	return 0;
}

/* 
 * Check that MetaBlock has reasonable header, sorted entries,
 */
int
chkMetaBlock(MetaBlock *mb)
{
	MetaChunk *mc;
	int oo, o, n, i;
	uchar *p;

	mc = vtMemAlloc(mb->nindex*sizeof(MetaChunk));
	p = mb->buf + MetaHeaderSize;
	for(i = 0; i<mb->nindex; i++){
		mc[i].offset = (p[0]<<8) | p[1];
		mc[i].size = (p[2]<<8) | p[3];
		mc[i].index = i;
		p += MetaIndexSize;
	}

	qsort(mc, mb->nindex, sizeof(MetaChunk), offsetCmp);

	/* check block looks ok */
	oo = MetaHeaderSize + mb->maxindex*MetaIndexSize;
	o = oo;
	n = 0;
	for(i=0; i<mb->nindex; i++){
		o = mc[i].offset;
		n = mc[i].size;
		if(o < oo)
			goto Err;
		oo += n;
	}
	if(o+n > mb->size)
		goto Err;
	if(mb->size - oo != mb->free)
		goto Err;

	vtMemFree(mc);
	return 1;
Err:
fprint(2, "metaChunks failed!\n");
oo = MetaHeaderSize + mb->maxindex*MetaIndexSize;
for(i=0; i<mb->nindex; i++){
fprint(2, "\t%d: %d %d\n", i, mc[i].offset, mc[i].offset + mc[i].size);
oo += mc[i].size;
}
fprint(2, "\tused=%d size=%d free=%d free2=%d\n", oo, mb->size, mb->free, mb->size - oo);
	vtMemFree(mc);
	return 0;
}

/*
 * Walk the source tree making sure that the BtData
 * sources containing directory entries are okay.
 *
 * Walks onto Venti, so takes a long time.
 */
static void
chkDir(char *name, Source *source, Source *meta)
{
	uchar *bm;
	Block *b, *bb;
	u32int nb, o;
	MetaBlock mb;
	DirEntry de;
	MetaEntry me;
	int i;
	char *s, *nn;
	Source *r, *mr;

	if(fast && globalToLocal(source->score)==NilBlock && globalToLocal(meta->score)==NilBlock)
		return;

	if(!sourceLock2(source, meta, OReadOnly)){
		warn("could not lock sources for %s: %R", name);
		return;
	}

	bm = vtMemAllocZ(sourceGetDirSize(source)/8 + 1);
	
	nb = (sourceGetSize(meta) + meta->dsize - 1)/meta->dsize;
	for(o=0; o<nb; o++){
		b = sourceBlock(meta, o, OReadOnly);
if(0)fprint(2, "source %V:%d block %d addr %d\n", source->score, source->offset, o, b->addr);
		if(b == nil){
			error("could not read block in meta file: %s[%ud]: %R", name, o);
			continue;
		}
		if(!mbUnpack(&mb, b->data, meta->dsize)){
			error("could not unpack meta block: %s[%ud]: %R", name, o);
			blockPut(b);
			continue;
		}
		if(!chkMetaBlock(&mb)){
			error("bad meta block: %s[%ud]: %R", name, o);
			blockPut(b);
			continue;
		}
		s = vtStrDup("");
		for(i=0; i<mb.nindex; i++){
			meUnpack(&me, &mb, i);
			if(!deUnpack(&de, &me)){
				error("cound not unpack dir entry: %s[%ud][%d]: %R", name, o, i);
				continue;
			}
			if(strcmp(s, de.elem) >= 0)
				error("dir entry out of order: %s[%ud][%d] = %s last = %s", name, o, i,
					de.elem, s);
			vtMemFree(s);
			s = vtStrDup(de.elem);
			nn = smprint("%s/%s", name, de.elem);
			if(!(de.mode & ModeDir)){
				r = openSource(source, nn, bm, de.entry, de.gen, 0);
				if(r != nil)
					sourceClose(r);
				deCleanup(&de);
				free(nn);
				continue;
			}

			r = openSource(source, nn, bm, de.entry, de.gen, 1);
			if(r == nil){
				deCleanup(&de);
				free(nn);
				continue;
			}

			mr = openSource(source, nn, bm, de.mentry, de.mgen, 0);
			if(mr == nil){
				sourceClose(r);
				deCleanup(&de);
				free(nn);
				continue;
			}
			
			chkDir(nn, r, mr);

			sourceClose(mr);
			sourceClose(r);
			deCleanup(&de);
			free(nn);
			deCleanup(&de);

		}
		vtMemFree(s);
		blockPut(b);
	}

	nb = sourceGetDirSize(source);
	for(o=0; o<nb; o++){
		if(getBit(bm, o))
			continue;
		r = sourceOpen(source, o, OReadOnly);
		if(r == nil)
			continue;
		warn("non referenced entry in source %s[%d]", name, o);
		if((bb = sourceBlock(source, o/(source->dsize/VtEntrySize), OReadOnly)) != nil){
			if(bb->addr != NilBlock)
				print("# clre %#ux %d\n", bb->addr, o%(source->dsize/VtEntrySize));
			blockPut(bb);
		}
		sourceClose(r);
	}
	
	sourceUnlock(source);
	sourceUnlock(meta);
	vtMemFree(bm);
}


static void
setBit(uchar *bmap, ulong addr)
{
	bmap[addr>>3] |= 1 << (addr & 7);
}

static int
getBit(uchar *bmap, ulong addr)
{
	return (bmap[addr>>3] >> (addr & 7)) & 1;
}

static int
readLabel(Label *l, u32int addr)
{
	int lpb;
	Block *b;
	u32int a;

	lpb = bsize / LabelSize;
	a = addr / lpb;
	b = cacheLocal(cache, PartLabel, a, OReadOnly);
	if(b == nil){
		blockPut(b);
		return 0;
	}

	if(!labelUnpack(l, b->data, addr%lpb)){
		print("labelUnpack %ux failed\n", addr);
		blockPut(b);
		return 0;
	}
	blockPut(b);
	return 1;
}

static void
error(char *fmt, ...)
{
	static nerr;
	va_list arg;
	char buf[128];


	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	print("error: %s\n", buf);

//	if(nerr++ > 20)
//		vtFatal("too many errors");
}

static void
warn(char *fmt, ...)
{
	static nerr;
	va_list arg;
	char buf[128];


	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	print("warn: %s\n", buf);
}
