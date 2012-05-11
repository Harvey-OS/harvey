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
	short	pad;
	short	tag;
	long	path;
};

static int thisblock = -1;
static Fs *thisfs;
static uchar *block;

/*
 * we end up reading 2x or 3x the number of blocks we need to read.
 * this is okay because we need to read so few.  if it wasn't okay, we could
 * have getblock return a pointer to a block, and keep a cache of the last
 * three read blocks.  that would get us down to the minimum.
 * but this is fine.
 */
static int
getblock(Fs *fs, ulong n)
{
	if(!block)
		block = malloc(16384);

	if(thisblock == n && thisfs == fs)
		return 0;
	thisblock = -1;
	if(fs->diskseek(fs, (vlong)n*fs->kfs.RBUFSIZE) < 0)
		return -1;
	if(fs->diskread(fs, block, fs->kfs.RBUFSIZE) != fs->kfs.RBUFSIZE)
		return -1;
	thisblock = n;
	thisfs = fs;

	return 1;
}

static int
checktag(Fs *fs, uchar *block, int tag, long qpath)
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
getblocktag(Fs *fs, ulong n, int tag, long qpath)
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

	fs->kfs.RBUFSIZE = atoi((char*)block+256+16);
	if(!fs->kfs.RBUFSIZE || (fs->kfs.RBUFSIZE&(fs->kfs.RBUFSIZE-1)))
		return -1;

	fs->kfs.BUFSIZE = fs->kfs.RBUFSIZE - sizeof(Tag);
	fs->kfs.DIRPERBUF = fs->kfs.BUFSIZE / sizeof(Dentry);
	fs->kfs.INDPERBUF = fs->kfs.BUFSIZE / sizeof(long);
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

static long
indfetch(Fs *fs, long addr, long off, int tag, long path)
{
	if(getblocktag(fs, addr, tag, path) < 0)
		return -1;
	return ((long*)block)[off];
}

static long
rel2abs(Fs *fs, Dentry *d, long a)
{
	long addr;

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
	long addr, m;

	m = n/fs->kfs.DIRPERBUF;
	if((addr = rel2abs(fs, d, m)) <= 0)
		return addr;
	if(getblocktag(fs, addr, Tdir, d->qid.path) < 0)
		return -1;
	*e = *(Dentry*)(block+(n%fs->kfs.DIRPERBUF)*sizeof(Dentry));
	return 1;
}

static int
getdatablock(Fs *fs, Dentry *d, long a)
{
	long addr;

	if((addr = rel2abs(fs, d, a)) == 0)
		return -1;
	return getblocktag(fs, addr, Tfile, QPNONE);
}

static int
walk(Fs *fs, Dentry *d, char *name, Dentry *e)
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

static long
kfsread(File *f, void *va, long len)
{
	uchar *a;
	long tot, off, o, n;
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
kfswalk(File *f, char *name)
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
