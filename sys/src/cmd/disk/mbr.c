/*
 * install new master boot record boot code on PC disk.
 */

#include <u.h>
#include <libc.h>
#include <disk.h>

enum {
	Toffset = 0x1BE	/* offset of partition table */
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

void
main(int argc, char **argv)
{
	Disk *disk;
	uchar *mbr, *buf;
	char *mbrfile;
	ulong secsize;
	int sysfd, nmbr;

	mbrfile = nil;
	ARGBEGIN {
	case 'm':
		mbrfile = ARGF();
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
	if(disk->secsize != 512)
		fatal("secsize %d invalid", disk->secsize);

	secsize = disk->secsize;
	buf = malloc(secsize);
	mbr = malloc(secsize);
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
		if((sysfd = open(mbrfile, OREAD)) < 0)
			fatal("open %s: %r", mbrfile);
		if((nmbr = read(sysfd, buf, secsize)) < 0)
			fatal("read %s: %r", mbrfile);
		if(nmbr > Toffset)
			fatal("master boot record too large");
		close(sysfd);
		memmove(mbr, buf, nmbr);
	}

	if(nmbr < Toffset)
		memset(mbr+nmbr, 0, Toffset-nmbr);

	mbr[secsize-2] = 0x55;
	mbr[secsize-1] = 0xAA;

	if(seek(disk->wfd, 0, 0) < 0)
		fatal("seek to MBR sector: %r\n");
	if(write(disk->wfd, mbr, secsize) != secsize)
		fatal("writing MBR: %r");
	
	exits(0);
}
