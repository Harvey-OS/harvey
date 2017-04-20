/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include <draw.h>
#include <event.h>

/* --- tree.h */
typedef struct Tree Tree;
typedef struct Tnode Tnode;

struct Tree
{
	Tnode *root;
	Point offset;
	Image *clipr;
};

struct Tnode
{
	Point offset;

	char *str;
//	char *(*strfn)(Tnode*);
//	uint (*draw)(Tnode*, Image*, Image*, Point);
	void (*expand)(Tnode*);
	void (*collapse)(Tnode*);

	uint expanded;
	Tnode **kid;
	int nkid;
	void *aux;
};

typedef struct Atree Atree;
struct Atree
{
	int resizefd;
	Tnode *root;
};

Atree *atreeinit(char*);

/* --- visfossil.c */
Tnode *initxheader(void);
Tnode *initxcache(char *name);
Tnode *initxsuper(void);
Tnode *initxlocalroot(char *name, uint32_t addr);
Tnode *initxentry(Entry);
Tnode *initxsource(Entry, int);
Tnode *initxentryblock(Block*, Entry*);
Tnode *initxdatablock(Block*, uint);
Tnode *initxroot(char *name, uint8_t[VtScoreSize]);

int fd;
Header h;
Super super;
VtSession *z;
VtRoot vac;
int showinactive;

/*
 * dumbed down versions of fossil routines
 */
char*
bsStr(int state)
{
	static char s[100];

	if(state == BsFree)
		return "Free";
	if(state == BsBad)
		return "Bad";

	sprint(s, "%x", state);
	if(!(state&BsAlloc))
		strcat(s, ",Free");	/* should not happen */
	if(state&BsVenti)
		strcat(s, ",Venti");
	if(state&BsClosed)
		strcat(s, ",Closed");
	return s;
}

char *bttab[] = {
	"BtData",
	"BtData+1",
	"BtData+2",
	"BtData+3",
	"BtData+4",
	"BtData+5",
	"BtData+6",
	"BtData+7",
	"BtDir",
	"BtDir+1",
	"BtDir+2",
	"BtDir+3",
	"BtDir+4",
	"BtDir+5",
	"BtDir+6",
	"BtDir+7",
};

char*
btStr(int type)
{
	if(type < nelem(bttab))
		return bttab[type];
	return "unknown";
}
//#pragma varargck argpos stringnode 1

Block*
allocBlock(void)
{
	Block *b;

	b = mallocz(sizeof(Block)+h.blockSize, 1);
	b->data = (void*)&b[1];
	return b;
}

void
blockPut(Block *b)
{
	free(b);
}

static uint32_t
partStart(int part)
{
	switch(part){
	default:
		assert(0);
	case PartSuper:
		return h.super;
	case PartLabel:
		return h.label;
	case PartData:
		return h.data;
	}
}


static uint32_t
partEnd(int part)
{
	switch(part){
	default:
		assert(0);
	case PartSuper:
		return h.super+1;
	case PartLabel:
		return h.data;
	case PartData:
		return h.end;
	}
}

Block*
readBlock(int part, uint32_t addr)
{
	uint32_t start, end;
	uint64_t offset;
	int n, nn;
	Block *b;
	uint8_t *buf;

	start = partStart(part);
	end = partEnd(part);
	if(addr >= end-start){
		werrstr("bad addr 0x%.8x; wanted 0x%.8x - 0x%.8x", addr, start, end);
		return nil;
	}

	b = allocBlock();
	b->addr = addr;
	buf = b->data;
	offset = ((uint64_t)(addr+start))*h.blockSize;
	n = h.blockSize;
	while(n > 0){
		nn = pread(fd, buf, n, offset);
		if(nn < 0){
			blockPut(b);
			return nil;
		}
		if(nn == 0){
			werrstr("short read");
			blockPut(b);
			return nil;
		}
		n -= nn;
		offset += nn;
		buf += nn;
	}
	return b;
}

