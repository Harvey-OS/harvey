/* seriously hacked to get it going. Want to add it back in someday. */
/*
 * ar - portable (ascii) format version
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ar.h>



typedef struct  Sym     Sym;
struct  Sym
{
        vlong   value;
        uint    sig;
        char    type;
        char    *name;
};

#define islocal(t)	((t)=='a' || (t)=='p')
typedef enum Kind		/* variable defs and references in obj */
{
	aNone,			/* we don't care about this prog */
	aName,			/* introduces a name */
	aText,			/* starts a function */
	aData,			/* references to a global object */
} Kind;

typedef struct	Prog	Prog;

struct Prog		/* info from .$O files */
{
	Kind	kind;		/* what kind of symbol */
	char	type;		/* type of the symbol: ie, 'T', 'a', etc. */
	char	sym;		/* index of symbol's name */
	char	*id;		/* name for the symbol, if it introduces one */
	uint	sig;		/* type signature for symbol */
};

#define UNKNOWN	'?'
//void		_offset(int, vlong);

enum
{
				/* object file types */
	Obj68020 = 0,		/* .2 */
	ObjSparc,		/* .k */
	ObjMips,		/* .v */
	Obj386,			/* .8 */
	Obj960,			/* retired */
	Obj3210,		/* retired */
	ObjMips2,		/* .4 */
	Obj29000,		/* retired */
	ObjArm,			/* .5 */
	ObjPower,		/* .q */
	ObjMips2le,		/* .0 */
	ObjAlpha,		/* .7 */
	ObjSparc64,		/* .u */
	ObjAmd64,		/* .6 */
	ObjSpim,		/* .0 */
	ObjPower64,		/* .9 */
	Maxobjtype,

	NNAMES	= 50,
	MAXIS	= 8,		/* max length to determine if a file is a .? file */
	MAXOFF	= 0x7fffffff,	/* larger than any possible local offset */
	NHASH	= 1024,		/* must be power of two */
	HASHMUL	= 79L,
};

int _is6(char *t);
int	_read6(Biobuf* b, Prog*p);

typedef struct Obj	Obj;
typedef struct Symtab	Symtab;

struct	Obj		/* functions to handle each intermediate (.$O) file */
{
	char	*name;				/* name of each $O file */
	int	(*is)(char*);			/* test for each type of $O file */
	int	(*read)(Biobuf*, Prog*);	/* read for each type of $O file*/
};

static Obj	obj[] =
{			/* functions to identify and parse each type of obj */
	[ObjAmd64]	"amd64 .6",	_is6, _read6,
	[Maxobjtype]	0, 0
};

struct	Symtab
{
	struct	Sym 	s;
	struct	Symtab	*next;
};

static	Symtab *objhash[NHASH];
static	Sym	*names[NNAMES];	/* working set of active names */

static	int	processprog(Prog*,int);	/* decode each symbol reference */
static	void	objreset(void);
static	void	objlookup(int, char *, int, uint);
static	void 	objupdate(int, int);

/*
 * little-endian long
 */
long
leswal(long l)
{
        uchar *p;

        p = (uchar*)&l;
        return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}
int
objtype(Biobuf *bp, char **name)
{
  if (name)
    *name = obj[ObjAmd64].name;
  return ObjAmd64;

}

int
isar(Biobuf *bp)
{
	int n;
	char magbuf[SARMAG];

	n = Bread(bp, magbuf, SARMAG);
	if(n == SARMAG && strncmp(magbuf, ARMAG, SARMAG) == 0)
		return 1;
	return 0;
}

/*
 * determine what kind of object file this is and process it.
 * return whether or not this was a recognized intermediate file.
 */
int
readobj(Biobuf *bp, int objtype)
{
	Prog p;

	if (objtype < 0 || objtype >= Maxobjtype || obj[objtype].is == 0)
		return 1;
	objreset();
	while ((*obj[objtype].read)(bp, &p))
		if (!processprog(&p, 1))
			return 0;
	return 1;
}

int
readar(Biobuf *bp, int objtype, vlong end, int doautos)
{
	Prog p;

	if (objtype < 0 || objtype >= Maxobjtype || obj[objtype].is == 0)
		return 1;
	objreset();
	while ((*obj[objtype].read)(bp, &p) && Boffset(bp) < end)
		if (!processprog(&p, doautos))
			return 0;
	return 1;
}

