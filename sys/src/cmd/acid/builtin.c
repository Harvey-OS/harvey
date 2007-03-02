#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#include <regexp.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

void	cvtatof(Node*, Node*);
void	cvtatoi(Node*, Node*);
void	cvtitoa(Node*, Node*);
void	bprint(Node*, Node*);
void	funcbound(Node*, Node*);
void	printto(Node*, Node*);
void	getfile(Node*, Node*);
void	fmt(Node*, Node*);
void	pcfile(Node*, Node*);
void	pcline(Node*, Node*);
void	setproc(Node*, Node*);
void	strace(Node*, Node*);
void	follow(Node*, Node*);
void	reason(Node*, Node*);
void	newproc(Node*, Node*);
void	startstop(Node*, Node*);
void	match(Node*, Node*);
void	status(Node*, Node*);
void	kill(Node*,Node*);
void	waitstop(Node*, Node*);
void	stop(Node*, Node*);
void	start(Node*, Node*);
void	filepc(Node*, Node*);
void	doerror(Node*, Node*);
void	rc(Node*, Node*);
void	doaccess(Node*, Node*);
void	map(Node*, Node*);
void	readfile(Node*, Node*);
void	interpret(Node*, Node*);
void	include(Node*, Node*);
void	regexp(Node*, Node*);
void	dosysr1(Node*, Node*);
void	fmtof(Node*, Node*) ;
void	dofmtsize(Node*, Node*) ;

typedef struct Btab Btab;
struct Btab
{
	char	*name;
	void	(*fn)(Node*, Node*);
} tab[] =
{
	"atof",		cvtatof,
	"atoi",		cvtatoi,
	"error",	doerror,
	"file",		getfile,
	"readfile",	readfile,
	"access",	doaccess,
	"filepc",	filepc,
	"fnbound",	funcbound,
	"fmt",		fmt,
	"follow",	follow,
	"itoa",		cvtitoa,
	"kill",		kill,
	"match",	match,
	"newproc",	newproc,
	"pcfile",	pcfile,
	"pcline",	pcline,
	"print",	bprint,
	"printto",	printto,
	"rc",		rc,
	"reason",	reason,
	"setproc",	setproc,
	"start",	start,
	"startstop",	startstop,
	"status",	status,
	"stop",		stop,
	"strace",	strace,
	"sysr1",	dosysr1,
	"waitstop",	waitstop,
	"map",		map,
	"interpret",	interpret,
	"include",	include,
	"regexp",	regexp,
	"fmtof",	fmtof,
	"fmtsize",	dofmtsize,
	0
};

char vfmt[] = "aBbcCdDfFgGiIoOqQrRsSuUVWxXYZ38";

void
mkprint(Lsym *s)
{
	prnt = malloc(sizeof(Node));
	memset(prnt, 0, sizeof(Node));
	prnt->op = OCALL;
	prnt->left = malloc(sizeof(Node));
	memset(prnt->left, 0, sizeof(Node));
	prnt->left->sym = s;
}

void
installbuiltin(void)
{
	Btab *b;
	Lsym *s;

	b = tab;
	while(b->name) {
		s = look(b->name);
		if(s == 0)
			s = enter(b->name, Tid);

		s->builtin = b->fn;
		if(b->fn == bprint)
			mkprint(s);
		b++;
	}
}

void
dosysr1(Node *r, Node*)
{
	extern int sysr1(void);
	
	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
	r->ival = sysr1();
}

