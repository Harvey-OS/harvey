#include <u.h>
#include <libc.h>

/*
   POSIX standard c89

   standard options: -c, -D name[=val], -E (preprocess to stdout),
       -g, -L dir, -o outfile, -O, -s, -U name
       (and operands can have -l lib interspersed)
   
    nonstandard but specified options: -S (assembly language left in .s),
       -Wx,arg1[,arg2...] (pass arg(s) to phase x, where x is p (cpp)
   			 0 (compiler), or l (loader)
    nonstandard options: -v (echo real commands to stdout as they execute)
	-A: turn on ANSI prototype warnings
 */

typedef struct Objtype {
	char	*name;
	char	*cc;
	char	*ld;
	char	*o;
} Objtype;

Objtype objtype[] = {
	{"68020",	"2c", "2l", "2"},
	{"arm",		"5c", "5l", "5"},
	{"amd64",	"6c", "6l", "6"},
	{"alpha",	"7c", "7l", "7"},
	{"386",		"8c", "8l", "8"},
	{"sparc",	"kc", "kl", "k"},
	{"power",	"qc", "ql", "q"},
	{"mips",	"vc", "vl", "v"},
};

enum {
	Nobjs = (sizeof objtype)/(sizeof objtype[0]),
	Maxlist = 2000,
};

typedef struct List {
	char	*strings[Maxlist];
	int	n;
} List;

List	srcs, objs, cpp, cc, ld, ldargs, srchlibs;
int	cflag, vflag, Eflag, Sflag, Aflag;
char	*allos = "2678kqv";

void	append(List *, char *);
char	*changeext(char *, char *);
void	doexec(char *, List *);
void	dopipe(char *, List *, char *, List *);
void	fatal(char *);
Objtype	*findoty(void);
void	printlist(List *);
char *searchlib(char *, char*);

void
main(int argc, char *argv[])
{
	char *s, *suf, *ccpath, *lib;
	char *oname;
	int haveoname = 0;
	int i, cppn, ccn;
	Objtype *ot;

	ot = findoty();
	oname = "a.out";
	append(&cpp, "cpp");
	append(&cpp, "-D__STDC__=1");	/* ANSI says so */
	append(&cpp, "-D_POSIX_SOURCE=");
	append(&cpp, "-N");		/* turn off standard includes */
	append(&cc, ot->cc);
	append(&ld, ot->ld);
	append(&srchlibs, smprint("/%s/lib/ape", ot->name));
	while(argc > 0) {
		ARGBEGIN {
		case 'c':
			cflag = 1;
			break;
		case 'l':
			lib = searchlib(ARGF(), ot->name);
			if(!lib)
				fprint(2, "cc: can't find library for -l\n");
			else
				append(&objs, lib);
			break;
		case 'o':
			oname = ARGF();
			haveoname = 1;
			if(!oname)
				fatal("cc: no -o argument");
			break;
		case 'D':
		case 'I':
		case 'U':
			append(&cpp, smprint("-%c%s", ARGC(), ARGF()));
			break;
		case 'E':
			Eflag = 1;
			cflag = 1;
			break;
		case 's':
		case 'g':
			break;
		case 'L':
			lib = ARGF();
			if(!lib)
				fprint(2, "cc: no -L argument\n");
			else
				append(&srchlibs, lib);
			break;
		case 'N':
		case 'T':
		case 'w':
			append(&cc, smprint("-%c", ARGC()));
			break;
		case 'O':
			break;
		case 'W':
			s = ARGF();
			if(s && s[1]==',') {
				switch (s[0]) {
				case 'p':
					append(&cpp, s+2);
					break;
				case '0':
					append(&cc, s+2);
					break;
				case 'l':
					append(&ldargs, s+2);
					break;
				default:
					fprint(2, "cc: pass letter after -W should be one of p0l; ignored\n");
				}
			} else
				fprint(2, "cc: bad option after -W; ignored\n");
			break;
		case 'v':
			vflag = 1;
			append(&ldargs, "-v");
			break;
		case 'A':
			Aflag = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		default:
			fprint(2, "cc: flag -%c ignored\n", ARGC());
			break;
		} ARGEND
		if(!Aflag) {
			append(&cc, "-J");		/* old/new decl mixture hack */
			append(&cc, "-B");		/* turn off non-prototype warnings */
		}
		if(argc > 0) {
			s = argv[0];
			suf = utfrrune(s, '.');
			if(suf) {
				suf++;
				if(strcmp(suf, "c") == 0) {
					append(&srcs, s);
					append(&objs, changeext(s, "o"));
				} else if(strcmp(suf, "o") == 0 ||
					  strcmp(suf, ot->o) == 0 ||
					  strcmp(suf, "a") == 0 ||
					  (suf[0] == 'a' && strcmp(suf+1, ot->o) == 0)) {
					append(&objs, s);
				} else if(utfrune(allos, suf[0]) != 0) {
					fprint(2, "cc: argument %s ignored: wrong architecture\n",
						s);
				}
			}
		}
	}
	if(objs.n == 0)
		fatal("no files to compile or load");
	ccpath = smprint("/bin/%s", ot->cc);
	append(&cpp, smprint("-I/%s/include/ape", ot->name));
	append(&cpp, "-I/sys/include/ape");
	cppn = cpp.n;
	ccn = cc.n;
	for(i = 0; i < srcs.n; i++) {
		append(&cpp, srcs.strings[i]);
		if(Eflag)
			doexec("/bin/cpp", &cpp);
		else {
			if(Sflag)
				append(&cc, "-S");
			else {
				append(&cc, "-o");
				if (haveoname && cflag)
					append(&cc, oname);
				else
					append(&cc, changeext(srcs.strings[i], "o"));
			}
			dopipe("/bin/cpp", &cpp, ccpath, &cc);
		}
		cpp.n = cppn;
		cc.n = ccn;
	}
	if(!cflag) {
		append(&ld, "-o");
		append(&ld, oname);
		for(i = 0; i < ldargs.n; i++)
			append(&ld, ldargs.strings[i]);
		for(i = 0; i < objs.n; i++)
			append(&ld, objs.strings[i]);
		append(&ld, smprint("/%s/lib/ape/libap.a", ot->name));
		doexec(smprint("/bin/%s", ot->ld), &ld);
		if(objs.n == 1)
			remove(objs.strings[0]);
	}

	exits(0);
}

