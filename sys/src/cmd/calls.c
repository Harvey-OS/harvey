/*
 * calls - print a paragraphed list of who calls whom within a body of C source
 *
 * Author: M.M. Taylor, DCIEM, Toronto, Canada.
 * Modified by Alexis Kwan (HCR at DCIEM),
 *	Kevin Szabo (watmath!wateng!ksbszabo, Elec Eng, U of Waterloo),
 *	Tony Hansen, AT&T-IS, pegasus!hansen.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <String.h>

#define CPP		"cpp -+"
#define RINSTERR	((Rinst *)-1)	/* ugly; error but keep going */

#define STREQ(a, b)	(*(a) == *(b) && strcmp(a, b) == 0)
#define OTHER(rdwr)	((rdwr) == Rd? Wr: Rd)
/* per 8c, all multibyte runes are considered alphabetic */
#define ISIDENT(r) (isascii(r) && isalnum(r) || (r) == '_' || (r) >= Runeself)

/* safe macros */
#define checksys(atom)		strbsearch(atom, sysword, nelem(sysword))

enum {
	Printstats =	0,		/* flag */
	Maxseen =	4000,		/* # of instances w/in a function */
	Maxdepth =	300,		/* max func call tree depth */
	Hashsize =	2048,

	Maxid =		256 + UTFmax,	/* max size of name */
	Tabwidth =	8,
	Maxwidth =	132,		/* limits tabbing */
	Defwidth =	80,		/* limits tabbing */

	Backslash =	'\\',
	Quote =		'\'',

	Rd =		0,		/* pipe indices */
	Wr,

	Stdin =		0,
	Stdout,
	Stderr,

	Defn =		0,
	Decl,
	Call,

	Nomore =	-1,
	Added,
	Found,
};

typedef struct Pushstate Pushstate;
typedef struct Rinst Rinst;
typedef struct Root Root;
typedef struct Rname Rname;
typedef struct Rnamehash Rnamehash;

struct Pushstate {
	int	kid;
	int	fd;	/* original fd */
	int	rfd;	/* replacement fd */
	int	input;
	int	open;
};

struct Rname {
	Rinst	*dlistp;
	int	rnameout;
	char	rnamecalled;
	char	rnamedefined;
	char	*namer;
	Rname	*next;		/* next in hash chain */
};

struct Rnamehash {
	Rname	*head;
};

/* list of calling instances of those names */
struct Rinst {
	Rname	*namep;
	Rinst	*calls;
	Rinst	*calledby;
};

struct Root {
	char	*func;
	Root	*next;
};

char *aseen[Maxseen];		/* names being gathered within a function */
Rnamehash nameshash[Hashsize];	/* names being tracked */
Rname *activelist[Maxdepth];	/* names being output */
String *cppopt;
Root *roots;

static struct stats {
	long	highestseen;	/* aseen high water mark */
	long	highestname;	/* namelist high water mark */
	long	highestact;	/* activelist high water mark */
	long	highgetfree;	/* getfrees high water mark */
} stats;

static long getfrees = 0;

int bracket = 0;			/* curly brace count in input */
int linect = 0;				/* line number in output */
int activep = 0;			/* current function being output */

char *infile;
int lineno = 1;				/* line number of input */
int prevc = '\n', thisc = '\n';

/* options */
int terse = 1;				/* track functions only once */
int ntabs = (Maxwidth - 20) / Tabwidth;	/* how wide to go */

char *dashes;				/* separators for deep nestings */

/*
 * These are C tokens after which a parenthesis is valid which would
 * otherwise be tagged as function names.  The reserved words which are not
 * listed are break, const, continue, default, goto and volatile.
 */
char *sysword[] = {
	"auto", "case", "char", "do", "double", "else", "enum",
	"extern", "float", "for", "if", "int", "long", "register",
	"return", "short", "sizeof", "static", "struct", "switch",
	"typedef", "union", "unsigned", "void", "while",
};

/*
 * warning - print best error message possible
 */
void
warning(char *s1, char *s2)
{
	fprint(2, "%s: ", argv0);
	fprint(2, s1, s2);
	fprint(2, "\n");
}

/*
 *   safe malloc() code.  Does the checking for nil returns from malloc()
 */
void *
emalloc(int size)
{
	void *f = mallocz(size, 1);

	if (f == nil)
		sysfatal("cannot allocate memory");
	return f;
}

unsigned
hash(char *s)
{
	unsigned h;
	unsigned char *cp;

	h = 0;
	for(cp = (unsigned char *)s; *cp; h += *cp++)
		h *= 1119;
	return h;
}

