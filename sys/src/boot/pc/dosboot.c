#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"fs.h"

struct Dosboot{
	uchar	magic[3];
	uchar	version[8];
	uchar	sectsize[2];
	uchar	clustsize;
	uchar	nresrv[2];
	uchar	nfats;
	uchar	rootsize[2];
	uchar	volsize[2];
	uchar	mediadesc;
	uchar	fatsize[2];
	uchar	trksize[2];
	uchar	nheads[2];
	uchar	nhidden[4];
	uchar	bigvolsize[4];
/* fat 32 */
	uchar	bigfatsize[4];
	uchar	extflags[2];
	uchar	fsversion[2];
	uchar	rootdirstartclust[4];
	uchar	fsinfosect[2];
	uchar	backupbootsect[2];
/* ???
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	reserved1[8];
*/
};

struct Dosdir{
	uchar	name[8];
	uchar	ext[3];
	uchar	attr;
	uchar	lowercase;
	uchar	hundredth;
	uchar	ctime[2];
	uchar	cdate[2];
	uchar	adate[2];
	uchar	highstart[2];
	uchar	mtime[2];
	uchar	mdate[2];
	uchar	start[2];
	uchar	length[4];
};

#define	DOSRONLY	0x01
#define	DOSHIDDEN	0x02
#define	DOSSYSTEM	0x04
#define	DOSVLABEL	0x08
#define	DOSDIR	0x10
#define	DOSARCH	0x20

/*
 *  predeclared
 */
static void	bootdump(Dosboot*);
static void	setname(Dosfile*, char*);

/*
 *  debugging
 */
#define chatty	0
#define chat	if(chatty)print

/*
 *  block io buffers
 */
enum
{
	Nbio=	16,
};
typedef struct	Clustbuf	Clustbuf;
struct Clustbuf
{
	int	age;
	long	sector;
	uchar	*iobuf;
	Dos	*dos;
	int	size;
};
Clustbuf	bio[Nbio];

/*
 *  get an io block from an io buffer
 */
Clustbuf*
getclust(Dos *dos, long sector)
{
	Fs *fs;
	Clustbuf *p, *oldest;
	int size;

	chat("getclust @ %ld\n", sector);

	/*
	 *  if we have it, just return it
	 */
	for(p = bio; p < &bio[Nbio]; p++){
		if(sector == p->sector && dos == p->dos){
			p->age = m->ticks;
			chat("getclust %ld in cache\n", sector);
			return p;
		}
	}

	/*
	 *  otherwise, reuse the oldest entry
	 */
	oldest = bio;
	for(p = &bio[1]; p < &bio[Nbio]; p++){
		if(p->age <= oldest->age)
			oldest = p;
	}
	p = oldest;

	/*
	 *  make sure the buffer is big enough
	 */
	size = dos->clustsize*dos->sectsize;
	if(p->iobuf==0 || p->size < size)
		p->iobuf = ialloc(size, 0);
	p->size = size;

	/*
	 *  read in the cluster
	 */
	fs = (Fs*)dos;
	chat("getclust addr %lud %p %p %p\n", (ulong)((sector+dos->start)*(vlong)dos->sectsize),
		fs, fs->diskseek, fs->diskread);
	if(fs->diskseek(fs, (sector+dos->start)*(vlong)dos->sectsize) < 0){
		chat("can't seek block\n");
		return 0;
	}
	if(fs->diskread(fs, p->iobuf, size) != size){
		chat("can't read block\n");
		return 0;
	}

	p->age = m->ticks;
	p->dos = dos;
	p->sector = sector;
	chat("getclust %ld read\n", sector);
	return p;
}

/*
 *  walk the fat one level ( n is a current cluster number ).
 *  return the new cluster number or -1 if no more.
 */
