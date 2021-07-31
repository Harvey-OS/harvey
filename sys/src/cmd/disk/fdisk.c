/*
 * fdisk - edit disk partition table
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

enum {
	Maxpath = 4*NAMELEN,
};

typedef struct Tentry	Tentry;
typedef struct Table	Table;
typedef struct Type	Type;
typedef struct Disk	Disk;
typedef struct Part	Part;
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
	TypeEXTHUGE	= 0x0F,		/* huge extended partition */
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
	NTentry		= 4,
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
	int	changed;
	uint offset;	/* sector of table */
	uint i;		/* entry in table */
	
	u32int	lba;
	u32int	size;
	Tentry;
};

struct Disk {
	Part	*primary[NTentry];
	Part	*extended;
};

struct Part {
	Tab;

	/* if entry is TypeEXTENDED, the subpartition table contents */
	Tab	secondary;	/* secondary partition */
	Part *extended;	/* extended partition */
};

Type types[256] = {
	[TypeEMPTY]		{ "EMPTY", "" },
	[TypeFAT12]		{ "FAT12", "dos" },
	[TypeFAT16]		{ "FAT16", "dos" },
	[TypeFAT32]		{ "FAT32", "dos" },
	[TypeEXTHUGE]		{ "EXTHUGE", "" },
	[TypeEXTENDED]		{ "EXTENDED", "" },
	[TypeFATHUGE]		{ "FATHUGE", "dos" },
	[TypeBB]		{ "BB", "bb" },

	[TypeXENIX]		{ "XENIX", "xenix" },
	[TypeXENIXUSR]		{ "XENIX USR", "xenixusr" },
	[TypeHPFS]		{ "HPFS", "ntfs" },
	[TypeAIXBOOT]		{ "AIX BOOT", "aixboot" },
	[TypeAIXDATA]		{ "AIX DATA", "aixdata" },
	[TypeOS2BOOT]		{ "OS/2 BOOT", "os2boot" },
	[TypeUNFORMATTED]	{ "UNFORMATTED", "" },
	[TypeHPFS2]		{ "HPFS2", "hpfs2" },
	[TypeCPM0]		{ "CPM0", "cpm0" },
	[TypeDMDDO]		{ "DMDDO", "dmdd0" },
	[TypeGB]		{ "GB", "gb" },
	[TypeSPEEDSTOR]		{ "SPEEDSTOR", "speedstor" },
	[TypeSYSV386]		{ "SYSV386", "sysv386" },
	[TypeNETWARE]		{ "NETWARE", "netware" },
	[TypePCIX]		{ "PCIX", "pcix" },
	[TypeMINIX13]		{ "MINIX V1.3", "minix13" },
	[TypeMINIX]		{ "MINIX V1.5", "minix15" },
	[TypeLINUXSWAP]		{ "LINUX SWAP", "linuxswap" },
	[TypeLINUX]		{ "LINUX", "linux" },
	[TypeAMOEBA]		{ "AMOEBA", "amoeba" },
	[TypeAMOEBABB]		{ "AMOEBA BB", "amoebaboot" },
	[TypeBSD386]		{ "BSD386", "bsd386" },
	[TypeBSDI]		{ "BSDI", "bsdi" },
	[TypeBSDISWAP]		{ "BSDI SWAP", "bsdiswap" },
	[TypeCPM]		{ "CPM", "cpm" },
	[TypeSPEEDSTOR12]	{ "SPEEDSTOR12", "speedstor" },
	[TypeSPEEDSTOR16]	{ "SPEEDSTOR16", "speedstor" },
	[TypeLANSTEP]		{ "LANSTEP", "lanstep" },

	[Type9]			{ "PLAN9", "plan9" },
};

int diskfd;
int diskwfd;
vlong size;
vlong secsize;
vlong secs;
vlong mbroffset;
char *special;
uchar *secbuf;
int changed;
int width;
Disk disk;
int old;
int rdonly;

void
error(char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	doprint(buf, buf+sizeof buf, fmt, va);
	va_end(va);
	
	fprint(2, "prep: %s: %r\n", buf);
	exits(buf);
}

char*
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

u32int
getle32(void* v)
{
	uchar *p;

	p = v;
	return (p[3]<<24)|(p[2]<<16)|(p[1]<<8)|p[0];
}

void
putle32(void* v, u32int i)
{
	uchar *p;

	p = v;
	p[0] = i;
	p[1] = i>>8;
	p[2] = i>>16;
	p[3] = i>>24;
}

Tab
mktab(Tentry t, uint offset, int i)
{
	Tab tb;

	tb.Tentry = t;
	tb.lba = getle32(t.xlba)+offset;
	tb.size = getle32(t.xsize);
	tb.offset = offset;
	tb.i = i;
	tb.changed = 0;

	if(t.type == TypeEMPTY)
		return tb;

	return tb;
}

