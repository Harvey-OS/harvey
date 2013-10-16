/*
 * fdisk - edit dos disk partition table
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <disk.h>
#include "edit.h"

typedef struct Dospart	Dospart;
enum {
	NTentry = 4,
	Mpart = 64,
};

static void rdpart(Edit*, uvlong, uvlong);
static void findmbr(Edit*);
static void autopart(Edit*);
static void wrpart(Edit*);
static void blankpart(Edit*);
static void cmdnamectl(Edit*);
static void recover(Edit*);
static int Dfmt(Fmt*);
static int blank;
static int dowrite;
static int file;
static int rdonly;
static int doauto;
static vlong mbroffset;
static int printflag;
static int printchs;
static int sec2cyl;
static int written;

static void 	cmdsum(Edit*, Part*, vlong, vlong);
static char 	*cmdadd(Edit*, char*, vlong, vlong);
static char 	*cmddel(Edit*, Part*);
static char 	*cmdext(Edit*, int, char**);
static char 	*cmdhelp(Edit*);
static char 	*cmdokname(Edit*, char*);
static char 	*cmdwrite(Edit*);
static void	cmdprintctl(Edit*, int);

#pragma varargck type "D" uchar*

Edit edit = {
	.add=	cmdadd,
	.del=		cmddel,
	.ext=		cmdext,
	.help=	cmdhelp,
	.okname=	cmdokname,
	.sum=	cmdsum,
	.write=	cmdwrite,
	.printctl=	cmdprintctl,

	.unit=	"cylinder",
};

/*
 * Catch the obvious error routines to fix up the disk.
 */
void
sysfatal(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(argv0)
		fprint(2, "%s: %s\n", argv0, buf);
	else
		fprint(2, "%s\n", buf);

	if(written)
		recover(&edit);

	exits(buf);
}

void
abort(void)
{
	fprint(2, "abort\n");
	recover(&edit);
}

