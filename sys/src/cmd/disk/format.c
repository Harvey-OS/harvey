#include <u.h>
#include <libc.h>

/*
 *  floppy types (all MFM encoding)
 */
typedef struct Type	Type;
struct Type
{
	char	*name;
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* number of heads */
	int	tracks;		/* tracks/disk */
	int	media;		/* media descriptor byte */
	int	cluster;	/* default cluster size */
};
Type floppytype[] =
{
 { "3½HD",	512, 18, 2, 80,	0xf0, 1, },
 { "3½DD",	512,  9, 2, 80,	0xf9, 2, },
 { "5¼HD",	512, 15, 2, 80,	0xf9, 1, },
 { "5¼DD",	512,  9, 2, 40,	0xfd, 2, },
};
#define NTYPES (sizeof(floppytype)/sizeof(Type))

typedef struct Dosboot	Dosboot;
struct Dosboot{
	uchar	magic[3];	/* really an xx86 JMP instruction */
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
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	type[8];
};
#define	PUTSHORT(p, v) { (p)[1] = (v)>>8; (p)[0] = (v); }
#define	PUTLONG(p, v) { PUTSHORT((p), (v)); PUTSHORT((p)+2, (v)>>16); }

typedef struct Dosdir	Dosdir;
struct Dosdir
{
	uchar	name[8];
	uchar	ext[3];
	uchar	attr;
	uchar	reserved[10];
	uchar	time[2];
	uchar	date[2];
	uchar	start[2];
	uchar	length[4];
};

#define	DRONLY	0x01
#define	DHIDDEN	0x02
#define	DSYSTEM	0x04
#define	DVLABEL	0x08
#define	DDIR	0x10
#define	DARCH	0x20

/*
 *  the boot program for the boot sector.
 */
uchar bootprog[512] =
{
[0x000]	0xEB, 0x3C, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
[0x03E] 0xFA, 0xFC, 0x8C, 0xC8, 0x8E, 0xD8, 0x8E, 0xD0,
	0xBC, 0x00, 0x7C, 0xBE, 0x77, 0x7C, 0xE8, 0x19,
	0x00, 0x33, 0xC0, 0xCD, 0x16, 0xBB, 0x40, 0x00,
	0x8E, 0xC3, 0xBB, 0x72, 0x00, 0xB8, 0x34, 0x12,
	0x26, 0x89, 0x07, 0xEA, 0x00, 0x00, 0xFF, 0xFF,
	0xEB, 0xD6, 0xAC, 0x0A, 0xC0, 0x74, 0x09, 0xB4,
	0x0E, 0xBB, 0x07, 0x00, 0xCD, 0x10, 0xEB, 0xF2,
	0xC3,  'N',  'o',  't',  ' ',  'a',  ' ',  'b',
	 'o',  'o',  't',  'a',  'b',  'l',  'e',  ' ',
	 'd',  'i',  's',  'c',  ' ',  'o',  'r',  ' ',
	 'd',  'i',  's',  'c',  ' ',  'e',  'r',  'r',
	 'o',  'r', '\r', '\n',  'P',  'r',  'e',  's',
	 's',  ' ',  'a',  'l',  'm',  'o',  's',  't',
	 ' ',  'a',  'n',  'y',  ' ',  'k',  'e',  'y',
	 ' ',  't',  'o',  ' ',  'r',  'e',  'b',  'o',
	 'o',  't',  '.',  '.',  '.', 0x00, 0x00, 0x00,
[0x1F0]	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA,
};

char *dev;
int clustersize;
uchar *fat;	/* the fat */
int fatbits;
int fatsecs;
int fatlast;	/* last cluster allocated */
int clusters;
int fatsecs;
int volsecs;
uchar *root;	/* first block of root */
int rootsecs;
int rootfiles;
int rootnext;
Type *t;
int fflag;
char file[64];	/* output file name */
char *bootfile;
char *type;

enum
{
	Sof = 1,	/* start of file */
	Eof = 2,	/* end of file */
};


