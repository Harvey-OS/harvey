#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

#define TBLOCK	512
#define NBLOCK	40	/* maximum blocksize */
#define DBLOCK	20	/* default blocksize */
#define NAMSIZ	100

enum {
	Maxpfx = 155,		/* from POSIX */
	Maxname = NAMSIZ + 1 + Maxpfx,
};

/* POSIX link flags */
enum {
	LF_PLAIN1 =	'\0',
	LF_PLAIN2 =	'0',
	LF_LINK =	'1',
	LF_SYMLINK1 =	'2',
	LF_SYMLINK2 =	's',
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
union	hblock
{
	char	dummy[TBLOCK];
	struct	header
	{
		char	name[NAMSIZ];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[NAMSIZ];
		/* rest are defined by POSIX's ustar format; see p1003.2b */
		char	magic[6];	/* "ustar" */
		char	version[2];
		char	uname[32];
		char	gname[32];
		char	devmajor[8];
		char	devminor[8];
		char	prefix[155];  /* if non-null, path = prefix "/" name */
	} dbuf;
} dblock, tbuf[NBLOCK];

Dir *stbuf;
Biobuf bout;
static int ustar;		/* flag: tape block just read is ustar format */
static char *fullname;			/* if non-nil, prefix "/" name */

int	rflag, xflag, vflag, tflag, mt, cflag, fflag, Tflag, Rflag;
int	uflag, gflag;
static int posix;		/* flag: we're writing ustar format archive */
int	chksum, recno, first;
int	nblock = DBLOCK;

void	usage(void);
void	dorep(char **);
int	endtar(void);
void	getdir(void);
void	passtar(void);
void	putfile(char*, char *, char *);
void	doxtract(char **);
void	dotable(void);
void	putempty(void);
void	longt(Dir *);
int	checkdir(char *, int, Qid*);
void	tomodes(Dir *);
int	checksum(void);
int	checkupdate(char *);
int	prefix(char *, char *);
int	readtar(char *);
int	writetar(char *);
void	backtar(void);
void	flushtar(void);
void	affix(int, char *);
int	volprompt(void);

static int
isustar(struct header *hp)
{
	return strcmp(hp->magic, "ustar") == 0;
}

static void
setustar(struct header *hp)
{
	strncpy(hp->magic, "ustar", sizeof hp->magic);
	strncpy(hp->version, "00", sizeof hp->version);
}

/*
 * s is at most n bytes long, but need not be NUL-terminated.
 * if shorter than n bytes, all bytes after the first NUL must also
 * be NUL.
 */
static int
strnlen(char *s, int n)
{
	if (s[n - 1] != '\0')
		return n;
	else
		return strlen(s);
}

/* set fullname from header; called from getdir() */
static void
getfullname(struct header *hp)
{
	int pfxlen, namlen;

	if (fullname != nil)
		free(fullname);
	namlen = strnlen(hp->name, sizeof hp->name);
	if (hp->prefix[0] == '\0' || !ustar) {
		fullname = malloc(namlen + 1);
		if (fullname == nil)
			sysfatal("out of memory: %r");
		memmove(fullname, hp->name, namlen);
		fullname[namlen] = '\0';
		return;
	}
	pfxlen = strnlen(hp->prefix, sizeof hp->prefix);
	fullname = malloc(pfxlen + 1 + namlen + 1);
	if (fullname == nil)
		sysfatal("out of memory: %r");
	memmove(fullname, hp->prefix, pfxlen);
	fullname[pfxlen] = '/';
	memmove(fullname + pfxlen + 1, hp->name, namlen);
	fullname[pfxlen + 1 + namlen] = '\0';
}

/*
 * if name is longer than NAMSIZ bytes, try to split it at a slash and fit the
 * pieces into hp->prefix and hp->name.
 */
static int
putfullname(struct header *hp, char *name)
{
	int namlen, pfxlen;
	char *sl, *osl;

	namlen = strlen(name);
	if (namlen <= NAMSIZ) {
		strncpy(hp->name, name, NAMSIZ);
		hp->prefix[0] = '\0';		/* ustar paranoia */
		return 0;
	}
	if (!posix || namlen > NAMSIZ + 1 + sizeof hp->prefix) {
		fprint(2, "tar: name too long for tar header: %s\n", name);
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
		if (pfxlen <= sizeof hp->prefix && namlen-1 - pfxlen <= NAMSIZ)
			break;
		osl = sl;
		*osl = '\0';
		sl = strrchr(name, '/');
		*osl = '/';
	}
	if (sl == nil) {
		fprint(2, "tar: name can't be split to fit tar header: %s\n",
			name);
		return -1;
	}
	*sl = '\0';
	strncpy(hp->prefix, name, sizeof hp->prefix);
	*sl = '/';
	strncpy(hp->name, sl + 1, sizeof hp->name);
	return 0;
}

void
main(int argc, char **argv)
{
	char *usefile;
	char *cp, *ap;

	if (argc < 2)
		usage();

	Binit(&bout, 1, OWRITE);
	usefile =  0;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			if(!usefile)
				usage();
			fflag++;
			break;
		case 'u':
			ap = *argv++;
			if(!ap)
				usage();
			uflag = strtoul(ap, 0, 0);
			break;
		case 'g':
			ap = *argv++;
			if(!ap)
				usage();
			gflag = strtoul(ap, 0, 0);
			break;
		case 'c':
			cflag++;
			rflag++;
			break;
		case 'p':
			posix++;
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 'T':
			Tflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'R':
			Rflag++;
			break;
		case '-':
			break;
		default:
			fprint(2, "tar: %c: unknown option\n", *cp);
			usage();
		}

