#include <u.h>
#include <libc.h>
#include <bio.h>
#include <flate.h>
#include <mp.h>
#include <libsec.h>
#include "paqfs.h"

enum {
	OffsetSize = 4,	/* size of block offset */
};

void paqfs(char *root, char *label);
PaqDir *paqFile(char *name, Dir *dir);
PaqDir *paqDir(char *name, Dir *dir);
PaqDir *paqDirAlloc(Dir *d, ulong offset);
void paqDirFree(PaqDir *pd);
void writeHeader(char *label);
void writeTrailer(ulong root);
ulong writeBlock(uchar *buf, int type);
void usage(void);
void outWrite(void *buf, int n);
int paqDirSize(PaqDir *dir);
void putDir(uchar *p, PaqDir *dir);
void putHeader(uchar *p, PaqHeader *h);
void putBlock(uchar *p, PaqBlock *h);
void putTrailer(uchar *p, PaqTrailer *t);
void putl(uchar *p, ulong v);
void puts(uchar *p, int x);
uchar *putstr(uchar *p, char *s);
void *emallocz(int size);
void warn(char *fmt, ...);

int uflag=0;			/* uncompressed */
long blocksize = 4*1024;

Biobuf *out;
DigestState *outdg;

void
main(int argc, char *argv[])
{
	char *s, *ss;
	char *outfile = nil;
	char *label = nil;
	char *file;

	ARGBEGIN {
	case 'u':
		uflag=1;
		break;
	case 'o':
		outfile = ARGF();
		break;
	case 'l':
		label = ARGF();
		if(label == nil)
			usage();
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
		if(blocksize < MinBlockSize)
			sysfatal("blocksize too small: must be at lease %d", MinBlockSize);
		if(blocksize > MaxBlockSize)
			sysfatal("blocksize too large: must be no greater than %d", MaxBlockSize);
		break;
	} ARGEND

	if(outfile == nil) {
		out = emallocz(sizeof(Biobuf));
		Binit(out, 1, OWRITE);
	} else {
		out = Bopen(outfile, OWRITE|OTRUNC);
		if(out == nil)
			sysfatal("could not create file: %s: %r", outfile);
	}

	deflateinit();

	file = argv[0];
	if(file == nil)
		file = ".";

	if(label == nil) {
		if(strrchr(file, '/'))
			label = strrchr(file, '/') + 1;
		else
			label = file;
	}

	paqfs(file, label);

	Bterm(out);

	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: %s [-u] [-b blocksize] -o output [root]\n", argv0);
	exits("usage");
}

void
paqfs(char *root, char *label)
{
	Dir *dir;
	PaqDir *pd;
	ulong offset;
	uchar *buf;

	dir = dirstat(root);
	if(dir == nil)
		sysfatal("could not stat root: %s: %r", root);
	writeHeader(label);
	if(dir->mode & DMDIR)
		pd = paqDir(root, dir);
	else
		pd = paqFile(root, dir);
	buf = emallocz(blocksize);
	putDir(buf, pd);
	offset = writeBlock(buf, DirBlock);
	writeTrailer(offset);
	paqDirFree(pd);
	free(dir);
}


PaqDir *
paqFile(char *name, Dir *dir)
{
	int fd, n, nn, nb;
	vlong tot;
	uchar *block, *pointer;
	ulong offset;

	fd = open(name, OREAD);
	if(fd < 0) {
		warn("could not open file: %s: %r", name);
		return nil;
	}

	block = emallocz(blocksize);
	pointer = emallocz(blocksize);
	nb = 0;
	n = 0;
	tot = 0;
	for(;;) {
		nn = read(fd, block+n, blocksize-n);
		if(nn < 0) {
			warn("read failed: %s: %r", name);
			goto Err;
		}
		tot += nn;
		if(nn == 0) {	
			if(n == 0)
				break;	
			/* pad out last block */
			memset(block+n, 0, blocksize-n);
			nn = blocksize - n;
		}
		n += nn;
		if(n < blocksize)
			continue;
		if(nb >= blocksize/OffsetSize) {
			warn("file too big for blocksize: %s", name);
			goto Err;
		}
		offset = writeBlock(block, DataBlock);
		putl(pointer+nb*OffsetSize, offset);
		nb++;
		n = 0;
	}

	offset = writeBlock(pointer, PointerBlock);

	close(fd);
	free(pointer);
	free(block);
	dir->length = tot;
	return paqDirAlloc(dir, offset);
Err:
	close(fd);
	free(pointer);
	free(block);
	return nil;
}

