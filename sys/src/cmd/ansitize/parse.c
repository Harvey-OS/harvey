#include "a.h"

/*
 * AST matching
 */
int
match(AST *ast, char *name)
{
	if(ast && ast->sym && ast->sym->name && strcmp(ast->sym->name, name) == 0)
		return 1;
	return 0;
}

int
matchrule(AST *ast, char *left, char *right0, ...)
{
	int i;
	char *name;
	va_list arg;
	
	if(!match(ast, left))
		return 0;
	va_start(arg, right0);
	for(i=0, name=right0; i<ast->nright && name; i++, name=va_arg(arg, char*))
		if(!match(*ast->right[i], name)){
			va_end(arg);
			return 0;
		}
	va_end(arg);
	if(i != ast->nright || name != nil)
		return 0;
	return 1;
}

/*
 * Grammar for grammars.
 */
static char *dequote(char*);
static DFA *dfa1;
static Ygram *gram1;
static int nprec1;

static DFA *dfa2;
static Ygram *gram2;
static int nlprec2, nyprec2;
static Hash *names2;
static Hash *literals2;
static Hash *regexps2;
static Hash *ignores2;
static char *lastbit;
static Ysym *start2;
static int action2(Ygram*, Yrule*, void*, void*);

#define Ignore	((Ysym*)-1)

static int
flagof(char *s)
{
	char *p;
	
	p = s+strlen(s);
	if(p>s && *(p-1) == 's')
		return RegexpShortest;
	return 0;
}

/* lexer - use dfa, unquote regexp and literal symbols */
static Ysym*
yylex1(Ygram *g, void *yys, void *arg)
{
	char *op, *s, **pp;
	int l;
	Ysym *sym;
	Gval *out;
	
	out = yys;
	pp = arg;
again:
	if(*pp == nil || **pp == 0)
		return yylooksym(g, "EOF");
	op = *pp;
	if((sym = rundfa(dfa1, *pp, pp)) == nil)
		sysfatal("input stuck on: %s", *pp);
	if(sym == Ignore)
		goto again;
	if(*pp == op)
		sysfatal("parsed empty string");
	l = *pp-op;
	s = malloc(l+1);
	memmove(s, op, l);
	s[l] = 0;
	if(strcmp(sym->name, "regexp") == 0 || strcmp(sym->name, "literal") == 0){
		out->flag = flagof(s);
		s = nameof(dequote(s));
	}
	out->x = nameof(s);
	out->yaccsym = sym;
	return sym;
}

/* turn /x/ into x */
static char*
dequote(char *s)
{
	char c;
	int l;
	
	c = *s;
	memmove(s, s+1, strlen(s));
	l = strlen(s);
	while(l > 0 && s[l-1] != c)
		s[--l] = 0;
	s[--l] = 0;
	
	return s;
}

/* add a literal to the grammar */
static void
literal1(char *lit, char *name)
{
	addtodfa(dfa1, lit, RegexpLiteral, --nprec1, yynewsym(gram1, name, nil, 0, 0));
}

/* parse action - decl: op namelist "\n" */
static int
asymdecl(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin;
	Ysym *sym;
	
	USED(g);
	USED(r);
	USED(vout);
	++nyprec2;
	forlist(sym, in[1].al.first){
		sym->prec = 2*nyprec2;
		sym->assoc = in[0].assoc;
	}
	return 0;
}

/* parse action - decl: name ":" namelist extra "\n" */
static int
aruledecl(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin;
	void **args;
	int n;
	
	USED(g);
	USED(r);
	USED(vout);
	n = listlen(in[2].al.first);
	args = listflatten(in[2].al.first);
	yyrule(gram2, action2, in[0].sym, in[3].prec, (Ysym**)args, n);
	return 0;
}

/* parse action - decl: start sym "\n" */
static int
astartdecl(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin;

	USED(g);
	USED(r);
	USED(vout);
	start2 = in[1].sym;
	return 0;
}

/* parse action - decl: ignore symlist "\n" */
static int
aignoredecl(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin;
	Ysym *sym;
	
	USED(g);
	USED(r);
	USED(vout);
	forlist(sym, in[1].al.first)
		hashput(ignores2, sym, sym);
	return 0;
}

/* parse action - sym: name */
static int
asymname(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin, *out = vout;
	Ysym *sym;

	USED(g);
	USED(r);
	if((sym = hashget(names2, in[0].x)) == nil){
		sym = yynewsym(gram2, in[0].x, nil, 0, 0);
		hashput(names2, in[0].x, sym);
	}
	out->sym = sym;
	return 0;
}