int
nextc(Biobuf *in)
{
	prevc = thisc;
	thisc = Bgetrune(in);
	return thisc;
}

int
ungetc(Biobuf *in)
{
	prevc = thisc;
	return Bungetrune(in);
}

int
newatom(Biobuf *in, char *atom)
{
	atom[0] = '\0';
	return nextc(in);
}

/*
 * lookup (name) accepts a pointer to a name and sees if the name is on the
 * namelist.  If so, it returns a pointer to the nameblock.  Otherwise it
 * returns nil.
 */
Rname *
lookfor(char *name)
{
	int i;
	unsigned buck;
	Rname *np;
	Rnamehash *hp;

	buck = hash(name) % Hashsize;
	hp = &nameshash[buck];
	i = 0;
	for (np = hp->head; np != nil; np = np->next, i++)
		if (STREQ(name, np->namer))
			return np;

	if (i > stats.highestname)
		stats.highestname = i;
	return nil;
}

/*
 * place() returns a pointer to the name on the namelist.  If the name was
 * not in the namelist, place adds it.
 */
Rname *
place(char name[])
{
	unsigned buck;
	Rname *np;
	Rnamehash *hp;

	np = lookfor(name);
	if (np != nil)
		return np;

	buck = hash(name) % Hashsize;
	hp = &nameshash[buck];
	np = emalloc(sizeof *np);
	np->namer = strdup(name);
	np->next = hp->head;
	hp->head = np;
	return np;
}

/*
 * getfree returns a pointer to the next free instance block on the list
 */
Rinst *
getfree(void)
{
	Rinst *ret, *new;
	static Rinst *prev;

	++getfrees;
	if (getfrees > stats.highgetfree)
		stats.highgetfree = getfrees;

	if (prev == nil)
		prev = emalloc(sizeof *prev);
	new = emalloc(sizeof *new);

	prev->calls = new;		/* also serves as next pointer */
	new->calledby = prev;

	ret = prev;
	prev = new;
	return ret;
}

/*
 * install(np, rp) puts a new instance of a function into the linked list.
 * It puts a pointer (np) to its own name (returned by place) into its
 * namepointer, a pointer to the calling routine (rp) into its called-by
 * pointer, and zero into the calls pointer.  It then puts a pointer to
 * itself into the last function in the chain.
 */
Rinst *
install(Rname *np, Rinst *rp)
{
	Rinst *newp;

	if (np == nil)
		return RINSTERR;
	if ((newp = getfree()) == nil)
		return nil;
	newp->namep = np;
	newp->calls = 0;
	if (rp) {
		while (rp->calls)
			rp = rp->calls;
		newp->calledby = rp->calledby;
		rp->calls = newp;
	} else {
		newp->calledby = (Rinst *)np;
		np->dlistp = newp;
	}
	return newp;
}

/*
 * When scanning the text, each function instance is inserted into a
 * linear list of names, using the Rname structure, when it is first
 * encountered.  It is also inserted into the linked list using the Rinst
 * structure.  The entry into the name list has a pointer to the defining
 * instance in the linked list, and each entry in the linked list has a
 * pointer back to the relevant name.  Newproc makes an entry in the
 * defining list, which is distinguished from the called list only
 * because it has no calledby link (value=0).  Add2proc enters into the
 * called list, by inserting a link to the new instance in the calls
 * pointer of the last entry (may be a defining instance, or a function
 * called by that defining instance), and points back to the defining
 * instance of the caller in its called-by pointer.
 */
Rinst *
newproc(char *name)
{
	int i;
	Rname *rp;

	for (i = 0; i < Maxseen; i++)
		if (aseen[i] != nil) {
			free(aseen[i]);
			aseen[i] = nil;
		}
	rp = place(name);
	if (rp == nil)
		return RINSTERR;
	/* declaration in a header file is enough to cause this. */
	if (0 && rp->rnamedefined)
		warning("function `%s' redeclared", name);
	rp->rnamedefined = 1;
	return install(rp, nil);
}

/*
 * add the function name to the calling stack of the current function.
 */
int
add2call(char name[], Rinst *curp)
{
	Rname *p = place(name);
	Rinst *ip = install(p, curp);

	if (p != nil && curp != nil && curp->namep != nil &&
	    !STREQ(p->namer, curp->namep->namer))
		p->rnamecalled = 1;
	return ip != nil;
}

/*
 * backup removes an item from the active stack
 */
void
backup(void)
{
	if (activep > 0)
		activelist[--activep] = nil;
}

