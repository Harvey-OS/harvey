/*
 * tar - `tape archiver', actually usable on any medium.
 *	POSIX "ustar" compliant when extracting, and by default when creating.
 *	this tar attempts to read and write multiple Tblock-byte blocks
 *	at once to and from the filesystem, and does not copy blocks
 *	around internally.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>		/* for %M */
#include <String.h>

/*
 * modified versions of those in libc.h; scans only the first arg for
 * keyletters and options.
 */
#define	TARGBEGIN {\
	(argv0 || (argv0 = *argv)), argv++, argc--;\
	if (argv[0]) {\
		char *_args, *_argt;\
		Rune _argc;\
		_args = &argv[0][0];\
		_argc = 0;\
		while(*_args && (_args += chartorune(&_argc, _args)))\
			switch(_argc)
#define	TARGEND	SET(_argt); USED(_argt);USED(_argc);USED(_args); \
	argc--, argv++; } \
	USED(argv); USED(argc); }
#define	TARGC() (_argc)

#define HOWMANY(a, size)	(((a) + (size) - 1) / (size))
#define BYTES2TBLKS(bytes)	HOWMANY(bytes, Tblock)

/* read big-endian binary integers; args must be (uchar *) */
#define	G2BEBYTE(x)	(((x)[0]<<8)  |  (x)[1])
#define	G3BEBYTE(x)	(((x)[0]<<16) | ((x)[1]<<8)  |  (x)[2])
#define	G4BEBYTE(x)	(((x)[0]<<24) | ((x)[1]<<16) | ((x)[2]<<8) | (x)[3])
#define	G8BEBYTE(x)	(((vlong)G4BEBYTE(x)<<32) | (u32int)G4BEBYTE((x)+4))

typedef vlong Off;
typedef char *(*Refill)(int ar, char *bufs, int justhdr);

enum { Stdin, Stdout, Stderr };
enum { Rd, Wr };			/* pipe fd-array indices */
enum { Output, Input };
enum { None, Toc, Xtract, Replace };
enum { Alldata, Justnxthdr };
enum {
	Tblock = 512,
	Namsiz = 100,
	Maxpfx = 155,		/* from POSIX */
	Maxname = Namsiz + 1 + Maxpfx,
	Binsize = 0x80,		/* flag in size[0], from gnu: positive binary size */
	Binnegsz = 0xff,	/* flag in size[0]: negative binary size */

	Nblock = 40,		/* maximum blocksize */
	Dblock = 20,		/* default blocksize */
	Debug = 0,
};

/* POSIX link flags */
enum {
	LF_PLAIN1 =	'\0',
	LF_PLAIN2 =	'0',
	LF_LINK =	'1',
	LF_SYMLINK1 =	'2',
	LF_SYMLINK2 =	's',		/* 4BSD used this */
	LF_CHR =	'3',
	LF_BLK =	'4',
	LF_DIR =	'5',
	LF_FIFO =	'6',
	LF_CONTIG =	'7',
	/* 'A' - 'Z' are reserved for custom implementations */
};

#define islink(lf)	(isreallink(lf) || issymlink(lf))
#define isreallink(lf)	((lf) == LF_LINK)
#define issymlink(lf)	((lf) == LF_SYMLINK1 || (lf) == LF_SYMLINK2)

typedef union {
	uchar	data[Tblock];
	struct {
		char	name[Namsiz];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[Namsiz];

		/* rest are defined by POSIX's ustar format; see p1003.2b */
		char	magic[6];	/* "ustar" */
		char	version[2];
		char	uname[32];
		char	gname[32];
		char	devmajor[8];
		char	devminor[8];
		char	prefix[Maxpfx]; /* if non-null, path= prefix "/" name */
	};
} Hdr;

typedef struct {
	char	*comp;
	char	*decomp;
	char	*sfx[4];
} Compress;

static Compress comps[] = {
	"gzip",		"gunzip",	{ ".tar.gz", ".tgz" },	/* default */
	"compress",	"uncompress",	{ ".tar.Z",  ".tz" },
	"bzip2",	"bunzip2",	{ ".tar.bz", ".tbz",
					  ".tar.bz2",".tbz2" },
};

typedef struct {
	int	kid;
	int	fd;	/* original fd */
	int	rfd;	/* replacement fd */
	int	input;
	int	open;
} Pushstate;

#define OTHER(rdwr) ((rdwr) == Rd? Wr: Rd)

