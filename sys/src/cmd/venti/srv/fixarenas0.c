/*
 * Check and fix an arena partition.
 *
 * This is a lot grittier than the rest of Venti because
 * it can't just give up if a byte here or there is wrong.
 *
 * The goal here (hopefully met!) is that block corruption
 * only ever has a local effect -- there are no blocks that
 * you can wipe out that will cause large portions of 
 * uncorrupted data blocks to be useless.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "whack.h"
#pragma varargck type "z" uvlong
#pragma varargck type "z" vlong
#pragma varargck type "t" uint

enum
{
	K = 1024,
	M = 1024*1024,
	G = 1024*1024*1024,
	
	Block = 4096,
};

int verbose;
Part *part;
char *file;
char *basename;
int fix;
int badreads;
uchar zero[MaxDiskBlock];

Arena lastarena;
ArenaPart ap;
uvlong arenasize;
int nbadread;
int nbad;
void checkarena(vlong, int);

void
usage(void)
{
	fprint(2, "usage: fixarenas [-fv] [-a arenasize] [-b blocksize] file [ranges]\n");
	threadexitsall(0);
}

/*
 * Format number in simplest way that is okay with unittoull.
 */
static int
zfmt(Fmt *fmt)
{
	vlong x;
	
	x = va_arg(fmt->args, vlong);
	if(x == 0)
		return fmtstrcpy(fmt, "0");
	if(x%G == 0)
		return fmtprint(fmt, "%lldG", x/G);
	if(x%M == 0)
		return fmtprint(fmt, "%lldM", x/M);
	if(x%K == 0)
		return fmtprint(fmt, "%lldK", x/K);
	return fmtprint(fmt, "%lld", x);
}

/*
 * Format time like ctime without newline.
 */
static int
tfmt(Fmt *fmt)
{
	uint t;
	char buf[30];
	
	t = va_arg(fmt->args, uint);
	strcpy(buf, ctime(t));
	buf[28] = 0;
	return fmtstrcpy(fmt, buf);
}

/*
 * Coalesce messages about unreadable sectors into larger ranges.
 * bad(0, 0) flushes the buffer.
 */
static void
bad(char *msg, vlong o, int len)
{
	static vlong lb0, lb1;
	static char *lmsg;

	if(msg == nil)
		msg = lmsg;
	if(o == -1){
		lmsg = nil;
		lb0 = 0;
		lb1 = 0;
		return;
	}
	if(lb1 != o || (msg && lmsg && strcmp(msg, lmsg) != 0)){
		if(lb0 != lb1)
			fprint(2, "%s %#llux+%#llux (%,lld+%,lld)\n",
				lmsg, lb0, lb1-lb0, lb0, lb1-lb0);
		lb0 = o;
	}
	lmsg = msg;
	lb1 = o+len;
}

/*
 * Read in the len bytes of data at the offset.  If can't for whatever reason,
 * fill it with garbage but print an error.
 */
static uchar*
readdisk(uchar *buf, vlong offset, int len)
{
	int i, j, k, n;

	if(offset >= part->size){
		memset(buf, 0xFB, sizeof buf);
		return buf;
	}
	
	if(offset+len > part->size){
		memset(buf, 0xFB, sizeof buf);
		len = part->size - offset;
	}

	if(readpart(part, offset, buf, len) >= 0)
		return buf;
	
	/*
	 * The read failed.  Clear the buffer to nonsense, and
	 * then try reading in smaller pieces.  If that fails,
	 * read in even smaller pieces.  And so on down to sectors.
	 */
	memset(buf, 0xFD, len);
	for(i=0; i<len; i+=64*K){
		n = 64*K;
		if(i+n > len)
			n = len-i;
		if(readpart(part, offset+i, buf+i, n) >= 0)
			continue;
		for(j=i; j<len && j<i+64*K; j+=4*K){
			n = 4*K;
			if(j+n > len)
				n = len-j;
			if(readpart(part, offset+j, buf+j, n) >= 0)
				continue;
			for(k=j; k<len && k<j+4*K; k+=512){
				if(readpart(part, offset+k, buf+k, 512) >= 0)
					continue;
				bad("disk read failed at", k, 512);
				badreads++;
			}
		}
	}
	bad(nil, 0, 0);
	return buf;
}

/*
 * Buffers to support running SHA1 hashes of the
 * pre-edit and post-edit disk.
 */
typedef struct Shabuf Shabuf;
struct Shabuf
{
	vlong offset0;
	vlong preoffset;
	vlong postoffset;
	DigestState pre;
	DigestState snap[10000];	/* enough for 10G arena! */
	DigestState post;
	int nds;
};

Shabuf shabuf;

void
sbrollback(Shabuf *sb, vlong offset)
{
	int x;
	vlong o;
	
	/* roll back to M boundary before or at offset */
	o = offset - sb->offset0;
	o -= o%M;
	x = o/M;
	if((vlong)x*M != o){
		print("bad math in shaupdate1\n");
		sb->offset0 = 0;
		return;
	}
	if(x >= nelem(sb->snap)){
		print("arena way too big: >10G ???; no sha1\n");
		sb->offset0 = 0;
		return;
	}
	sb->post = sb->snap[x];
	sb->postoffset = sb->offset0 + o;
	assert(sb->postoffset <= offset);
	assert((sb->postoffset - sb->offset0)%M == 0);
}

void
sbposthash(Shabuf *sb, uchar *p, int len)
{
	int o, n, x;
	
	o = (sb->postoffset - sb->offset0)%M;
	for(; len > 0; p += n, len -= n){
		n = M-o;
		o = 0;
		if(n > len)
			n = len;
		sha1(p, n, nil, &sb->post);
		sb->postoffset += n;
		if((sb->postoffset - sb->offset0)%M == 0){
			/* (test may not be true in last iteration) */
			x = (sb->postoffset - sb->offset0)/M;
			assert(x < nelem(sb->snap));
			sb->snap[x] = sb->post;
		}
	}
}