int vtType[BtMax] = {
	VtDataType,		/* BtData | 0  */
	VtPointerType0,		/* BtData | 1  */
	VtPointerType1,		/* BtData | 2  */
	VtPointerType2,		/* BtData | 3  */
	VtPointerType3,		/* BtData | 4  */
	VtPointerType4,		/* BtData | 5  */
	VtPointerType5,		/* BtData | 6  */
	VtPointerType6,		/* BtData | 7  */
	VtDirType,		/* BtDir | 0  */
	VtPointerType0,		/* BtDir | 1  */
	VtPointerType1,		/* BtDir | 2  */
	VtPointerType2,		/* BtDir | 3  */
	VtPointerType3,		/* BtDir | 4  */
	VtPointerType4,		/* BtDir | 5  */
	VtPointerType5,		/* BtDir | 6  */
	VtPointerType6,		/* BtDir | 7  */
};

Block*
ventiBlock(uint8_t score[VtScoreSize], uint type)
{
	int n;
	Block *b;

	b = allocBlock();
	memmove(b->score, score, VtScoreSize);
	b->addr = NilBlock;

	n = vtRead(z, b->score, vtType[type], b->data, h.blockSize);
	if(n < 0){
		fprint(2, "vtRead returns %d: %R\n", n);
		blockPut(b);
		return nil;
	}
	vtZeroExtend(vtType[type], b->data, n, h.blockSize);
	b->l.type = type;
	b->l.state = 0;
	b->l.tag = 0;
	b->l.epoch = 0;
	return b;
}

Block*
dataBlock(uint8_t score[VtScoreSize], uint type, uint tag)
{
	Block *b, *bl;
	int lpb;
	Label l;
	uint32_t addr;

	addr = globalToLocal(score);
	if(addr == NilBlock)
		return ventiBlock(score, type);

	lpb = h.blockSize/LabelSize;
	bl = readBlock(PartLabel, addr/lpb);
	if(bl == nil)
		return nil;
	if(!labelUnpack(&l, bl->data, addr%lpb)){
		werrstr("%R");
		blockPut(bl);
		return nil;
	}
	blockPut(bl);
	if(l.type != type){
		werrstr("type mismatch; got %d (%s) wanted %d (%s)",
			l.type, btStr(l.type), type, btStr(type));
		return nil;
	}
	if(tag && l.tag != tag){
		werrstr("tag mismatch; got 0x%.8x wanted 0x%.8x",
			l.tag, tag);
		return nil;
	}
	b = readBlock(PartData, addr);
	if(b == nil)
		return nil;
	b->l = l;
	return b;
}

Entry*
copyEntry(Entry e)
{
	Entry *p;

	p = mallocz(sizeof *p, 1);
	*p = e;
	return p;
}

MetaBlock*
copyMetaBlock(MetaBlock mb)
{
	MetaBlock *p;

	p = mallocz(sizeof mb, 1);
	*p = mb;
	return p;
}

/*
 * visualizer 
 */

//#pragma	varargck	argpos	stringnode	1

Tnode*
stringnode(char *fmt, ...)
{
	va_list arg;
	Tnode *t;

	t = mallocz(sizeof(Tnode), 1);
	va_start(arg, fmt);
	t->str = vsmprint(fmt, arg);
	va_end(arg);
	t->nkid = -1;
	return t;
}

void
xcacheexpand(Tnode *t)
{
	if(t->nkid >= 0)
		return;

	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*t->nkid, 1);
	t->kid[0] = initxheader();
}

Tnode*
initxcache(char *name)
{
	Tnode *t;

	if((fd = open(name, OREAD)) < 0)
		sysfatal("cannot open %s: %r", name);

	t = stringnode("%s", name);
	t->expand = xcacheexpand;
	return t;
}

void
xheaderexpand(Tnode *t)
{
	if(t->nkid >= 0)
		return;

	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*t->nkid, 1);
	t->kid[0] = initxsuper();
	//t->kid[1] = initxlabel(h.label);
	//t->kid[2] = initxdata(h.data);
}

