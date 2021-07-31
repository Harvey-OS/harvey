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
	NTentry = 4
};

static void rdpart(Edit*);
static void	findmbr(Edit*);
static void autopart(Edit*);
static void wrpart(Edit*);
static void blankpart(Edit*);
static void cmdnamectl(Edit*);

static int blank;
static int dowrite;
static int file;
static int rdonly;
static int doauto;
static vlong mbroffset;
static int printflag;
static int printchs;
static Dospart *primary[NTentry];

static void 	cmdsum(Edit*, Part*, vlong, vlong);
static char 	*cmdadd(Edit*, char*, vlong, vlong);
static char 	*cmddel(Edit*, Part*);
static char 	*cmdext(Edit*, int, char**);
static char 	*cmdhelp(Edit*);
static char 	*cmdokname(Edit*, char*);
static char 	*cmdwrite(Edit*);
static void		cmdprintctl(Edit*, int);

Edit edit = {
	.add=	cmdadd,
	.del=		cmddel,
	.ext=		cmdext,
	.help=	cmdhelp,
	.okname=	cmdokname,
	.sum=	cmdsum,
	.write=	cmdwrite,
	.printctl=	cmdprintctl,
};

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

	findmbr(&edit);

	if(blank)
		blankpart(&edit);
	else
		rdpart(&edit);

	if(doauto)
		autopart(&edit);

	if(dowrite)
		wrpart(&edit);

	if(printflag)
		runcmd(&edit, "P");

	if(dowrite || printflag)
		exits(0);

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

struct Tentry {
	uchar	active;			/* active flag */
	uchar	starth;			/* starting head */
	uchar	starts;			/* starting sector */
	uchar	startc;			/* starting cylinder */
	uchar	type;			/* partition type */
	uchar	endh;			/* ending head */
	uchar	ends;			/* ending sector */
	uchar	endc;			/* ending cylinder */
	uchar	xlba[4];			/* starting LBA from beginning of disc */
	uchar	xsize[4];		/* size in sectors */
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
	TypeEXTHUGE	= 0x0F,		/* FAT 32 extended partition */
	TypeUNFORMATTED	= 0x16,		/* unformatted primary partition (OS/2 FDISK)? */
	TypeHPFS2	= 0x17,
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
	TypeAMOEBA	= 0x93,
	TypeAMOEBABB	= 0x94,
	TypeBSD386	= 0xA5,
	TypeBSDI	= 0xB7,
	TypeBSDISWAP	= 0xB8,
	TypeCPM		= 0xDB,
	TypeSPEEDSTOR12	= 0xE1,
	TypeSPEEDSTOR16	= 0xE4,
	TypeLANSTEP	= 0xFE,

	Type9		= 0x39,

	Toffset		= 446,		/* offset of partition table in sector */
	Magic0		= 0x55,
	Magic1		= 0xAA,
};

struct Table {
	Tentry	entry[NTentry];
	uchar	magic[2];
};

struct Type {
	char *desc;
	char *name;
};

struct Tab {
	Tentry;
	uint offset;	/* sector of table */
	uint i;		/* entry in table */
	
	u32int	lba;
	u32int	size;
};

struct Dospart {
	Part;
	Tab;

	Tab secondary;
	Dospart *extend;	
};

static Type types[256] = {
	[TypeEMPTY]		{ "EMPTY", "" },
	[TypeFAT12]		{ "FAT12", "dos" },
	[TypeFAT16]		{ "FAT16", "dos" },
	[TypeFAT32]		{ "FAT32", "dos" },
	[TypeFAT32LBA]		{ "FAT32LBA", "dos" },
	[TypeEXTHUGE]		{ "EXTHUGE", "" },
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
	[TypeAMOEBA]		{ "AMOEBA", "amoeba" },
	[TypeAMOEBABB]		{ "AMOEBABB", "amoebaboot" },
	[TypeBSD386]		{ "BSD386", "bsd386" },
	[TypeBSDI]		{ "BSDI", "bsdi" },
	[TypeBSDISWAP]		{ "BSDISWAP", "bsdiswap" },
	[TypeCPM]		{ "CPM", "cpm" },
	[TypeSPEEDSTOR12]	{ "SPEEDSTOR12", "speedstor" },
	[TypeSPEEDSTOR16]	{ "SPEEDSTOR16", "speedstor" },
	[TypeLANSTEP]		{ "LANSTEP", "lanstep" },

	[Type9]			{ "PLAN9", "plan9" },
};