static int debug;
static int fixednblock;
static int verb;
static int posix = 1;
static int docreate;
static int aruid;
static int argid;
static int relative = 1;
static int settime;
static int verbose;
static int docompress;
static int keepexisting;
static int ignerrs;		/* flag: ignore i/o errors if possible */
static Off blkoff;		/* offset of the current archive block (not Tblock) */
static Off nexthdr;

static int nblock = Dblock;
static int resync;
static char *usefile, *arname = "archive";
static char origdir[Maxname*2];
static Hdr *tpblk, *endblk;
static Hdr *curblk;

static void
usage(void)
{
	fprint(2, "usage: %s {crtx}[PRTfgikmpsuvz] [archive] [file1 file2...]\n",
		argv0);
	exits("usage");
}

/* I/O, with error retry or exit */

static int
cope(char *name, int fd, void *buf, long len, Off off)
{
	fprint(2, "%s: %serror reading %s: %r\n", argv0,
		(ignerrs? "ignoring ": ""), name);
	if (!ignerrs)
		exits("read error");

	/* pretend we read len bytes of zeroes */
	memset(buf, 0, len);
	if (off >= 0)			/* seekable? */
		seek(fd, off + len, 0);
	return len;
}

static int
eread(char *name, int fd, void *buf, long len)
{
	int rd;
	Off off;

	off = seek(fd, 0, 1);		/* for coping with errors */
	rd = read(fd, buf, len);
	if (rd < 0)
		rd = cope(name, fd, buf, len, off);
	return rd;
}

static int
ereadn(char *name, int fd, void *buf, long len)
{
	int rd;
	Off off;

	off = seek(fd, 0, 1);
	rd = readn(fd, buf, len);
	if (rd < 0)
		rd = cope(name, fd, buf, len, off);
	return rd;
}

static int
ewrite(char *name, int fd, void *buf, long len)
{
	int rd;

	werrstr("");
	rd = write(fd, buf, len);
	if (rd != len)
		sysfatal("error writing %s: %r", name);
	return rd;
}

/* compression */

static Compress *
compmethod(char *name)
{
	int i, nmlen = strlen(name), sfxlen;
	Compress *cp;

	for (cp = comps; cp < comps + nelem(comps); cp++)
		for (i = 0; i < nelem(cp->sfx) && cp->sfx[i]; i++) {
			sfxlen = strlen(cp->sfx[i]);
			if (nmlen > sfxlen &&
			    strcmp(cp->sfx[i], name + nmlen - sfxlen) == 0)
				return cp;
		}
	return docompress? comps: nil;
}

/*
 * push a filter, cmd, onto fd.  if input, it's an input descriptor.
 * returns a descriptor to replace fd, or -1 on error.
 */
static int
push(int fd, char *cmd, int input, Pushstate *ps)
{
	int nfd, pifds[2];
	String *s;

	ps->open = 0;
	ps->fd = fd;
	ps->input = input;
	if (fd < 0 || pipe(pifds) < 0)
		return -1;
	ps->kid = fork();
	switch (ps->kid) {
	case -1:
		return -1;
	case 0:
		if (input)
			dup(pifds[Wr], Stdout);
		else
			dup(pifds[Rd], Stdin);
		close(pifds[input? Rd: Wr]);
		dup(fd, (input? Stdin: Stdout));
		s = s_new();
		if (cmd[0] != '/')
			s_append(s, "/bin/");
		s_append(s, cmd);
		execl(s_to_c(s), cmd, nil);
		sysfatal("can't exec %s: %r", cmd);
	default:
		nfd = pifds[input? Rd: Wr];
		close(pifds[input? Wr: Rd]);
		break;
	}
	ps->rfd = nfd;
	ps->open = 1;
	return nfd;
}

static char *
pushclose(Pushstate *ps)
{
	Waitmsg *wm;

	if (ps->fd < 0 || ps->rfd < 0 || !ps->open)
		return "not open";
	close(ps->rfd);
	ps->rfd = -1;
	ps->open = 0;
	while ((wm = wait()) != nil && wm->pid != ps->kid)
		continue;
	return wm? wm->msg: nil;
}

/*
 * block-buffer management
 */

static void
initblks(void)
{
	free(tpblk);
	tpblk = malloc(Tblock * nblock);
	assert(tpblk != nil);
	endblk = tpblk + nblock;
}

/*
 * (re)fill block buffers from archive.  `justhdr' means we don't care
 * about the data before the next header block.
 */