Part*
mkpart(Tentry *t, uint offset, int i)
{
	Part *p;

	p = malloc(sizeof *p);
	assert(p != nil);

	memset(p, 0, sizeof(*p));
	p->Tab = mktab(*t, offset, i);
	return p;
}

void
rdextend(Part *pp, uint lba, uint base)
{
	Table table;
	Tentry *tp;
	Part *p;
	int i;
	int havesec;

	seek(diskfd, (mbroffset+lba)*secsize+Toffset, 0);
	if(read(diskfd, &table, sizeof(Table)) < 0){
		fprint(2, "\twarning: read %s: %r\n", special);
		return;
	}

	if(table.magic[0] != Magic0 || table.magic[1] != Magic1){
		fprint(2, "\twarning: no extended partition table found\n");
		return;
	}

	havesec = 0;
	for(i=0, tp=table.entry; i<NTentry; i++, tp++) {
		switch(table.entry[i].type){
		case TypeEMPTY:
			break;			
		case TypeEXTENDED:
		case TypeEXTHUGE:
			if(pp->extended != nil) {
				fprint(2, "\twarning: ignoring second extended partition in table\n");
				break;
			}
			p = mkpart(&table.entry[i], base, i);
			pp->extended = p;
			rdextend(p, p->lba, base);
			break;
		default:
			if(havesec) {
				fprint(2, "\twarning: ignoring second secondary partition (%s)\n",
					typestr(table.entry[i].type));
				break;
			}
			pp->secondary = mktab(table.entry[i], lba, i);
			havesec = 1;
			break;
		}
	}
}

void
rdmbr(void)
{
	Table table;
	Tentry *tp;
	Part *p;
	int i;

	seek(diskfd, mbroffset*secsize+Toffset, 0);
	if(read(diskfd, &table, sizeof(Table)) < 0)
		error("read %s: %r\n", special);

	if(table.magic[0] != Magic0 || table.magic[1] != Magic1)
		error("no partition table found");

	for(i=0, tp=table.entry; i<NTentry; i++, tp++) {
		p = mkpart(&table.entry[i], 0, i);
		disk.primary[i] = p;
		if(p->type == TypeEXTENDED){
			rdextend(p, p->lba, p->lba);
			if(disk.extended != nil)
				fprint(2, "\twarning: more than one extended partition in root\n");
			else
				disk.extended = p;
		}
	}
}

void
rddosinfo(void)
{
	char name[Maxpath];
	Table table;
	Tentry *tp;

	sprint(name, "%sdisk", special);
	if((diskfd = open(name, ORDWR)) < 0)
		error("opening %s", name);
	seek(diskfd, Toffset, 0);
	if(read(diskfd, &table, sizeof(Table)) < 0)
		error("read %s: %r\n", name);
	if(table.magic[0] != Magic0 || table.magic[1] != Magic1)
		error("did not find master boot record");

	for(tp = table.entry; tp < &table.entry[NTentry]; tp++)
		if(tp->type == TypeDMDDO)
			mbroffset = 63;

	rdmbr();
}

void
rddiskinfo(void)
{
	Dir d;
	char name[Maxpath];

	if(strcmp(special + strlen(special) - 4, "disk") == 0)
		*(special + strlen(special) - 4) = 0;
	sprint(name, "%sdisk", special);
	if(dirstat(name, &d) < 0)
		error("stating %s", name);
	size = d.length;
	sprint(name, "%spartition", special);
	if(dirstat(name, &d) < 0)
		error("stating %s", name);
	secsize = d.length;
	secs = size/secsize;
	if(secbuf == 0)
		secbuf = malloc(secsize+1);

	diskfd = open(name, OREAD);
	if(diskfd < 0)
		error("opening %s", name);
	if(rdonly == 0) {
		diskwfd = open(name, OWRITE);
		if(diskwfd < 0)
			rdonly = 1;
	}
}

typedef struct Name Name;
struct Name {
	char name[NAMELEN];
	Name *link;
};

Name *namelist;

void
dumpplan9tab(Tab *t, char*)
{
	int i, ok;
	Name *n;
	char name[NAMELEN], *ty;

	if(t->type == TypeEMPTY)
		return;

	ty = types[t->type].name;
	if(ty == nil)
		return;

	if(strcmp(ty, "") == 0)
		return;

	i = 0;
	strcpy(name, ty);
	do {
		ok = 1;
		for(n=namelist; n; n=n->link) {
			if(strcmp(name, n->name) == 0) {
				i++;
				sprint(name, "%s.%d", ty, i);
				ok = 0;
			}
		}
	} while(ok == 0);

	n = malloc(sizeof(*n));
	assert(n != nil);
	strcpy(n->name, name);
	n->link = namelist;
	namelist = n;

	print("part %s %ud %ud\n", name, t->lba, t->lba+t->size);
}

