#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#define	HUGEINT	0x7fffffff

typedef	struct txtsym Txtsym;		/* Text Symbol table */

struct txtsym {
	int 	n;			/* number of local vars */
	Sym	**locals;		/* array of ptrs to autos */
	Sym	*sym;			/* function symbol entry */
};

static	Txtsym	*txt;			/* Base of text symbol table */
static	int	ntxt;			/* number of text symbols */

typedef	struct hist Hist;		/* Stack of include files & #line directives */

struct hist {
	char	*name;			/* Assumes names Null terminated in file */
	long	line;			/* line # where it was included */
	long	offset;			/* line # of #line directive */
};

typedef	struct	file File;		/* Per input file header to history stack */

struct file {
	long		addr;		/* address of first text sym */
	union {
		Txtsym		*txt;		/* first text symbol */
		Sym		*sym;		/* only during initilization */
	};
	int		n;		/* size of history stack */
	Hist		*hist;		/* history stack */
};

static	int	isbuilt;		/* set when tables built */
static	int	nfiles;			/* number of files */
static	File	*files;			/* Base of file arena */
static	Sym	**globals;		/* globals by addr table */
static	long	nglob;			/* number of globals */
static	Sym	**fnames;		/* file names path component table */
static	int	fmax;			/* largest file path index */
static	int	debug = 0;

#define min(a, b)	((a)<(b)?(a):(b))

static int	encfname(char *, short *);
static int	eqsym(Sym *, char *);
static Hist 	*fline(char *, int, long, Hist *);
static void	fillsym(Sym *, Symbol *);
static int	findglobal(char *, Symbol *);
static int	findlocvar(Symbol *, char *, Symbol *);
static int	findtext(char *, Symbol *);
static int	hcomp(Hist *, short *, int);
static int	hline(File *, short *, int n, ulong *);
static void	printhist(char *, Hist *, int);
static int	buildtbls(void);
static int	symcomp(void *, void *);
static int	txtcomp(void *, void *);
static int	filecomp(void *, void *);

/*
 *	initialize internal symbol tables
 */
static int
buildtbls(void)
{
	
	Sym *p;
	int i, j, na, nh, ng, nt;
	File *f;
	Txtsym *tp;
	Hist *hp;
	Sym **ap;

	if (isbuilt)
		return 1;
	isbuilt = 1;
		/* count global vars, text symbols, and file names */
	na = nh = 0;
	for (p = symbase(&i); i-- > 0; p++) {
		p->value = beswal(p->value);
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
			if (strcmp(p->name, ".frame") == 0) {
				p->type = 'm';
				na++;
			} else if (p->value > fmax)
				fmax = p->value;	/* highest path index */
			break;
		case 'a':
		case 'p':
		case 'm':
			na++;
			break;
		case 'z':
			if (p->value == 1) {		/* one extra per file */
				nh++;
				nfiles++;
			}
			nh++;
			break;
		default:
			break;
		}
	}
	if (debug)
		print ("NG: %d NT: %d NF: %d\n", nglob, ntxt, fmax);
			/* allocate the tables */
	if (nglob) {
		globals = malloc(nglob*sizeof(*globals));
		if (!globals) {
			symerror = "can't allocate global symbol table\n";
			return 0;
		}
	}
	if (ntxt) {
		txt = malloc(ntxt*sizeof(*txt));
		if (!txt) {
			symerror = "can't allocate text symbol table\n";
			return 0;
		}
	}
	fmax++;
	fnames = malloc(fmax*sizeof(*fnames));
	if (!fnames) {
		symerror = "can't allocate file name table\n";
		return 0;
	}
	memset(fnames, 0, fmax*sizeof(*fnames));
	files = malloc(nfiles*sizeof(*files));
	if (!files) {
		symerror = "can't allocate file table\n";
		return 0;
	}
	hp = malloc(nh*sizeof(Hist));
	if (hp == 0) {
		symerror = "can't allocate history stack\n";
		return .0;
	}
	ap = malloc(na*sizeof(Sym*));
	if (ap == 0) {
		symerror = "can't allocate automatic vector\n";
		return 0;
	}
		/* load the tables */
	ng = nt = nh = 0;
	f = 0;
	tp = 0;
	for (p = symbase(&i); i-- > 0; p++) {
		switch(p->type) {
		case 'D':
		case 'd':
		case 'B':
		case 'b':
			if (debug)
				print("Global: %s %lux\n", p->name, p->value);
			globals[ng++] = p;
			break;
		case 'z':
			if (p->value == 1) {		/* New file */
				if (f) {
					f->n = nh;
					f->hist[nh].name = 0;	/* one extra */
					hp += nh+1;
					f++;
				}
				else f = files;
				f->hist = hp;
				f->sym = 0;
				f->addr = 0;
				nh = 0;
			}
				/* alloc one slot extra as terminator */
			f->hist[nh].name = p->name;
			f->hist[nh].line = p->value;
			f->hist[nh].offset = 0;
			if (debug)
				printhist("-> ", &f->hist[nh], 1);
			nh++;
			break;
		case 'Z':
			if (f && nh > 0)
				f->hist[nh-1].offset = p->value;
			break;
		case 'T':
		case 't':	/* Text: terminate history if first in file */
		case 'L':
		case 'l':
			tp = &txt[nt++];
			tp->n = 0;
			tp->sym = p;
			tp->locals = ap;
			if (debug)
				print("TEXT: %s at %lux\n", p->name, p->value);
			if (f && !f->sym) {			/* first  */
				f->sym = p;
				f->addr = p->value;
			}
			break;
		case 'a':
		case 'p':
		case 'm':		/* Local Vars */
			if (!tp)
				print("Warning: Free floating local var");
			else {
				if (debug)
					print("Local: %s %lux\n", p->name, p->value);
				tp->locals[tp->n] = p;
				tp->n++;
				ap++;
			}
			break;
		case 'f':		/* File names */
			if (debug)
				print("Fname: %s\n", p->name);
			fnames[p->value] = p;
			break;
		default:
			break;
		}
	}
		/* sort global and text tables into ascending address order */
	qsort(globals, nglob, sizeof(Sym*), symcomp);
	qsort(txt, ntxt, sizeof(Txtsym), txtcomp);
	qsort(files, nfiles, sizeof(File), filecomp);
	tp = txt;
	for (i = 0, f = files; i < nfiles; i++, f++) {
		for (j = 0; j < ntxt; j++) {
			if (f->sym == tp->sym) {
				if (debug) {
					print("LINK: %s to", f->sym->name);
					printhist("... ", f->hist, 1);
				}
				f->txt = tp++;
				break;
			}
			if (++tp >= txt+ntxt)	/* wrap around */
				tp = txt;
		}
	}
	return 1;
}

