/*
 * Quick and dirty u-boot mkImage utility for gzipped Plan 9 kernels.
 * See https://github.com/u-boot/u-boot/blob/master/include/image.h
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include <flate.h>

enum {
	MAGIC		= 0x27051956,
	OS_OPENBSD	= 1,
	OS_LINUX	= 5,		/* only this starts all harts */
	OS_UBOOT	= 17,
	OS_PLAN9	= 23,

	IMAGETYPE_STAND	= 1,
	IMAGETYPE_KERNEL= 2,		/* only this starts all harts */
	IMAGETYPE_SCRIPT= 6,

	ARCH_ARM	= 2,
	ARCH_RISCV	= 26,

	COMPRESSION_NONE= 0,
	COMPRESSION_GZIP= 1,

	ADDR		= 0x80000000,
	EPADDR		= ADDR,			/* +0x20 for a.out */
	CRC_POLY	= 0xEDB88320,
};

typedef struct hdr Hdr;
struct hdr {
	uint	magic;
	uint	headercrc;
	uint	time;
	uint	size;
	uint	addr;		/* data load address */
	uint	epaddr;		/* entry point address */
	uint	datacrc;
	uchar	os, arch, imagetype, compression;
	char	name[32];
};
typedef struct scripthdr Scripthdr;
struct scripthdr {
	uint	size;
	uint	zero;
};

typedef struct Archmap Archmap;
struct Archmap {
	char	*objtype;
	int	archid;
};
Archmap archmap[] = {
	"arm",		ARCH_ARM,
	"arm64",	ARCH_ARM,
	"riscv",	ARCH_RISCV,
	"riscv64",	ARCH_RISCV,
};

ulong *crctab;

static int copy(int, int, uint*);

void
main(int argc, char **argv)
{
	Hdr *h;
	Scripthdr sh;
	Dir *dir;
	uint crc;
	int fin = 0, fout = 1;
	char *objtype;
	Archmap *amp;
	int script = 0;
	uint n;

	objtype = getenv("objtype");
	if (objtype == nil)
		objtype = getenv("cputype");
	ARGBEGIN {
	case 't':
		script = 1;
		break;
	}ARGEND
	switch (argc) {
	case 2:
		if ((fout = create(argv[1], OWRITE, 0666)) < 0)
			sysfatal("%r");
		/* fall through */
	case 1:
		if ((fin = open(argv[0], OREAD)) < 0)
			sysfatal("%r");
		/* fall through */
	case 0:
		break;
	default:
		fprint(2, "Usage: mkimage [-t] [9xxx.gz [uImage]]\n");
		exits("usage");
	}

	if ((dir = dirfstat(fin)) == nil)
		sysfatal("%s: %r", argv[1]);
	h = mallocz(sizeof *h, 1);
	hnputl(&h->magic, MAGIC);
	hnputl(&h->time, dir->mtime);
	n = (uint)dir->length;
	if (script){
		hnputl(&sh.size, n);
		sh.zero = 0;
		n += sizeof sh;
	}
	hnputl(&h->size, n);
	if (!script) {
		hnputl(&h->addr, ADDR);
		hnputl(&h->epaddr, EPADDR);
	}
//	h->os = OS_UBOOT;	/* starts hart 1 only */
//	h->os = OS_PLAN9;	/* starts hart 15 then dies */
	h->os = OS_LINUX;

	h->arch = ARCH_RISCV;
	for (amp = archmap; amp < archmap + nelem(archmap); amp++)
		if (objtype && strcmp(objtype, amp->objtype) == 0) {
			h->arch = amp->archid;
			break;
		}
	if (script)
		h->imagetype = IMAGETYPE_SCRIPT;
	else {
		h->imagetype = IMAGETYPE_KERNEL;
		h->compression = COMPRESSION_GZIP;
	}
	strncpy(h->name, dir->name, sizeof(h->name));

	crctab = mkcrctab(CRC_POLY);

	seek(fout, sizeof *h, 0);
	if (!script)
		crc = 0;
	else {
		crc = blockcrc(crctab, 0, &sh, sizeof sh);
		if (write(fout, &sh, sizeof sh) != sizeof sh)
			sysfatal("error writing script header");
	}
	if (copy(fin, fout, &crc) < 0)
		sysfatal("error writing output file");
	hnputl(&h->datacrc, crc);
	hnputl(&h->headercrc, blockcrc(crctab, 0, h, sizeof *h));
	if (seek(fout, 0, 0) != 0 || write(fout, h, sizeof *h) != sizeof *h)
		sysfatal("error writing u-boot header");
	exits(0);
}

static int
copy(int fin, int fout, uint *acrc)
{
	char buf[64*1024];
	uint crc;
	int n;
	
	crc = *acrc;
	while ((n = read(fin, buf, sizeof buf)) > 0) {
		crc = blockcrc(crctab, crc, buf, n);
		if (write(fout, buf, n) != n)
			return -1;
	}
	if (n < 0)
		return -1;
	*acrc = crc;
	return 0;
}