/*
 *	decode a symbol reference or definition
 */
static	int
processprog(Prog *p, int doautos)
{
	if(p->kind == aNone)
		return 1;
	if(p->sym < 0 || p->sym >= NNAMES)
		return 0;
	switch(p->kind)
	{
	case aName:
		if (!doautos)
		if(p->type != 'U' && p->type != 'b')
			break;
		objlookup(p->sym, p->id, p->type, p->sig);
		break;
	case aText:
		objupdate(p->sym, 'T');
		break;
	case aData:
		objupdate(p->sym, 'D');
		break;
	default:
		break;
	}
	return 1;
}

/*
 * find the entry for s in the symbol array.
 * make a new entry if it is not already there.
 */
static void
objlookup(int id, char *name, int type, uint sig)
{
	long h;
	char *cp;
	Sym *s;
	Symtab *sp;

	s = names[id];
	if(s && strcmp(s->name, name) == 0) {
		s->type = type;
		s->sig = sig;
		return;
	}

	h = *name;
	for(cp = name+1; *cp; h += *cp++)
		h *= HASHMUL;
	if(h < 0)
		h = ~h;
	h &= (NHASH-1);
	if (type == 'U' || type == 'b' || islocal(type)) {
		for(sp = objhash[h]; sp; sp = sp->next)
			if(strcmp(sp->s.name, name) == 0) {
				switch(sp->s.type) {
				case 'T':
				case 'D':
				case 'U':
					if (type == 'U') {
						names[id] = &sp->s;
						return;
					}
					break;
				case 't':
				case 'd':
				case 'b':
					if (type == 'b') {
						names[id] = &sp->s;
						return;
					}
					break;
				case 'a':
				case 'p':
					if (islocal(type)) {
						names[id] = &sp->s;
						return;
					}
					break;
				default:
					break;
				}
			}
	}
	sp = malloc(sizeof(Symtab));
	sp->s.name = name;
	sp->s.type = type;
	sp->s.sig = sig;
	sp->s.value = islocal(type) ? MAXOFF : 0;
	names[id] = &sp->s;
	sp->next = objhash[h];
	objhash[h] = sp;
	return;
}
/*
 *	traverse the symbol lists
 */
void
objtraverse(void (*fn)(Sym*, void*), void *pointer)
{
	int i;
	Symtab *s;

	for(i = 0; i < NHASH; i++)
		for(s = objhash[i]; s; s = s->next)
			(*fn)(&s->s, pointer);
}

/*
 * update the offset information for a 'a' or 'p' symbol in an intermediate file
 */
void
_offset(int id, vlong off)
{
	Sym *s;

	s = names[id];
	if (s && s->name[0] && islocal(s->type) && s->value > off)
		s->value = off;
}

/*
 * update the type of a global text or data symbol
 */
static void 
objupdate(int id, int type)
{
	Sym *s;

	s = names[id];
	if (s && s->name[0])
		if (s->type == 'U')
			s->type = type;
		else if (s->type == 'b')
			s->type = tolower(type);
}

/*
 * look for the next file in an archive
 */
int
nextar(Biobuf *bp, int offset, char *buf)
{
	struct ar_hdr a;
	int i, r;
	long arsize;
	if (offset&01)
		offset++;
	Bseek(bp, offset, 0);
	r = Bread(bp, &a, SAR_HDR);
	if(r != SAR_HDR)
		return 0;
	if(strncmp(a.fmag, ARFMAG, sizeof(a.fmag)))
		return -1;
	for(i=0; i<sizeof(a.name) && i<SARNAME && a.name[i] != ' '; i++)
		buf[i] = a.name[i];
	buf[i] = 0;
	arsize = strtol(a.size, 0, 0);
	if (arsize&1)
		arsize++;
	return arsize + SAR_HDR;
}

static void
objreset(void)
{
	int i;
	Symtab *s, *n;

	for(i = 0; i < NHASH; i++) {
		for(s = objhash[i]; s; s = n) {
			n = s->next;
			free(s->s.name);
			free(s);
		}
		objhash[i] = 0;
	}
	memset(names, 0, sizeof names);
}

