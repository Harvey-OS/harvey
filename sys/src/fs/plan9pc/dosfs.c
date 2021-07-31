#include "all.h"
#include "io.h"
#include "mem.h"

#include "dosfs.h"

#define	GSHORT(p) (((p)[1]<<8)|(p)[0])
#define	GLONG(p) ((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p) (((p)[0]<<8)|(p)[1])
#define	GLLONG(p) ((GLSHORT(p)<<16)|GLSHORT(p+2))

/*
 *  debugging
 */
#define chatty	0
#define	chat	if(chatty)print

/*
 *  block io buffers
 */
typedef struct Clustbuf	Clustbuf;

struct Clustbuf
{
	int	flags;
	int	age;
	long	sector;
	uchar *	iobuf;
	Dos *	dos;
	int	size;
	int	bufsize;
};

enum
{
	Nbio=	16,
	LOCKED=	1,
	MOD=	2,
	IMMED=	4,
};

static void	puttime(Dosdir*);

static Clustbuf	bio[Nbio];

/*
 *  write an io buffer and update its flags
 */
static void
writeclust(Clustbuf *p)
{
	Dos *dos;
	int addr;

	dos = p->dos;
	addr = (p->sector+dos->start)*dos->sectbytes;
	chat("writeclust @ %ld addr %d...", p->sector, addr);
	if((*dos->seek)(dos->dev, addr) < 0)
		panic("writeclust: seek");
	if((*dos->write)(dos->dev, p->iobuf, p->size) != p->size)
		panic("writeclust: write");
	p->flags &= ~(MOD|IMMED);
	chat("OK\n");
}

/*
 *  write any dirty buffers
 */
static void
syncclust(void)
{
	Clustbuf *p;

	for(p = bio; p < &bio[Nbio]; p++){
		if(p->flags & LOCKED)
			panic("syncclust");
		if(p->flags & MOD)
			writeclust(p);
	}
}

/*
 *  get an io buffer, possibly with valid data
 */
static Clustbuf*
getclust0(Dos *dos, long sector)
{
	Clustbuf *p, *oldest;

	chat("getclust0 @ %ld\n", sector);

	/*
	 *  if we have it, just return it
	 *  otherwise, reuse the oldest unlocked entry
	 */
	oldest = 0;
	for(p = bio; p < &bio[Nbio]; p++){
		if(sector == p->sector && dos == p->dos){
			if(p->flags & LOCKED)
				panic("getclust0 locked");
			chat("getclust0 %ld in cache\n", sector);
			p->flags |= LOCKED;
			return p;
		}
		if(p->flags & LOCKED)
			continue;
		if(oldest == 0 || p->age <= oldest->age)
			oldest = p;
	}
	p = oldest;
	if(p == 0)
		panic("getclust0 all locked");
	p->flags |= LOCKED;
	if(p->flags & MOD)
		writeclust(p);

	/*
	 *  make sure the buffer is big enough
	 */
	if(p->iobuf==0 || p->bufsize < dos->clustbytes){
		p->bufsize = dos->clustbytes;
		p->iobuf = ialloc(p->bufsize, 0);
	}
	if(sector >= dos->dataaddr)
		p->size = dos->clustbytes;
	else
		p->size = dos->sectbytes;
	p->dos = 0;	/* make it invalid */
	return p;
}

/*
 *  get an io block from an io buffer
 */
static Clustbuf*
getclust(Dos *dos, long sector)
{
	Clustbuf *p;
	int addr;

	p = getclust0(dos, sector);
	if(p->dos){
		p->age = MACHP(0)->ticks;
		return p;
	}
	addr = (sector+dos->start)*dos->sectbytes;
	chat("getclust seek addr %d\n", addr);
	if((*dos->seek)(dos->dev, addr) < 0){
		chat("can't seek block\n");
		return 0;
	}
	chat("getclust read addr %d\n", addr);
	if((*dos->read)(dos->dev, p->iobuf, p->size) != p->size){
		chat("can't read block\n");
		return 0;
	}

	p->age = MACHP(0)->ticks;
	p->dos = dos;
	p->sector = sector;
	chat("getclust %ld read\n", sector);
	return p;
}

/*
 *  get an io block from an io buffer;
 *  any current data is discarded.
 */
static Clustbuf*
getclustz(Dos *dos, long sector)
{
	Clustbuf *p;

	p = getclust0(dos, sector);
	p->age = MACHP(0)->ticks;
	p->dos = dos;
	p->sector = sector;
	memset(p->iobuf, 0, p->size);
	p->flags |= MOD;
	chat("getclustz %ld\n", sector);
	return p;
}