void	dosfs(int, char*, int, char*[]);
ulong	clustalloc(int);
void	addrname(uchar*, Dir*, ulong);

void
usage(void)
{
	fprint(2, "usage: format [-b bfile] [-c csize] [-df] [-l label] [-t type] file [args ...]\n");
	exits("usage");
}

void
fatal(char *fmt, ...)
{
	int n;
	char err[128];

	n = doprint(err, err+sizeof(err), fmt, &fmt+1) - err;
	err[n] = 0;
	fprint(2, "format: %s\n", err);
	if(fflag && file[0])
		remove(file);
	exits(err);
}

void
main(int argc, char **argv)
{
	int n, dos;
	int cfd;
	char buf[512];
	char label[11];
	char *a;

	dos = 0;
	type = 0;
	clustersize = 0;
	memmove(label, "CYLINDRICAL", sizeof(label));
	ARGBEGIN {
	case 'b':
		bootfile = ARGF();
		break;
	case 'd':
		dos = 1;
		break;
	case 'c':
		clustersize = atoi(ARGF());
		break;
	case 'f':
		fflag = 1;
		break;
	case 'l':
		a = ARGF();
		n = strlen(a);
		if(n > sizeof(label))
			n = sizeof(label);
		memmove(label, a, n);
		while(n < sizeof(label))
			label[n++] = ' ';
		break;
	case 't':
		type = ARGF();
		break;
	default:
		usage();
	} ARGEND

	if(argc < 1)
		usage();

	dev = argv[0];
	cfd = -1;
	if(fflag == 0){
		n = strlen(argv[0]);
		if(n > 4 && strcmp(argv[0]+n-4, "disk") == 0)
			*(argv[0]+n-4) = 0;
		else if(n > 3 && strcmp(argv[0]+n-3, "ctl") == 0)
			*(argv[0]+n-3) = 0;

		sprint(buf, "%sctl", dev);
		cfd = open(buf, ORDWR);
		if(cfd < 0)
			fatal("opening %s: %r", buf);
		print("Formatting floppy %s\n", dev);
		if(type)
			sprint(buf, "format %s", type);
		else
			strcpy(buf, "format");
		if(write(cfd, buf, strlen(buf)) < 0)
			fatal("formatting tracks: %r");
	}

	if(dos)
		dosfs(cfd, label, argc-1, argv+1);
	if(cfd >= 0)
		close(cfd);
	exits(0);
}

