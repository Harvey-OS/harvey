/*
 * 9load - load next kernel from disk and start it
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/error.h"
#include	"../port/netif.h"
#include	"dosfs.h"
#include	"../port/sd.h"

/* from <libc.h> */
#define	DIRMAX	(sizeof(Dir)+STATMAX)	/* max length of Dir structure */ 
#define	STATMAX	65535U	/* max length of machine-independent stat structure */

enum {
	Bufsize = 8192,
};

int	dosdirread(File *f, char ***nmarray);
int	isconf(char *name);

static int progress = 1;
static Bootfs fs;

/*
 * from 9load's bootp.c:
 */

static int
dumpfile(char *file)
{
	int n;
	char *buf;

	buf = smalloc(Maxfile + 1);
	n = readfile(file, buf, Maxfile);
	if (n < 0)
		return -1;
	buf[n] = 0;
	print("%s (%d bytes):\n", file, n);
	print("%s\n", buf);
	free(buf);
	return 0;
}

long
dirread0(Chan *c, uchar *p, long n)
{
	long nn, nnn;
	vlong off;

	/*
	 * The offset is passed through on directories, normally.
	 * Sysseek complains, but pread is used by servers like exportfs,
	 * that shouldn't need to worry about this issue.
	 *
	 * Notice that c->devoffset is the offset that c's dev is seeing.
	 * The number of bytes read on this fd (c->offset) may be different
	 * due to rewritings in rockfix.
	 */
	/* use and maintain channel's offset */
	off = c->offset;
	if(off < 0)
		error(Enegoff);

	if(off == 0){	/* rewind to the beginning of the directory */
		c->offset = 0;
		c->devoffset = 0;
		mountrewind(c);
		unionrewind(c);
	}

	if(c->qid.type & QTDIR){
		if(mountrockread(c, p, n, &nn)){
			/* do nothing: mountrockread filled buffer */
		}else if(c->umh)
			nn = unionread(c, p, n);
		else{
			if(off != c->offset)
				error(Edirseek);
			nn = devtab[c->type]->read(c, p, n, c->devoffset);
		}
		nnn = mountfix(c, p, nn, n);
	}else
		nnn = nn = devtab[c->type]->read(c, p, n, off);

	lock(c);
	c->devoffset += nn;
	c->offset += nnn;
	unlock(c);

	/* nnn == 54, sizeof(Dir) == 60 */
	return nnn;
}

long
dirread(Chan *c, Dir **d)
{
	uchar *buf;
	long ts;

	buf = malloc(DIRMAX);
	if(buf == nil)
		return -1;
	ts = dirread0(c, buf, DIRMAX);
	if(ts >= 0)
		/* convert machine-independent representation to Dirs */
		ts = dirpackage(buf, ts, d);
	free(buf);
	return ts;
}

static int
addsdev(Dir *dirp)
{
	int n, f, lines, flds;
	vlong start, end;
	char *buf, *part;
	char *line[64], *fld[5];
	char ctl[64], disk[64];

	buf = smalloc(Maxfile + 1);
	snprint(ctl, sizeof ctl, "#S/%s/ctl", dirp->name);
	n = readfile(ctl, buf, Maxfile);
	if (n < 0) {
		free(buf);
		return -1;
	}
	buf[n] = 0;

	lines = getfields(buf, line, nelem(line), 0, "\r\n");
	part = nil;
	for (f = 0; f < lines; f++) {
		flds = tokenize(line[f], fld, nelem(fld));
		if (flds < 4 || strcmp(fld[0], "part") != 0)
			continue;
		kstrdup(&part, fld[1]);
		start = strtoull(fld[2], nil, 0);
		end   = strtoull(fld[3], nil, 0);
		if (end > (vlong)100*(vlong)MB*MB) {
			print("addsdev: implausible partition #S/%s/%s %lld %lld\n",
				dirp->name, part, start, end);
			continue;
		}
		/*
		 * We are likely to only see a "data" partition on each disk.
		 *
		 * Read the on-disk partition tables & set in-core partitions
		 * (disk, part, start, end).
		 */
		print("found partition #S/%s/%s %,lld %,lld\n",
			dirp->name, part, start, end);
		snprint(disk, sizeof disk, "#S/%s", dirp->name);
		readparts(disk);
	}
	free(buf);
	return 0;
}

static File file;

/*
 * look for kernels on a 9fat; if there's just one, return it.
 * could treat x and x.gz as one kernel.
 */
static char *
findonekernel(Bootfs *fs)
{
	int n, kerns;
	char *bootfile, *name;
	char **array;

	if(fswalk(fs, "", &file) <= 0) {
		print("can't walk to ''\n");
		return nil;
	}
	dosdirread(&file, &array);
	bootfile = nil;
	kerns = 0;
	for (n = 0; (name = array[n]) != nil; n++)
		if(strncmp(name, "9pc", 3) == 0 ||
		   strncmp(name, "9k8", 3) == 0 ||
		   strncmp(name, "9k10", 4) == 0){
			bootfile = name;
			kerns++;
		}
	if (kerns > 1) {
		print("found these kernels:");
		for (n = 0; (name = array[n]) != nil; n++)
			print(" %s", name);
		print("\n");
	}
	return kerns == 1? bootfile: nil;
}

int
partboot(char *path)
{
	long n;
	char *buf;
	Boot boot;
	Boot *b;
	Chan *ch;

	b = &boot;
	memset(b, 0, sizeof *b);
	b->state = INITKERNEL;
	ch = namecopen(path, OREAD);
	if (ch == nil) {
		print("can't open partition %s\n", path);
		return -1;
	}
	print("loading %s\n", path);
	buf = smalloc(Bufsize);
	while((n = devtab[ch->type]->read(ch, buf, Bufsize, ch->offset)) > 0)
		if(bootpass(b, buf, n) != MORE)
			break;
	bootpass(b, nil, 0);		/* attempts to boot */

	free(buf);
	cclose(ch);
	return -1;
}