static char *
refill(int ar, char *bufs, int justhdr)
{
	int i, n;
	unsigned bytes = Tblock * nblock;
	static int done, first = 1, seekable;

	if (done)
		return nil;

	blkoff = seek(ar, 0, 1);		/* note position for `tar r' */
	if (first)
		seekable = blkoff >= 0;
	/* try to size non-pipe input at first read */
	if (first && usefile && !fixednblock) {
		n = eread(arname, ar, bufs, bytes);
		if (n == 0)
			sysfatal("EOF reading archive %s: %r", arname);
		i = n;
		if (i % Tblock != 0)
			sysfatal("%s: archive block size (%d) error", arname, i);
		i /= Tblock;
		if (i != nblock) {
			nblock = i;
			fprint(2, "%s: blocking = %d\n", argv0, nblock);
			endblk = (Hdr *)bufs + nblock;
			bytes = n;
		}
	} else if (justhdr && seekable && nexthdr - blkoff >= bytes) {
		/* optimisation for huge archive members on seekable media */
		if (seek(ar, bytes, 1) < 0)
			sysfatal("can't seek on archive %s: %r", arname);
		n = bytes;
	} else
		n = ereadn(arname, ar, bufs, bytes);
	first = 0;

	if (n == 0)
		sysfatal("unexpected EOF reading archive %s", arname);
	if (n % Tblock != 0)
		sysfatal("partial block read from archive %s", arname);
	if (n != bytes) {
		done = 1;
		memset(bufs + n, 0, bytes - n);
	}
	return bufs;
}

static Hdr *
getblk(int ar, Refill rfp, int justhdr)
{
	if (curblk == nil || curblk >= endblk) {  /* input block exhausted? */
		if (rfp != nil && (*rfp)(ar, (char *)tpblk, justhdr) == nil)
			return nil;
		curblk = tpblk;
	}
	return curblk++;
}

static Hdr *
getblkrd(int ar, int justhdr)
{
	return getblk(ar, refill, justhdr);
}

static Hdr *
getblke(int ar)
{
	return getblk(ar, nil, Alldata);
}

static Hdr *
getblkz(int ar)
{
	Hdr *hp = getblke(ar);

	if (hp != nil)
		memset(hp->data, 0, Tblock);
	return hp;
}

/*
 * how many block buffers are available, starting at the address
 * just returned by getblk*?
 */
static int
gothowmany(int max)
{
	int n = endblk - (curblk - 1);

	return n > max? max: n;
}

/*
 * indicate that one is done with the last block obtained from getblke
 * and it is now available to be written into the archive.
 */
static void
putlastblk(int ar)
{
	unsigned bytes = Tblock * nblock;

	/* if writing end-of-archive, aid compression (good hygiene too) */
	if (curblk < endblk)
		memset(curblk, 0, (char *)endblk - (char *)curblk);
	ewrite(arname, ar, tpblk, bytes);
}

static void
putblk(int ar)
{
	if (curblk >= endblk)
		putlastblk(ar);
}

static void
putbackblk(int ar)
{
	curblk--;
	USED(ar);
}

static void
putreadblks(int ar, int blks)
{
	curblk += blks - 1;
	USED(ar);
}

static void
putblkmany(int ar, int blks)
{
	assert(blks > 0);
	curblk += blks - 1;
	putblk(ar);
}

/*
 * common routines
 */

/*
 * modifies hp->chksum but restores it; important for the last block of the
 * old archive when updating with `tar rf archive'
 */
static long
chksum(Hdr *hp)
{
	int n = Tblock;
	long i = 0;
	uchar *cp = hp->data;
	char oldsum[sizeof hp->chksum];

	memmove(oldsum, hp->chksum, sizeof oldsum);
	memset(hp->chksum, ' ', sizeof hp->chksum);
	while (n-- > 0)
		i += *cp++;
	memmove(hp->chksum, oldsum, sizeof oldsum);
	return i;
}

static int
isustar(Hdr *hp)
{
	return strcmp(hp->magic, "ustar") == 0;
}

/*
 * s is at most n bytes long, but need not be NUL-terminated.
 * if shorter than n bytes, all bytes after the first NUL must also
 * be NUL.
 */
static int
strnlen(char *s, int n)
{
	return s[n - 1] != '\0'? n: strlen(s);
}

/* set fullname from header */
static char *
name(Hdr *hp)
{
	int pfxlen, namlen;
	char *fullname;
	static char fullnamebuf[2+Maxname+1];  /* 2+ for ./ on relative names */

	fullname = fullnamebuf+2;
	namlen = strnlen(hp->name, sizeof hp->name);
	if (hp->prefix[0] == '\0' || !isustar(hp)) {	/* old-style name? */
		memmove(fullname, hp->name, namlen);
		fullname[namlen] = '\0';
		return fullname;
	}

	/* name is in two pieces */
	pfxlen = strnlen(hp->prefix, sizeof hp->prefix);
	memmove(fullname, hp->prefix, pfxlen);
	fullname[pfxlen] = '/';
	memmove(fullname + pfxlen + 1, hp->name, namlen);
	fullname[pfxlen + 1 + namlen] = '\0';
	return fullname;
}