void
sbrollforward(Shabuf *sb, vlong offset)
{
	int len;
	static uchar xbuf[M];
	
	assert((sb->postoffset - sb->offset0)%M == 0);
	while(sb->postoffset < offset){
		readdisk(xbuf, sb->postoffset, M);
		len = M;
		if(sb->postoffset+len > offset)
			len = offset - sb->postoffset;
		sbposthash(sb, xbuf, len);
	}
	assert(sb->postoffset == offset);
}

void
shaupdate1(Shabuf *sb, uchar *p, vlong offset, int len)
{
	int x;

	if(sb->offset0 == 0)
		return;
	assert(sb->preoffset >= offset);
	
	/*
	 * Update pre-edit digest.
	 */
	if(sb->preoffset < offset+len){
		x = sb->preoffset - offset;
		sha1(p+x, len-x, nil, &sb->pre);
		sb->preoffset += len-x;
		assert(sb->preoffset == offset+len);
	}
	
	/*
	 * Update post-edit digest.
	 */
	if(offset < sb->postoffset)
		sbrollback(sb, offset);
	if(offset > sb->postoffset){
		sbrollback(sb, sb->postoffset);
		sbrollforward(sb, offset);
	}
	assert(offset == sb->postoffset);
	sbposthash(sb, p, len);
}
		
void
shafromdisk(Shabuf *sb, vlong offset)
{
	vlong o;
	int off, n;
	static uchar xbuf[4*M];
	
	if(sb->offset0 == 0)
		return;

	off = sb->preoffset%M;
	o = sb->preoffset - off;
	for(; o < offset; o+=n){
		n = 4*M;
		if(o+n > offset)
			n = ((offset-o)+Block-1)&~(Block-1);
		readdisk(xbuf, o, n);
		shaupdate1(sb, xbuf+off, o+off, n-off);
		off = 0;
	}
	if(sb->offset0 == 0)	/* error happened */
		return;
	assert(sb->preoffset >= offset);
	assert(sb->postoffset >= offset);
}

void
shaupdate(Shabuf *sb, uchar *p, vlong offset, int len)
{	
	if(sb->offset0 == 0)	/* not started yet */
		return;

	if(sb->preoffset < offset)
		shafromdisk(sb, offset);

	if(sb->offset0 == 0)	/* error happened */
		return;
	assert(sb->preoffset >= offset);
	assert(sb->postoffset >= offset);
	
	shaupdate1(sb, p, offset, len);
	assert(sb->preoffset >= offset+len);
	assert(sb->postoffset >= offset+len);
}

/*
 * If we're fixing arenas, then editing this memory edits the disk!
 * It will be written back out as new data is paged in.  Also update
 * the sha buffer as we see data go by.
 */
uchar buf[4*M];
uchar sbuf[4*M];
vlong bufoffset;
int buflen;

static void pageout(void);
static uchar*
pagein(vlong offset, int len)
{
	pageout();
	if(offset >= part->size){
		memset(buf, 0xFB, sizeof buf);
		return buf;
	}
	
	if(offset+len > part->size){
		memset(buf, 0xFB, sizeof buf);
		len = part->size - offset;
	}
	bufoffset = offset;
	buflen = len;
	readdisk(buf, offset, len);
	memmove(sbuf, buf, len);
	shaupdate(&shabuf, buf, offset, len);
	return buf;
}

static void
pageout(void)
{
	shaupdate(&shabuf, buf, bufoffset, buflen);
	if(buflen==0 || !fix || memcmp(buf, sbuf, buflen) == 0){
		buflen = 0;
		return;
	}
	if(writepart(part, bufoffset, buf, buflen) < 0)
		fprint(2, "disk write failed at %#llux+%#ux (%,lld+%,d)\n",
			bufoffset, buflen, bufoffset, buflen);
	buflen = 0;
}

static void
zerorange(vlong offset, int len)
{
	int i;
	vlong ooff;
	int olen;
	enum { MinBlock = 4*K, MaxBlock = 8*K };
	
	if(0)
	if(bufoffset <= offset && offset+len <= bufoffset+buflen){
		memset(buf+(offset-bufoffset), 0, len);
		return;
	}
	
	ooff = bufoffset;
	olen = buflen;
	
	i = offset%MinBlock;
	if(i+len < MaxBlock){
		pagein(offset-i, (len+MinBlock-1)&~(MinBlock-1));
		memset(buf+i, 0, len);
	}else{
		pagein(offset-i, MaxBlock);
		memset(buf+i, 0, MaxBlock-i);
		offset += MaxBlock-i;
		len -= MaxBlock-i;
		while(len >= MaxBlock){
			pagein(offset, MaxBlock);
			memset(buf, 0, MaxBlock);
			offset += MaxBlock;
			len -= MaxBlock;
		}
		pagein(offset, (len+MinBlock-1)&~(MinBlock-1));
		memset(buf, 0, len);
	}
	pagein(ooff, olen);
}

/*
 * read/write integers
 */
static void
p16(uchar *p, u16int u)
{
	p[0] = (u>>8) & 0xFF;
	p[1] = u & 0xFF;
}

static u16int
u16(uchar *p)
{
	return (p[0]<<8)|p[1];
}

static void
p32(uchar *p, u32int u)
{
	p[0] = (u>>24) & 0xFF;
	p[1] = (u>>16) & 0xFF;
	p[2] = (u>>8) & 0xFF;
	p[3] = u & 0xFF;
}

static u32int
u32(uchar *p)
{
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}

static void
p64(uchar *p, u64int u)
{
	p32(p, u>>32);
	p32(p, u);
}

static u64int
u64(uchar *p)
{
	return ((u64int)u32(p)<<32) | u32(p+4);
}

static int
vlongcmp(const void *va, const void *vb)
{
	vlong a, b;
	
	a = *(vlong*)va;
	b = *(vlong*)vb;
	if(a < b)
		return -1;
	if(b > a)
		return 1;
	return 0;
}

/* D and S are in draw.h */
#define D VD
#define S VS

enum
{
	D = 0x10000,
	Z = 0x20000,
	S = 0x30000,
	T = 0x40000,
	N = 0xFFFF
};
typedef struct Info Info;
struct Info
{
	int len;
	char *name;
};