/* fsroot must be nil or a fat root directory already dosinit'ed */
static void
trybootfile(char *bootfile, Bootfs *fsroot)
{
	int nf;
	char fat[64];
	char *disk, *part, *file, *bootcopy;
	char *fields[4];
	Boot boot;
	static int didaddconf;

	bootcopy = file = nil;
	kstrdup(&bootcopy, bootfile);
	nf = getfields(bootcopy, fields, nelem(fields), 0, "!");
	switch(nf){
	case 3:
		file = fields[2];
		/* fall through */
	case 2:
		disk = fields[0];
		part = fields[1];
		break;
	default:
		print("bad bootfile syntax: %s\n", bootfile);
		return;
	}

	if(didaddconf == 0) {
		didaddconf = 1;
		sdaddallconfs(sdaddconf);
	}

	snprint(fat, sizeof fat, "#S/%s/%s", disk, part);
	if (file == nil) { /* if no file, try to load from partition directly */
		partboot(fat);
		return;
	}

	if (fsroot == nil) {
		fsroot = &fs;
		memset(fsroot, 0, sizeof *fsroot);
		if (dosinit(fsroot, fat) < 0) {
			print("dosinit %s failed\n", fat);
			return;
		}
	}

	/* load kernel and jump to it */
	memset(&boot, 0, sizeof boot);
	boot.state = INITKERNEL;
	fsboot(fsroot, file, &boot);

	/* failed to boot */
}

/*
 * for a given disk's 9fat, find & load plan9.ini, parse it,
 * extract kernel filename, load that kernel and jump to it.
 */
static void
trydiskboot(char *disk)
{
	int n;
	char fat[80];
	char *ini, *bootfile;

	/* mount the disk's 9fat */
	memset(&fs, 0, sizeof fs);
	snprint(fat, sizeof fat, "#S/%s/9fat", disk);
	if (dosinit(&fs, fat) < 0) {
		print("dosinit %s failed\n", fat);
		return;
	}

	/* open plan9.ini, read it */
	ini = smalloc(Maxfile+1);
	if(fswalk(&fs, "plan9.ini", &file) <= 0) {
		print("no plan9.ini in %s\n", fat);
		n = 0;
	} else {
		n = fsread(&file, ini, Maxfile);
		if (n < 0)
			panic("error reading %s", ini);
	}
	ini[n] = 0;

	/*
	 * take note of plan9.ini contents.  consumes ini to make config vars,
	 * thus we can't free ini.
	 */
	dotini(ini);
	i8250console();			/* (re)configure serial port */

	bootfile = nil;			/* for kstrdup in askbootfile */
	if(isconf("bootfile")) {
		kstrdup(&bootfile, getconf("bootfile"));
		if(strcmp(bootfile, "manual") == 0)
			askbootfile(fat, sizeof fat, &bootfile, 0, "");

		/* pass arguments to kernels that can use them */
		strecpy(BOOTLINE, BOOTLINE+BOOTLINELEN, bootfile);
	} else if ((bootfile = findonekernel(&fs)) != nil) {  /* look in fat */
		snprint(fat, sizeof fat, "%s!9fat!%s", disk, bootfile);
		bootfile = fat;
		print("no bootfile named in plan9.ini; found %s\n", bootfile);
	} else {
		/* if #S/disk/kernel partition exists, load from it. */
		snprint(fat, sizeof fat, "#S/%s/kernel", disk);
		partboot(fat);
		/* last resort: ask the user */
		askbootfile(fat, sizeof fat, &bootfile, Promptsecs,
			"sdC0!9fat!9pccpu");
	}
	trybootfile(bootfile, &fs);

	/* failed; try again */
}

/*
 * find all the disks in #S, read their partition tables and set those
 * partitions in core, mainly so that we can access 9fat file systems.
 * for each disk's 9fat, read plan9.ini and boot the named kernel.
 */
void
bootloadproc(void *)
{
	int n, dirs, sdev;
	char kern[64];
	char *sdevs[128];
	Chan *sdch;
	Dir *dirp, *dp;

	memset(sdevs, 0, sizeof sdevs);
	sdch = nil;
	while(waserror()) {
		print("error caught at top level in bootload\n");
		if(sdch) {
			cclose(sdch);
			sdch = nil;
		}
	}
	bind("#S", "/dev", MAFTER);		/* try to force an attach */
	sdch = namecopen("#S", OREAD);
	if (sdch == nil)
		panic("no disks (no #S)");
	sdev = 0;
	while ((dirs = dirread(sdch, &dirp)) > 0) {
		for (dp = dirp; dirs-- > 0; dp++)
			if (strcmp(dp->name, "sdctl") != 0) {
				addsdev(dp);
				if (sdev >= nelem(sdevs))
					print("too many sdevs; ignoring %s\n",
						dp->name);
				else
					kstrdup(&sdevs[sdev++], dp->name);
			}
		free(dirp);
	}
	cclose(sdch);
	sdch = nil;
	if (sdev == 0)
		panic("no disks (in #S)");

	print("disks:");
	for (n = 0; n < sdev; n++)
		print(" %s", sdevs[n]);
	print("\n");

	for (n = 0; n < sdev; n++) {
		print("trying %s...", sdevs[n]);
		trydiskboot(sdevs[n]);
	}
	USED(sdch);
	for (;;) {
		askbootfile(kern, sizeof kern, nil, 0, "");
		trybootfile(kern, nil);
	}
	// poperror();
}
