/*
 * Memory-only VtBlock patch.
 *
 * The patchd Venti blocks are in the hash chains.
 * The patchd local blocks are only in the blocks array.
 * The free blocks are in the heap, which is supposed to
 * be indexed by second-to-last use but actually
 * appears to be last use.
 */
/* the change here is to a patch model: a local block is a patch replacing a 
 * global block. So much of what this code does is useful that 
 * I'm using most of it. A 'global' has a pointer to the data for a score
 * in the venti data area. A 'local' has a pointer to a block, consisting
 * of modified data. There can be many more blocks than currently, since
 * we have to keep everything around until a snap. This is not a big deal: 
 * we're planning to support, e.g., 512GB venti arenas on machines with 
 * a TB, so we've easily got enough memory for all the patches. 
 * Main change is that the patch structure gets
 * a redo. Maybe it would be best to do a full rewrite but I don't have
 * the time or the brains to get it right in the time I do have ...
 * you are welcome to make this much better. This 
 * code is intended to be linked with mmventi. 
 */

#include <u.h>
#include <libc.h>
#include <venti.h>
int getdata(u8int *score, u8int *data, u8int len, u8int blocktype);
int putdata(u8int *score, u8int *data, int len, uchar blocktype);

int vtpatchnread;
int vtpatchncopy;
int vtpatchnwrite;
int vttracelevel;

enum {
	BioLocal = 1,
	BioVenti,
	BioReading,
	BioWriting,
	BioEmpty,
	BioVentiError
};
enum {
	BadHeap = ~0
};

typedef struct VtPatch VtPatch;

struct VtPatch
{
	QLock	lk;
	VtConn	*z;
	u32int	blocksize;
	u32int	now;		/* ticks for usage time stamps */
	VtBlock	**hash;	/* hash table for finding addresses */
	int		nhash;
	VtBlock	**heap;	/* heap for finding victims */
	int		nheap;
	VtBlock	*block;	/* all allocated blocks */
	int		nblock;
	uchar	*mem;	/* memory for all blocks and data */
	int		(*write)(VtConn*, uchar[VtScoreSize], uint, uchar*, int);
};

static void patchcheck(VtPatch*);

VtPatch*
vtpatchalloc(int blocksize, ulong nblock)
{
	uchar *p;
	VtPatch *c;
	int i;
	VtBlock *b;

	c = vtmallocz(sizeof(VtPatch));

	c->blocksize = (blocksize + 127) & ~127;
	c->nblock = nblock;
	c->nhash = nblock;
	c->hash = vtmallocz(nblock*sizeof(VtBlock*));
	c->heap = vtmallocz(nblock*sizeof(VtBlock*));
	c->block = vtmallocz(nblock*sizeof(VtBlock));
	c->mem = vtmallocz(nblock*c->blocksize);

	p = c->mem;
	for(i=0; i<nblock; i++){
		b = &c->block[i];
		b->addr = NilBlock;
		b->c = (VtCache *)c; /* XXX */
		b->data = p;
		b->heap = i;
		c->heap[i] = b;
		p += c->blocksize;
	}
	c->nheap = nblock;
	patchcheck(c);
	return c;
}

void
vtpatchfree(VtPatch *c)
{
	int i;

	qlock(&c->lk);

	patchcheck(c);
	for(i=0; i<c->nblock; i++)
		assert(c->block[i].ref == 0);

	vtfree(c->hash);
	vtfree(c->heap);
	vtfree(c->block);
	vtfree(c->mem);
	vtfree(c);
}

static void
vtpatchdump(VtPatch *c)
{
	int i;
	VtBlock *b;

	for(i=0; i<c->nblock; i++){
		b = &c->block[i];
		print("patch block %d: type %d score %V iostate %d addr %d ref %d nlock %d\n",
			i, b->type, b->score, b->iostate, b->addr, b->ref, b->nlock);
	}
}