/*
 * find symbol function.var by name.
 *	fn != 0 && var != 0	=> look for fn in text, var in data
 *	fn != 0 && var == 0	=> look for fn in text
 *	fn == 0 && var != 0	=> look for var first in text then in data space.
 */
int
lookup(char *fn, char *var, Symbol *s)
{
	int found;

	symerror = 0;
	if (!isbuilt)
		if (buildtbls() == 0)
			return 0;
	if (fn) {
		found = findtext(fn, s);
		if (var == 0)		/* case 2: fn not in text */
			return found;
		else if (!found)	/* case 1: fn not found */
			return 0;
	} else if (var) {
		found = findtext(var, s);
		if (found)
			return 1;	/* case 3: var found in text */
	} else return 0;		/* case 4: fn & var == zero */
	if (found)
		return findlocal(s, var, s);	/* case 1: fn found */
	return findglobal(var, s);		/* case 3: var not found */

}
/*
 * find a function by name
 */
static int
findtext(char *name, Symbol *s)
{
	int i;

	for (i = 0; i < ntxt; i++) {
		if (eqsym(txt[i].sym, name)) {
			fillsym(txt[i].sym, s);
			s->handle = (void *) &txt[i];
			return 1;
		}
	}
	return 0;
}
/*
 * find global variable by name
 */
static int
findglobal(char *name, Symbol *s)
{
	int i;

	for (i = 0; i < nglob; i++) {
		if (eqsym(globals[i], name)) {
			fillsym(globals[i], s);
			return 1;
		}
	}
	return 0;
}
/*
 *	find the local variable by name within a given function
 */
int
findlocal(Symbol *s1, char *name, Symbol *s2)
{
	symerror = 0;
	if (s1 == 0)
		return 0;
	if (buildtbls() == 0)
		return 0;
	return findlocvar(s1, name, s2);
}
/*
 *	find the local variable by name within a given function
 *		(internal function - does no parameter validation)
 */
static int
findlocvar(Symbol *s1, char *name, Symbol *s2)
{
	Txtsym *tp;
	int i;

	tp = (Txtsym *) s1->handle;
	if (tp && tp->locals) {
		for (i = 0; i < tp->n; i++)
			if (eqsym(tp->locals[i], name)) {
				fillsym(tp->locals[i], s2);
				s2->handle = (void *) tp;
				return 1;
			}
	}
	return 0;
}
/*
 *	Get ith text symbol
 */
