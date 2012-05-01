/*
 * nm.c -- drive nm
 */
#include <u.h>
#include <libc.h>
#include <ar.h>
#include <bio.h>
#include <mach.h>

enum{
	CHUNK	=	256	/* must be power of 2 */
};

char	*errs;			/* exit status */
char	*filename;		/* current file */
char	symname[]="__.SYMDEF";	/* table of contents file name */
int	multifile;		/* processing multiple files */
int	aflag;
int	gflag;
int	hflag;
int	nflag;
int	sflag;
int	uflag;
int	Tflag;


typedef struct  Sym     Sym;
struct  Sym
{
        vlong   value;
        uint    sig;
        char    type;
        char    *name;
};

typedef struct	Prog	Prog;

typedef enum Kind		/* variable defs and references in obj */
{
	aNone,			/* we don't care about this prog */
	aName,			/* introduces a name */
	aText,			/* starts a function */
	aData,			/* references to a global object */
} Kind;

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

Sym	**fnames;		/* file path translation table */
Sym	**symptr;
int	nsym;
Biobuf	bout;

int	cmp(void*, void*);
void	error(char*, ...);
void	execsyms(int);
void	psym(Sym*, void*);
void	printsyms(Sym**, long);
void	doar(Biobuf*);
void	dofile(Biobuf*);
void	zenter(Sym*);

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
	[ObjAmd64]	"amd64 .6",	NULL, _read6,
	[Maxobjtype]	0, 0
};

struct	Symtab
{
	struct	Sym 	s;
	struct	Symtab	*next;
};

static	Symtab *objhash[NHASH];
static	Sym	*names[NNAMES];	/* working set of active names */

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