PaqDir *
paqDir(char *name, Dir *dir)
{
	Dir *dirs, *p;
	PaqDir *pd;
	int i, n, nb, fd, ndir;
	uchar *block, *pointer;
	char *nname;
	ulong offset;

	fd = open(name, OREAD);
	if(fd < 0) {
		warn("could not open directory: %s: %r", name);
		return nil;
	}

	ndir = dirreadall(fd, &dirs);
	close(fd);

	if(ndir < 0) {
		warn("could not read directory: %s: %r", name);
		return nil;
	}

	block = emallocz(blocksize);
	pointer = emallocz(blocksize);
	nb = 0;
	n = 0;
	nname = nil;
	pd = nil;

	for(i=0; i<ndir; i++) {
		p = dirs + i;
		free(nname);
		nname = emallocz(strlen(name) + strlen(p->name) + 2);
		sprint(nname, "%s/%s", name, p->name);
		if(p->mode & DMDIR)
			pd = paqDir(nname, p);
		else
			pd = paqFile(nname, p);
		if(pd == nil)
			continue;

		if(n+paqDirSize(pd) >= blocksize) {

			/* zero fill the block */
			memset(block+n, 0, blocksize-n);
			offset = writeBlock(block, DirBlock);
			n = 0;
			if(nb >= blocksize/OffsetSize) {
				warn("directory too big for blocksize: %s", nname);
				goto Err;
			}
			putl(pointer+nb*OffsetSize, offset);
			nb++;
		}
		if(n+paqDirSize(pd) >= blocksize) {
			warn("directory entry does not fit in a block: %s", nname);
			paqDirFree(pd);
			continue;
		}
		putDir(block+n, pd);
		n += paqDirSize(pd);
		paqDirFree(pd);
		pd = nil;
	}

	if(n > 0) {
		/* zero fill the block */
		memset(block+n, 0, blocksize-n);
		offset = writeBlock(block, DirBlock);
		if(nb >= blocksize/OffsetSize) {
			warn("directory too big for blocksize: %s", nname);
			goto Err;
		}
		putl(pointer+nb*OffsetSize, offset);
	}
	offset = writeBlock(pointer, PointerBlock);

	free(nname);
	free(dirs);
	paqDirFree(pd);
	free(block);
	free(pointer);
	return paqDirAlloc(dir, offset);
Err:	
	free(nname);
	free(dirs);
	paqDirFree(pd);
	free(block);
	free(pointer);
	return nil;
}

PaqDir *
paqDirAlloc(Dir *dir, ulong offset)
{
	PaqDir *pd;
	static ulong qid = 1;

	pd = emallocz(sizeof(PaqDir));
	
	pd->name = strdup(dir->name);
	pd->qid = qid++;
	pd->mode = dir->mode & (DMDIR|DMAPPEND|0777);
	pd->mtime = dir->mtime;
	pd->length = dir->length;
	pd->uid = strdup(dir->uid);
	pd->gid = strdup(dir->gid);
	pd->offset = offset;

	return pd;
}

void
paqDirFree(PaqDir *pd)
{
	if(pd == nil)
		return;
	free(pd->name);
	free(pd->uid);
	free(pd->gid);
	free(pd);
}


void
writeHeader(char *label)
{
	PaqHeader hdr;
	uchar buf[HeaderSize];

	memset(&hdr, 0, sizeof(hdr));
	hdr.magic = HeaderMagic;
	hdr.version = Version;
	hdr.blocksize = blocksize;
	hdr.time = time(nil);
	strncpy(hdr.label, label, sizeof(hdr.label));
	hdr.label[sizeof(hdr.label)-1] = 0;
	putHeader(buf, &hdr);
	outWrite(buf, sizeof(buf));
}

