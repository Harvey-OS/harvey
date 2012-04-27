#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/* little endian */
#define SHORT(p)	(((uchar*)(p))[0] | (((uchar*)(p))[1] << 8))
#define LONG(p)	((ulong)SHORT(p) |(((ulong)SHORT((p)+2)) << 16))

typedef struct Ofile	Ofile;
typedef struct Odir	Odir;

enum {
	/* special block map entries */
	Bspecial = 0xFFFFFFFD,
	Bendchain = 0xFFFFFFFE,
	Bunused = 0xFFFFFFFF,

	Blocksize = 0x200,

	Odirsize = 0x80,

	/* Odir types */
	Tstorage = 1,
	Tstream = 2,
	Troot = 5,
};

/*
 * the file consists of chains of blocks of size 0x200.
 * to find what block follows block n, you look at 
 * blockmap[n].  that block follows it unless it is Bspecial
 * or Bendchain.
 * 
 * it's like the MS-DOS file system allocation tables.
 */
struct Ofile {
	Biobuf *b;
	ulong nblock;
	ulong *blockmap;
	ulong rootblock;
	ulong smapblock;
	ulong *smallmap;
};

/* Odir headers are found in directory listings in the Olefile */
/* prev and next form a binary tree of directory entries */
struct Odir {
	Ofile *f;
	Rune name[32+1];
	uchar type;
	uchar isroot;
	ulong left;
	ulong right;
	ulong dir;
	ulong start;
	ulong size;
};

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	assert(v != nil);
	return v;
}

int
convM2OD(Odir *f, void *buf, int nbuf)
{
	int i;
	char *p;
	int len;

	if(nbuf < Odirsize)
		return -1;

	/*
	 * the short at 0x40 is the length of the name.
	 * when zero, it means there is no Odir here.
	 */
	p = buf;
	len = SHORT(p+0x40);
	if(len == 0)
		return 0;

	if(len > 32)	/* shouldn't happen */
		len = 32;

	for(i=0; i<len; i++)
		f->name[i] = SHORT(p+i*2);
	f->name[len] = 0;

	f->type = p[0x42];
	f->left = LONG(p+0x44);
	f->right = LONG(p+0x48);
	f->dir = LONG(p+0x4C);
	f->start = LONG(p+0x74);
	f->size = LONG(p+0x78);

	/* BUG: grab time in ms format from here */

	return 1;
}

int
oreadblock(Ofile *f, int block, ulong off, char *buf, int nbuf)
{
	int n;

	if(block < 0 || block >= f->nblock) {
		werrstr("attempt to read %x/%lux\n", block, f->nblock);
		return -1;
	}

	if(off >= Blocksize){
		print("offset too far into block\n");
		return 0;
	}

	if(off+nbuf > Blocksize)
		nbuf = Blocksize-off;

	/* blocks start numbering at -1 [sic] */
	off += (block+1)*Blocksize;

	if(Bseek(f->b, off, 0) != off){
		print("seek failed\n");
		return -1;
	}

	n = Bread(f->b, buf, nbuf);
	if(n < 0)
		print("Bread failed: %r");
	return n;
}

int
chainlen(Ofile *f, ulong start)
{
	int i;
	for(i=0; start < 0xFFFF0000; i++)
		start = f->blockmap[start];

	return i;
}

/*
 * read nbuf bytes starting at offset off from the 
 * chain whose first block is block.  the chain is linked
 * together via the blockmap as described above,
 * like the MS-DOS file allocation tables.
 */
int
oreadchain(Ofile *f, ulong block, int off, char *buf, int nbuf)
{
	int i;
	int offblock;

	offblock = off/Blocksize;
	for(i=0; i<offblock && block < 0xFFFF0000; i++)
		block = f->blockmap[block];
	return oreadblock(f, block, off%Blocksize, buf, nbuf);
}

int 
oreadfile(Odir *d, int off, char *buf, int nbuf)
{
	/*
	 * if d->size < 0x1000 then d->start refers
	 * to a small depot block, else a big one.
	 * if this is the root entry, it's a big one
	 * no matter what.
	 */

	if(off >= d->size)
		return 0;
	if(off+nbuf > d->size)
		nbuf = d->size-off;

	if(d->size >= 0x1000 
	|| memcmp(d->name, L"Root Entry", 11*sizeof(Rune)) == 0)
		return oreadchain(d->f, d->start, off, buf, nbuf);
	else {	/* small block */
		off += d->start*64;
		return oreadchain(d->f, d->f->smapblock, off, buf, nbuf);
	}
}