	fmtinstall('M', dirmodefmt);

	if (rflag) {
		if (!usefile) {
			if (cflag == 0) {
				fprint(2, "tar: can only create standard output archives\n");
				exits("arg error");
			}
			mt = dup(1, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, ORDWR)) < 0) {
			if (cflag == 0 || (mt = create(usefile, OWRITE, 0666)) < 0) {
				fprint(2, "tar: cannot open %s: %r\n", usefile);
				exits("open");
			}
		}
		dorep(argv);
	}
	else if (xflag)  {
		if (!usefile) {
			mt = dup(0, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, OREAD)) < 0) {
			fprint(2, "tar: cannot open %s: %r\n", usefile);
			exits("open");
		}
		doxtract(argv);
	}
	else if (tflag) {
		if (!usefile) {
			mt = dup(0, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, OREAD)) < 0) {
			fprint(2, "tar: cannot open %s: %r\n", usefile);
			exits("open");
		}
		dotable();
	}
	else
		usage();
	exits(0);
}

void
usage(void)
{
	fprint(2, "tar: usage  tar {txrc}[Rvf] [tarfile] file1 file2...\n");
	exits("usage");
}

void
dorep(char **argv)
{
	char cwdbuf[2048], *cwd, thisdir[2048];
	char *cp, *cp2;
	int cd;

	if (getwd(cwdbuf, sizeof(cwdbuf)) == 0) {
		fprint(2, "tar: can't find current directory: %r\n");
		exits("cwd");
	}
	cwd = cwdbuf;

	if (!cflag) {
		getdir();
		do {
			passtar();
			getdir();
		} while (!endtar());
	}

	while (*argv) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0)
				perror(*argv);
			cwd = *argv;
			argv++;
			continue;
		}
		cd = 0;
		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			chdir(*argv);
			if(**argv == '/')
				strncpy(thisdir, *argv, sizeof(thisdir));
			else
				snprint(thisdir, sizeof(thisdir), "%s/%s", cwd, *argv);
			*cp2 = '/';
			cp2++;
			cd = 1;
		} else
			strncpy(thisdir, cwd, sizeof(thisdir));
		putfile(thisdir, *argv++, cp2);
		if(cd && chdir(cwd) < 0) {
			fprint(2, "tar: can't cd back to %s: %r\n", cwd);
			exits("cwd");
		}
	}
	putempty();
	putempty();
	flushtar();
}

int
endtar(void)
{
	if (dblock.dbuf.name[0] == '\0') {
		backtar();
		return(1);
	}
	else
		return(0);
}