void
dosfs(int cfd, char *label, int argc, char *argv[])
{
	char r[16];
	Dosboot *b;
	uchar *buf;
	Dir d;
	int n, fd, sysfd;
	ulong length, x;
	uchar *p;

	print("Initialising MS-DOS file system\n");

	if(fflag){
		sprint(file, "%s", dev);
		if((fd = create(dev, ORDWR, 0666)) < 0)
			fatal("create %s: %r", file);
		t = floppytype;
		if(type){
			for(t = floppytype; t < &floppytype[NTYPES]; t++)
				if(strcmp(type, t->name) == 0)
					break;
			if(t == &floppytype[NTYPES])
				fatal("unknown floppy type %s", type);
		}
		length = t->bytes*t->sectors*t->heads*t->tracks;
	}
	else{
		sprint(file, "%sdisk", dev);
		fd = open(file, ORDWR);
		if(fd < 0)
			fatal("open %s: %r", file);
		if(dirfstat(fd, &d) < 0)
			fatal("stat %s: %r", file);
		length = d.length;
	
		t = 0;
		seek(cfd, 0, 0);
		n = read(cfd, file, sizeof(file)-1);
		if(n < 0)
			fatal("reading floppy type");
		else {
			file[n] = 0;
			for(t = floppytype; t < &floppytype[NTYPES]; t++)
				if(strcmp(file, t->name) == 0)
					break;
			if(t == &floppytype[NTYPES])
				fatal("unknown floppy type %s", file);
		}
	}
	print("floppy type %s, %d tracks, %d heads, %d sectors/track, %d bytes/sec\n",
		t->name, t->tracks, t->heads, t->sectors, t->bytes);

	if(clustersize == 0)
		clustersize = t->cluster;
	clusters = length/(t->bytes*clustersize);
	if(clusters < 4087)
		fatbits = 12;
	else
		fatbits = 16;
	volsecs = length/t->bytes;
	fatsecs = (fatbits*clusters + 8*t->bytes - 1)/(8*t->bytes);
	rootsecs = volsecs/200;
	rootfiles = rootsecs * (t->bytes/sizeof(Dosdir));
	buf = malloc(t->bytes);
	if(buf == 0)
		fatal("out of memory");

	/*
	 *  write bootstrap & parameter block
	 */
	if(bootfile){
		if((sysfd = open(bootfile, OREAD)) < 0)
			fatal("open %s: %r", bootfile);
		if(read(sysfd, buf, t->bytes) < 0)
			fatal("read %s: %r", bootfile);
		close(sysfd);
	}
	else
		memmove(buf, bootprog, sizeof(bootprog));
	b = (Dosboot*)buf;
	b->magic[0] = 0xEB;
	b->magic[1] = 0x3C;
	b->magic[2] = 0x90;
	memmove(b->version, "Plan9.00", sizeof(b->version));
	PUTSHORT(b->sectsize, t->bytes);
	b->clustsize = clustersize;
	PUTSHORT(b->nresrv, 1);
	b->nfats = 2;
	PUTSHORT(b->rootsize, rootfiles);
	if(volsecs < (1<<16)){
		PUTSHORT(b->volsize, volsecs);
	}
	PUTLONG(b->bigvolsize, volsecs);
	b->mediadesc = t->media;
	PUTSHORT(b->fatsize, fatsecs);
	PUTSHORT(b->trksize, t->sectors);
	PUTSHORT(b->nheads, t->heads);
	PUTLONG(b->nhidden, 0);
	b->driveno = 0;
	b->bootsig = 0x29;
	x = time(0);
	PUTLONG(b->volid, x);
	memmove(b->label, label, sizeof(b->label));
	sprint(r, "FAT%d    ", fatbits);
	memmove(b->type, r, sizeof(b->type));
	buf[t->bytes-2] = 0x55;
	buf[t->bytes-1] = 0xAA;
	if(seek(fd, 0, 0) < 0)
		fatal("seek to boot sector: %r\n");
	if(write(fd, buf, t->bytes) != t->bytes)
		fatal("writing boot sector: %r");
	free(buf);

	/*
	 *  allocate an in memory fat
	 */
	fat = malloc(fatsecs*t->bytes);
	if(fat == 0)
		fatal("out of memory");
	memset(fat, 0, fatsecs*t->bytes);
	fat[0] = t->media;
	fat[1] = 0xff;
	fat[2] = 0xff;
	if(fatbits == 16)
		fat[3] = 0xff;
	fatlast = 1;
	seek(fd, 2*fatsecs*t->bytes, 1);	/* 2 fats */

	/*
	 *  allocate an in memory root
	 */
	root = malloc(rootsecs*t->bytes);
	if(root == 0)
		fatal("out of memory");
	memset(root, 0, rootsecs*t->bytes);
	seek(fd, rootsecs*t->bytes, 1);		/* rootsecs */

	/*
	 * Now positioned at the Files Area.
	 * If we have any arguments, process 
	 * them and write out.
	 */
	for(p = root; argc > 0; argc--, argv++, p += sizeof(Dosdir)){
		if(p >= (root+(rootsecs*t->bytes)))
			fatal("too many files in root");
		/*
		 * Open the file and get its length.
		 */
		if((sysfd = open(*argv, OREAD)) < 0)
			fatal("open %s: %r", *argv);
		if(dirfstat(sysfd, &d) < 0)
			fatal("stat %s: %r", *argv);
		print("Adding file %s, length %ld\n", *argv, d.length);

		length = d.length;
		if(length){
			/*
			 * Allocate a buffer to read the entire file into.
			 * This must be rounded up to a cluster boundary.
			 *
			 * Read the file and write it out to the Files Area.
			 */
			length += t->bytes*clustersize - 1;
			length /= t->bytes*clustersize;
			length *= t->bytes*clustersize;
			if((buf = malloc(length)) == 0)
				fatal("out of memory");
	
			if(read(sysfd, buf, d.length) < 0)
				fatal("read %s: %r", *argv);
			memset(buf+d.length, 0, length-d.length);
			if(write(fd, buf, length) < 0)
				fatal("write %s: %r", *argv);
			free(buf);

			close(sysfd);
	
			/*
			 * Allocate the FAT clusters.
			 * We're assuming here that where we
			 * wrote the file is in sync with
			 * the cluster allocation.
			 * Save the starting cluster.
			 */
			length /= t->bytes*clustersize;
			x = clustalloc(Sof);
			for(n = 0; n < length-1; n++)
				clustalloc(0);
			clustalloc(Eof);
		}
		else
			x = 0;

		/*
		 * Add the filename to the root.
		 */
		addrname(p, &d, x);
	}

	/*
	 *  write the fats and root
	 */
	seek(fd, t->bytes, 0);
	if(write(fd, fat, fatsecs*t->bytes) < 0)
		fatal("writing fat #1: %r");
	if(write(fd, fat, fatsecs*t->bytes) < 0)
		fatal("writing fat #2: %r");
	if(write(fd, root, rootsecs*t->bytes) < 0)
		fatal("writing root: %r");

	if(fflag){
		seek(fd, t->bytes*t->sectors*t->heads*t->tracks-1, 0);
		write(fd, "9", 1);
	}
}