static void
patchcheck(VtPatch *c)
{
	u32int size, now;
	int i, k, refed;
	VtBlock *b;

	size = c->blocksize;
	now = c->now;

	for(i = 0; i < c->nheap; i++){
		if(c->heap[i]->heap != i)
			sysfatal("mis-heaped at %d: %d", i, c->heap[i]->heap);
		if(i > 0 && c->heap[(i - 1) >> 1]->used - now > c->heap[i]->used - now)
			sysfatal("bad heap ordering");
		k = (i << 1) + 1;
		if(k < c->nheap && c->heap[i]->used - now > c->heap[k]->used - now)
			sysfatal("bad heap ordering");
		k++;
		if(k < c->nheap && c->heap[i]->used - now > c->heap[k]->used - now)
			sysfatal("bad heap ordering");
	}

	refed = 0;
	for(i = 0; i < c->nblock; i++){
		b = &c->block[i];
		if(b->data != &c->mem[i * size])
			sysfatal("mis-blocked at %d", i);
		if(b->ref && b->heap == BadHeap)
			refed++;
		else if(b->addr != NilBlock)
			refed++;
	}
	assert(c->nheap + refed == c->nblock);
	refed = 0;
	for(i = 0; i < c->nblock; i++){
		b = &c->block[i];
		if(b->ref){
			refed++;
		}
	}
}

static int
upheap(int i, VtBlock *b)
{
	VtBlock *bb;
	u32int now;
	int p;
	VtPatch *c;
	
	c = (VtPatch *)b->c;
	now = c->now;
	for(; i != 0; i = p){
		p = (i - 1) >> 1;
		bb = c->heap[p];
		if(b->used - now >= bb->used - now)
			break;
		c->heap[i] = bb;
		bb->heap = i;
	}
	c->heap[i] = b;
	b->heap = i;

	return i;
}

static int
downheap(int i, VtBlock *b)
{
	VtBlock *bb;
	u32int now;
	int k;
	VtPatch *c;
	
	c = (VtPatch *)b->c;
	now = c->now;
	for(; ; i = k){
		k = (i << 1) + 1;
		if(k >= c->nheap)
			break;
		if(k + 1 < c->nheap && c->heap[k]->used - now > c->heap[k + 1]->used - now)
			k++;
		bb = c->heap[k];
		if(b->used - now <= bb->used - now)
			break;
		c->heap[i] = bb;
		bb->heap = i;
	}
	c->heap[i] = b;
	b->heap = i;
	return i;
}

/*
 * Delete a block from the heap.
 * Called with c->lk held.
 */
static void
heapdel(VtBlock *b)
{
	int i, si;
	VtPatch *c;

	c = (VtPatch *)b->c;

	si = b->heap;
	if(si == BadHeap)
		return;
	b->heap = BadHeap;
	c->nheap--;
	if(si == c->nheap)
		return;
	b = c->heap[c->nheap];
	i = upheap(si, b);
	if(i == si)
		downheap(i, b);
}

/*
 * Insert a block into the heap.
 * Called with c->lk held.
 */
static void
heapins(VtBlock *b)
{
	assert(b->heap == BadHeap);
	upheap(((VtPatch *)b->c)->nheap++, b);
}

/*
 * locate the vtBlock with the oldest second to last use.
 * remove it from the heap, and fix up the heap.
 */
/* called with c->lk held */
static VtBlock*
vtpatchbumpblock(VtPatch *c)
{
	VtBlock *b;

	/*
	 * locate the vtBlock with the oldest second to last use.
	 * remove it from the heap, and fix up the heap.
	 */
	if(c->nheap == 0){
		vtpatchdump(c);
		fprint(2, "vtpatchbumpblock: no free blocks in vtPatch");
		abort();
	}
	b = c->heap[0];
	heapdel(b);

	assert(b->heap == BadHeap);
	assert(b->ref == 0);

	/*
	 * unchain the vtBlock from hash chain if any
	 */
	if(b->prev){
		*(b->prev) = b->next;
		if(b->next)
			b->next->prev = b->prev;
		b->prev = nil;
	}

 	
if(0)fprint(2, "droping %x:%V\n", b->addr, b->score);
	/* set vtBlock to a reasonable state */
	b->ref = 1;
	b->iostate = BioEmpty;
	return b;
}