/*
 *  release an io buffer
 */
static void
putclust(Clustbuf *p)
{
	if(!(p->flags & LOCKED))
		panic("putclust lock");
	if((p->flags & (MOD|IMMED)) == (MOD|IMMED))
		writeclust(p);
	p->flags &= ~LOCKED;
	chat("putclust @ sector %ld...", p->sector);
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
	default:
		return -1;
	}
	if(k >= dos->fatbytes)
		panic("getfat");

	sect = k/dos->sectbytes + dos->fataddr;
	o = k%dos->sectbytes;
	p = getclust(dos, sect);
	k = p->iobuf[o++];
	if(o >= dos->sectbytes){
		putclust(p);
		p = getclust(dos, sect+1);
		o = 0;
	}
	k |= p->iobuf[o]<<8;
	putclust(p);
	if(dos->fatbits == 12){
		if(n&1)
			k >>= 4;
		else
			k &= 0xfff;
		if(k >= 0xff8)
			k |= 0xf000;
	}
	k = k < 0xfff8 ? k : -1;
	chat("fatwalk %d -> %lud\n", n, k);
	return k;
}

/*
 *  write a value into each copy of the fat.
 */
static void
fatwrite(Dos *dos, int n, int val)
{
	ulong k, sect;
	Clustbuf *p;
	int i, o;

	chat("fatwrite %d %d...", n, val);

	if(n < 2 || n >= dos->fatclusters)
		panic("fatwrite n");

	switch(dos->fatbits){
	case 12:
		k = (3*n)/2; break;
	case 16:
		k = 2*n; break;
	default:
		panic("fatwrite fatbits");
		return;
	}
	if(k >= dos->fatbytes)
		panic("fatwrite k");

	for(i=0; i<dos->nfats; i++, k+=dos->fatbytes){
		sect = k/dos->sectbytes + dos->fataddr;
		o = k%dos->sectbytes;
		p = getclust(dos, sect);
		if(p == 0)
			panic("fatwrite getclust");
		switch(dos->fatbits){
		case 12:
			if(n&1){
				p->iobuf[o] &= 0x0f;
				p->iobuf[o++] |= val<<4;
			}else
				p->iobuf[o++] = val;
			if(o >= dos->sectbytes){
				p->flags |= MOD;
				putclust(p);
				p = getclust(dos, sect+1);
				if(p == 0)
					panic("fatwrite getclust");
				o = 0;
			}
			if(n&1)
				p->iobuf[o] = val>>4;
			else{
				p->iobuf[o] &= 0xf0;
				p->iobuf[o] |= (val>>8)&0x0f;
			}
			break;
		case 16:
			p->iobuf[o++] = val;
			p->iobuf[o] = val>>8;
			break;
		}
		p->flags |= MOD;
		putclust(p);
	}
	chat("OK\n");
}

/*
 *  allocate a free cluster from the fat.
 */
static int
fatalloc(Dos *dos)
{
	Clustbuf *p;
	int n;

	n = dos->freeptr;
	for(;;){
		if(fatwalk(dos, n) == 0)
			break;
		if(++n >= dos->fatclusters)
			n = 2;
		if(n == dos->freeptr)
			return -1;
	}
	dos->freeptr = n+1;
	if(dos->freeptr >= dos->fatclusters)
		dos->freeptr = 2;
	fatwrite(dos, n, 0xffff);
	p = getclustz(dos, dos->dataaddr + (n-2)*dos->clustsize);
	putclust(p);
	return n;
}

/*
 *  map a file's logical sector address to a physical sector address
 */