static long
fatwalk(Dos *dos, int n)
{
	ulong k, sect;
	Clustbuf *p;
	int o;

	chat("fatwalk %d\n", n);

	if(n < 2 || n >= dos->fatclusters)
		return -1;

	switch(dos->fatbits){
	case 12:
		k = (3*n)/2; break;
	case 16:
		k = 2*n; break;
	case 32:
		k = 4*n; break;
	default:
		return -1;
	}
	if(k >= dos->fatsize*dos->sectsize)
		panic("getfat");

	sect = (k/(dos->sectsize*dos->clustsize))*dos->clustsize + dos->fataddr;
	o = k%(dos->sectsize*dos->clustsize);
	p = getclust(dos, sect);
	k = p->iobuf[o++];
	if(o >= dos->sectsize*dos->clustsize){
		p = getclust(dos, sect+dos->clustsize);
		o = 0;
	}
	k |= p->iobuf[o++]<<8;
	if(dos->fatbits == 12){
		if(n&1)
			k >>= 4;
		else
			k &= 0xfff;
		if(k >= 0xff8)
			k = -1;
	}
	else if (dos->fatbits == 32){
		if(o >= dos->sectsize*dos->clustsize){
			p = getclust(dos, sect+dos->clustsize);
			o = 0;
		}
		k |= p->iobuf[o++]<<16;
		k |= p->iobuf[o]<<24;
		if (k >= 0xfffffff8)
			k = -1;
	}
	else
		k = k < 0xfff8 ? k : -1;
	chat("fatwalk %d -> %lud\n", n, k);
	return k;
}

/*
 *  map a file's logical cluster address to a physical sector address
 */
static long
fileaddr(Dosfile *fp, long ltarget)
{
	Dos *dos = fp->dos;
	long l;
	long p;

	chat("fileaddr %8.8s %ld\n", fp->name, ltarget);
	/*
	 *  root directory is contiguous and easy (unless FAT32)
	 */
	if(fp->pstart == 0 && dos->rootsize != 0) {
		if(ltarget*dos->sectsize*dos->clustsize >= dos->rootsize*sizeof(Dosdir))
			return -1;
		l = dos->rootaddr + ltarget*dos->clustsize;
		chat("fileaddr %ld -> %ld\n", ltarget, l);
		return l;
	}

	/*
	 *  anything else requires a walk through the fat
	 */
	if(ltarget >= fp->lcurrent && fp->pcurrent){
		/* start at the currrent point */
		l = fp->lcurrent;
		p = fp->pcurrent;
	} else {
		/* go back to the beginning */
		l = 0;
		p = fp->pstart;
	}
	while(l != ltarget){
		/* walk the fat */
		p = fatwalk(dos, p);
		if(p < 0)
			return -1;
		l++;
	}
	fp->lcurrent = l;
	fp->pcurrent = p;

	/*
	 *  clusters start at 2 instead of 0 (why? - presotto)
	 */
	l =  dos->dataaddr + (p-2)*dos->clustsize;
	chat("fileaddr %ld -> %ld\n", ltarget, l);
	return l;
}

/*
 *  read from a dos file
 */
long
dosread(Dosfile *fp, void *a, long n)
{
	long addr;
	long rv;
	int i;
	int off;
	Clustbuf *p;
	uchar *from, *to;

	if((fp->attr & DOSDIR) == 0){
		if(fp->offset >= fp->length)
			return 0;
		if(fp->offset+n > fp->length)
			n = fp->length - fp->offset;
	}

	to = a;
	for(rv = 0; rv < n; rv+=i){
		/*
		 *  read the cluster
		 */
		addr = fileaddr(fp, fp->offset/fp->dos->clustbytes);
		if(addr < 0)
			return -1;
		p = getclust(fp->dos, addr);
		if(p == 0)
			return -1;

		/*
		 *  copy the bytes we need
		 */
		off = fp->offset % fp->dos->clustbytes;
		from = &p->iobuf[off];
		i = n - rv;
		if(i > fp->dos->clustbytes - off)
			i = fp->dos->clustbytes - off;
		memmove(to, from, i);
		to += i;
		fp->offset += i;
	}

	return rv;
}

/*
 *  walk a directory returns
 * 	-1 if something went wrong
 *	 0 if not found
 *	 1 if found
 */