/*
 * fetch a local block from the memory patch.
 * if it's not there, load it, bumping some other Block.
 * if we're out of free blocks, we're screwed.
 */
VtBlock*
vtpatchlocal(VtPatch *c, u32int addr, int type)
{
	VtBlock *b;

	if(addr == 0)
		sysfatal("vtpatchlocal: asked for nonexistent block 0");
	if(addr > c->nblock)
		sysfatal("vtpatchlocal: asked for block #%ud; only %d blocks",
			addr, c->nblock);

	b = &c->block[addr-1];
	if(b->addr == NilBlock || b->iostate != BioLocal)
		sysfatal("vtpatchlocal: block is not local");

	if(b->type != type)
		sysfatal("vtpatchlocal: block has wrong type %d != %d", b->type, type);

	qlock(&c->lk);
	b->ref++;
	qunlock(&c->lk);

	qlock(&b->lk);
	b->nlock = 1;
	b->pc = getcallerpc(&c);
	return b;
}

VtBlock*
vtpatchallocblock(VtPatch *c, int type)
{
	VtBlock *b;

	qlock(&c->lk);
	b = vtpatchbumpblock(c);
	b->iostate = BioLocal;
	b->type = type;
	b->addr = (b - c->block)+1;
	vtzeroextend(type, b->data, 0, c->blocksize);
	vtlocaltoglobal(b->addr, b->score);
	qunlock(&c->lk);

	qlock(&b->lk);
	b->nlock = 1;
	b->pc = getcallerpc(&c);
	return b;
}

/*
 * fetch a global (Venti) block from the memory patch.
 * Return a pointer to it. 
 */
VtBlock*
vtpatchglobal(VtPatch *c, uchar score[VtScoreSize], int type)
{
	VtBlock *b;
	ulong h;
	int n;
	u32int addr;

	if(vttracelevel)
		fprint(2, "vtpatchglobal %V %d from %p\n", score, type, getcallerpc(&c));
	addr = vtglobaltolocal(score);
	if(addr != NilBlock){
		if(vttracelevel)
			fprint(2, "vtpatchglobal %V %d => local\n", score, type);
		b = vtpatchlocal(c, addr, type);
		if(b)
			b->pc = getcallerpc(&c);
		return b;
	}

	h = (u32int)(score[0]|(score[1]<<8)|(score[2]<<16)|(score[3]<<24)) % c->nhash;

	/*
	 * look for the block in the patch
	 */
	qlock(&c->lk);
	for(b = c->hash[h]; b != nil; b = b->next){
		if(b->addr != NilBlock || memcmp(b->score, score, VtScoreSize) != 0 || b->type != type)
			continue;
		heapdel(b);
		b->ref++;
		qunlock(&c->lk);
		if(vttracelevel)
			fprint(2, "vtpatchglobal %V %d => found in patch %p; locking\n", score, type, b);
		qlock(&b->lk);
		b->nlock = 1;
		if(b->iostate == BioVentiError){
			if(chattyventi)
				fprint(2, "patchd read error for %V\n", score);
			if(vttracelevel)
				fprint(2, "vtpatchglobal %V %d => patch read error\n", score, type);
			vtblockput(b);
			werrstr("venti i/o error");
			return nil;
		}
		if(vttracelevel)
			fprint(2, "vtpatchglobal %V %d => found in patch; returning\n", score, type);
		b->pc = getcallerpc(&c);
		return b;
	}

	/*
	 * not found
	 */
	b = vtpatchbumpblock(c);
	b->addr = NilBlock;
	b->type = type;
	memmove(b->score, score, VtScoreSize);
	/* chain onto correct hash */
	b->next = c->hash[h];
	c->hash[h] = b;
	if(b->next != nil)
		b->next->prev = &b->next;
	b->prev = &c->hash[h];

	/*
	 * Lock b before unlocking c, so that others wait while we read.
	 * 
	 * You might think there is a race between this qlock(b) before qunlock(c)
	 * and the qlock(c) while holding a qlock(b) in vtblockwrite.  However,
	 * the block here can never be the block in a vtblockwrite, so we're safe.
	 * We're certainly living on the edge.
	 */
	if(vttracelevel)
		fprint(2, "vtpatchglobal %V %d => bumped; locking %p\n", score, type, b);
	qlock(&b->lk);
	b->nlock = 1;
	qunlock(&c->lk);

	vtpatchnread++;
	n = getdata(score, b->data, c->blocksize, type);
	if(n < 0){
		if(chattyventi)
			fprint(2, "read %V: %r\n", score);
		if(vttracelevel)
			fprint(2, "vtpatchglobal %V %d => bumped; read error\n", score, type);
		b->iostate = BioVentiError;
		vtblockput(b);
		return nil;
	}
	vtzeroextend(type, b->data, n, c->blocksize);
	b->iostate = BioVenti;
	b->nlock = 1;
	if(vttracelevel)
		fprint(2, "vtpatchglobal %V %d => loaded into patch; returning\n", score, type);
	b->pc = getcallerpc(&b);
	return b;
}

