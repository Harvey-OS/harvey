#include	"cc.h"
#include	"y.tab.h"

enum
{
	Fnone	= 0,
	Fl,
	Fvl,
	Fignor,
	Fstar,
	Fadj,

	Fverb	= 10,
};

typedef	struct	Tprot	Tprot;
struct	Tprot
{
	Type*	type;
	Bits	flag;
	Tprot*	link;
};

typedef	struct	Tname	Tname;
struct	Tname
{
	char*	name;
	int	param;
	Tname*	link;
};

static	Type*	indchar;
static	uchar	flagbits[512];
static	char	fmtbuf[100];
static	int	lastadj;
static	int	lastverb;
static	int	nstar;
static	Tprot*	tprot;
static	Tname*	tname;

void
argflag(int c, int v)
{

	switch(v) {
	case Fignor:
	case Fstar:
	case Fl:
	case Fvl:
		flagbits[c] = v;
		break;
	case Fverb:
		flagbits[c] = lastverb;
/*print("flag-v %c %d\n", c, lastadj);*/
		lastverb++;
		break;
	case Fadj:
		flagbits[c] = lastadj;
/*print("flag-l %c %d\n", c, lastadj);*/
		lastadj++;
		break;
	}
}

Bits
getflag(char *s)
{
	Bits flag;
	int f;
	char *fmt;
	Rune c;

	fmt = fmtbuf;
	flag = zbits;
	nstar = 0;
	for(;;) {
		s += chartorune(&c, s);
		if(c == 0 || c >= nelem(flagbits))
			break;
		fmt += runetochar(fmt, &c);
		f = flagbits[c];
		switch(f) {
		case Fnone:
			argflag(c, Fverb);
			f = flagbits[c];
			break;
		case Fstar:
			nstar++;
		case Fignor:
			continue;
		case Fl:
			if(bset(flag, Fl))
				flag = bor(flag, blsh(Fvl));
		}
		flag = bor(flag, blsh(f));
		if(f >= Fverb)
			break;
	}
	*fmt = 0;
	return flag;
}

void
newprot(Sym *m, Type *t, char *s)
{
	Bits flag;
	Tprot *l;

	if(t == T) {
		warn(Z, "%s: newprot: type not defined", m->name);
		return;
	}
	flag = getflag(s);
	for(l=tprot; l; l=l->link)
		if(beq(flag, l->flag) && sametype(t, l->type))
			return;
	l = alloc(sizeof(*l));
	l->type = t;
	l->flag = flag;
	l->link = tprot;
	tprot = l;
}

void
newname(char *s, int p)
{
	Tname *l;

	for(l=tname; l; l=l->link)
		if(strcmp(l->name, s) == 0) {
			if(l->param != p)
				yyerror("vargck %s already defined\n", s);
			return;
		}
	l = alloc(sizeof(*l));
	l->name = s;
	l->param = p;
	l->link = tname;
	tname = l;
}

void
arginit(void)
{
	int i;

/* debug['F'] = 1;*/
/* debug['w'] = 1;*/

	lastadj = Fadj;
	lastverb = Fverb;
	indchar = typ(TIND, types[TCHAR]);

	memset(flagbits, Fnone, sizeof(flagbits));

	for(i='0'; i<='9'; i++)
		argflag(i, Fignor);
	argflag('.', Fignor);
	argflag('#', Fignor);
	argflag('u', Fignor);
	argflag('h', Fignor);
	argflag('+', Fignor);
	argflag('-', Fignor);

	argflag('*', Fstar);
	argflag('l', Fl);

	argflag('o', Fverb);
	flagbits['x'] = flagbits['o'];
	flagbits['X'] = flagbits['o'];
}

void
pragvararg(void)
{
	Sym *s;
	int n, c;
	char *t;
	Rune r;
	Type *ty;

	if(!debug['F'])
		goto out;
	s = getsym();
	if(s && strcmp(s->name, "argpos") == 0)
		goto ckpos;
	if(s && strcmp(s->name, "type") == 0)
		goto cktype;
	if(s && strcmp(s->name, "flag") == 0)
		goto ckflag;
	yyerror("syntax in #pragma varargck");
	goto out;

ckpos:
/*#pragma	varargck	argpos	warn	2*/
	s = getsym();
	if(s == S)
		goto bad;
	n = getnsn();
	if(n < 0)
		goto bad;
	newname(s->name, n);
	goto out;

ckflag:
/*#pragma	varargck	flag	'c'*/
	c = getnsc();
	if(c != '\'')
		goto bad;
	c = getr();
	if(c == '\\')
		c = getr();
	else if(c == '\'')
		goto bad;
	if(c == '\n')
		goto bad;
	if(getc() != '\'')
		goto bad;
	argflag(c, Fignor);
	goto out;

cktype:
/*#pragma	varargck	type	O	int*/
	c = getnsc();
	if(c != '"')
		goto bad;
	t = fmtbuf;
	for(;;) {
		r = getr();
		if(r == ' ' || r == '\n')
			goto bad;
		if(r == '"')
			break;
		t += runetochar(t, &r);
	}
	*t = 0;
	t = strdup(fmtbuf);
	s = getsym();
	if(s == S)
		goto bad;
	ty = s->type;
	while((c = getnsc()) == '*')
		ty = typ(TIND, ty);
	unget(c);
	newprot(s, ty, t);
	goto out;

bad:
	yyerror("syntax in #pragma varargck");

out:
	while(getnsc() != '\n')
		;
}

Node*
nextarg(Node *n, Node **a)
{
	if(n == Z) {
		*a = Z;
		return Z;
	}
	if(n->op == OLIST) {
		*a = n->left;
		return n->right;
	}
	*a = n;
	return Z;
}

