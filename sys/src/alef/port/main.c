#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#define Extern
#include "globl.h"

int	ac;
char	*av[256];

void
usage(void)
{
	fprint(2, "usage: %s [-SNwc] [-Idir] [-Dname[=def]] [-o file] [-D#] files ...\n", argv0);
	exits("usage");
}

void
rexit(void)
{
	if(nerr)
		exits("errors");

	exits(0);
}

void
main(int argc, char *argv[])
{
	Waitmsg w;
	char *p, *t, o, *of;
	int pid, launched, nproc;

	argv0 = argv[0];
	if(argc == 1)
		usage();

	flag['N'] = 1;		/* optimizer on by default */

	av[ac++] = CPP;
	
	of = 0;
	ARGBEGIN{
	case 'I':
	case 'D':
		av[ac++] = argv[0];
		while(*_args && (_args += chartorune(&_argc, _args)))
			;
		break;

	case 'S':
		asm++;
		break;

	case 'o':
		of = ARGF();
		if(of == 0)
			usage();
		break;

	case 'w':
		Owarn++;
		break;

	case 'N':
		flag['N'] = 0;
		break;

	case 'c':
		chks++;
		break;

	case 'd':
		t = ARGF();
		while(*t) {
			o = *t;
			if(o < 'a' && o > 'z' && o < 'A' && o > 'Z')
				usage();
			flag[o]++;
			t++;
		}
		break;
	default:
		usage();
	}ARGEND

	av[ac++] = IALEF;

	fmtinstall('T', typeconv);
	fmtinstall('N', nodeconv);
	fmtinstall('P', protoconv);
	fmtinstall('|', VBconv);

	kinit();
	typeinit();
	outinit();
	reginit();

	if(argc == 1) {
		compile(argv[0], of);
		rexit();
	}
	if(of) {
		Owarn++;
		warn(0, "-o ignored");
		Owarn--;
	}
	flag['f']++;

	nproc = 4;
	p = getenv("NPROC");
	if(p)
		nproc = atoi(p);

	launched = 0;
	while(argc) {
		if(launched < nproc) {
			switch(fork()) {
			case -1:
				fatal("fork failed: %r");
			case 0:
				compile(argv[0], 0);
				break;
			default:
				launched++;
				argc--;
				argv++;
			}
		}
		if(launched == nproc) {
			launched--;
			pid = wait(&w);
			if(pid < 0)
				fatal("wait: %r");
			if(w.msg[0])
				nerr++;
		}
	}
	while(launched) {
		wait(&w);
		if(w.msg[0])
			nerr++;
		launched--;
	}
	rexit();
}

void
umap(char *s)
{
	s = strchr(s, '_');
	if(s)
		*s = '.';
}

void
fatal(char *fmt, ...)
{
	char buf[512];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s : %L (fatal compiler problem) %s\n", argv0, line, buf);
	if(opt('A'))
		abort();
	exits(buf);
}

void
diag(Node *n, char *fmt, ...)
{
	char buf[512];
	int srcline;

	srcline = line;
	if(n)
		srcline = n->srcline;
	
	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	umap(buf);
	fprint(2, "%L %s\n", srcline, buf);
	if(nerr++ > 10) {
		fprint(2, "%L too many errors, giving up\n", line);
		exits("errors");
	}
}

void
warn(Node *n, char *fmt, ...)
{
	char buf[512];
	int srcline;

	if(Owarn == 0)
		return;

	srcline = line;
	if(n)
		srcline = n->srcline;
	
	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	umap(buf);
	fprint(2, "%L warning: %s\n", srcline, buf);
}

void
yyerror(char *a, ...)
{
	char buf[128];

	if(strcmp(a, "syntax error") == 0) {
		yyerror("syntax error, near symbol '%s'", symbol);
		return;
	}
	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);
	print("%L %s\n", line, buf);
	if(nerr++ > 10) {
		fprint(2, "%L too many errors, giving up\n", line);
		rexit();
	}
}

void
compile(char *s, char *of)
{
	int fd[2];
	long ti[4];
	char *p, buf[128], asmfile[128], ofile[128];

	if(access(s, OREAD) < 0) {
		nerr++;
		fprint(2, "%s: no source file %s\n", argv0, s);
		exits("errors");
	}

	if(opt('f'))
		fprint(2, "%s:\n", s);

	if(pipe(fd) < 0)
		fatal("pipe failed: %r");

	file = strdup(s);
	strcpy(buf, s);
	p = strrchr(buf, '.');
	if(p)
		*p = '\0';
	p = strrchr(buf, '/');
	if(p)
		memmove(buf, p+1, strlen(p+1)+1);

	if(asm) {
		strcpy(asmfile, buf);
		strcat(asmfile, ".s");
		remove(asmfile);
	}
	else if(of)
		strcpy(ofile, of);
	else {
		strcpy(ofile, buf);
		strcat(ofile, AOUTL);
		remove(ofile);
	}

	av[ac++] = s;
	av[ac] = 0;

	switch(fork()) {
	default:
		close(fd[0]);
		bin = malloc(sizeof(Biobuf));
		Binit(bin, fd[1], OREAD);
		break;

	case -1:
		fatal("fork cpp failed: %r");

	case 0:
		close(0);
		dup(fd[0], 1);
		close(fd[0]);
		close(fd[1]);
		exec(CPP, av);
		fatal("exec %s: %r", CPP);
	}

	file = s;
	line = 1;
	linehist(s, 0);		/* Push start */
	yyparse();
	Bclose(bin);

	if(nerr == 0) {
		if(asm)
			sfile(asmfile);
		else
			objfile(ofile);
	}

	if(opt('T')) {
		print("%8d bytes\n", stats.mem);
		print("%8d nodes\n", stats.nodes);
		print("%8d types\n", stats.types);
		times(ti);
		print("%gu %gs\n", ((float)ti[0])/1000.0, ((float)ti[1])/1000.0);
	}
	rexit();
}

void
free(void *a)			/* Prevent redifinition of malloc */
{
	USED(a);
}

void *
malloc(long a)
{
	static char *arena;
	static long mem;
	char *p;

	a = a+sizeof(long);
	a &= ~(sizeof(long)-1);

	stats.mem += a;

	for(;;) {
		if(a < mem) {
			mem -= a;
			p = arena;
			arena += a;
			return p;
		}
		arena = sbrk(Chunk);
		if(arena == (char*)-1)
			fatal("no memory");
		mem = Chunk;
	}
	return 0;
}

int
VBconv(void *o, Fconv *f)
{
	char str[128];
	int i, n, t, pc;
	extern printcol;

	n = *(int*)o;
	pc = printcol;
	i = 0;
	while(pc < n) {
		t = (pc+8) & ~7;
		if(t <= n) {
			str[i++] = '\t';
			pc = t;
			continue;
		}
		str[i++] = ' ';
		pc++;
	}
	str[i] = 0;
	strconv(str, f);
	return sizeof(n);
}
