/*
 * ar - portable (ascii) format version
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ar.h>

/*
 *	The algorithm uses up to 3 temp files.  The "pivot member" is the
 *	archive member specified by and a, b, or i option.  Temp file astart
 *	contains existing members up to and including the pivot member.  Temp
 *	file amiddle contains new files moved or inserted behind the pivot.
 *	Temp file aend, contains the existing members of the archive that follow
 *	the pivot member.  When all members have been processed, function
 *	'install' streams the temp files, in order back into the archive.
 */

typedef struct	Armember	/* Temp file entry - one per archive member */
{
	struct Armember	*next;
	struct ar_hdr	hdr;
	long		size;
	long		date;
	void		*member;
} Armember;

typedef	struct Arfile		/* Temp file control block - one per tempfile */
{
	int	paged;		/*set when some data paged to disk */
	char	*fname;		/* paging file name */
	int	fd;		/* paging file descriptor */
	Armember *head;		/* head of member chain */
	Armember *tail;		/* tail of member chain */
} Arfile;
		/* constants and flags */
char	*man =		"mrxtdpq";
char	*opt =		"uvnbailo";
char	artemp[] =	"/tmp/vXXXXX";
char	movtemp[] =	"/tmp/v1XXXXX";
char	tailtemp[] =	"/tmp/v2XXXXX";

int	aflag;				/* command line flags */
int	bflag;
int	cflag;
int	dflag;
int	iflag;
int	lflag;
int	mflag;
int	nflag;
int	oflag;
int	pflag;
int	qflag;
int	rflag;
int	tflag;
int	uflag;
int	vflag;
int	xflag;

Arfile *astart, *amiddle, *aend;	/* Temp file control block pointers */
	
#define	ARNAMESIZE	sizeof(astart->tail->hdr.name)

char	poname[ARNAMESIZE+1];		/* name of pivot member */
char	*file;				/* current file or member being worked on */
Biobuf	bout;

void	arcopy(int, Arfile*, Armember*);
int	arcreate(char*);
void	arfree(Arfile*);
void	arinsert(Arfile*, Armember*);
char	*armalloc(int);
void	armove(int, Arfile*, Armember*);
void	arread(int, Armember*, int);
void	arstream(int, Arfile*);
int	arwrite(int, Armember*);
int	bamatch(char*, char*);
Armember *getdir(int);
int	getspace(void);
void	install(char*, Arfile*, Arfile*, Arfile*);
void	longt(Armember*);
int	match(int, char**);
void	mesg(int, char*);
Arfile	*newtempfile(char*);
Armember *newmember(void);
int	openar(char*, int, int);
int	page(Arfile*);
void	pmode(long);
void	select(int*, long);
void	setcom(void(*)(char*, int, char**));
void	skip(int, long);
void	trim(char*, char*, int);
void	usage(void);
void	wrerr(void);

void	rcmd(char*, int, char**);		/* command processing */
void	dcmd(char*, int, char**);
void	xcmd(char*, int, char**);
void	tcmd(char*, int, char**);
void	pcmd(char*, int, char**);
void	mcmd(char*, int, char**);
void	qcmd(char*, int, char**);
void	(*comfun)(char*, int, char**);

void
main(int argc, char *argv[])
{
	char *cp;

	Binit(&bout, 1, OWRITE);
	if(argc < 3)
		usage();
	for (cp = argv[1]; *cp; cp++) {
		switch(*cp) {
		case 'a':	aflag = 1;	break;
		case 'b':	bflag = 1;	break;
		case 'c':	cflag = 1;	break;
		case 'd':	setcom(dcmd);	break;
		case 'i':	bflag = 1;	break;
		case 'l':
				strcpy(artemp, "vXXXXX");
				strcpy(movtemp, "v1XXXXX");
				strcpy(tailtemp, "v2XXXXX");
				break;
		case 'm':	setcom(mcmd);	break;
		case 'n':	nflag = 1;	break;
		case 'o':	oflag = 1;	break;
		case 'p':	setcom(pcmd);	break;
		case 'q':	setcom(qcmd);	break;
		case 'r':	setcom(rcmd);	break;
		case 't':	setcom(tcmd);	break;
		case 'u':	uflag = 1;	break;
		case 'v':	vflag = 1;	break;
		case 'x':	setcom(xcmd);	break;
		default:
			fprint(2, "ar: bad option `%c'\n", *cp);
			exits("error");
		}
	}
	if (aflag && bflag) {
		fprint(2, "ar: only one of 'a' and 'b' can be specified\n");
		usage();
	}
	if(aflag || bflag) {
		trim(argv[2], poname, sizeof(poname));
		argv++;
		argc--;
		if(argc < 3)
			usage();
	}
	if(comfun == 0) {
		if(uflag == 0) {
			fprint(2, "ar: one of [%s] must be specified\n", man);
			usage();
		}
		setcom(rcmd);
	}
	cp = argv[2];
	argc -= 3;
	argv += 3;
	(*comfun)(cp, argc, argv);	/* do the command */
	cp = 0;
	while (argc--) {
		if (*argv) {
			fprint(2, "ar: %s not found\n", *argv);
			cp = "error";
		}
		argv++;
	}
	exits(cp);
}
/*
 *	select a command
 */