/*
 * makeactive simply puts a pointer to the nameblock into a stack with
 * maximum depth Maxdepth.  the error return only happens for stack
 * overflow.
 */
int
makeactive(Rname *func)
{
	if (activep < Maxdepth) {
		if (activep > stats.highestact)
			stats.highestact = activep;
		activelist[activep++] = func;
		return 1;
	}
	return 0;
}

/*
 * active checks whether the pointer which is its argument has already
 * occurred on the active list, and returns 1 if so.
 */
int
active(Rname *func)
{
	int i;

	for (i = 0; i < activep - 1; i++)
		if (func == activelist[i])
			return 1;
	return 0;
}

/*
 * output is a recursive routine to print one tab for each level of
 * recursion, then the name of the function called, followed by the next
 * function called by the same higher level routine.  In doing this, it
 * calls itself to output the name of the first function called by the
 * function whose name it is printing.  It maintains an active list of
 * functions currently being printed by the different levels of
 * recursion, and if it finds itself asked to print one which is already
 * active, it terminates, marking that call with a '*'.
 */
void
output(Rname *func, int tabc)
{
	int i, tabd, tabstar, tflag;
	Rinst *nextp;

	++linect;
	print("\n%d", linect);
	if (!makeactive(func)) {
		print("   * nesting is too deep");
		return;
	}
	tabstar = 0;
	tabd = tabc;
	for (; tabd > ntabs; tabstar++)
		tabd -= ntabs;
	if (tabstar > 0) {
		print("  ");
		for (i = 0; i < tabstar; i++)
			print("<");
	}
	if (tabd == 0)
		print("   ");
	else
		for (i = 0; i < tabd; i++)
			print("\t");
	if (active(func))
		print("<<< %s", func->namer);		/* recursive call */
	else if (func->dlistp == nil)
		print("%s [external]", func->namer);
	else {
		print("%s", func->namer);
		nextp = func->dlistp->calls;
		if (!terse || !func->rnameout) {
			++tabc;
			if (!func->rnameout)
				func->rnameout = linect;
			if (tabc > ntabs && tabc%ntabs == 1 && nextp) {
				print("\n%s", dashes);
				tflag = 1;
			} else
				tflag = 0;
			for (; nextp; nextp = nextp->calls)
				output(nextp->namep, tabc);
			if (tflag) {
				print("\n%s", dashes);
				tflag = 0;
				USED(tflag);
			}
		} else if (nextp != nil)		/* not a leaf */
			print(" ... [see line %d]",  func->rnameout);
	}
	backup();
}

/*
 * Dumptree() lists out the calling stacks.  All names will be listed out
 * unless some function names are specified in -f options.
 */
void
dumptree(void)
{
	unsigned buck;
	Root *rp;
	Rname *np;

	if (roots != nil)
		for (rp = roots; rp != nil; rp = rp->next)
			if ((np = lookfor(rp->func)) != nil) {
				output(np, 0);
				print("\n\n");
			} else
				fprint(2, "%s: function '%s' not found\n",
					argv0, rp->func);
	else
		/* print everything */
		for (buck = 0; buck < Hashsize; buck++)
			for (np = nameshash[buck].head; np != nil; np = np->next)
				if (!np->rnamecalled) {
					output(np, 0);
					print("\n\n");
				}
}

/*
 * Skipcomments() skips past any blanks and comments in the input stream.
 */
int
skipcomments(Biobuf *in, int firstc)
{
	int c;

	for (c = firstc; isascii(c) && isspace(c) || c == '/'; c = nextc(in)) {
		if (c == '\n')
			lineno++;
		if (c != '/')
			continue;
		c = nextc(in);			/* read ahead */
		if (c == Beof)
			break;
		if (c != '*' && c != '/') {	/* not comment start? */
			ungetc(in);		/* push back readahead */
			return '/';
		}
		if (c == '/') {			/* c++ style */
			while ((c = nextc(in)) != '\n' && c != Beof)
				;
			if (c == '\n')
				lineno++;
			continue;
		}
		for (;;) {
			/* skip to possible closing delimiter */
			while ((c = nextc(in)) != '*' && c != Beof)
				if (c == '\n')
					lineno++;
			if (c == Beof)
				break;
			/* else c == '*' */
			c = nextc(in);		 /* read ahead */
			if (c == Beof || c == '/') /* comment end? */
				break;
			ungetc(in);		/* push back readahead */
		}
	}
	return c;
}