static int
isdir(Hdr *hp)
{
	/* the mode test is ugly but sometimes necessary */
	return hp->linkflag == LF_DIR ||
		strrchr(name(hp), '\0')[-1] == '/' ||
		(strtoul(hp->mode, nil, 8)&0170000) == 040000;
}

static int
eotar(Hdr *hp)
{
	return name(hp)[0] == '\0';
}

/*
static uvlong
getbe(uchar *src, int size)
{
	uvlong vl = 0;

	while (size-- > 0) {
		vl <<= 8;
		vl |= *src++;
	}
	return vl;
}
 */

static void
putbe(uchar *dest, uvlong vl, int size)
{
	for (dest += size; size-- > 0; vl >>= 8)
		*--dest = vl;
}

/*
 * cautious parsing of octal numbers as ascii strings in
 * a tar header block.  this is particularly important for
 * trusting the checksum when trying to resync.
 */
static uvlong
hdrotoull(char *st, char *end, uvlong errval, char *name, char *field)
{
	char *numb;

	for (numb = st; (*numb == ' ' || *numb == '\0') && numb < end; numb++)
		;
	if (numb < end && isascii(*numb) && isdigit(*numb))
		return strtoull(numb, nil, 8);
	else if (numb >= end)
		fprint(2, "%s: %s: empty %s in header\n", argv0, name, field);
	else
		fprint(2, "%s: %s: %s: non-numeric %s in header\n",
			argv0, name, numb, field);
	return errval;
}

/*
 * return the nominal size from the header block, which is not always the
 * size in the archive (the archive size may be zero for some file types
 * regardless of the nominal size).
 *
 * gnu and freebsd tars are now recording vlongs as big-endian binary
 * with a flag in byte 0 to indicate this, which permits file sizes up to
 * 2^64-1 (actually 2^80-1 but our file sizes are vlongs) rather than 2^33-1.
 */
static Off
hdrsize(Hdr *hp)
{
	uchar *p;

	if((uchar)hp->size[0] == Binnegsz) {
		fprint(2, "%s: %s: negative length, which is insane\n",
			argv0, name(hp));
		return 0;
	} else if((uchar)hp->size[0] == Binsize) {
		p = (uchar *)hp->size + sizeof hp->size - 1 -
			sizeof(vlong);		/* -1 for terminating space */
		return G8BEBYTE(p);
	}

	return hdrotoull(hp->size, hp->size + sizeof hp->size, 0,
		name(hp), "size");
}

/*
 * return the number of bytes recorded in the archive.
 */
static Off
arsize(Hdr *hp)
{
	if(isdir(hp) || islink(hp->linkflag))
		return 0;
	return hdrsize(hp);
}

static long
parsecksum(char *cksum, char *name)
{
	Hdr *hp;

	return hdrotoull(cksum, cksum + sizeof hp->chksum, (uvlong)-1LL,
		name, "checksum");
}

static Hdr *
readhdr(int ar)
{
	long hdrcksum;
	Hdr *hp;

	hp = getblkrd(ar, Alldata);
	if (hp == nil)
		sysfatal("unexpected EOF instead of archive header in %s",
			arname);
	if (eotar(hp))			/* end-of-archive block? */
		return nil;

	hdrcksum = parsecksum(hp->chksum, name(hp));
	if (hdrcksum == -1 || chksum(hp) != hdrcksum) {
		if (!resync)
			sysfatal("bad archive header checksum in %s: "
				"name %.100s...; expected %#luo got %#luo",
				arname, hp->name, hdrcksum, chksum(hp));
		fprint(2, "%s: skipping past archive header with bad checksum in %s...",
			argv0, arname);
		do {
			hp = getblkrd(ar, Alldata);
			if (hp == nil)
				sysfatal("unexpected EOF looking for archive header in %s",
					arname);
			hdrcksum = parsecksum(hp->chksum, name(hp));
		} while (hdrcksum == -1 || chksum(hp) != hdrcksum);
		fprint(2, "found %s\n", name(hp));
	}
	nexthdr += Tblock*(1 + BYTES2TBLKS(arsize(hp)));
	return hp;
}

/*
 * tar r[c]
 */