void
writeTrailer(ulong root)
{
	PaqTrailer tlr;
	uchar buf[TrailerSize];

	memset(&tlr, 0, sizeof(tlr));
	tlr.magic = TrailerMagic;
	tlr.root = root;
	putTrailer(buf, &tlr);
	outWrite(buf, sizeof(buf));
}

ulong
writeBlock(uchar *b, int type)
{
	uchar *cb, *ob;
	int n;
	PaqBlock bh;
	uchar buf[BlockSize];
	ulong offset;

	offset = Boffset(out);

	bh.magic = BlockMagic;
	bh.size = blocksize;	
	bh.type = type;
	bh.encoding = NoEnc;
	bh.adler32 = adler32(0, b, blocksize);
	ob = b;

	if(!uflag) {
		cb = emallocz(blocksize);
		n = deflateblock(cb, blocksize, b, blocksize, 6, 0);
		if(n > 0 && n < blocksize) {
			bh.encoding = DeflateEnc;
			bh.size = n;
			ob = cb;
		}	
	}

	putBlock(buf, &bh);
	outWrite(buf, sizeof(buf));
	outWrite(ob, bh.size);
	
	if(ob != b)
		free(ob);
	return offset;
}


void
outWrite(void *buf, int n)
{
	if(Bwrite(out, buf, n) < n)
		sysfatal("write failed: %r");
	outdg = sha1((uchar*)buf, n, nil, outdg);
}

int
paqDirSize(PaqDir *d)
{
	return MinDirSize + strlen(d->name) + strlen(d->uid) + strlen(d->gid);
}

void
putHeader(uchar *p, PaqHeader *h)
{
	if(h->blocksize < 65536){
		putl(p, h->magic);
		puts(p+4, h->version);
		puts(p+6, h->blocksize);
	}else{
		assert(h->magic == HeaderMagic);
		puts(p, BigHeaderMagic);
		puts(p+2, h->version);
		putl(p+4, h->blocksize);
	}
	putl(p+8, h->time);
	memmove(p+12, h->label, sizeof(h->label));
}

void
putTrailer(uchar *p, PaqTrailer *h)
{
	putl(p, h->magic);
	putl(p+4, h->root);
	outdg = sha1(p, 8, p+8, outdg);
}

void
putBlock(uchar *p, PaqBlock *b)
{
	if(b->size < 65536){
		putl(p, b->magic);
		puts(p+4, b->size);
	}else{
		assert(b->magic == BlockMagic);
		puts(p, BigBlockMagic);
		putl(p+2, b->size);
	}
	p[6] = b->type;
	p[7] = b->encoding;
	putl(p+8, b->adler32);
}

void
putDir(uchar *p, PaqDir *d)
{
	uchar *q;

	puts(p, paqDirSize(d));
	putl(p+2, d->qid);	
	putl(p+6, d->mode);
	putl(p+10, d->mtime);
	putl(p+14, d->length);
	putl(p+18, d->offset);
	q = putstr(p+22, d->name);
	q = putstr(q, d->uid);
	q = putstr(q, d->gid);
	assert(q-p == paqDirSize(d));
}

void
putl(uchar *p, ulong v)
{
	p[0] = v>>24;
	p[1] = v>>16;
	p[2] = v>>8;
	p[3] = v;
}

void
puts(uchar *p, int v)
{
	assert(v < (1<<16));
	p[0] = v>>8;
	p[1] = v;
}

uchar *
putstr(uchar *p, char *s)
{
	int n = strlen(s);
	puts(p, n+2);
	memmove(p+2, s, n);
	return p+2+n;
}


void *
emallocz(int size)
{
	void *p;

	p = malloc(size);
	if(p == nil)
		sysfatal("malloc failed");
	memset(p, 0, size);
	return p;
}

void
warn(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
}
