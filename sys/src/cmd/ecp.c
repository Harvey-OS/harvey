/*
 * ecp - copy a file fast (in big blocks), cope with errors, optionally verify.
 *
 * Transfers a block at a time.  On error, retries one sector at a time,
 * and reports all errors on the retry.
 * Unlike dd, ecp ignores EOF, since it is sometimes reported on error.
 * Also unlike `dd conv=noerror,sync', ecp doesn't get stuck nor give up.
 *
 * Written by Geoff Collyer, originally to run on RSX-11M(!) in 1979.
 * Later simplified for UNIX and ultimately Plan 9.
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>

/* fundamental constants */
enum {
	No = 0,
	Yes,

	Noseek = 0,		/* need not seek, may seek on seekable files */
	Mustseek,

	Enone = 0,
	Eio,
};

/* tunable parameters */
enum {
	Defsectsz = 512,	/* default sector size */
	/* 10K is a good size for HP WORM drives */
	Defblksz = 16*1024,	/* default block (big-transfer) size */
	Mingoodblks = 3,	/* after this many, go back to fast mode */
};

#define TTY "/dev/cons"			/* plan 9 */

#define badsect(errno) ((errno) != Enone)  /* was last transfer in error? */

/* disk address (in bytes or sectors), also type of 2nd arg. to seek */
typedef uvlong Daddr;
typedef vlong Sdaddr;				/* signed disk address */
typedef long Rdwrfn(int, void *, long);		/* plan 9 read or write */

typedef struct {
	char	*name;
	int	fd;
	Daddr	startsect;
	int	fast;
	int	seekable;

	ulong	maxconerrs;		/* maximum consecutive errors */
	ulong	conerrs;		/* current consecutive errors */
	Daddr	congoodblks;

	Daddr	harderrs;
	Daddr	lasterr;		/* sector #s */
	Daddr	lastgood;
} File;

/* exports */
char *argv0;

/* privates */
static int reblock = No, progress = No, swizzle = No;
static int reverse = No;
static ulong sectsz = Defsectsz;
static ulong blocksize = Defblksz;

static char *buf, *vfybuf;
static int blksects;

/*
 * warning - print best error message possible and clear errno
 */
void
warning(char *s1, char *s2)
{
	char err[100], msg[256];
	char *np, *ep = msg + sizeof msg - 1;

	errstr(err, sizeof err);		/* save error string */
	np = seprint(msg, ep, "%s: ", argv0);
	np = seprint(np, ep, s1, s2);
	errstr(err, sizeof err);		/* restore error string */
	seprint(np, ep, ": %r\n");

	fprint(2, "%s", msg);
}

int
eopen(char *file, int mode)
{
	int fd = open(file, mode);

	if (fd < 0)
		sysfatal("can't open %s: %r", file);
	return fd;
}

static int					/* boolean */
confirm(File *src, File *dest)
{
	int absent, n, tty = eopen(TTY, 2);
	char c, junk;
	Dir *stp;

	if ((stp = dirstat(src->name)) == nil)
		sysfatal("no input file %s: %r", src->name);
	free(stp);
	stp = dirstat(dest->name);
	absent = (stp == nil);
	free(stp);
	fprint(2, "%s: copy %s to %s%s? ", argv0, src->name, dest->name,
		(absent? " (missing)": ""));
	n = read(tty, &c, 1);
	junk = c;
	if (n < 1)
		c = 'n';
	while (n > 0 && junk != '\n')
		n = read(tty, &junk, 1);
	close(tty);
	if (isascii(c) && isupper(c))
		c = tolower(c);
	return c == 'y';
}

static char *
sectid(File *fp, Daddr sect)
{
	static char sectname[256];

	if (fp->startsect == 0)
		snprint(sectname, sizeof sectname, "%s sector %llud",
			fp->name, sect);
	else
		snprint(sectname, sizeof sectname,
			"%s sector %llud (relative %llud)",
			fp->name, sect + fp->startsect, sect);
	return sectname;
}

static void
io_expl(File *fp, char *rw, Daddr sect)		/* explain an i/o error */
{
	/* print only first 2 bad sectors in a range, if going forward */
	if (reverse || fp->conerrs == 0) {
		char msg[128];

		snprint(msg, sizeof msg, "%s %s", rw, sectid(fp, sect));
		warning("%s", msg);
	} else if (fp->conerrs == 1)
		fprint(2, "%s: ...\n", argv0);
}

static void
repos(File *fp, Daddr sect)
{
	if (!fp->seekable)
		sysfatal("%s: trying to seek on unseekable file", fp->name);
	if (seek(fp->fd, (sect+fp->startsect)*sectsz, 0) == -1)
		sysfatal("can't seek on %s: %r", fp->name);
}

