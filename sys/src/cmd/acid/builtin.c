#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

void	cvtatof(Node*, Node*);
void	cvtatoi(Node*, Node*);
void	cvtitoa(Node*, Node*);
void	bprint(Node*, Node*);
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
void	tag(Node*,Node*);
void	waitstop(Node*, Node*);
void	stop(Node*, Node*);
void	start(Node*, Node*);
void	filepc(Node*, Node*);
void	doerror(Node*, Node*);
void	rc(Node*, Node*);
void	doaccess(Node*, Node*);

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
	"access",	doaccess,
	"filepc",	filepc,
	"fmt",		fmt,
	"follow",	follow,
	"itoa",		cvtitoa,
	"kill",		kill,
	"match",	match,
	"newproc",	newproc,
	"pcfile",	pcfile,
	"pcline",	pcline,
	"print",	bprint,
	"rc",		rc,
	"reason",	reason,
	"setproc",	setproc,
	"start",	start,
	"startstop",	startstop,
	"status",	status,
	"stop",		stop,
	"strace",	strace,
	"tag",		tag,
	"waitstop",	waitstop,
	0
};

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
		b++;
	}
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
	char *argv[Maxarg], buf[2048];

	i = 1;
	argv[0] = aout;

	if(args) {
		expr(args, &res);
		if(res.type != TSTRING)
			error("new(): arg not string");
		if(res.string->len >= sizeof(buf))
			error("new(): too many arguments");
		memmove(buf, res.string->string, res.string->len);
		p = buf;
		e = buf+res.string->len;
		for(;;) {
			while(p < e && (*p == '\t' || *p == ' '))
				*p++ = '\0';
			if(p >= e)
				break;
			argv[i++] = p;
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
	int fd;
	Node res;
	char buf[128], *p;

	USED(r);
	if(args == 0)
		error("status(pid): no pid");
	expr(args, &res);
	if(res.type != TINT)
		error("status(pid): arg type");

	sprint(buf, "/proc/%d/status", res.ival);
	fd = open(buf, OREAD);
	if(fd < 0)
		error("open %s: %r", buf);
	read(fd, buf, sizeof(buf));
	close(fd);
	r->op = OCONST;
	r->fmt = 's';
	p = buf+56+12;			/* Do better! */
	while(*p == ' ')
		p--;
	p[1] = '\0';
	r->string = strnode(buf+56);	/* Ditto */
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

	strc.cause = res.ival;
	(*machdata->excep)();
	r->op = OCONST;
	r->type = TSTRING;
	r->fmt = 's';
	r->string = strnode(strc.excep);
}

void
follow(Node *r, Node *args)
{
	int n, i;
	Node res;
	ulong f[10];
	List **tail, *l;

	if(args == 0)
		error("follow(addr): no addr");
	expr(args, &res);
	if(res.type != TINT)
		error("follow(addr): arg type");

	n = (*machdata->foll)(res.ival, f);

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
	char *p;
	Node res;

	if(args == 0)
		error("filepc(filename:line): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("filepc(filename:line): arg type");

	p = strchr(res.string->string, ':');
	if(p == 0)
		error("filepc(filename:line): bad arg format");

	*p++ = '\0';
	r->ival = file2pc(res.string->string, atoi(p));
	if(r->ival == 0)
		error("filepc(filename:line): can't find address");

	r->op = OCONST;
	r->type = TINT;
	r->fmt = 'D';
}

void
rc(Node *r, Node *args)
{
	Node res;
	Waitmsg w;
	int pid, n;
	char *argv[4];

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
		for(;;) {
			n = wait(&w);
			if(n < 0)
				error("wait %r");
			if(n == pid)
				break;
		}	
	}
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
	if(access(res.string->string, OREAD) == 0)
		r->ival = 1;
}

void
getfile(Node *r, Node *args)
{
	char *p;
	Node res;
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
		if(p == 0)
			break;
		new = al(TSTRING);
		p[BLINELEN(bp)-1] = '\0';
		new->string = strnode(p);
		new->fmt = 's';
		*l = new;
		l = &new->next;			
	}
	Bclose(bp);
}