/* parse action - sym: literal */
static int
asymliteral(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin, *out = vout;
	Ysym *sym;
	char buf[40];
	
	USED(g);
	USED(r);
	if((sym = hashget(literals2, in[0].x)) == nil){
		snprint(buf, sizeof buf, "'%s", in[0].x);
		sym = yynewsym(gram2, buf, nil, 0, 0);
		addtodfa(dfa2, in[0].x, RegexpLiteral, --nlprec2, sym);
		hashput(literals2, in[0].x, sym);
	}
	out->sym = sym;
	return 0;
}

/* parse action - sym: regexp */
static int
asymregexp(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin, *out = vout;
	Ysym *sym;
	char buf[40];
	
	USED(g);
	USED(r);
	if((sym = hashget(regexps2, in[0].x)) == nil){
		snprint(buf, sizeof buf, "/%s", in[0].x);
		sym = yynewsym(gram2, buf, nil, 0, 0);
		addtodfa(dfa2, in[0].x, RegexpNormal|in[0].flag, --nlprec2, sym);
		hashput(regexps2, in[0].x, sym);
	}
	out->sym = sym;
	return 0;
}

/* parse action - symlist: */
static int
asymlistempty(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *out = vout;
	
	USED(g);
	USED(r);
	USED(vin);
	listreset(&out->al);
	return 0;
}

/* parse action - symlist: symlist sym */
static int
asymlistsym(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin, *out = vout;

	USED(g);
	USED(r);
	listadd(&out->al, in[1].sym);
	return 0;
}

/* parse action - extra: */
static int
aextranone(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *out = vout;

	USED(g);
	USED(r);
	USED(vin);
	out->prec = nil;
	return 0;
}

/* parse action - extra: %prec sym */
static int
aextraprec(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *in = vin, *out = vout;

	USED(g);
	USED(r);
	if(in[1].sym == nil)
		return 0;
	out->prec = in[1].sym;
	return 0;
}

/* parse action - op: %left */
static int
aopleft(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *out = vout;

	USED(g);
	USED(r);
	USED(vin);
	out->assoc = Yleft;
	return 0;
}

/* parse action - op: %right */
static int
aopright(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *out = vout;
	
	USED(g);
	USED(r);
	USED(vin);
	out->assoc = Yright;
	return 0;
}

/* parse action - op: %token */
static int
aoptoken(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Gval *out = vout;

	USED(g);
	USED(r);
	USED(vin);
	out->assoc = Ynonassoc;
	return 0;
}

static void
parsergrammar(Ygram *g, DFA *dfa)
{
	g->valsize = sizeof(Gval);

	addtodfa(dfa, "[ \t]+", RegexpNormal, 0, Ignore);
	addtodfa(dfa, "#.*", RegexpNormal, 0, Ignore);

	literal1("%left", "left");
	literal1("%right", "right");
	literal1("%token", "token");
	literal1("%prec", "prec");
	literal1("%start", "start");
	literal1("%ignore", "ignore");
	literal1("%alt", "alt");
	literal1(":", ":");
	literal1("\n", "\\n");

	addtodfa(dfa, "[a-zA-Z0-9_\\-+*?,]+", RegexpNormal, --nprec1, 
		yynewsym(g, "name", nil, 0, 0));
	addtodfa(dfa, "\"[^\"]+\"", RegexpNormal, --nprec1,
		yynewsym(g, "literal", nil, 0, 0));
	addtodfa(dfa, "(/([^/]|\\\\/)+/|\\$([^$]|\\\\\\$)+\\$)s?", RegexpNormal, --nprec1,
		yynewsym(g, "regexp", nil, 0, 0));
///([^$]|\\\\\\$)+\\
	yynewsym(g, "decl", nil, 0, 0);
	yynewsym(g, "op", nil, 0, 0);
	yynewsym(g, "sym", nil, 0, 0);
	yynewsym(g, "symlist", nil, 0, 0);
	yynewsym(g, "extra", nil, 0, 0);
	yynewsym(g, "decl", nil, 0, 0);

	yynewsym(g, "prog", nil, 0, 0);
	yyrulestr(g, nil, "prog", nil, nil);
	yyrulestr(g, nil, "prog", nil, "prog", "decl", nil);
	yyrulestr(g, nil, "prog", nil, "prog", "\\n", nil);

	yyrulestr(g, asymdecl, "decl", nil, "op", "symlist", "\\n", nil);
	yyrulestr(g, aruledecl, "decl", nil, "sym", ":", "symlist", "extra", "\\n", nil);
	yyrulestr(g, astartdecl, "decl", nil, "start", "sym", "\\n", nil);
	yyrulestr(g, aignoredecl, "decl", nil, "ignore", "symlist", "\\n", nil);
	
	yyrulestr(g, asymname, "sym", nil, "name", nil);
	yyrulestr(g, asymliteral, "sym", nil, "literal", nil);
	yyrulestr(g, asymregexp, "sym", nil, "regexp", nil);

	yyrulestr(g, asymlistempty, "symlist", nil, nil);
	yyrulestr(g, asymlistsym, "symlist", nil, "symlist", "sym", nil);

	yyrulestr(g, aextranone, "extra", nil, nil);
	yyrulestr(g, aextraprec, "extra", nil, "prec", "sym", nil);

	yyrulestr(g, aopleft, "op", nil, "left", nil);
	yyrulestr(g, aopright, "op", nil, "right", nil);
	yyrulestr(g, aoptoken, "op", nil, "token", nil);
}