void
setcom(void (*fun)(char *, int, char**))
{

	if(comfun != 0) {
		fprint(2, "ar: only one of [%s] allowed\n", man);
		usage();
	}
	comfun = fun;
}
/*
 *	perform the 'r' and 'u' commands
 */
void
rcmd(char *arname, int count, char **files)
{
	int fd, fd2;
	int i;
	Arfile *ap;
	Armember *bp;
	Dir d;

	fd = openar(arname, ORDWR, 1);
	astart = newtempfile(artemp);
	ap = astart;
	aend = 0;
	while (fd > 0) {
		bp = getdir(fd);
		if (!bp)
			break;
		if (bamatch(file, poname)) {		/* check for pivot */
			aend = newtempfile(tailtemp);
			ap = aend;
		}
		if (count == 0 || match(count, files)) {
			fd2 = open(file, OREAD);
			if (fd2 < 0) {
				if (count != 0)
					fprint(2, "ar: cannot open %s\n", file);
				arcopy(fd, ap, bp);
			} else if (uflag) {
				if (dirfstat(fd2, &d) < 0) {
					fprint(2, "ar: cannot stat %s\n", file);
					arcopy(fd, ap, bp);
				} else if (d.mtime <= bp->date)
					arcopy(fd, ap, bp);
				else {
					mesg('r', file);
					skip(fd, bp->size);
					armove(fd2, ap, bp);
				}
				close(fd2);
			} else {
				mesg('r', file);
				skip(fd, bp->size);
				armove(fd2, ap, bp);
				close(fd2);
			}
		} else
			arcopy(fd, ap, bp);
	}
	if(fd < 0) {
		if(!cflag)
			fprint(2, "ar: creating %s\n", arname);
	} else
		close(fd);
		/* copy in remaining files named on command line */
	for (i = 0; i < count; i++) {
		file = files[i];
		if(file == 0)
			continue;
		files[i] = 0;
		fd2 = open(file, OREAD);
		if (fd2 < 0)
			fprint(2, "ar: %s cannot open\n", file);
		else {
			mesg('a', file);
			armove(fd2, astart, newmember());
			close(fd2);
		}
	}
	install(arname, astart, 0, aend);
}
void
dcmd(char *arname, int count, char **files)
{
	Armember *bp;
	int fd;


	if (!count)
		return;
	fd = openar(arname, ORDWR, 0);
	astart = newtempfile(artemp);
	while (bp = getdir(fd)) {
		if(match(count, files)) {
			mesg('d', file);
			skip(fd, bp->size);
		} else
			arcopy(fd, astart, bp);
	}
	install(arname, astart, 0, 0);
}