void
getdir(void)
{
	Dir *sp;

	readtar((char*)&dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	if(stbuf == nil){
		stbuf = malloc(sizeof(Dir));
		if(stbuf == nil)
			sysfatal("out of memory: %r");
	}
	sp = stbuf;
	sp->mode = strtol(dblock.dbuf.mode, 0, 8);
	sp->uid = "adm";
	sp->gid = "adm";
	sp->length = strtol(dblock.dbuf.size, 0, 8);
	sp->mtime = strtol(dblock.dbuf.mtime, 0, 8);
	chksum = strtol(dblock.dbuf.chksum, 0, 8);
	if (chksum != checksum())
		sysfatal("header checksum error");
	sp->qid.type = 0;
	ustar = isustar(&dblock.dbuf);
	getfullname(&dblock.dbuf);
	/* the mode test is ugly but sometimes necessary */
	if (dblock.dbuf.linkflag == LF_DIR || (sp->mode&0170000) == 040000 ||
	    strrchr(fullname, '\0')[-1] == '/') {
		sp->qid.type |= QTDIR;
		sp->mode |= DMDIR;
	}
}

void
passtar(void)
{
	long blocks;
	char buf[TBLOCK];

	switch (dblock.dbuf.linkflag) {
	case LF_LINK:
	case LF_SYMLINK1:
	case LF_SYMLINK2:
	case LF_FIFO:
		return;
	}
	blocks = stbuf->length;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		readtar(buf);
}

void
putfile(char *dir, char *longname, char *sname)
{
	int infile;
	long blocks;
	char buf[TBLOCK];
	char curdir[4096];
	char shortname[4096];
	char *cp;
	Dir *db;
	int i, n;

	if(strlen(sname) > sizeof shortname - 3){
		fprint(2, "tar: %s: name too long (max %d)\n", sname, sizeof shortname - 3);
		return;
	}
	
	snprint(shortname, sizeof shortname, "./%s", sname);
	infile = open(shortname, OREAD);
	if (infile < 0) {
		fprint(2, "tar: %s: cannot open file - %r\n", longname);
		return;
	}

	if(stbuf != nil)
		free(stbuf);
	stbuf = dirfstat(infile);

	if (stbuf->qid.type & QTDIR) {
		/* Directory */
		for (i = 0, cp = buf; *cp++ = longname[i++];);
		*--cp = '/';
		*++cp = 0;
		stbuf->length = 0;

		tomodes(stbuf);
		if (putfullname(&dblock.dbuf, buf) < 0) {
			close(infile);
			return;		/* putfullname already complained */
		}
		dblock.dbuf.linkflag = LF_DIR;
		sprint(dblock.dbuf.chksum, "%6o", checksum());
		writetar( (char *) &dblock);

		if (chdir(shortname) < 0) {
			fprint(2, "tar: can't cd to %s: %r\n", shortname);
			snprint(curdir, sizeof(curdir), "cd %s", shortname);
			exits(curdir);
		}
		sprint(curdir, "%s/%s", dir, sname);
		while ((n = dirread(infile, &db)) > 0) {
			for(i = 0; i < n; i++){
				strncpy(cp, db[i].name, sizeof buf - (cp-buf));
				putfile(curdir, buf, db[i].name);
			}
			free(db);
		}
		close(infile);
		if (chdir(dir) < 0 && chdir("..") < 0) {
			fprint(2, "tar: can't cd to ..(%s): %r\n", dir);
			snprint(curdir, sizeof(curdir), "cd ..(%s)", dir);
			exits(curdir);
		}
		return;
	}

	/* plain file; write header block first */
	tomodes(stbuf);
	if (putfullname(&dblock.dbuf, longname) < 0) {
		close(infile);
		return;		/* putfullname already complained */
	}
	blocks = (stbuf->length + (TBLOCK-1)) / TBLOCK;
	if (vflag) {
		fprint(2, "a %s ", longname);
		fprint(2, "%ld blocks\n", blocks);
	}
	dblock.dbuf.linkflag = LF_PLAIN1;
	sprint(dblock.dbuf.chksum, "%6o", checksum());
	writetar( (char *) &dblock);

	/* then copy contents */
	while ((i = readn(infile, buf, TBLOCK)) > 0 && blocks > 0) {
		writetar(buf);
		blocks--;
	}
	close(infile);
	if (blocks != 0 || i != 0)
		fprint(2, "%s: file changed size\n", longname);
	while (blocks-- >  0)
		putempty();
}


void
doxtract(char **argv)
{
	Dir null;
	int wrsize;
	long blocks, bytes;
	char buf[TBLOCK], outname[Maxname+3+1];
	char **cp;
	int ofile;

	for (;;) {
		getdir();
		if (endtar())
			break;

		if (*argv == 0)
			goto gotit;

		for (cp = argv; *cp; cp++)
			if (prefix(*cp, fullname))
				goto gotit;
		passtar();
		continue;

gotit:
		if(checkdir(fullname, stbuf->mode, &stbuf->qid))
			continue;

		if (dblock.dbuf.linkflag == LF_LINK) {
			fprint(2, "tar: can't link %s %s\n",
				dblock.dbuf.linkname, fullname);
			remove(fullname);
			continue;
		}
		if (dblock.dbuf.linkflag == LF_SYMLINK1 ||
		    dblock.dbuf.linkflag == LF_SYMLINK2) {
			fprint(2, "tar: %s: cannot symlink\n", fullname);
			continue;
		}
		if(fullname[0] != '/' || Rflag)
			sprint(outname, "./%s", fullname);
		else
			strcpy(outname, fullname);
		if ((ofile = create(outname, OWRITE, stbuf->mode & 0777)) < 0) {
			fprint(2, "tar: %s - cannot create: %r\n", outname);
			passtar();
			continue;
		}

		blocks = ((bytes = stbuf->length) + TBLOCK-1)/TBLOCK;
		if (vflag)
			fprint(2, "x %s, %ld bytes\n", fullname, bytes);
		while (blocks-- > 0) {
			readtar(buf);
			wrsize = (bytes > TBLOCK? TBLOCK: bytes);
			if (write(ofile, buf, wrsize) != wrsize) {
				fprint(2,
				    "tar: %s: HELP - extract write error: %r\n",
					fullname);
				exits("extract write");
			}
			bytes -= TBLOCK;
		}
		if(Tflag){
			nulldir(&null);
			null.mtime = stbuf->mtime;
			dirfwstat(ofile, &null);
		}
		close(ofile);
	}
}

void
dotable(void)
{
	for (;;) {
		getdir();
		if (endtar())
			break;
		if (vflag)
			longt(stbuf);
		Bprint(&bout, "%s", fullname);
		if (dblock.dbuf.linkflag == '1')
			Bprint(&bout, " linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == 's')
			Bprint(&bout, " -> %s", dblock.dbuf.linkname);
		Bprint(&bout, "\n");
		passtar();
	}
}

void
putempty(void)
{
	char buf[TBLOCK];

	memset(buf, 0, TBLOCK);
	writetar(buf);
}

void
longt(Dir *st)
{
	char *cp;

	Bprint(&bout, "%M %4d/%1d ", st->mode, 0, 0);	/* 0/0 uid/gid */
	Bprint(&bout, "%8lld", st->length);
	cp = ctime(st->mtime);
	Bprint(&bout, " %-12.12s %-4.4s ", cp+4, cp+24);
}

int
checkdir(char *name, int mode, Qid *qid)
{
	char *cp;
	int f;
	Dir *d, null;

	if(Rflag && *name == '/')
		name++;
	cp = name;
	if(*cp == '/')
		cp++;
	for (; *cp; cp++) {
		if (*cp == '/') {
			*cp = '\0';
			if (access(name, 0) < 0) {
				f = create(name, OREAD, DMDIR + 0775L);
				if(f < 0)
					fprint(2, "tar: mkdir %s failed: %r\n", name);
				close(f);
			}
			*cp = '/';
		}
	}

	/* if this is a directory, chmod it to the mode in the tar plus 700 */
	if(cp[-1] == '/' || (qid->type&QTDIR)){
		if((d=dirstat(name)) != 0){
			nulldir(&null);
			null.mode = DMDIR | (mode & 0777) | 0700;
			dirwstat(name, &null);
			free(d);
		}
		return 1;
	} else
		return 0;
}

void
tomodes(Dir *sp)
{
	memset(dblock.dummy, 0, sizeof(dblock.dummy));
	sprint(dblock.dbuf.mode, "%6lo ", sp->mode & 0777);
	sprint(dblock.dbuf.uid, "%6o ", uflag);
	sprint(dblock.dbuf.gid, "%6o ", gflag);
	sprint(dblock.dbuf.size, "%11llo ", sp->length);
	sprint(dblock.dbuf.mtime, "%11lo ", sp->mtime);
	if (posix) {
		setustar(&dblock.dbuf);
		strncpy(dblock.dbuf.uname, sp->uid, sizeof dblock.dbuf.uname);
		strncpy(dblock.dbuf.gname, sp->gid, sizeof dblock.dbuf.gname);
	}
}

int
checksum(void)
{
	int i;
	char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp & 0xff;
	return(i);
}

int
prefix(char *s1, char *s2)
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

int
readtar(char *buffer)
{
	int i;

	if (recno >= nblock || first == 0) {
		if ((i = readn(mt, tbuf, TBLOCK*nblock)) <= 0) {
			if (i == 0)
				werrstr("unexpected end of file");
			fprint(2, "tar: archive read error: %r\n");
			exits("archive read");
		}
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprint(2, "tar: archive blocksize error: %r\n");
				exits("blocksize");
			}
			i /= TBLOCK;
			if (i != nblock) {
				fprint(2, "tar: blocksize = %d\n", i);
				nblock = i;
			}
		}
		recno = 0;
	}
	first = 1;
	memmove(buffer, &tbuf[recno++], TBLOCK);
	return(TBLOCK);
}

int
writetar(char *buffer)
{
	first = 1;
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) != TBLOCK*nblock) {
			fprint(2, "tar: archive write error: %r\n");
			exits("write");
		}
		recno = 0;
	}
	memmove(&tbuf[recno++], buffer, TBLOCK);
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) != TBLOCK*nblock) {
			fprint(2, "tar: archive write error: %r\n");
			exits("write");
		}
		recno = 0;
	}
	return(TBLOCK);
}

/*
 * backup over last tar block
 */
void
backtar(void)
{
	seek(mt, -TBLOCK*nblock, 1);
	recno--;
}

void
flushtar(void)
{
	write(mt, tbuf, TBLOCK*nblock);
}