/*
 * if name is longer than Namsiz bytes, try to split it at a slash and fit the
 * pieces into hp->prefix and hp->name.
 */
static int
putfullname(Hdr *hp, char *name)
{
	int namlen, pfxlen;
	char *sl, *osl;
	String *slname = nil;

	if (isdir(hp)) {
		slname = s_new();
		s_append(slname, name);
		s_append(slname, "/");		/* posix requires this */
		name = s_to_c(slname);
	}

	namlen = strlen(name);
	if (namlen <= Namsiz) {
		strncpy(hp->name, name, Namsiz);
		hp->prefix[0] = '\0';		/* ustar paranoia */
		return 0;
	}

	if (!posix || namlen > Maxname) {
		fprint(2, "%s: name too long for tar header: %s\n",
			argv0, name);
		return -1;
	}
	/*
	 * try various splits until one results in pieces that fit into the
	 * appropriate fields of the header.  look for slashes from right
	 * to left, in the hopes of putting the largest part of the name into
	 * hp->prefix, which is larger than hp->name.
	 */
	sl = strrchr(name, '/');
	while (sl != nil) {
		pfxlen = sl - name;
		if (pfxlen <= sizeof hp->prefix && namlen-1 - pfxlen <= Namsiz)
			break;
		osl = sl;
		*osl = '\0';
		sl = strrchr(name, '/');
		*osl = '/';
	}
	if (sl == nil) {
		fprint(2, "%s: name can't be split to fit tar header: %s\n",
			argv0, name);
		return -1;
	}
	*sl = '\0';
	strncpy(hp->prefix, name, sizeof hp->prefix);
	*sl++ = '/';
	strncpy(hp->name, sl, sizeof hp->name);
	if (slname)
		s_free(slname);
	return 0;
}

static int
mkhdr(Hdr *hp, Dir *dir, char *file)
{
	int r;

	/*
	 * some of these fields run together, so we format them left-to-right
	 * and don't use snprint.
	 */
	sprint(hp->mode, "%6lo ", dir->mode & 0777);
	sprint(hp->uid, "%6o ", aruid);
	sprint(hp->gid, "%6o ", argid);
	if (dir->length >= (Off)1<<32) {
		static int printed;

		if (!printed) {
			printed = 1;
			fprint(2, "%s: storing large sizes in \"base 256\"\n", argv0);
		}
		hp->size[0] = Binsize;
		/* emit so-called `base 256' representation of size */
		putbe((uchar *)hp->size+1, dir->length, sizeof hp->size - 2);
		hp->size[sizeof hp->size - 1] = ' ';
	} else
		sprint(hp->size, "%11lluo ", dir->length);
	sprint(hp->mtime, "%11luo ", dir->mtime);
	hp->linkflag = (dir->mode&DMDIR? LF_DIR: LF_PLAIN1);
	r = putfullname(hp, file);
	if (posix) {
		strncpy(hp->magic, "ustar", sizeof hp->magic);
		strncpy(hp->version, "00", sizeof hp->version);
		strncpy(hp->uname, dir->uid, sizeof hp->uname);
		strncpy(hp->gname, dir->gid, sizeof hp->gname);
	}
	sprint(hp->chksum, "%6luo", chksum(hp));
	return r;
}

static void addtoar(int ar, char *file, char *shortf);

static void
addtreetoar(int ar, char *file, char *shortf, int fd)
{
	int n;
	Dir *dent, *dirents;
	String *name = s_new();

	n = dirreadall(fd, &dirents);
	if (n < 0)
		fprint(2, "%s: dirreadall %s: %r\n", argv0, file);
	close(fd);
	if (n <= 0)
		return;

	if (chdir(shortf) < 0)
		sysfatal("chdir %s: %r", file);
	if (Debug)
		fprint(2, "chdir %s\t# %s\n", shortf, file);

	for (dent = dirents; dent < dirents + n; dent++) {
		s_reset(name);
		s_append(name, file);
		s_append(name, "/");
		s_append(name, dent->name);
		addtoar(ar, s_to_c(name), dent->name);
	}
	s_free(name);
	free(dirents);

	/*
	 * this assumes that shortf is just one component, which is true
	 * during directory descent, but not necessarily true of command-line
	 * arguments.  Our caller (or addtoar's) must reset the working
	 * directory if necessary.
	 */
	if (chdir("..") < 0)
		sysfatal("chdir %s/..: %r", file);
	if (Debug)
		fprint(2, "chdir ..\n");
}