/*
 * we're done with the block.
 * unlock it.  can't use it after calling this.
 */
void
patchblockput(VtBlock* b)
{
	VtPatch *c;

	if(b == nil)
		return;

if(0)fprint(2, "vtblockput: %d: %x %d %d\n", getpid(), b->addr, c->nheap, b->iostate);
	if(vttracelevel)
		fprint(2, "vtblockput %p from %p\n", b, getcallerpc(&b));

	if(--b->nlock > 0)
		return;

	/*
	 * b->nlock should probably stay at zero while
	 * the vtBlock is unlocked, but diskThread and vtSleep
	 * conspire to assume that they can just qlock(&b->lk); vtblockput(b),
	 * so we have to keep b->nlock set to 1 even 
	 * when the vtBlock is unlocked.
	 */
	assert(b->nlock == 0);
	b->nlock = 1;

	qunlock(&b->lk);
	c = (VtPatch *)b->c;
	qlock(&c->lk);

	if(--b->ref > 0){
		qunlock(&c->lk);
		return;
	}

	assert(b->ref == 0);
	switch(b->iostate){
	case BioVenti:
/*if(b->addr != NilBlock) print("blockput %d\n", b->addr); */
		b->used = c->now++;
		/* fall through */
	case BioVentiError:
		heapins(b);
		break;
	case BioLocal:
		break;
	}
	qunlock(&c->lk);
}

int
patchblockwrite(VtBlock *b)
{
	uchar score[VtScoreSize];
	VtPatch *c;
	uint h;
	int n;

	if(b->iostate != BioLocal){
		werrstr("vtblockwrite: not a local block");
		return -1;
	}

	c = (VtPatch *)b->c;
	n = vtzerotruncate(b->type, b->data, c->blocksize);
	vtpatchnwrite++;
	if(putdata(score, b->data, n, b->type) < 0)
		return -1;

	memmove(b->score, score, VtScoreSize);

	qlock(&c->lk);
	b->addr = NilBlock;	/* now on venti */
	b->iostate = BioVenti;
	h = (u32int)(score[0]|(score[1]<<8)|(score[2]<<16)|(score[3]<<24)) % c->nhash;
	b->next = c->hash[h];
	c->hash[h] = b;
	if(b->next != nil)
		b->next->prev = &b->next;
	b->prev = &c->hash[h];
	qunlock(&c->lk);
	return 0;
}

VtBlock*
patchblockcopy(VtBlock *b)
{
	VtBlock *bb;

	vtpatchncopy++;
	bb = vtpatchallocblock(((VtPatch *)b->c), b->type);
	if(bb == nil){
		patchblockput(b);
		return nil;
	}
	memmove(bb->data, b->data, ((VtPatch *)b->c)->blocksize);
	patchblockput(b);
	bb->pc = getcallerpc(&b);
	return bb;
}