static char*
typestr(int type)
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

static Tab
mktab(Tentry t, uint offset, uint base, int i)
{
	Tab tb;

	tb.Tentry = t;
	tb.lba = getle32(t.xlba)+base;
	tb.size = getle32(t.xsize);
	tb.offset = offset;
	tb.i = i;

	if(t.type == TypeEMPTY)
		return tb;

	return tb;
}

static Dospart*
mkpart(Tentry *t, uint offset, uint base, int i)
{
	Dospart *p;

	p = malloc(sizeof *p);
	assert(p != nil);

	memset(p, 0, sizeof(*p));
	p->Tab = mktab(*t, offset, base, i);
	p->changed = 0;
	p->start = p->lba;
	p->end = p->lba+p->size;
	p->extend = nil;
	memset(&p->secondary, 0, sizeof(p->secondary));

	return p;
}

static int next;	/* no. of extended partitions */
static void
rdextend(Disk *disk, Dospart *pp, uint lba, uint base)
{
	Table table;
	Tentry *tp;
	Dospart *p;
	int i;
	int havesec;

	seek(disk->fd, (mbroffset+lba)*disk->secsize+Toffset, 0);
	if(read(disk->fd, &table, sizeof(Table)) < 0){
		fprint(2, "warning: read: %r\n");
		return;
	}

	if(table.magic[0] != Magic0 || table.magic[1] != Magic1){
	//	fprint(2, "warning: no extended partition table found\n");
		return;
	}

	havesec = 0;
	for(i=0, tp=table.entry; i<NTentry; i++, tp++) {
		switch(table.entry[i].type){
		case TypeEMPTY:
			break;			
		case TypeEXTENDED:
		case TypeEXTHUGE:
			if(pp->extend != nil) {
				fprint(2, "warning: ignoring second extended partition in table\n");
				break;
			}
			p = mkpart(&table.entry[i], lba, base, i);
			pp->extend = p;
			sprint(p->name, "e%d", ++next);
			rdextend(disk, p, p->lba, base);
			break;
		default:
			if(havesec) {
			//	fprint(2, "warning: ignoring second secondary partition (%s)\n",
			//		typestr(table.entry[i].type));
				break;
			}
			pp->secondary = mktab(table.entry[i], lba, base, i);
			havesec = 1;
			break;
		}
	}
}

Table ombr;

static void
rdpart(Edit *edit)
{
	Table table;
	Tentry *tp;
	Dospart *p;
	int i;
	char *err;

	seek(edit->disk->fd, mbroffset*edit->disk->secsize+Toffset, 0);
	if(read(edit->disk->fd, &table, sizeof(Table)) < 0)
		sysfatal("read: %r\n");

	if(table.magic[0] != Magic0 || table.magic[1] != Magic1)
		sysfatal("no partition table found");

	ombr = table;

	for(i=0, tp=table.entry; i<NTentry; i++, tp++) {
		p = mkpart(&table.entry[i], mbroffset, 0, i);
		sprint(p->name, "p%d", i+1);
		primary[i] = p;
		if(tp->type == TypeEMPTY)
			continue;
		if(err = addpart(edit, p))
			sysfatal("reading mbr: %s\n", err);
		if(tp->type == TypeEXTENDED || tp->type == TypeEXTHUGE)
			rdextend(edit->disk, p, p->lba, p->lba);
	}
}