int
textsym(Symbol *s, int index)
{

	if (buildtbls() == 0)
		return 0;
	if (index >= ntxt)
		return 0;
	fillsym(txt[index].sym, s);
	s->handle = (void *) &txt[index];
	return 1;
}
/*	
 *	Get ith file name
 */
int
filesym(int index, char *buf, int n)
{
	Hist *hp;

	if (buildtbls() == 0)
		return 0;
	if (index >= nfiles)
		return 0;
	hp = files[index].hist;
	if (!hp || !hp->name)
		return 0;
	return fileelem(fnames, (uchar*) hp->name, buf, n);
}
/*
 *	Lookup name of local variable located at an offset into the frame.
 *	The type selects either a parameter or automatic.
 */
int
getauto(Symbol *s1, int off, int type, Symbol *s2)
{
	Txtsym *tp;
	Sym *p;
	int i, t;

	symerror = 0;
	if (s1 == 0)
		return 0;
	if (type == CPARAM)
		t = 'p';
	else if (type == CAUTO)
		t = 'a';
	else return 0;
	if (buildtbls() == 0)
		return 0;
	tp = (Txtsym *) s1->handle;
	if (tp == 0)
		return 0;
	for (i = 0; i < tp->n; i++) {
		p = tp->locals[i];
		if (p->type == t && p->value == off) {
			fillsym(p, s2);
			s2->handle = s1->handle;
			return 1;
		}
	}
	return 0;
}

/*
 * Find symbol containing val; binary search assumes both text & data
 *		arrays are sorted by addr
 */
int
findsym(long w, int type, Symbol *s)
{
	ulong val;
	int top, bot, mid;
	Sym *sp;

	symerror = 0;
	if (buildtbls() == 0)
		return 0;
	val = w;
	if (type == CTEXT || type == CANY) {
		bot = 0;
		top = ntxt;
		for (mid = (bot+top)/2; mid < top; mid = (bot+top)/2) {
			sp = txt[mid].sym;
			if (val < (ulong)sp->value)
				top = mid;
			else if (mid != ntxt-1 && val >= txt[mid+1].sym->value)
				bot = mid;
			else {
				if (mid == ntxt-1 && type == CANY)
					break;
				fillsym(sp, s);
				s->handle = (void *) &txt[mid];
				return 1;
			}
		}
	}
	if (type == CDATA || type == CANY) {
		bot = 0;
		top = nglob;
		for (mid = (bot+top)/2; mid < top; mid = (bot+top)/2) {
			sp = globals[mid];
			if (val < (ulong)sp->value)
				top = mid;
			else if (mid < nglob-1 && val >= globals[mid+1]->value)
				bot = mid;
			else {
				fillsym(sp, s);
				return 1;
			}
		}
	}
	return 0;
}
/*
 * get the ith local symbol for a function
 * the input symbol table is reverse ordered, so we reverse
 * accesses here to maintain approx. parameter ordering in a stack trace.
 */
int
localsym(Symbol *s, int index)
{
	Txtsym *tp;

	symerror = 0;
	if (s == 0)
		return 0;
	if (buildtbls() == 0)
		return 0;
	tp = (Txtsym *) s->handle;
	if (tp && tp->locals && index < tp->n) {
		fillsym(tp->locals[tp->n-index-1], s);	/* reverse */
		s->handle = (void *) tp;
		return 1;
	}
	return 0;
}
/*
 * get the ith global symbol
 */
int
globalsym(Symbol *s, int index)
{
	symerror = 0;
	if (s == 0)
		return 0;
	if (buildtbls() == 0)
		return 0;
	if (index < nglob) {
		fillsym(globals[index], s);
		return 1;
	}
	return 0;
}
/*
 *	find the pc given a file name and line offset into it.
 */