Info partinfo[] = {
	4,	"magic",
	D|4,	"version",
	Z|4,	"blocksize",
	4,	"arenabase",
	0
};

Info headinfo4[] = {
	4,	"magic",
	D|4,	"version",
	S|ANameSize,	"name",
	Z|4,	"blocksize",
	Z|8,	"size",
	0
};

Info headinfo5[] = {
	4,	"magic",
	D|4,	"version",
	S|ANameSize,	"name",
	Z|4,	"blocksize",
	Z|8,	"size",
	4,	"clumpmagic",
	0
};

Info tailinfo4[] = {
	4,	"magic",
	D|4,	"version",
	S|ANameSize,	"name",
	D|4,	"clumps",
	D|4,	"cclumps",
	T|4,	"ctime",
	T|4,	"wtime",
	D|8,	"used",
	D|8,	"uncsize",
	1,	"sealed",
	0
};
	
Info tailinfo5[] = {
	4,	"magic",
	D|4,	"version",
	S|ANameSize,	"name",
	D|4,	"clumps",
	D|4,	"cclumps",
	T|4,	"ctime",
	T|4,	"wtime",
	4,	"clumpmagic",
	D|8,	"used",
	D|8,	"uncsize",
	1,	"sealed",
	0
};

void
showdiffs(uchar *want, uchar *have, int len, Info *info)
{
	int n;
	
	while(len > 0 && (n=info->len&N) > 0){
		if(memcmp(have, want, n) != 0){
			switch(info->len){
			case 1:
				print("\t%s: correct=%d disk=%d\n",
					info->name, *want, *have);
				break;
			case 4:
				print("\t%s: correct=%#ux disk=%#ux\n",
					info->name, u32(want), u32(have));
				break;
			case D|4:
				print("\t%s: correct=%,ud disk=%,ud\n",
					info->name, u32(want), u32(have));
				break;
			case T|4:
				print("\t%s: correct=%t\n\t\tdisk=%t\n",
					info->name, u32(want), u32(have));
				break;
			case Z|4:
				print("\t%s: correct=%z disk=%z\n",
					info->name, (uvlong)u32(want), (uvlong)u32(have));
				break;
			case D|8:
				print("\t%s: correct=%,lld disk=%,lld\n",
					info->name, u64(want), u64(have));
				break;
			case Z|8:
				print("\t%s: correct=%z disk=%z\n",
					info->name, u64(want), u64(have));
				break;
			case S|ANameSize:
				print("\t%s: correct=%s disk=%.*s\n",
					info->name, (char*)want, 
					utfnlen((char*)have, ANameSize-1),
					(char*)have);
			default:
				print("\t%s: correct=%.*H disk=%.*H\n",
					info->name, n, want, n, have);
				break;
			}
		}
		have += n;
		want += n;
		len -= n;
		info++;
	}
	if(len > 0 && memcmp(have, want, len) != 0){
		if(memcmp(want, zero, len) != 0)
			print("!!\textra want data in showdiffs (bug in fixarenas)\n");
		else
			print("\tnon-zero data on disk after structure\n");
		if(verbose){
			print("want: %.*H\n", len, want);
			print("have: %.*H\n", len, have);
		}
	}
}

/*
 * Poke around on the disk to guess what the ArenaPart numbers are.
 */
void
guessgeometry(void)
{
	int i, j, n, bestn, ndiff, nhead, ntail;
	uchar *p, *ep, *sp;
	u64int diff[100], head[3], tail[3];
	u64int offset, bestdiff;
	
	ap.version = ArenaPartVersion;

	if(arenasize == 0 || ap.blocksize == 0){
		/*
		 * The ArenaPart block at offset PartBlank may be corrupt or just wrong.
		 * Instead, look for the individual arena headers and tails, which there
		 * are many of, and once we've seen enough, infer the spacing.
		 *
		 * Of course, nothing in the file format requires that arenas be evenly
		 * spaced, but fmtarenas always does that for us.
		 */
		nhead = 0;
		ntail = 0;
		for(offset=PartBlank; offset<part->size; offset+=4*M){
			p = pagein(offset, 4*M);
			for(sp=p, ep=p+4*M; p<ep; p+=K){
				if(u32(p) == ArenaHeadMagic && nhead < nelem(head)){
					if(verbose)
						fprint(2, "arena head at %#llx\n", offset+(p-sp));
					head[nhead++] = offset+(p-sp);
				}
				if(u32(p) == ArenaMagic && ntail < nelem(tail)){
					tail[ntail++] = offset+(p-sp);
					if(verbose)
						fprint(2, "arena tail at %#llx\n", offset+(p-sp));
				}
			}
			if(nhead == nelem(head) && ntail == nelem(tail))
				break;
		}
		if(nhead < 3 && ntail < 3)
			sysfatal("too few intact arenas: %d heads, %d tails", nhead, ntail);
	
		/* 
		 * Arena size is likely the most common
		 * inter-head or inter-tail spacing.
		 */
		ndiff = 0;
		for(i=1; i<nhead; i++)
			diff[ndiff++] = head[i] - head[i-1];
		for(i=1; i<ntail; i++)
			diff[ndiff++] = tail[i] - tail[i-1];
		qsort(diff, ndiff, sizeof diff[0], vlongcmp);
		bestn = 0;
		bestdiff = 0;
		for(i=1, n=1; i<=ndiff; i++, n++){
			if(i==ndiff || diff[i] != diff[i-1]){
				if(n > bestn){
					bestn = n;
					bestdiff = diff[i-1];
				}
				n = 0;
			}
		}
		fprint(2, "arena size likely %z (%d of %d)\n", bestdiff, bestn, ndiff);
		if(arenasize != 0 && arenasize != bestdiff)
			fprint(2, "using user-specified size %z instead\n", arenasize);
		else
			arenasize = bestdiff;

		/*
		 * The arena tail for an arena is arenasize-blocksize from the head.
		 */
		ndiff = 0;
		for(i=j=0; i<nhead && j<ntail; ){
			if(head[i] > tail[j]){
				j++;
				continue;
			}
			if(head[i]+arenasize > tail[j]){
				diff[ndiff++] = head[i]+arenasize - tail[j];
				continue;
			}
		}
		if(ndiff < 3)
			sysfatal("too few intact arenas: %d head, tail pairs", ndiff);
		qsort(diff, ndiff, sizeof diff[0], vlongcmp);
		bestn = 0;
		bestdiff = 0;
		for(i=1, n=1; i<=ndiff; i++, n++){
			if(i==ndiff || diff[i] != diff[i-1]){
				if(n > bestn){
					bestn = n;
					bestdiff = diff[i-1];
				}
				n = 0;
			}
		}
		fprint(2, "block size likely %z (%d of %d)\n", bestdiff, bestn, ndiff);
		if(ap.blocksize != 0 && ap.blocksize != bestdiff)
			fprint(2, "using user-specified size %z instead\n", (vlong)ap.blocksize);
		else
			ap.blocksize = bestdiff;
		if(ap.blocksize == 0 || ap.blocksize&(ap.blocksize-1))
			sysfatal("block size not a power of two");
		if(ap.blocksize > MaxDiskBlock)
			sysfatal("block size too big (max=%d)", MaxDiskBlock);
	}
	
	/* standard computation - fmtarenas always uses tabsize==512k */
	ap.arenabase = (PartBlank+HeadSize+512*K+ap.blocksize-1)&~(ap.blocksize-1);
	p = pagein(ap.arenabase, Block);
	fprint(2, "arena base likely %z%s\n", (vlong)ap.arenabase, u32(p)!=ArenaHeadMagic ? " (but no arena head there)" : "");
}

