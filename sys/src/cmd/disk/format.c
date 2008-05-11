#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <disk.h>

/*
 *  disk types (all MFM encoding)
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
 { "3½HD",	512, 18,  2, 80, 0xf0, 1, },
 { "3½DD",	512,  9,  2, 80, 0xf9, 2, },
 { "3½QD",	512, 36, 2, 80, 0xf9, 2, },	/* invented */
 { "5¼HD",	512, 15,  2, 80, 0xf9, 1, },
 { "5¼DD",	512,  9,  2, 40, 0xfd, 2, },
 { "hard",	512,  0,  0, 0, 0xf8, 4, },
};

#define NTYPES (sizeof(floppytype)/sizeof(Type))

typedef struct Dosboot	Dosboot;
struct Dosboot{
	uchar	magic[3];	/* really an x86 JMP instruction */
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
#define	GETSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GETLONG(p)	(((ulong)GETSHORT(p+2)<<16)|(ulong)GETSHORT(p))

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
int nbootprog = 188;	/* no. of bytes of boot program, including the first 0x3E */
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
vlong volsecs;
uchar *root;	/* first block of root */
int rootsecs;
int rootfiles;
int rootnext;
int nresrv = 1;
int chatty;
vlong length;
Type *t;
int fflag;
int hflag;
int xflag;
char *file;
char *pbs;
char *type;
char *bootfile;
int dos;

enum
{
	Sof = 1,	/* start of file */
	Eof = 2,	/* end of file */
};

void	dosfs(int, int, Disk*, char*, int, char*[], int);
ulong	clustalloc(int);
void	addrname(uchar*, Dir*, char*, ulong);
void	sanitycheck(Disk*);

void
usage(void)
{
	fprint(2, "usage: disk/format [-df] [-b bootblock] [-c csize] "
		"[-l label] [-r nresrv] [-t type] disk [files ...]\n");
	exits("usage");
}

void
fatal(char *fmt, ...)
{
	char err[128];
	va_list arg;

	va_start(arg, fmt);
	vsnprint(err, sizeof(err), fmt, arg);
	va_end(arg);
	fprint(2, "format: %s\n", err);
	if(fflag && file)
		remove(file);
	exits(err);
}

void
main(int argc, char **argv)
{
	int fd, n, writepbs;
	char buf[512], label[11];
	char *a;
	Disk *disk;

	dos = 0;
	type = nil;
	clustersize = 0;
	writepbs = 0;
	memmove(label, "CYLINDRICAL", sizeof(label));
	ARGBEGIN {
	case 'b':
		pbs = EARGF(usage());
		writepbs = 1;
		break;
	case 'c':
		clustersize = atoi(EARGF(usage()));
		break;
	case 'd':
		dos = 1;
		writepbs = 1;
		break;
	case 'f':
		fflag = 1;
		break;
	case 'l':
		a = EARGF(usage());
		n = strlen(a);
		if(n > sizeof(label))
			n = sizeof(label);
		memmove(label, a, n);
		while(n < sizeof(label))
			label[n++] = ' ';
		break;
	case 'r':
		nresrv = atoi(EARGF(usage()));
		break;
	case 't':
		type = EARGF(usage());
		break;
	case 'v':
		chatty++;
		break;
	case 'x':
		xflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if(argc < 1)
		usage();

	disk = opendisk(argv[0], 0, 0);
	if(disk == nil) {
		if(fflag) {
			if((fd = create(argv[0], ORDWR, 0666)) >= 0) {
				file = argv[0];
				close(fd);
				disk = opendisk(argv[0], 0, 0);
			}
		}
	}
	if(disk == nil)
		fatal("opendisk: %r");

	if(disk->type == Tfile)
		fflag = 1;

	if(type == nil) {
		switch(disk->type){
		case Tfile:
			type = "3½HD";
			break;
		case Tfloppy:
			seek(disk->ctlfd, 0, 0);
			n = read(disk->ctlfd, buf, 10);
			if(n <= 0 || n >= 10)
				fatal("reading floppy type");
			buf[n] = 0;
			type = strdup(buf);
			if(type == nil)
				fatal("out of memory");
			break;
		case Tsd:
			type = "hard";
			break;
		default:
			type = "unknown";
			break;
		}
	}

	if(!fflag && disk->type == Tfloppy)
		if(fprint(disk->ctlfd, "format %s", type) < 0)
			fatal("formatting floppy as %s: %r", type);

	if(disk->type != Tfloppy)
		sanitycheck(disk);

	/* check that everything will succeed */
	dosfs(dos, writepbs, disk, label, argc-1, argv+1, 0);

	/* commit */
	dosfs(dos, writepbs, disk, label, argc-1, argv+1, 1);

	print("used %lld bytes\n", fatlast*clustersize*disk->secsize);
	exits(0);
}

/*
 * Look for a partition table on sector 1, as would be the
 * case if we were erroneously formatting 9fat without -r 2.
 * If it's there and nresrv is not big enough, complain and exit.
 * I've blown away my partition table too many times.
 */
void
sanitycheck(Disk *disk)
{
	char buf[512];
	int bad;

	if(xflag)
		return;

	bad = 0;
	if(dos && nresrv < 2 && seek(disk->fd, disk->secsize, 0) == disk->secsize
	&& read(disk->fd, buf, sizeof(buf)) >= 5 && strncmp(buf, "part ", 5) == 0) {
		fprint(2, 
			"there's a plan9 partition on the disk\n"
			"and you didn't specify -r 2 (or greater).\n"
			"either specify -r 2 or -x to disable this check.\n");
		bad = 1;
	}

	if(disk->type == Tsd && disk->offset == 0LL) {
		fprint(2,
			"you're attempting to format your disk (/dev/sdXX/data)\n"
			"rather than a partition like /dev/sdXX/9fat;\n"
			"this is likely a mistake.  specify -x to disable this check.\n");
		bad = 1;
	}

	if(bad)
		exits("failed disk sanity check");
}

/*
 * Return the BIOS drive number for the disk.
 * 0x80 is the first fixed disk, 0x81 the next, etc.
 * We map sdC0=0x80, sdC1=0x81, sdD0=0x82, sdD1=0x83
 */
int
getdriveno(Disk *disk)
{
	char buf[64], *p;

	if(disk->type != Tsd)
		return 0x80;	/* first hard disk */

	if(fd2path(disk->fd, buf, sizeof(buf)) < 0)
		return 0x80;

	/*
	 * The name is of the format #SsdC0/foo 
	 * or /dev/sdC0/foo.
	 * So that we can just look for /sdC0, turn 
	 * #SsdC0/foo into #/sdC0/foo.
	 */
	if(buf[0] == '#' && buf[1] == 'S')
		buf[1] = '/';

	for(p=buf; *p; p++)
		if(p[0] == 's' && p[1] == 'd' && (p[2]=='C' || p[2]=='D') &&
		    (p[3]=='0' || p[3]=='1'))
			return 0x80 + (p[2]-'C')*2 + (p[3]-'0');
		
	return 0x80;
}

long
writen(int fd, void *buf, long n)
{
	long m, tot;

	/* write 8k at a time, to be nice to the disk subsystem */
	for(tot=0; tot<n; tot+=m){
		m = n - tot;
		if(m > 8192)
			m = 8192;
		if(write(fd, (uchar*)buf+tot, m) != m)
			break;
	}
	return tot;
}

void
dosfs(int dofat, int dopbs, Disk *disk, char *label, int argc, char *argv[], int commit)
{
	char r[16];
	Dosboot *b;
	uchar *buf, *pbsbuf, *p;
	Dir *d;
	int i, data, newclusters, npbs, n, sysfd;
	ulong x;
	vlong length, secsize;

	if(dofat == 0 && dopbs == 0)
		return;

	for(t = floppytype; t < &floppytype[NTYPES]; t++)
		if(strcmp(type, t->name) == 0)
			break;
	if(t == &floppytype[NTYPES])
		fatal("unknown floppy type %s", type);

	if(t->sectors == 0 && strcmp(type, "hard") == 0) {
		t->sectors = disk->s;
		t->heads = disk->h;
		t->tracks = disk->c;
	}

	if(t->sectors == 0 && dofat)
		fatal("cannot format fat with type %s: geometry unknown\n", type);

	if(fflag){
		disk->size = t->bytes*t->sectors*t->heads*t->tracks;
		disk->secsize = t->bytes;
		disk->secs = disk->size / disk->secsize;
	}

	secsize = disk->secsize;
	length = disk->size;

	buf = malloc(secsize);
	if(buf == 0)
		fatal("out of memory");

	/*
	 * Make disk full size if a file.
	 */
	if(fflag && disk->type == Tfile){
		if((d = dirfstat(disk->wfd)) == nil)
			fatal("fstat disk: %r");
		if(commit && d->length < disk->size) {
			if(seek(disk->wfd, disk->size-1, 0) < 0)
				fatal("seek to 9: %r");
			if(write(disk->wfd, "9", 1) < 0)
				fatal("writing 9: @%lld %r", seek(disk->wfd, 0LL, 1));
		}
		free(d);
	}

	/*
	 * Start with initial sector from disk
	 */
	if(seek(disk->fd, 0, 0) < 0)
		fatal("seek to boot sector: %r\n");
	if(commit && read(disk->fd, buf, secsize) != secsize)
		fatal("reading boot sector: %r");

	if(dofat)
		memset(buf, 0, sizeof(Dosboot));

	/*
	 * Jump instruction and OEM name.
	 */
	b = (Dosboot*)buf;
	b->magic[0] = 0xEB;
	b->magic[1] = 0x3C;
	b->magic[2] = 0x90;
	memmove(b->version, "Plan9.00", sizeof(b->version));
	
	/*
	 * Add bootstrapping code; assume it starts 
	 * at 0x3E (the destination of the jump we just
	 * wrote to b->magic).
	 */
	if(dopbs) {
		pbsbuf = malloc(secsize);
		if(pbsbuf == 0)
			fatal("out of memory");

		if(pbs){
			if((sysfd = open(pbs, OREAD)) < 0)
				fatal("open %s: %r", pbs);
			if((npbs = read(sysfd, pbsbuf, secsize)) < 0)
				fatal("read %s: %r", pbs);

			if(npbs > secsize-2)
				fatal("boot block too large");

			close(sysfd);
		}
		else {
			memmove(pbsbuf, bootprog, sizeof(bootprog));
			npbs = nbootprog;
		}
		if(npbs <= 0x3E)
			fprint(2, "warning: pbs too small\n");
		else
			memmove(buf+0x3E, pbsbuf+0x3E, npbs-0x3E);

		free(pbsbuf);
	}

	/*
	 * Add FAT BIOS parameter block.
	 */
	if(dofat) {
		if(commit) {
			print("Initializing FAT file system\n");
			print("type %s, %d tracks, %d heads, %d sectors/track, %lld bytes/sec\n",
				t->name, t->tracks, t->heads, t->sectors, secsize);
		}

		if(clustersize == 0)
			clustersize = t->cluster;
		/*
		 * the number of fat bits depends on how much disk is left
		 * over after you subtract out the space taken up by the fat tables. 
		 * try both.  what a crock.
		 */
		fatbits = 12;
Tryagain:
		volsecs = length/secsize;
		/*
		 * here's a crock inside a crock.  even having fixed fatbits,
		 * the number of fat sectors depends on the number of clusters,
		 * but of course we don't know yet.  maybe iterating will get us there.
		 * or maybe it will cycle.
		 */
		clusters = 0;
		for(i=0;; i++){
			fatsecs = (fatbits*clusters + 8*secsize - 1)/(8*secsize);
			rootsecs = volsecs/200;
			rootfiles = rootsecs * (secsize/sizeof(Dosdir));
			if(rootfiles > 512){
				rootfiles = 512;
				rootsecs = rootfiles/(secsize/sizeof(Dosdir));
			}
			data = nresrv + 2*fatsecs + (rootfiles*sizeof(Dosdir) + secsize-1)/secsize;
			newclusters = 2 + (volsecs - data)/clustersize;
			if(newclusters == clusters)
				break;
			clusters = newclusters;
			if(i > 10)
				fatal("can't decide how many clusters to use (%d? %d?)", clusters, newclusters);
if(chatty) print("clusters %d\n", clusters);
		}
				
if(chatty) print("try %d fatbits => %d clusters of %d\n", fatbits, clusters, clustersize);
		switch(fatbits){
		case 12:
			if(clusters >= 4087){
				fatbits = 16;
				goto Tryagain;
			}
			break;
		case 16:
			if(clusters >= 65527)
				fatal("disk too big; implement fat32");
			break;
		}
		PUTSHORT(b->sectsize, secsize);
		b->clustsize = clustersize;
		PUTSHORT(b->nresrv, nresrv);
		b->nfats = 2;
		PUTSHORT(b->rootsize, rootfiles);
		if(volsecs < (1<<16))
			PUTSHORT(b->volsize, volsecs);
		b->mediadesc = t->media;
		PUTSHORT(b->fatsize, fatsecs);
		PUTSHORT(b->trksize, t->sectors);
		PUTSHORT(b->nheads, t->heads);
		PUTLONG(b->nhidden, disk->offset);
		PUTLONG(b->bigvolsize, volsecs);
	
		/*
		 * Extended BIOS Parameter Block.
		 */
		if(t->media == 0xF8)
			b->driveno = getdriveno(disk);
		else
			b->driveno = 0;
if(chatty) print("driveno = %ux\n", b->driveno);
	
		b->bootsig = 0x29;
		x = disk->offset + b->nfats*fatsecs + nresrv;
		PUTLONG(b->volid, x);
if(chatty) print("volid = %lux %lux\n", x, GETLONG(b->volid));
		memmove(b->label, label, sizeof(b->label));
		sprint(r, "FAT%d    ", fatbits);
		memmove(b->type, r, sizeof(b->type));
	}

	buf[secsize-2] = 0x55;
	buf[secsize-1] = 0xAA;

	if(commit) {
		if(seek(disk->wfd, 0, 0) < 0)
			fatal("seek to boot sector: %r\n");
		if(write(disk->wfd, buf, secsize) != secsize)
			fatal("writing boot sector: %r");
	}

	free(buf);

	/*
	 * If we were only called to write the PBS, leave now.
	 */
	if(dofat == 0)
		return;

	/*
	 *  allocate an in memory fat
	 */
	if(seek(disk->wfd, nresrv*secsize, 0) < 0)
		fatal("seek to fat: %r\n");
if(chatty) print("fat @%lluX\n", seek(disk->wfd, 0, 1));
	fat = malloc(fatsecs*secsize);
	if(fat == 0)
		fatal("out of memory");
	memset(fat, 0, fatsecs*secsize);
	fat[0] = t->media;
	fat[1] = 0xff;
	fat[2] = 0xff;
	if(fatbits == 16)
		fat[3] = 0xff;
	fatlast = 1;
	if(seek(disk->wfd, 2*fatsecs*secsize, 1) < 0)	/* 2 fats */
		fatal("seek to root: %r");
if(chatty) print("root @%lluX\n", seek(disk->wfd, 0LL, 1));

	/*
	 *  allocate an in memory root
	 */
	root = malloc(rootsecs*secsize);
	if(root == 0)
		fatal("out of memory");
	memset(root, 0, rootsecs*secsize);
	if(seek(disk->wfd, rootsecs*secsize, 1) < 0)	/* rootsecs */
		fatal("seek to files: %r");
if(chatty) print("files @%lluX\n", seek(disk->wfd, 0LL, 1));

	/*
	 * Now positioned at the Files Area.
	 * If we have any arguments, process 
	 * them and write out.
	 */
	for(p = root; argc > 0; argc--, argv++, p += sizeof(Dosdir)){
		if(p >= (root+(rootsecs*secsize)))
			fatal("too many files in root");
		/*
		 * Open the file and get its length.
		 */
		if((sysfd = open(*argv, OREAD)) < 0)
			fatal("open %s: %r", *argv);
		if((d = dirfstat(sysfd)) == nil)
			fatal("stat %s: %r", *argv);
		if(d->length > 0xFFFFFFFFU)
			fatal("file %s too big\n", *argv, d->length);
		if(commit)
			print("Adding file %s, length %lld\n", *argv, d->length);

		length = d->length;
		if(length){
			/*
			 * Allocate a buffer to read the entire file into.
			 * This must be rounded up to a cluster boundary.
			 *
			 * Read the file and write it out to the Files Area.
			 */
			length += secsize*clustersize - 1;
			length /= secsize*clustersize;
			length *= secsize*clustersize;
			if((buf = malloc(length)) == 0)
				fatal("out of memory");
	
			if(readn(sysfd, buf, d->length) != d->length)
				fatal("read %s: %r", *argv);
			memset(buf+d->length, 0, length-d->length);
if(chatty) print("%s @%lluX\n", d->name, seek(disk->wfd, 0LL, 1));
			if(commit && writen(disk->wfd, buf, length) != length)
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
			length /= secsize*clustersize;
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
fprint(2, "add %s at clust %lux\n", d->name, x);
		addrname(p, d, *argv, x);
		free(d);
	}

	/*
	 *  write the fats and root
	 */
	if(commit) {
		if(seek(disk->wfd, nresrv*secsize, 0) < 0)
			fatal("seek to fat #1: %r");
		if(write(disk->wfd, fat, fatsecs*secsize) < 0)
			fatal("writing fat #1: %r");
		if(write(disk->wfd, fat, fatsecs*secsize) < 0)
			fatal("writing fat #2: %r");
		if(write(disk->wfd, root, rootsecs*secsize) < 0)
			fatal("writing root: %r");
	}

	free(fat);
	free(root);
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
	else{
		++fatlast;
		if(fatlast >= clusters)
			sysfatal("data does not fit on disk (%d %d)", fatlast, clusters);
		return fatlast;
	}
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
addrname(uchar *entry, Dir *dir, char *name, ulong start)
{
	char *s;
	Dosdir *d;

	s = strrchr(name, '/');
	if(s)
		s++;
	else
		s = name;

	d = (Dosdir*)entry;
	putname(s, d);
	if(strcmp(s, "9load") == 0)
		d->attr = DSYSTEM;
	else
		d->attr = 0;
	puttime(d);
	d->start[0] = start;
	d->start[1] = start>>8;
	d->length[0] = dir->length;
	d->length[1] = dir->length>>8;
	d->length[2] = dir->length>>16;
	d->length[3] = dir->length>>24;
}