/*
 *	The algorithm uses up to 3 temp files.  The "pivot member" is the
 *	archive member specified by and a, b, or i option.  The temp files are
 *	astart - contains existing members up to and including the pivot member.
 *	amiddle - contains new files moved or inserted behind the pivot.
 *	aend - contains the existing members that follow the pivot member.
 *	When all members have been processed, function 'install' streams the
 * 	temp files, in order, back into the archive.
 */

typedef struct	Arsymref
{
	char	*name;
	int	type;
	int	len;
	vlong	offset;
	struct	Arsymref *next;
} Arsymref;

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
	int	paged;		/* set when some data paged to disk */
	char	*fname;		/* paging file name */
	int	fd;		/* paging file descriptor */
	vlong	size;
	Armember *head;		/* head of member chain */
	Armember *tail;		/* tail of member chain */
	Arsymref *sym;		/* head of defined symbol chain */
} Arfile;

typedef struct Hashchain
{
	char	*name;
	struct Hashchain *next;
} Hashchain;

#define	NHASH	1024

/*
 *	macro to portably read/write archive header.
 *	'cmd' is read/write/Bread/Bwrite, etc.
 */
#define	HEADER_IO(cmd, f, h)	cmd(f, h.name, sizeof(h.name)) != sizeof(h.name)\
				|| cmd(f, h.date, sizeof(h.date)) != sizeof(h.date)\
				|| cmd(f, h.uid, sizeof(h.uid)) != sizeof(h.uid)\
				|| cmd(f, h.gid, sizeof(h.gid)) != sizeof(h.gid)\
				|| cmd(f, h.mode, sizeof(h.mode)) != sizeof(h.mode)\
				|| cmd(f, h.size, sizeof(h.size)) != sizeof(h.size)\
				|| cmd(f, h.fmag, sizeof(h.fmag)) != sizeof(h.fmag)

		/* constants and flags */
char	*man =		"mrxtdpq";
char	*opt =		"uvnbailo";
char	artemp[] =	"/tmp/vXXXXX";
char	movtemp[] =	"/tmp/v1XXXXX";
char	tailtemp[] =	"/tmp/v2XXXXX";
char	symdef[] =	"__.SYMDEF";

int	aflag;				/* command line flags */
int	bflag;
int	cflag;
int	oflag;
int	uflag;
int	vflag;

Arfile *astart, *amiddle, *aend;	/* Temp file control block pointers */
int	allobj = 1;			/* set when all members are object files of the same type */
int	symdefsize;			/* size of symdef file */
int	dupfound;			/* flag for duplicate symbol */
Hashchain	*hash[NHASH];		/* hash table of text symbols */
	
#define	ARNAMESIZE	sizeof(astart->tail->hdr.name)

char	poname[ARNAMESIZE+1];		/* name of pivot member */
char	*file;				/* current file or member being worked on */
Biobuf	bout;
Biobuf bar;

/* ar.c */
void main(int argc, char *argv[]);
void setcom(void (*fun)(char *, int, char **));
void plan9rcmd(char *arname, int count, char **files);
void dcmd(char *arname, int count, char **files);
void xcmd(char *arname, int count, char **files);
void pcmd(char *arname, int count, char **files);
void mcmd(char *arname, int count, char **files);
void tcmd(char *arname, int count, char **files);
void qcmd(char *arname, int count, char **files);
void scanobj(Biobuf *b, Arfile *ap, long size);
void objsym(Sym *s, void *p);
int duplicate(char *name);
int openar(char *arname, int mode, int errok);
int arcreate(char *arname);
void wrerr(void);
void rderr(void);
void phaseerr(int offset);
void usage(void);
Armember *getdir(Biobuf *b);
void armove(Biobuf *b, Arfile *ap, Armember *bp);
void arcopy(Biobuf *b, Arfile *ap, Armember *bp);
void skip(Biobuf *bp, vlong len);
void install(char *arname, Arfile *astart, Arfile *amiddle, Arfile *aend, int createflag);
void rl(int fd);
void wrsym(Biobuf *bp, long offset, Arsymref *as);
int match(int count, char **files);
int bamatch(char *file, char *pivot);
void mesg(int c, char *file);
void trim(char *s, char *buf, int n);
void longt(Armember *bp);
void pmode(long mode);
void plan9arselect(int *ap, long mode);
Arfile *newtempfile(char *name);
Armember *newmember(void);
void arread(Biobuf *b, Armember *bp, int n);
void arinsert(Arfile *ap, Armember *bp);
void arstream(int fd, Arfile *ap);
int arwrite(int fd, Armember *bp);
int page(Arfile *ap);
int getspace(void);
void arfree(Arfile *ap);
char *armalloc(int n);

