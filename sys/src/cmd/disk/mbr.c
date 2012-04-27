/*
 * install new master boot record boot code on PC disk.
 */

#include <u.h>
#include <libc.h>
#include <disk.h>

typedef struct {
	uchar	active;			/* active flag */
	uchar	starth;			/* starting head */
	uchar	starts;			/* starting sector */
	uchar	startc;			/* starting cylinder */
	uchar	type;			/* partition type */
	uchar	endh;			/* ending head */
	uchar	ends;			/* ending sector */
	uchar	endc;			/* ending cylinder */
	uchar	lba[4];			/* starting LBA */
	uchar	size[4];		/* size in sectors */
} Tentry;

enum {
	Toffset = 0x1BE,		/* offset of partition table */

	Type9	= 0x39,
};

/*
 * Default boot block prints an error message and reboots. 
 */
static int ndefmbr = Toffset;
static char defmbr[512] = {
[0x000]	0xEB, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
};

void
usage(void)
{
	fprint(2, "usage: disk/mbr [-m mbrfile] disk\n");
	exits("usage");
}

void
fatal(char *fmt, ...)
{
	char err[ERRMAX];
	va_list arg;

	va_start(arg, fmt);
	vsnprint(err, ERRMAX, fmt, arg);
	va_end(arg);
	fprint(2, "mbr: %s\n", err);
	exits(err);
}

static void
putle32(void* v, u32int i)
{
	uchar *p;

	p = v;
	p[0] = i;
	p[1] = i>>8;
	p[2] = i>>16;
	p[3] = i>>24;
}

static void
writechs(Disk *disk, uchar *p, vlong lba)
{
	int c, h, s;

	s = lba % disk->s;
	h = (lba / disk->s) % disk->h;
	c = lba / (disk->s * disk->h);

	if(c >= 1024) {
		c = 1023;
		h = disk->h - 1;
		s = disk->s - 1;
	}

	p[0] = h;
	p[1] = ((s+1) & 0x3F) | ((c>>2) & 0xC0);
	p[2] = c;
}

static void
wrtentry(Disk *disk, Tentry *tp, int type, u32int base, u32int lba, u32int end)
{
	tp->active = 0x80;		/* make this sole partition active */
	tp->type = type;
	writechs(disk, &tp->starth, lba);
	writechs(disk, &tp->endh, end-1);
	putle32(tp->lba, lba-base);
	putle32(tp->size, end-lba);
}

void
main(int argc, char **argv)
{
	Disk *disk;
	Tentry *tp;
	uchar *mbr, *buf;
	char *mbrfile;
	ulong secsize;
	int flag9, sysfd, nmbr;

	flag9 = 0;
	mbrfile = nil;
	ARGBEGIN {
	case '9':
		flag9 = 1;
		break;
	case 'm':
		mbrfile = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if(argc < 1)
		usage();

	disk = opendisk(argv[0], 0, 0);
	if(disk == nil)
		fatal("opendisk %s: %r", argv[0]);

	if(disk->type == Tfloppy)
		fatal("will not install mbr on floppy");
	/*
	 * we need to cope with 4k-byte sectors on some newer disks.
	 * we're only interested in 512 bytes of mbr, so
	 * on 4k disks, rely on /dev/sd to read-modify-write.
	 */
	secsize = 512;
	if(disk->secsize != secsize)
		fprint(2, "%s: sector size %lld not %ld, should be okay\n",
			argv0, disk->secsize, secsize);

	buf = malloc(secsize*(disk->s+1));
	mbr = malloc(secsize*disk->s);
	if(buf == nil || mbr == nil)
		fatal("out of memory");

	/*
	 * Start with initial sector from disk.
	 */
	if(seek(disk->fd, 0, 0) < 0)
		fatal("seek to boot sector: %r\n");
	if(read(disk->fd, mbr, secsize) != secsize)
		fatal("reading boot sector: %r");

	if(mbrfile == nil){
		nmbr = ndefmbr;
		memmove(mbr, defmbr, nmbr);
	} else {
		memset(buf, 0, secsize*disk->s);
		if((sysfd = open(mbrfile, OREAD)) < 0)
			fatal("open %s: %r", mbrfile);
		if((nmbr = read(sysfd, buf, secsize*(disk->s+1))) < 0)
			fatal("read %s: %r", mbrfile);
		if(nmbr > secsize*disk->s)
			fatal("master boot record too large %d > %d", nmbr, secsize*disk->s);
		if(nmbr < secsize)
			nmbr = secsize;
		close(sysfd);
		memmove(buf+Toffset, mbr+Toffset, secsize-Toffset);
		memmove(mbr, buf, nmbr);
	}

	if(flag9){
		tp = (Tentry*)(mbr+Toffset);
		memset(tp, 0, secsize-Toffset);
		wrtentry(disk, tp, Type9, 0, disk->s, disk->secs);
	}
	mbr[secsize-2] = 0x55;
	mbr[secsize-1] = 0xAA;
	nmbr = (nmbr+secsize-1)&~(secsize-1);
	if(seek(disk->wfd, 0, 0) < 0)
		fatal("seek to MBR sector: %r\n");
	if(write(disk->wfd, mbr, nmbr) != nmbr)
		fatal("writing MBR: %r");
	
	exits(0);
}