void
checkargs(Node *nn, char *s, int pos)
{
	Node *a, *n;
	Bits flag;
	Tprot *l;

	if(!debug['F'])
		return;
	n = nn;
	for(;;) {
		s = strchr(s, '%');
		if(s == 0) {
			nextarg(n, &a);
			if(a != Z)
				warn(nn, "more arguments than format %T",
					a->type);
			return;
		}
		s++;
		flag = getflag(s);
		while(nstar > 0) {
			n = nextarg(n, &a);
			pos++;
			nstar--;
			if(a == Z) {
				warn(nn, "more format than arguments %s",
					fmtbuf);
				return;
			}
			if(a->type == T)
				continue;
			if(!sametype(types[TINT], a->type) &&
			   !sametype(types[TUINT], a->type))
				warn(nn, "format mismatch '*' in %s %T, arg %d",
					fmtbuf, a->type, pos);
		}
		for(l=tprot; l; l=l->link)
			if(sametype(types[TVOID], l->type)) {
				if(beq(flag, l->flag)) {
					s++;
					goto loop;
				}
			}

		n = nextarg(n, &a);
		pos++;
		if(a == Z) {
			warn(nn, "more format than arguments %s",
				fmtbuf);
			return;
		}
		if(a->type == 0)
			continue;
		for(l=tprot; l; l=l->link)
			if(sametype(a->type, l->type)) {
/*print("checking %T/%ulx %T/%ulx\n", a->type, flag.b[0], l->type, l->flag.b[0]);*/
				if(beq(flag, l->flag))
					goto loop;
			}
		warn(nn, "format mismatch %s %T, arg %d", fmtbuf, a->type, pos);
	loop:;
	}
}

void
dpcheck(Node *n)
{
	char *s;
	Node *a, *b;
	Tname *l;
	int i;

	if(n == Z)
		return;
	b = n->left;
	if(b == Z || b->op != ONAME)
		return;
	s = b->sym->name;
	for(l=tname; l; l=l->link)
		if(strcmp(s, l->name) == 0)
			break;
	if(l == 0)
		return;

	i = l->param;
	b = n->right;
	while(i > 0) {
		b = nextarg(b, &a);
		i--;
	}
	if(a == Z) {
		warn(n, "cant find format arg");
		return;
	}
	if(!sametype(indchar, a->type)) {
		warn(n, "format arg type %T", a->type);
		return;
	}
	if(a->op != OADDR || a->left->op != ONAME || a->left->sym != symstring) {
/*		warn(n, "format arg not constant string");*/
		return;
	}
	s = a->left->cstring;
	checkargs(b, s, l->param);
}

void
pragpack(void)
{
	Sym *s;

	packflg = 0;
	s = getsym();
	if(s) {
		packflg = atoi(s->name+1);
		if(strcmp(s->name, "on") == 0 ||
		   strcmp(s->name, "yes") == 0)
			packflg = 1;
	}
	while(getnsc() != '\n')
		;
	if(debug['f'])
		if(packflg)
			print("%4ld: pack %d\n", lineno, packflg);
		else
			print("%4ld: pack off\n", lineno);
}

void
pragfpround(void)
{
	Sym *s;

	fproundflg = 0;
	s = getsym();
	if(s) {
		fproundflg = atoi(s->name+1);
		if(strcmp(s->name, "on") == 0 ||
		   strcmp(s->name, "yes") == 0)
			fproundflg = 1;
	}
	while(getnsc() != '\n')
		;
	if(debug['f'])
		if(fproundflg)
			print("%4ld: fproundflg %d\n", lineno, fproundflg);
		else
			print("%4ld: fproundflg off\n", lineno);
}

void
pragprofile(void)
{
	Sym *s;

	profileflg = 0;
	s = getsym();
	if(s) {
		profileflg = atoi(s->name+1);
		if(strcmp(s->name, "on") == 0 ||
		   strcmp(s->name, "yes") == 0)
			profileflg = 1;
	}
	while(getnsc() != '\n')
		;
	if(debug['f'])
		if(profileflg)
			print("%4ld: profileflg %d\n", lineno, profileflg);
		else
			print("%4ld: profileflg off\n", lineno);
}

void
pragincomplete(void)
{
	Sym *s;
	Type *t;
	int istag, w, et;

	istag = 0;
	s = getsym();
	if(s == nil)
		goto out;
	et = 0;
	w = s->lexical;
	if(w == LSTRUCT)
		et = TSTRUCT;
	else if(w == LUNION)
		et = TUNION;
	if(et != 0){
		s = getsym();
		if(s == nil){
			yyerror("missing struct/union tag in pragma incomplete");
			goto out;
		}
		if(s->lexical != LNAME && s->lexical != LTYPE){
			yyerror("invalid struct/union tag: %s", s->name);
			goto out;
		}
		dotag(s, et, 0);
		istag = 1;
	}else if(strcmp(s->name, "_off_") == 0){
		debug['T'] = 0;
		goto out;
	}else if(strcmp(s->name, "_on_") == 0){
		debug['T'] = 1;
		goto out;
	}
	t = s->type;
	if(istag)
		t = s->suetag;
	if(t == T)
		yyerror("unknown type %s in pragma incomplete", s->name);
	else if(!typesu[t->etype])
		yyerror("not struct/union type in pragma incomplete: %s", s->name);
	else
		t->garb |= GINCOMPLETE;
out:
	while(getnsc() != '\n')
		;
	if(debug['f'])
		print("%s incomplete\n", s->name);
}