/*
 * Check the arena partition blocks and then the arenas listed in range.
 */
void
checkarenas(char *range)
{
	char *s, *t;
	int i, lo, hi, narena;
	uchar dbuf[HeadSize];
	uchar *p;

	guessgeometry();

	memset(dbuf, 0, sizeof dbuf);
	packarenapart(&ap, dbuf);
	p = pagein(PartBlank, Block);
	if(memcmp(p, dbuf, HeadSize) != 0){
		print("on-disk arena part superblock incorrect\n");
		showdiffs(dbuf, p, HeadSize, partinfo);
	}
	memmove(p, dbuf, HeadSize);

	narena = (part->size-ap.arenabase + arenasize-1)/arenasize;
	if(range == nil){
		for(i=0; i<narena; i++)
			checkarena(ap.arenabase+(vlong)i*arenasize, i);
	}else{
		/* parse, e.g., -4,8-9,10- */
		for(s=range; *s; s=t){
			t = strchr(s, ',');
			if(t)
				*t++ = 0;
			else
				t = s+strlen(s);
			if(*s == '-')
				lo = 0;
			else
				lo = strtol(s, &s, 0);
			hi = lo;
			if(*s == '-'){
				s++;
				if(*s == 0)
					hi = narena-1;
				else
					hi = strtol(s, &s, 0);
			}
			if(*s != 0){
				fprint(2, "bad range: %s\n", s);
				continue;
			}
			for(i=lo; i<=hi; i++)
				checkarena(ap.arenabase+(vlong)i*arenasize, i);
		}
	}
}

/*
 * Is there a clump here at p?
 */
static int
isclump(uchar *p, Clump *cl, u32int *pmagic)
{
	int n;
	u32int magic;
	uchar score[VtScoreSize], *bp;
	Unwhack uw;
	uchar ubuf[70*1024];
	
	bp = p;
	magic = u32(p);
	if(magic == 0)
		return 0;
	p += U32Size;

	cl->info.type = vtfromdisktype(*p);
	if(cl->info.type == 0xFF)
		return 0;
	p++;
	cl->info.size = u16(p);
	p += U16Size;
	cl->info.uncsize = u16(p);
	if(cl->info.size > cl->info.uncsize)
		return 0;
	p += U16Size;
	scorecp(cl->info.score, p);
	p += VtScoreSize;
	cl->encoding = *p;
	p++;
	cl->creator = u32(p);
	p += U32Size;
	cl->time = u32(p);
	p += U32Size;

	switch(cl->encoding){
	case ClumpENone:
		if(cl->info.size != cl->info.uncsize)
			return 0;
		scoremem(score, p, cl->info.size);
		if(scorecmp(score, cl->info.score) != 0)
			return 0;
		break;
	case ClumpECompress:
		if(cl->info.size >= cl->info.uncsize)
			return 0;
		unwhackinit(&uw);
		n = unwhack(&uw, ubuf, cl->info.uncsize, p, cl->info.size);
		if(n != cl->info.uncsize)
			return 0;
		scoremem(score, ubuf, cl->info.uncsize);
		if(scorecmp(score, cl->info.score) != 0)
			return 0;
		break;
	default:
		return 0;
	}
	p += cl->info.size;
	
	/* it all worked out in the end */
	*pmagic = magic;
	return p - bp;
}

/*
 * All ClumpInfos seen in this arena.
 * Kept in binary tree so we can look up by score.
 */
typedef struct Cit Cit;
struct Cit
{
	Cit *left;
	Cit *right;
	vlong corrupt;
	ClumpInfo ci;
};
Cit *cibuf;
Cit *ciroot;
int ncibuf, mcibuf;

void
resetcibuf(void)
{
	ncibuf = 0;
	ciroot = nil;
}

Cit**
ltreewalk(Cit **l, uchar *score)
{
	int i;
	
	for(;;){
		if(*l == nil)
			return l;
		i = scorecmp((*l)->ci.score, score);
		if(i < 0)
			l = &(*l)->right;
		else 
			l = &(*l)->left;
	}
	return nil; 	/* stupid 8c */
}

void
addcibuf(ClumpInfo *ci, vlong corrupt)
{
	Cit *cit;
	
	if(ncibuf == mcibuf){
		mcibuf += 256;
		cibuf = vtrealloc(cibuf, mcibuf*sizeof cibuf[0]);
	}
	cit = &cibuf[ncibuf++];
	cit->ci = *ci;
	cit->left = nil;
	cit->right = nil;
	cit->corrupt = corrupt;
	if(!corrupt)
		*ltreewalk(&ciroot, ci->score) = cit;
}