void
xcmd(char *arname, int count, char **files)
{
	int fd, f, mode, i;
	Armember *bp;
	Dir d;

	fd = openar(arname, OREAD, 0);
	i = 0;
	while (bp = getdir(fd)) {
		if(count == 0 || match(count, files)) {
			mode = strtoul(bp->hdr.mode, 0, 8) & 0777;
			f = create(file, OWRITE, mode);
			if(f < 0) {
				fprint(2, "ar: %s cannot create\n", file);
				skip(fd, bp->size);
			} else {
				mesg('x', file);
				arcopy(fd, 0, bp);
				if (write(f, bp->member, bp->size) < 0)
					wrerr();
				if(oflag) {
					if(dirfstat(f, &d) < 0)
						perror(file);
					else {
						d.atime = bp->date;
						d.mtime = bp->date;
						if(dirwstat(file, &d) < 0)
							perror(file);
					}
				}
				free(bp->member);
				close(f);
			}
			free(bp);
			if (count && ++i >= count)
				break;
		} else {
			skip(fd, bp->size);
			free(bp);
		}
	}
	close(fd);
}
void
pcmd(char *arname, int count, char **files)
{
	int fd;
	Armember *bp;

	fd = openar(arname, OREAD, 0);
	while(bp = getdir(fd)) {
		if(count == 0 || match(count, files)) {
			if(vflag)
				print("\n<%s>\n\n", file);
			arcopy(fd, 0, bp);
			if (write(1, bp->member, bp->size) < 0)
				wrerr();
		} else
			skip(fd, bp->size);
		free(bp);
	}
	close(fd);
}
void
mcmd(char *arname, int count, char **files)
{
	int fd;
	Arfile *ap;
	Armember *bp;

	if (count == 0)
		return;
	fd = openar(arname, ORDWR, 0);
	astart = newtempfile(artemp);
	amiddle = newtempfile(movtemp);
	aend = 0;
	ap = astart;
	while(bp = getdir(fd)) {
		if (bamatch(file, poname)) {
			aend = newtempfile(tailtemp);
			ap = aend;
		}
		if(match(count, files)) {
			mesg('m', file);
			arcopy(fd, amiddle, bp);
		} else
			arcopy(fd, ap, bp);
	}
	close(fd);
	if (poname[0] && aend == 0)
		fprint(2, "ar: %s not found - files moved to end.\n", poname);
	install(arname, astart, amiddle, aend);
}
void
tcmd(char *arname, int count, char **files)
{
	int fd;
	Armember *bp;
	char name[ARNAMESIZE+1];

	fd = openar(arname, OREAD, 0);
	while(bp = getdir(fd)) {
		if(count == 0 || match(count, files)) {
			if(vflag)
				longt(bp);
			trim(file, name, sizeof(name));
			Bprint(&bout, "%s\n", name);
		}
		skip(fd, bp->size);
		free(bp);
	}
	close(fd);
}
void
qcmd(char *arname, int count, char **files)
{
	int fd, fd2, i;
	Armember *bp;

	if(aflag || bflag) {
		fprint(2, "ar: abi not allowed with q\n");
		exits("error");
	}
	fd = openar(arname, ORDWR, 1);
	if (fd < 0) {
		if(!cflag)
			fprint(2, "ar: creating %s\n", arname);
		fd = arcreate(arname);
	}
	/* leave note group behind when writing archive; i.e. sidestep interrupts */
	rfork(RFNOTEG);
	seek(fd, 0, 2);
	bp = newmember();
	for(i=0; i<count && files[i]; i++) {
		file = files[i];
		files[i] = 0;
		fd2 = open(file, OREAD);
		if(fd2 < 0)
			fprint(2, "ar: %s cannot open\n", file);
		else {
			mesg('q', file);
			armove(fd2, 0, bp);
			if (!arwrite(fd, bp))
				wrerr();
			free(bp->member);
			bp->member = 0;
			close(fd2);
		}
	}
	free(bp);
	close(fd);
}
/*
 *	open an archive and validate its header
 */
int
openar(char *arname, int mode, int errok)
{
	int fd;
	char mbuf[SARMAG];

	fd = open(arname, mode);
	if(fd >= 0){
		if(read(fd, mbuf, SARMAG) != SARMAG || strncmp(mbuf, ARMAG, SARMAG)) {
			fprint(2, "ar: %s not in archive format\n", arname);
			exits("error");
		}
	}else if(!errok){
		fprint(2, "ar: cannot open %s: %r\n", arname);
		exits("error");
	}
	return fd;
}
/*
 *	create an archive and set its header
 */
int
arcreate(char *arname)
{
	int fd;

	fd = create(arname, OWRITE, 0664);
	if(fd < 0){
		fprint(2, "ar: cannot create %s: %r\n", arname);
		exits("error");
	}
	if(write(fd, ARMAG, SARMAG) != SARMAG)
		wrerr();
	return fd;
}
/*
 *		error handling
 */
void
wrerr(void)
{
	perror("ar: write error");
	exits("error");
}

void
rderr(void)
{
	perror("ar: read error");
	exits("error");
}

void
phaseerr(int offset)
{
	fprint(2, "ar: phase error at offset %d\n", offset);
	exits("error");
}

void
usage(void)
{
	fprint(2, "usage: ar [%s][%s] archive files ...\n", opt, man);
	exits("error");
}
/*
 *	read the header for the next archive member
 */
Armember *
getdir(int fd)
{
	Armember *bp;
	char *cp;
	int i;
	static char name[ARNAMESIZE+1];

	bp = newmember();
	i = read(fd, &bp->hdr, SAR_HDR);
	if(i != SAR_HDR) {
		free(bp);
		return 0;
	}
	if(strncmp(bp->hdr.fmag, ARFMAG, sizeof(bp->hdr.fmag)))
		phaseerr(seek(fd, 0, 1));
	strncpy(name, bp->hdr.name, sizeof(bp->hdr.name));
	cp = name+sizeof(name)-1;
	while(*--cp==' ')
		;
	cp[1] = '\0';
	file = name;
	bp->date = atol(bp->hdr.date);
	bp->size = atol(bp->hdr.size);
	return bp;
}
/*
 *	Copy the file referenced by fd to the temp file
 */