static void
rewind(File *fp)
{
	repos(fp, 0);
}

/*
 * transfer (many) sectors.  reblock input as needed.
 * returns Enone if no failures, others on failure with errstr set.
 */
static int
bio(File *fp, Rdwrfn *rdwr, char *buff, Daddr stsect, int sects, int mustseek)
{
	int xfered;
	ulong toread, bytes = sects * sectsz;
	static int reblocked = 0;

	if (mustseek) {
		if (!fp->seekable)
			sysfatal("%s: need to seek on unseekable file",
				fp->name);
		repos(fp, stsect);
	}
	if ((long)blocksize != blocksize || (long)bytes != bytes)
		sysfatal("i/o count too big: %lud", bytes);

	werrstr("");
	xfered = (*rdwr)(fp->fd, buff, bytes);
	if (xfered == bytes)
		return Enone;			/* did as we asked */
	if (xfered < 0)
		return Eio;			/* out-and-out i/o error */
	/*
	 * Kernel transferred less than asked.  Shouldn't happen;
	 * probably indicates disk driver error or trying to
	 * transfer past the end of a disk partition.  Treat as an
	 * I/O error that reads zeros past the point of error,
	 * unless reblocking input and this is a read.
	 */
	if (rdwr == write)
		return Eio;
	if (!reblock) {
		memset(buff+xfered, '\0', bytes-xfered);
		return Eio;			/* short read */
	}

	/* for pipes that return less than asked */
	if (progress && !reblocked) {
		fprint(2, "%s: reblocking input\n", argv0);
		reblocked++;
	}
	for (toread = bytes - xfered; toread != 0; toread -= xfered) {
		xfered = (*rdwr)(fp->fd, buff+bytes-toread, toread);
		if (xfered <= 0)
			break;
	}
	if (xfered < 0)
		return Eio;			/* out-and-out i/o error */
	if (toread != 0)			/* early EOF? */
		memset(buff+bytes-toread, '\0', toread);
	return Enone;
}

/* called only after a single-sector transfer */
static int
toomanyerrs(File *fp, Daddr sect)
{
	if (sect == fp->lasterr+1)
		fp->conerrs++;
	else
		fp->conerrs = 0;
	fp->lasterr = sect;
	return fp->maxconerrs != 0 && fp->conerrs >= fp->maxconerrs &&
		fp->lastgood == -1;
}

static void
ckendrange(File *fp)
{
	if (!reverse && fp->conerrs > 0)
		fprint(2, "%s: %lld: ... last bad sector in range\n",
			argv0, fp->lasterr);
}

static int
transfer(File *fp, Rdwrfn *rdwr, char *buff, Daddr stsect, int sects,
	int mustseek)
{
	int res = bio(fp, rdwr, buff, stsect, sects, mustseek);

	if (badsect(res)) {
		fp->fast = 0;		/* read single sectors for a while */
		fp->congoodblks = 0;
	} else
		fp->lastgood = stsect + sects - 1;
	return res;
}

/*
 * Read or write many sectors at once.
 * If it fails, retry the individual sectors and report errors.
 */
static void
bigxfer(File *fp, Rdwrfn *rdwr, char *buff, Daddr stsect, int sects,
	int mustseek)
{
	int i, badsects = 0, wasfast = fp->fast;
	char *rw = (rdwr == read? "read": "write");

	if (fp->fast) {
		if (!badsect(transfer(fp, rdwr, buff, stsect, sects, mustseek)))
			return;
		if (progress)
			fprint(2, "%s: breaking up big transfer on %s error "
				"`%r' on %s\n", argv0, rw, sectid(fp, stsect));
	}

	for (i = 0; i < sects; i++)
		if (badsect(transfer(fp, rdwr, buff+i*sectsz, stsect+i, 1,
		    Mustseek))) {
			io_expl(fp, rw, stsect+i);
			badsects++;
			fp->harderrs++;
			if (toomanyerrs(fp, stsect+i))
				sysfatal("more than %lud consecutive I/O errors",
					fp->maxconerrs);
		} else {
			ckendrange(fp);
			fp->conerrs = 0;
		}
	if (badsects == 0) {
		ckendrange(fp);
		fp->conerrs = 0;
		if (wasfast)
			fprint(2, "%s: %s error on big transfer at %s but none "
				"on retries!\n", argv0, rw, sectid(fp, stsect));
		++fp->congoodblks;
		if (fp->congoodblks >= Mingoodblks) {
			fprint(2, "%s: %s: back to big transfers\n", argv0,
				fp->name);
			fp->fast = 1;
		}
	} else
		/*
		 * the last sector could have been in error, so the seek pointer
		 * may need to be corrected.
		 */
		repos(fp, stsect + sects);
}