Tnode*
initxheader(void)
{
	uint8_t buf[HeaderSize];
	Tnode *t;

	if(pread(fd, buf, HeaderSize, HeaderOffset) < HeaderSize)
		return stringnode("error reading header: %r");
	if(!headerUnpack(&h, buf))
		return stringnode("error unpacking header: %R");

	t = stringnode("header "
		"version=%#x (%d) "
		"blockSize=%#x (%d) "
		"super=%#lx (%ld) "
		"label=%#lx (%ld) "
		"data=%#lx (%ld) "
		"end=%#lx (%ld)",
		h.version, h.version, h.blockSize, h.blockSize,
		h.super, h.super,
		h.label, h.label, h.data, h.data, h.end, h.end);
	t->expand = xheaderexpand;
	return t;
}

void
xsuperexpand(Tnode *t)
{
	if(t->nkid >= 0)
		return;

	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*t->nkid, 1);
	t->kid[0] = initxlocalroot("active", super.active);
//	t->kid[1] = initxlocalroot("next", super.next);
//	t->kid[2] = initxlocalroot("current", super.current);
}

Tnode*
initxsuper(void)
{
	Block *b;
	Tnode *t;

	b = readBlock(PartSuper, 0);
	if(b == nil)
		return stringnode("reading super: %r");
	if(!superUnpack(&super, b->data)){
		blockPut(b);
		return stringnode("unpacking super: %R");
	}
	blockPut(b);
	t = stringnode("super "
		"version=%#x "
		"epoch=[%#x,%#x) "
		"qid=%#llx "
		"active=%#x "
		"next=%#x "
		"current=%#x "
		"last=%V "
		"name=%s",
		super.version, super.epochLow, super.epochHigh,
		super.qid, super.active, super.next, super.current,
		super.last, super.name);
	t->expand = xsuperexpand;
	return t;
}

void
xvacrootexpand(Tnode *t)
{
	if(t->nkid >= 0)
		return;

	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*t->nkid, 1);
	t->kid[0] = initxroot("root", vac.score);
}

Tnode*
initxvacroot(uint8_t score[VtScoreSize])
{
	Tnode *t;
	uint8_t buf[VtRootSize];
	int n;

	if((n = vtRead(z, score, VtRootType, buf, VtRootSize)) < 0)
		return stringnode("reading root %V: %R", score);

	if(!vtRootUnpack(&vac, buf))
		return stringnode("unpack %d-byte root: %R", n);

	h.blockSize = vac.blockSize;
	t = stringnode("vac version=%#x name=%s type=%s blockSize=%u score=%V prev=%V",
		vac.version, vac.name, vac.type, vac.blockSize, vac.score, vac.prev);
	t->expand = xvacrootexpand;
	return t;
}

Tnode*
initxlabel(Label l)
{
	return stringnode("label type=%s state=%s epoch=%#x tag=%#x",
		btStr(l.type), bsStr(l.state), l.epoch, l.tag);
}

typedef struct Xblock Xblock;
struct Xblock
{
	Tnode;
	Block *b;
	int (*gen)(void*, Block*, int, Tnode**);
	void *arg;
	int printlabel;
};

void
xblockexpand(Tnode *tt)
{
	int i, j;
	enum { Q = 32 };
	Xblock *t = (Xblock*)tt;
	Tnode *nn;

	if(t->nkid >= 0)
		return;

	j = 0;
	if(t->printlabel){
		t->kid = mallocz(Q*sizeof(t->kid[0]), 1);
		t->kid[0] = initxlabel(t->b->l);
		j = 1;
	}

	for(i=0;; i++){
		switch((*t->gen)(t->arg, t->b, i, &nn)){
		case -1:
			t->nkid = j;
			return;
		case 0:
			break;
		case 1:
			if(j%Q == 0)
				t->kid = realloc(t->kid, (j+Q)*sizeof(t->kid[0]));
			t->kid[j++] = nn;
			break;
		}
	}
}

