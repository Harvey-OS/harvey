#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "acid.h"
#include "y.tab.h"

static Biobuf	bioin;
static Biobuf	bioout;
static char	prog[128];
static char	*lm[16];
static int	nlm;

extern	int Sconv(void*, Fconv*);
	int isnumeric(char*);

void
main(int argc, char *argv[])
{
	Lsym *procs;
	int twice, pid, ipid, i;

	pid = 0;
	argv0 = argv[0];
	aout = "v.out";

	ARGBEGIN{
	case 'w':
		wtflag = 1;
		break;
	case 'l':
		lm[nlm++] = ARGF();
		break;
	default:
		fprint(2, "usage: acid [-l module] [-w] [pid] [file]\n");
		break;
	}ARGEND

	if(argc > 0) {
		if(isnumeric(argv[0])) {
			pid = atoi(argv[0]);
			sprint(prog, "/proc/%d/text", pid);
			aout = prog;
		}
		else
			aout = argv[0];
	}

	fmtinstall('S', Sconv);
	Binit(&bioout, 1, OWRITE);
	bout = &bioout;

	kinit();
	readtext(aout);
	varreg();
	installbuiltin();
	loadmodule("/lib/acid/port");
	machinit();
	for(i = 0; i < nlm; i++)
		if(loadmodule(lm[i]) == 0)
			print("%s: %r - not loaded\n", lm[i]);

	varsym();

	Binit(&bioin, 0, OREAD);
	bin = &bioin;
	interactive = 1;
	line = 1;

	procs = mkvar("proclist");
	notify(catcher);

	twice = 0;
	ipid = pid;
	for(;;) {
		if(setjmp(err))
			unwind();

		stacked = 0;
		if(pid != 0) {
			pid = 0;
			sproc(ipid);
		}

		Bprint(bout, "acid: ");

		if(yyparse() != 1) {
			if(procs->v->l == 0 || twice)
				break;
			print("\nActive processes\n");
			twice = 1;
		}

		unwind();
	}
}

void
machinit(void)
{
	Lsym *l;
	Node *n;
	Value *v;
	char buf[128], *p;

	l = mkvar("objtype");
	v = l->v;
	v->fmt = 's';
	v->set = 1;
	v->string = strnode(mach->name);
	v->type = TSTRING;

	sprint(buf, "/lib/acid/%s", mach->name);
	loadmodule(buf);
	p = getenv("home");
	if(p != 0) {
		sprint(buf, "%s/lib/acid", p);
		loadmodule(buf);
	}

	l = look("acidinit");
	if(l && l->proc) {
		n = an(ONAME, ZN, ZN);
		n->sym = l;
		n = an(OCALL, n, ZN);
		execute(n);
	}
}

int
loadmodule(char *s)
{
	bin = Bopen(s, OREAD);
	if(bin == 0)
		return 0;

	if(setjmp(err)) {
		unwind();
		return 1;
	}
	filename = s;
	line = 0;
	yyparse();
	filename = 0;
	Bclose(bin);
	return 1;
}

void
readtext(char *s)
{
	Lsym *l;
	Symbol sym;
	extern Machdata mipsmach, m68020mach, sparcmach, i386mach, hobbitmach;
	
	if(text != 0) {
		close(text);
		text = 0;
	}
	text = open(s, OREAD);
	if(text < 0) {
		print("open %s: %r\n", s);
		return;
	}
	if(!crackhdr(text, &fhdr)) {
		print("decode header: %s\n", symerror);
		return;
	}

	symmap = loadmap(0, text, &fhdr);
	if(symmap == 0)
		print("loadmap: cannot make symbol map");

	if(syminit(text, &fhdr) < 0) {
		if (symerror)
			print("syminit: %s\n", symerror);
		else
			print("syminit: cannot load symbols\n");
		return;
	}
	print("%s: %s\n", s, fhdr.name);

	/* Dummy lookup to get local byte order */
	lookup(0, "main", &sym);

	if(mach->sbreg && lookup(0, mach->sbreg, &sym)) {
		mach->sb = sym.value;
		l = enter("SB", Tid);
		l->v->fmt = 'X';
		l->v->ival = mach->sb;
		l->v->type = TINT;
		l->v->set = 1;
	}

	switch(fhdr.type) {
	default:
		machdata = &mipsmach;
		break;
	case F68020:			/* 2.out */
	case FNEXTB:			/* Next bootable */
	case F68020B:			/* 68020 bootable */
		machdata = &m68020mach;
		break;
	case FSPARC:			/* k.out */
	case FSPARCB:			/* Sparc bootable */
		machdata = &sparcmach;
		break;
	case FI386:			/* 8.out */
	case FI386B:			/* I386 bootable */
		machdata = &i386mach;
		break;
	case FHOBBIT:			/* z.out */
	case FHOBBITB:			/* Hobbit bootable */
		machdata = &hobbitmach;
		break;
	}
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
con(int v)
{
	Node *n;

	n = an(OCONST, ZN, ZN);
	n->ival = v;
	n->fmt = 'X';
	n->type = TINT;
	return n;
}

void
fatal(char *fmt, ...)
{
	char buf[128];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s: %d (fatal problem) %s\n", argv0, line, buf);
	exits(buf);
}

void
yyerror(char *a, ...)
{
	char buf[128];

	Bflush(bin);

	if(strcmp(a, "syntax error") == 0) {
		yyerror("syntax error, near symbol '%s'", symbol);
		return;
	}
	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);
	if(filename)
		print("%d (%s) %s\n", line, filename, buf);
	else
		print("%d %s\n", line, buf);
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
		}
		l = l->next;
	}
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
	}
}

void
gc(void)
{
	int i;
	Lsym *f;
	Value *v;
	Gc *m, **p, *next;

	if(dogc < 128*1024)
		return;

	/* Mark */
	for(m = gcl; m; m = m->gclink)
		m->gcmark = 0;

	/* Scan */
	for(i = 0; i < Hashsize; i++) {
		for(f = hash[i]; f; f = f->hash) {
			if(f->lexval == Tid) {
				v = f->v;
				if(v->pop)
					fatal("gc");
				switch(v->type) {
				case TSTRING:
					v->string->gcmark = 1;
					break;
				case TLIST:
					marklist(v->l);
					break;
				}
			}
			marktree(f->proc);
		}
	}

	/* Free */
	p = &gcl;
	for(m = gcl; m; m = next) {
		next = m->gclink;
		if(!m->gcmark) {
			*p = m->gclink;
			free(m);	/* Sleazy reliance on my malloc */
		}
		else
			p = &m->gclink;
	}
	dogc = 0;
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
	Dir d1, d2;
	char buf[128];

	if(dirfstat(f1, &d1) < 0)
		fatal("checkqid: dirfstat: %r");

	sprint(buf, "/proc/%d/text", pid);
	fd = open(buf, OREAD);
	if(fd < 0 || dirfstat(fd, &d2) < 0)
		fatal("checkqid: dirstat %s: %r", buf);

	close(fd);
	if(memcmp(&d1.qid, &d2.qid, sizeof(d2.qid)))
		print("warning: image does not match text\n");
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
