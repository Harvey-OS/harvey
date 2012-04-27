#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "acid.h"
#include "y.tab.h"

extern int _ifmt(Fmt*);

static Biobuf	bioout;
static char	prog[128];
static char*	lm[16];
static int	nlm;
static char*	mtype;

static	int attachfiles(char*, int);
int	xfmt(Fmt*);
int	isnumeric(char*);
void	die(void);
void	loadmoduleobjtype(void);

void
usage(void)
{
	fprint(2, "usage: acid [-kqw] [-l library] [-m machine] [pid] [file]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Lsym *l;
	Node *n;
	char *s;
	int pid, i;

	argv0 = argv[0];
	pid = 0;
	aout = "8.out";
	quiet = 1;

	mtype = 0;
	ARGBEGIN{
	case 'm':
		mtype = ARGF();
		break;
	case 'w':
		wtflag = 1;
		break;
	case 'l':
		s = ARGF();
		if(s == 0)
			usage();
		lm[nlm++] = s;
		break;
	case 'k':
		kernel++;
		break;
	case 'q':
		quiet = 0;
		break;
	case 'r':
		pid = 1;
		remote++;
		kernel++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0) {
		if(remote)
			aout = argv[0];
		else
		if(isnumeric(argv[0])) {
			pid = strtol(argv[0], 0, 0);
			snprint(prog, sizeof(prog), "/proc/%d/text", pid);
			aout = prog;
			if(argc > 1)
				aout = argv[1];
			else if(kernel)
				aout = system();
		}
		else {
			if(kernel) {
				fprint(2, "acid: -k requires a pid\n");
				usage();
			}
			aout = argv[0];
		}
	} else
	if(remote)
		aout = "/mips/9ch";

	fmtinstall('x', xfmt);
	fmtinstall('L', Lfmt);
	Binit(&bioout, 1, OWRITE);
	bout = &bioout;

	kinit();
	initialising = 1;
	pushfile(0);
	loadvars();
	installbuiltin();

	if(mtype && machbyname(mtype) == 0)
		print("unknown machine %s", mtype);

	if (attachfiles(aout, pid) < 0)
		varreg();		/* use default register set on error */

	loadmodule("/sys/lib/acid/port");
	loadmoduleobjtype();

	for(i = 0; i < nlm; i++) {
		if(access(lm[i], AREAD) >= 0)
			loadmodule(lm[i]);
		else {
			s = smprint("/sys/lib/acid/%s", lm[i]);
			loadmodule(s);
			free(s);
		}
	}

	userinit();
	varsym();

	l = look("acidmap");
	if(l && l->proc) {
		n = an(ONAME, ZN, ZN);
		n->sym = l;
		n = an(OCALL, n, ZN);
		execute(n);
	}

	interactive = 1;
	initialising = 0;
	line = 1;

	notify(catcher);

	for(;;) {
		if(setjmp(err)) {
			Binit(&bioout, 1, OWRITE);
			unwind();
		}
		stacked = 0;

		Bprint(bout, "acid: ");

		if(yyparse() != 1)
			die();
		restartio();

		unwind();
	}
	/* not reached */
}

static int
attachfiles(char *aout, int pid)
{
	interactive = 0;
	if(setjmp(err))
		return -1;

	if(aout) {				/* executable given */
		if(wtflag)
			text = open(aout, ORDWR);
		else
			text = open(aout, OREAD);

		if(text < 0)
			error("%s: can't open %s: %r\n", argv0, aout);
		readtext(aout);
	}
	if(pid)					/* pid given */
		sproc(pid);
	return 0;
}

void
die(void)
{
	Lsym *s;
	List *f;

	Bprint(bout, "\n");

	s = look("proclist");
	if(s && s->v->type == TLIST) {
		for(f = s->v->l; f; f = f->next)
			Bprint(bout, "echo kill > /proc/%d/ctl\n", (int)f->ival);
	}
	exits(0);
}

void
loadmoduleobjtype(void)
{
	char *buf;

	buf = smprint("/sys/lib/acid/%s", mach->name);
	loadmodule(buf);
	free(buf);
}

void
userinit(void)
{
	Lsym *l;
	Node *n;
	char *buf, *p;

	p = getenv("home");
	if(p != 0) {
		buf = smprint("%s/lib/acid", p);
		silent = 1;
		loadmodule(buf);
		free(buf);
	}

	interactive = 0;
	if(setjmp(err)) {
		unwind();
		return;
	}
	l = look("acidinit");
	if(l && l->proc) {
		n = an(ONAME, ZN, ZN);
		n->sym = l;
		n = an(OCALL, n, ZN);
		execute(n);
	}
}

void
loadmodule(char *s)
{
	interactive = 0;
	if(setjmp(err)) {
		unwind();
		return;
	}
	pushfile(s);
	silent = 0;
	yyparse();
	popio();
	return;
}

void
readtext(char *s)
{
	Dir *d;
	Lsym *l;
	Value *v;
	uvlong length;
	Symbol sym;
	extern Machdata mipsmach;

	if(mtype != 0){
		symmap = newmap(0, 1);
		if(symmap == 0)
			print("%s: (error) loadmap: cannot make symbol map\n", argv0);
		length = 1<<24;
		d = dirfstat(text);
		if(d != nil){
			length = d->length;
			free(d);
		}
		setmap(symmap, text, 0, length, 0, "binary");
		return;
	}

	machdata = &mipsmach;

	if(!crackhdr(text, &fhdr)) {
		print("can't decode file header\n");
		return;
	}

	symmap = loadmap(0, text, &fhdr);
	if(symmap == 0)
		print("%s: (error) loadmap: cannot make symbol map\n", argv0);

	if(syminit(text, &fhdr) < 0) {
		print("%s: (error) syminit: %r\n", argv0);
		return;
	}
	print("%s:%s\n", s, fhdr.name);

	if(mach->sbreg && lookup(0, mach->sbreg, &sym)) {
		mach->sb = sym.value;
		l = enter("SB", Tid);
		l->v->fmt = 'X';
		l->v->ival = mach->sb;
		l->v->type = TINT;
		l->v->set = 1;
	}

	l = mkvar("objtype");
	v = l->v;
	v->fmt = 's';
	v->set = 1;
	v->string = strnode(mach->name);
	v->type = TSTRING;

	l = mkvar("textfile");
	v = l->v;
	v->fmt = 's';
	v->set = 1;
	v->string = strnode(s);
	v->type = TSTRING;

	machbytype(fhdr.type);
	varreg();
}

Node*
an(int op, Node *l, Node *r)
{
	Node *n;

	n = gmalloc(sizeof(Node));
	memset(n, 0, sizeof(Node));
	n->gclink = gcl;
	gcl = n;
	n->op = op;
	n->left = l;
	n->right = r;
	return n;
}

List*
al(int t)
{
	List *l;

	l = gmalloc(sizeof(List));
	memset(l, 0, sizeof(List));
	l->type = t;
	l->gclink = gcl;
	gcl = l;
	return l;
}

Node*
con(vlong v)
{
	Node *n;

	n = an(OCONST, ZN, ZN);
	n->ival = v;
	n->fmt = 'W';
	n->type = TINT;
	return n;
}

void
fatal(char *fmt, ...)
{
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %L (fatal problem) %s\n", argv0, buf);
	exits(buf);
}

void
yyerror(char *fmt, ...)
{
	char buf[128];
	va_list arg;

	if(strcmp(fmt, "syntax error") == 0) {
		yyerror("syntax error, near symbol '%s'", symbol);
		return;
	}
	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	print("%L: %s\n", buf);
}

void
marktree(Node *n)
{

	if(n == 0)
		return;

	marktree(n->left);
	marktree(n->right);

	n->gcmark = 1;
	if(n->op != OCONST)
		return;

	switch(n->type) {
	case TSTRING:
		n->string->gcmark = 1;
		break;
	case TLIST:
		marklist(n->l);
		break;
	case TCODE:
		marktree(n->cc);
		break;
	}
}

void
marklist(List *l)
{
	while(l) {
		l->gcmark = 1;
		switch(l->type) {
		case TSTRING:
			l->string->gcmark = 1;
			break;
		case TLIST:
			marklist(l->l);
			break;
		case TCODE:
			marktree(l->cc);
			break;
		}
		l = l->next;
	}
}

void
gc(void)
{
	int i;
	Lsym *f;
	Value *v;
	Gc *m, **p, *next;

	if(dogc < Mempergc)
		return;
	dogc = 0;

	/* Mark */
	for(m = gcl; m; m = m->gclink)
		m->gcmark = 0;

	/* Scan */
	for(i = 0; i < Hashsize; i++) {
		for(f = hash[i]; f; f = f->hash) {
			marktree(f->proc);
			if(f->lexval != Tid)
				continue;
			for(v = f->v; v; v = v->pop) {
				switch(v->type) {
				case TSTRING:
					v->string->gcmark = 1;
					break;
				case TLIST:
					marklist(v->l);
					break;
				case TCODE:
					marktree(v->cc);
					break;
				}
			}
		}
	}

	/* Free */
	p = &gcl;
	for(m = gcl; m; m = next) {
		next = m->gclink;
		if(m->gcmark == 0) {
			*p = next;
			free(m);	/* Sleazy reliance on my malloc */
		}
		else
			p = &m->gclink;
	}
}

void*
gmalloc(long l)
{
	void *p;

	dogc += l;
	p = malloc(l);
	if(p == 0)
		fatal("out of memory");
	return p;
}

void
checkqid(int f1, int pid)
{
	int fd;
	Dir *d1, *d2;
	char buf[128];

	if(kernel)
		return;

	d1 = dirfstat(f1);
	if(d1 == nil){
		print("checkqid: (qid not checked) dirfstat: %r\n");
		return;
	}

	snprint(buf, sizeof(buf), "/proc/%d/text", pid);
	fd = open(buf, OREAD);
	if(fd < 0 || (d2 = dirfstat(fd)) == nil){
		print("checkqid: (qid not checked) dirstat %s: %r\n", buf);
		free(d1);
		if(fd >= 0)
			close(fd);
		return;
	}

	close(fd);

	if(d1->qid.path != d2->qid.path || d1->qid.vers != d2->qid.vers || d1->qid.type != d2->qid.type){
		print("path %llux %llux vers %lud %lud type %d %d\n",
			d1->qid.path, d2->qid.path, d1->qid.vers, d2->qid.vers, d1->qid.type, d2->qid.type);
		print("warning: image does not match text for pid %d\n", pid);
	}
	free(d1);
	free(d2);
}

void
catcher(void *junk, char *s)
{
	USED(junk);

	if(strstr(s, "interrupt")) {
		gotint = 1;
		noted(NCONT);
	}
	noted(NDFLT);
}

char*
system(void)
{
	char *cpu, *p, *q;
	static char *kernel;

	cpu = getenv("cputype");
	if(cpu == 0) {
		cpu = "mips";
		print("$cputype not set; assuming %s\n", cpu);
	}
	p = getenv("terminal");
	if(p == 0 || (p=strchr(p, ' ')) == 0 || p[1] == ' ' || p[1] == 0) {
		p = "ch";
		print("missing or bad $terminal; assuming %s\n", p);
	}
	else{
		p++;
		q = strchr(p, ' ');
		if(q)
			*q = 0;
	}

	if(kernel != nil)
		free(kernel);
	kernel = smprint("/%s/9%s", cpu, p);

	return kernel;
}

int
isnumeric(char *s)
{
	while(*s) {
		if(*s < '0' || *s > '9')
			return 0;
		s++;
	}
	return 1;
}

int
xfmt(Fmt *f)
{
	f->flags ^= FmtSharp;
	return _ifmt(f);
}
