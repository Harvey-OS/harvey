/*
 * prep - prepare plan9 disk partition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "edit.h"

enum {
	Maxpath = 128,
};

static int		blank;
static int		file;
static int		doauto;
static int		printflag;
static Part		**opart;
static int		nopart;
static char	*osecbuf;
static char	*secbuf;
static int		rdonly;
static int		dowrite;
static int		docache;
static int		donvram;

static void		autopart(Edit*);
static vlong	memsize(void);
static Part		*mkpart(char*, vlong, vlong, int);
static void		rdpart(Edit*);
static void		wrpart(Edit*);
static void		checkfat(Disk*);

static void 	cmdsum(Edit*, Part*, vlong, vlong);
static char 	*cmdadd(Edit*, char*, vlong, vlong);
static char 	*cmddel(Edit*, Part*);
static char 	*cmdokname(Edit*, char*);
static char 	*cmdwrite(Edit*);
static char	*cmdctlprint(Edit*, int, char**);

Edit edit = {
	.add=	cmdadd,
	.del=		cmddel,
	.okname=	cmdokname,
	.sum=	cmdsum,
	.write=	cmdwrite,

	.unit=	"sector",
};

void
usage(void)
{
	fprint(2, "usage: disk/prep [-abcfprw] [-s sectorsize] /dev/sdC0/plan9\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	Disk *disk;
	vlong secsize;

	secsize = 0;
	ARGBEGIN{
	case 'a':
		doauto++;
		break;
	case 'b':
		blank++;
		break;
	case 'c':
		docache++;
		break;
	case 'f':
		file++;
		break;
	case 'n':
		donvram++;
		break;
	case 'p':
		printflag++;
		rdonly++;
		break;
	case 'r':
		rdonly++;
		break;
	case 's':
		secsize = atoi(ARGF());
		break;
	case 'w':
		dowrite++;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	disk = opendisk(argv[0], rdonly, file);
	if(disk == nil) {
		fprint(2, "cannot open disk: %r\n");
		exits("opendisk");
	}

	if(secsize != 0) {
		disk->secsize = secsize;
		disk->secs = disk->size / secsize;
	}
	edit.end = disk->secs;

	checkfat(disk);

	secbuf = emalloc(disk->secsize+1);
	osecbuf = emalloc(disk->secsize+1);
	edit.disk = disk;

	if(blank == 0)
		rdpart(&edit);

	opart = emalloc(edit.npart*sizeof(opart[0]));

	/* save old partition table */
	for(i=0; i<edit.npart; i++)
		opart[i] = edit.part[i];
	nopart = edit.npart;

	if(printflag) {
		runcmd(&edit, "P");
		exits(0);
	}

	if(doauto)
		autopart(&edit);

	if(dowrite) {
		runcmd(&edit, "w");
		exits(0);
	}

	runcmd(&edit, "p");
	for(;;) {
		fprint(2, ">>> ");
		runcmd(&edit, getline(&edit));
	}
}

#define GB (1024*1024*1024)
#define MB (1024*1024)
#define KB (1024)