/*
 *  allocate a cluster
 */
ulong
clustalloc(int flag)
{
	ulong o, x;

	if(flag != Sof){
		x = (flag == Eof) ? 0xffff : (fatlast+1);
		if(fatbits == 12){
			x &= 0xfff;
			o = (3*fatlast)/2;
			if(fatlast & 1){
				fat[o] = (fat[o]&0x0f) | (x<<4);
				fat[o+1] = (x>>4);
			} else {
				fat[o] = x;
				fat[o+1] = (fat[o+1]&0xf0) | ((x>>8) & 0x0F);
			}
		} else {
			o = 2*fatlast;
			fat[o] = x;
			fat[o+1] = x>>8;
		}
	}
		
	if(flag == Eof)
		return 0;
	else
		return ++fatlast;
}

void
putname(char *p, Dosdir *d)
{
	int i;

	memset(d->name, ' ', sizeof d->name+sizeof d->ext);
	for(i = 0; i< sizeof(d->name); i++){
		if(*p == 0 || *p == '.')
			break;
		d->name[i] = toupper(*p++);
	}
	p = strrchr(p, '.');
	if(p){
		for(i = 0; i < sizeof d->ext; i++){
			if(*++p == 0)
				break;
			d->ext[i] = toupper(*p);
		}
	}
}

void
puttime(Dosdir *d)
{
	Tm *t = localtime(time(0));
	ushort x;

	x = (t->hour<<11) | (t->min<<5) | (t->sec>>1);
	d->time[0] = x;
	d->time[1] = x>>8;
	x = ((t->year-80)<<9) | ((t->mon+1)<<5) | t->mday;
	d->date[0] = x;
	d->date[1] = x>>8;
}

void
addrname(uchar *entry, Dir *dir, ulong start)
{
	Dosdir *d;

	d = (Dosdir*)entry;
	putname(dir->name, d);
	d->attr = DRONLY;
	puttime(d);
	d->start[0] = start;
	d->start[1] = start>>8;
	d->length[0] = dir->length;
	d->length[1] = dir->length>>8;
	d->length[2] = dir->length>>16;
	d->length[3] = dir->length>>24;
}