int
oreaddir(Ofile *f, int entry, Odir *d)
{
	char buf[Odirsize];

	if(oreadchain(f, f->rootblock, entry*Odirsize, buf, Odirsize) != Odirsize)
		return -1;

	d->f = f;
	return convM2OD(d, buf, Odirsize);
}

void
dumpdir(Ofile *f, ulong dnum)
{
	Odir d;

	if(oreaddir(f, dnum, &d) != 1) {
		fprint(2, "dumpdir %lux failed\n", dnum);
		return;
	}

	fprint(2, "%.8lux type %d size %lud l %.8lux r %.8lux d %.8lux (%S)\n", dnum, d.type, d.size, d.left, d.right, d.dir, d.name);
	if(d.left != (ulong)-1) 
		dumpdir(f, d.left);
	if(d.right != (ulong)-1)
		dumpdir(f, d.right);
	if(d.dir != (ulong)-1)
		dumpdir(f, d.dir);
}

Ofile*
oleopen(char *fn)
{
	int i, j, k, block;
	int ndepot;
	ulong u;
	Odir rootdir;
	ulong extrablock;
	uchar buf[Blocksize];

	Ofile *f;
	Biobuf *b;
	static char magic[] = {
		0xD0, 0xCF, 0x11, 0xE0,
		0xA1, 0xB1, 0x1A, 0xE1
	};

	b = Bopen(fn, OREAD);
	if(b == nil)
		return nil;

	/* the first bytes are magic */
	if(Bread(b, buf, sizeof magic) != sizeof magic
	|| memcmp(buf, magic, sizeof magic) != 0) {
		Bterm(b);
		werrstr("bad magic: not OLE file");
		return nil;
	}

	f = emalloc(sizeof *f);
	f->b = b;

	/*
	 * the header contains a list of depots, which are
	 * block maps.  we assimilate them into one large map,
	 * kept in main memory.
	 */
	Bseek(b, 0, 0);
	if(Bread(b, buf, Blocksize) != Blocksize) {
		Bterm(b);
		free(f);
		print("short read\n");
		return nil;
	}

	ndepot = LONG(buf+0x2C);
	f->nblock = ndepot*(Blocksize/4);
//	fprint(2, "ndepot = %d f->nblock = %lud\n", ndepot, f->nblock);
	f->rootblock = LONG(buf+0x30);
	f->smapblock = LONG(buf+0x3C);
	f->blockmap = emalloc(sizeof(f->blockmap[0])*f->nblock);
	extrablock = LONG(buf+0x44);

	u = 0;

	/* the big block map fills to the end of the first 512-byte block */
	for(i=0; i<ndepot && i<(0x200-0x4C)/4; i++) {
		if(Bseek(b, 0x4C+4*i, 0) != 0x4C+4*i
		|| Bread(b, buf, 4) != 4) {
			print("bseek %d fail\n", 0x4C+4*i);
			goto Die;
		}
		block = LONG(buf);
		if((ulong)block == Bendchain) {
			ndepot = i;
			f->nblock = ndepot*(Blocksize/4);
			break;
		}

		if(Bseek(b, (block+1)*Blocksize, 0) != (block+1)*Blocksize) {
			print("Xbseek %d fail\n", (block+1)*Blocksize);
			goto Die;
		}
		for(j=0; j<Blocksize/4; j++) {
			if(Bread(b, buf, 4) != 4) {
				print("Bread fail seek block %x, %d i %d ndepot %d\n", block, (block+1)*Blocksize, i, ndepot);
				goto Die;
			}
			f->blockmap[u++] = LONG(buf);
		}
	}
	/*
	 * if the first block can't hold it, it continues in the block at LONG(hdr+0x44).
	 * if that in turn is not big enough, there's a next block number at the end of 
	 * each block.
	 */
	while(i < ndepot) {
		for(k=0; k<(0x200-4)/4 && i<ndepot; i++, k++) {
			if(Bseek(b, 0x200+extrablock*Blocksize+4*i, 0) != 0x200+extrablock*0x200+4*i
			|| Bread(b, buf, 4) != 4) {
				print("bseek %d fail\n", 0x4C+4*i);
				goto Die;
			}
			block = LONG(buf);
			if((ulong)block == Bendchain) {
				ndepot = i;
				f->nblock = ndepot*(Blocksize/4);
				goto Break2;
			}

			if(Bseek(b, (block+1)*Blocksize, 0) != (block+1)*Blocksize) {
				print("Xbseek %d fail\n", (block+1)*Blocksize);
				goto Die;
			}
			for(j=0; j<Blocksize/4; j++) {
				if(Bread(b, buf, 4) != 4) {
					print("Bread fail seek block %x, %d i %d ndepot %d\n", block, (block+1)*Blocksize, i, ndepot);
					goto Die;
				}
				f->blockmap[u++] = LONG(buf);
			}
		}
		if(Bseek(b, 0x200+extrablock*Blocksize+Blocksize-4, 0) != 0x200+extrablock*Blocksize+Blocksize-4
		|| Bread(b, buf, 4) != 4) {
			print("bseek %d fail\n", 0x4C+4*i);
			goto Die;
		}
		extrablock = LONG(buf);
	}
Break2:;

	if(oreaddir(f, 0, &rootdir) <= 0){
		print("oreaddir could not read root\n");
		goto Die;
	}

	f->smapblock = rootdir.start;
	return f;

Die:
	Bterm(b);
	free(f->blockmap);
	free(f);
	return nil;
}

