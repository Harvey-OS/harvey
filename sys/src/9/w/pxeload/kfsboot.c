/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"fs.h"

typedef struct Tag Tag;

/*
 * tags on block
 */
enum
{
	Tnone		= 0,
	Tsuper,			/* the super block */
	Tdir,			/* directory contents */
	Tind1,			/* points to blocks */
	Tind2,			/* points to Tind1 */
	Tfile,			/* file contents */
	Tfree,			/* in free list */
	Tbuck,			/* cache fs bucket */
	Tvirgo,			/* fake worm virgin bits */
	Tcache,			/* cw cache things */
	MAXTAG
};

#define	QPDIR	0x80000000L
#define	QPNONE	0
#define	QPROOT	1
#define	QPSUPER	2

/* DONT TOUCH, this is the disk structure */
struct	Tag
{
	int16_t	pad;
	int16_t	tag;
	int32_t	path;
};

static int thisblock = -1;
static Fs *thisfs;
static uint8_t *block;

/*
 * we end up reading 2x or 3x the number of blocks we need to read.
 * this is okay because we need to read so few.  if it wasn't okay, we could
 * have getblock return a pointer to a block, and keep a cache of the last
 * three read blocks.  that would get us down to the minimum.
 * but this is fine.
 */
static int
getblock(Fs *fs, uint32_t n)
{
	if(!block)
		block = malloc(16384);

	if(thisblock == n && thisfs == fs)
		return 0;
	thisblock = -1;
	if(fs->diskseek(fs, (int64_t)n*fs->kfs.RBUFSIZE) < 0)
		return -1;
	if(fs->diskread(fs, block, fs->kfs.RBUFSIZE) != fs->kfs.RBUFSIZE)
		return -1;
	thisblock = n;
	thisfs = fs;

	return 1;
}

static int
checktag(Fs *fs, uint8_t *block, int tag, int32_t qpath)
{
	Tag *t;

	t = (Tag*)(block+fs->kfs.BUFSIZE);
	if(t->tag != tag)
		return -1;
	if(qpath != QPNONE && (qpath&~QPDIR) != t->path)
		return -1;
	return 1;
}

static int
getblocktag(Fs *fs, uint32_t n, int tag, int32_t qpath)
{
	if(getblock(fs, n) < 0 || checktag(fs, block, tag, qpath) < 0)
		return -1;
	return 1;
}

static int
readinfo(Fs *fs)
{
	fs->kfs.RBUFSIZE = 512;
	if(getblock(fs, 0) < 0)
		return -1;

	if(memcmp(block+256, "kfs wren device\n", 16) != 0)
		return -1;

	fs->kfs.RBUFSIZE = atoi((int8_t*)block+256+16);
	if(!fs->kfs.RBUFSIZE || (fs->kfs.RBUFSIZE&(fs->kfs.RBUFSIZE-1)))
		return -1;

	fs->kfs.BUFSIZE = fs->kfs.RBUFSIZE - sizeof(Tag);
	fs->kfs.DIRPERBUF = fs->kfs.BUFSIZE / sizeof(Dentry);
	fs->kfs.INDPERBUF = fs->kfs.BUFSIZE / sizeof(int32_t);
	fs->kfs.INDPERBUF2 = fs->kfs.INDPERBUF * fs->kfs.INDPERBUF;

	return 1;
}

static int
readroot(Fs *fs, Dentry *d)
{
	Dentry *d2;

	if(getblocktag(fs, 2, Tdir, QPROOT) < 0)
		return -1;
	d2 = (Dentry*)block;
	if(strcmp(d2->name, "/") != 0)
		return -1;
	*d = *(Dentry*)block;
	return 1;
}

static int32_t
indfetch(Fs *fs, int32_t addr, int32_t off, int tag, int32_t path)
{
	if(getblocktag(fs, addr, tag, path) < 0)
		return -1;
	return ((int32_t*)block)[off];
}

static int32_t
rel2abs(Fs *fs, Dentry *d, int32_t a)
{
	int32_t addr;

	if(a < NDBLOCK)
		return d->dblock[a];
	a -= NDBLOCK;
	if(a < fs->kfs.INDPERBUF){
		if(d->iblock == 0)
			return 0;
		addr = indfetch(fs, d->iblock, a, Tind1, d->qid.path);
		if(addr == 0)
			print("rel2abs indfetch 0 %s %ld\n", d->name, a);
		return addr;
	}
	a -= fs->kfs.INDPERBUF;
	if(a < fs->kfs.INDPERBUF2){
		if(d->diblock == 0)
			return 0;
		addr = indfetch(fs, d->diblock, a/fs->kfs.INDPERBUF, Tind2, d->qid.path);
		if(addr == 0){
			print("rel2abs indfetch 0 %s %ld\n", d->name, a/fs->kfs.INDPERBUF);
			return 0;
		}
		addr = indfetch(fs, addr, a%fs->kfs.INDPERBUF, Tind1, d->qid.path);
		return addr;
	}
	print("rel2abs trip ind %s %ld\n", d->name, a);
	return -1;
}

static int
readdentry(Fs *fs, Dentry *d, int n, Dentry *e)
{
	int32_t addr, m;

	m = n/fs->kfs.DIRPERBUF;
	if((addr = rel2abs(fs, d, m)) <= 0)
		return addr;
	if(getblocktag(fs, addr, Tdir, d->qid.path) < 0)
		return -1;
	*e = *(Dentry*)(block+(n%fs->kfs.DIRPERBUF)*sizeof(Dentry));
	return 1;
}

static int
getdatablock(Fs *fs, Dentry *d, int32_t a)
{
	int32_t addr;

	if((addr = rel2abs(fs, d, a)) == 0)
		return -1;
	return getblocktag(fs, addr, Tfile, QPNONE);
}

static int
walk(Fs *fs, Dentry *d, int8_t *name, Dentry *e)
{
	int i, n;
	Dentry x;

	for(i=0;; i++){
		if((n=readdentry(fs, d, i, &x)) <= 0)
			return n;
		if(strcmp(x.name, name) == 0){
			*e = x;
			return 1;
		}
	}
}

static int32_t
kfsread(File *f, void *va, int32_t len)
{
	uint8_t *a;
	int32_t tot, off, o, n;
	Fs *fs;

	a = va;
	fs = f->fs;
	off = f->kfs.off;
	tot = 0;
	while(tot < len){
		if(getdatablock(fs, &f->kfs, off/fs->kfs.BUFSIZE) < 0)
			return -1;
		o = off%fs->kfs.BUFSIZE;
		n = fs->kfs.BUFSIZE - o;
		if(n > len-tot)
			n = len-tot;
		memmove(a+tot, block+o, n);
		off += n;
		tot += n;
	}
	f->kfs.off = off;
	return tot;
}

static int
kfswalk(File *f, int8_t *name)
{
	int n;

	n = walk(f->fs, &f->kfs, name, &f->kfs);
	if(n < 0)
		return -1;
	f->kfs.off = 0;
	return 1;
}

int
kfsinit(Fs *fs)
{
	if(readinfo(fs) < 0 || readroot(fs, &fs->root.kfs) < 0)
		return -1;

	fs->root.fs = fs;
	fs->read = kfsread;
	fs->walk = kfswalk;
	return 0;
}
