#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct AHash	AHash;

/*
 * hash table for finding arena's based on their names.
 */
struct AHash
{
	AHash	*next;
	Arena	*arena;
};

enum
{
	AHashSize	= 512
};

static AHash	*ahash[AHashSize];

static u32int
hashStr(char *s)
{
	u32int h;
	int c;

	h = 0;
	for(; c = *s; s++){
		c ^= c << 6;
		h += (c << 11) ^ (c >> 1);
		c = *s;
		h ^= (c << 14) + (c << 7) + (c << 4) + c;
	}
	return h;
}

int
addArena(Arena *arena)
{
	AHash *a;
	u32int h;

	h = hashStr(arena->name) & (AHashSize - 1);
	a = MK(AHash);
	if(a == nil)
		return 0;
	a->arena = arena;
	a->next = ahash[h];
	ahash[h] = a;
	return 1;
}

Arena*
findArena(char *name)
{
	AHash *a;
	u32int h;

	h = hashStr(name) & (AHashSize - 1);
	for(a = ahash[h]; a != nil; a = a->next)
		if(strcmp(a->arena->name, name) == 0)
			return a->arena;
	return nil;
}

int
delArena(Arena *arena)
{
	AHash *a, *last;
	u32int h;

	h = hashStr(arena->name) & (AHashSize - 1);
	last = nil;
	for(a = ahash[h]; a != nil; a = a->next){
		if(a->arena == arena){
			if(last != nil)
				last->next = a->next;
			else
				ahash[h] = a->next;
			free(a);
			return 1;
		}
		last = a;
	}
	return 0;
}

ArenaPart*
initArenaPart(Part *part)
{
	AMapN amn;
	ArenaPart *ap;
	ZBlock *b;
	u32int i;
	int ok;

	b = allocZBlock(HeadSize, 0);
	if(b == nil || !readPart(part, PartBlank, b->data, HeadSize)){
		setErr(EAdmin, "can't read arena partition header: %R");
		return nil;
	}

	ap = MKZ(ArenaPart);
	if(ap == nil){
		freeZBlock(b);
		return nil;
	}
	ap->part = part;
	ok = unpackArenaPart(ap, b->data);
	freeZBlock(b);
	if(!ok){
		setErr(ECorrupt, "corrupted arena partition header: %R");
		freeArenaPart(ap, 0);
		return nil;
	}

	ap->tabBase = (PartBlank + HeadSize + ap->blockSize - 1) & ~(ap->blockSize - 1);
	if(ap->version != ArenaPartVersion){
		setErr(ECorrupt, "unknown arena partition version %d", ap->version);
		freeArenaPart(ap, 0);
		return nil;
	}
	if(ap->blockSize & (ap->blockSize - 1)){
		setErr(ECorrupt, "illegal non-power-of-2 block size %d\n", ap->blockSize);
		freeArenaPart(ap, 0);
		return nil;
	}
	if(ap->tabBase >= ap->arenaBase){
		setErr(ECorrupt, "arena partition table overlaps with arena storage");
		freeArenaPart(ap, 0);
		return nil;
	}
	ap->tabSize = ap->arenaBase - ap->tabBase;
	partBlockSize(part, ap->blockSize);
	ap->size = ap->part->size & ~(u64int)(ap->blockSize - 1);

	if(!readArenaMap(&amn, part, ap->tabBase, ap->tabSize)){
		freeArenaPart(ap, 0);
		return nil;
	}
	ap->narenas = amn.n;
	ap->map = amn.map;
	if(!okAMap(ap->map, ap->narenas, ap->arenaBase, ap->size, "arena table")){
		freeArenaPart(ap, 0);
		return nil;
	}

	ap->arenas = MKNZ(Arena*, ap->narenas);
	for(i = 0; i < ap->narenas; i++){
		ap->arenas[i] = initArena(part, ap->map[i].start, ap->map[i].stop - ap->map[i].start, ap->blockSize);
		if(ap->arenas[i] == nil){
			freeArenaPart(ap, 1);
			return nil;
		}
		if(!nameEq(ap->map[i].name, ap->arenas[i]->name)){
			setErr(ECorrupt, "arena name mismatches with expected name: %s vs. %s",
				ap->map[i].name, ap->arenas[i]->name);
			freeArenaPart(ap, 1);
			return nil;
		}
		if(findArena(ap->arenas[i]->name)){
			setErr(ECorrupt, "duplicate arena name %s in %s",
				ap->map[i].name, ap->part->name);
			freeArenaPart(ap, 1);
			return nil;
		}
	}

	for(i = 0; i < ap->narenas; i++)
		addArena(ap->arenas[i]);

	return ap;
}

ArenaPart*
newArenaPart(Part *part, u32int blockSize, u32int tabSize)
{
	ArenaPart *ap;

	if(blockSize & (blockSize - 1)){
		setErr(ECorrupt, "illegal non-power-of-2 block size %d\n", blockSize);
		return nil;
	}
	ap = MKZ(ArenaPart);
	if(ap == nil)
		return nil;

	ap->version = ArenaPartVersion;
	ap->part = part;
	ap->blockSize = blockSize;
	partBlockSize(part, blockSize);
	ap->size = part->size & ~(u64int)(blockSize - 1);
	ap->tabBase = (PartBlank + HeadSize + blockSize - 1) & ~(blockSize - 1);
	ap->arenaBase = (ap->tabBase + tabSize + blockSize - 1) & ~(blockSize - 1);
	ap->tabSize = ap->arenaBase - ap->tabBase;
	ap->narenas = 0;

	if(!wbArenaPart(ap)){
		freeArenaPart(ap, 0);
		return nil;
	}

	return ap;
}