/*
 * isfndefn differentiates between an external declaration and a real
 * function definition.  For instance, between:
 *
 *	extern char *getenv(char *), *strcmp(char *, char *);
 *  and
 *	char *getenv(char *name)
 *	{}
 *
 * It does so by making the observation that nothing (except blanks and
 * comments) can be between the right parenthesis and the semi-colon or
 * comma following the extern declaration.
 */
int
isfndefn(Biobuf *in)
{
	int c;

	c = skipcomments(in, nextc(in));
	while (c != ')' && c != Beof)	/* consume arg. decl.s */
		c = nextc(in);
	if (c == Beof)
		return 1;		/* definition at Beof */
	c = skipcomments(in, nextc(in)); /* skip blanks between ) and ; */

	if (c == ';' || c == ',')
		return 0;		/* an extern declaration */
	if (c != Beof)
		ungetc(in);
	return 1;			/* a definition */
}

/*
 * Binary search -- from Knuth (6.2.1) Algorithm B.  Special case like this
 * is WAY faster than the generic bsearch().
 */
int
strbsearch(char *key, char **base, unsigned nel)
{
	int cmp;
	char **last = base + nel - 1, **pos;

	while (last >= base) {
		pos = base + ((last - base) >> 1);
		cmp = key[0] - (*pos)[0];
		if (cmp == 0) {
			/* there are no empty strings in the table */
			cmp = strcmp(key, *pos);
			if (cmp == 0)
				return 1;
		}
		if (cmp < 0)
			last = pos - 1;
		else
			base = pos + 1;
	}
	return 0;
}

/*
 * see if we have seen this function within this process
 */
int
seen(char *atom)
{
	int i;

	for (i = 0; aseen[i] != nil && i < Maxseen-1; i++)
		if (STREQ(atom, aseen[i]))
			return Found;
	if (i >= Maxseen-1)
		return Nomore;
	aseen[i] = strdup(atom);
	if (i > stats.highestseen)
		stats.highestseen = i;
	return Added;
}

/*
 * getfunc returns the name of a function in atom and Defn for a definition,
 * Call for an internal call, or Beof.
 */
int
getfunc(Biobuf *in, char *atom)
{
	int c, nf, last, ss, quote;
	char *ln, *nm, *ap, *ep = &atom[Maxid-1-UTFmax];
	char *flds[4];
	Rune r;

	c = nextc(in);
	while (c != Beof) {
		if (ISIDENT(c)) {
			ap = atom;
			do {
				if (isascii(c))
					*ap++ = c;
				else {
					r = c;
					ap += runetochar(ap, &r);
				}
				c = nextc(in);
			} while(ap < ep && ISIDENT(c));
			*ap = '\0';
			if (ap >= ep) {	/* uncommon case: id won't fit */
				/* consume remainder of too-long id */
				while (ISIDENT(c))
					c = nextc(in);
			}
		}

		switch (c) {
		case Beof:
			return Beof;
		case '\n':
			lineno++;
			/* fall through */
		case '\t':		/* ignore white space */
		case ' ':
		case '\f':
		case '\r':
		case '/':		/* potential comment? */
			c = skipcomments(in, nextc(in));
			break;
		case Backslash:		/* consume a newline or something */
		case ')':		/* end of parameter list */
		default:
			c = newatom(in, atom);
			break;
		case '#':
			if (prevc != '\n') {	/* cpp # or ## operator? */
				c = nextc(in);	/* read ahead */
				break;
			}
			/* it's a cpp directive */
			ln = Brdline(in, '\n');
			if (ln == nil)
				thisc = c = Beof;
			else {
				nf = tokenize(ln, flds, nelem(flds));
				if (nf >= 3 && strcmp(flds[0], "line") == 0) {
					lineno = atoi(flds[1]);
					free(infile);
					nm = flds[2];
					if (nm[0] == '"')
						nm++;
					last = strlen(nm) - 1;
					if (nm[last] == '"')
						nm[last] = '\0';
					infile = strdup(nm);
				} else
					lineno++;
				c = nextc(in);	/* read ahead */
			}
			break;
		case Quote:		/* character constant */
		case '\"':		/* string constant */
			quote = c;
			atom[0] = '\0';
			while ((c = nextc(in)) != quote && c != Beof)
				if (c == Backslash)
					nextc(in);
			if (c == quote)
				c = nextc(in);
			break;
		case '{':		/* start of a block */
			bracket++;
			c = newatom(in, atom);
			break;
		case '}':		/* end of a block */
			if (bracket < 1)
				fprint(2, "%s: %s:%d: too many closing braces; "
					"previous open brace missing\n",
					argv0, infile, lineno);
			else
				--bracket;
			c = newatom(in, atom);
			break;
		case '(':		/* parameter list for function? */
			if (atom[0] != '\0' && !checksys(atom)) {
				if (bracket == 0)
					if (isfndefn(in))
						return Defn;
					else {
						c = nextc(in);
						break;		/* ext. decl. */
					}
				ss = seen(atom);
				if (ss == Nomore)
					fprint(2, "%s: %s:%d: more than %d "
						"identifiers in a function\n",
						argv0, infile, lineno, Maxseen);
				if (bracket > 0 && ss == Added)
					return Call;
			}
			c = newatom(in, atom);
			break;
		}
	}
	return Beof;
}