int
nilgen(void*, Block*, int, Tnode**)
{
	return -1;
}

Tnode*
initxblock(Block *b, char *s, int (*gen)(void*, Block*, int, Tnode**),
	   void *arg)
{
	Xblock *t;

	if(gen == nil)
		gen = nilgen;
	t = mallocz(sizeof(Xblock), 1);
	t->b = b;
	t->gen = gen;
	t->arg = arg;
	if(b->addr == NilBlock)
		t->str = smprint("Block %V: %s", b->score, s);
	else
		t->str = smprint("Block %#x: %s", b->addr, s);
	t->printlabel = 1;
	t->nkid = -1;
	t->expand = xblockexpand;
	return t;
}

int
xentrygen(void *v, Block *b, int o, Tnode **tp)
{
	Entry e;
	Entry *ed;

	ed = v;
	if(o >= ed->dsize/VtEntrySize)
		return -1;

	entryUnpack(&e, b->data, o);
	if(!showinactive && !(e.flags & VtEntryActive))
		return 0;
	*tp = initxentry(e);
	return 1;
}

Tnode*
initxentryblock(Block *b, Entry *ed)
{
	return initxblock(b, "entry", xentrygen, ed);
}

typedef struct Xentry Xentry;
struct Xentry 
{
	Tnode;
	Entry e;
};

void
xentryexpand(Tnode *tt)
{
	Xentry *t = (Xentry*)tt;

	if(t->nkid >= 0)
		return;

	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*t->nkid, 1);
	t->kid[0] = initxsource(t->e, 1);
}

Tnode*
initxentry(Entry e)
{
	Xentry *t;

	t = mallocz(sizeof *t, 1);
	t->nkid = -1;
	t->str = smprint("Entry gen=%#x psize=%d dsize=%d depth=%d flags=%#x size=%lld score=%V",
		e.gen, e.psize, e.dsize, e.depth, e.flags, e.size, e.score);
	if(e.flags & VtEntryLocal)
		t->str = smprint("%s archive=%d snap=%d tag=%#x", t->str, e.archive, e.snap, e.tag);
	t->expand = xentryexpand;
	t->e = e;
	return t;	
}

int
ptrgen(void *v, Block *b, int o, Tnode **tp)
{
	Entry *ed;
	Entry e;

	ed = v;
	if(o >= ed->psize/VtScoreSize)
		return -1;

	e = *ed;
	e.depth--;
	memmove(e.score, b->data+o*VtScoreSize, VtScoreSize);
	if(memcmp(e.score, vtZeroScore, VtScoreSize) == 0)
		return 0;
	*tp = initxsource(e, 0);
	return 1;
}

static int
etype(int flags, int depth)
{
	uint t;

	if(flags&VtEntryDir)
		t = BtDir;
	else
		t = BtData;
	return t+depth;
}

Tnode*
initxsource(Entry e, int dowrap)
{
	Block *b;
	Tnode *t, *tt;

	b = dataBlock(e.score, etype(e.flags, e.depth), e.tag);
	if(b == nil)
		return stringnode("dataBlock: %r");

	if((e.flags & VtEntryActive) == 0)
		return stringnode("inactive Entry");

	if(e.depth == 0){
		if(e.flags & VtEntryDir)
			tt = initxentryblock(b, copyEntry(e));
		else
			tt = initxdatablock(b, e.dsize);
	}else{
		tt = initxblock(b, smprint("%s+%d pointer", (e.flags & VtEntryDir) ? "BtDir" : "BtData", e.depth),
			ptrgen, copyEntry(e));
	}

	/*
	 * wrap the contents of the Source in a Source node,
	 * just so it's closer to what you see in the code.
	 */
	if(dowrap){
		t = stringnode("Source");
		t->nkid = 1;
		t->kid = mallocz(sizeof(Tnode*)*1, 1);
		t->kid[0] = tt;
		tt = t;
	}
	return tt;
}