/*
 * Second-level - create AST from user-specified grammar.
 */
static int line2;

static int
action2(Ygram *g, Yrule *r, void *vout, void *vin)
{
	Pval *out=vout, *in=vin;
	int i;
	
	USED(g);
	out->ast = mkast(r->left, r->nright);
	for(i=0; i<r->nright; i++){
		(*out->ast)->right[i] = in[i].ast;
		(*in[i].ast)->up = out->ast;
	}
	return 0;
}

AST**
mkast(Ysym *sym, int nright)
{
	AST **p;
	AST *a;
	
	/*
	 * Allocate space as well as a pointer to it.
	 * This allows the merge function to swap the
	 * pointer to swap two already-pointed-at ASTs.
	 * This is very dodgy and could be avoided by 
	 * making the GLR routines a little smarter.
	 */
	p = malloc(sizeof a+(sizeof *a)+(nright+1)*sizeof a->right[0]);
	a = (AST*)(p+1);
	*p = a;
	memset(a, 0, sizeof *a);
	a->nright = nright;
	a->right = (void*)(a+1);
	a->sym = sym;
	return p;
}

static void
merge(void *va, void *vb)
{
	Pval *a=va, *b=vb;
	AST **ast, *tmp;
	
	/*
	 * we need to overwrite the data pointed to by a,
	 * because a might have been copied already.
	 * all this stupid double-indirection could go away
	 * if glr-parse sorted things properly, but this was
	 * easier for the time being.  there is a comment 
	 * in glr-parse.c explaining.
	 */
	ast = mkast(nil, 2);

	/* swap data pointers - now ast is a copy of a */
	tmp = *ast;
	*ast = *a->ast;
	*a->ast = tmp;

	(*a->ast)->right[0] = ast;
	(*a->ast)->right[1] = b->ast;
	(*ast)->up = a->ast;
	(*b->ast)->up = a->ast;
}

static Ysym*
yylex2(Ygram *g, void *yys, void *arg)
{
	char *op, *opi, *s, **pp;
	int l;
	Ysym *sym;
	Pval *out;

	pp = arg;
	out = yys;
	opi = *pp;
again:
	op = *pp;
	if(**pp == 0){
		lastbit = opi;
		return yylooksym(g, "EOF");
	}
	if((sym = rundfa(dfa2, *pp, pp)) == nil)
		sysfatal("input stuck on: %s", *pp);
//print("%.*s #", utfnlen(op, *pp-op), op);
	if(hashget(ignores2, sym))
		goto again;
	if(*pp == op)
		sysfatal("parsed empty string");
	l = *pp-op;
	s = malloc(l+1);
	memmove(s, op, l);
	s[l] = 0;
	out->s = s;
	out->ast = mkast(sym, 0);
	(*out->ast)->sym = sym;
	(*out->ast)->s = s;
	if(opi != op){
		(*out->ast)->pre = malloc(op-opi+1);
		memmove((*out->ast)->pre, opi, op-opi);
		(*out->ast)->pre[op-opi] = 0;
	}
	for(s=opi; s<*pp; s++)
		if(*s == '\n')
			line2++;
	out->yaccsym = sym;
	return sym;
}