void    (*comfun)(char*, int, char**);

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
		case 'o':	oflag = 1;	break;
		case 'p':	setcom(pcmd);	break;
		case 'q':	setcom(qcmd);	break;
		case 'r':	setcom(plan9rcmd);	break;
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
		setcom(plan9rcmd);
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
 *	plan9arselect a command
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
plan9rcmd(char *arname, int count, char **files)
{
	int fd;
	int i;
	Arfile *ap;
	Armember *bp;
	Dir *d;
	Biobuf *bfile;

	fd = openar(arname, ORDWR, 1);
	if (fd >= 0) {
		Binit(&bar, fd, OREAD);
		Bseek(&bar,seek(fd,0,1), 1);
	}
	astart = newtempfile(artemp);
	ap = astart;
	aend = 0;
	for(i = 0; fd >= 0; i++) {
		bp = getdir(&bar);
		if (!bp)
			break;
		if (bamatch(file, poname)) {		/* check for pivot */
			aend = newtempfile(tailtemp);
			ap = aend;
		}
			/* pitch symdef file */
		if (i == 0 && strcmp(file, symdef) == 0) {
			skip(&bar, bp->size);
			continue;
		}
		if (count && !match(count, files)) {
			scanobj(&bar, ap, bp->size);
			arcopy(&bar, ap, bp);
			continue;
		}
		bfile = Bopen(file, OREAD);
		if (!bfile) {
			if (count != 0)
				fprint(2, "ar: cannot open %s\n", file);
			scanobj(&bar, ap, bp->size);
			arcopy(&bar, ap, bp);
			continue;
		}
		d = dirfstat(Bfildes(bfile));
		if(d == nil)
			fprint(2, "ar: cannot stat %s: %r\n", file);
		if (uflag && (d==nil || d->mtime <= bp->date)) {
			scanobj(&bar, ap, bp->size);
			arcopy(&bar, ap, bp);
			Bterm(bfile);
			free(d);
			continue;
		}
		mesg('r', file);
		skip(&bar, bp->size);
		scanobj(bfile, ap, d->length);
		free(d);
		armove(bfile, ap, bp);
		Bterm(bfile);
	}
	if(fd >= 0)
		close(fd);
		/* copy in remaining files named on command line */
	for (i = 0; i < count; i++) {
		file = files[i];
		if(file == 0)
			continue;
		files[i] = 0;
		bfile = Bopen(file, OREAD);
		if (!bfile)
			fprint(2, "ar: %s cannot open\n", file);
		else {
			mesg('a', file);
			d = dirfstat(Bfildes(bfile));
			if (d == nil)
				fprint(2, "can't stat %s\n", file);
			else {
				scanobj(bfile, astart, d->length);
				armove(bfile, astart, newmember());
				free(d);
			}
			Bterm(bfile);
		}
	}
	if(fd < 0 && !cflag)
		install(arname, astart, 0, aend, 1);	/* issue 'creating' msg */
	else
		install(arname, astart, 0, aend, 0);
}

void
dcmd(char *arname, int count, char **files)
{
	Armember *bp;
	int fd, i;

	if (!count)
		return;
	fd = openar(arname, ORDWR, 0);
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	astart = newtempfile(artemp);
	for (i = 0; bp = getdir(&bar); i++) {
		if(match(count, files)) {
			mesg('d', file);
			skip(&bar, bp->size);
			if (strcmp(file, symdef) == 0)
				allobj = 0;
		} else if (i == 0 && strcmp(file, symdef) == 0)
				skip(&bar, bp->size);
		else {
			scanobj(&bar, astart, bp->size);
			arcopy(&bar, astart, bp);
		}
	}
	close(fd);
	install(arname, astart, 0, 0, 0);
}