int
wbArenaPart(ArenaPart *ap)
{
	ZBlock *b;

	if(!okAMap(ap->map, ap->narenas, ap->arenaBase, ap->size, "arena table"))
		return 0;
	b = allocZBlock(HeadSize, 1);
	if(b == nil)
//ZZZ set error message?
		return 0;

	if(!packArenaPart(ap, b->data)){
		setErr(ECorrupt, "can't make arena partition header: %R");
		freeZBlock(b);
		return 0;
	}
	if(!writePart(ap->part, PartBlank, b->data, HeadSize)){
		setErr(EAdmin, "can't write arena partition header: %R");
		freeZBlock(b);
		return 0;
	}
	freeZBlock(b);

	return wbArenaMap(ap->map, ap->narenas, ap->part, ap->tabBase, ap->tabSize);
}

void
freeArenaPart(ArenaPart *ap, int freeArenas)
{
	int i;

	if(ap == nil)
		return;
	if(freeArenas){
		for(i = 0; i < ap->narenas; i++){
			delArena(ap->arenas[i]);
			freeArena(ap->arenas[i]);
		}
	}
	free(ap->map);
	free(ap->arenas);
	free(ap);
}

int
okAMap(AMap *am, int n, u64int start, u64int stop, char *what)
{
	u64int last;
	u32int i;

	last = start;
	for(i = 0; i < n; i++){
		if(am[i].start < last){
			if(i == 0)
				setErr(ECorrupt, "invalid start address in %s", what);
			else
				setErr(ECorrupt, "overlapping ranges in %s", what);
			return 0;
		}
		if(am[i].stop < am[i].start){
			setErr(ECorrupt, "invalid range in %s", what);
			return 0;
		}
		last = am[i].stop;
	}
	if(last > stop){
		setErr(ECorrupt, "invalid ending address in %s", what);
		return 0;
	}
	return 1;
}

int
mapArenas(AMap *am, Arena **arenas, int n, char *what)
{
	u32int i;

	for(i = 0; i < n; i++){
		arenas[i] = findArena(am[i].name);
		if(arenas[i] == nil){
			setErr(EAdmin, "can't find arena '%s' for '%s'\n", am[i].name, what);
			return 0;
		}
	}
	return 1;
}

int
readArenaMap(AMapN *amn, Part *part, u64int base, u32int size)
{
	IFile f;
	u32int ok;

	if(!partIFile(&f, part, base, size))
		return 0;
	ok = parseAMap(&f, amn);
	freeIFile(&f);
	return ok;
}

int
wbArenaMap(AMap *am, int n, Part *part, u64int base, u64int size)
{
	Fmt f;
	ZBlock *b;

	b = allocZBlock(size, 1);
	if(b == nil)
		return 0;

	fmtZBInit(&f, b);

	if(!outputAMap(&f, am, n)){
		setErr(ECorrupt, "arena set size too small");
		freeZBlock(b);
		return 0;
	}
	if(!writePart(part, base, b->data, size)){
		setErr(EAdmin, "can't write arena set: %R");
		freeZBlock(b);
		return 0;
	}
	freeZBlock(b);
	return 1;
}

/*
 * amap: n '\n' amapelem * n
 * n: u32int
 * amapelem: name '\t' astart '\t' asize '\n'
 * astart, asize: u64int
 */
int
parseAMap(IFile *f, AMapN *amn)
{
	AMap *am;
	u64int v64;
	u32int v;
	char *s, *flds[4];
	int i, n;

	/*
	 * arenas
	 */
	if(!ifileU32Int(f, &v)){
		setErr(ECorrupt, "syntax error: bad number of elements in %s", f->name);
		return 0;
	}
	n = v;
	if(n > MaxAMap){
		setErr(ECorrupt, "illegal number of elements in %s", f->name);
		return 0;
	}
	am = MKNZ(AMap, n);
	if(am == nil)
		return 0;
	for(i = 0; i < n; i++){
		s = ifileLine(f);
		if(s == nil || getfields(s, flds, 4, 0, "\t") != 3)
			return 0;
		if(!nameOk(flds[0]))
			return 0;
		nameCp(am[i].name, flds[0]);
		if(!strU64Int(flds[1], &v64)){
			setErr(ECorrupt, "syntax error: bad arena base address in %s", f->name);
			free(am);
			return 0;
		}
		am[i].start = v64;
		if(!strU64Int(flds[2], &v64)){
			setErr(ECorrupt, "syntax error: bad arena size in %s", f->name);
			free(am);
			return 0;
		}
		am[i].stop = v64;
	}

	amn->map = am;
	amn->n = n;
	return 1;
}

int
outputAMap(Fmt *f, AMap *am, int n)
{
	int i;

	if(fmtprint(f, "%ud\n", n) < 0)
		return 0;
	for(i = 0; i < n; i++)
		if(fmtprint(f, "%s\t%llud\t%llud\n", am[i].name, am[i].start, am[i].stop) < 0)
			return 0;
	return 1;
}