static void
addtoar(int ar, char *file, char *shortf)
{
	int n, fd, isdir;
	long bytes, blksread;
	ulong blksleft;
	Hdr *hbp;
	Dir *dir;
	String *name = nil;

	if (shortf[0] == '#') {
		name = s_new();
		s_append(name, "./");
		s_append(name, shortf);
		shortf = s_to_c(name);
	}

	if (Debug)
		fprint(2, "opening %s	# %s\n", shortf, file);
	fd = open(shortf, OREAD);
	if (fd < 0) {
		fprint(2, "%s: can't open %s: %r\n", argv0, file);
		if (name)
			s_free(name);
		return;
	}
	dir = dirfstat(fd);
	if (dir == nil)
		sysfatal("can't fstat %s: %r", file);

	hbp = getblkz(ar);
	isdir = (dir->qid.type & QTDIR) != 0;
	if (mkhdr(hbp, dir, file) < 0) {
		putbackblk(ar);
		free(dir);
		close(fd);
		if (name)
			s_free(name);
		return;
	}
	putblk(ar);

	blksleft = BYTES2TBLKS(dir->length);
	free(dir);

	if (isdir)
		addtreetoar(ar, file, shortf, fd);
	else {
		for (; blksleft > 0; blksleft -= blksread) {
			hbp = getblke(ar);
			blksread = gothowmany(blksleft);
			assert(blksread >= 0);
			bytes = blksread * Tblock;
			n = ereadn(file, fd, hbp->data, bytes);
			assert(n >= 0);
			/*
			 * ignore EOF.  zero any partial block to aid
			 * compression and emergency recovery of data.
			 */
			if (n < Tblock)
				memset(hbp->data + n, 0, bytes - n);
			putblkmany(ar, blksread);
		}
		close(fd);
		if (verbose)
			fprint(2, "%s\n", file);
	}
	if (name)
		s_free(name);
}

static char *
replace(char **argv)
{
	int i, ar;
	ulong blksleft, blksread;
	Off bytes;
	char *arg;
	Hdr *hp;
	Compress *comp = nil;
	Pushstate ps;

	if (usefile && docreate) {
		ar = create(usefile, OWRITE, 0666);
		if (docompress)
			comp = compmethod(usefile);
	} else if (usefile)
		ar = open(usefile, ORDWR);
	else
		ar = Stdout;
	if (comp)
		ar = push(ar, comp->comp, Output, &ps);
	if (ar < 0)
		sysfatal("can't open archive %s: %r", usefile);

	if (usefile && !docreate) {
		/* skip quickly to the end */
		while ((hp = readhdr(ar)) != nil) {
			bytes = arsize(hp);
			for (blksleft = BYTES2TBLKS(bytes);
			     blksleft > 0 && getblkrd(ar, Justnxthdr) != nil;
			     blksleft -= blksread) {
				blksread = gothowmany(blksleft);
				putreadblks(ar, blksread);
			}
		}
		/*
		 * we have just read the end-of-archive Tblock.
		 * now seek back over the (big) archive block containing it,
		 * and back up curblk ptr over end-of-archive Tblock in memory.
		 */
		if (seek(ar, blkoff, 0) < 0)
			sysfatal("can't seek back over end-of-archive in %s: %r",
				arname);
		curblk--;
	}

	for (i = 0; argv[i] != nil; i++) {
		arg = argv[i];
		cleanname(arg);
		if (strcmp(arg, "..") == 0 || strncmp(arg, "../", 3) == 0)
			fprint(2, "%s: name starting with .. is a bad idea\n",
				argv0);
		addtoar(ar, arg, arg);
		chdir(origdir);		/* for correctness & profiling */
	}

	/* write end-of-archive marker */
	getblkz(ar);
	putblk(ar);
	getblkz(ar);
	putlastblk(ar);

	if (comp)
		return pushclose(&ps);
	if (ar > Stderr)
		close(ar);
	return nil;
}

/*
 * tar [xt]
 */

/* is pfx a file-name prefix of name? */
static int
prefix(char *name, char *pfx)
{
	int pfxlen = strlen(pfx);
	char clpfx[Maxname+1];

	if (pfxlen > Maxname)
		return 0;
	strcpy(clpfx, pfx);
	cleanname(clpfx);
	return strncmp(clpfx, name, pfxlen) == 0 &&
		(name[pfxlen] == '\0' || name[pfxlen] == '/');
}

static int
match(char *name, char **argv)
{
	int i;
	char clname[Maxname+1];

	if (argv[0] == nil)
		return 1;
	strcpy(clname, name);
	cleanname(clname);
	for (i = 0; argv[i] != nil; i++)
		if (prefix(clname, argv[i]))
			return 1;
	return 0;
}