static void
vrfyfailed(File *src, File *dest, Daddr stsect)
{
	char *srcsect = strdup(sectid(src, stsect));

	fprint(2, "%s: verify failed at %s (%s)\n", argv0, srcsect,
		sectid(dest, stsect));
	free(srcsect);
}

/*
 * I've seen SCSI read errors that the kernel printed but then didn't
 * report to the program doing the read, so if a big verify fails,
 * break it up and verify each sector separately to isolate the bad sector(s).
 */
int						/* error count */
verify(File *src, File *dest, char *buff, char *buft, Daddr stsect,
	int sectors)
{
	int i, errors = 0;

	for (i = 0; i < sectors; i++)
		if (memcmp(buff + i*sectsz, buft + i*sectsz, sectsz) != 0)
			errors++;
	if (errors == 0)
		return errors;			/* normal case */

	if (sectors == 1) {
		vrfyfailed(src, dest, stsect);
		return errors;
	}

	/* re-read and verify each sector individually */
	errors = 0;
	for (i = 0; i < sectors; i++) {
		int thissect = stsect + i;

		if (badsect(bio(src,  read, buff, thissect, 1, Mustseek)))
			io_expl(src,  "read",  thissect);
		if (badsect(bio(dest, read, buft, thissect, 1, Mustseek)))
			io_expl(dest, "write", thissect);
		if (memcmp(buff, buft, sectsz) != 0) {
			vrfyfailed(src, dest, thissect);
			++errors;
		}
	}
	if (errors == 0) {
		char *srcsect = strdup(sectid(src, stsect));

		fprint(2, "%s: verification failed on big read at %s (%s) "
			"but not on retries!\n", argv0, srcsect,
			sectid(dest, stsect));
		free(srcsect);
	}
	/*
	 * the last sector of each could have been in error, so the seek
	 * pointers may need to be corrected.
	 */
	repos(src,  stsect + sectors);
	repos(dest, stsect + sectors);
	return errors;
}

/*
 * start is starting sector of proposed transfer;
 * nsects is the total number of sectors being copied;
 * maxxfr is the block size in sectors.
 */
int
sectsleft(Daddr start, Daddr nsects, int maxxfr)
{
	/* nsects-start is sectors to the end */
	if (start + maxxfr <= nsects - 1)
		return maxxfr;
	else
		return nsects - start;
}

enum {
	Rotbits = 3,
};

void
swizzlebits(char *buff, int sects)
{
	uchar *bp, *endbp;

	endbp = (uchar *)(buff+sects*sectsz);
	for (bp = (uchar *)buff; bp < endbp; bp++)
		*bp = ~(*bp>>Rotbits | *bp<<(8-Rotbits));
}

/*
 * copy at most blksects sectors, with error retries.
 * stsect is relative to the start of the copy; 0 is the first sector.
 * to get actual sector numbers, add e.g. dest->startsect.
 */
static int
copysects(File *src, File *dest, Daddr stsect, Daddr nsects, int mustseek)
{
	int xfrsects = sectsleft(stsect, nsects, blksects);

	if (xfrsects > blksects) {
		fprint(2, "%s: block size of %d is too big.\n", argv0, xfrsects);
		exits("block size too big");
	}
	bigxfer(src,  read,  buf, stsect, xfrsects, mustseek);
	if (swizzle)
		swizzlebits(buf, xfrsects);
	bigxfer(dest, write, buf, stsect, xfrsects, mustseek);
	/* give a few reassurances at the start, then every 10MB */
	if (progress &&
	    (stsect < blksects*10 || stsect%(10*1024*1024/sectsz) == 0))
		fprint(2, "%s: copied%s to relative sector %llud\n", argv0,
			(swizzle? " swizzled": ""), stsect + xfrsects - 1);
	return 0;
}

/*
 * verify at most blksects sectors, with error retries.
 * return error count.
 */
static int
vrfysects(File *src, File *dest, Daddr stsect, Daddr nsects, int mustseek)
{
	int xfrsects = sectsleft(stsect, nsects, blksects);

	if (xfrsects > blksects) {
		fprint(2, "%s: block size of %d is too big.\n", argv0, xfrsects);
		exits("block size too big");
	}
	bigxfer(src,  read, buf,    stsect, xfrsects, mustseek);
	bigxfer(dest, read, vfybuf, stsect, xfrsects, mustseek);
	return verify(src, dest, buf, vfybuf, stsect, xfrsects);
}