void
dumptab(Tab *t, char *prefix)
{
	fprint(2, "%s ", prefix);
	if(t->type == TypeEMPTY)
		fprint(2, "empty\n");
	else
		fprint(2, "%*d - %*d %s\n", width, t->lba, width, t->lba+t->size, typestr(t->type));
}

void
dumppart(Part *p, void (*dump)(Tab*, char*), char *prefix)
{
	dump(&p->Tab, prefix);
	if(p->type == TypeEXTENDED) {
		dump(&p->secondary, "\t  ");
		if(p->extended)
			dumppart(p->extended, dump, "\t  ");
	}
}

void
dumpdisk(void (*dump)(Tab*, char*))
{
	int i;
	Part *p;
	char pref[10];

	for(i=0; i<NTentry; i++) {
		p = disk.primary[i];
		sprint(pref, "\t%c%c%d. ", p->changed ? '\'' : ' ', p->active&0x80 ? '*' : ' ', i+1);
		dumppart(disk.primary[i], dump, pref);
	}
}

char*
getline(void)
{
	static char buf[128];
	int n;

	n = read(0, buf, sizeof buf);
	if(n <= 0)
		return nil;
	if(n > 127)
		n = 127;
	buf[n] = '\0';
	return buf;
}

void
cmdprint(int n)
{
	char prefix[10];
	sprint(prefix, "\t%d. ", n+1);

	if(n == 0)
		dumpdisk(dumptab);
	else
		dumppart(disk.primary[n-1], dumptab, prefix);
}

void
cmddelete(int n)
{
	if(n == 0){
		fprint(2, "\t?specify a partition to delete\n");
		return;
	}
	disk.primary[n-1]->type = TypeEMPTY;
	disk.primary[n-1]->changed = 1;
	changed = 1;
}

char *help = 
	"\t#A	make partition active\n"
	"\t#a	add plan9 partition\n"
	"\t#d	delete partition\n"
	"\th	print this message\n"
	"\t[#]p	print partition info\n"
	"\ts	print disk usage summary\n"
	"\tw	write partition table to disk\n"
	"\tq	quit\n";

void
cmdhelp(int)
{
	write(2, help, strlen(help));
}

Part*
nextpartition(int first)
{
	int i;
	Part *p, *r;

	r = nil;
	for(i=0; i<NTentry; i++) {
		p = disk.primary[i];
		if(p->type != TypeEMPTY
		&& first <= p->lba && (r == nil || p->lba < r->lba))
			r = p;
	}
	return r;
}

#define KB 1000
#define MB (KB*1000)
#define GB (MB*1000)
void
printsum(uint a, uint b, char *s)
{
	vlong sz;
	char *suf;
	vlong div;

	sz = (b-a)*secsize;
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
	fprint(2, "\t%*ud - %*ud %lld.%.2d %s %s\n", width, a, width, b, sz/div, (int)((sz/(div/100))%100), suf, s);
}

void
cmdsummary(int n)
{
	uint first;
	uint lastlba;
	Part *last;
	Part *p;

	if(n!=0){
		fprint(2, "?s takes no argument\n");
		return;
	}

	first = 0;
	lastlba = 0;
	last = nil;
	while(p = nextpartition(lastlba)) {
		if(last && p->lba < first)
			fprint(2, "warning: partition %d overlaps partition %d\n", p->i+1, last->i+1);
		if(first > 63 && first < p->lba)
			printsum(first, p->lba, "empty");
		printsum(p->lba, p->lba+p->size, typestr(p->type));
		first = p->lba+p->size;
		lastlba = p->lba+1;
		last = p;
	}

	if(first < secs)
		printsum(first, secs, "empty");
}

ulong
getnum(char *s)
{
	char *p;
	fprint(2, "\t%s? ", s);
	p = getline();
	if(p == nil)
		exits(0);
	return strtoul(p, nil, 0);
}

void
writetab(Tab t)
{
	putle32(t.xlba, t.lba);
	putle32(t.xsize, t.size);
	if(seek(diskwfd, t.offset+Toffset+sizeof(Tentry)*t.i, 0) < 0) {
		fprint(2, "\tcould not seek in disk!\n");
		exits("seek");
	}
	if(write(diskwfd, &t.Tentry, sizeof(Tentry)) != sizeof(Tentry)) {
		fprint(2, "\terror writing disk\n");
		exits("write");
	}
}

