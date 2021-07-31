#include <u.h>
#include <libc.h>


typedef struct Objtype {
	char	*name;
	char	*cc;
	char	*ld;
	char	*o;
	char	*oname;
} Objtype;

Objtype objtype[] = {
	{"arm",	"5c", "5l", "5", "5.out"},
	{"mips2",	"4c", "4l", "4", "4.out"},
	{"mips",	"vc", "vl", "v", "v.out"},
	{"68000",	"1c", "1l", "1", "1.out"},
	{"68020",	"2c", "2l", "2", "2.out"},
	{"sparc",	"kc", "kl", "k", "k.out"},
	{"386",		"8c", "8l", "8", "8.out"},
	{"alpha",	"7c", "7l", "7", "7.out"},
	{"power",	"qc", "ql", "q", "q.out"},
};

enum {
	Nobjs = (sizeof objtype)/(sizeof objtype[0]),
	Maxlist = 500,
};

typedef struct List {
	char	*strings[Maxlist];
	int	n;
} List;

List	srcs, objs, cpp, cc, ld, ldargs;
int	cflag, vflag, Eflag, Pflag;
char	*allos = "2478kqv";

void	append(List *, char *);
char	*changeext(char *, char *);
void	doexec(char *, List *);
void	dopipe(char *, List *, char *, List *);
void	fatal(char *);
Objtype	*findoty(void);
void	printlist(List *);
char	*str(char *, ...);

void
main(int argc, char *argv[])
{
	Objtype *ot;
	char *s, *suf, *ccpath;
	char *oname;
	int i, cppn, ccn, oflag;

	oflag = 0;
	ot = findoty();
	oname = ot->oname;
	append(&cpp, "cpp");
	append(&cpp, "-D__STDC__=1");	/* ANSI says so */
	append(&cpp, "-N");		/* turn off standard includes */
	append(&cc, ot->cc);
	append(&ld, ot->ld);
	while(argc > 0) {
		ARGBEGIN {
		case '+':
			append(&cpp, str("-%c", ARGC()));
			break;
		case 'c':
			cflag = 1;
			break;
		case 'l':
			append(&objs, str("/%s/lib/ape/lib%s.a", ot->name, ARGF()));
			break;
		case 'o':
			oflag = 1;
			oname = ARGF();
			if(!oname)
				fatal("no -o argument");
			break;
		case 'w':
		case 'B':
		case 'N':
		case 'S':
		case 'V':
			append(&cc, str("-%c", ARGC()));
			break;
		case 's':
			append(&cc, str("-s%s", ARGF()));
			break;
		case 'D':
		case 'I':
		case 'U':
			append(&cpp, str("-%c%s", ARGC(), ARGF()));
			break;
		case 'v':
			vflag = 1;
			append(&ldargs, "-v");
			break;
		case 'P':
			Pflag = 1;
			cflag = 1;
			break;
		case 'E':
			Eflag = 1;
			cflag = 1;
			break;
		case 'p':
			append(&ldargs, "-p");
			break;
		case 'a':
			/* hacky look inside ARGBEGIN insides, to see if we have -aa */
			if(*_args == 'a') {
				append(&cc, "-aa");
				_args++;
			} else
				append(&cc, "-a");
			cflag = 1;
			break;
		case 'F':
			append(&cc, "-F");
			break;
		default:
			fprint(2, "pcc: flag -%c ignored\n", ARGC());
			break;
		} ARGEND
		if(argc > 0) {
			s = argv[0];
			suf = utfrrune(s, '.');
			if(suf) {
				suf++;
				if(strcmp(suf, "c") == 0) {
					append(&srcs, s);
					append(&objs, changeext(s, ot->o));
				} else if(strcmp(suf, ot->o) == 0 ||
					  strcmp(suf, "a") == 0 ||
					  (suf[0] == 'a' && strcmp(suf+1, ot->o) == 0)) {
					append(&objs, s);
				} else if(utfrune(allos, suf[0]) != 0) {
					fprint(2, "pcc: argument %s ignored: wrong architecture\n",
						s);
				}
			}
		}
	}
	if(objs.n == 0)
		fatal("no files to compile or load");
	ccpath = str("/bin/%s", ot->cc);
	append(&cpp, str("-I/%s/include/ape", ot->name));
	append(&cpp, "-I/sys/include/ape");
	cppn = cpp.n;
	ccn = cc.n;
	for(i = 0; i < srcs.n; i++) {
		append(&cpp, srcs.strings[i]);
		if(Pflag)
			append(&cpp, changeext(objs.strings[i], ".i"));
		if(Eflag || Pflag)
			doexec("/bin/cpp", &cpp);
		else {
			append(&cc, "-o");
			if(oflag && cflag)
				append(&cc, oname);
			else
				append(&cc, changeext(srcs.strings[i], ot->o));
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
		doexec(str("/bin/%s", ot->ld), &ld);
		if(objs.n == 1){
			/* prevent removal of a library */
			if(strstr(objs.strings[0], ".a") == 0)
				remove(objs.strings[0]);
		}
	}

	exits(0);
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
	Waitmsg w;
	int pid;

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
	pid = wait(&w);
	if(pid < 0)
		fatal("wait failed");
	if(w.msg[0])
		fatal(str("%s: %s", a->strings[0], w.msg));
}

void
dopipe(char *c1, List *a1, char *c2, List *a2)
{
	Waitmsg w;
	int pid, pid1, got;
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
		pid = wait(&w);
		if(pid < 0)
			fatal("wait failed");
		if(w.msg[0])
			fatal(str("%s: %s",
			   (pid == pid1) ? a1->strings[0] : a2->strings[0], w.msg));
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
	fprint(2, "pcc: %s\n", msg);
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
	ans = str("%s.%s", b, ext);
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

char *
str(char *fmt, ...)
{
	char *s;
	char buf[1000];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, &buf[sizeof buf], fmt, arg);
	va_end(arg);
	s = malloc(strlen(buf)+1);
	strcpy(s, buf);
	return s;
}