int
xlocalrootgen(void*, Block *b, int o, Tnode **tp)
{
	Entry e;

	if(o >= 1)
		return -1;
	entryUnpack(&e, b->data, o);
	*tp = initxentry(e);
	return 1;
}

Tnode*
initxlocalroot(char *name, uint32_t addr)
{
	uint8_t score[VtScoreSize];
	Block *b;

	localToGlobal(addr, score);
	b = dataBlock(score, BtDir, RootTag);
	if(b == nil)
		return stringnode("read data block %#x: %R", addr);
	return initxblock(b, smprint("'%s' fs root", name), xlocalrootgen, nil);
}

int
xvacrootgen(void*, Block *b, int o, Tnode **tp)
{
	Entry e;

	if(o >= 3)
		return -1;
	entryUnpack(&e, b->data, o);
	*tp = initxentry(e);
	return 1;
}

Tnode*
initxroot(char *name, uint8_t score[VtScoreSize])
{
	Block *b;

	b = dataBlock(score, BtDir, RootTag);
	if(b == nil)
		return stringnode("read data block %V: %R", score);
	return initxblock(b, smprint("'%s' fs root", name), xvacrootgen, nil);
}
Tnode*
initxdirentry(MetaEntry *me)
{
	DirEntry dir;
	Tnode *t;

	if(!deUnpack(&dir, me))
		return stringnode("deUnpack: %R");

	t = stringnode("dirEntry elem=%s size=%llu data=%#lx/%#lx meta=%#lx/%#lx", dir.elem, dir.size, dir.entry, dir.gen, dir.mentry, dir.mgen);
	t->nkid = 1;
	t->kid = mallocz(sizeof(t->kid[0])*1, 1);
	t->kid[0] = stringnode(
		"qid=%#llx\n"
		"uid=%s gid=%s mid=%s\n"
		"mtime=%lu mcount=%lu ctime=%lu atime=%lu\n"
		"mode=%lo\n"
		"plan9 %d p9path %#llx p9version %lu\n"
		"qidSpace %d offset %#llx max %#llx",
		dir.qid,
		dir.uid, dir.gid, dir.mid,
		dir.mtime, dir.mcount, dir.ctime, dir.atime,
		dir.mode,
		dir.plan9, dir.p9path, dir.p9version,
		dir.qidSpace, dir.qidOffset, dir.qidMax);
	return t;
}

int
metaentrygen(void *v, Block*, int o, Tnode **tp)
{
	Tnode *t;
	MetaBlock *mb;
	MetaEntry me;

	mb = v;
	if(o >= mb->nindex)
		return -1;
	meUnpack(&me, mb, o);

	t = stringnode("MetaEntry %d bytes", mb->size);
	t->kid = mallocz(sizeof(t->kid[0])*1, 1);
	t->kid[0] = initxdirentry(&me);
	t->nkid = 1;
	*tp = t;
	return 1;
}

int
metablockgen(void *v, Block *b, int o, Tnode **tp)
{
	Xblock *t;
	MetaBlock *mb;

	if(o >= 1)
		return -1;

	/* hack: reuse initxblock as a generic iterator */
	mb = v;
	t = (Xblock*)initxblock(b, "", metaentrygen, mb);
	t->str = smprint("MetaBlock %d/%d space used, %d add'l free %d/%d table used%s",
		mb->size, mb->maxsize, mb->free, mb->nindex, mb->maxindex,
		mb->botch ? " [BOTCH]" : "");
	t->printlabel = 0;
	*tp = t;
	return 1;
}

/*
 * attempt to guess at the type of data in the block.
 * it could just be data from a file, but we're hoping it's MetaBlocks.
 */
Tnode*
initxdatablock(Block *b, uint n)
{
	MetaBlock mb;

	if(n > h.blockSize)
		n = h.blockSize;

	if(mbUnpack(&mb, b->data, n))
		return initxblock(b, "metadata", metablockgen, copyMetaBlock(mb));

	return initxblock(b, "data", nil, nil);
}