static void
setupfile(File *fp, int mode)
{
	fp->fd = open(fp->name, mode);
	if (fp->fd < 0)
		sysfatal("can't open %s: %r", fp->name);
	fp->seekable = (seek(fp->fd, 0, 1) >= 0);
	if (fp->startsect != 0)
		rewind(fp);
}

static Daddr
copyfile(File *src, File *dest, Daddr nsects, int plsverify)
{
	Sdaddr stsect, vererrs = 0;
	Dir *stp;

	setupfile(src, OREAD);
	if ((stp = dirstat(dest->name)) == nil) {
		int fd = create(dest->name, ORDWR, 0666);

		if (fd >= 0)
			close(fd);
	}
	free(stp);
	setupfile(dest, ORDWR);

	if (progress)
		fprint(2, "%s: copying first sectors\n", argv0);
	if (reverse)
		for (stsect = (nsects/blksects)*blksects; stsect >= 0;
		     stsect -= blksects)
			vererrs += copysects(src, dest, stsect, nsects, Mustseek);
	else {
		for (stsect = 0; stsect < nsects; stsect += blksects)
			vererrs += copysects(src, dest, stsect, nsects, Noseek);
		ckendrange(src);
		ckendrange(dest);
	}

	/*
	 * verification is done as a separate pass rather than immediately after
	 * writing, in part to defeat caching in clever disk controllers.
	 * we really want to see the bits that hit the disk.
	 */
	if (plsverify) {
		fprint(2, "%s: copy done; verifying...\n", argv0);
		rewind(src);
		rewind(dest);
		for (stsect = 0; stsect < nsects; stsect += blksects) /* forward */
			vererrs += vrfysects(src, dest, stsect, nsects, Noseek);
		if (vererrs <= 0)
			fprint(2, "%s: no", argv0);
		else
			fprint(2, "%s: %llud", argv0, vererrs);
		fprint(2, " error%s during verification\n",
			(vererrs != 1? "s": ""));
	}
	close(src->fd);
	close(dest->fd);
	return vererrs;
}

static void
usage(void)
{
	fprint(2, "usage: %s [-bcprvZ][-B blocksz][-e errs][-s sectsz]"
		"[-i issect][-o ossect] sectors from to\n", argv0);
	exits("usage");
}

void
initfile(File *fp)
{
	memset(fp, 0, sizeof *fp);
	fp->fast = 1;
	fp->lasterr = -1;
	fp->lastgood = -1;
}

void
main(int argc, char **argv)
{
	int errflg = 0, plsconfirm = No, plsverify = No;
	long lval;
	File src, dest;
	Sdaddr sect;

	initfile(&src);
	initfile(&dest);
	ARGBEGIN {
	case 'b':
		reblock = Yes;
		break;
	case 'B':
		lval = atol(EARGF(usage()));
		if (lval < 0)
			usage();
		blocksize = lval;
		break;
	case 'c':
		plsconfirm = Yes;
		break;
	case 'e':
		lval = atol(EARGF(usage()));
		if (lval < 0)
			usage();
		src.maxconerrs = lval;
		dest.maxconerrs = lval;
		break;
	case 'i':
		sect = atoll(EARGF(usage()));
		if (sect < 0)
			usage();
		src.startsect = sect;
		break;
	case 'o':
		sect = atoll(EARGF(usage()));
		if (sect < 0)
			usage();
		dest.startsect = sect;
		break;
	case 'p':
		progress = Yes;
		break;
	case 'r':
		reverse = Yes;
		break;
	case 's':
		sectsz = atol(EARGF(usage()));
		if (sectsz <= 0 || sectsz % 512 != 0)
			usage();
		break;
	case 'v':
		plsverify = Yes;
		break;
	case 'Z':
		swizzle = Yes;
		break;
	default:
		errflg++;
		break;
	} ARGEND
	if (errflg || argc != 3)
		usage();
	if (blocksize <= 0 || blocksize % sectsz != 0)
		sysfatal("block size not a multiple of sector size");

	if (!isascii(argv[0][0]) || !isdigit(argv[0][0])) {
		fprint(2, "%s: %s is not numeric\n", argv0, argv[0]);
		exits("non-numeric sector count");
	}
	src.name =  argv[1];
	dest.name = argv[2];

	blksects = blocksize / sectsz;
	if (blksects < 1)
		blksects = 1;
	buf = malloc(blocksize);
	vfybuf = malloc(blocksize);
	if (buf == nil || vfybuf == nil)
		sysfatal("out of memory: %r");

	if (plsconfirm? confirm(&src, &dest): Yes)
		copyfile(&src, &dest, atoll(argv[0]), plsverify);
	exits(src.harderrs || dest.harderrs? "hard errors": 0);
}
