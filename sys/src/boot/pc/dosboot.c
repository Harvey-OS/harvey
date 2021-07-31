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
extern char*	nextelem(char*, char*);
static void	setname(Dosfile*, char*);
long		dosreadseg(Dosfile*, long, long);

/*
 *  debugging
 */
int chatty;
#define chat if(chatty)print

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

	chat("getclust @ %d\n", sector);

	/*
	 *  if we have it, just return it
	 */
	for(p = bio; p < &bio[Nbio]; p++){
		if(sector == p->sector && dos == p->dos){
			p->age = m->ticks;
			chat("getclust %d in cache\n", sector);
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
	chat("getclust addr %d\n", (sector+dos->start)*dos->sectsize);
	if((*dos->seek)(dos->dev, (sector+dos->start)*dos->sectsize) < 0){
		chat("can't seek block\n");
		return 0;
	}
	if((*dos->read)(dos->dev, p->iobuf, size) != size){
		chat("can't read block\n");
		return 0;
	}

	p->age = m->ticks;
	p->dos = dos;
	p->sector = sector;
	chat("getclust %d read\n", sector);
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
	k |= p->iobuf[o]<<8;
	if(dos->fatbits == 12){
		if(n&1)
			k >>= 4;
		else
			k &= 0xfff;
		if(k >= 0xff8)
			k |= 0xf000;
	}
	k = k < 0xfff8 ? k : -1;
	chat("fatwalk %d -> %d\n", n, k);
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

	chat("fileaddr %8.8s %d\n", fp->name, ltarget);
	/*
	 *  root directory is contiguous and easy
	 */
	if(fp->pstart == 0){
		if(ltarget*dos->sectsize*dos->clustsize >= dos->rootsize*sizeof(Dosdir))
			return -1;
		l = dos->rootaddr + ltarget*dos->clustsize;
		chat("fileaddr %d -> %d\n", ltarget, l);
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
	chat("fileaddr %d -> %d\n", ltarget, l);
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

	if((fp->attr) & DDIR == 0){
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
		chat("comparing to %8.8s.%3.3s\n", d.name, d.ext);
		if(memcmp(file->name, d.name, sizeof(d.name)) != 0)
			continue;
		if(memcmp(file->ext, d.ext, sizeof(d.ext)) != 0)
			continue;
		file->attr = d.attr;
		file->pstart = GSHORT(d.start);
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
 *  read dos file system properties
 */
int
dosinit(Dos *dos)
{
	Dosboot *b;
	int i;
	Clustbuf *p;
	Dospart *dp;


	/* defaults till we know better */
	dos->start = 0;
	dos->sectsize = 512;
	dos->clustsize = 1;

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
print("%lux %lux\n", p->iobuf, &(p->iobuf[0x1fe]));
print("m[0] %2.2ux m[1] %2.2ux m[2] %2.2ux p[1fe] %2.2ux p[1ff] %2.2ux\n",
b->magic[0], b->magic[1], b->magic[2], p->iobuf[0x1fe], p->iobuf[0x1ff]);
		if(p->iobuf[0x1fe] != 0x55 || p->iobuf[0x1ff] != 0xaa){
			print("no dos file system or partition table\n");
			return -1;
		}
		dp = (Dospart*)&p->iobuf[0x1be];
		for(i = 0; i < 4; i++, dp++)
			if(dp->type && dp->flag == 0x80)
				break;
		if(i == 4)
			return -1;
		dos->start = GLONG(dp->start);
		p = getclust(dos, 0);
		if(p == 0){
			chat("can't read boot block\n");
			return -1;
		}
		p->dos = 0;
	}

	b = (Dosboot *)p->iobuf;
	if(b->magic[0] != JMPNEAR && (b->magic[0] != JMPSHORT || b->magic[2] != 0x90)){
		print("no dos file system\n");
		return -1;
	}

	if(chatty)
		bootdump(b);/**/

	/*
	 *  determine the systems' wondersous properties
	 */
	dos->sectsize = GSHORT(b->sectsize);
	dos->clustsize = b->clustsize;
	dos->clustbytes = dos->sectsize*dos->clustsize;
	dos->nresrv = GSHORT(b->nresrv);
	dos->nfats = b->nfats;
	dos->rootsize = GSHORT(b->rootsize);
	dos->volsize = GSHORT(b->volsize);
	if(dos->volsize == 0)
		dos->volsize = GLONG(b->bigvolsize);
	dos->mediadesc = b->mediadesc;
	dos->fatsize = GSHORT(b->fatsize);
	dos->fataddr = dos->nresrv;
	dos->rootaddr = dos->fataddr + dos->nfats*dos->fatsize;
	i = dos->rootsize*sizeof(Dosdir) + dos->sectsize - 1;
	i = i/dos->sectsize;
	dos->dataaddr = dos->rootaddr + i;
	dos->fatclusters = (dos->volsize - dos->dataaddr)/dos->clustsize;
	if(dos->fatclusters < 4087)
		dos->fatbits = 12;
	else
		dos->fatbits = 16;
	dos->freeptr = 2;

	/*
	 *  set up the root
	 */
	dos->root.dos = dos;
	dos->root.pstart = 0;
	dos->root.pcurrent = dos->root.lcurrent = 0;
	dos->root.offset = 0;
	dos->root.attr = DDIR;
	dos->root.length = dos->rootsize*sizeof(Dosdir);

	return 0;
}

static void
bootdump(Dosboot *b)
{
	print("magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	print("version: \"%8.8s\"\n", b->version);
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
	print("driveno: %d\n", b->driveno);
	print("reserved0: 0x%2.2x\n", b->reserved0);
	print("bootsig: 0x%2.2x\n", b->bootsig);
	print("volid: 0x%8.8x\n", GLONG(b->volid));
	print("label: \"%11.11s\"\n", b->label);
}

typedef struct Exec Exec;
struct	Exec
{
	uchar	magic[4];		/* magic number */
	uchar	text[4];	 	/* size of text segment */
	uchar	data[4];	 	/* size of initialized data */
	uchar	bss[4];	  		/* size of uninitialized data */
	uchar	syms[4];	 	/* size of symbol table */
	uchar	entry[4];	 	/* entry point */
	uchar	spsz[4];		/* size of sp/pc offset table */
	uchar	pcsz[4];		/* size of pc/line number table */
};

#define I_MAGIC		((((4*11)+0)*11)+7)

/*
 *  boot
 */
int
dosboot(Dos *dos, char *path)
{
	Dosfile file;
	long n;
	long addr;
	Exec *ep;
	char element[NAMELEN];
	void (*b)(void);
	extern int getline(void);

	if(dosinit(dos) < 0)
		return -1;

	file = dos->root;

	while(path = nextelem(path, element))
		switch(doswalk(&file, element)){
		case -1:
			print("error walking to %8.8s.%3.3s\n", file.name, file.ext);
			return -1;
		case 0:
			print("%8.8s.%3.3s %d %d not found\n", file.name, file.ext,
				file.pstart, file.length);
			return -1;
		case 1:
			print("found %8.8s.%3.3s attr 0x%ux start 0x%lux len %d\n", file.name,
				file.ext, file.attr, file.pstart, file.length);
			break;
		}

	/*
	 *  read header
	 */
	addr = BY2PG;
	n = sizeof(Exec);
	if(dosreadseg(&file, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}
	ep = (Exec *)BY2PG;
	if(GLLONG(ep->magic) != I_MAGIC){
		print("bad magic 0x%lux not a plan 9 executable!\n", GLLONG(ep->magic));
		return -1;
	}

	/*
	 *  read text (starts at second page)
	 */
	addr += sizeof(Exec);
	n = GLLONG(ep->text);
	print("%d", n);
	if(dosreadseg(&file, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  read data (starts at first page after kernel)
	 */
	addr = PGROUND(addr+n);
	n = GLLONG(ep->data);
	print("+%d", n);
	if(dosreadseg(&file, n, addr) != n){
		print("premature EOF\n");
		return -1;
	}

	/*
	 *  bss and entry point
	 */
	print("+%d start at 0x%lux\n", GLLONG(ep->bss), GLLONG(ep->entry));

	/*
	 *  Go to new code.  To avoid assumptions about where the program
	 *  thinks it is mapped, mask off the high part of the entry
	 *  address.  It's up to the program to get it's PC relocated to
	 *  the right place.
	 */
	b = (void (*)(void))(GLLONG(ep->entry) & 0xffff);
	(*b)();
	return 0;
}

/*
 *  read in a segment
 */
long
dosreadseg(Dosfile *fp, long len, long addr)
{
	char *a;
	long n, sofar;

	a = (char *)addr;
	for(sofar = 0; sofar < len; sofar += n){
		n = 8*1024;
		if(len - sofar < n)
			n = len - sofar;
		n = dosread(fp, a + sofar, n);
		if(n < 0)
			break;
		print(".");
	}
	return sofar;
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
	
	to = fp->ext;
	for(; *from && to-fp->ext < 3; from++, to++){
		if(*from >= 'a' && *from <= 'z')
			*to = *from + 'A' - 'a';
		else
			*to = *from;
	}
	while(to-fp->ext < 3)
		*to++ = ' ';

	chat("name is %8.8s %3.3s\n", fp->name, fp->ext);
}