int
parseScore(uint8_t *score, char *buf, int n)
{
	int i, c;

	memset(score, 0, VtScoreSize);

	if(n < VtScoreSize*2)
		return 0;
	for(i=0; i<VtScoreSize*2; i++){
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else{
			return 0;
		}

		if((i & 1) == 0)
			c <<= 4;
	
		score[i>>1] |= c;
	}
	return 1;
}

int
scoreFmt(Fmt *f)
{
	uint8_t *v;
	int i;
	uint32_t addr;

	v = va_arg(f->args, uint8_t*);
	if(v == nil){
		fmtprint(f, "*");
	}else if((addr = globalToLocal(v)) != NilBlock)
		fmtprint(f, "0x%.8x", addr);
	else{
		for(i = 0; i < VtScoreSize; i++)
			fmtprint(f, "%2.2x", v[i]);
	}

	return 0;
}

Atree*
atreeinit(char *arg)
{
	Atree *a;
	uint8_t score[VtScoreSize];

	vtAttach();

	fmtinstall('V', scoreFmt);
	fmtinstall('R', vtErrFmt);

	z = vtDial(nil, 1);
	if(z == nil)
		fprint(2, "warning: cannot dial venti: %R\n");
	if(!vtConnect(z, 0)){
		fprint(2, "warning: cannot connect to venti: %R\n");
		z = nil;
	}
	a = mallocz(sizeof(Atree), 1);
	if(strncmp(arg, "vac:", 4) == 0){
		if(!parseScore(score, arg+4, strlen(arg+4))){
			fprint(2, "cannot parse score\n");
			return nil;
		}
		a->root = initxvacroot(score);
	}else
		a->root = initxcache(arg);
	a->resizefd = -1;
	return a;
}

/* --- tree.c */
enum
{
	Nubwidth = 11,
	Nubheight = 11,
	Linewidth = Nubwidth*2+4,
};

uint
drawtext(char *s, Image *m, Image *clipr, Point o)
{
	char *t, *nt, *e;
	uint dy;

	if(s == nil)
		s = "???";

	dy = 0;
	for(t=s; t&&*t; t=nt){
		if(nt = strchr(t, '\n')){
			e = nt;
			nt++;
		}else
			e = t+strlen(t);

		_string(m, Pt(o.x, o.y+dy), display->black, ZP, display->defaultfont,
			t, nil, e-t, clipr->clipr, nil, ZP, SoverD);
		dy += display->defaultfont->height;
	}
	return dy;
}

void
drawnub(Image *m, Image *clipr, Point o, Tnode *t)
{
	clipr = nil;

	if(t->nkid == 0)
		return;
	if(t->nkid == -1 && t->expand == nil)
		return;

	o.y += (display->defaultfont->height-Nubheight)/2;
	draw(m, rectaddpt(Rect(0,0,1,Nubheight), o), display->black, clipr, ZP);
	draw(m, rectaddpt(Rect(0,0,Nubwidth,1), o), display->black, clipr, o);
	draw(m, rectaddpt(Rect(Nubwidth-1,0,Nubwidth,Nubheight), o), 
		display->black, clipr, addpt(o, Pt(Nubwidth-1, 0)));
	draw(m, rectaddpt(Rect(0, Nubheight-1, Nubwidth, Nubheight), o),
		display->black, clipr, addpt(o, Pt(0, Nubheight-1)));

	draw(m, rectaddpt(Rect(0, Nubheight/2, Nubwidth, Nubheight/2+1), o),
		display->black, clipr, addpt(o, Pt(0, Nubheight/2)));
	if(!t->expanded)
		draw(m, rectaddpt(Rect(Nubwidth/2, 0, Nubwidth/2+1, Nubheight), o),
			display->black, clipr, addpt(o, Pt(Nubwidth/2, 0)));

}