/*
 * Main driver
 */
void
loadgrammar(char *file)
{
	char *a;
	
	a = readfile(file);
	parsegrammar(a);
}

void
parsegrammar(char *a)
{
	static int did;
	
	if(!did){
		did++;
		dfa1 = mkdfa();
		gram1 = yynewgram();
	//	gram1->debug = 1;
		parsergrammar(gram1, dfa1);
		
		dfa2 = mkdfa();
		gram2 = yynewgram();
		gram2->valsize = sizeof(Pval);
		names2 = mkhash();
		literals2 = mkhash();
		regexps2 = mkhash();
		ignores2 = mkhash();
	}

	if(yyparse(gram1, "prog", nil, yylex1, &a) == nil)
		sysfatal("parsing grammar: %r");
}

AST*
parsefile(char *file)
{
	Yparse *p;
	char *a;
	Pval *pv;
	AST *ast;

	p = yynewparse();
	p->merge = merge;
	a = readfile(file);
	lastbit = nil;
	line2 = 1;
	if((pv = yyparse(gram2, start2 ? start2->name : nil, p, yylex2, &a)) == nil){
		fprint(2, "%s:%d: parse error\n", file, line2);
		return nil;
	}
	ast = *pv->ast;
	if(lastbit && ast)
		ast->post = lastbit;
	return ast;
}

char*
readfile(char *name)
{
	int fd;
	char *buf;
	int n, tot;
	
	if((fd = open(name, OREAD)) < 0)
		sysfatal("open %s: %r", name);
	tot = 0;
	buf = malloc(8192+1);
	while((n = read(fd, buf+tot, 8192)) > 0){
		tot += n;
		buf = realloc(buf, tot+8192+1);
	}
	buf[tot] = 0;
	close(fd);
	return buf;
}

int
astfmt(Fmt *fmt)
{
	int i;
	AST *a;
	
	a = va_arg(fmt->args, AST*);
	if(a == nil)
		return fmtprint(fmt, "Z");
	if(a->pre)
		fmtprint(fmt, "%s", a->pre);
	if(a->s)
		fmtprint(fmt, "%s", a->s);
	else{
		if(a->sym == nil)	/* merge */
			fmtprint(fmt, "%A", *a->right[0]);
		else{
			for(i=0; i<a->nright; i++)
				fmtprint(fmt, "%A", *a->right[i]);
		}
	}
	if(a->post)
		fmtprint(fmt, "%s", a->post);
	return 0;
}

void
yyerror(Ygram *g, Yparse *p, char *msg)
{
	USED(g);
	USED(p);
	sysfatal("yyerror: %s", msg);
}

#if 0
typedef struct Input Input;
typedef struct Gval Gval;
typedef struct Ytype2 Ytype2;
typedef struct AST AST;
typedef struct Type Type;

struct Input
{
	Biobuf *b;
	char *p;
};

struct Gval
{
	Ysym *yaccsym;	/* must be first - known to yacc.c */
	
	int op;	/* Left, Right, Token */
	int flag;
	Ysym *prec;
	int assoc;
	Name x;	/* name, literal, regexp */
	Ysym *sym;
	Alist al;
};

struct Ytype2
{
	Ysym *yaccsym;	/* must be first - known to yacc.c */
	char *s;
	char *out;
	AST **ast;
};

struct AST
{
	Ysym *sym;
	char *pre;
	char *post;
	char *s;
	AST ***right;
	int nright;
	Type *type;
};

/*
 * Grammar for grammar files.
 */

/*
 * Second grammar: created by reading the grammar file,
 * then used for parsing the input file.
 *
 * Grep for gram2 above to find where rules and symbols get added.
 */

void _printast(AST*, int, char*);
void
printast(AST *ast)
{
	_printast(ast, 0, "");
	print("\n");
}
void
_printast(AST *ast, int indent, char *lead)
{
	int i;

	if(ast == nil)
		return;
	if(ast->s){
		print("%s%*s'%s", lead, indent*2, "", ast->s);
		return;
	}
	print("%s%*s(%s", lead, indent*2, "", ast->sym ? ast->sym->name : "<merge>", ast);
	if(ast->s){
		print(" %s)", ast->s);
		return;
	}
	if(ast->nright == 0){
		print(")");
		return;
	}
	for(i=0; i<ast->nright; i++)
		_printast(*ast->right[i], indent+1, "\n");
	print(")");
}


#endif
