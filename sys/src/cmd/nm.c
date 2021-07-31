/*
 * nm.c -- drive nm
 */
#include <u.h>
#include <libc.h>
#include <ar.h>
#include <bio.h>
#include "mach.h"
#include "obj.h"


Sym	*_sym;			/* for obj files */
Sym	**sptrs;		/* for executable files */
Sym	**fnames;		/* file path translation table */

char	*errs,
	*_filename,
	_firstname[NNAME],
	_symname[] = "__.SYMDEF";
int	nfname,			/* number of file name pieces for 'z' symbols*/
	multifile,		/* processing multiple files */
	aflag, gflag, hflag, nflag, sflag, uflag, vflag;

Biobuf	*_bin;
Biobuf	_bout;

int	cmp(Sym **, Sym **);
void	error(char*, ...),
	execsyms(char *),
	printsyms(Sym *, long),
	printsym(Sym **, long),
	doar(void),
	dofile(void),
	zenter(Sym *);

void
main(int argc, char *argv[])
{
	char magbuf[SARMAG];
	int i, n;

	Binit(&_bout, 1, OWRITE);
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
	_assure(CHUNK);
	multifile = argc > 1;
	for(i=0; i<argc; i++){
		_filename = argv[i];
		_bin = Bopen(_filename, OREAD);
		if(_bin == 0){
			fprint(2, "%s: cannot open %s\n", argv0, _filename);
			continue;
		}
		n = Bread(_bin, magbuf, SARMAG);
		if(n == SARMAG && strncmp(magbuf, ARMAG, SARMAG) == 0)
			doar();
		else{
			Bseek(_bin, 0, 0);
			dofile();
		}
		Bclose(_bin);
	}
	exits(errs);
}

/*
 * read an archive file,
 * processing the symbols for each intermediate file in it.
 */
void
doar(void)
{
	_symsize = 0;
	_off = 0;
	multifile = 1;
	while(_nextar())
		_objsyms(1, 1, printsyms);
}

/*
 * process symbols in a file
 */
void
dofile(void)
{
	if(_objsyms(0, 1, printsyms) == 0)
		execsyms(_filename);
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
		do {
			maxf += CHUNK;
		} while (s->value > maxf);
		fnames = realloc(fnames, maxf*sizeof(*fnames));
		if(fnames == 0){
			fprint(2, "%s: out of memory\n", argv0);
			exits("memory");
		}
	}
	fnames[s->value] = s;
}

/* executable nm */

/*
 * get the symbol table from an executable file, if it has one
 */
void
execsyms(char *file)
{
	Fhdr f;
	Sym *s;
	long nsyms;

	Bseek(_bin, 0, 0);
	if (crackhdr(Bfildes(_bin), &f) == 0) {
		error("Can't read header for %s\n", file);
		return;
	}
	if (syminit(Bfildes(_bin), &f) < 0)
		return;
	s = symbase(&nsyms);
	printsyms(s, nsyms);
	Bclose(_bin);
}

void
printsyms(Sym *sp, long nsym)
{
	Sym *s;
	Sym **symptr;
	int n;

	n = nsym*sizeof(Sym*);
	symptr = malloc(n);
	memset(symptr, 0, n);
	if (symptr == 0) {
		error("out of memory\n");
		return;
	}
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
	printsym(symptr, n);
}

void
printsym(Sym **symptr, int nsym)
{
	Sym *s;
	char *cp;
	char path[512];

	while (nsym-- > 0) {
		s = *symptr++;
		if (multifile && !hflag)
			Bprint(&_bout, "%s:", _filename);
		if (s->type == 'z') {
			fileelem(fnames, (uchar *) s->name, path, 512);
			cp = path;
		} else
			cp = s->name;
		if (s->value || s->type == 'a' || s->type == 'p')
			Bprint(&_bout, "%8lux %c %s\n", s->value, s->type, cp);
		else
			Bprint(&_bout, "         %c %s\n", s->type, cp);
	}
}

void
error(char *fmt, ...)
{
	char buf[8192], *s;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	errs = "errors";
	return;
}