void
tag(Node *r, Node *args)
{
	Node res;

	USED(r);
	if(args == 0)
		error("tag(filename): arg count");
	expr(args, &res);
	if(res.type != TSTRING)
		error("tag(filename): arg type");

	ltag(res.string->string);
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
	r->ival = strtoul(res.string->string, 0, 0);
}

void
cvtitoa(Node *r, Node *args)
{
	Node res;
	char buf[128];

	if(args == 0)
		error("itoa(integer): arg count");
	expr(args, &res);
	if(res.type != TINT)
		error("itoa(integer): arg type");

	sprint(buf, "%d", res.ival);
	r->op = OCONST;
	r->type = TSTRING;
	r->string = strnode(buf);
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

	na = 0;
	flatten(av, args);
	if(na != 2)
		error("strace(pc, sp): arg count");

	n = av[0];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp): pc bad type");
	strc.pc = res.ival;
	n = av[1];
	expr(n, &res);
	if(res.type != TINT)
		error("strace(pc, sp): sp bad type");
	strc.sp = res.ival;
	strc.l = 0;
	(*machdata->ctrace)(0);
	r->type = TLIST;
	r->l = strc.l;
}

char vfmt[] = "cCbsxXdDuUoOaFfiIqQrRY";

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
		error("fmt(obj, fmt): bad format");
	expr(av[0], r);
	r->fmt = res.ival;
}

void
patom(char type, Store *res)
{
	char *p;

	switch(res->fmt) {
	case 'c':
		Bprint(bout, "%c ", res->ival);
		break;
	case 'C':
		if(res->ival < ' ' || res->ival >= 0x7f)
			Bprint(bout, "%3d ", res->ival&0xff);
		else
			Bprint(bout, "%3c ", res->ival);
		break;
	case 'r':
		print("%C ", res->ival);
		break;
	case 'b':
		Bprint(bout, "%3d ", res->ival&0xff);
		break;
	case 'X':
		Bprint(bout, "0x%.8lux ", res->ival);
		break;
	case 'x':
		Bprint(bout, "0x%.4lux ", res->ival&0xffff);
		break;
	case 'D':
		Bprint(bout, "%d ", res->ival);
		break;
	case 'd':
		Bprint(bout, "%d ", (ushort)res->ival);
		break;
	case 'u':
		Bprint(bout, "%d ", res->ival&0xffff);
		break;
	case 'U':
		Bprint(bout, "%d ", (ulong)res->ival);
		break;
	case 'o':
		Bprint(bout, "0%.11uo ", res->ival&0xffff);
		break;
	case 'O':
		Bprint(bout, "0%.6uo ", res->ival);
		break;
	case 'q':
		Bprint(bout, "0%.11o ", (short)(res->ival&0xffff));
		break;
	case 'Q':
		Bprint(bout, "0%.6o ", res->ival);
		break;
	case 'f':
	case 'F':
		Bprint(bout, "%f ", res->fval);
		break;
	case 's':
		if(type != TSTRING)
			break;
		Bprint(bout, "%s", res->string->string);
		break;
	case 'R':
		if(type != TSTRING)
			break;
		Bprint(bout, "%R", res->string->string);
		break;
	case 'a':
		psymoff(res->ival, SEGANY, "");
		break;
	case 'Y':
		p = ctime(res->ival);
		p[strlen(p)-1] = '\0';
		Bprint(bout, "%s", p);
		break;
	case 'I':
	case 'i':
		if(type != TINT)
			break;
		asmbuf[0] = '\0';
		dot = res->ival;
		(*machdata->printins)(symmap, res->fmt, SEGANY);
		break;
	}
}

void
blprint(List *l)
{
	Bprint(bout, "{");
	while(l) {
		if(l->type != TLIST)
			patom(l->type, &l->Store);
		else
			blprint(l->l);
		l = l->next;
		if(l)
			Bprint(bout, ", ");
	}
	Bprint(bout, "}");
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
			patom(res.type, &res.Store);
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
	r->ival = atoi(p+1);	
}