void
match(Node *r, Node *args)
{
	int i;
	List *f;
	Node *av[Maxarg];
	Node resi, resl;

	na = 0;
	flatten(av, args);
	if(na != 2)
		error("match(obj, list): arg count");

	expr(av[1], &resl);
	if(resl.type != TLIST)
		error("match(obj, list): need list");
	expr(av[0], &resi);

	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
	r->ival = -1;

	i = 0;
	for(f = resl.l; f; f = f->next) {
		if(resi.type == f->type) {
			switch(resi.type) {
			case TINT:
				if(resi.ival == f->ival) {
					r->ival = i;
					return;
				}
				break;
			case TFLOAT:
				if(resi.fval == f->fval) {
					r->ival = i;
					return;
				}
				break;
			case TSTRING:
				if(scmp(resi.string, f->string)) {
					r->ival = i;
					return;
				}
				break;
			case TLIST:
				error("match(obj, list): not defined for list");
			}
		}
		i++;
	}
}

void
newproc(Node *r, Node *args)
{
	int i;
	Node res;
	char *p, *e;
	char *argv[Maxarg], buf[Strsize];

	i = 1;
	argv[0] = aout;

	if(args) {
		expr(args, &res);
		if(res.type != TSTRING)
			error("newproc(): arg not string");
		if(res.string->len >= sizeof(buf))
			error("newproc(): too many arguments");
		memmove(buf, res.string->string, res.string->len);
		buf[res.string->len] = '\0';
		p = buf;
		e = buf+res.string->len;
		for(;;) {
			while(p < e && (*p == '\t' || *p == ' '))
				*p++ = '\0';
			if(p >= e)
				break;
			argv[i++] = p;
			if(i >= Maxarg)
				error("newproc: too many arguments");
			while(p < e && *p != '\t' && *p != ' ')
				p++;
		}
	}
	argv[i] = 0;
	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
	r->ival = nproc(argv);
}