uint
drawnode(Tnode *t, Image *m, Image *clipr, Point o)
{
	int i;
	char *fs, *s;
	uint dy;
	Point oo;

	if(t == nil)
		return 0;

	t->offset = o;

	oo = Pt(o.x+Nubwidth+2, o.y);
//	if(t->draw)
//		dy = (*t->draw)(t, m, clipr, oo);
//	else{
		fs = nil;
		if(t->str)
			s = t->str;
	//	else if(t->strfn)
	//		fs = s = (*t->strfn)(t);
		else
			s = "???";
		dy = drawtext(s, m, clipr, oo);
		free(fs);
//	}

	if(t->expanded){
		if(t->nkid == -1 && t->expand)
			(*t->expand)(t);
		oo = Pt(o.x+Nubwidth+(Linewidth-Nubwidth)/2, o.y+dy);
		for(i=0; i<t->nkid; i++)
			oo.y += drawnode(t->kid[i], m, clipr, oo);
		dy = oo.y - o.y;
	}
	drawnub(m, clipr, o, t);
	return dy;
}

void
drawtree(Tree *t, Image *m, Rectangle r)
{
	Point p;

	draw(m, r, display->white, nil, ZP);

	replclipr(t->clipr, 1, r);
	p = addpt(t->offset, r.min);
	drawnode(t->root, m, t->clipr, p);
}

Tnode*
findnode(Tnode *t, Point p)
{
	int i;
	Tnode *tt;

	if(ptinrect(p, rectaddpt(Rect(0,0,Nubwidth, Nubheight), t->offset)))
		return t;
	if(!t->expanded)
		return nil;
	for(i=0; i<t->nkid; i++)
		if(tt = findnode(t->kid[i], p))
			return tt;
	return nil;
}

void
usage(void)
{
	fprint(2, "usage: vtree /dev/sdC0/fossil\n");
	exits("usage");
}

Tree t;

void
eresized(int new)
{
	Rectangle r;
	r = screen->r;
	if(new && getwindow(display, Refnone) < 0)
		fprint(2,"can't reattach to window");
	drawtree(&t, screen, screen->r);
}

enum
{
	Left = 1<<0,
	Middle = 1<<1,
	Right = 1<<2,

	MMenu = 2,
};

char *items[] = { "exit", 0 };
enum { IExit, };

Menu menu;

void
main(int argc, char **argv)
{
	int n;
	char *dir;
	Event e;
	Point op, p;
	Tnode *tn;
	Mouse m;
	int Eready;
	Atree *fs;

	ARGBEGIN{
	case 'a':
		showinactive = 1;
		break;
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 1:
		dir = argv[0];
		break;
	}

	fs = atreeinit(dir);
	initdraw(0, "/lib/font/bit/lucidasans/unicode.8.font", "tree");
	t.root = fs->root;
	t.offset = ZP;
	t.clipr = allocimage(display, Rect(0,0,1,1), GREY1, 1, DOpaque);

	eresized(0);
	flushimage(display, 1);

	einit(Emouse);

	menu.item = items;
	menu.gen = 0;
	menu.lasthit = 0;
	if(fs->resizefd > 0){
		Eready = 1<<3;
		estart(Eready, fs->resizefd, 1);
	}else
		Eready = 0;

	for(;;){
		switch(n=eread(Emouse|Eready, &e)){
		default:
			if(Eready && n==Eready)
				eresized(0);
			break;
		case Emouse:
			m = e.mouse;
			switch(m.buttons){
			case Left:
				op = t.offset;
				p = m.xy;
				do {
					t.offset = addpt(t.offset, subpt(m.xy, p));
					p = m.xy;
					eresized(0);
					m = emouse();
				}while(m.buttons == Left);
				if(m.buttons){
					t.offset = op;
					eresized(0);
				}
				break;
			case Middle:
				n = emenuhit(MMenu, &m, &menu);
				if(n == -1)
					break;
				switch(n){
				case IExit:
					exits(nil);
				}
				break;
			case Right:
				do
					m = emouse();
				while(m.buttons == Right);
				if(m.buttons)
					break;
				tn = findnode(t.root, m.xy);
				if(tn){
					tn->expanded = !tn->expanded;
					eresized(0);
				}
				break;
			}
		}
	}
}