static long
fileaddr(Dosfile *fp, long ltarget, Clustbuf *pdir)
{
	Dos *dos = fp->dos;
	Dosdir *dp;
	long p;

	chat("fileaddr %8.8s %ld\n", fp->name, ltarget);
	/*
	 *  root directory is contiguous and easy
	 */
	if(fp->pdir == 0){
		if(ltarget*dos->sectbytes >= dos->rootsize*sizeof(Dosdir))
			return -1;
		p = dos->rootaddr + ltarget;
		chat("fileaddr %ld -> %ld\n", ltarget, p);
		return p;
	}
	if(fp->pstart == 0){	/* empty file */
		if(!pdir)
			return -1;
		p = fatalloc(dos);
		if(p <= 0)
			return -1;
		chat("fileaddr initial alloc %ld\n", p);
		dp = (Dosdir *)(pdir->iobuf + fp->odir);
		puttime(dp);
		dp->start[0] = p;
		dp->start[1] = p>>8;
		pdir->flags |= MOD;
		fp->pstart = p;
		fp->pcurrent = p;
		fp->lcurrent = 0;
	}
	/*
	 *  anything else requires a walk through the fat
	 *  [lp]current will point to the last cluster if we run off the end
	 */
	ltarget /= dos->clustsize;
	if(fp->pcurrent == 0 || fp->lcurrent > ltarget){
		/* go back to the beginning */
		fp->lcurrent = 0;
		fp->pcurrent = fp->pstart;
	}
	while(fp->lcurrent < ltarget){
		/* walk the fat */
		p = fatwalk(dos, fp->pcurrent);
		if(p < 0){
			if(!pdir)
				return -1;
			p = fatalloc(dos);
			if(p < 0){
				print("file system full\n");
				return -1;
			}
			fatwrite(dos, fp->pcurrent, p);
		}
		fp->pcurrent = p;
		++fp->lcurrent;
	}

	/*
	 *  clusters start at 2 instead of 0 (why? - presotto)
	 */
	p = dos->dataaddr + (fp->pcurrent-2)*dos->clustsize;
	chat("fileaddr %ld -> %ld\n", ltarget, p);
	return p;
}

/*
 *  set up a dos file name
 */
static void
setname(char *name, char *ext, char *from)
{
	char *to;

	memset(name, ' ', 8);
	memset(ext, ' ', 3);

	to = name;
	for(; *from && to-name < 8; from++, to++){
		if(*from == '.'){
			from++;
			break;
		}
		if(*from >= 'a' && *from <= 'z')
			*to = *from + 'A' - 'a';
		else
			*to = *from;
	}
	to = ext;
	for(; *from && to-ext < 3; from++, to++){
		if(*from >= 'a' && *from <= 'z')
			*to = *from + 'A' - 'a';
		else
			*to = *from;
	}

	chat("name is %8.8s %3.3s\n", name, ext);
}

/*
 *  walk a directory returns
 * 	-1 if something went wrong
 *	 0 if not found
 *	 1 if found
 */
static int
doswalk(Dosfile *fp, char *name)
{
	char dname[8], dext[3];
	Clustbuf *p;
	Dosdir *dp;
	long o, addr;

	if((fp->attr & DOSDIR) == 0){
		chat("walking non-directory!\n");
		return -1;
	}

	setname(dname, dext, name);

	fp->offset = 0;	/* start at the beginning */
	for(;;){
		addr = fileaddr(fp, fp->offset/fp->dos->sectbytes, 0);
		if(addr < 0)
			return 0;
		p = getclust(fp->dos, addr);
		if(p == 0)
			return -1;
		for(o=0; o<p->size; o += sizeof(Dosdir)){
			dp = (Dosdir *)(p->iobuf + o);
			chat("comparing to %8.8s.%3.3s\n", (char*)dp->name, (char*)dp->ext);
			if(memcmp(dname, dp->name, sizeof(dp->name)) != 0)
				continue;
			if(memcmp(dext, dp->ext, sizeof(dp->ext)) == 0)
				goto Found;
		}
		fp->offset += p->size;
		putclust(p);
	}
	panic("doswalk");
Found:
	fp->pdir = p->sector;
	fp->odir = o;
	putclust(p);
	memmove(fp->name, dname, sizeof(fp->name));
	memmove(fp->ext, dext, sizeof(fp->ext));
	fp->attr = dp->attr;
	fp->length = GLONG(dp->length);
	fp->pstart = GSHORT(dp->start);
	fp->pcurrent = 0;
	fp->lcurrent = 0;
	fp->offset = 0;
	return 1;
}

static void
bootdump(Dosboot *b)
{
	if(chatty == 0)
		return;
	print("magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	print("version: \"%8.8s\"\n", (char*)b->version);
	print("sectbytes: %d\n", GSHORT(b->sectbytes));
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
	print("driveno: %d\n", b->driveno);
	print("reserved0: 0x%2.2x\n", b->reserved0);
	print("bootsig: 0x%2.2x\n", b->bootsig);
	print("volid: 0x%8.8x\n", GLONG(b->volid));
	print("label: \"%11.11s\"\n", (char*)b->label);
}

/*
 *  instructions that boot blocks can start with
 */
