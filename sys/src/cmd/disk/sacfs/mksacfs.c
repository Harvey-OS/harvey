#include <u.h>
#include <libc.h>
#include "sac.h"
#include "sacfs.h"

enum {
	NCACHE = 1024,	/* must be power of two */
	OffsetSize = 4,	/* size of block offset */
};

int	warn(char *);
int	seen(Dir *);
void usage(void);
void outwrite(void *buf, int n, long off);
void putl(void *p, uint v);
void *emalloc(int size);
void sacfs(char *root);
void sacfile(char *name, Dir *dir, SacDir *sd);
void sacdir(char *name, Dir *dir, SacDir *sd);

int	uflag=0;
int	fflag=0;
long blocksize = 4*1024;

struct Out
{
	int	fd;
	long size;
} out;

typedef	struct	Cache	Cache;
struct	Cache
{
	Dir*	cache;
	int	n;
	int	max;
} cache[NCACHE];

void
main(int argc, char *argv[])
{
	char *s, *ss;
	char *outfile = nil;

	ARGBEGIN {
	case 'u':
		uflag=1;
		break;
	case 'o':
		outfile = ARGF();
		break;
	case 'b':
		s = ARGF();
		if(s) {
			blocksize = strtoul(s, &ss, 0);
			if(s == ss)
				usage();
			if(*ss == 'k')
				blocksize *= 1024;
		}
		if(blocksize < sizeof(SacDir))
			sysfatal("blocksize too small");
		break;
	} ARGEND

	if(outfile == nil) {
		sysfatal("still to do: need to create temp file");
	} else {
		out.fd = create(outfile, OWRITE|OTRUNC, 0664);
		if(out.fd < 0)
			sysfatal("could not create file: %s: %r", outfile);
	}

	if(argc==0)
		sacfs(".");
	else
		sacfs(argv[0]);

	close(out.fd);

	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: %s [-u] [-b blocksize] -o output [root]\n", argv0);
	exits("usage");
}

void
sacfs(char *root)
{
	Dir *dir;
	long offset;
	SacHeader hdr;
	SacDir sd;

	dir = dirstat(root);
	if(dir == nil)
		sysfatal("could not stat root: %s: %r", root);
	offset = out.size;
	out.size += sizeof(SacHeader) + sizeof(SacDir);
	memset(&hdr, 0, sizeof(hdr));
	putl(hdr.magic, Magic);
	putl(hdr.blocksize, blocksize);
	if(dir->mode & DMDIR)
		sacdir(root, dir, &sd);
	else
		sacfile(root, dir, &sd);
	putl(hdr.length, out.size);
	outwrite(&hdr, sizeof(hdr), offset);
	outwrite(&sd, sizeof(sd), offset+sizeof(SacHeader));
	free(dir);
}

void
setsd(SacDir *sd, Dir *dir, long length, long blocks)
{
	static qid = 1;

	memset(sd, 0, sizeof(SacDir));
	strncpy(sd->name, dir->name, NAMELEN);
	strncpy(sd->uid, dir->uid, NAMELEN);
	strncpy(sd->gid, dir->gid, NAMELEN);
	putl(sd->qid, qid++|(dir->mode&DMDIR));
	putl(sd->mode, dir->mode);
	putl(sd->atime, dir->atime);
	putl(sd->mtime, dir->mtime);
	putl(sd->length, length);
	putl(sd->blocks, blocks);
}


