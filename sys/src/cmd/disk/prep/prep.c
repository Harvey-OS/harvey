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

static int	blank;
static int	file;
static int	doautox;
static int	printflag;
static Part	**opart;
static int	nopart;
static char	*osecbuf;
static char	*secbuf;
static int	rdonly;
static int	dowrite;
static int	docache;
static int	donvram;

static void	autoxpart(Edit*);
static Part	*mkpart(char*, vlong, vlong, int);
static void	rdpart(Edit*);
static void	wrpart(Edit*);
static void	checkfat(Disk*);

static void 	cmdsum(Edit*, Part*, vlong, vlong);
static char 	*cmdadd(Edit*, char*, vlong, vlong);
static char 	*cmddel(Edit*, Part*);
static char 	*cmdokname(Edit*, char*);
static char 	*cmdwrite(Edit*);
static char	*cmdctlprint(Edit*, int, char**);

Edit edit = {
	.add=	cmdadd,
	.del=	cmddel,
	.okname=cmdokname,
	.sum=	cmdsum,
	.write=	cmdwrite,

	.unit=	"sector",
};

typedef struct Auto Auto;
struct Auto
{
	char	*name;
	uvlong	min;
	uvlong	max;
	uint	weight;
	uchar	alloc;
	uvlong	size;
};

#define TB (1024LL*GB)
#define GB (1024*1024*1024)
#define MB (1024*1024)
#define KB (1024)

/*
 * Order matters -- this is the layout order on disk.
 */
Auto autox[] = 
{
	{	"9fat",		10*MB,	100*MB,	10,	},
	{	"nvram",	512,	512,	1,	},
	{	"fscfg",	1024,	8192,	1,	},
	{	"fs",		200*MB,	0,	10,	},
	{	"fossil",	200*MB,	0,	4,	},
	{	"arenas",	500*MB,	0,	20,	},
	{	"isect",	25*MB,	0,	1,	},
	{	"bloom",	4*MB,	512*MB,	1,	},

	{	"other",	200*MB,	0,	4,	},
	{	"swap",		100*MB,	512*MB,	1,	},
	{	"cache",	50*MB,	1*GB,	2,	},
};

void
usage(void)
{
	fprint(2, "usage: disk/prep [-bcfprw] [-a partname]... [-s sectorsize] /dev/sdC0/plan9\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	char *p;
	Disk *disk;
	vlong secsize;

	secsize = 0;
	ARGBEGIN{
	case 'a':
		p = EARGF(usage());
		for(i=0; i<nelem(autox); i++){
			if(strcmp(p, autox[i].name) == 0){
				if(autox[i].alloc){
					fprint(2, "you said -a %s more than once.\n", p);
					usage();
				}
				autox[i].alloc = 1;
				break;
			}
		}
		if(i == nelem(autox)){
			fprint(2, "don't know how to create automatic partition %s\n", p);
			usage();
		}
		doautox = 1;
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
	if(disk == nil)
		sysfatal("cannot open disk: %r");

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

	if(doautox)
		autoxpart(&edit);

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

static void
cmdsum(Edit *edit, Part *p, vlong a, vlong b)
{
	vlong sz, div;
	char *suf, *name;
	char c;

	c = p && p->changed ? '\'' : ' ';
	name = p ? p->name : "empty";

	sz = (b-a)*edit->disk->secsize;
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
		if (sz < 0)
			fprint(2, "%s: negative size!\n", argv0);
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
	vlong a, b;
	char *line[128];
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
autoxpart(Edit *edit)
{
	int i, totw, futz;
	vlong secs, secsize, s;
	char *err;

	if(edit->npart > 0) {
		if(doautox)
			fprint(2, "partitions already exist; not repartitioning\n");
		return;
	}

	secs = edit->disk->secs;
	secsize = edit->disk->secsize;
	for(;;){
		/* compute total weights */
		totw = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			totw += autox[i].weight;
		}
		if(totw == 0)
			break;

		if(secs <= 0){
			fprint(2, "ran out of disk space during autoxpartition.\n");
			return;
		}

		/* assign any minimums for small disks */
		futz = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			if(s < autox[i].min/secsize){
				autox[i].size = autox[i].min/secsize;
				secs -= autox[i].size;
				futz = 1;
				break;
			}
		}
		if(futz)
			continue;

		/* assign any maximums for big disks */
		futz = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			if(autox[i].max && s > autox[i].max/secsize){
				autox[i].size = autox[i].max/secsize;
				secs -= autox[i].size;
				futz = 1;
				break;
			}
		}
		if(futz)
			continue;

		/* finally, assign partition sizes according to weights */
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			autox[i].size = s;

			/* use entire disk even in face of rounding errors */
			secs -= autox[i].size;
			totw -= autox[i].weight;
		}
	}

	for(i=0; i<nelem(autox); i++)
		if(autox[i].alloc)
			print("%s %llud\n", autox[i].name, autox[i].size);

	s = 0;
	for(i=0; i<nelem(autox); i++){
		if(autox[i].alloc == 0)
			continue;
		if(err = addpart(edit, mkpart(autox[i].name, s, s+autox[i].size, 1)))
			fprint(2, "addpart %s: %s\n", autox[i].name, err);
		s += autox[i].size;
	}
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