void
armove(int fd, Arfile *ap, Armember *bp)
{
	char *cp;
	Dir d;

	if (dirfstat(fd, &d) < 0) {
		fprint(2, "ar: cannot stat %s\n", file);
		return;
	}
	trim(file, bp->hdr.name, sizeof(bp->hdr.name));
	for (cp = strchr(bp->hdr.name, 0);		/* blank pad on right */
		cp < bp->hdr.name+sizeof(bp->hdr.name); cp++)
			*cp = ' ';
	sprint(bp->hdr.date, "%-12ld", d.mtime);
	sprint(bp->hdr.uid, "%-6d", 0);
	sprint(bp->hdr.gid, "%-6d", 0);
	sprint(bp->hdr.mode, "%-8lo", d.mode);
	sprint(bp->hdr.size, "%-10ld", d.length);
	strncpy(bp->hdr.fmag, ARFMAG, 2);
	bp->size = d.length;
	bp->date = d.mtime;
	arread(fd, bp, bp->size);
	if (ap)
		arinsert(ap, bp);
}
/*
 *	Copy the archive member at the current offset into the temp file.
 */
void
arcopy(int fd, Arfile *ap, Armember *bp)
{
	int n;

	n = bp->size;
	if (n & 01)
		n++;
	arread(fd, bp, n);
	if (ap)
		arinsert(ap, bp);
}
/*
 *	Skip an archive member
 */
void
skip(int fd, long len)
{
	if (len & 01)
		len++;
	seek(fd, len, 1);
}
/*
 *	Stream the three temp files to an archive
 */
void
install(char *arname, Arfile *astart, Arfile *amiddle, Arfile *aend)
{
	int fd;

	/* leave note group behind when copying back; i.e. sidestep interrupts */
	rfork(RFNOTEG);
	fd = arcreate(arname);
	if (astart) {
		arstream(fd, astart);
		arfree(astart);
	}
	if (amiddle) {
		arstream(fd, amiddle);
		arfree(amiddle);
	}
	if (aend) {
		arstream(fd, aend);
		arfree(aend);
	}
	close(fd);
}
/*
 *	Check if the archive member matches an entry on the command line.
 */
int
match(int count, char **files)
{
	int i;
	char name[ARNAMESIZE+1];

	for(i=0; i<count; i++) {
		if(files[i] == 0)
			continue;
		trim(files[i], name, sizeof(name));
		if(strncmp(name, file, sizeof(name)) == 0) {
			file = files[i];
			files[i] = 0;
			return 1;
		}
	}
	return 0;
}
/*
 *	compare the current member to the name of the pivot member
 */
int
bamatch(char *file, char *pivot)
{
	static int state = 0;

	switch(state)
	{
	case 0:			/* looking for position file */
		if (aflag) {
			if (!strncmp(file, pivot, ARNAMESIZE))
				state = 1;
		} else if (bflag) {
			if (!strncmp(file, pivot, ARNAMESIZE)) {
				state = 2;	/* found */
				return 1;
			}
		}
		break;
	case 1:			/* found - after previous file */
		state = 2;
		return 1;
	case 2:			/* already found position file */
		break;
	}
	return 0;
}
/*
 *	output a message, if 'v' option was specified
 */
void
mesg(int c, char *file)
{

	if(vflag)
		Bprint(&bout, "%c - %s\n", c, file);
}
/*
 *	isolate file name by stripping leading directories and trailing slashes
 */
void
trim(char *s, char *buf, int n)
{
	char *p;

	for(;;) {
		p = strrchr(s, '/');
		if (!p) {		/* no slash in name */
			strncpy(buf, s, n);
			return;
		}
		if (p[1] != 0) {	/* p+1 is first char of file name */
			strncpy(buf, p+1, n);
			return;
		}
		*p = 0;			/* strip trailing slash */
	}
}
/*
 *	utilities for printing long form of 't' command
 */
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

void
longt(Armember *bp)
{
	char *cp;

	pmode(strtoul(bp->hdr.mode, 0, 8));
	Bprint(&bout, "%3d/%1d", atol(bp->hdr.uid), atol(bp->hdr.gid));
	Bprint(&bout, "%7ld", bp->size);
	cp = ctime(bp->date);
	Bprint(&bout, " %-12.12s %-4.4s ", cp+4, cp+24);
}

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