long
file2pc(char *file, ulong line)
{
	File *fp;
	int i, n;
	long pc;
	ulong start, end;
	short name[NNAME];

	symerror = 0;
	if (buildtbls() == 0 || files == 0)
		return -1;
	n = encfname(file, name);		/* encode the file name */
	if (n == 0) {
		print("file not found");
		return -1;
	} 
		/* find this history stack */
	for (i = 0, fp = files; i < nfiles; i++, fp++)
		if (hline(fp, name, n, &line))
			break;
	if (i >= nfiles) {
		symerror = "file or line not found";
		return -1;
	}
	start = fp->addr;		/* first text addr this file */
	if (i+1 < nfiles)
		end = (fp+1)->addr;	/* first text addr next file */
	else
		end = 0;		/* last file in load module */
	/*
	 * At this point, line contains the offset into the file.
	 * run the state machine to locate the pc closest to that value.
	 */
	if (debug)
		print("find pc for %d - between: %lux and %lux\n", line, start, end);
	pc = line2addr(line, start, end);
	if (pc == -1) {
		symerror = "Line not found in specified file";
		return -1;
	}
	return pc;
}
/*
 *	search for a path component index
 */
static int
pathcomp(char *s, int n)
{
	int i;

	for (i = 0; i <= fmax; i++)
		if (fnames[i] && strncmp(s, fnames[i]->name, n) == 0)
			return i;
	return -1;
}
/*
 *	Encode a char file name as a sequence of short indices
 *	into the file name dictionary.
 */
static int
encfname(char *file, short *dest)
{
	int i, j;
	char *cp, *cp2;

	if (*file == '/') {	/* always check first '/' */
		cp2 = file+1;
	} else {
		cp2 = strchr(file, '/');
		if (!cp2)
			cp2 = strchr(file, 0);
	}
	cp = file;
	for (i = 0; *cp; i++) {
		j = pathcomp(cp, cp2-cp);
		if (j < 0)
			return 0;	/* not found */
		dest[i] = j;
		cp = cp2;
		while (*cp == '/')	/* skip embedded '/'s */
			cp++;
		cp2 = strchr(cp, '/');
		if (!cp2)
			cp2 = strchr(cp, 0);
	}
	dest[i] = 0;
	return i;
}
/*
 *	Search a history stack for a matching file name accumulating
 *	the size of intervening files in the stack.
 */
static int
hline(File *fp, short *name, int n, ulong *line)
{
	Hist *hp;
	int offset, depth;
	long ln;

	for (hp = fp->hist; hp->name; hp++)		/* find name in stack */
		if (hp->name[1] || hp->name[2]) {
			if (hcomp(hp, name, n))
				break;
		}
	if (!hp->name)		/* match not found */
		return 0;
	if (debug)
		printhist("hline found ... ", hp, 1);
	/* unwind the stack until empty or we hit an entry beyond our line */
	ln = *line;
	offset = hp->line-1;
	depth = 1;
	for (hp++; depth && hp->name; hp++) {
		if (debug)
			printhist("hline inspect ... ", hp, 1);
		if (hp->name[1] || hp->name[2]) {
			if (hp->offset){		/* Z record */
				offset = 0;
				if (hcomp(hp, name, n)) {
					if (*line <= hp->offset)
						break;
					ln = *line+hp->line-hp->offset;
					depth = 1;	/* implicit pop */
				} else depth = 2;	/* implicit push */
			} else if (depth == 1 && ln < hp->line-offset)
					break;		/* Beyond our line */
			else if (depth++ == 1)	/* push	*/
				offset -= hp->line;
		} else if (--depth == 1)		/* pop */
			offset += hp->line;	
	}
	*line = ln+offset;
	return 1;
}
/*
 *	compare two encoded file names
 */
static int
hcomp(Hist *hp, short *sp, int n)
{
	uchar *cp;

	for (cp = (uchar *) hp->name+1; n--; cp += 2) {
		if (*sp != ((*cp<<8) | cp[1]))	/* not short aligned */
			return 0;
		if (*sp++ == 0)
			break;
	}
	return 1;
}
/*
 *	Convert a pc to a "file:line {file:line}" string.
 */
int
fileline(char *str, int n, ulong dot)
{
	long line;
	int i;
	File *f, *found;
	int delta;

	symerror = 0;
	if (buildtbls() == 0)
		return 0;
	*str = 0;
	delta = HUGEINT;
	found = 0;
		/* The file list isn't sorted */
	for (i = 0, f = files; i < nfiles; i++, f++) {
		if (f->txt && dot >= f->addr && dot-f->addr < delta) {
			delta = dot-f->addr;
			found = f;
			if (delta == 0)
				break;
		}
	}
	if (!found)
		return 0;
	line = pc2line(dot);
	if (line > 0 && fline(str, n, line, found->hist))
		return 1;
	return 0;
}
/*
 *	Convert a line number within a composite file to relative line
 *	number in a source file.  a composite file, is the source
 *	file with included files inserted in line.
 */