static void
cantcreate(char *s, int mode)
{
	int len;
	static char *last;

	/*
	 * Always print about files.  Only print about directories
	 * we haven't printed about.  (Assumes archive is ordered
	 * nicely.)
	 */
	if(mode&DMDIR){
		if(last){
			/* already printed this directory */
			if(strcmp(s, last) == 0)
				return;
			/* printed a higher directory, so printed this one */
			len = strlen(s);
			if(memcmp(s, last, len) == 0 && last[len] == '/')
				return;
		}
		/* save */
		free(last);
		last = strdup(s);
	}
	fprint(2, "%s: can't create %s: %r\n", argv0, s);
}

static int
makedir(char *s)
{
	int f;

	if (access(s, AEXIST) == 0)
		return -1;
	f = create(s, OREAD, DMDIR | 0777);
	if (f >= 0)
		close(f);
	else
		cantcreate(s, DMDIR);
	return f;
}

static int
mkpdirs(char *s)
{
	int err;
	char *p;

	p = s;
	err = 0;
	while (!err && (p = strchr(p+1, '/')) != nil) {
		*p = '\0';
		err = (access(s, AEXIST) < 0 && makedir(s) < 0);
		*p = '/';
	}
	return -err;
}

/* Call access but preserve the error string. */
static int
xaccess(char *name, int mode)
{
	char err[ERRMAX];
	int rv;

	err[0] = 0;
	errstr(err, sizeof err);
	rv = access(name, mode);
	errstr(err, sizeof err);
	return rv;
}

static int
openfname(Hdr *hp, char *fname, int dir, int mode)
{
	int fd;

	fd = -1;
	cleanname(fname);
	switch (hp->linkflag) {
	case LF_LINK:
	case LF_SYMLINK1:
	case LF_SYMLINK2:
		fprint(2, "%s: can't make (sym)link %s\n",
			argv0, fname);
		break;
	case LF_FIFO:
		fprint(2, "%s: can't make fifo %s\n", argv0, fname);
		break;
	default:
		if (!keepexisting || access(fname, AEXIST) < 0) {
			int rw = (dir? OREAD: OWRITE);

			fd = create(fname, rw, mode);
			if (fd < 0) {
				mkpdirs(fname);
				fd = create(fname, rw, mode);
			}
			if (fd < 0 && (!dir || xaccess(fname, AEXIST) < 0))
			    	cantcreate(fname, mode);
		}
		if (fd >= 0 && verbose)
			fprint(2, "%s\n", fname);
		break;
	}
	return fd;
}

/* copy from archive to file system (or nowhere for table-of-contents) */
static void
copyfromar(int ar, int fd, char *fname, ulong blksleft, Off bytes)
{
	int wrbytes;
	ulong blksread;
	Hdr *hbp;

	if (blksleft == 0 || bytes < 0)
		bytes = 0;
	for (; blksleft > 0; blksleft -= blksread) {
		hbp = getblkrd(ar, (fd >= 0? Alldata: Justnxthdr));
		if (hbp == nil)
			sysfatal("unexpected EOF on archive extracting %s from %s",
				fname, arname);
		blksread = gothowmany(blksleft);
		if (blksread <= 0) {
			fprint(2, "%s: got %ld blocks reading %s!\n",
				argv0, blksread, fname);
			blksread = 0;
		}
		wrbytes = Tblock*blksread;
		assert(bytes >= 0);
		if(wrbytes > bytes)
			wrbytes = bytes;
		assert(wrbytes >= 0);
		if (fd >= 0)
			ewrite(fname, fd, hbp->data, wrbytes);
		putreadblks(ar, blksread);
		bytes -= wrbytes;
		assert(bytes >= 0);
	}
	if (bytes > 0)
		fprint(2, "%s: %lld bytes uncopied at EOF on archive %s; "
			"%s not fully extracted\n", argv0, bytes, arname, fname);
}

static void
wrmeta(int fd, Hdr *hp, long mtime, int mode)		/* update metadata */
{
	Dir nd;

	nulldir(&nd);
	nd.mtime = mtime;
	nd.mode = mode;
	dirfwstat(fd, &nd);
	if (isustar(hp)) {
		nulldir(&nd);
		nd.gid = hp->gname;
		dirfwstat(fd, &nd);
		nulldir(&nd);
		nd.uid = hp->uname;
		dirfwstat(fd, &nd);
	}
}

