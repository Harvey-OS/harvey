#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"dosfs.h"

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
	chat("getclust addr %lld\n", (sector+dos->start)*(vlong)dos->sectsize);
	if((*dos->seek)(dos, (sector+dos->start)*(vlong)dos->sectsize) < 0){
		chat("can't seek block\n");
		return 0;
	}
	if((*dos->read)(dos, p->iobuf, size) != size){
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

	if((fp->attr & DDIR) == 0){
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
doswalk(Dosfile *file, char *name)
{
	Dosdir d;
	long n;

	if((file->attr & DDIR) == 0){
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
		if(d.attr & DVLABEL){
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

int
dosinit(Dos *dos)
{
	Clustbuf *p;
	Dosboot *b;
	int i;

	/* defaults till we know better */
	dos->sectsize = 512;
	dos->clustsize = 1;

	/* get first sector */
	p = getclust(dos, 0);
	if(p == 0){
		chat("can't read boot block\n");
		return -1;
	}

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
		print("dos file system has incorrect cluster size\n");
		for(i=0; i<3+8+2+1; i++)
			print(" %.2ux", p->iobuf[i]);
		print("\n");
		return -1;
	}

	/*
	 *  determine the systems' wondersous properties
	 */
	dos->fatbits = 0;
	dos->sectsize = GSHORT(b->sectsize);
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

	/*
	 *  set up the root
	 */
	dos->root.dos = dos;
	dos->root.pstart = dos->rootsize == 0 ? dos->rootclust : 0;
	dos->root.pcurrent = dos->root.lcurrent = 0;
	dos->root.offset = 0;
	dos->root.attr = DDIR;
	dos->root.length = dos->rootsize*sizeof(Dosdir);

	return 0;
}

static void
bootdump(Dosboot *b)
{
	if(chatty == 0)
		return;
	print("magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	print("version: \"%8.8s\"\n", (char*)b->version);
	print("sectsize: %d\n", GSHORT(b->sectsize));
	print("allocsize: %d\n", b->clustsize);
	print("nresrv: %d\n", GSHORT(b->nresrv));
	print("nfats: %d\n", b->nfats);
	print("rootsize: %d\n", GSHORT(b->rootsize));
	print("volsize: %d\n", GSHORT(b->volsize));
	print("mediadesc: 0x%2.2x\n", b->mediadesc);
	print("fatsize: %d\n", GSHORT(b->fatsize));
	print("trksize: %d\n", GSHORT(b->trksize));
	print("nheads: %d\n", GSHORT(b->nheads));
	print("nhidden: %d\n", GLONG(b->nhidden));
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
 *  grab next element from a path, return the pointer to unprocessed portion of
 *  path.
 */
static char *
nextelem(char *path, char *elem)
{
	int i;

	while(*path == '/')
		path++;
	if(*path==0 || *path==' ')
		return 0;
	for(i=0; *path!='\0' && *path!='/' && *path!=' '; i++){
		if(i==28){
			print("name component too long\n");
			return 0;
		}
		*elem++ = *path++;
	}
	*elem = '\0';
	return path;
}

int
dosstat(Dos *dos, char *path, Dosfile *f)
{
	char element[NAMELEN];

	*f = dos->root;
	while(path = nextelem(path, element)){
		switch(doswalk(f, element)){
		case -1:
			return -1;
		case 0:
			return 0;
		}
	}
	return 1;
}

/*
 *  read in a segment
 */
long
dosreadseg(Dosfile *fp, void *va, long len)
{
	char *a;
	long n, sofar;

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

/*
 *  boot
 */
int
dosboot(Dos *dos, char *path, Boot *b)
{
	Dosfile file;
	long n;
	static char buf[8192];

	switch(dosstat(dos, path, &file)){
	case -1:
		print("error walking to %s\n", path);
		return -1;
	case 0:
		print("%s not found\n", path);
		return -1;
	case 1:
		print("found %8.8s.%3.3s attr 0x%ux start 0x%lux len %lud\n", file.name,
			file.ext, file.attr, file.pstart, file.length);
		break;
	}

	while((n = dosreadseg(&file, buf, sizeof buf)) > 0) {
		if(bootpass(b, buf, n) != MORE)
			break;
	}

	bootpass(b, nil, 0);	/* tries boot */
	return -1;
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