#define	JMPSHORT	0xeb
#define JMPNEAR		0xe9

/*
 *  read dos file system properties
 */
int
dosinit(Dos *dos)
{
	Clustbuf *p;
	Dospart *dp;
	Dosboot *b;
	int i;


	/* defaults till we know better */
	dos->start = 0;
	dos->sectbytes = 512;
	dos->clustsize = 1;
	dos->clustbytes = 512;

	/* get first sector */
	p = getclust(dos, 0);
	if(p == 0){
		chat("can't read boot block\n");
		return -1;
	}
	p->dos = 0;

	/* if a hard disk format, look for an active partition */
	b = (Dosboot *)p->iobuf;
	if(b->magic[0] != JMPNEAR && (b->magic[0] != JMPSHORT || b->magic[2] != 0x90)){
		if(p->iobuf[0x1fe] != 0x55 || p->iobuf[0x1ff] != 0xaa){
			print("no dos file system or partition table\n");
			putclust(p);
			return -1;
		}
		dp = (Dospart*)&p->iobuf[0x1be];
		for(i = 0; i < 4; i++, dp++)
			if(dp->type && dp->flag == 0x80)
				break;
		if(i == 4){
			putclust(p);
			return -1;
		}
		dos->start = GLONG(dp->start);
		putclust(p);
		p = getclust(dos, 0);
		if(p == 0){
			chat("can't read boot block\n");
			putclust(p);
			return -1;
		}
		p->dos = 0;
	}

	b = (Dosboot *)p->iobuf;
	if(b->magic[0] != JMPNEAR && (b->magic[0] != JMPSHORT || b->magic[2] != 0x90)){
		print("no dos file system\n");
		putclust(p);
		return -1;
	}

	if(chatty)
		bootdump(b);/**/

	/*
	 *  determine the systems' wondersous properties
	 */
	dos->sectbytes = GSHORT(b->sectbytes);
	dos->clustsize = b->clustsize;
	dos->clustbytes = dos->sectbytes*dos->clustsize;
	dos->nresrv = GSHORT(b->nresrv);
	dos->nfats = b->nfats;
	dos->rootsize = GSHORT(b->rootsize);
	dos->volsize = GSHORT(b->volsize);
	if(dos->volsize == 0)
		dos->volsize = GLONG(b->bigvolsize);
	dos->mediadesc = b->mediadesc;
	dos->fatsize = GSHORT(b->fatsize);
	dos->fatbytes = dos->sectbytes*dos->fatsize;
	dos->fataddr = dos->nresrv;
	dos->rootaddr = dos->fataddr + dos->nfats*dos->fatsize;
	i = dos->rootsize*sizeof(Dosdir) + dos->sectbytes - 1;
	i = i/dos->sectbytes;
	dos->dataaddr = dos->rootaddr + i;
	dos->fatclusters = 2+(dos->volsize - dos->dataaddr)/dos->clustsize;
	if(dos->fatclusters < 4087)
		dos->fatbits = 12;
	else
		dos->fatbits = 16;
	dos->freeptr = 2;
	putclust(p);

	/*
	 *  set up the root
	 */
	dos->root.dos = dos;
	dos->root.pdir = 0;
	dos->root.odir = 0;
	memmove(dos->root.name, "<root>  ", 8);
	memmove(dos->root.ext, "   ", 3);
	dos->root.attr = DOSDIR;
	dos->root.length = dos->rootsize*sizeof(Dosdir);
	dos->root.pstart = 0;
	dos->root.pcurrent = dos->root.lcurrent = 0;
	dos->root.offset = 0;

	syncclust();
	return 0;
}

static char *
nextelem(char *path, char *elem)
{
	int i;

	while(*path == '/')
		path++;
	if(*path==0 || *path==' ')
		return 0;
	for(i=0; *path && *path != '/' && *path != ' '; i++){
		if(i >= NAMELEN){
			print("name component too long\n");
			return 0;
		}
		*elem++ = *path++;
	}
	*elem = 0;
	return path;
}

static void
puttime(Dosdir *d)
{
	ulong secs;
	Rtc rtc;
	ushort x;

	secs = rtctime();
	sec2rtc(secs, &rtc);
	x = (rtc.hour<<11) | (rtc.min<<5) | (rtc.sec>>1);
	d->time[0] = x;
	d->time[1] = x>>8;
	x = ((rtc.year-80)<<9) | ((rtc.mon+1)<<5) | rtc.mday;
	d->date[0] = x;
	d->date[1] = x>>8;
}