void
addcicorrupt(vlong len)
{
	static ClumpInfo zci;
	
	addcibuf(&zci, len);
}

int
haveclump(uchar *score)
{
	int i;
	Cit *t;
	
	t = ciroot;
	for(;;){
		if(t == nil)
			return 0;
		i = scorecmp(t->ci.score, score);
		if(i == 0)
			return 1;
		if(i < 0)
			t = t->right;
		else
			t = t->left;
	}
	return 0;	/* stupid 8c */
}

int
matchci(ClumpInfo *ci, uchar *p)
{
	if(ci->type != vtfromdisktype(p[0]))
		return 0;
	if(ci->size != u16(p+1))
		return 0;
	if(ci->uncsize != u16(p+3))
		return 0;
	if(scorecmp(ci->score, p+5) != 0)
		return 0;
	return 1;
}

/* XXX */
int
sealedarena(uchar *p, int blocksize)
{
	int v, n;
	
	v = u32(p+4);
	switch(v){
	default:
		return 0;
	case ArenaVersion4:
		n = ArenaSize4;
		break;
	case ArenaVersion5:
		n = ArenaSize5;
		break;
	}
	if(p[n-1] != 1){
		print("arena tail says not sealed\n");
		return 0;
	}
	if(memcmp(p+n, zero, blocksize-VtScoreSize-n) != 0){
		print("arena tail followed by non-zero data\n");
		return 0;
	}
	if(memcmp(p+blocksize-VtScoreSize, zero, VtScoreSize) == 0){
		print("arena score zero\n");
		return 0;
	}
	return 1;
}

int
okayname(char *name, int n)
{
	char buf[20];
	
	if(nameok(name) < 0)
		return 0;
	sprint(buf, "%d", n);
	if(strlen(name) < strlen(buf) 
	|| strcmp(name+strlen(name)-strlen(buf), buf) != 0)
		return 0;
	return 1;
}

int
clumpinfocmp(ClumpInfo *a, ClumpInfo *b)
{
	if(a->type != b->type)
		return a->type - b->type;
	if(a->size != b->size)
		return a->size - b->size;
	if(a->uncsize != b->uncsize)
		return a->uncsize - b->uncsize;
	return scorecmp(a->score, b->score);
}

ClumpInfo*
loadci(vlong offset, Arena *arena, int nci)
{
	int i, j, per;
	uchar *p, *sp;
	ClumpInfo *bci, *ci;
	
	per = arena->blocksize/ClumpInfoSize;
	bci = vtmalloc(nci*sizeof bci[0]);
	ci = bci;
	offset += arena->size - arena->blocksize;
	p = sp = nil;
	for(i=0; i<nci; i+=per){
		if(p == sp){
			sp = pagein(offset-4*M, 4*M);
			p = sp+4*M;
		}
		p -= arena->blocksize;
		offset -= arena->blocksize;
		for(j=0; j<per && i+j<nci; j++)
			unpackclumpinfo(ci++, p+j*ClumpInfoSize);
	}
	return bci;
}

vlong
writeci(vlong offset, Arena *arena, ClumpInfo *ci, int nci)
{
	int i, j, per;
	uchar *p, *sp;
	
	per = arena->blocksize/ClumpInfoSize;
	offset += arena->size - arena->blocksize;
	p = sp = nil;
	for(i=0; i<nci; i+=per){
		if(p == sp){
			sp = pagein(offset-4*M, 4*M);
			p = sp+4*M;
		}
		p -= arena->blocksize;
		offset -= arena->blocksize;
		memset(p, 0, arena->blocksize);
		for(j=0; j<per && i+j<nci; j++)
			packclumpinfo(ci++, p+j*ClumpInfoSize);
	}
	return offset;
}

void
loadarenabasics(vlong offset0, int anum, ArenaHead *head, Arena *arena)
{
	char dname[ANameSize];
	static char lastbase[ANameSize];
	uchar *p;
	u32int x;

	/*
	 * Fmtarenas makes all arenas the same size
	 * except the last, which may be smaller.
	 * It uses the same block size for arenas as for
	 * the arena partition blocks.
	 */
	arena->size = arenasize;
	if(offset0+arena->size > part->size)
		arena->size = part->size - offset0;
	head->size = arena->size;
	
	arena->blocksize = ap.blocksize;
	head->blocksize = arena->blocksize;
	
	/* 
	 * Look for clump magic and name in head/tail blocks.
	 * All the other info we will reconstruct just in case.
	 */
	p = pagein(offset0, arena->blocksize);
	if(u32(p) == ArenaHeadMagic)
	if((x=u32(p+4)) == ArenaVersion4 || x == ArenaVersion5){
		head->version = x;
		if(x == ArenaVersion4)
			head->clumpmagic = _ClumpMagic;
		else
			head->clumpmagic = 
				u32(p+2*U32Size+ANameSize+U32Size+U64Size);
		memmove(dname, p+2*U32Size, ANameSize);
		dname[ANameSize-1] = 0;
		if(okayname(dname, anum))
			strcpy(head->name, dname);
	}

	p = pagein(offset0+arena->size-arena->blocksize, 
		arena->blocksize);

	if(u32(p)==ArenaMagic)
	if((x=u32(p+4))==ArenaVersion4 || x==ArenaVersion5){
		arena->version = x;
		if(x == ArenaVersion4)
			arena->clumpmagic = _ClumpMagic;
		else
			arena->clumpmagic = 
				u32(p+2*U32Size+ANameSize+4*U32Size);
		memmove(dname, p+2*U32Size, ANameSize);
		dname[ANameSize-1] = 0;
		if(okayname(dname, anum))
			strcpy(arena->name, dname);
		arena->diskstats.clumps = u32(p+2*U32Size+ANameSize);
	}

	/* Head trumps arena. */
	if(head->version){
		arena->version = head->version;
		arena->clumpmagic = head->clumpmagic;
	}
	if(arena->version == 0)
		arena->version = ArenaVersion5;
	if(basename)
		snprint(arena->name, ANameSize, "%s%d", basename, anum);
	else if(lastbase[0])
		snprint(arena->name, ANameSize, "%s%d", lastbase, anum);
	else if(head->name[0])
		strcpy(arena->name, head->name);
	else if(arena->name[0] == 0)
		sysfatal("cannot determine base name for arena; use -n");
	strcpy(lastbase, arena->name);
	sprint(dname, "%d", anum);
	lastbase[strlen(lastbase)-strlen(dname)] = 0;
	
	/* Was working in arena, now copy to head. */
	head->version = arena->version;
	memmove(head->name, arena->name, sizeof head->name);
	head->blocksize = arena->blocksize;
	head->size = arena->size;
}