void
xcmd(char *arname, int count, char **files)
{
	int fd, f, mode, i;
	Armember *bp;
	Dir dx;

	fd = openar(arname, OREAD, 0);
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	i = 0;
	while (bp = getdir(&bar)) {
		if(count == 0 || match(count, files)) {
			mode = strtoul(bp->hdr.mode, 0, 8) & 0777;
			f = create(file, OWRITE, mode);
			if(f < 0) {
				fprint(2, "ar: %s cannot create\n", file);
				skip(&bar, bp->size);
			} else {
				mesg('x', file);
				arcopy(&bar, 0, bp);
				if (write(f, bp->member, bp->size) < 0)
					wrerr();
				if(oflag) {
					nulldir(&dx);
					dx.atime = bp->date;
					dx.mtime = bp->date;
					if(dirwstat(file, &dx) < 0)
						perror(file);
				}
				free(bp->member);
				close(f);
			}
			free(bp);
			if (count && ++i >= count)
				break;
		} else {
			skip(&bar, bp->size);
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
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	while(bp = getdir(&bar)) {
		if(count == 0 || match(count, files)) {
			if(vflag)
				print("\n<%s>\n\n", file);
			arcopy(&bar, 0, bp);
			if (write(1, bp->member, bp->size) < 0)
				wrerr();
		} else
			skip(&bar, bp->size);
		free(bp);
	}
	close(fd);
}
void
mcmd(char *arname, int count, char **files)
{
	int fd, i;
	Arfile *ap;
	Armember *bp;

	if (count == 0)
		return;
	fd = openar(arname, ORDWR, 0);
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	astart = newtempfile(artemp);
	amiddle = newtempfile(movtemp);
	aend = 0;
	ap = astart;
	for (i = 0; bp = getdir(&bar); i++) {
		if (bamatch(file, poname)) {
			aend = newtempfile(tailtemp);
			ap = aend;
		}
		if(match(count, files)) {
			mesg('m', file);
			scanobj(&bar, amiddle, bp->size);
			arcopy(&bar, amiddle, bp);
		} else
			/*
			 * pitch the symdef file if it is at the beginning
			 * of the archive and we aren't inserting in front
			 * of it (ap == astart).
			 */
		if (ap == astart && i == 0 && strcmp(file, symdef) == 0)
			skip(&bar, bp->size);
		else {
			scanobj(&bar, ap, bp->size);
			arcopy(&bar, ap, bp);
		}
	}
	close(fd);
	if (poname[0] && aend == 0)
		fprint(2, "ar: %s not found - files moved to end.\n", poname);
	install(arname, astart, amiddle, aend, 0);
}
void
tcmd(char *arname, int count, char **files)
{
	int fd;
	Armember *bp;
	char name[ARNAMESIZE+1];

	fd = openar(arname, OREAD, 0);
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	while(bp = getdir(&bar)) {
		if(count == 0 || match(count, files)) {
			if(vflag)
				longt(bp);
			trim(file, name, ARNAMESIZE);
			Bprint(&bout, "%s\n", name);
		}
		skip(&bar, bp->size);
		free(bp);
	}
	close(fd);
}
void
qcmd(char *arname, int count, char **files)
{
	int fd, i;
	Armember *bp;
	Biobuf *bfile;

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
	Binit(&bar, fd, OREAD);
	Bseek(&bar,seek(fd,0,1), 1);
	/* leave note group behind when writing archive; i.e. sidestep interrupts */
	rfork(RFNOTEG);
	Bseek(&bar, 0, 2);
	bp = newmember();
	for(i=0; i<count && files[i]; i++) {
		file = files[i];
		files[i] = 0;
		bfile = Bopen(file, OREAD);
		if(!bfile)
			fprint(2, "ar: %s cannot open\n", file);
		else {
			mesg('q', file);
			armove(bfile, 0, bp);
			if (!arwrite(fd, bp))
				wrerr();
			free(bp->member);
			bp->member = 0;
			Bterm(bfile);
		}
	}
	free(bp);
	close(fd);
}

/*
 *	extract the symbol references from an object file
 */
void
scanobj(Biobuf *b, Arfile *ap, long size)
{
	int obj;
	vlong offset;
	Dir *d;
	static int lastobj = -1;
	if (!allobj)			/* non-object file encountered */
		return;
	offset = Boffset(b);
	obj = objtype(b, 0);
	if (obj < 0) {			/* not an object file */
		allobj = 0;
		d = dirfstat(Bfildes(b));
		if (d != nil && d->length == 0)
			fprint(2, "ar: zero length file %s\n", file);
		free(d);
		Bseek(b, offset, 0);
		return;
	}
	if (lastobj >= 0 && obj != lastobj) {
		fprint(2, "ar: inconsistent object file %s\n", file);
		allobj = 0;
		Bseek(b, offset, 0);
		return;
	}
	lastobj = obj;
	if (!readar(b, obj, offset+size, 0)) {
		fprint(2, "ar: invalid symbol reference in file %s\n", file);
		allobj = 0;
		Bseek(b, offset, 0);
		return;
	}
	Bseek(b, offset, 0);
	objtraverse(objsym, ap);
}

/*
 *	add text and data symbols to the symbol list
 */
void
objsym(Sym *s, void *p)
{
	int n;
	Arsymref *as;
	Arfile *ap;

	if (s->type != 'T' &&  s->type != 'D')
		return;
	ap = (Arfile*)p;
	as = (Arsymref*)armalloc(sizeof(Arsymref));
	as->offset = ap->size;
	n = strlen(s->name);
	as->name = armalloc(n+1);
	strcpy(as->name, s->name);
	if(s->type == 'T' && duplicate(as->name)) {
		dupfound = 1;
		fprint(2, "duplicate text symbol: %s\n", as->name);
		free(as->name);
		free(as);
		return;
	}
	as->type = s->type;
	symdefsize += 4+(n+1)+1;
	as->len = n;
	as->next = ap->sym;
	ap->sym = as;
}

/*
 *	Check the symbol table for duplicate text symbols
 */
int
duplicate(char *name)
{
	Hashchain *p;
	char *cp;
	int h;

	h = 0;
	for(cp = name; *cp; h += *cp++)
		h *= 1119;
	if(h < 0)
		h = ~h;
	h %= NHASH;

	for(p = hash[h]; p; p = p->next)
		if(strcmp(p->name, name) == 0)
			return 1;
	p = (Hashchain*) armalloc(sizeof(Hashchain));
	p->next = hash[h];
	p->name = name;
	hash[h] = p;
	return 0;
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
getdir(Biobuf *b)
{
	Armember *bp;
	char *cp;
	static char name[ARNAMESIZE+1];

	bp = newmember();
	if(HEADER_IO(Bread, b, bp->hdr)) {
		free(bp);
		return 0;
	}
	if(strncmp(bp->hdr.fmag, ARFMAG, sizeof(bp->hdr.fmag)) != 0)
		phaseerr(Boffset(b));
	strncpy(name, bp->hdr.name, sizeof(bp->hdr.name));
	cp = name+sizeof(name)-1;
	*cp = '\0';
	/* skip trailing spaces and (gnu-produced) slashes */
	while(*--cp == ' ' || *cp == '/')
		;
	cp[1] = '\0';
	file = name;
	bp->date = strtol(bp->hdr.date, 0, 0);
	bp->size = strtol(bp->hdr.size, 0, 0);
	return bp;
}

/*
 *	Copy the file referenced by fd to the temp file
 */
void
armove(Biobuf *b, Arfile *ap, Armember *bp)
{
	char *cp;
	Dir *d;

	d = dirfstat(Bfildes(b));
	if (d == nil) {
		fprint(2, "ar: cannot stat %s\n", file);
		return;
	}
	trim(file, bp->hdr.name, sizeof(bp->hdr.name));
	for (cp = strchr(bp->hdr.name, 0);		/* blank pad on right */
		cp < bp->hdr.name+sizeof(bp->hdr.name); cp++)
			*cp = ' ';
	sprint(bp->hdr.date, "%-12ld", d->mtime);
	sprint(bp->hdr.uid, "%-6d", 0);
	sprint(bp->hdr.gid, "%-6d", 0);
	sprint(bp->hdr.mode, "%-8lo", d->mode);
	sprint(bp->hdr.size, "%-10lld", d->length);
	strncpy(bp->hdr.fmag, ARFMAG, 2);
	bp->size = d->length;
	arread(b, bp, bp->size);
	if (d->length&0x01)
		d->length++;
	if (ap) {
		arinsert(ap, bp);
		ap->size += d->length+SAR_HDR;
	}
	free(d);
}

/*
 *	Copy the archive member at the current offset into the temp file.
 */
void
arcopy(Biobuf *b, Arfile *ap, Armember *bp)
{
	long n;

	n = bp->size;
	if (n & 01)
		n++;
	arread(b, bp, n);
	if (ap) {
		arinsert(ap, bp);
		ap->size += n+SAR_HDR;
	}
}

/*
 *	Skip an archive member
 */
void
skip(Biobuf *bp, vlong len)
{
	if (len & 01)
		len++;
	Bseek(bp, len, 1);
}

/*
 *	Stream the three temp files to an archive
 */
void
install(char *arname, Arfile *astart, Arfile *amiddle, Arfile *aend, int createflag)
{
	int fd;

	if(allobj && dupfound) {
		fprint(2, "%s not changed\n", arname);
		return;
	}
	/* leave note group behind when copying back; i.e. sidestep interrupts */
	rfork(RFNOTEG);

	if(createflag)
		fprint(2, "ar: creating %s\n", arname);
	fd = arcreate(arname);

	if(allobj)
		rl(fd);

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

void
rl(int fd)
{

	Biobuf b;
	char *cp;
	struct ar_hdr a;
	long len;

	Binit(&b, fd, OWRITE);
	Bseek(&b,seek(fd,0,1), 0);

	len = symdefsize;
	if(len&01)
		len++;
	sprint(a.date, "%-12ld", time(0));
	sprint(a.uid, "%-6d", 0);
	sprint(a.gid, "%-6d", 0);
	sprint(a.mode, "%-8lo", 0644L);
	sprint(a.size, "%-10ld", len);
	strncpy(a.fmag, ARFMAG, 2);
	strcpy(a.name, symdef);
	for (cp = strchr(a.name, 0);		/* blank pad on right */
		cp < a.name+sizeof(a.name); cp++)
			*cp = ' ';
	if(HEADER_IO(Bwrite, &b, a))
			wrerr();

	len += Boffset(&b);
	if (astart) {
		wrsym(&b, len, astart->sym);
		len += astart->size;
	}
	if(amiddle) {
		wrsym(&b, len, amiddle->sym);
		len += amiddle->size;
	}
	if(aend)
		wrsym(&b, len, aend->sym);

	if(symdefsize&0x01)
		Bputc(&b, 0);
	Bterm(&b);
}

/*
 *	Write the defined symbols to the symdef file
 */
void
wrsym(Biobuf *bp, long offset, Arsymref *as)
{
	int off;

	while(as) {
		Bputc(bp, as->type);
		off = as->offset+offset;
		Bputc(bp, off);
		Bputc(bp, off>>8);
		Bputc(bp, off>>16);
		Bputc(bp, off>>24);
		if (Bwrite(bp, as->name, as->len+1) != as->len+1)
			wrerr();
		as = as->next;
	}
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
		trim(files[i], name, ARNAMESIZE);
		if(strncmp(name, file, ARNAMESIZE) == 0) {
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
			if (strncmp(file, pivot, ARNAMESIZE) == 0)
				state = 1;
		} else if (bflag) {
			if (strncmp(file, pivot, ARNAMESIZE) == 0) {
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
	Bprint(&bout, "%3ld/%1ld", strtol(bp->hdr.uid, 0, 0), strtol(bp->hdr.gid, 0, 0));
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
		plan9arselect(*mp++, mode);
}

void
plan9arselect(int *ap, long mode)
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
arread(Biobuf *b, Armember *bp, int n)	/* read an image into a member buffer */
{
	int i;

	bp->member = armalloc(n);
	i = Bread(b, bp->member, n);
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

	if(HEADER_IO(write, fd, bp->hdr))
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