Dosfile*
dosopen(Dos *dos, char *path, Dosfile *fp)
{
	char element[NAMELEN];

	*fp = dos->root;
	while(path = nextelem(path, element)){
		switch(doswalk(fp, element)){
		case -1:
			print("error walking to %s\n", element);
			return 0;
		case 0:
			print("%s not found\n", element);
			return 0;
		case 1:
			print("found %s attr 0x%ux start 0x%lux len %ld\n",
				element, fp->attr, fp->pstart, fp->length);
			break;
		}
	}

	syncclust();
	return fp;
}

/*
 *  read from a dos file
 */
long
dosread(Dosfile *fp, void *a, long n)
{
	long addr, k, o;
	Clustbuf *p;
	uchar *to;

	if((fp->attr & DOSDIR) == 0){
		if(fp->offset >= fp->length)
			return 0;
		if(fp->offset+n > fp->length)
			n = fp->length - fp->offset;
	}
	to = a;
	while(n > 0){
		/*
		 *  read the data; sectors below dos->dataaddr
		 *  are read one at a time.
		 */
		addr = fileaddr(fp, fp->offset/fp->dos->sectbytes, 0);
		if(addr < 0)
			return -1;
		p = getclust(fp->dos, addr);
		if(p == 0)
			return -1;
		/*
		 *  copy the bytes we need
		 */
		o = fp->offset % p->size;
		k = p->size - o;
		if(k > n)
			k = n;
		memmove(to, p->iobuf+o, k);
		putclust(p);
		to += k;
		fp->offset += k;
		n -= k;
	}
	syncclust();
	return to - (uchar *)a;
}

/*
 *  write to a dos file
 */
long
doswrite(Dosfile *fp, void *a, long n)
{
	long blksize, addr, k, o;
	Clustbuf *p, *pdir;
	Dosdir *dp;
	uchar *from;

	if(fp->attr & DOSDIR){
		print("write dir\n");
		return -1;
	}
	if(fp->pdir){
		pdir = getclust(fp->dos, fp->pdir);
		/*
		 *  should do consistency check if
		 *  concurrent access is possible.
		 */
		if(pdir == 0)
			panic("doswrite");
	}else
		pdir = 0;
	blksize = pdir ? fp->dos->clustbytes : fp->dos->sectbytes;
	from = a;
	while(n > 0){
		addr = fileaddr(fp, fp->offset/fp->dos->sectbytes, pdir);
		if(addr < 0)
			return -1;
		o = fp->offset % blksize;
		if(o == 0 && n >= blksize)
			p = getclustz(fp->dos, addr);
		else
			p = getclust(fp->dos, addr);
		if(p == 0)
			return -1;
		/*
		 *  copy the bytes we need
		 */
		k = p->size - o;
		if(k > n)
			k = n;
		memmove(p->iobuf+o, from, k);
		p->flags |= MOD;
		putclust(p);
		from += k;
		fp->offset += k;
		n -= k;
	}
	if(pdir){
		dp = (Dosdir *)(pdir->iobuf + fp->odir);
		puttime(dp);
		if(fp->offset > fp->length){
			fp->length = fp->offset;
			dp->length[0] = fp->length;
			dp->length[1] = fp->length>>8;
			dp->length[2] = fp->length>>16;
			dp->length[3] = fp->length>>24;
		}
		pdir->flags |= MOD;
		putclust(pdir);
	}
	syncclust();
	return from - (uchar *)a;
}

/*
 *  truncate a dos file to zero length
 */
int
dostrunc(Dosfile *fp)
{
	Clustbuf *pdir;
	Dosdir *dp;
	int p, np;

	if(fp->attr & DOSDIR){
		print("trunc dir\n");
		return -1;
	}
	pdir = getclust(fp->dos, fp->pdir);
	if(pdir == 0)
		panic("dostrunc");
	p = fatwalk(fp->dos, fp->pstart);
	fatwrite(fp->dos, fp->pstart, 0xffff);
	while(p >= 0){
		np = fatwalk(fp->dos, p);
		fatwrite(fp->dos, p, 0);
		p = np;
	}
	fp->length = 0;
	dp = (Dosdir *)(pdir->iobuf + fp->odir);
	puttime(dp);
	dp->length[0] = 0;
	dp->length[1] = 0;
	dp->length[2] = 0;
	dp->length[3] = 0;
	pdir->flags |= MOD;
	putclust(pdir);
	syncclust();
	return 0;
}