/*
 * Poke around in the arena to find the clump data
 * and compute the relevant statistics.
 */
void
guessarena(vlong offset0, int anum, ArenaHead *head, Arena *arena,
	uchar *oldscore, uchar *score)
{
	static char lastbase[ANameSize];
	uchar headbuf[MaxDiskBlock];
	int needtozero, clumps, nb1, nb2, minclumps;
	int diff, inbad, n, ncib, printed, sealing, smart;
	u32int magic;
	uchar *sp, *ep, *p;
	vlong boffset, eoffset, lastclumpend, leaked;
	vlong offset, oldshathru, shathru, toffset, totalcorrupt, v;
	Clump cl;
	ClumpInfo *bci, *ci, *eci, *xci;
	Cit *bcit, *cit, *ecit;
	
	/*
	 * We expect to find an arena, with data, between offset
	 * and offset+arenasize.  With any luck, the data starts at
	 * offset+ap.blocksize.  The blocks have variable size and
	 * aren't padded at all, which doesn't give us any alignment
	 * constraints.  The blocks are compressed or high entropy,
	 * but the headers are pretty low entropy (except the score):
	 *
	 *	type[1] (range 0 thru 9, 13)
	 *	size[2]
	 *	uncsize[2] (<= size)
	 *
	 * so we can look for these.  We check the scores as we go,
	 * so we can't make any wrong turns.  If we find ourselves
	 * in a dead end, scan forward looking for a new start.
	 */

	resetcibuf();
	memset(head, 0, sizeof *head);
	memset(arena, 0, sizeof *arena);
	memset(score, 0, VtScoreSize);
	memset(&shabuf, 0, sizeof shabuf);

	loadarenabasics(offset0, anum, head, arena);
	
	/* start the clump hunt */
	clumps = 0;
	totalcorrupt = 0;
	sealing = 1;
	shathru = 0;
	boffset = offset0 + arena->blocksize;
	offset = boffset;
	eoffset = offset0+arena->size - arena->blocksize;
	toffset = eoffset;
	sp = pagein(offset0, 4*M);
	oldshathru = 0;
	if(sealing){
		sha1(sp, 4*M, nil, &dsold);
		oldshathru = bufoffset+4*M;
	}
	ep = sp+4*M;
	p = sp + (boffset - offset0);
	ncib = arena->blocksize / ClumpInfoSize;	/* ci per block in index */
	lastclumpend = offset;
	nbad = 0;
	inbad = 0;
	needtozero = 0;
	minclumps = 0;
	while(offset < eoffset){
		/*
		 * Shift buffer if we're running out of room.
		 */
		if(p+70*K >= ep){
			/*
			 * Start the SHA1 buffer.   By now we should know the
			 * clumpmagic and arena version, so we can create a
			 * correct head block to get things going.
			 */
			if(sealing){
				if(shabuf.offset0 == 0){
					if(arena->clumpmagic == 0){
print("no clumpmagic; no seal\n");
						sealing = 0;
						goto Noseal;
					}
					memset(headbuf, 0, arena->blocksize);
					head->clumpmagic = arena->clumpmagic;
					packarenahead(head, headbuf);
					shabuf.offset0 = offset0;
					sha1(headbuf, arena->blocksize, nil, &dsnew);
					shathru = offset0 + arena->blocksize;
				}
				n = 4*M-256*K;
				if(bufoffset+n > eoffset)
					n = eoffset - bufoffset;
				if(n > 0){
					if(shathru < bufoffset)
						fprint(2, "bad sha: shathru=%,lld < bufoffset=%,lld\n", shathru, bufoffset);
					diff = shathru - bufoffset;
					sha1(sp+diff, n-diff, nil, &dsnew);
					shathru += n-diff;
				}
			}
		Noseal:

			n = 4*M-256*K;
			pagein(bufoffset+n, 4*M);
			p -= n;
			
			if(bufoffset+256*K+n > toffset)
				n = toffset - (bufoffset+256*K);
			if(sealing){
				sha1(sp+256*K, n, nil, &dsold);
				oldshathru = bufoffset+256*K+n;
			}
		}

		/*
		 * Check for a clump at p, which is at offset in the disk.
		 * Duplicate clumps happen in corrupted disks
		 * (the same pattern gets written many times in a row)
		 * and should never happen during regular use.
		 */
		if((n = isclump(p, &cl, &magic)) > 0 && !haveclump(cl.info.score)){
			/*
			 * If we were in the middle of some corrupted data,
			 * flush a warning about it and then add any clump
			 * info blocks as necessary.
			 */
			if(inbad){
				inbad = 0;
				v = offset-lastclumpend;
				if(needtozero){
					zerorange(lastclumpend, v);
					print("corrupt clump data - %#llux+%#llux (%,llud bytes)\n",
						lastclumpend, v, v);
				}
				addcicorrupt(v);
				totalcorrupt += v;
				nb1 = (minclumps+ncib-1)/ncib;
				minclumps += (v+ClumpSize+VtMaxLumpSize-1)/(ClumpSize+VtMaxLumpSize);
				nb2 = (minclumps+ncib-1)/ncib;
				eoffset -= (nb2-nb1)*arena->blocksize;
			}

			/*
			 * If clumps use different magic numbers, we don't care.
			 * We'll just use the first one we find and make the others
			 * follow suit.
			 */
			if(arena->clumpmagic == 0){
				print("clump type %d size %d score %V magic %x\n",
					cl.info.type, cl.info.size, cl.info.score, magic);
				arena->clumpmagic = magic;
				if(magic == _ClumpMagic)
					arena->version = ArenaVersion4;
				else
					arena->version = ArenaVersion5;
			}
			if(magic != arena->clumpmagic)
				p32(p, arena->clumpmagic);
			if(clumps == 0)
				arena->ctime = cl.time;

			/*
			 * Record the clump, update arena stats,
			 * grow clump info blocks if needed.
			 */
			if(verbose > 1)
				fprint(2, "\tclump %d: %d %V at %#llux+%#ux (%d)\n", 
					clumps, cl.info.type, cl.info.score, offset, n, n);
			addcibuf(&cl.info, 0);
			if(minclumps%ncib == 0)
				eoffset -= arena->blocksize;
			minclumps++;
			clumps++;
			if(cl.encoding != ClumpENone)
				arena->diskstats.cclumps++;
			arena->diskstats.uncsize += cl.info.uncsize;
			arena->wtime = cl.time;
			
			/*
			 * Move to next clump.
			 */
			offset += n;
			p += n;
			lastclumpend = offset;
		}else{
			/*
			 * Overwrite malformed clump data with zeros later.
			 * For now, just record whether it needs to be overwritten.
			 * Bad regions must be of size at least ClumpSize.
			 * Postponing the overwriting keeps us from writing past
			 * the end of the arena data (which might be directory data)
			 * with zeros.
			 */
			if(!inbad){
				inbad = 1;
				needtozero = 0;
				if(memcmp(p, zero, ClumpSize) != 0)
					needtozero = 1;
				p += ClumpSize;
				offset += ClumpSize;
				nbad++;
			}else{
				if(*p != 0)
					needtozero = 1;
				p++;
				offset++;
			}
		}
	}
fprint(2, "good clumps: %d; min clumps: %d\n", clumps, minclumps);
	arena->diskstats.used = lastclumpend - boffset;
	leaked = eoffset - lastclumpend;
	if(verbose)
		fprint(2, "used from %#llux to %#llux = %,lld (%,lld unused)\n",
			boffset, lastclumpend, arena->diskstats.used, leaked);

	/*
	 * Finish the SHA1 of the old data.
	 */
	if(sealing){
		for(; oldshathru<toffset; oldshathru+=n){
			n = 4*M;
			if(oldshathru+n > toffset)
				n = toffset - oldshathru;
			p = pagein(oldshathru, n);
			sha1(p, n, nil, &dsold);
		}
		p = pagein(toffset, arena->blocksize);
		sha1(p, arena->blocksize-VtScoreSize, nil, &dsold);
		sha1(zero, VtScoreSize, nil, &dsold);
		sha1(nil, 0, oldscore, &dsold);
	}

	/*
	 * If we still don't know the clump magic, the arena
	 * must be empty.  It still needs a value, so make 
	 * something up.
	 */
	if(arena->version == 0)
		arena->version = ArenaVersion5;
	if(arena->clumpmagic == 0){
		if(arena->version == ArenaVersion4)
			arena->clumpmagic = _ClumpMagic;
		else{
			do
				arena->clumpmagic = fastrand();
			while(arena->clumpmagic==_ClumpMagic
				||arena->clumpmagic==0);
		}
		head->clumpmagic = arena->clumpmagic;
	}
	
	/*
	 * Guess at number of clumpinfo blocks to load.
	 * If we guess high, it's no big deal.  If we guess low,
	 * we'll 
	 */
	if(clumps == 0 
	|| arena->diskstats.used == totalcorrupt)
		goto Nocib;
	if(clumps < arena->diskstats.clumps)
		clumps = arena->diskstats.clumps;
	if(clumps < ncibuf)
		clumps = ncibuf;
	clumps += totalcorrupt/
		((arena->diskstats.used - totalcorrupt)/clumps);
	clumps += totalcorrupt/2000;
	if(clumps < minclumps)
		clumps = minclumps;
	clumps += ncib-1;
	clumps -= clumps%ncib;
	/*
	 * Can't go into the actual data.
	 */
	v = offset0 + arena->size - arena->blocksize;
	v -= (clumps+ncib-1)/ncib * arena->blocksize;
	if(v < lastclumpend){
		v = offset0 + arena->size - arena->blocksize;
		clumps = (v-lastclumpend)/arena->blocksize * ncib;
	}
	
	if(clumps < minclumps)
		print("cannot happen?\n");

	/*
	 * Check clumpinfo blocks against directory we created.
	 * The tricky part is handling the corrupt sections of arena.
	 * If possible, we remark just the affected directory entries
	 * rather than slide everything down.
	 * 
	 * Allocate clumps+1 blocks and check that we don't need
	 * the last one at the end.
	 */
	bci = loadci(offset0, arena, clumps+1);
	eci = bci+clumps+1;
	bcit = cibuf;
	ecit = cibuf+ncibuf;
	smart = 1;
Again:
	nbad = 0;
	ci = bci;
	for(cit=bcit; cit<ecit && ci<eci; cit++){
		if(cit->corrupt){
			vlong n, m;
			if(smart){
				/*
				 * If we can, just mark existing entries as corrupt.
				 */
				n = cit->corrupt;
				for(xci=ci; n>0 && xci<eci; xci++)
					n -= ClumpSize+xci->size;
				if(n > 0 || xci >= eci)
					goto Dumb;
				printed = 0;
				for(; ci<xci; ci++){
					if(verbose && ci->type != VtCorruptType){
						if(!printed){
							print("marking directory %d-%d as corrupt\n",
								(int)(ci-bci), (int)(xci-bci));
							printed = 1;
						}
						print("\ttype=%d size=%d uncsize=%d score=%V\n",
							ci->type, ci->size, ci->uncsize, ci->score);
					}
					ci->type = VtCorruptType;
				}
			}else{
			Dumb:
				/*
				 * Otherwise, blaze a new trail.
				 */
xci = ci;
				n = cit->corrupt;
				while(n > 0 && ci < eci){
					if(n < ClumpSize)
						sysfatal("bad math in clump corrupt");
					if(n <= VtMaxLumpSize+ClumpSize)
						m = n;
					else{
						m = VtMaxLumpSize+ClumpSize;
						if(n-m < ClumpSize)
							m -= ClumpSize;
					}
					ci->type = VtCorruptType;
					ci->size = m-ClumpSize;
					ci->uncsize = m-ClumpSize;
					memset(ci->score, 0, VtScoreSize);
					ci++;
					n -= m;
				}
fprint(2, "blaze %d %d: %lld bytes in %ld\n", xci-bci, ci-bci, cit->corrupt, ci-xci);
			}
			continue;
		}
		if(clumpinfocmp(&cit->ci, ci) != 0){
			if(verbose && (smart || verbose>1)){
				print("clumpinfo %d\n", (int)(ci-bci));
				print("\twant: %d %d %d %V\n", 
					cit->ci.type, cit->ci.size,
					cit->ci.uncsize, cit->ci.score);
				print("\thave: %d %d %d %V\n", 
					ci->type, ci->size, 
					ci->uncsize, ci->score);
			}
			*ci = cit->ci;
			nbad++;
		}
		ci++;
	}
	if(ci >= eci || cit < ecit){
		print("ran out of space editing existing directory; rewriting\n");
print("eci %d ci %d ecit %d cit %d\n", eci-bci, ci-bci, ecit-bcit, cit-bcit);
		assert(smart);	/* can't happen second time thru */
		smart = 0;
		goto Again;
	}
	
	assert(ci <= eci);
	arena->diskstats.clumps = ci-bci;
print("new clumps %d\n", ci-bci);
	v = writeci(offset0, arena, bci, ci-bci);
	if(v - lastclumpend > 64*1024)
		sealing = 0;
	if(lastclumpend > v)
		print("arena directory overwrote blocks!  cannot happen!\n");
	free(bci);
	if(smart && nbad)
		print("arena directory has %d bad or missing entries\n", nbad);
Nocib:

	/*
	 * Finish the SHA1 of the new data.
	 */
	arena->diskstats.sealed = sealing;
	if(sealing){
		eoffset = offset0 + arena->size - arena->blocksize;
		for(; shathru<eoffset; shathru+=n){
			n = 4*M;
			if(shathru+n > eoffset)
				n = eoffset - shathru;
			p = pagein(shathru, n);
			sha1(p, n, nil, &dsnew);
		}
		memset(headbuf, 0, sizeof headbuf);
		packarena(arena, headbuf);
		sha1(headbuf, arena->blocksize, nil, &dsnew);
		sha1(nil, 0, score, &dsnew);
	}
	
	memset(&shabuf, 0, sizeof shabuf);
}