int
doswalk(File *f, char *name)
{
	Dosdir d;
	long n;
	Dosfile *file;

	chat("doswalk %s\n", name);

	file = &f->dos;

	if((file->attr & DOSDIR) == 0){
		chat("walking non-directory!\n");
		return -1;
	}

	setname(file, name);

	file->offset = 0;	/* start at the beginning */
	while((n = dosread(file, &d, sizeof(d))) == sizeof(d)){
		chat("comparing to %8.8s.%3.3s\n", (char*)d.name, (char*)d.ext);
		if(memcmp(file->name, d.name, sizeof(d.name)) != 0)
			continue;
		if(memcmp(file->ext, d.ext, sizeof(d.ext)) != 0)
			continue;
		if(d.attr & DOSVLABEL){
			chat("%8.8s.%3.3s is a LABEL\n", (char*)d.name, (char*)d.ext);
			continue;
		}
		file->attr = d.attr;
		file->pstart = GSHORT(d.start);
		if (file->dos->fatbits == 32)
			file->pstart |= GSHORT(d.highstart) << 16;
		file->length = GLONG(d.length);
		file->pcurrent = 0;
		file->lcurrent = 0;
		file->offset = 0;
		return 1;
	}
	return n >= 0 ? 0 : -1;
}

/*
 *  instructions that boot blocks can start with
 */
#define	JMPSHORT	0xeb
#define JMPNEAR		0xe9

/*
 *  read in a segment
 */
long
dosreadseg(File *f, void *va, long len)
{
	char *a;
	long n, sofar;
	Dosfile *fp;

	fp = &f->dos;
	a = va;
	for(sofar = 0; sofar < len; sofar += n){
		n = 8*1024;
		if(len - sofar < n)
			n = len - sofar;
		n = dosread(fp, a + sofar, n);
		if(n <= 0)
			break;
		print(".");
	}
	return sofar;
}

int
dosinit(Fs *fs)
{
	Clustbuf *p;
	Dosboot *b;
	int i;
	Dos *dos;
	Dosfile *root;

chat("dosinit0 %p %p %p\n", fs, fs->diskseek, fs->diskread);

	dos = &fs->dos;
	/* defaults till we know better */
	dos->sectsize = 512;
	dos->clustsize = 1;

	/* get first sector */
	p = getclust(dos, 0);
	if(p == 0){
		chat("can't read boot block\n");
		return -1;
	}

chat("dosinit0a\n");

	p->dos = 0;
	b = (Dosboot *)p->iobuf;
	if(b->magic[0] != JMPNEAR && (b->magic[0] != JMPSHORT || b->magic[2] != 0x90)){
		chat("no dos file system %x %x %x %x\n",
			b->magic[0], b->magic[1], b->magic[2], b->magic[3]);
		return -1;
	}

	if(chatty)
		bootdump(b);

	if(b->clustsize == 0) {
unreasonable:
		if(chatty){
			print("unreasonable FAT BPB: ");
			for(i=0; i<3+8+2+1; i++)
				print(" %.2ux", p->iobuf[i]);
			print("\n");
		}
		return -1;
	}

chat("dosinit1\n");

	/*
	 * Determine the systems' wondrous properties.
	 * There are heuristics here, but there's no real way
	 * of knowing if this is a reasonable FAT.
	 */
	dos->fatbits = 0;
	dos->sectsize = GSHORT(b->sectsize);
	if(dos->sectsize & 0xFF)
		goto unreasonable;
	dos->clustsize = b->clustsize;
	dos->clustbytes = dos->sectsize*dos->clustsize;
	dos->nresrv = GSHORT(b->nresrv);
	dos->nfats = b->nfats;
	dos->fatsize = GSHORT(b->fatsize);
	dos->rootsize = GSHORT(b->rootsize);
	dos->volsize = GSHORT(b->volsize);
	if(dos->volsize == 0)
		dos->volsize = GLONG(b->bigvolsize);
	dos->mediadesc = b->mediadesc;
	if(dos->fatsize == 0) {
		chat("fat32\n");
		dos->rootsize = 0;
		dos->fatsize = GLONG(b->bigfatsize);
		dos->fatbits = 32;
	}
	dos->fataddr = dos->nresrv;
	if (dos->rootsize == 0) {
		dos->rootaddr = 0;
		dos->rootclust = GLONG(b->rootdirstartclust);
		dos->dataaddr = dos->fataddr + dos->nfats*dos->fatsize;
	} else {
		dos->rootaddr = dos->fataddr + dos->nfats*dos->fatsize;
		i = dos->rootsize*sizeof(Dosdir) + dos->sectsize - 1;
		i = i/dos->sectsize;
		dos->dataaddr = dos->rootaddr + i;
	}
	dos->fatclusters = 2+(dos->volsize - dos->dataaddr)/dos->clustsize;
	if(dos->fatbits != 32) {
		if(dos->fatclusters < 4087)
			dos->fatbits = 12;
		else
			dos->fatbits = 16;
	}
	dos->freeptr = 2;

	if(dos->clustbytes < 512 || dos->clustbytes > 64*1024)
		goto unreasonable;

chat("dosinit2\n");

	/*
	 *  set up the root
	 */

	fs->root.fs = fs;
	root = &fs->root.dos;
	root->dos = dos;
	root->pstart = dos->rootsize == 0 ? dos->rootclust : 0;
	root->pcurrent = root->lcurrent = 0;
	root->offset = 0;
	root->attr = DOSDIR;
	root->length = dos->rootsize*sizeof(Dosdir);

chat("dosinit3\n");

	fs->read = dosreadseg;
	fs->walk = doswalk;
	return 0;
}