void
startstop(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("startstop(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("startstop(pid): arg type");

	msg(res.ival, "startstop");
	notes(res.ival);
	dostop(res.ival);
}

void
waitstop(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("waitstop(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("waitstop(pid): arg type");

	Bflush(bout);
	msg(res.ival, "waitstop");
	notes(res.ival);
	dostop(res.ival);
}

void
start(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("start(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("start(pid): arg type");

	msg(res.ival, "start");
}

void
stop(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("stop(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("stop(pid): arg type");

	Bflush(bout);
	msg(res.ival, "stop");
	notes(res.ival);
	dostop(res.ival);
}

void
kill(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("kill(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("kill(pid): arg type");

	msg(res.ival, "kill");
	deinstall(res.ival);
}

void
status(Node *r, Node *args)
{
	Node res;
	char *p;

	USED(r);
	if(args == 0)
		error("status(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("status(pid): arg type");

	p = getstatus(res.ival);
	r->string = strnode(p);
	r->op = OCONST;
	r->fmt = 's';
	r->type = TSTRING;
}

void
reason(Node *r, Node *args)
{
	Node res;

	if(args == 0)
		error("reason(cause): no cause");
	expr(args, &res);
	if(res.type != TINT)
		error("reason(cause): arg type");

	r->op = OCONST;
	r->type = TSTRING;
	r->fmt = 's';
	r->string = strnode((*machdata->excep)(cormap, rget));
}

void
follow(Node *r, Node *args)
{
	int n, i;
	Node res;
	uvlong f[10];
	List **tail, *l;

	if(args == 0)
		error("follow(addr): no addr");
	expr(args, &res);
	if(res.type != TINT)
		error("follow(addr): arg type");

	n = (*machdata->foll)(cormap, res.ival, rget, f);
	if (n < 0)
		error("follow(addr): %r");
	tail = &r->l;
	for(i = 0; i < n; i++) {
		l = al(TINT);
		l->ival = f[i];
		l->fmt = 'X';
		*tail = l;
		tail = &l->next;
	}
}

void
funcbound(Node *r, Node *args)
{
	int n;
	Node res;
	uvlong bounds[2];
	List *l;

	if(args == 0)
		error("fnbound(addr): no addr");
	expr(args, &res);
	if(res.type != TINT)
		error("fnbound(addr): arg type");

	n = fnbound(res.ival, bounds);
	if (n != 0) {
		r->l = al(TINT);
		l = r->l;
		l->ival = bounds[0];
		l->fmt = 'X';
		l->next = al(TINT);
		l = l->next;
		l->ival = bounds[1];
		l->fmt = 'X';
	}
}

void
setproc(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("setproc(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("setproc(pid): arg type");

	sproc(res.ival);
}

void
filepc(Node *r, Node *args)
{
	Node res;
	char *p, c;

	if(args == 0)
		error("filepc(filename:line): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("filepc(filename:line): arg type");

	p = strchr(res.string->string, ':');
	if(p == 0)
		error("filepc(filename:line): bad arg format");

	c = *p;
	*p++ = '\0';
	r->ival = file2pc(res.string->string, strtol(p, 0, 0));
	p[-1] = c;
	if(r->ival == ~0)
		error("filepc(filename:line): can't find address");

	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'V';
}

void
interpret(Node *r, Node *args)
{
	Node res;
	int isave;

	if(args == 0)
		error("interpret(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("interpret(string): arg type");

	pushstr(&res);

	isave = interactive;
	interactive = 0;
	r->ival = yyparse();
	interactive = isave;
	popio();
	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
}

void
include(Node *r, Node *args)
{
	Node res;
	int isave;

	if(args == 0)
		error("include(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("include(string): arg type");

	pushfile(res.string->string);

	isave = interactive;
	interactive = 0;
	r->ival = yyparse();
	interactive = isave;
	popio();
	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
}

void
rc(Node *r, Node *args)
{
	Node res;
	int pid;
	char *p, *q, *argv[4];
	Waitmsg *w;

	USED(r);
	if(args == 0)
		error("error(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("error(string): arg type");

	argv[0] = "/bin/rc";
	argv[1] = "-c";
	argv[2] = res.string->string;
	argv[3] = 0;

	pid = fork();
	switch(pid) {
	case -1:
		error("fork %r");
	case 0:
		exec("/bin/rc", argv);
		exits(0);
	default:
		w = waitfor(pid);
		break;
	}
	p = w->msg;
	q = strrchr(p, ':');
	if (q)
		p = q+1;

	r->op = OCONST;
	r->type = TSTRING;
	r->string = strnode(p);
	free(w);
	r->fmt = 's';
}

void
doerror(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("error(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("error(string): arg type");

	error(res.string->string);
}

void
doaccess(Node *r, Node *args)
{
	Node res;

	if(args == 0)
		error("access(filename): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("access(filename): arg type");

	r->op = OCONST;
	r->type = TINT;
	r->ival = 0;		
	if(access(res.string->string, 4) == 0)
		r->ival = 1;
}

void
readfile(Node *r, Node *args)
{
	Node res;
	int n, fd;
	char *buf;
	Dir *db;

	if(args == 0)
		error("readfile(filename): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("readfile(filename): arg type");

	fd = open(res.string->string, OREAD);
	if(fd < 0)
		return;

	db = dirfstat(fd);
	if(db == nil || db->length == 0)
		n = 8192;
	else
		n = db->length;
	free(db);

	buf = malloc(n);
	n = read(fd, buf, n);

	if(n > 0) {
		r->op = OCONST;
		r->type = TSTRING;
		r->string = strnodlen(buf, n);
		r->fmt = 's';
	}
	free(buf);
	close(fd);
}

void
getfile(Node *r, Node *args)
{
	int n;
	char *p;
	Node res;
	String *s;
	Biobuf *bp;
	List **l, *new;

	if(args == 0)
		error("file(filename): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("file(filename): arg type");

	r->op = OCONST;
	r->type = TLIST;
	r->l = 0;

	p = res.string->string;
	bp = Bopen(p, OREAD);
	if(bp == 0)
		return;

	l = &r->l;
	for(;;) {
		p = Brdline(bp, '\n');
		n = Blinelen(bp);
		if(p == 0) {
			if(n == 0)
				break;
			s = strnodlen(0, n);
			Bread(bp, s->string, n);
		}
		else
			s = strnodlen(p, n-1);

		new = al(TSTRING);
		new->string = s;
		new->fmt = 's';
		*l = new;
		l = &new->next;
	}
	Bterm(bp);
}

void
cvtatof(Node *r, Node *args)
{
	Node res;

	if(args == 0)
		error("atof(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("atof(string): arg type");

	r->op = OCONST;
	r->type = TFLOAT;
	r->fval = atof(res.string->string);
	r->fmt = 'f';
}

void
cvtatoi(Node *r, Node *args)
{
	Node res;

	if(args == 0)
		error("atoi(string): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("atoi(string): arg type");

	r->op = OCONST;
	r->type = TINT;
	r->ival = strtoull(res.string->string, 0, 0);
	r->fmt = 'V';
}

static char *fmtflags = "-0123456789. #,u";
static char *fmtverbs = "bdox";

static int
acidfmt(char *fmt, char *buf, int blen)
{
	char *r, *w, *e;
	
	w = buf;
	e = buf+blen;
	for(r=fmt; *r; r++){
		if(w >= e)
			return -1;
		if(*r != '%'){
			*w++ = *r;
			continue;
		}
		if(*r == '%'){
			*w++ = *r++;
			if(*r == '%'){
				if(w >= e)
					return -1;
				*w++ = *r;
				continue;
			}
			while(*r && strchr(fmtflags, *r)){
				if(w >= e)
					return -1;
				*w++ = *r++;
			}
			if(*r == 0 || strchr(fmtverbs, *r) == nil)
				return -1;
			if(w+3 > e)
				return -1;
			*w++ = 'l';
			*w++ = 'l';
			*w++ = *r;
		}
	}
	if(w >= e)
		return -1;
	*w = 0;

	return 0;
}

void
cvtitoa(Node *r, Node *args)
{
	Node res;
	Node *av[Maxarg];
	vlong ival;
	char buf[128], fmt[32];

	if(args == 0)
err:
		error("itoa(number [, fmt]): arg count");
	na = 0;
	flatten(av, args);
	if(na == 0 || na > 2)
		goto err;
	expr(av[0], &res);
	if(res.type != TINT)
		error("itoa(number [, fmt]): arg type");
	ival = res.ival;
	strncpy(fmt, "%lld", sizeof(fmt));
	if(na == 2){
		expr(av[1], &res);
		if(res.type != TSTRING)
			error("itoa(number [, fmt]): fmt type");
		if(acidfmt(res.string->string, fmt, sizeof(buf)))
			error("itoa(number [, fmt]): malformed fmt");
	}

	snprint(buf, sizeof(buf), fmt, ival);
	r->op = OCONST;
	r->type = TSTRING;
	r->string = strnode(buf);
	r->fmt = 's';
}

List*
mapent(Map *m)
{
	int i;
	List *l, *n, **t, *h;

	h = 0;
	t = &h;
	for(i = 0; i < m->nsegs; i++) {
		if(m->seg[i].inuse == 0)
			continue;
		l = al(TSTRING);
		n = al(TLIST);
		n->l = l;
		*t = n;
		t = &n->next;
		l->string = strnode(m->seg[i].name);
		l->fmt = 's';
		l->next = al(TINT);
		l = l->next;
		l->ival = m->seg[i].b;
		l->fmt = 'W';
		l->next = al(TINT);
		l = l->next;
		l->ival = m->seg[i].e;
		l->fmt = 'W';
		l->next = al(TINT);
		l = l->next;
		l->ival = m->seg[i].f;
		l->fmt = 'W';
	}
	return h;
}

void
map(Node *r, Node *args)
{
	int i;
	Map *m;
	List *l;
	char *ent;
	Node *av[Maxarg], res;

	na = 0;
	flatten(av, args);

	if(na != 0) {
		expr(av[0], &res);
		if(res.type != TLIST)
			error("map(list): map needs a list");
		if(listlen(res.l) != 4)
			error("map(list): list must have 4 entries");

		l = res.l;
		if(l->type != TSTRING)
			error("map name must be a string");
		ent = l->string->string;
		m = symmap;
		i = findseg(m, ent);
		if(i < 0) {
			m = cormap;
			i = findseg(m, ent);
		}
		if(i < 0)
			error("%s is not a map entry", ent);	
		l = l->next;
		if(l->type != TINT)
			error("map entry not int");
		m->seg[i].b = l->ival;
		if (strcmp(ent, "text") == 0)
			textseg(l->ival, &fhdr);
		l = l->next;
		if(l->type != TINT)
			error("map entry not int");
		m->seg[i].e = l->ival;
		l = l->next;
		if(l->type != TINT)
			error("map entry not int");
		m->seg[i].f = l->ival;
	}

	r->type = TLIST;
	r->l = 0;
	if(symmap)
		r->l = mapent(symmap);
	if(cormap) {
		if(r->l == 0)
			r->l = mapent(cormap);
		else {
			for(l = r->l; l->next; l = l->next)
				;
			l->next = mapent(cormap);
		}
	}
}

void 
flatten(Node **av, Node *n)
{
	if(n == 0)
		return;

	switch(n->op) {
	case OLIST:
		flatten(av, n->left);
		flatten(av, n->right);
		break;
	default:
		av[na++] = n;
		if(na >= Maxarg)
			error("too many function arguments");
		break;
	}
}

void
strace(Node *r, Node *args)
{
	Node *av[Maxarg], *n, res;
	uvlong pc, sp;

	na = 0;
	flatten(av, args);
	if(na != 3)
		error("strace(pc, sp, link): arg count");

	n = av[0];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): pc bad type");
	pc = res.ival;

	n = av[1];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): sp bad type");
	sp = res.ival;

	n = av[2];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): link bad type");

	tracelist = 0;
	if ((*machdata->ctrace)(cormap, pc, sp, res.ival, trlist) <= 0)
		error("no stack frame: %r");
	r->type = TLIST;
	r->l = tracelist;
}

void
regerror(char *msg)
{
	error(msg);
}

void
regexp(Node *r, Node *args)
{
	Node res;
	Reprog *rp;
	Node *av[Maxarg];

	na = 0;
	flatten(av, args);
	if(na != 2)
		error("regexp(pattern, string): arg count");
	expr(av[0], &res);
	if(res.type != TSTRING)
		error("regexp(pattern, string): pattern must be string");
	rp = regcomp(res.string->string);
	if(rp == 0)
		return;

	expr(av[1], &res);
	if(res.type != TSTRING)
		error("regexp(pattern, string): bad string");

	r->fmt = 'D';
	r->type = TINT;
	r->ival = regexec(rp, res.string->string, 0, 0);
	free(rp);
}

void
fmt(Node *r, Node *args)
{
	Node res;
	Node *av[Maxarg];

	na = 0;
	flatten(av, args);
	if(na != 2)
		error("fmt(obj, fmt): arg count");
	expr(av[1], &res);
	if(res.type != TINT || strchr(vfmt, res.ival) == 0)
		error("fmt(obj, fmt): bad format '%c'", (char)res.ival);
	expr(av[0], r);
	r->fmt = res.ival;
}

void
patom(char type, Store *res)
{
	int i;
	char buf[512];
	extern char *typenames[];

	switch(res->fmt) {
	case 'c':
		Bprint(bout, "%c", (int)res->ival);
		break;
	case 'C':
		if(res->ival < ' ' || res->ival >= 0x7f)
			Bprint(bout, "%3d", (int)res->ival&0xff);
		else
			Bprint(bout, "%3c", (int)res->ival);
		break;
	case 'r':
		Bprint(bout, "%C", (int)res->ival);
		break;
	case 'B':
		memset(buf, '0', 34);
		buf[1] = 'b';
		for(i = 0; i < 32; i++) {
			if(res->ival & (1<<i))
				buf[33-i] = '1';
		}
		buf[35] = '\0';
		Bprint(bout, "%s", buf);
		break;
	case 'b':
		Bprint(bout, "%.2x", (int)res->ival&0xff);
		break;
	case 'X':
		Bprint(bout, "%.8lux", (ulong)res->ival);
		break;
	case 'x':
		Bprint(bout, "%.4lux", (ulong)res->ival&0xffff);
		break;
	case 'D':
		Bprint(bout, "%d", (int)res->ival);
		break;
	case 'd':
		Bprint(bout, "%d", (ushort)res->ival);
		break;
	case 'u':
		Bprint(bout, "%d", (int)res->ival&0xffff);
		break;
	case 'U':
		Bprint(bout, "%lud", (ulong)res->ival);
		break;
	case 'Z':
		Bprint(bout, "%llud", res->ival);
		break;
	case 'V':
		Bprint(bout, "%lld", res->ival);
		break;
	case 'W':
		Bprint(bout, "%.8llux", res->ival);
		break;
	case 'Y':
		Bprint(bout, "%.16llux", res->ival);
		break;
	case 'o':
		Bprint(bout, "0%.11uo", (int)res->ival&0xffff);
		break;
	case 'O':
		Bprint(bout, "0%.6uo", (int)res->ival);
		break;
	case 'q':
		Bprint(bout, "0%.11o", (short)(res->ival&0xffff));
		break;
	case 'Q':
		Bprint(bout, "0%.6o", (int)res->ival);
		break;
	case 'f':
	case 'F':
	case '3':
	case '8':
		if(type != TFLOAT)
			Bprint(bout, "*%c<%s>*", res->fmt, typenames[type]);
		else
			Bprint(bout, "%g", res->fval);
		break;
	case 's':
	case 'g':
	case 'G':
		if(type != TSTRING)
			Bprint(bout, "*%c<%s>*", res->fmt, typenames[type]);
		else
			Bwrite(bout, res->string->string, res->string->len);
		break;
	case 'R':
		if(type != TSTRING)
			Bprint(bout, "*%c<%s>*", res->fmt, typenames[type]);
		else
			Bprint(bout, "%S", (Rune*)res->string->string);
		break;
	case 'a':
	case 'A':
		symoff(buf, sizeof(buf), res->ival, CANY);
		Bprint(bout, "%s", buf);
		break;
	case 'I':
	case 'i':
		if(type != TINT)
			Bprint(bout, "*%c<%s>*", res->fmt, typenames[type]);
		else {
			if (symmap == nil || (*machdata->das)(symmap, res->ival, res->fmt, buf, sizeof(buf)) < 0)
				Bprint(bout, "no instruction");
			else
				Bprint(bout, "%s", buf);
		}
		break;
	}
}

void
blprint(List *l)
{
	Bprint(bout, "{");
	while(l) {
		switch(l->type) {
		default:
			patom(l->type, &l->Store);
			break;
		case TSTRING:
			Bputc(bout, '"');
			patom(l->type, &l->Store);
			Bputc(bout, '"');
			break;
		case TLIST:
			blprint(l->l);
			break;
		case TCODE:
			pcode(l->cc, 0);
			break;
		}
		l = l->next;
		if(l)
			Bprint(bout, ", ");
	}
	Bprint(bout, "}");
}

int
comx(Node res)
{
	Lsym *sl;
	Node *n, xx;

	if(res.fmt != 'a' && res.fmt != 'A')
		return 0;

	if(res.comt == 0 || res.comt->base == 0)
		return 0;

	sl = res.comt->base;
	if(sl->proc) {
		res.left = ZN;
		res.right = ZN;
		n = an(ONAME, ZN, ZN);
		n->sym = sl;
		n = an(OCALL, n, &res);
			n->left->sym = sl;
		expr(n, &xx);
		return 1;
	}
	print("(%s)", sl->name);
	return 0;
}

void
bprint(Node *r, Node *args)
{
	int i, nas;
	Node res, *av[Maxarg];

	USED(r);
	na = 0;
	flatten(av, args);
	nas = na;
	for(i = 0; i < nas; i++) {
		expr(av[i], &res);
		switch(res.type) {
		default:
			if(comx(res))
				break;
			patom(res.type, &res.Store);
			break;
		case TCODE:
			pcode(res.cc, 0);
			break;
		case TLIST:
			blprint(res.l);
			break;
		}
	}
	if(ret == 0)
		Bputc(bout, '\n');
}

void
printto(Node *r, Node *args)
{
	int fd;
	Biobuf *b;
	int i, nas;
	Node res, *av[Maxarg];

	USED(r);
	na = 0;
	flatten(av, args);
	nas = na;

	expr(av[0], &res);
	if(res.type != TSTRING)
		error("printto(string, ...): need string");

	fd = create(res.string->string, OWRITE, 0666);
	if(fd < 0)
		fd = open(res.string->string, OWRITE);
	if(fd < 0)
		error("printto: open %s: %r", res.string->string);

	b = gmalloc(sizeof(Biobuf));
	Binit(b, fd, OWRITE);

	Bflush(bout);
	io[iop++] = bout;
	bout = b;

	for(i = 1; i < nas; i++) {
		expr(av[i], &res);
		switch(res.type) {
		default:
			if(comx(res))
				break;
			patom(res.type, &res.Store);
			break;
		case TLIST:
			blprint(res.l);
			break;
		}
	}
	if(ret == 0)
		Bputc(bout, '\n');

	Bterm(b);
	close(fd);
	free(b);
	bout = io[--iop];
}

void
pcfile(Node *r, Node *args)
{
	Node res;
	char *p, buf[128];

	if(args == 0)
		error("pcfile(addr): arg count");
	expr(args, &res);
	if(res.type != TINT)
		error("pcfile(addr): arg type");

	r->type = TSTRING;
	r->fmt = 's';
	if(fileline(buf, sizeof(buf), res.ival) == 0) {
		r->string = strnode("?file?");
		return;
	}
	p = strrchr(buf, ':');
	if(p == 0)
		error("pcfile(addr): funny file %s", buf);
	*p = '\0';
	r->string = strnode(buf);	
}

void
pcline(Node *r, Node *args)
{
	Node res;
	char *p, buf[128];

	if(args == 0)
		error("pcline(addr): arg count");
	expr(args, &res);
	if(res.type != TINT)
		error("pcline(addr): arg type");

	r->type = TINT;
	r->fmt = 'D';
	if(fileline(buf, sizeof(buf), res.ival) == 0) {
		r->ival = 0;
		return;
	}

	p = strrchr(buf, ':');
	if(p == 0)
		error("pcline(addr): funny file %s", buf);
	r->ival = strtol(p+1, 0, 0);	
}

void fmtof(Node *r, Node *args)
{
	Node *av[Maxarg];
	Node res;

	na = 0;
	flatten(av, args);
	if(na < 1)
		error("fmtof(obj): no argument");
	if(na > 1)
		error("fmtof(obj): too many arguments") ;
	expr(av[0], &res);

	r->op = OCONST;
	r->type = TINT ;
	r->ival = res.fmt ;
	r->fmt = 'c';
}

void dofmtsize(Node *r, Node *args)
{
	Node *av[Maxarg];
	Node res;
	Store * s ;
	Value v ;

	na = 0;
	flatten(av, args);
	if(na < 1)
		error("fmtsize(obj): no argument");
	if(na > 1)
		error("fmtsize(obj): too many arguments") ;
	expr(av[0], &res);

	v.type = res.type ;
	s = &v.Store ;
	*s = res ;

	r->op = OCONST;
	r->type = TINT ;
	r->ival = fmtsize(&v) ;
	r->fmt = 'D';
}