void
oleread(Req *r)
{
	Odir *d;
	char *p;
	int e, n;
	long c;
	vlong o;

	o = r->ifcall.offset;
	d = r->fid->file->aux;
	if(d == nil) {
		respond(r, "cannot happen");
		return;
	}

	c = r->ifcall.count;

	if(o >= d->size) {
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}

	if(o+c > d->size)
		c = d->size-o;

	/*
	 * oreadfile returns so little data, it will
	 * help to read as much as we can.
	 */
	e = c+o;
	n = 0;
	for(p=r->ofcall.data; o<e; o+=n, p+=n) {
		n = oreadfile(d, o, p, e-o);
		if(n <= 0)
			break;
	}

	if(n == -1 && o == r->ifcall.offset)
		respond(r, "error reading word file");
	else {
		r->ofcall.count = o - r->ifcall.offset;
		respond(r, nil);
	}
}

Odir*
copydir(Odir *d)
{
	Odir *e;

	e = emalloc(sizeof(*d));
	*e = *d;
	return e;
}
		
void
filldir(File *t, Ofile *f, int dnum, int nrecur)
{
	Odir d;
	int i;
	Rune rbuf[40];
	char buf[UTFmax*nelem(rbuf)];
	File *nt;

	if(dnum == 0xFFFFFFFF || oreaddir(f, dnum, &d) != 1)
		return;

	/*
	 * i hope there are no broken files with
	 * circular trees.  i hate infinite loops.
	 */
	if(nrecur > 100)
		sysfatal("tree too large in office file: probably circular");

	filldir(t, f, d.left, nrecur+1);

	/* add current tree entry */
	runestrecpy(rbuf, rbuf+sizeof rbuf, d.name);
	for(i=0; rbuf[i]; i++)
		if(rbuf[i] == L' ')
			rbuf[i] = L'␣';
		else if(rbuf[i] <= 0x20 || rbuf[i] == L'/' 
			|| (0x80 <= rbuf[i] && rbuf[i] <= 0x9F))
				rbuf[i] = ':';
	
	snprint(buf, sizeof buf, "%S", rbuf);

	if(d.dir == 0xFFFFFFFF) {
		/* make file */
		nt = createfile(t, buf, nil, 0444, nil);
		if(nt == nil)
			sysfatal("nt nil: create %s: %r", buf);
		nt->aux = copydir(&d);
		nt->length = d.size;
	} else /* make directory */
		nt = createfile(t, buf, nil, DMDIR|0777, nil);

	filldir(t, f, d.right, nrecur+1);

	if(d.dir != 0xFFFFFFFF)
		filldir(nt, f, d.dir, nrecur+1);

	closefile(nt);
}

Srv olesrv = {
	.read=	oleread,
};

void
main(int argc, char **argv)
{
	char *mtpt;
	Ofile *f;
	Odir d;

	mtpt = "/mnt/doc";
	ARGBEGIN{
	case 'm':
		mtpt = ARGF();
		break;
	
	default:
		goto Usage;
	}ARGEND

	if(argc != 1) {
	Usage:
		fprint(2, "usage: olefs file\n");
		exits("usage");
	}

	f = oleopen(argv[0]);
	if(f == nil) {
		print("error opening %s: %r\n", argv[0]);
		exits("open");
	}

//	dumpdir(f, 0);

	if(oreaddir(f, 0, &d) != 1) {
		fprint(2, "oreaddir error: %r\n");
		exits("oreaddir");
	}

	olesrv.tree = alloctree(nil, nil, DMDIR|0777, nil);
	filldir(olesrv.tree->root, f, d.dir, 0);
	postmountsrv(&olesrv, nil, mtpt, MREPL);
	exits(0);
}
