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
int	multifile,		/* processing multiple files */
	aflag,
	gflag,
	hflag,
	nflag,
	sflag,
	uflag,
	vflag;

Sym	**fnames;		/* file path translation table */
Biobuf	bout;

int	cmp(Sym **, Sym **);
void	error(char*, ...),
	execsyms(Biobuf*),
	printsyms(Sym*, long),
	psym(Sym**, long),
	doar(Biobuf*),
	dofile(Biobuf*),
	zenter(Sym*);

void
main(int argc, char *argv[])
{
	int i;
	Biobuf	*bin;

	Binit(&bout, 1, OWRITE);
	argv0 = argv[0];
	ARGBEGIN {
	case 'a':	aflag = 1; break;
	case 'g':	gflag = 1; break;
	case 'h':	hflag = 1; break;
	case 'n':	nflag = 1; break;
	case 's':	sflag = 1; break;
	case 'u':	uflag = 1; break;
	case 'v':	vflag = 1; break;
	} ARGEND
	if (argc > 1)
		multifile++;
	for(i=0; i<argc; i++){
		filename = argv[i];
		bin = Bopen(filename, OREAD);
		if(bin == 0){
			error("cannot open %s\n", filename);
			continue;
		}
		if (isar(bin))
			doar(bin);
		else{
			Bseek(bin, 0, 0);
			dofile(bin);
		}
		Bclose(bin);
	}
	exits(errs);
}

/*
 * read an archive file,
 * processing the symbols for each intermediate file in it.
 */
void
doar(Biobuf *bp)
{
	int offset, size, obj;
	Sym *s;
	long n;
	char membername[NNAME];

	multifile = 1;
	for (offset = BOFFSET(bp);;offset += size) {
		objreset();
		size = nextar(bp, offset, membername);
		if (size < 0) {
			error("phase error on ar header %ld\n", offset);
			return;
		}
		if (size == 0)
			return;
		if (strcmp(membername, symname) == 0)
			continue;
		obj = objtype(bp);
		if (obj < 0) {
			error("inconsistent file %s in %s\n",
					membername, filename);
			return;
		}
		if (!readar(bp, obj, offset+size)) {
			error("invalid symbol reference in file %s\n",
					membername);
			return;
		}
		s = objbase(&n);
		filename = membername;
		printsyms(s, n);
	}
}

/*
 * process symbols in a file
 */
void
dofile(Biobuf *bp)
{
	Sym *s;
	long n;
	int obj;

	obj = objtype(bp);
	if (obj < 0)
		execsyms(bp);
	else {
		objreset();
		if (readobj(bp, obj)) {
			s = objbase(&n);
			printsyms(s, n);
		}
	}
}

/*
 * comparison routine for sorting the symbol table
 *	this screws up on 'z' records when aflag == 1
 */
int
cmp(Sym **s, Sym **t)
{
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
		fnames = realloc(fnames, maxf*sizeof(*fnames));
		if(fnames == 0) {
			error("out of memory\n", argv0);
			exits("memory");
		}
	}
	fnames[s->value] = s;
}

/*
 * get the symbol table from an executable file, if it has one
 */
void
execsyms(Biobuf *bp)
{
	Fhdr f;
	Sym *s;
	long nsyms;

	Bseek(bp, 0, 0);
	if (crackhdr(Bfildes(bp), &f) == 0) {
		error("Can't read header for %s\n", filename);
		return;
	}
	if (syminit(Bfildes(bp), &f) < 0)
		return;
	s = symbase(&nsyms);
	printsyms(s, nsyms);
	Bclose(bp);
}

void
printsyms(Sym *sp, long nsym)
{
	Sym *s;
	Sym **symptr;
	int n;

	n = nsym*sizeof(Sym*);
	symptr = malloc(n);
	if (symptr == 0) {
		error("out of memory\n");
		return;
	}
	memset(symptr, 0, n);
	for (s = sp, n = 0; nsym-- > 0; s++) {
		s->value = beswal(s->value);
		switch(s->type) {
		case 'T':
		case 'L':
		case 'D':
		case 'B':
			if (!uflag)
				if (aflag || ((s->name[0] != '.'
						&& s->name[0] != '$')))
					symptr[n++] = s;
			break;
		case 'b':
		case 'd':
		case 'l':
		case 't':
			if (!uflag && !gflag)
				if (aflag || ((s->name[0] != '.'
						&& s->name[0] != '$')))
					symptr[n++] = s;
			break;
		case 'U':
			if (!gflag)
				symptr[n++] = s;
			break;
		case 'Z':
			if (aflag)
				symptr[n++] = s;
			break;
		case 'f':	/* we only see a 'z' when the following is true*/
			if (aflag && !uflag && !gflag) {
				if (strcmp(s->name, ".frame"))
					zenter(s);
				symptr[n++] = s;
			}
			break;
		case 'a':
		case 'p':
		case 'm':
		case 'z':
		default:
			if (aflag && !uflag && !gflag)
				symptr[n++] = s;
			break;
		}
	}
	if(!sflag)
		qsort(symptr, n, sizeof(*symptr), cmp);
	psym(symptr, n);
	free(symptr);
}

void
psym(Sym **symptr, int nsym)
{
	Sym *s;
	char *cp;
	char path[512];

	while (nsym-- > 0) {
		s = *symptr++;
		if (multifile && !hflag)
			Bprint(&bout, "%s:", filename);
		if (s->type == 'z') {
			fileelem(fnames, (uchar *) s->name, path, 512);
			cp = path;
		} else
			cp = s->name;
		if (s->value || s->type == 'a' || s->type == 'p')
			Bprint(&bout, "%8lux %c %s\n", s->value, s->type, cp);
		else
			Bprint(&bout, "         %c %s\n", s->type, cp);
	}
}

void
error(char *fmt, ...)
{
	char buf[4096], *s;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	errs = "errors";
}