static void
blankpart(Edit *edit)
{
	int i;
	Tentry t;
	Dospart *p;

	memset(&t, 0, sizeof t);
	for(i=0; i<NTentry; i++) {
		p = mkpart(&t, mbroffset, 0, i);
		sprint(p->name, "p%d", i+1);
		primary[i] = p;
		p->changed = 1;
	}
	edit->changed = 1;
}

static void
findmbr(Edit *edit)
{
	Table table;
	Tentry *tp;
	Tentry t;
	Dospart *p;

	seek(edit->disk->fd, Toffset, 0);
	if(read(edit->disk->fd, &table, sizeof(Table)) < 0)
		sysfatal("read: %r\n");
	if(table.magic[0] != Magic0 || table.magic[1] != Magic1)
		sysfatal("did not find master boot record");

	for(tp = table.entry; tp < &table.entry[NTentry]; tp++)
		if(tp->type == TypeDMDDO)
			mbroffset = 63;

	memset(&t, 0, sizeof t);
	putle32(t.xlba, 0);
	putle32(t.xsize, mbroffset+edit->disk->s);
	p = mkpart(&t, mbroffset, 0, 0);
	sprint(p->name, "mbr");
	addpart(edit, p);
}

static void
restore(Disk *disk)
{
	if(seek(disk->wfd, mbroffset*disk->secsize+Toffset, 0) < 0) {
		fprint(2, "could not seek; disk may be inconsistent\n");
		exits("inconsistent");
	}
	if(write(disk->wfd, &ombr, sizeof(ombr)) < 0) {
		fprint(2, "error restoring mbr; disk may be inconsistent\n");
		exits("inconsistent");
	}

	fprint(2, "restored old master boot record\n");
	exits("restored");
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
fixchs(Disk *disk, Dospart *p)
{
	p->lba = p->start;
	p->size = p->end - p->start;

	if(disk->c && disk->h && disk->s) {
		writechs(disk, &p->starth, p->start);
		writechs(disk, &p->endh, p->end-1);
	}
	putle32(p->xlba, p->lba);
	putle32(p->xsize, p->size);
}

static void
writedos(Disk *disk, Dospart *p)
{
	if(seek(disk->wfd, (mbroffset+p->offset)*disk->secsize+Toffset+sizeof(Tentry)*p->i, 0) < 0) {
		fprint(2, "could not seek in disk!\n");
		restore(disk);
	}
	if(write(disk->wfd, &p->Tentry, sizeof(Tentry)) != sizeof(Tentry)) {
		fprint(2, "error writing disk!\n");
		restore(disk);
	}
}

static void
wrpart(Edit *edit)
{
	int i;
	Disk *disk;

	disk = edit->disk;
	if(seek(disk->wfd, mbroffset*disk->secsize+Toffset+NTentry*sizeof(Tentry), 0) < 0) {
		fprint(2, "error seeking to magic bytes %lld: %r\n",
			mbroffset*disk->secsize+Toffset+NTentry*sizeof(Tentry));
		exits("restored");
	}

	if(write(disk->wfd, "\x55\xAA", 2) != 2) {
		fprint(2, "error writing disk\n");
		exits("restored");
	}
	
	for(i=0; i<NTentry; i++) {
		if(primary[i]->changed) {
			writedos(disk, primary[i]);
			primary[i]->changed = 0;
		}
	}

	cmdprintctl(edit, disk->ctlfd);
}

static void
autopart(Edit *edit)
{
	int i, active;
	vlong bigstart, bigsize;
	vlong start;
	Part *p;

	for(i=0; i<NTentry; i++)
		if(primary[i]->type == Type9)
			return;

	/* look for biggest gap after the usual empty track */
	start = edit->disk->s;
	bigsize = 0;
	SET(bigstart);
	for(i=0; i<edit->npart; i++) {
		p = edit->part[i];
		if(p->start > start && p->start - start > bigsize) {
			bigsize = p->start - start;
			bigstart = start;
		}
		start = p->end;
	}

	if(edit->disk->secs - start > bigsize) {
		bigsize = edit->disk->secs - start;
		bigstart = start;
	}
	if(bigsize < (1024*1024)/edit->disk->secsize) {
		fprint(2, "couldn't find room for plan9 partition\n");
		return;
	}

	active = Active;
	for(i=0; i<NTentry; i++) 
		if(primary[i]->active & Active)
			active = 0;

	for(i=0; i<NTentry; i++) {
		if(primary[i]->type == TypeEMPTY) {
			primary[i]->start = bigstart;
			primary[i]->end = bigstart+bigsize;
			primary[i]->changed = 1;
			primary[i]->type = Type9;
			primary[i]->active = active;
			fixchs(edit->disk, primary[i]);
			addpart(edit, primary[i]);
			break;
		}
	}
	if(i == NTentry) {
		fprint(2, "couldn't find room in partition table to add plan9 partition\n");
		return;
	}
}

typedef struct Name Name;
struct Name {
	char name[NAMELEN];
	Name *link;
};
Name *namelist;
static void
plan9print(Dospart *part, char *vname, vlong start, vlong end, int fd)
{
	int i, ok;
	char name[NAMELEN];
	Name *n;

	i = 0;
	strcpy(name, vname);
	do {
		ok = 1;
		for(n=namelist; n; n=n->link) {
			if(strcmp(name, n->name) == 0) {
				i++;
				sprint(name, "%s.%d", vname, i);
				ok = 0;
			}
		}
	} while(ok == 0);

	n = malloc(sizeof(*n));
	assert(n != nil);
	strcpy(n->name, name);
	n->link = namelist;
	namelist = n;

	strcpy(part->ctlname, name);

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
printtab(Dospart *part, Tab *p, int fd)
{
	char *ty;

	assert(p->type != TypeEMPTY);

	ty = types[p->type].name;
	if(ty == nil)
		return;

	if(strcmp(ty, "") == 0) {
		strcpy(part->ctlname, "");
		return;
	}

	plan9print(part, ty, mbroffset+p->lba, mbroffset+p->lba+p->size, fd);
}

static void
plan9namepart(Dospart *p, int fd, int isextra)
{
	if(strcmp(p->name, "mbr") == 0)
		return;

	printtab(p, &p->Tab, isextra ? fd : -1);
	if(p->type == TypeEXTENDED || p->type == TypeEXTHUGE) {
		printtab(p, &p->secondary, 0);
		if(p->extend) {
			assert(p->extend != p);
			plan9namepart(p->extend, fd, 1);
		}
	}
}

static void
cmdprintctl(Edit *edit, int ctlfd)
{
	int i;

	freenamelist();
	for(i=0; i<edit->npart; i++)
		plan9namepart((Dospart*)edit->part[i], -1, 0);
	ctldiff(edit, ctlfd);
	for(i=0; i<edit->npart; i++)
		plan9namepart((Dospart*)edit->part[i], ctlfd, 0);
}

static char*
cmdokname(Edit*, char *name)
{
	if(strlen(name) != 2 || name[0] != 'p' || name[1] < '1' || name[1] > '4')
		return "name must be p1, p2, p3, or p4";
	return nil;
}

#define GB (1024*1024*1024)
#define MB (1024*1024)
#define KB (1024)

static void
printsumline(Disk *disk, char *s, char *name, Dospart *p, vlong a, vlong b, char *ty)
{
	vlong sz, div;
	char *suf;

	USED(p);

	sz = (b-a)*disk->secsize;
	if(sz > 1*GB){
		suf = "GB";
		div = GB;
	}else if(sz > 1*MB){
		suf = "MB";
		div = MB;
	}else if(sz > 1*KB){
		suf = "KB";
		div = KB;
	}else{
		suf = "B ";
		div = 1;
	}

	if(div == 1)
		print("%s %-12s %*lld %-*lld (%lld sectors, %lld %s) %s\n", s, name,
			disk->width, a, disk->width, b, 
			b-a, sz, suf, ty);
	else
		print("%s %-12s %*lld %-*lld (%lld sectors, %lld.%.2d %s) %s\n", s, name,
			disk->width, a, disk->width, b,  b-a, sz/div, (int)((sz/(div/100))%100),
			suf, ty);

	if(printchs && p != nil)
		print("\t%d/%d/%d %d/%d/%d\n",
			p->startc | ((p->starts<<2) & 0x300),
			p->starth,
			p->starts & 63,
			p->endc | ((p->ends<<2) & 0x300),
			p->endh,
			p->ends & 63);
}

static void
sumextend(Disk *disk, Dospart *p)
{
	if(p->secondary.type != TypeEMPTY)
		printsumline(disk, "  ", "", p, p->secondary.lba, p->secondary.lba+p->secondary.size, typestr(p->secondary.type));
	if(p->extend)
		sumextend(disk, p->extend);
}

static void
cmdsum(Edit *edit, Part *vp, vlong a, vlong b)
{
	char *name, *ty;
	char buf[3];
	Dospart *p;

	p = (Dospart*)vp;

	buf[0] = p && p->changed ? '\'' : ' ';
	buf[1] = p && (p->active & Active) ? '*' : ' ';
	buf[2] = '\0';

	name = p ? p->name : "empty";
	ty = p ? typestr(p->type) : "";

	printsumline(edit->disk, buf, name, p, a, b, ty);
	if(p)
		sumextend(edit->disk, p);
}

static char*
cmdadd(Edit *edit, char *name, vlong start, vlong end)
{
	int i, sh;
	char *err;
	static char cylerr[128];

	i = name[1]-'1';
	assert(primary[i]->type == TypeEMPTY);

	sh = edit->disk->s * edit->disk->h;
	if(start % sh || end % sh) {
		/*
		 * The first partition can start at C/H/S 0/1/1; I guess
		 * it gets special treatment because of the MBR at 0/0/1.
		 */
		if(start < edit->disk->h)
			start = edit->disk->h;
		else if(start % sh)
			start += sh - (start % sh);
		end -= end % sh;
		sprint(cylerr, "partition must be aligned on cylinder boundaries; try a %s %lld %lld\n",
			name, start, end);
		return cylerr;
	}

	primary[i]->start = start;
	primary[i]->end = end;
	fixchs(edit->disk, primary[i]);
	if(err = addpart(edit, primary[i]))
		return err;
	primary[i]->type = Type9;
	primary[i]->changed = 1;
	return nil;
}

static char*
cmddel(Edit *edit, Part *p)
{
	int i;
	char *err;

	if(strcmp(p->name, "mbr") == 0)
		return "cannot delete mbr track";

	for(i=0; i<NTentry; i++) {
		if(primary[i] == p) {
			err = delpart(edit, primary[i]);
			if(err == nil) {
				primary[i]->type = TypeEMPTY;
				primary[i]->start = 0;
				primary[i]->end = 0;
				primary[i]->changed = 1;
				primary[i]->extend = nil;
				primary[i]->secondary.type = TypeEMPTY;
			}
			return err;
		}
	}
	assert(0);
	return "no such partition";
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
	Dospart *p;
	int i;

	if(nf != 2)
		return "args";

	p = (Dospart*) findpart(edit, f[1]);
	if(p == nil)
		return "unknown name";

	if(p->type == TypeEMPTY)
		return "can't set empty partition active";

	for(i=0; i<NTentry; i++) {
		if((primary[i]->active & Active) && primary[i] != p) {
			primary[i]->active &= ~Active;
			primary[i]->changed = 1;
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
cmdempty(Edit*, int nf, char**)
{
	int i, n;

	if(nf != 1)
		return "args";

	print("empty partitions: ");
	n = 0;
	for(i=0; i<NTentry; i++) {
		if(primary[i]->type == TypeEMPTY) {
			print(" p%d", i+1);
			n++;
		}
	}
	if(n == 0)
		print(" none\n");
	else
		print("\n");
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
	case 'e':
		return cmdempty(edit, nf, f);
	case 't':
		return cmdtype(edit, nf, f);
	default:
		return "unknown command";
	}
}