void
pmode(long mode)
{
	int **mp;

	for(mp = &m[0]; mp < &m[9];)
		select(*mp++, mode);
}

void
select(int *ap, long mode)
{
	int n;

	n = *ap++;
	while(--n>=0 && (mode&*ap++)==0)
		ap++;
	Bputc(&bout, *ap);
}
/*
 *	Temp file I/O subsystem.  We attempt to cache all three temp files in
 *	core.  When we run out of memory we spill to disk.
 *	The I/O model assumes that temp files:
 *		1) are only written on the end
 *		2) are only read from the beginning
 *		3) are only read after all writing is complete.
 *	The architecture uses one control block per temp file.  Each control
 *	block anchors a chain of buffers, each containing an archive member.
 */
Arfile *
newtempfile(char *name)		/* allocate a file control block */
{
	Arfile *ap;

	ap = (Arfile *) armalloc(sizeof(Arfile));
	ap->fname = name;
	return ap;
}

Armember *
newmember(void)			/* allocate a member buffer */
{
	return (Armember *)armalloc(sizeof(Armember));
}

void
arread(int fd, Armember *bp, int n)	/* read an image into a member buffer */
{
	int i;

	bp->member = armalloc(n);
	i = read(fd, bp->member, n);
	if (i < 0) {
		free(bp->member);
		bp->member = 0;
		rderr();
	}
}
/*
 * insert a member buffer into the member chain
 */
void
arinsert(Arfile *ap, Armember *bp)
{
	bp->next = 0;
	if (!ap->tail)
		ap->head = bp;
	else
		ap->tail->next = bp;
	ap->tail = bp;
}
/*
 *	stream the members in a temp file to the file referenced by 'fd'.
 */
void
arstream(int fd, Arfile *ap)
{
	Armember *bp;
	int i;
	char buf[8192];

	if (ap->paged) {		/* copy from disk */
		seek(ap->fd, 0, 0);
		for (;;) {
			i = read(ap->fd, buf, sizeof(buf));
			if (i < 0)
				rderr();
			if (i == 0)
				break;
			if (write(fd, buf, i) != i)
				wrerr();
		}
		close(ap->fd);
		ap->paged = 0;
	}
		/* dump the in-core buffers */
	for (bp = ap->head; bp; bp = bp->next) {
		if (!arwrite(fd, bp))
			wrerr();
	}
}
/*
 *	write a member to 'fd'.
 */	
int
arwrite(int fd, Armember *bp)
{
	int len;

	if (write(fd, &bp->hdr, SAR_HDR) != SAR_HDR)
		return 0;
	len = bp->size;
	if (len & 01)
		len++;
	if (write(fd, bp->member, len) != len)
		return 0;
	return 1;
}
/*
 *	Spill a member to a disk copy of a temp file
 */
int
page(Arfile *ap)
{
	Armember *bp;

	bp = ap->head;
	if (!ap->paged) {		/* not yet paged - create file */
		ap->fname = mktemp(ap->fname);
		ap->fd = create(ap->fname, ORDWR|ORCLOSE, 0600);
		if (ap->fd < 0) {
			fprint(2,"ar: can't create temp file\n");
			return 0;
		}
		ap->paged = 1;
	}
	if (!arwrite(ap->fd, bp))	/* write member and free buffer block */
		return 0;
	ap->head = bp->next;
	if (ap->tail == bp)
		ap->tail = bp->next;
	free(bp->member);
	free(bp);
	return 1;
}
/*
 *	try to reclaim space by paging.  we try to spill the start, middle,
 *	and end files, in that order.  there is no particular reason for the
 *	ordering.
 */
int
getspace(void)
{
	if (astart && astart->head && page(astart))
			return 1;
	if (amiddle && amiddle->head && page(amiddle))
			return 1;
	if (aend && aend->head && page(aend))
			return 1;
	return 0;
}

void
arfree(Arfile *ap)		/* free a member buffer */
{
	Armember *bp, *next;

	for (bp = ap->head; bp; bp = next) {
		next = bp->next;
		if (bp->member)
			free(bp->member);
		free(bp);
	}
	free(ap);
}
/*
 *	allocate space for a control block or member buffer.  if the malloc
 *	fails we try to reclaim space by spilling previously allocated
 *	member buffers.
 */
char *
armalloc(int n)
{
	char *cp;

	do {
		cp = malloc(n);
		if (cp) {
			memset(cp, 0, n);
			return cp;
		}
	} while (getspace());
	fprint(2, "ar: out of memory\n");
	exits("malloc");
	return 0;
}