char *
searchlib(char *s, char *objtype)
{
	char *l;
	int i;

	if(!s)
		return 0;
	for(i = srchlibs.n-1; i>=0; i--) {
		l = smprint("%s/lib%s.a", srchlibs.strings[i], s);
		if(access(l, 0) >= 0)
			return l;
	}
	if(s[1] == 0)
		switch(s[0]) {
		case 'c':
			l = smprint("/%s/lib/ape/libap.a", objtype);
			break;
		case 'm':
			l = smprint("/%s/lib/ape/libap.a", objtype);
			break;
		case 'l':
			l = smprint("/%s/lib/ape/libl.a", objtype);
			break;
		case 'y':
			l = smprint("/%s/lib/ape/liby.a", objtype);
			break;
		default:
			l = 0;
		}
	else
		l = 0;
	return l;
}

void
append(List *l, char *s)
{
	if(l->n >= Maxlist-1)
		fatal("too many arguments");
	l->strings[l->n++] = s;
	l->strings[l->n] = 0;
}

void
doexec(char *c, List *a)
{
	Waitmsg *w;

	if(vflag) {
		printlist(a);
		fprint(2, "\n");
	}
	switch(fork()) {
	case -1:
		fatal("fork failed");
	case 0:
		exec(c, a->strings);
		fatal("exec failed");
	}
	if((w = wait()) == nil)
		fatal("wait failed");
	if(w->msg[0])
		fatal(smprint("%s: %s", a->strings[0], w->msg));
	free(w);
}

void
dopipe(char *c1, List *a1, char *c2, List *a2)
{
	Waitmsg *w;
	int pid1, got;
	int fd[2];

	if(vflag) {
		printlist(a1);
		fprint(2, " | ");
		printlist(a2);
		fprint(2, "\n");
	}
	if(pipe(fd) < 0)
		fatal("pipe failed");
	switch((pid1 = fork())) {
	case -1:
		fatal("fork failed");
	case 0:
		dup(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		exec(c2, a2->strings);
		fatal("exec failed");
	}
	switch(fork()) {
	case -1:
		fatal("fork failed");
	case 0:
		close(0);
		dup(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		exec(c1, a1->strings);
		fatal("exec failed");
	}
	close(fd[0]);
	close(fd[1]);
	for(got = 0; got < 2; got++) {
		if((w = wait()) == nil)
			fatal("wait failed");
		if(w->msg[0])
			fatal(smprint("%s: %s", (w->pid == pid1) ? a1->strings[0] : a2->strings[0], w->msg));
		free(w);
	}
}

Objtype *
findoty(void)
{
	char *o;
	Objtype *oty;

	o = getenv("objtype");
	if(!o)
		fatal("no $objtype in environment");
	for(oty = objtype; oty < &objtype[Nobjs]; oty++)
		if(strcmp(o, oty->name) == 0)
			return oty;
	fatal("unknown $objtype");
	return 0;			/* shut compiler up */
}

void
fatal(char *msg)
{
	fprint(2, "cc: %s\n", msg);
	exits(msg);
}

/* src ends in .something; return copy of basename with .ext added */
char *
changeext(char *src, char *ext)
{
	char *b, *e, *ans;

	b = utfrrune(src, '/');
	if(b)
		b++;
	else
		b = src;
	e = utfrrune(src, '.');
	if(!e)
		return 0;
	*e = 0;
	ans = smprint("%s.%s", b, ext);
	*e = '.';
	return ans;
}

void
printlist(List *l)
{
	int i;

	for(i = 0; i < l->n; i++) {
		fprint(2, "%s", l->strings[i]);
		if(i < l->n - 1)
			fprint(2, " ");
	}
}
