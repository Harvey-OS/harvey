/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
void	dosysr0(Node*, Node*);
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
	{ "atof",	cvtatof, },
	{ "atoi",	cvtatoi, },
	{ "error",	doerror, },
	{ "file",	getfile, },
	{ "readfile",	readfile, },
	{ "access",	doaccess, },
	{ "filepc",	filepc, },
	{ "fnbound",	funcbound, },
	{ "fmt",	fmt, },
	{ "follow",	follow, },
	{ "itoa",	cvtitoa, },
	{ "kill",	kill, },
	{ "match",	match, },
	{ "newproc",	newproc, },
	{ "pcfile",	pcfile, },
	{ "pcline",	pcline, },
	{ "print",	bprint, },
	{ "printto",	printto, },
	{ "rc",		rc, },
	{ "reason",	reason, },
	{ "setproc",	setproc, },
	{ "start",	start, },
	{ "startstop",	startstop, },
	{ "status",	status, },
	{ "stop",	stop, },
	{ "strace",	strace, },
	{ "sysr0",	dosysr0, },
	{ "waitstop",	waitstop, },
	{ "map",	map, },
	{ "interpret",	interpret, },
	{ "include",	include, },
	{ "regexp",	regexp, },
	{ "fmtof",	fmtof, },
	{ "fmtsize",	dofmtsize, },
	{ 0 },
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
dosysr0(Node *r, Node*_)
{
	extern int sysr0(void);

	r->op = OCONST;
	r->type = TINT;
	r->store.fmt = 'D';
	fprint(2, "Thank Giacomo -- no sysr1 -- we'll bring it back\n");
	//r->ival = sysr1();
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
	r->store.fmt = 'D';
	r->store.ival = -1;

	i = 0;
	for(f = resl.store.l; f; f = f->next) {
		if(resi.type == f->type) {
			switch(resi.type) {
			case TINT:
				if(resi.store.ival == f->store.ival) {
					r->store.ival = i;
					return;
				}
				break;
			case TFLOAT:
				if(resi.store.fval == f->store.fval) {
					r->store.ival = i;
					return;
				}
				break;
			case TSTRING:
				if(scmp(resi.store.string, f->store.string)) {
					r->store.ival = i;
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
	argv[0] = exe;

	if(args) {
		expr(args, &res);
		if(res.type != TSTRING)
			error("newproc(): arg not string");
		if(res.store.string->len >= sizeof(buf))
			error("newproc(): too many arguments");
		memmove(buf, res.store.string->string, res.store.string->len);
		buf[res.store.string->len] = '\0';
		p = buf;
		e = buf+res.store.string->len;
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
	r->store.fmt = 'D';
	r->store.ival = nproc(argv);
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

	msg(res.store.ival, "startstop");
	notes(res.store.ival);
	dostop(res.store.ival);
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
	msg(res.store.ival, "waitstop");
	notes(res.store.ival);
	dostop(res.store.ival);
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

	msg(res.store.ival, "start");
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
	msg(res.store.ival, "stop");
	notes(res.store.ival);
	dostop(res.store.ival);
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

	msg(res.store.ival, "kill");
	deinstall(res.store.ival);
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

	p = getstatus(res.store.ival);
	r->store.string = strnode(p);
	r->op = OCONST;
	r->store.fmt = 's';
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
	r->store.fmt = 's';
	r->store.string = strnode((*machdata->excep)(cormap, rget));
}

void
follow(Node *r, Node *args)
{
	int n, i;
	Node res;
	uint64_t f[10];
	List **tail, *l;

	if(args == 0)
		error("follow(addr): no addr");
	expr(args, &res);
	if(res.type != TINT)
		error("follow(addr): arg type");

	n = (*machdata->foll)(cormap, res.store.ival, rget, f);
	if (n < 0)
		error("follow(addr): %r");
	tail = &r->store.l;
	for(i = 0; i < n; i++) {
		l = al(TINT);
		l->store.ival = f[i];
		l->store.fmt = 'X';
		*tail = l;
		tail = &l->next;
	}
}

void
funcbound(Node *r, Node *args)
{
	int n;
	Node res;
	uint64_t bounds[2];
	List *l;

	if(args == 0)
		error("fnbound(addr): no addr");
	expr(args, &res);
	if(res.type != TINT)
		error("fnbound(addr): arg type");

	n = fnbound(res.store.ival, bounds);
	if (n != 0) {
		r->store.l = al(TINT);
		l = r->store.l;
		l->store.ival = bounds[0];
		l->store.fmt = 'X';
		l->next = al(TINT);
		l = l->next;
		l->store.ival = bounds[1];
		l->store.fmt = 'X';
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

	sproc(res.store.ival);
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

	p = strchr(res.store.string->string, ':');
	if(p == 0)
		error("filepc(filename:line): bad arg format");

	c = *p;
	*p++ = '\0';
	r->store.ival = file2pc(res.store.string->string, strtol(p, 0, 0));
	p[-1] = c;
	if(r->store.ival == ~0)
		error("filepc(filename:line): can't find address");

	r->op = OCONST;
	r->type = TINT;
	r->store.fmt = 'V';
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
	r->store.ival = yyparse();
	interactive = isave;
	popio();
	r->op = OCONST;
	r->type = TINT;
	r->store.fmt = 'D';
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

	pushfile(res.store.string->string);

	isave = interactive;
	interactive = 0;
	r->store.ival = yyparse();
	interactive = isave;
	popio();
	r->op = OCONST;
	r->type = TINT;
	r->store.fmt = 'D';
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
	argv[2] = res.store.string->string;
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
	r->store.string = strnode(p);
	free(w);
	r->store.fmt = 's';
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

	error(res.store.string->string);
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
	r->store.ival = 0;
	if(access(res.store.string->string, 4) == 0)
		r->store.ival = 1;
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

	fd = open(res.store.string->string, OREAD);
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
		r->store.string = strnodlen(buf, n);
		r->store.fmt = 's';
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
	r->store.l = 0;

	p = res.store.string->string;
	bp = Bopen(p, OREAD);
	if(bp == 0)
		return;

	l = &r->store.l;
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
		new->store.string = s;
		new->store.fmt = 's';
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
	r->store.fval = atof(res.store.string->string);
	r->store.fmt = 'f';
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
	r->store.ival = strtoull(res.store.string->string, 0, 0);
	r->store.fmt = 'V';
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
	int64_t ival;
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
	ival = res.store.ival;
	strncpy(fmt, "%lld", sizeof(fmt));
	if(na == 2){
		expr(av[1], &res);
		if(res.type != TSTRING)
			error("itoa(number [, fmt]): fmt type");
		if(acidfmt(res.store.string->string, fmt, sizeof(buf)))
			error("itoa(number [, fmt]): malformed fmt");
	}

	snprint(buf, sizeof(buf), fmt, ival);
	r->op = OCONST;
	r->type = TSTRING;
	r->store.string = strnode(buf);
	r->store.fmt = 's';
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
		n->store.l = l;
		*t = n;
		t = &n->next;
		l->store.string = strnode(m->seg[i].name);
		l->store.fmt = 's';
		l->next = al(TINT);
		l = l->next;
		l->store.ival = m->seg[i].b;
		l->store.fmt = 'W';
		l->next = al(TINT);
		l = l->next;
		l->store.ival = m->seg[i].e;
		l->store.fmt = 'W';
		l->next = al(TINT);
		l = l->next;
		l->store.ival = m->seg[i].f;
		l->store.fmt = 'W';
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
		if(listlen(res.store.l) != 4)
			error("map(list): list must have 4 entries");

		l = res.store.l;
		if(l->type != TSTRING)
			error("map name must be a string");
		ent = l->store.string->string;
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
		m->seg[i].b = l->store.ival;
		if (strcmp(ent, "text") == 0)
			textseg(l->store.ival, &fhdr);
		l = l->next;
		if(l->type != TINT)
			error("map entry not int");
		m->seg[i].e = l->store.ival;
		l = l->next;
		if(l->type != TINT)
			error("map entry not int");
		m->seg[i].f = l->store.ival;
	}

	r->type = TLIST;
	r->store.l = 0;
	if(symmap)
		r->store.l = mapent(symmap);
	if(cormap) {
		if(r->store.l == 0)
			r->store.l = mapent(cormap);
		else {
			for(l = r->store.l; l->next; l = l->next)
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
	uint64_t pc, sp;

	na = 0;
	flatten(av, args);
	if(na != 3)
		error("strace(pc, sp, link): arg count");

	n = av[0];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): pc bad type");
	pc = res.store.ival;

	n = av[1];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): sp bad type");
	sp = res.store.ival;

	n = av[2];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp, link): link bad type");

	tracelist = 0;
	if ((*machdata->ctrace)(cormap, pc, sp, res.store.ival, trlist) <= 0)
		error("no stack frame: %r");
	r->type = TLIST;
	r->store.l = tracelist;
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
	rp = regcomp(res.store.string->string);
	if(rp == 0)
		return;

	expr(av[1], &res);
	if(res.type != TSTRING)
		error("regexp(pattern, string): bad string");

	r->store.fmt = 'D';
	r->type = TINT;
	r->store.ival = regexec(rp, res.store.string->string, 0, 0);
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
	if(res.type != TINT || strchr(vfmt, res.store.ival) == 0)
		error("fmt(obj, fmt): bad format '%c'", (char)res.store.ival);
	expr(av[0], r);
	r->store.fmt = res.store.ival;
}

void
patom(uint8_t type, Store *res)
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
		Bprint(bout, "%.8lx", (uint32_t)res->ival);
		break;
	case 'x':
		Bprint(bout, "%.4lx", (uint32_t)res->ival&0xffff);
		break;
	case 'D':
		Bprint(bout, "%d", (int)res->ival);
		break;
	case 'd':
		Bprint(bout, "%d", (uint16_t)res->ival);
		break;
	case 'u':
		Bprint(bout, "%d", (int)res->ival&0xffff);
		break;
	case 'U':
		Bprint(bout, "%lu", (uint32_t)res->ival);
		break;
	case 'Z':
		Bprint(bout, "%llu", res->ival);
		break;
	case 'V':
		Bprint(bout, "%lld", res->ival);
		break;
	case 'W':
		Bprint(bout, "%.8llx", res->ival);
		break;
	case 'Y':
		Bprint(bout, "%.16llx", res->ival);
		break;
	case 'o':
		Bprint(bout, "0%.11o", (int)res->ival&0xffff);
		break;
	case 'O':
		Bprint(bout, "0%.6o", (int)res->ival);
		break;
	case 'q':
		Bprint(bout, "0%.11o", (int16_t)(res->ival&0xffff));
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
			patom(l->type, &l->store);
			break;
		case TSTRING:
			Bputc(bout, '"');
			patom(l->type, &l->store);
			Bputc(bout, '"');
			break;
		case TLIST:
			blprint(l->store.l);
			break;
		case TCODE:
			pcode(l->store.cc, 0);
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

	if(res.store.fmt != 'a' && res.store.fmt != 'A')
		return 0;

	if(res.store.comt == 0 || res.store.comt->base == 0)
		return 0;

	sl = res.store.comt->base;
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
			patom(res.type, &res.store);
			break;
		case TCODE:
			pcode(res.store.cc, 0);
			break;
		case TLIST:
			blprint(res.store.l);
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

	fd = create(res.store.string->string, OWRITE, 0666);
	if(fd < 0)
		fd = open(res.store.string->string, OWRITE);
	if(fd < 0)
		error("printto: open %s: %r", res.store.string->string);

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
			patom(res.type, &res.store);
			break;
		case TLIST:
			blprint(res.store.l);
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
	r->store.fmt = 's';
	if(fileline(buf, sizeof(buf), res.store.ival) == 0) {
		r->store.string = strnode("?file?");
		return;
	}
	p = strrchr(buf, ':');
	if(p == 0)
		error("pcfile(addr): funny file %s", buf);
	*p = '\0';
	r->store.string = strnode(buf);
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
	r->store.fmt = 'D';
	if(fileline(buf, sizeof(buf), res.store.ival) == 0) {
		r->store.ival = 0;
		return;
	}

	p = strrchr(buf, ':');
	if(p == 0)
		error("pcline(addr): funny file %s", buf);
	r->store.ival = strtol(p+1, 0, 0);
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
	r->store.ival = res.store.fmt ;
	r->store.fmt = 'c';
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
	s = &v.store ;
	*s = res.store ;

	r->op = OCONST;
	r->type = TINT ;
	r->store.ival = fmtsize(&v) ;
	r->store.fmt = 'D';
}