void
usage(void)
{
	fprint(2, "usage: disk/fdisk [-abfprvw] [-s sectorsize] /dev/sdC0/data\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	vlong secsize;

	secsize = 0;
	ARGBEGIN{
	case 'a':
		doauto++;
		break;
	case 'b':
		blank++;
		break;
	case 'f':
		file++;
		break;
	case 'p':
		printflag++;
		break;
	case 'r':
		rdonly++;
		break;
	case 's':
		secsize = atoi(ARGF());
		break;
	case 'v':
		printchs++;
		break;
	case 'w':
		dowrite++;
		break;
	}ARGEND;

	fmtinstall('D', Dfmt);

	if(argc != 1)
		usage();

	edit.disk = opendisk(argv[0], rdonly, file);
	if(edit.disk == nil) {
		fprint(2, "cannot open disk: %r\n");
		exits("opendisk");
	}

	if(secsize != 0) {
		edit.disk->secsize = secsize;
		edit.disk->secs = edit.disk->size / secsize;
	}

	sec2cyl = edit.disk->h * edit.disk->s;
	edit.end = edit.disk->secs / sec2cyl;

	findmbr(&edit);

	if(blank)
		blankpart(&edit);
	else
		rdpart(&edit, 0, 0);

	if(doauto)
		autopart(&edit);

	if(dowrite)
		runcmd(&edit, "w");

	if(printflag)
		runcmd(&edit, "P");

	if(dowrite || printflag)
		exits(0);

	fprint(2, "cylinder = %lld bytes\n", sec2cyl*edit.disk->secsize);
	runcmd(&edit, "p");
	for(;;) {
		fprint(2, ">>> ");
		runcmd(&edit, getline(&edit));
	}
}

typedef struct Tentry	Tentry;
typedef struct Table	Table;
typedef struct Type	Type;
typedef struct Tab	Tab;
typedef struct Recover Recover;

struct Tentry {
	uchar	active;			/* active flag */
	uchar	starth;			/* starting head */
	uchar	starts;			/* starting sector */
	uchar	startc;			/* starting cylinder */
	uchar	type;			/* partition type */
	uchar	endh;			/* ending head */
	uchar	ends;			/* ending sector */
	uchar	endc;			/* ending cylinder */
	uchar	xlba[4];		/* starting LBA from beginning of disc or ext. partition */
	uchar	xsize[4];		/* size in sectors */
};

struct Table {
	Tentry	entry[NTentry];
	uchar	magic[2];
	uchar	size[];
};

enum {
	Active		= 0x80,		/* partition is active */
	Primary		= 0x01,		/* internal flag */

	TypeBB		= 0xFF,

	TypeEMPTY	= 0x00,
	TypeFAT12	= 0x01,
	TypeXENIX	= 0x02,		/* root */
	TypeXENIXUSR	= 0x03,		/* usr */
	TypeFAT16	= 0x04,
	TypeEXTENDED	= 0x05,
	TypeFATHUGE	= 0x06,
	TypeHPFS	= 0x07,
	TypeAIXBOOT	= 0x08,
	TypeAIXDATA	= 0x09,
	TypeOS2BOOT	= 0x0A,		/* OS/2 Boot Manager */
	TypeFAT32	= 0x0B,		/* FAT 32 */
	TypeFAT32LBA	= 0x0C,		/* FAT 32 needing LBA support */
	TypeFAT16X	= 0x0E,		/* FAT 16 needing LBA support */
	TypeEXTHUGE	= 0x0F,		/* FAT 32 extended partition */
	TypeUNFORMATTED	= 0x16,		/* unformatted primary partition (OS/2 FDISK)? */
	TypeHPFS2	= 0x17,
	TypeIBMRecovery = 0x1C,		/* really hidden fat */
	TypeCPM0	= 0x52,
	TypeDMDDO	= 0x54,		/* Disk Manager Dynamic Disk Overlay */
	TypeGB		= 0x56,		/* ???? */
	TypeSPEEDSTOR	= 0x61,
	TypeSYSV386	= 0x63,		/* also HURD? */
	TypeNETWARE	= 0x64,
	TypePCIX	= 0x75,
	TypeMINIX13	= 0x80,		/* Minix v1.3 and below */
	TypeMINIX	= 0x81,		/* Minix v1.5+ */
	TypeLINUXSWAP	= 0x82,
	TypeLINUX	= 0x83,
	TypeLINUXEXT	= 0x85,
	TypeLINUXLVM	= 0x8E,		/* logical volume manager */
	TypeAMOEBA	= 0x93,
	TypeAMOEBABB	= 0x94,
	TypeBSD386	= 0xA5,
	TypeNETBSD	= 0xA9,
	TypeBSDI	= 0xB7,
	TypeBSDISWAP	= 0xB8,
	TypeOTHER	= 0xDA,
	TypeCPM		= 0xDB,
	TypeDellRecovery= 0xDE,
	TypeSPEEDSTOR12	= 0xE1,
	TypeSPEEDSTOR16	= 0xE4,
	TypeEFIProtect	= 0xEE,
	TypeEFI		= 0xEF,
	TypeLANSTEP	= 0xFE,

	Type9		= 0x39,

	Toffset		= 446,		/* offset of partition table in sector */
	Magic0		= 0x55,
	Magic1		= 0xAA,

	Tablesz		= offsetof(Table, size[0]),
};

struct Type {
	char *desc;
	char *name;
};

struct Dospart {
	Part;
	Tentry;

	u32int	lba;
	u32int	size;
	int		primary;
};

struct Recover {
	Table	table;
	ulong	lba;
};

static Type types[256] = {
	[TypeEMPTY]		{ "EMPTY", "" },
	[TypeFAT12]		{ "FAT12", "dos" },
	[TypeFAT16]		{ "FAT16", "dos" },
	[TypeFAT32]		{ "FAT32", "dos" },
	[TypeFAT32LBA]		{ "FAT32LBA", "dos" },
	[TypeFAT16X]		{ "FAT16X", "dos" },
	[TypeEXTHUGE]		{ "EXTHUGE", "" },
	[TypeIBMRecovery]	{ "IBMRECOVERY", "ibm" },
	[TypeEXTENDED]		{ "EXTENDED", "" },
	[TypeFATHUGE]		{ "FATHUGE", "dos" },
	[TypeBB]		{ "BB", "bb" },

	[TypeXENIX]		{ "XENIX", "xenix" },
	[TypeXENIXUSR]		{ "XENIX USR", "xenixusr" },
	[TypeHPFS]		{ "HPFS", "ntfs" },
	[TypeAIXBOOT]		{ "AIXBOOT", "aixboot" },
	[TypeAIXDATA]		{ "AIXDATA", "aixdata" },
	[TypeOS2BOOT]		{ "OS/2BOOT", "os2boot" },
	[TypeUNFORMATTED]	{ "UNFORMATTED", "" },
	[TypeHPFS2]		{ "HPFS2", "hpfs2" },
	[TypeCPM0]		{ "CPM0", "cpm0" },
	[TypeDMDDO]		{ "DMDDO", "dmdd0" },
	[TypeGB]		{ "GB", "gb" },
	[TypeSPEEDSTOR]		{ "SPEEDSTOR", "speedstor" },
	[TypeSYSV386]		{ "SYSV386", "sysv386" },
	[TypeNETWARE]		{ "NETWARE", "netware" },
	[TypePCIX]		{ "PCIX", "pcix" },
	[TypeMINIX13]		{ "MINIXV1.3", "minix13" },
	[TypeMINIX]		{ "MINIXV1.5", "minix15" },
	[TypeLINUXSWAP]		{ "LINUXSWAP", "linuxswap" },
	[TypeLINUX]		{ "LINUX", "linux" },
	[TypeLINUXEXT]		{ "LINUXEXTENDED", "" },
	[TypeLINUXLVM]		{ "LINUXLVM", "linuxlvm" },
	[TypeAMOEBA]		{ "AMOEBA", "amoeba" },
	[TypeAMOEBABB]		{ "AMOEBABB", "amoebaboot" },
	[TypeBSD386]		{ "BSD386", "bsd386" },
	[TypeNETBSD]		{ "NETBSD", "netbsd" },
	[TypeBSDI]		{ "BSDI", "bsdi" },
	[TypeBSDISWAP]		{ "BSDISWAP", "bsdiswap" },
	[TypeOTHER]		{ "OTHER", "other" },
	[TypeCPM]		{ "CPM", "cpm" },
	[TypeDellRecovery]	{ "DELLRECOVERY", "dell" },
	[TypeSPEEDSTOR12]	{ "SPEEDSTOR12", "speedstor" },
	[TypeSPEEDSTOR16]	{ "SPEEDSTOR16", "speedstor" },
	[TypeEFIProtect]	{ "EFIPROTECT", "efiprotect" },
	[TypeEFI]		{ "EFI", "efi" },
	[TypeLANSTEP]		{ "LANSTEP", "lanstep" },

	[Type9]			{ "PLAN9", "plan9" },
};

static Dospart	part[Mpart];
static int		npart;

static char*
typestr0(int type)
{
	static char buf[100];

	sprint(buf, "type %d", type);
	if(type < 0 || type >= 256)
		return buf;
	if(types[type].desc == nil)
		return buf;
	return types[type].desc;
}

static u32int
getle32(void* v)
{
	uchar *p;

	p = v;
	return (p[3]<<24)|(p[2]<<16)|(p[1]<<8)|p[0];
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
diskread(Disk *disk, void *data, int ndata, u32int sec, u32int off)
{
	if(seek(disk->fd, (vlong)sec*disk->secsize+off, 0) != (vlong)sec*disk->secsize+off)
		sysfatal("diskread seek %lud.%lud: %r", (ulong)sec, (ulong)off);
	if(readn(disk->fd, data, ndata) != ndata)
		sysfatal("diskread %lud at %lud.%lud: %r", (ulong)ndata, (ulong)sec, (ulong)off);
}

static int
diskwrite(Disk *disk, void *data, int ndata, u32int sec, u32int off)
{
	written = 1;
	if(seek(disk->wfd, (vlong)sec*disk->secsize+off, 0) != (vlong)sec*disk->secsize+off)
		goto Error;
	if(write(disk->wfd, data, ndata) != ndata)
		goto Error;
	return 0;

Error:
	fprint(2, "write %d bytes at %lud.%lud failed: %r\n", ndata, (ulong)sec, (ulong)off);
	return -1;
}

static Dospart*
mkpart(char *name, int primary, vlong lba, vlong size, Tentry *t)
{
	static int n;
	Dospart *p;

	p = emalloc(sizeof(*p));
	if(name)
		p->name = estrdup(name);
	else{
		p->name = emalloc(20);
		sprint(p->name, "%c%d", primary ? 'p' : 's', ++n);
	}

	if(t)
		p->Tentry = *t;
	else
		memset(&p->Tentry, 0, sizeof(Tentry));

	p->changed = 0;
	p->start = lba/sec2cyl;
	p->end = (lba+size)/sec2cyl;
	p->ctlstart = lba;
	p->ctlend = lba+size;
	p->lba = lba;
	if (p->lba != lba)
		fprint(2, "%s: start of partition (%lld) won't fit in MBR table\n", argv0, lba);
	p->size = size;
	if (p->size != size)
		fprint(2, "%s: size of partition (%lld) won't fit in MBR table\n", argv0, size);
	p->primary = primary;
	return p;
}

/*
 * Recovery takes care of remembering what the various tables
 * looked like when we started, attempting to restore them when
 * we are finished.
 */
static Recover	*rtab;
static int		nrtab;

static void
addrecover(Table t, ulong lba)
{
	if((nrtab%8) == 0) {
		rtab = realloc(rtab, (nrtab+8)*sizeof(rtab[0]));
		if(rtab == nil)
			sysfatal("out of memory");
	}
	rtab[nrtab] = (Recover){t, lba};
	nrtab++;
}

static void
recover(Edit *edit)
{
	int err, i, ctlfd;
	vlong offset;

	err = 0;
	for(i=0; i<nrtab; i++)
		if(diskwrite(edit->disk, &rtab[i].table, Tablesz, rtab[i].lba, Toffset) < 0)
			err = 1;
	if(err) {
		fprint(2, "warning: some writes failed during restoration of old partition tables\n");
		exits("inconsistent");
	} else {
		fprint(2, "restored old partition tables\n");
	}

	ctlfd = edit->disk->ctlfd;
	offset = edit->disk->offset;
	if(ctlfd >= 0){
		for(i=0; i<edit->npart; i++)
			if(edit->part[i]->ctlname && fprint(ctlfd, "delpart %s", edit->part[i]->ctlname)<0)
				fprint(2, "delpart failed: %s: %r", edit->part[i]->ctlname);
		for(i=0; i<edit->nctlpart; i++)
			if(edit->part[i]->name && fprint(ctlfd, "delpart %s", edit->ctlpart[i]->name)<0)
				fprint(2, "delpart failed: %s: %r", edit->ctlpart[i]->name);
		for(i=0; i<edit->nctlpart; i++){
			if(fprint(ctlfd, "part %s %lld %lld", edit->ctlpart[i]->name,
				edit->ctlpart[i]->start+offset, edit->ctlpart[i]->end+offset) < 0){
				fprint(2, "restored disk partition table but not kernel; reboot\n");
				exits("inconsistent");
			}
		}
	}
	exits("restored");

}

/*
 * Read the partition table (including extended partition tables)
 * from the disk into the part array.
 */
static void
rdpart(Edit *edit, uvlong lba, uvlong xbase)
{
	char *err;
	Table table;
	Tentry *tp, *ep;
	Dospart *p;

	if(xbase == 0)
		xbase = lba;

	diskread(edit->disk, &table, Tablesz, mbroffset+lba, Toffset);
	addrecover(table, mbroffset+lba);

	if(table.magic[0] != Magic0 || table.magic[1] != Magic1) {
		assert(lba != 0);
		return;
	}

	for(tp=table.entry, ep=tp+NTentry; tp<ep && npart < Mpart; tp++) {
		switch(tp->type) {
		case TypeEMPTY:
			break;
		case TypeEXTENDED:
		case TypeEXTHUGE:
		case TypeLINUXEXT:
			rdpart(edit, xbase+getle32(tp->xlba), xbase);
			break;
		default:
			p = mkpart(nil, lba==0, lba+getle32(tp->xlba), getle32(tp->xsize), tp);
			if(err = addpart(edit, p))
				fprint(2, "adding partition: %s\n", err);
			break;
		}
	}
}

static void
blankpart(Edit *edit)
{
	edit->changed = 1;
}

static void
findmbr(Edit *edit)
{
	Table table;
	Tentry *tp;

	diskread(edit->disk, &table, Tablesz, 0, Toffset);
	if(table.magic[0] != Magic0 || table.magic[1] != Magic1)
		sysfatal("did not find master boot record");

	for(tp = table.entry; tp < &table.entry[NTentry]; tp++)
		if(tp->type == TypeDMDDO)
			mbroffset = edit->disk->s;
}

static int
haveroom(Edit *edit, int primary, vlong start)
{
	int i, lastsec, n;
	Dospart *p, *q;
	ulong pend, qstart;

	if(primary) {
		/*
		 * must be open primary slot.
		 * primary slots are taken by primary partitions
		 * and runs of secondary partitions.
		 */
		n = 0;
		lastsec = 0;
		for(i=0; i<edit->npart; i++) {
			p = (Dospart*)edit->part[i];
			if(p->primary)
				n++, lastsec=0;
			else if(!lastsec)
				n++, lastsec=1;
		}
		return n<4;
	}

	/* 
	 * secondary partitions can be inserted between two primary
	 * partitions only if there is an empty primary slot.
	 * otherwise, we can put a new secondary partition next
	 * to a secondary partition no problem.
	 */
	n = 0;
	for(i=0; i<edit->npart; i++){
		p = (Dospart*)edit->part[i];
		if(p->primary)
			n++;
		pend = p->end;
		if(i+1<edit->npart){
			q = (Dospart*)edit->part[i+1];
			qstart = q->start;
		}else{
			qstart = edit->end;
			q = nil;
		}
		if(start < pend || start >= qstart)
			continue;
		/* we go between these two */
		if(p->primary==0 || (q && q->primary==0))
			return 1;
	}
	/* not next to a secondary, need a new primary */
	return n<4;
}

static void
autopart(Edit *edit)
{
	char *err;
	int active, i;
	vlong bigstart, bigsize, start;
	Dospart *p;

	for(i=0; i<edit->npart; i++)
		if(((Dospart*)edit->part[i])->type == Type9)
			return;

	/* look for the biggest gap in which we can put a primary partition */
	start = 0;
	bigsize = 0;
	SET(bigstart);
	for(i=0; i<edit->npart; i++) {
		p = (Dospart*)edit->part[i];
		if(p->start > start && p->start - start > bigsize && haveroom(edit, 1, start)) {
			bigsize = p->start - start;
			bigstart = start;
		}
		start = p->end;
	}

	if(edit->end - start > bigsize && haveroom(edit, 1, start)) {
		bigsize = edit->end - start;
		bigstart = start;
	}
	if(bigsize < 1) {
		fprint(2, "couldn't find space or partition slot for plan 9 partition\n");
		return;
	}

	/* set new partition active only if no others are */
	active = Active;	
	for(i=0; i<edit->npart; i++)
		if(((Dospart*)edit->part[i])->primary && (((Dospart*)edit->part[i])->active & Active))
			active = 0;

	/* add new plan 9 partition */
	bigsize *= sec2cyl;
	bigstart *= sec2cyl;
	if(bigstart == 0) {
		bigstart += edit->disk->s;
		bigsize -= edit->disk->s;
	}
	p = mkpart(nil, 1, bigstart, bigsize, nil);
	p->active = active;
	p->changed = 1;
	p->type = Type9;
	edit->changed = 1;
	if(err = addpart(edit, p)) {
		fprint(2, "error adding plan9 partition: %s\n", err);
		return;
	}
}

typedef struct Name Name;
struct Name {
	char *name;
	Name *link;
};
Name *namelist;
static void
plan9print(Dospart *part, int fd)
{
	int i, ok;
	char *name, *vname;
	Name *n;
	vlong start, end;
	char *sep;

	vname = types[part->type].name;
	if(vname==nil || strcmp(vname, "")==0) {
		part->ctlname = "";
		return;
	}

	start = mbroffset+part->lba;
	end = start+part->size;

	/* avoid names like plan90 */
	i = strlen(vname) - 1;
	if(vname[i] >= '0' && vname[i] <= '9')
		sep = ".";
	else
		sep = "";

	i = 0;
	name = emalloc(strlen(vname)+10);

	sprint(name, "%s", vname);
	do {
		ok = 1;
		for(n=namelist; n; n=n->link) {
			if(strcmp(name, n->name) == 0) {
				i++;
				sprint(name, "%s%s%d", vname, sep, i);
				ok = 0;
			}
		}
	} while(ok == 0);

	n = emalloc(sizeof(*n));
	n->name = name;
	n->link = namelist;
	namelist = n;
	part->ctlname = name;

	if(fd >= 0)
		print("part %s %lld %lld\n", name, start, end);
}

static void
freenamelist(void)
{
	Name *n, *next;

	for(n=namelist; n; n=next) {
		next = n->link;
		free(n);
	}
	namelist = nil;
}

static void
cmdprintctl(Edit *edit, int ctlfd)
{
	int i;

	freenamelist();
	for(i=0; i<edit->npart; i++)
		plan9print((Dospart*)edit->part[i], -1);
	ctldiff(edit, ctlfd);
}

static char*
cmdokname(Edit*, char *name)
{
	char *q;

	if(name[0] != 'p' && name[0] != 's')
		return "name must be pN or sN";

	strtol(name+1, &q, 10);
	if(*q != '\0')
		return "name must be pN or sN";

	return nil;
}

#define TB (1024LL*GB)
#define GB (1024*1024*1024)
#define MB (1024*1024)
#define KB (1024)

static void
cmdsum(Edit *edit, Part *vp, vlong a, vlong b)
{
	char *name, *ty;
	char buf[3];
	char *suf;
	Dospart *p;
	vlong sz, div;

	p = (Dospart*)vp;

	buf[0] = p && p->changed ? '\'' : ' ';
	buf[1] = p && (p->active & Active) ? '*' : ' ';
	buf[2] = '\0';

	name = p ? p->name : "empty";
	ty = p ? typestr0(p->type) : "";

	sz = (b-a)*edit->disk->secsize*sec2cyl;
	if(sz >= 1*TB){
		suf = "TB";
		div = TB;
	}else if(sz >= 1*GB){
		suf = "GB";
		div = GB;
	}else if(sz >= 1*MB){
		suf = "MB";
		div = MB;
	}else if(sz >= 1*KB){
		suf = "KB";
		div = KB;
	}else{
		suf = "B ";
		div = 1;
	}

	if(div == 1)
		print("%s %-12s %*lld %-*lld (%lld cylinders, %lld %s) %s\n", buf, name,
			edit->disk->width, a, edit->disk->width, b, b-a, sz, suf, ty);
	else
		print("%s %-12s %*lld %-*lld (%lld cylinders, %lld.%.2d %s) %s\n", buf, name,
			edit->disk->width, a, edit->disk->width, b,  b-a,
			sz/div, (int)(((sz%div)*100)/div), suf, ty);
}

static char*
cmdadd(Edit *edit, char *name, vlong start, vlong end)
{
	Dospart *p;

	if(!haveroom(edit, name[0]=='p', start))
		return "no room for partition";
	start *= sec2cyl;
	end *= sec2cyl;
	if(start == 0 || name[0] != 'p')
		start += edit->disk->s;
	p = mkpart(name, name[0]=='p', start, end-start, nil);
	p->changed = 1;
	p->type = Type9;
	return addpart(edit, p);
}

static char*
cmddel(Edit *edit, Part *p)
{
	return delpart(edit, p);
}

static char*
cmdwrite(Edit *edit)
{
	wrpart(edit);
	return nil;
}

static char *help = 
	"A name - set partition active\n"
	"P - print table in ctl format\n"
	"R - restore disk back to initial configuration and exit\n"
	"e - show empty dos partitions\n"
	"t name [type] - set partition type\n";

static char*
cmdhelp(Edit*)
{
	print("%s\n", help);
	return nil;
}

static char*
cmdactive(Edit *edit, int nf, char **f)
{
	int i;
	Dospart *p, *ip;

	if(nf != 2)
		return "args";

	if(f[1][0] != 'p')
		return "cannot set secondary partition active";

	if((p = (Dospart*)findpart(edit, f[1])) == nil)
		return "unknown partition";

	for(i=0; i<edit->npart; i++) {
		ip = (Dospart*)edit->part[i];
		if(ip->active & Active) {
			ip->active &= ~Active;
			ip->changed = 1;
			edit->changed = 1;
		}
	}

	if((p->active & Active) == 0) {
		p->active |= Active;
		p->changed = 1;
		edit->changed = 1;
	}

	return nil;
}

static char*
strupr(char *s)
{
	char *p;

	for(p=s; *p; p++)
		*p = toupper(*p);
	return s;
}

static void
dumplist(void)
{
	int i, n;

	n = 0;
	for(i=0; i<256; i++) {
		if(types[i].desc) {
			print("%-16s", types[i].desc);
			if(n++%4 == 3)
				print("\n");
		}
	}
	if(n%4)
		print("\n");
}

static char*
cmdtype(Edit *edit, int nf, char **f)
{
	char *q;
	Dospart *p;
	int i;

	if(nf < 2)
		return "args";

	if((p = (Dospart*)findpart(edit, f[1])) == nil)
		return "unknown partition";

	if(nf == 2) {
		for(;;) {
			fprint(2, "new partition type [? for list]: ");
			q = getline(edit);
			if(q[0] == '?')
				dumplist();
			else
				break;
		}
	} else
		q = f[2];

	strupr(q);
	for(i=0; i<256; i++)
		if(types[i].desc && strcmp(types[i].desc, q) == 0)
			break;
	if(i < 256 && p->type != i) {
		p->type = i;
		p->changed = 1;
		edit->changed = 1;
	}
	return nil;
}

static char*
cmdext(Edit *edit, int nf, char **f)
{
	switch(f[0][0]) {
	case 'A':
		return cmdactive(edit, nf, f);
	case 't':
		return cmdtype(edit, nf, f);
	case 'R':
		recover(edit);
		return nil;
	default:
		return "unknown command";
	}
}

static int
Dfmt(Fmt *f)
{
	char buf[60];
	uchar *p;
	int c, h, s;

	p = va_arg(f->args, uchar*);
	h = p[0];
	c = p[2];
	c |= (p[1]&0xC0)<<2;
	s = (p[1] & 0x3F);

	sprint(buf, "%d/%d/%d", c, h, s);
	return fmtstrcpy(f, buf);
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
wrtentry(Disk *disk, Tentry *tp, int type, u32int xbase, u32int lba, u32int end)
{
	tp->type = type;
	writechs(disk, &tp->starth, lba);
	writechs(disk, &tp->endh, end-1);
	putle32(tp->xlba, lba-xbase);
	putle32(tp->xsize, end-lba);
}

static int
wrextend(Edit *edit, int i, vlong xbase, vlong startlba, vlong *endlba)
{
	int ni;
	Table table;
	Tentry *tp, *ep;
	Dospart *p;
	Disk *disk;

	if(i == edit->npart){
		*endlba = edit->disk->secs;
	Finish:
		if(startlba < *endlba){
			disk = edit->disk;
			diskread(disk, &table, Tablesz, mbroffset+startlba, Toffset);
			tp = table.entry;
			ep = tp+NTentry;
			for(; tp<ep; tp++)
				memset(tp, 0, sizeof *tp);
			table.magic[0] = Magic0;
			table.magic[1] = Magic1;

			if(diskwrite(edit->disk, &table, Tablesz, mbroffset+startlba, Toffset) < 0)
				recover(edit);
		}
		return i;
	}

	p = (Dospart*)edit->part[i];
	if(p->primary){
		*endlba = (vlong)p->start*sec2cyl;
		goto Finish;
	}

	disk = edit->disk;
	diskread(disk, &table, Tablesz, mbroffset+startlba, Toffset);
	tp = table.entry;
	ep = tp+NTentry;

	ni = wrextend(edit, i+1, xbase, p->end*sec2cyl, endlba);

	*tp = p->Tentry;
	wrtentry(disk, tp, p->type, startlba, startlba+disk->s, p->end*sec2cyl);
	tp++;

	if(p->end*sec2cyl != *endlba){
		memset(tp, 0, sizeof *tp);
		wrtentry(disk, tp, TypeEXTENDED, xbase, p->end*sec2cyl, *endlba);
		tp++;
	}

	for(; tp<ep; tp++)
		memset(tp, 0, sizeof *tp);

	table.magic[0] = Magic0;
	table.magic[1] = Magic1;

	if(diskwrite(edit->disk, &table, Tablesz, mbroffset+startlba, Toffset) < 0)
		recover(edit);
	return ni;
}	

static void
wrpart(Edit *edit)
{	
	int i, ni, t;
	Table table;
	Tentry *tp, *ep;
	Disk *disk;
	vlong s, endlba;
	Dospart *p;

	disk = edit->disk;

	diskread(disk, &table, Tablesz, mbroffset, Toffset);

	tp = table.entry;
	ep = tp+NTentry;
	for(i=0; i<edit->npart && tp<ep; ) {
		p = (Dospart*)edit->part[i];
		if(p->start == 0)
			s = disk->s;
		else
			s = p->start*sec2cyl;
		if(p->primary) {
			*tp = p->Tentry;
			wrtentry(disk, tp, p->type, 0, s, p->end*sec2cyl);
			tp++;
			i++;
		} else {
			ni = wrextend(edit, i, p->start*sec2cyl, p->start*sec2cyl, &endlba);
			memset(tp, 0, sizeof *tp);
			if(endlba >= 1024*sec2cyl)
				t = TypeEXTHUGE;
			else
				t = TypeEXTENDED;
			wrtentry(disk, tp, t, 0, s, endlba);
			tp++;
			i = ni;
		}
	}
	for(; tp<ep; tp++)
		memset(tp, 0, sizeof(*tp));
		
	if(i != edit->npart)
		sysfatal("cannot happen #1");

	if(diskwrite(disk, &table, Tablesz, mbroffset, Toffset) < 0)
		recover(edit);

	/* bring parts up to date */
	freenamelist();
	for(i=0; i<edit->npart; i++)
		plan9print((Dospart*)edit->part[i], -1);

	if(ctldiff(edit, disk->ctlfd) < 0)
		fprint(2, "?warning: partitions could not be updated in devsd\n");
}