static void
bootdump(Dosboot *b)
{
	if(chatty == 0)
		return;
	print("magic: 0x%2.2x 0x%2.2x 0x%2.2x ",
		b->magic[0], b->magic[1], b->magic[2]);
	print("version: \"%8.8s\"\n", (char*)b->version);
	print("sectsize: %d ", GSHORT(b->sectsize));
	print("allocsize: %d ", b->clustsize);
	print("nresrv: %d ", GSHORT(b->nresrv));
	print("nfats: %d\n", b->nfats);
	print("rootsize: %d ", GSHORT(b->rootsize));
	print("volsize: %d ", GSHORT(b->volsize));
	print("mediadesc: 0x%2.2x\n", b->mediadesc);
	print("fatsize: %d ", GSHORT(b->fatsize));
	print("trksize: %d ", GSHORT(b->trksize));
	print("nheads: %d ", GSHORT(b->nheads));
	print("nhidden: %d ", GLONG(b->nhidden));
	print("bigvolsize: %d\n", GLONG(b->bigvolsize));
/*
	print("driveno: %d\n", b->driveno);
	print("reserved0: 0x%2.2x\n", b->reserved0);
	print("bootsig: 0x%2.2x\n", b->bootsig);
	print("volid: 0x%8.8x\n", GLONG(b->volid));
	print("label: \"%11.11s\"\n", b->label);
*/
}


/*
 *  set up a dos file name
 */
static void
setname(Dosfile *fp, char *from)
{
	char *to;

	to = fp->name;
	for(; *from && to-fp->name < 8; from++, to++){
		if(*from == '.'){
			from++;
			break;
		}
		if(*from >= 'a' && *from <= 'z')
			*to = *from + 'A' - 'a';
		else
			*to = *from;
	}
	while(to - fp->name < 8)
		*to++ = ' ';
	
	/* from might be 12345678.123: don't save the '.' in ext */
	if(*from == '.')
		from++;

	to = fp->ext;
	for(; *from && to-fp->ext < 3; from++, to++){
		if(*from >= 'a' && *from <= 'z')
			*to = *from + 'A' - 'a';
		else
			*to = *from;
	}
	while(to-fp->ext < 3)
		*to++ = ' ';

	chat("name is %8.8s.%3.3s\n", fp->name, fp->ext);
}
