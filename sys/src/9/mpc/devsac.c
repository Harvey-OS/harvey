#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nram	= 512,
	CacheSize = 20,
	OffsetSize = 4,		/* size in bytes of an offset */
};

typedef struct SacPath SacPath;
typedef struct Sac Sac;
typedef struct SacHeader SacHeader;
typedef struct SacDir SacDir;
typedef struct Cache Cache;

enum {
	Magic = 0x5acf5,
};

struct SacDir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	uchar	qid[4];
	uchar	mode[4];
	uchar	atime[4];
	uchar	mtime[4];
	uchar	length[8];
	uchar	blocks[8];
};

struct SacHeader
{
	uchar	magic[4];
	uchar	length[8];
	uchar	blocksize[4];
	uchar	md5[16];
};


struct Sac
{
	SacDir;
	SacPath *path;
};

struct SacPath
{
	Ref;
	SacPath *up;
	long blocks;
	int entry;
	int nentry;
};

struct Cache
{
	long block;
	ulong age;
	uchar *data;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

static char *sacfs = "fs.sac";
static uchar *data;
static int blocksize;
static Sac root;
static Cache cache[CacheSize];
static ulong cacheage;

static void	sacdir(Chan *, SacDir*, char*);
static ulong	getl(void *p);
static Sac	*saccpy(Sac *s);
static Sac *saclookup(Sac *s, char *name);
static int sacdirread(Chan *, char *p, long off, long cnt);
static void loadblock(void *buf, uchar *offset, int blocksize);
static void sacfree(Sac*);

static void
sacinit(void)
{
	SacHeader *hdr;
	uchar *p;
	char *s;
	int i;

print("sacinit\n");
	s = getconf("flash");
	if(s == nil) {
		print("devsac: no flash file system\n");
		return;
	}

	p = (uchar*)strtoul(s, 0, 0);
	if(p == 0) {
		print("devsac: bad address for flash file system\n");
		return;
	}
	data = tarlookup(p, sacfs, &i);
	if(data == 0) {
		print("devsac: could not find file: %s\n", sacfs);
		return;
	}
	hdr = (SacHeader*)data;
	if(getl(hdr->magic) != Magic) {
print("devsac: bad magic\n");
		return;
	}
	blocksize = getl(hdr->blocksize);
	root.SacDir = *(SacDir*)(data + sizeof(SacHeader));
	p = malloc(CacheSize*blocksize);
	if(p == nil)
		error("allocating cache");
	for(i=0; i<CacheSize; i++) {
		cache[i].data = p;
		p += blocksize;
	}
}

static Chan*
sacattach(char* spec)
{
	Chan *c;
	int dev;

	dev = atoi(spec);
	if(dev != 0)
		error("bad specification");

	// check if init found sac file system in memory
	if(blocksize == 0)
		error("devsac: bad magic");

	c = devattach('C', spec);
	c->qid = (Qid){getl(root.qid), 0};
	c->dev = dev;
	c->aux = saccpy(&root);
	return c;
}

static Chan*
sacclone(Chan *c, Chan *nc)
{
	nc = devclone(c, nc);
	nc->aux = saccpy(c->aux);
	return nc;
}

static int
sacwalk(Chan *c, char *name)
{
	Sac *sac;

//print("walk %s\n", name);

	isdir(c);
	if(name[0]=='.' && name[1]==0)
		return 1;
	sac = c->aux;
	sac = saclookup(sac, name);
	if(sac == nil) {
		strncpy(up->error, Enonexist, NAMELEN);
		return 0;
	}
	c->aux = sac;
	c->qid = (Qid){getl(sac->qid), 0};
	return 1;
}

static Chan*
sacopen(Chan *c, int omode)
{
	ulong t, mode;
	Sac *sac;
	static int access[] = { 0400, 0200, 0600, 0100 };

	sac = c->aux;
	mode = getl(sac->mode);
	if(strcmp(up->user, sac->uid) == 0)
		mode = mode;
	else if(strcmp(up->user, sac->gid) == 0)
		mode = mode<<3;
	else
		mode = mode<<6;

	t = access[omode&3];
	if((t & mode) != t)
			error(Eperm);
	c->offset = 0;
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
}


static long
sacread(Chan *c, void *a, long n, vlong voff)
{
	Sac *sac;
	char *buf, *buf2;
	int nn, cnt, i, j;
	uchar *blocks;
	long length;
	long off = voff;

	buf = a;
	cnt = n;
	if(c->qid.path & CHDIR){
		cnt = (cnt/DIRLEN)*DIRLEN;
		if(off%DIRLEN)
			error("i/o error");
		return sacdirread(c, buf, off, cnt);
	}
	sac = c->aux;
	length = getl(sac->length);
	if(off >= length)
		return 0;
	if(cnt > length-off)
		cnt = length-off;
	if(cnt == 0)
		return 0;
	n = cnt;
	blocks = data + getl(sac->blocks);
	buf2 = malloc(blocksize);
	while(cnt > 0) {
		i = off/blocksize;
		nn = blocksize;
		if(nn > length-i*blocksize)
			nn = length-i*blocksize;
		loadblock(buf2, blocks+i*OffsetSize, nn);
		j = off-i*blocksize;
		nn -= j;
		if(nn > cnt)
			nn = cnt;
		memmove(buf, buf2+j, nn);
		cnt -= nn;
		off += nn;
		buf += nn;
	}
	free(buf2);
	return n;
}

static long
sacwrite(Chan *, void *, long, vlong)
{
	error(Eperm);
	return 0;
}

static void
sacclose(Chan* c)
{
	Sac *sac = c->aux;
	c->aux = nil;
	sacfree(sac);
}


static void
sacstat(Chan *c, char *db)
{
	sacdir(c, c->aux, db);
}

static Sac*
saccpy(Sac *s)
{
	Sac *ss;
	
	ss = malloc(sizeof(Sac));
	*ss = *s;
	if(ss->path)
		incref(ss->path);
	return ss;
}

static SacPath *
sacpathalloc(SacPath *p, long blocks, int entry, int nentry)
{
	SacPath *pp = malloc(sizeof(SacPath));
	pp->ref = 1;
	pp->blocks = blocks;
	pp->entry = entry;
	pp->nentry = nentry;
	pp->up = p;
	return pp;
}

static void
sacpathfree(SacPath *p)
{
	if(p == nil)
		return;
	if(decref(p) > 0)
		return;
	sacpathfree(p->up);
	free(p);
}


static void
sacfree(Sac *s)
{
	sacpathfree(s->path);
	free(s);
}

static void
sacdir(Chan *c, SacDir *s, char *buf)
{
	Dir dir;

	memmove(dir.name, s->name, NAMELEN);
	dir.qid = (Qid){getl(s->qid), 0};
	dir.mode = getl(s->mode);
	dir.length = getl(s->length);
	if(dir.mode &CHDIR)
		dir.length *= DIRLEN;
	memmove(dir.uid, s->uid, NAMELEN);
	memmove(dir.gid, s->gid, NAMELEN);
	dir.atime = getl(s->atime);
	dir.mtime = getl(s->mtime);
	dir.type = devtab[c->type]->dc;
	dir.dev = c->dev;
	convD2M(&dir, buf);
}

static void
loadblock(void *buf, uchar *offset, int blocksize)
{
	long block, n;
	ulong age;
	int i, j;

	block = getl(offset);
	if(block < 0) {
		block = -block;
		cacheage++;
		// age has wraped
		if(cacheage == 0) {
			for(i=0; i<CacheSize; i++)
				cache[i].age = 0;
		}
		j = 0;
		age = cache[0].age;
		for(i=0; i<CacheSize; i++) {
			if(cache[i].age < age) {
				age = cache[i].age;
				j = i;
			}
			if(cache[i].block != block)
				continue;
			memmove(buf, cache[i].data, blocksize);
			cache[i].age = cacheage;
			return;
		}

		n = getl(offset+OffsetSize);
		if(n < 0)
			n = -n;
		n -= block;
		if(unsac(buf, data+block, blocksize, n)<0)
			panic("unsac failed!");
		memmove(cache[j].data, buf, blocksize);
		cache[j].age = cacheage;
		cache[j].block = block;
	} else {
		memmove(buf, data+block, blocksize);
	}
}

static Sac*
sacparent(Sac *s)
{
	uchar *blocks;
	SacDir *buf;
	int per, i, n;
	SacPath *p;

	p = s->path;
	if(p == nil || p->up == nil) {
		sacpathfree(p);
		*s = root;
		return s;
	}
	p = p->up;

	blocks = data + p->blocks;
	per = blocksize/sizeof(SacDir);
	i = p->entry/per;
	n = per;
	if(n > p->nentry-i*per)
		n = p->nentry-i*per;
	buf = malloc(per*sizeof(SacDir));
	loadblock(buf, blocks + i*OffsetSize, n*sizeof(SacDir));
	s->SacDir = buf[p->entry-i*per];
	free(buf);
	incref(p);
	sacpathfree(s->path);
	s->path = p;
	return s;
}

static int
sacdirread(Chan *c, char *p, long off, long cnt)
{
	uchar *blocks;
	SacDir *buf;
	int iblock, per, i, j, n, ndir;
	Sac *s;

	s = c->aux;
	blocks = data + getl(s->blocks);
	per = blocksize/sizeof(SacDir);
	ndir = getl(s->length);
	off /= DIRLEN;
	cnt /= DIRLEN;
	if(off >= ndir)
		return 0;
	if(cnt > ndir-off)
		cnt = ndir-off;
	iblock = -1;
	buf = malloc(per*sizeof(SacDir));
	for(i=off; i<off+cnt; i++) {
		j = i/per;
		if(j != iblock) {
			n = per;
			if(n > ndir-j*per)
				n = ndir-j*per;
			loadblock(buf, blocks + j*OffsetSize, n*sizeof(SacDir));
			iblock = j;
		}
		j *= per;
		sacdir(c, buf+i-j, p);
		p += DIRLEN;
	}
	free(buf);
	return cnt*DIRLEN;
}

static Sac*
saclookup(Sac *s, char *name)
{
	int ndir;
	int i, j, k, n, per;
	uchar *blocks;
	SacDir *buf;
	int iblock;
	SacDir *sd;
	
	if(strcmp(name, "..") == 0)
		return sacparent(s);
	blocks = data + getl(s->blocks);
	per = blocksize/sizeof(SacDir);
	ndir = getl(s->length);
	buf = malloc(per*sizeof(SacDir));
	iblock = -1;

	// linear search
	for(i=0; i<ndir; i++) {
		j = i/per;
		if(j != iblock) {
			n = per;
			if(n > ndir-j*per)
				n = ndir-j*per;
			loadblock(buf, blocks + j*OffsetSize, n*sizeof(SacDir));
			iblock = j;
		}
		j *= per;
		sd = buf+i-j;
		k = strcmp(name, sd->name);
		if(k == 0) {
		s->path = sacpathalloc(s->path, getl(s->blocks), i, ndir);
			s->SacDir = *sd;
			free(buf);
			return s;
		}
	}
	free(buf);
	return 0;
}

static ulong
getl(void *p)
{
	uchar *a = p;

	return (a[0]<<24) | (a[1]<<16) | (a[2]<<8) | a[3];
}

Dev sacdevtab = {
	'C',
	"sac",

	devreset,
	sacinit,
	sacattach,
	sacclone,
	sacwalk,
	sacstat,
	sacopen,
	devcreate,
	sacclose,
	sacread,
	devbread,
	sacwrite,
	devbwrite,
	devremove,
	devwstat,
};