void
usage(void)
{
	fprint(2, "usage: nm [-aghnsTu] file ...\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i;
	Biobuf	*bin;

	Binit(&bout, 1, OWRITE);
	argv0 = argv[0];
	ARGBEGIN {
	default:	usage();
	case 'a':	aflag = 1; break;
	case 'g':	gflag = 1; break;
	case 'h':	hflag = 1; break;
	case 'n':	nflag = 1; break;
	case 's':	sflag = 1; break;
	case 'u':	uflag = 1; break;
	case 'T':	Tflag = 1; break;
	} ARGEND
	if (argc == 0)
		usage();
	if (argc > 1)
		multifile++;
	for(i=0; i<argc; i++){
		filename = argv[i];
		bin = Bopen(filename, OREAD);
		if(bin == 0){
			error("cannot open %s", filename);
			continue;
		}
		if (isar(bin))
			doar(bin);
		else{
			Bseek(bin, 0, 0);
			dofile(bin);
		}
		Bterm(bin);
	}
	exits(errs);
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
 * read an archive file,
 * processing the symbols for each intermediate file in it.
 */
void
doar(Biobuf *bp)
{
	int offset, size, obj;
	char membername[SARNAME];

	multifile = 1;
	for (offset = Boffset(bp);;offset += size) {
		size = nextar(bp, offset, membername);
		if (size < 0) {
			error("phase error on ar header %ld", offset);
			return;
		}
		if (size == 0)
			return;
		if (strcmp(membername, symname) == 0)
			continue;
		obj = objtype(bp, 0);
		if (obj < 0) {
			error("inconsistent file %s in %s",
					membername, filename);
			return;
		}
		if (!readar(bp, obj, offset+size, 1)) {
			error("invalid symbol reference in file %s",
					membername);
			return;
		}
		filename = membername;
		nsym=0;
		objtraverse(psym, 0);
		printsyms(symptr, nsym);
	}
}

/*
 * process symbols in a file
 */
void
dofile(Biobuf *bp)
{
	int obj;

	obj = objtype(bp, 0);
	if (obj < 0)
		execsyms(Bfildes(bp));
	else
	if (readobj(bp, obj)) {
		nsym = 0;
		objtraverse(psym, 0);
		printsyms(symptr, nsym);
	}
}

/*
 * comparison routine for sorting the symbol table
 *	this screws up on 'z' records when aflag == 1
 */
int
cmp(void *vs, void *vt)
{
	Sym **s, **t;

	s = vs;
	t = vt;
	if(nflag)
		if((*s)->value < (*t)->value)
			return -1;
		else
			return (*s)->value > (*t)->value;
	return strcmp((*s)->name, (*t)->name);
}
/*
 * enter a symbol in the table of filename elements
 */
void
zenter(Sym *s)
{
	static int maxf = 0;

	if (s->value > maxf) {
		maxf = (s->value+CHUNK-1) &~ (CHUNK-1);
		fnames = realloc(fnames, (maxf+1)*sizeof(*fnames));
		if(fnames == 0) {
			error("out of memory", argv0);
			exits("memory");
		}
	}
	fnames[s->value] = s;
}

/*
 *	initialize the symbol tables
 */
int
syminit(int fd, Fhdr *fp)
{
	Sym *p;
	long i, l, size;
	vlong vl;
	Biobuf b;
	int svalsz;

	if(fp->symsz == 0)
		return 0;
	if(fp->type == FNONE)
		return 0;

	cleansyms();
	textseg(fp->txtaddr, fp);
		/* minimum symbol record size = 4+1+2 bytes */
	symbols = malloc((fp->symsz/(4+1+2)+1)*sizeof(Sym));
	if(symbols == 0) {
		werrstr("can't malloc %ld bytes", fp->symsz);
		return -1;
	}
	Binit(&b, fd, OREAD);
	Bseek(&b, fp->symoff, 0);
	nsym = 0;
	size = 0;
	for(p = symbols; size < fp->symsz; p++, nsym++) {
		if(fp->_magic && (fp->magic & HDR_MAGIC)){
			svalsz = 8;
			if(Bread(&b, &vl, 8) != 8)
				return symerrmsg(8, "symbol");
			p->value = beswav(vl);
		}
		else{
			svalsz = 4;
			if(Bread(&b, &l, 4) != 4)
				return symerrmsg(4, "symbol");
			p->value = (u32int)beswal(l);
		}
		if(Bread(&b, &p->type, sizeof(p->type)) != sizeof(p->type))
			return symerrmsg(sizeof(p->value), "symbol");

		i = decodename(&b, p);
		if(i < 0)
			return -1;
		size += i+svalsz+sizeof(p->type);

		/* count global & auto vars, text symbols, and file names */
		switch (p->type) {
		case 'l':
		case 'L':
		case 't':
		case 'T':
			ntxt++;
			break;
		case 'd':
		case 'D':
		case 'b':
		case 'B':
			nglob++;
			break;
		case 'f':
			if(strcmp(p->name, ".frame") == 0) {
				p->type = 'm';
				nauto++;
			}
			else if(p->value > fmax)
				fmax = p->value;	/* highest path index */
			break;
		case 'a':
		case 'p':
		case 'm':
			nauto++;
			break;
		case 'z':
			if(p->value == 1) {		/* one extra per file */
				nhist++;
				nfiles++;
			}
			nhist++;
			break;
		default:
			break;
		}
	}
	if (debug)
		print("NG: %ld NT: %d NF: %d\n", nglob, ntxt, fmax);
	if (fp->sppcsz) {			/* pc-sp offset table */
		spoff = (uchar *)malloc(fp->sppcsz);
		if(spoff == 0) {
			werrstr("can't malloc %ld bytes", fp->sppcsz);
			return -1;
		}
		Bseek(&b, fp->sppcoff, 0);
		if(Bread(&b, spoff, fp->sppcsz) != fp->sppcsz){
			spoff = 0;
			return symerrmsg(fp->sppcsz, "sp-pc");
		}
		spoffend = spoff+fp->sppcsz;
	}
	if (fp->lnpcsz) {			/* pc-line number table */
		pcline = (uchar *)malloc(fp->lnpcsz);
		if(pcline == 0) {
			werrstr("can't malloc %ld bytes", fp->lnpcsz);
			return -1;
		}
		Bseek(&b, fp->lnpcoff, 0);
		if(Bread(&b, pcline, fp->lnpcsz) != fp->lnpcsz){
			pcline = 0;
			return symerrmsg(fp->lnpcsz, "pc-line");
		}
		pclineend = pcline+fp->lnpcsz;
	}
	return nsym;
}

static int
symerrmsg(int n, char *table)
{
	werrstr("can't read %d bytes of %s table", n, table);
	return -1;
}

/*
 * get the symbol table from an executable file, if it has one
 */
void
execsyms(int fd)
{
	Fhdr f;
	Sym *s;
	long n;

	seek(fd, 0, 0);
	if (crackhdr(fd, &f) == 0) {
		error("Can't read header for %s", filename);
		return;
	}
	if (syminit(fd, &f) < 0)
		return;
	s = symbase(&n);
	nsym = 0;
	while(n--)
		psym(s++, 0);

	printsyms(symptr, nsym);
}

void
psym(Sym *s, void* p)
{
	USED(p);
	switch(s->type) {
	case 'T':
	case 'L':
	case 'D':
	case 'B':
		if (uflag)
			return;
		if (!aflag && ((s->name[0] == '.' || s->name[0] == '$')))
			return;
		break;
	case 'b':
	case 'd':
	case 'l':
	case 't':
		if (uflag || gflag)
			return;
		if (!aflag && ((s->name[0] == '.' || s->name[0] == '$')))
			return;
		break;
	case 'U':
		if (gflag)
			return;
		break;
	case 'Z':
		if (!aflag)
			return;
		break;
	case 'm':
	case 'f':	/* we only see a 'z' when the following is true*/
		if(!aflag || uflag || gflag)
			return;
		if (strcmp(s->name, ".frame"))
			zenter(s);
		break;
	case 'a':
	case 'p':
	case 'z':
	default:
		if(!aflag || uflag || gflag)
			return;
		break;
	}
	symptr = realloc(symptr, (nsym+1)*sizeof(Sym*));
	if (symptr == 0) {
		error("out of memory");
		exits("memory");
	}
	symptr[nsym++] = s;
}

void
printsyms(Sym **symptr, long nsym)
{
	int i, wid;
	Sym *s;
	char *cp;
	char path[512];

	if(!sflag)
		qsort(symptr, nsym, sizeof(*symptr), cmp);
	
	wid = 0;
	for (i=0; i<nsym; i++) {
		s = symptr[i];
		if (s->value && wid == 0)
			wid = 8;
		else if (s->value >= 0x100000000LL && wid == 8)
			wid = 16;
	}	
	for (i=0; i<nsym; i++) {
		s = symptr[i];
		if (multifile && !hflag)
			Bprint(&bout, "%s:", filename);
		if (s->type == 'z') {
			fileelem(fnames, (uchar *) s->name, path, 512);
			cp = path;
		} else
			cp = s->name;
		if (Tflag)
			Bprint(&bout, "%8ux ", s->sig);
		if (s->value || s->type == 'a' || s->type == 'p')
			Bprint(&bout, "%*llux ", wid, s->value);
		else
			Bprint(&bout, "%*s ", wid, "");
		Bprint(&bout, "%c %s\n", s->type, cp);
	}
}

void
error(char *fmt, ...)
{
	Fmt f;
	char buf[128];
	va_list arg;

	fmtfdinit(&f, 2, buf, sizeof buf);
	fmtprint(&f, "%s: ", argv0);
	va_start(arg, fmt);
	fmtvprint(&f, fmt, arg);
	va_end(arg);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
	errs = "errors";
}