void
checkarena(vlong offset, int anum)
{
	uchar dbuf[MaxDiskBlock];
	uchar *p, oldscore[VtScoreSize], score[VtScoreSize];
	Arena arena;
	ArenaHead head;
	
	print("# arena %d: offset %#llux\n", anum, offset);

	guessarena(offset, anum, &head, &arena, oldscore, score);

	if(verbose){
		print("#\tversion=%d name=%s blocksize=%d size=%z",
			head.version, head.name, head.blocksize, head.size);
		if(head.clumpmagic)
			print(" clumpmagic=%#.8ux", head.clumpmagic);
		print("\n#\tclumps=%d cclumps=%d used=%,lld uncsize=%,lld\n",
			arena.diskstats.clumps, arena.diskstats.cclumps,
			arena.diskstats.used, arena.diskstats.uncsize);
		print("#\tctime=%t\n", arena.ctime);
		print("#\twtime=%t\n", arena.wtime);
		if(arena.diskstats.sealed)
			print("#\tsealed score=%V\n", score);
	}

	memset(dbuf, 0, sizeof dbuf);
	packarenahead(&head, dbuf);
	p = pagein(offset, arena.blocksize);
	if(memcmp(dbuf, p, arena.blocksize) != 0){
		print("on-disk arena header incorrect\n");
		showdiffs(dbuf, p, arena.blocksize, 
			arena.version==ArenaVersion4 ? headinfo4 : headinfo5);
	}
	memmove(p, dbuf, arena.blocksize);
	
	memset(dbuf, 0, sizeof dbuf);
	packarena(&arena, dbuf);
	if(arena.diskstats.sealed)
		scorecp(dbuf+arena.blocksize-VtScoreSize, score);
	p = pagein(offset+arena.size-arena.blocksize, arena.blocksize);
	if(memcmp(dbuf, p, arena.blocksize-VtScoreSize) != 0){
		print("on-disk arena tail incorrect\n");
		showdiffs(dbuf, p, arena.blocksize-VtScoreSize,
			arena.version==ArenaVersion4 ? tailinfo4 : tailinfo5);
	}
	if(arena.diskstats.sealed){
		if(scorecmp(p+arena.blocksize-VtScoreSize, oldscore) != 0){
			print("on-disk arena seal score incorrect\n");
			print("\tcorrect=%V\n", oldscore);
			print("\t   disk=%V\n", p+arena.blocksize-VtScoreSize);
		}
	}
	memmove(p, dbuf, arena.blocksize);
	
	pageout();
}

int mainstacksize = 512*1024;

void
threadmain(int argc, char **argv)
{
	int mode;
	
	mode = OREAD;
	readonly = 1;	
	ARGBEGIN{
	case 'a':
		arenasize = unittoull(EARGF(usage()));
		break;
	case 'b':
		ap.blocksize = unittoull(EARGF(usage()));
		break;
	case 'f':
		fix = 1;
		mode = ORDWR;
		readonly = 0;
		break;
	case 'n':
		basename = EARGF(usage());
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}ARGEND
	
	if(argc != 1 && argc != 2)
		usage();

	file = argv[0];
	
	ventifmtinstall();
	fmtinstall('z', zfmt);
	fmtinstall('t', tfmt);
	quotefmtinstall();
	
	part = initpart(file, mode|ODIRECT);
	if(part == nil)
		sysfatal("can't open %s: %r", file);
	
	checkarenas(argc > 1 ? argv[1] : nil);
	threadexitsall(nil);
}