/*
 * addfuncs() scans the input file for function names and adds them to the
 * calling list.
 */
void
addfuncs(int infd)
{
	int intern;
	uintptr ok = 1;
	char atom[Maxid];
	Biobuf inbb;
	Biobuf *in;
	Rinst *curproc = nil;

	in = &inbb;
	Binit(in, infd, OREAD);
	atom[0] = '\0';
	while ((intern = getfunc(in, atom)) != Beof && ok)
		if (intern == Call)
			ok = add2call(atom, curproc);	/* function call */
		else
			ok = (uintptr)(curproc = newproc(atom)); /* fn def'n */
	Bterm(in);
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
		execl("/bin/rc", "rc", "-c", cmd, nil);
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
 * invoke the C preprocessor on the named files so that its
 * output can be read.
 *
 * must fork/exec cpp for each input file.
 * otherwise we get macro redefinitions and other problems.
 * also plan 9's cpp can only process one input file per invocation.
 */
void
scanfiles(int argc, char **argv)
{
	int i, infd;
	char *sts;
	Pushstate ps;
	String *cmd;

	cmd = s_new();
	for (i = 0; i < argc; i++) {
		s_reset(cmd);
		s_append(cmd, s_to_c(cppopt));
		s_append(cmd, " ");
		s_append(cmd, argv[i]);

		infd = push(Stdin, s_to_c(cmd), Rd, &ps);
		if (infd < 0) {
			warning("can't execute cmd `%s'", s_to_c(cmd));
			return;
		}

		free(infile);
		infile = strdup(argv[i]);
		lineno = 1;
		addfuncs(infd);

		sts = pushclose(&ps);
		if (sts != nil && sts[0] != '\0') {
			warning("cmd `%s' failed", s_to_c(cmd));
			fprint(2, "exit status %s\n", sts);
		}
	}
	s_free(cmd);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-ptv] [-f func] [-w width] [-D define] [-U undef]"
		" [-I dir] [file...]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, width = Defwidth;
	char _dashes[1024];
	Root *rp;

	cppopt = s_copy(CPP);
	ARGBEGIN{
	case 'f':			/* start from function arg. */
		rp = emalloc(sizeof *rp);
		rp->func = EARGF(usage());
		rp->next = roots;
		roots = rp;
		break;
	case 'p':			/* ape includes */
		s_append(cppopt, " -I /sys/include/ape");
		s_append(cppopt, " -I /");
		s_append(cppopt, getenv("objtype"));
		s_append(cppopt, "/include/ape");
		break;
	case 't':			/* terse (default) */
		terse = 1;
		break;
	case 'v':
		terse = 0;
		break;
	case 'w':			/* output width */
		width = atoi(EARGF(usage()));
		if (width <= 0)
			width = Defwidth;
		break;
	case 'D':
	case 'I':
	case 'U':
		s_append(cppopt, " -");
		s_putc(cppopt, ARGC());
		s_append(cppopt, EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND

	/* initialize the dashed separator list for deep nesting */
	ntabs = (width - 20) / Tabwidth;
	for (i = 0; i < width && i+1 < sizeof dashes; i += 2) {
		_dashes[i] = '-';
		_dashes[i+1] = ' ';
	}
	if (i < sizeof dashes)
		_dashes[i] = '\0';
	else
		_dashes[sizeof dashes - 1] = '\0';
	dashes = _dashes;

	scanfiles(argc, argv);
	dumptree();

	if (Printstats) {
		fprint(2, "%ld/%d aseen entries\n", stats.highestseen, Maxseen);
		fprint(2, "%ld longest namelist hash chain\n", stats.highestname);
		fprint(2, "%ld/%d activelist high water mark\n",
			stats.highestact, Maxdepth);
		fprint(2, "%ld dlist high water mark\n", stats.highgetfree);
	}
	exits(0);
}