void
sacfile(char *name, Dir *dir, SacDir *sd)
{
	int fd, i, n, nn;
	long nblock;
	uchar *blocks;
	uchar *buf, *cbuf;
	long offset;
	long block;

	fd = open(name, OREAD);
	if(fd < 0)
		sysfatal("could not open file: %s: %r", name);
	nblock = (dir->length + blocksize-1)/blocksize;
	blocks = emalloc((nblock+1)*OffsetSize);
	buf = emalloc(blocksize);
	cbuf = emalloc(blocksize);
	offset = out.size;
	out.size += (nblock+1)*OffsetSize;
	for(i=0; i<nblock; i++) {
		n = read(fd, buf, blocksize);
		if(n < 0)
			sysfatal("read failed: %s: %r", name);
		if(n == 0)
			sysfatal("unexpected eof: %s", name);
		if(n < blocksize && i != nblock-1)
			sysfatal("short read: %s: got %d", name, n);
		block = out.size;
		nn = sac(cbuf, buf, n);
		if(nn < 0 || uflag) {
			outwrite(buf, n, out.size);
			out.size += n;
		} else {
			block = -block;
			outwrite(cbuf, nn, out.size);
			out.size += nn;
		}
		putl(blocks+i*OffsetSize, block);
	}
	putl(blocks+i*OffsetSize, out.size);
	outwrite(blocks, (nblock+1)*OffsetSize, offset);
	setsd(sd, dir, dir->length, offset);
	close(fd);
	free(buf);
	free(cbuf);
	free(blocks);
}

void
sacdir(char *name, Dir *dir, SacDir *sd)
{
	Dir *dirs, *p;
	int i, n, nn, per;
	SacDir *sds;
	int ndir, fd, nblock;
	long offset, block;
	uchar *blocks, *cbuf;
	char file[512];

	fd = open(name, OREAD);
	if(fd < 0)
		sysfatal("could not open directory: %s: %r", name);
	ndir = dirreadall(fd, &dirs);
	if(ndir < 0)
		sysfatal("could not read directory: %s: %r", name);
	close(fd);
	per = blocksize/sizeof(SacDir);
	nblock = (ndir+per-1)/per;
	sds = emalloc(nblock*per*sizeof(SacDir));
	p = dirs;
	for(i=0; i<ndir; i++,p++) {
		sprint(file, "%s/%s", name, p->name);
		if(p->mode & DMDIR)
			sacdir(file, p, sds+i);
		else
			sacfile(file, p, sds+i);
	}
	free(dirs);
	blocks = emalloc((nblock+1)*OffsetSize);
	offset = out.size;
	out.size += (nblock+1)*OffsetSize;
	n = per*sizeof(SacDir);
	cbuf = emalloc(n);
	for(i=0; i<nblock; i++) {
		block = out.size;
		if(n > (ndir-i*per)*sizeof(SacDir))
			n = (ndir-i*per)*sizeof(SacDir);
		nn = sac(cbuf, (uchar*)(sds+i*per), n);
		if(nn < 0 || uflag) {
			outwrite(sds+i*per, n, out.size);
			out.size += n;
		} else {
			block = -block;
			outwrite(cbuf, nn, out.size);
			out.size += nn;
		}
		putl(blocks+i*OffsetSize, block);
	}
	free(cbuf);
	putl(blocks+i*OffsetSize, out.size);
	outwrite(blocks, (nblock+1)*OffsetSize, offset);
	setsd(sd, dir, ndir, offset);
	free(sds);
	free(blocks);
}

int
seen(Dir *dir)
{
	Dir *dp;
	int i;
	Cache *c;

	c = &cache[dir->qid.path&(NCACHE-1)];
	dp = c->cache;
	for(i=0; i<c->n; i++, dp++)
		if(dir->qid.path == dp->qid.path &&
		   dir->type == dp->type &&
		   dir->dev == dp->dev)
			return 1;
	if(c->n == c->max){
		c->cache = realloc(c->cache, (c->max+=20)*sizeof(Dir));
		if(cache == 0)
			sysfatal("malloc failure");
	}
	c->cache[c->n++] = *dir;
	return 0;
}

void
outwrite(void *buf, int n, long offset)
{
	if(seek(out.fd, offset, 0) < 0)
		sysfatal("seek failed: %r");
	if(write(out.fd, buf, n) < n)
		sysfatal("write failed: %r");
}

void
putl(void *p, uint v)
{
	uchar *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void *
emalloc(int size)
{
	void *p;

	p = malloc(size);
	if(p == nil)
		sysfatal("malloc failed");
	memset(p, 0, size);
	return p;
}