/*
 * copy a file from the archive into the filesystem.
 * fname is result of name(), so has two extra bytes at beginning.
 */
static void
extract1(int ar, Hdr *hp, char *fname)
{
	int fd = -1, dir = 0;
	long mtime = strtol(hp->mtime, nil, 8);
	ulong mode = strtoul(hp->mode, nil, 8) & 0777;
	Off bytes = hdrsize(hp);		/* for printing */
	ulong blksleft = BYTES2TBLKS(arsize(hp));

	/* fiddle name, figure out mode and blocks */
	if (isdir(hp)) {
		mode |= DMDIR|0700;
		dir = 1;
	}
	switch (hp->linkflag) {
	case LF_LINK:
	case LF_SYMLINK1:
	case LF_SYMLINK2:
	case LF_FIFO:
		blksleft = 0;
		break;
	}
	if (relative)
		if(fname[0] == '/')
			*--fname = '.';
		else if(fname[0] == '#'){
			*--fname = '/';
			*--fname = '.';
		}

	if (verb == Xtract)
		fd = openfname(hp, fname, dir, mode);
	else if (verbose) {
		char *cp = ctime(mtime);

		print("%M %8lld %-12.12s %-4.4s %s\n",
			mode, bytes, cp+4, cp+24, fname);
	} else
		print("%s\n", fname);

	copyfromar(ar, fd, fname, blksleft, bytes);

	/* touch up meta data and close */
	if (fd >= 0) {
		/*
		 * directories should be wstated *after* we're done
		 * creating files in them, but we don't do that.
		 */
		if (settime)
			wrmeta(fd, hp, mtime, mode);
		close(fd);
	}
}

static void
skip(int ar, Hdr *hp, char *fname)
{
	ulong blksleft, blksread;
	Hdr *hbp;

	for (blksleft = BYTES2TBLKS(arsize(hp)); blksleft > 0;
	     blksleft -= blksread) {
		hbp = getblkrd(ar, Justnxthdr);
		if (hbp == nil)
			sysfatal("unexpected EOF on archive extracting %s from %s",
				fname, arname);
		blksread = gothowmany(blksleft);
		putreadblks(ar, blksread);
	}
}

static char *
extract(char **argv)
{
	int ar;
	char *longname;
	Hdr *hp;
	Compress *comp = nil;
	Pushstate ps;

	if (usefile) {
		ar = open(usefile, OREAD);
		comp = compmethod(usefile);
	} else
		ar = Stdin;
	if (comp)
		ar = push(ar, comp->decomp, Input, &ps);
	if (ar < 0)
		sysfatal("can't open archive %s: %r", usefile);

	while ((hp = readhdr(ar)) != nil) {
		longname = name(hp);
		if (match(longname, argv))
			extract1(ar, hp, longname);
		else
			skip(ar, hp, longname);
	}

	if (comp)
		return pushclose(&ps);
	if (ar > Stderr)
		close(ar);
	return nil;
}

void
main(int argc, char *argv[])
{
	int errflg = 0;
	char *ret = nil;

	fmtinstall('M', dirmodefmt);

	TARGBEGIN {
	case 'c':
		docreate++;
		verb = Replace;
		break;
	case 'f':
		usefile = arname = EARGF(usage());
		break;
	case 'g':
		argid = strtoul(EARGF(usage()), 0, 0);
		break;
	case 'i':
		ignerrs = 1;
		break;
	case 'k':
		keepexisting++;
		break;
	case 'm':	/* compatibility */
		settime = 0;
		break;
	case 'p':
		posix++;
		break;
	case 'P':
		posix = 0;
		break;
	case 'r':
		verb = Replace;
		break;
	case 'R':
		relative = 0;
		break;
	case 's':
		resync++;
		break;
	case 't':
		verb = Toc;
		break;
	case 'T':
		settime++;
		break;
	case 'u':
		aruid = strtoul(EARGF(usage()), 0, 0);
		break;
	case 'v':
		verbose++;
		break;
	case 'x':
		verb = Xtract;
		break;
	case 'z':
		docompress++;
		break;
	case '-':
		break;
	default:
		fprint(2, "tar: unknown letter %C\n", TARGC());
		errflg++;
		break;
	} TARGEND

	if (argc < 0 || errflg)
		usage();

	initblks();
	switch (verb) {
	case Toc:
	case Xtract:
		ret = extract(argv);
		break;
	case Replace:
		if (getwd(origdir, sizeof origdir) == nil)
			strcpy(origdir, "/tmp");
		ret = replace(argv);
		break;
	default:
		usage();
		break;
	}
	exits(ret);
}