void
cmdadd(int n)
{
	Part *p;
	char *q;
	ulong begin, end;
	int i;

	if(n == 0){
		fprint(2, "\t?specify partition number to create\n");
		return;
	}

	p = disk.primary[n-1];
	if(p->type != TypeEMPTY){
		fprint(2, "\t?partition already exists; delete it before creating a new one\n");
		return;
	}

	fprint(2, "\tfirst sector? ");
	q = getline();
	if(q == nil)
		exits(0);
	begin = strtoul(q, nil, 0);
	fprint(2, "\tlast sector+1? ");
	q = getline();
	if(q == nil)
		exits(0);
	while(isspace(*q))
		q++;
	if(*q == '+')
		end = begin + strtoul(q+1, 0, 0);
	else
		end = strtoul(q, 0, 0);

	if(begin >= end) {
		fprint(2, "?start can't be after last\n");
		return;
	}

	for(i=0; i<NTentry; i++){
		p = disk.primary[i];
		if(p->type != TypeEMPTY && begin < p->lba+p->size && p->lba < end){
			fprint(2, "\t?that overlaps partition %d\n", i+1);	
			return;
		}
	}

	p = disk.primary[n-1];
	p->type = Type9;
	p->lba = begin;
	p->size = end - begin;
	p->startc = p->starth = p->starts = 0xFF;
	p->endc = p->endh = p->ends = 0xFF;
	p->extended = nil;
	p->changed = 1;
	changed = 1;
}

void
cmdactive(int n)
{	
	int i;

	if(n == 0){
		fprint(2, "\t?specify partition to make active\n");
		return;
	}

	if(disk.primary[n-1]->type == TypeEMPTY){
		fprint(2, "\t?partition is empty; not making active\n");
		return;
	}

	if(disk.primary[n-1]->active & 0x80)
		return;

	for(i=0; i<NTentry; i++) {
		if(disk.primary[i]->active & 0x80) {
			disk.primary[i]->active &= ~0x80;
			disk.primary[i]->changed = 1;
		}
	}
	
	disk.primary[n-1]->active |= 0x80;
	disk.primary[n-1]->changed = 1;
	changed = 1;
}

void
cmdwrite(int n)
{
	char *p;
	int i;

	if(n != 0) {
		fprint(2, "\t?you can't write one partition at a time\n");
		return;
	}

	if(!changed){
		fprint(2, "\t?no changes to write\n");
		return;
	}

	if(rdonly){
		fprint(2, "?read only\n");
		return;
	}

	fprint(2, "\tare you sure you want to write the partition table [y/n]? ");
	p = getline();
	if(p == 0)
		exits(0);
	if(strcmp(p, "y\n") != 0)
		fprint(2, "\tpartition table not written\n");
	else {
		for(i=0; i<NTentry; i++) {
			if(disk.primary[i]->changed)
				writetab(disk.primary[i]->Tab);
			disk.primary[i]->changed = 0;
		}
		changed = 0;
		fprint(2, "\tpartition table written\n");
	}
}

void
cmdquit(int n)
{
	if(n != 0) {
		fprint(2, "\t?\n");
		return;
	}

	if(changed)
		fprint(2, "\n *** WARNING: partition table changed, not written ***\n");
	exits(0);
}


typedef struct Cmd	Cmd;
struct Cmd {
	char c;
	void (*fn)(int addr);
};

Cmd cmd[] = {
	'?',	cmdhelp,
	'A',	cmdactive,
	'a',	cmdadd,
	'd',	cmddelete,
	'h',	cmdhelp,
	'p',	cmdprint,
	's',	cmdsummary,
	'w',	cmdwrite,
	'q',	cmdquit,
};

void
usage(void)
{
	fprint(2, "disk/fdisk [-p] prefix\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *p;
	int i, n, dump;
	char buf[100];

	dump = 0;
	ARGBEGIN{
	case 'p':
		dump = 1;
		break;
	case 'r':
		rdonly = 1;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	special = argv[0];
	rddiskinfo();
	rddosinfo();

	sprint(buf, "%lld", secs);
	width = strlen(buf);

	if(dump){
		dumpdisk(dumpplan9tab);
		exits(0);
	}

	fprint(2, "\tsector = %lld bytes, disk = %lld sectors, all numbers are in sectors\n", secsize, size/secsize);

	cmdprint(0);
	fprint(2, "\n");
	cmdsummary(0);
	while(p = getline()) {
		while(isspace(*p))
			p++;

		if(isdigit(*p))
			n = strtol(p, &p, 10);
		else
			n = 0;

		if(n < 0 || n > NTentry){
			fprint(2, "\t?bad disk number\n");
			continue;
		}

		for(i=0; i<nelem(cmd); i++) {
			if(p[0] == cmd[i].c && p[1] == '\n') {
				cmd[i].fn(n);
				break;
			}
		}
		if(i == nelem(cmd))
			fprint(2, "\t?unknown command\n");
	}
	cmdquit(0);
}