static Hist *
fline(char *str, int n, long line, Hist *base)
{
	Hist *start;			/* start of current level */
	Hist *h;			/* current entry */
	int delta;			/* sum of size of files this level */
	int k;

	start = h = base;
	delta = h->line;
	while(h && h->name && line > h->line) {
		if (h->name[1] || h->name[2]) {
			if (h->offset != 0) {	/* #line Directive */
				delta = h->line-h->offset+1;
				start = base = h++;
			} else {		/* beginning of File */
				if (start == base)
					start = h++;
				else {
					h = fline(str, n, line, start);
					if (h == 0)
						break;
				}
			}
		} else {
			if (start == base)	/* end of recursion level */
				return h;
			else {			/* end of included file */
				delta += (h++)->line-start->line;
				start = base;
			}
		}
	}
	if (!h)
		return 0;
	if (start != base)
		line = line-start->line+1;
	else line = line-delta+1;
	if (!h->name)
		strncpy(str, "<eof>", n);
	else {
		k = fileelem(fnames, (uchar*) start->name, str, n);
		if (k+8 < n)
			sprint(str+k, ":%ld", line);
	}
/**********Remove comments if complete trace of include sequence desired
 *	if (start != base) {
 *		k = strlen(str);
 *		if (k+2 < n) {
 *			str[k++] = ' ';
 *			str[k++] = '{';
 *		}
 *		k += fileelem(fnames, (uchar*) base->name, str+k, n-k);
 *		if (k+10 < n)
 *			sprint(str+k, ":%ld}", start->line-delta);
 *	}
 ********************/
	return h;
}
/*
 *	compare the values of two symbol table entries.
 */
static int
symcomp(void *a, void *b)
{
	return (*(Sym**)a)->value - (*(Sym**)b)->value;
}
/*
 *	compare the values of the symbols referenced by two text table entries
 */
static int
txtcomp(void *a, void *b)
{
	return ((Txtsym*)a)->sym->value - ((Txtsym*)b)->sym->value;
}
/*
 *	compare the values of the symbols referenced by two file table entries
 */
static int
filecomp(void *a, void *b)
{
	return ((File*)a)->addr - ((File*)b)->addr;
}
/*
 * compare symbol name with a string.
 * hacks like leading _ and length restrictions go here
 */
static int
eqsym(Sym *sp, char *s)
{
	return (strcmp(sp->name, s) == 0);
}
/*
 *	fill an interface Symbol structure from a symbol table entry
 */
static void
fillsym(Sym *sp, Symbol *s)
{
	s->type = sp->type;
	s->value = sp->value;
	s->name = sp->name;
	switch(sp->type) {
	case 'b':
	case 'B':
	case 'D':
	case 'd':
		s->class = CDATA;
		break;
	case 't':
	case 'T':
	case 'l':
	case 'L':
		s->class = CTEXT;
		break;
	case 'a':
		s->class = CAUTO;
		break;
	case 'p':
		s->class = CPARAM;
		break;
	case 'm':
		s->class = CSTAB;
		break;
	default:
		s->class = CNONE;
		break;
	}
	s->handle = 0;
}
/*
 *	Print a history stack (debug). if count is 0, prints the whole stack
 */
void
printhist(char *msg, Hist *hp, int count)
{
	int i;
	uchar *cp;
	char buf[128];

	if (count)
		i = 1;
	else i = 0;
	while(hp->name && (count -= i) >= 0) {
		print("%s Line: %x (%d)  Offset: %x (%d)  Name: ", msg,
			hp->line, hp->line, hp->offset, hp->offset);
		for (cp = (uchar *) hp->name+1;(*cp<<8)|cp[1]; cp += 2) {
			if (cp != (uchar *) hp->name+1)
				print("/");
			print("%x", (*cp<<8)|cp[1]);
		}
		fileelem(fnames, (uchar *) hp->name, buf, sizeof(buf));
		print(" (%s)\n", buf);
		hp++;
	}
}

#ifdef DEBUG
/*
 *	print the history stack for a file. (debug only)
 *	if (name == 0) => print all history stacks.
 */
void
dumphist(char *name)
{
	int i;
	File *f;
	short fname[NNAME/2];

	if (buildtbls() == 0)
		return;
	if (name)
		encfname(name, fname);
	for (i = 0, f = files; i < nfiles; i++, f++) {
		if (name == 0 || hcomp(f->hist, fname, NNAME/2)) {
			printhist("> ", f->hist, f->n);
		}
	}
}
#endif