static void
cmdsum(Edit *edit, Part *p, vlong a, vlong b)
{
	vlong sz, div;
	char *suf, *name;
	char c;

	c = p && p->changed ? '\'' : ' ';
	name = p ? p->name : "empty";

	sz = (b-a)*edit->disk->secsize;
	if(sz >= 1*GB){
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
		print("%c %-12s %*lld %-*lld (%lld sectors, %lld %s)\n", c, name,
			edit->disk->width, a, edit->disk->width, b, b-a, sz, suf);
	else
		print("%c %-12s %*lld %-*lld (%lld sectors, %lld.%.2d %s)\n", c, name,
			edit->disk->width, a, edit->disk->width, b, b-a,
			sz/div, (int)(((sz%div)*100)/div), suf);
}

static char*
cmdadd(Edit *edit, char *name, vlong start, vlong end)
{
	if(start < 2 && strcmp(name, "9fat") != 0)
		return "overlaps with the pbs and/or the partition table";

	return addpart(edit, mkpart(name, start, end, 1));
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

static char isfrog[256]={
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1,
	[' ']	1,
	['/']	1,
	[0x7f]	1,
};

static char*
cmdokname(Edit*, char *elem)
{
	for(; *elem; elem++)
		if(isfrog[*(uchar*)elem])
			return "bad character in name";
	return nil;
}

/*
 *  return memory size in bytes
 */
static vlong
memsize(void)
{
	int fd, n, by2pg;
	char *p;
	char buf[128];
	vlong mem;

	p = getenv("cputype");
	if(p && (strcmp(p, "68020") == 0 || strcmp(p, "alpha") == 0))
		by2pg = 8*1024;
	else
		by2pg = 4*1024;

	mem = 64*1024*1024;
	fd = open("/dev/swap", OREAD);
	if(fd < 0)
		return mem;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return mem;
	buf[n] = 0;
	p = strchr(buf, '/');
	if(p)
		mem = strtoul(p+1, 0, 0) * (vlong)by2pg;
	return mem;
}

static Part*
mkpart(char *name, vlong start, vlong end, int changed)
{
	Part *p;

	p = emalloc(sizeof(*p));
	p->name = estrdup(name);
	p->ctlname = estrdup(name);
	p->start = start;
	p->end = end;
	p->changed = changed;
	return p;
}

/* plan9 partition is first sector of the disk */
static void
rdpart(Edit *edit)
{
	int i, nline, nf, waserr;
	char *line[128];
	vlong a, b;
	char *f[5];
	char *err;
	Disk *disk;

	disk = edit->disk;
	seek(disk->fd, disk->secsize, 0);
	if(readn(disk->fd, osecbuf, disk->secsize) != disk->secsize)
		return;
	osecbuf[disk->secsize] = '\0';
	memmove(secbuf, osecbuf, disk->secsize+1);

	if(strncmp(secbuf, "part", 4) != 0){
		fprint(2, "no plan9 partition table found\n");
		return;
	}

	waserr = 0;
	nline = getfields(secbuf, line, nelem(line), 1, "\n");
	for(i=0; i<nline; i++){
		if(strncmp(line[i], "part", 4) != 0) {
		Error:
			if(waserr == 0)
				fprint(2, "syntax error reading partition\n");
			waserr = 1;
			continue;
		}

		nf = getfields(line[i], f, nelem(f), 1, " \t\r");
		if(nf != 4 || strcmp(f[0], "part") != 0)
			goto Error;

		a = strtoll(f[2], 0, 0);
		b = strtoll(f[3], 0, 0);
		if(a >= b)
			goto Error;

		if(err = addpart(edit, mkpart(f[1], a, b, 0))) {
			fprint(2, "?%s: not continuing\n", err);
			exits("partition");
		}
	}
}

static vlong
min(vlong a, vlong b)
{
	if(a < b)
		return a;
	return b;
}

static void
autopart(Edit *edit)
{
	vlong fat, fs, swap, secs, secsize, cache, nvram;
	char *err;

	if(edit->npart > 0) {
		if(doauto)
			fprint(2, "partitions already exist; not repartitioning\n");
		return;
	}

	if(doauto == 0)
		fprint(2, "initializing default plan9 partition table\n");

	secs = edit->disk->secs;
	secsize = edit->disk->secsize;

	fat = min(secs/10, (10*1024*1024)/secsize)+2;
	swap = min(secs/5, memsize()/secsize);
	fs = secs - fat - swap;
	if(docache) {
		cache = fs/2;
		fs -= cache;
	} else
		cache = 0;
	if(donvram) {
		nvram = 1;
		fs -= nvram;
	} else
		nvram = 0;

	if(err = addpart(edit, mkpart("9fat", 0, fat, 1)))
		fprint(2, "autopart: %s\n", err);
	if(err = addpart(edit, mkpart("fs", fat, fat+fs, 1)))
		fprint(2, "autopart: %s\n", err);
	if(cache && (err = addpart(edit, mkpart("cache", fat+fs, fat+fs+cache, 1))))
		fprint(2, "autopart: %s\n", err);
	if(nvram && (err = addpart(edit, mkpart("nvram", fat+fs+cache, fat+fs+cache+nvram, 1))))
		fprint(2, "autopart: %s\n", err);
	if(err = addpart(edit, mkpart("swap", fat+fs+cache+nvram, fat+fs+cache+nvram+swap, 1)))
		fprint(2, "autopart: %s\n", err);
}

static void
restore(Edit *edit, int ctlfd)
{
	int i;
	vlong offset;

	offset = edit->disk->offset;
	fprint(2, "attempting to restore partitions to previous state\n");
	if(seek(edit->disk->wfd, edit->disk->secsize, 0) != 0){
		fprint(2, "cannot restore: error seeking on disk\n");
		exits("inconsistent");
	}

	if(write(edit->disk->wfd, osecbuf, edit->disk->secsize) != edit->disk->secsize){
		fprint(2, "cannot restore: couldn't write old partition table to disk\n");
		exits("inconsistent");
	}

	if(ctlfd >= 0){
		for(i=0; i<edit->npart; i++)
			fprint(ctlfd, "delpart %s", edit->part[i]->name);
		for(i=0; i<nopart; i++){
			if(fprint(ctlfd, "part %s %lld %lld", opart[i]->name, opart[i]->start+offset, opart[i]->end+offset) < 0){
				fprint(2, "restored disk partition table but not kernel; reboot\n");
				exits("inconsistent");
			}
		}
	}
	exits("restored");
}

static void
wrpart(Edit *edit)
{
	int i, n;
	Disk *disk;

	disk = edit->disk;

	memset(secbuf, 0, disk->secsize);
	n = 0;
	for(i=0; i<edit->npart; i++)
		n += snprint(secbuf+n, disk->secsize-n, "part %s %lld %lld\n", 
			edit->part[i]->name, edit->part[i]->start, edit->part[i]->end);

	if(seek(disk->wfd, disk->secsize, 0) != disk->secsize){
		fprint(2, "error seeking %d %lld on disk: %r\n", disk->wfd, disk->secsize);
		exits("seek");
	}

	if(write(disk->wfd, secbuf, disk->secsize) != disk->secsize){
		fprint(2, "error writing partition table to disk\n");
		restore(edit, -1);
	}

	if(ctldiff(edit, disk->ctlfd) < 0)
		fprint(2, "?warning: partitions could not be updated in devsd\n");
}

/*
 * Look for a boot sector in sector 1, as would be
 * the case if editing /dev/sdC0/data when that
 * was really a bootable disk.
 */
static void
checkfat(Disk *disk)
{
	uchar buf[32];

	if(seek(disk->fd, disk->secsize, 0) < 0
	|| read(disk->fd, buf, sizeof(buf)) < sizeof(buf))
		return;

	if(buf[0] != 0xEB || buf[1] != 0x3C || buf[2] != 0x90)
		return;

	fprint(2, 
		"there's a fat partition where the\n"
		"plan9 partition table would go.\n"
		"if you really want to overwrite it, zero\n"
		"the second sector of the disk and try again\n");

	exits("fat partition");
}
