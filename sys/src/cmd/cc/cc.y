%{
#include "cc.h"
%}
%union	{
	Node*	node;
	Sym*	sym;
	Ref*	ref;
	Type*	type;
	struct {
		Type*	t;
		char	c;
	} tycl;
	struct {
		Type*	t1;
		Type*	t2;
	} tyty;
	struct {
		Sym*	sym;
		Ref*	ref;
	} syrf;
	long	lval;
	double	dval;
	char*	sval;
	ushort*	rval;
}
%type	<syrf>	ltag
%type	<lval>	tname tnlist
%type	<type>	tlist sbody complex
%type	<tycl>	types
%type	<node>	zarglist arglist zcexpr
%type	<node>	name block stmnt cexpr expr xuexpr pexpr
%type	<node>	zelist elist adecl slist uexpr
%type	<node>	xdecor xdecor2 labels label ulstmnt
%type	<node>	adlist edecor tag qual
%type	<node>	abdecor abdecor1 abdecor2 abdecor3
%type	<node>	zexpr lexpr init ilist lcexpr

%left	';'
%left	','
%right	'=' LPE LME LMLE LDVE LMDE LRSHE LLSHE LANDE LXORE LORE
%right	'?' ':'
%left	LOROR
%left	LANDAND
%left	'|'
%left	'^'
%left	'&'
%left	LEQ LNE
%left	'<' '>' LLE LGE
%left	LLSH LRSH
%left	'+' '-'
%left	'*' '/' '%'
%right	LMM LPP LMG '.' '[' '('

%token	<sym>	LNAME LTYPE
%token	<dval>	LFCONST LVLCONST LDCONST
%token	<lval>	LCONST LLCONST LUCONST LULCONST
%token	<sval>	LSTRING
%token	<rval>	LLSTRING
%token		LAUTO LBREAK LCASE LCHAR LCONTINUE LDEFAULT LDO
%token		LDOUBLE LELSE LEXTERN LFLOAT LFOR LGOTO
%token	LIF LINT LLONG LREGISTER LRETURN LSHORT LSIZEOF LUSED
%token	LSTATIC LSTRUCT LSWITCH LTYPEDEF LUNION LUNSIGNED LWHILE
%token	LVOID LENUM LSIGNED LCONSTNT LVOLATILE LSET
%%
prog:
|	prog xdecl

/*
 * external declarator
 */
xdecl:
	zctlist ';'
	{
		dodecl(xdecl, lastclass, lasttype, Z);
	}
|	zctlist xdlist ';'
|	zctlist xdecor
	{
		lastdcl = T;
		firstarg = S;
		dodecl(xdecl, lastclass, lasttype, $2);
		if(lastdcl == T || lastdcl->etype != TFUNC) {
			diag($2, "not a function");
			lastdcl = types[TFUNC];
		}
		thisfn = lastdcl;
		markdcl();
		firstdcl = dclstack;
		argmark($2, 0);
	}
	pdecl
	{
		argmark($2, 1);
	}
	block
	{
		revertdcl();
		if(!debug['H'])
			codgen($6, $2);
	}

xdlist:
	xdecor
	{
		dodecl(xdecl, lastclass, lasttype, $1);
	}
|	xdecor
	{
		$1 = dodecl(xdecl, lastclass, lasttype, $1);	/* dirty use of $1 */
	}
	'=' init
	{
		doinit($1->sym, $1->type, 0L, $4);
	}
|	xdlist ',' xdlist

xdecor:
	xdecor2
|	'*' garbage xdecor
	{
		$$ = new(OIND, $3, Z);
	}

xdecor2:
	tag
|	'(' xdecor ')'
	{
		$$ = $2;
	}
|	xdecor2 '(' zarglist ')'
	{
		$$ = new(OFUNC, $1, $3);
	}
|	xdecor2 '[' zexpr ']'
	{
		$$ = new(OARRAY, $1, $3);
	}

/*
 * automatic declarator
 */
adecl:
	{
		$$ = Z;
	}
|	adecl ctlist ';'
	{
		$$ = dodecl(adecl, lastclass, lasttype, Z);
		if($1 != Z)
			if($$ != Z)
				$$ = new(OLIST, $1, $$);
			else
				$$ = $1;
	}
|	adecl ctlist adlist ';'
	{
		$$ = $1;
		if($3 != Z) {
			$$ = $3;
			if($1 != Z)
				$$ = new(OLIST, $1, $3);
		}
	}

adlist:
	xdecor
	{
		dodecl(adecl, lastclass, lasttype, $1);
		$$ = Z;
	}
|	xdecor
	{
		$1 = dodecl(adecl, lastclass, lasttype, $1);
	}
	'=' init
	{
		long w;

		w = $1->sym->type->width;
		$$ = doinit($1->sym, $1->type, 0L, $4);
		$$ = contig($1->sym, $$, w);
	}
|	adlist ',' adlist
	{
		$$ = $1;
		if($3 != Z) {
			$$ = $3;
			if($1 != Z)
				$$ = new(OLIST, $1, $3);
		}
	}

/*
 * parameter declarator
 */
pdecl:
|	pdecl ctlist pdlist ';'

pdlist:
	xdecor
	{
		dodecl(pdecl, lastclass, lasttype, $1);
	}
|	pdlist ',' pdlist

/*
 * structure element declarator
 */
edecl:
	tlist
	{
		lasttype = $1;
	}
	zedlist ';'
|	edecl tlist
	{
		lasttype = $2;
	}
	zedlist ';'

zedlist:					/* extension */
	{
		edecl(CXXX, lasttype, S);
	}
|	edlist

edlist:
	edecor
	{
		dodecl(edecl, CXXX, lasttype, $1);
	}
|	edlist ',' edlist

edecor:
	xdecor
	{
		lastbit = 0;
		firstbit = 1;
	}
|	tag ':' lexpr
	{
		$$ = new(OBIT, $1, $3);
	}
|	':' lexpr
	{
		$$ = new(OBIT, Z, $2);
	}

/*
 * abstract declarator
 */
abdecor:
	{
		$$ = (Z);
	}
|	abdecor1

abdecor1:
	'*' garbage
	{
		$$ = new(OIND, (Z), Z);
	}
|	'*' garbage abdecor1
	{
		$$ = new(OIND, $3, Z);
	}
|	abdecor2

abdecor2:
	abdecor3
|	abdecor2 '(' zarglist ')'
	{
		$$ = new(OFUNC, $1, $3);
	}
|	abdecor2 '[' zexpr ']'
	{
		$$ = new(OARRAY, $1, $3);
	}

abdecor3:
	'(' ')'
	{
		$$ = new(OFUNC, (Z), Z);
	}
|	'[' zexpr ']'
	{
		$$ = new(OARRAY, (Z), $2);
	}
|	'(' abdecor1 ')'
	{
		$$ = $2;
	}

init:
	expr
|	'{' ilist '}'
	{
		$$ = new(OINIT, invert($2), Z);
	}

qual:
	'[' lexpr ']'
	{
		$$ = new(OARRAY, $2, Z);
	}
|	'.' tag
	{
		$$ = new(ODOT, $2, Z);
	}
|	'[' lexpr ']' qual
	{
		$$ = new(OARRAY, $2, Z);
		$$ = new(OLIST, $$, $4);
	}
|	'.' tag qual
	{
		$$ = new(ODOT, $2, Z);
		$$ = new(OLIST, $$, $3);
	}

ilist:
	init
|	qual init
	{
		$$ = new(OLIST, $1, $2);
	}
|	qual '=' init
	{
		$$ = new(OLIST, $1, $3);
	}
|	ilist ',' init
	{
		$$ = new(OLIST, $1, $3);
	}
|	ilist ',' qual init
	{
		$$ = new(OLIST, $1, $3);
		$$ = new(OLIST, $$, $4);
	}
|	ilist ',' qual '=' init
	{
		$$ = new(OLIST, $1, $3);
		$$ = new(OLIST, $$, $5);
	}
|	ilist ','

zarglist:
	{
		$$ = Z;
	}
|	arglist
	{
		$$ = invert($1);
	}


arglist:
	name
|	tlist abdecor
	{
		$$ = new(OPROTO, $2, Z);
		$$->type = $1;
	}
|	tlist xdecor
	{
		$$ = new(OPROTO, $2, Z);
		$$->type = $1;
	}
|	'.' '.' '.'
	{
		$$ = new(ODOTDOT, Z, Z);
	}
|	arglist ',' arglist
	{
		$$ = new(OLIST, $1, $3);
	}

block:
	'{' adecl slist '}'
	{
		$$ = invert($3);
		if($2 != Z)
			$$ = new(OLIST, $2, $$);
	}

slist:
	{
		$$ = Z;
	}
|	slist stmnt
	{
		$$ = new(OLIST, $1, $2);
	}

labels:
	label
|	labels label
	{
		$$ = new(OLIST, $1, $2);
	}

label:
	LCASE expr ':'
	{
		$$ = new(OCASE, $2, Z);
	}
|	LDEFAULT ':'
	{
		$$ = new(OCASE, Z, Z);
	}
|	LNAME ':'
	{
		$$ = new(OLABEL, dcllabel($1, 1), Z);
		if($1->ref)
			$1->ref->class = CLABEL+CLAST;
	}

stmnt:
	error ';'
	{
		$$ = Z;
	}
|	ulstmnt
|	labels ulstmnt
	{
		$$ = new(OLIST, $1, $2);
	}

ulstmnt:
	zcexpr ';'
|	{
		markdcl();
	}
	block
	{
		revertdcl();
		$$ = $2;
	}
|	LIF '(' cexpr ')' stmnt
	{
		$$ = new(OIF, $3, new(OLIST, $5, Z));
	}
|	LIF '(' cexpr ')' stmnt LELSE stmnt
	{
		$$ = new(OIF, $3, new(OLIST, $5, $7));
	}
|	LFOR '(' zcexpr ';' zcexpr ';' zcexpr ')' stmnt
	{
		$$ = new(OFOR, new(OLIST, $5, new(OLIST, $3, $7)), $9);
	}
|	LWHILE '(' cexpr ')' stmnt
	{
		$$ = new(OWHILE, $3, $5);
	}
|	LDO stmnt LWHILE '(' cexpr ')' ';'
	{
		$$ = new(ODWHILE, $5, $2);
	}
|	LRETURN zcexpr ';'
	{
		$$ = new(ORETURN, $2, Z);
		$$->type = thisfn->link;
	}
|	LSWITCH '(' lcexpr ')' stmnt
	{
		$$ = new(OSWITCH, $3, $5);
	}
|	LBREAK ';'
	{
		$$ = new(OBREAK, Z, Z);
	}
|	LCONTINUE ';'
	{
		$$ = new(OCONTINUE, Z, Z);
	}
|	LGOTO LNAME ';'
	{
		$$ = new(OGOTO, dcllabel($2, 0), Z);
		if($2->ref)
			$2->ref->class = CLABEL;
	}
|	LUSED '(' zelist ')' ';'
	{
		$$ = new(OUSED, $3, Z);
	}
|	LSET '(' zelist ')' ';'
	{
		$$ = new(OSET, $3, Z);
	}

zcexpr:
	{
		$$ = Z;
	}
|	cexpr

zexpr:
	{
		$$ = Z;
	}
|	lexpr

lexpr:
	expr
	{
		$$ = new(OCAST, $1, Z);
		$$->type = types[TLONG];
	}

lcexpr:
	cexpr
	{
		$$ = new(OCAST, $1, Z);
		$$->type = types[TLONG];
	}

cexpr:
	expr
|	cexpr ',' cexpr
	{
		$$ = new(OCOMMA, $1, $3);
	}

expr:
	xuexpr
|	expr '*' expr
	{
		$$ = new(OMUL, $1, $3);
	}
|	expr '/' expr
	{
		$$ = new(ODIV, $1, $3);
	}
|	expr '%' expr
	{
		$$ = new(OMOD, $1, $3);
	}
|	expr '+' expr
	{
		$$ = new(OADD, $1, $3);
	}
|	expr '-' expr
	{
		$$ = new(OSUB, $1, $3);
	}
|	expr LRSH expr
	{
		$$ = new(OASHR, $1, $3);
	}
|	expr LLSH expr
	{
		$$ = new(OASHL, $1, $3);
	}
|	expr '<' expr
	{
		$$ = new(OLT, $1, $3);
	}
|	expr '>' expr
	{
		$$ = new(OGT, $1, $3);
	}
|	expr LLE expr
	{
		$$ = new(OLE, $1, $3);
	}
|	expr LGE expr
	{
		$$ = new(OGE, $1, $3);
	}
|	expr LEQ expr
	{
		$$ = new(OEQ, $1, $3);
	}
|	expr LNE expr
	{
		$$ = new(ONE, $1, $3);
	}
|	expr '&' expr
	{
		$$ = new(OAND, $1, $3);
	}
|	expr '^' expr
	{
		$$ = new(OXOR, $1, $3);
	}
|	expr '|' expr
	{
		$$ = new(OOR, $1, $3);
	}
|	expr LANDAND expr
	{
		$$ = new(OANDAND, $1, $3);
	}
|	expr LOROR expr
	{
		$$ = new(OOROR, $1, $3);
	}
|	expr '?' cexpr ':' expr
	{
		$$ = new(OCOND, $1, new(OLIST, $3, $5));
	}
|	expr '=' expr
	{
		$$ = new(OAS, $1, $3);
	}
|	expr LPE expr
	{
		$$ = new(OASADD, $1, $3);
	}
|	expr LME expr
	{
		$$ = new(OASSUB, $1, $3);
	}
|	expr LMLE expr
	{
		$$ = new(OASMUL, $1, $3);
	}
|	expr LDVE expr
	{
		$$ = new(OASDIV, $1, $3);
	}
|	expr LMDE expr
	{
		$$ = new(OASMOD, $1, $3);
	}
|	expr LLSHE expr
	{
		$$ = new(OASASHL, $1, $3);
	}
|	expr LRSHE expr
	{
		$$ = new(OASASHR, $1, $3);
	}
|	expr LANDE expr
	{
		$$ = new(OASAND, $1, $3);
	}
|	expr LXORE expr
	{
		$$ = new(OASXOR, $1, $3);
	}
|	expr LORE expr
	{
		$$ = new(OASOR, $1, $3);
	}

xuexpr:
	uexpr
|	'(' tlist abdecor ')' xuexpr
	{
		$$ = new(OCAST, $5, Z);
		dodecl(NODECL, CXXX, $2, $3);
		$$->type = lastdcl;
	}
|	'(' tlist abdecor ')' '{' ilist '}'	/* extension */
	{
		$$ = new(OSTRUCT, $6, Z);
		dodecl(NODECL, CXXX, $2, $3);
		$$->type = lastdcl;
	}

uexpr:
	pexpr
|	'*' xuexpr
	{
		$$ = new(OIND, $2, Z);
	}
|	'&' xuexpr
	{
		$$ = new(OADDR, $2, Z);
	}
|	'+' xuexpr
	{
		$$ = new(OCONST, Z, Z);
		$$->offset = 0;
		$$->type = tint;
		$2 = new(OSUB, $$, $2);

		$$ = new(OCONST, Z, Z);
		$$->offset = 0;
		$$->type = tint;
		$$ = new(OSUB, $$, $2);
	}
|	'-' xuexpr
	{
		$$ = new(OCONST, Z, Z);
		$$->offset = 0;
		$$->type = tint;
		$$ = new(OSUB, $$, $2);
	}
|	'!' xuexpr
	{
		$$ = new(ONOT, $2, Z);
	}
|	'~' xuexpr
	{
		$$ = new(OCONST, Z, Z);
		$$->offset = -1;
		$$->type = tint;
		$$ = new(OXOR, $$, $2);
	}
|	LPP xuexpr
	{
		$$ = new(OPREINC, $2, Z);
	}
|	LMM xuexpr
	{
		$$ = new(OPREDEC, $2, Z);
	}
|	LSIZEOF uexpr
	{
		$$ = new(OSIZE, $2, Z);
	}

pexpr:
	'(' cexpr ')'
	{
		$$ = $2;
	}
|	LSIZEOF '(' tlist abdecor ')'
	{
		$$ = new(OSIZE, Z, Z);
		dodecl(NODECL, CXXX, $3, $4);
		$$->type = lastdcl;
	}
|	pexpr '(' zelist ')'
	{
		$$ = new(OFUNC, $1, Z);
		if($1->op == ONAME)
		if($1->type == T)
			dodecl(xdecl, CXXX, tint, $$);
		$$->right = invert($3);
	}
|	pexpr '[' cexpr ']'
	{
		$$ = new(OIND, new(OADD, $1, $3), Z);
	}
|	pexpr LMG ltag
	{
		$$ = new(ODOT, new(OIND, $1, Z), Z);
		$$->sym = $3.sym;
		if($3.ref) {
			$3.ref->class = CSELEM;
			$$->ref = $3.ref;
		}
	}
|	pexpr '.' ltag
	{
		$$ = new(ODOT, $1, Z);
		$$->sym = $3.sym;
		if($3.ref) {
			$3.ref->class = CSELEM;
			$$->ref = $3.ref;
		}
	}
|	pexpr LPP
	{
		$$ = new(OPOSTINC, $1, Z);
	}
|	pexpr LMM
	{
		$$ = new(OPOSTDEC, $1, Z);
	}
|	name
|	LCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = tint;
		$$->offset = $1;
	}
|	LLCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = types[TLONG];
		$$->offset = $1;
	}
|	LUCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = tuint;
		$$->offset = $1;
	}
|	LULCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = types[TULONG];
		$$->offset = $1;
	}
|	LDCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = types[TDOUBLE];
		$$->ud = $1;
	}
|	LFCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = types[TFLOAT];
		$$->ud = $1;
	}
|	LVLCONST
	{
		$$ = new(OCONST, Z, Z);
		$$->type = types[TVLONG];
		$$->ud = $1;
	}
|	LSTRING
	{
		$$ = new(OSTRING, Z, Z);
		$$->us = $1;
		$$->sym = symstring;
		$$->type = typ(TARRAY, types[TCHAR]);
		$$->etype = TARRAY;
		$$->type->width = lnstring;
		$$->class = CSTATIC;
	}
|	LLSTRING
	{
		$$ = new(OLSTRING, Z, Z);
		$$->rs = $1;
		$$->sym = symstring;
		$$->type = typ(TARRAY, types[TUSHORT]);
		$$->etype = TARRAY;
		$$->type->width = lnstring;
		$$->class = CSTATIC;
	}

zelist:
	{
		$$ = Z;
	}
|	elist

elist:
	expr
|	elist ',' elist
	{
		$$ = new(OLIST, $1, $3);
	}

sbody:
	'{'
	{
		$<tyty>$.t1 = strf;
		$<tyty>$.t2 = strl;
		strf = T;
		strl = T;
		lastbit = 0;
		firstbit = 1;
	}
	edecl '}'
	{
		$$ = strf;
		strf = $<tyty>2.t1;
		strl = $<tyty>2.t2;
	}

zctlist:
	{
		lastclass = CXXX;
		lasttype = tint;
	}
|	ctlist

types:
	complex
	{
		$$.t = $1;
		$$.c = CXXX;
	}
|	complex tnlist
	{
		$$.t = $1;
		$$.c = simplec($2);
		if($2 & ~BCLASS)
			diag(Z, "illegal combination of types 1: %Q/%T", $2, $1);
	}
|	tnlist
	{
		$$.t = simplet($1);
		$$.c = simplec($1);
	}
|	tnlist complex
	{
		$$.t = $2;
		$$.c = simplec($1);
		if($1 & ~BCLASS)
			diag(Z, "illegal combination of types 1: %Q/%T", $1, $2);
	}
|	tnlist complex tnlist
	{
		$$.t = $2;
		$$.c = simplec($1|$3);
		if(($1|$3) & ~BCLASS)
			diag(Z, "illegal combination of types 1: %Q/%T", $1|$3, $2);
	}

tlist:
	types
	{
		$$ = $1.t;
		if($1.c != CXXX)
			diag(Z, "illegal combination of class 3: %s", cnames[$1.c]);
	}

ctlist:
	types
	{
		lasttype = $1.t;
		lastclass = $1.c;
	}

complex:
	LSTRUCT ltag
	{
		dotag($2.sym, TSTRUCT, 0);
		$$ = $2.sym->suetag;
		if($2.ref)
			$2.ref->class = CSUETAG;
	}
|	LSTRUCT ltag
	{
		dotag($2.sym, TSTRUCT, autobn);
		if($2.ref)
			$2.ref->class = CSUETAG+CLAST;
	}
	sbody
	{
		$$ = $2.sym->suetag;
		if($$->link != T)
			diag(Z, "redeclare tag: %s", $2.sym->name);
		$$->link = $4;
		suallign($$);
		dbgprint($2.sym, $$);
	}
|	LSTRUCT sbody
	{
		$$ = typ(TSTRUCT, $2);
		suallign($$);
	}
|	LUNION ltag
	{
		dotag($2.sym, TUNION, 0);
		$$ = $2.sym->suetag;
		if($2.ref)
			$2.ref->class = CSUETAG;
	}
|	LUNION ltag
	{
		dotag($2.sym, TUNION, autobn);
		if($2.ref)
			$2.ref->class = CSUETAG+CLAST;
	}
	sbody
	{
		$$ = $2.sym->suetag;
		if($$->link != T)
			diag(Z, "redeclare tag: %s", $2.sym->name);
		$$->link = $4;
		suallign($$);
		dbgprint($2.sym, $$);
	}
|	LUNION sbody
	{
		$$ = typ(TUNION, $2);
		suallign($$);
	}
|	LENUM ltag
	{
		dotag($2.sym, TENUM, 0);
		$$ = $2.sym->suetag;
		if($$->link == T)
			$$->link = tint;
		$$ = $$->link;
		if($2.ref)
			$2.ref->class = CSUETAG;
	}
|	LENUM ltag
	{
		dotag($2.sym, TENUM, autobn);
		if($2.ref)
			$2.ref->class = CSUETAG+CLAST;
	}
	'{'
	{
		lastenum = 0;
		maxenum = 0;
	}
	enum '}'
	{
		$$ = $2.sym->suetag;
		if($$->link != T)
			diag(Z, "redeclare tag: %s", $2.sym->name);
		$$->link = maxtype(maxenum);
		$$ = $$->link;
	}
|	LENUM '{'
	{
		lastenum = 0;
		maxenum = 0;
	}
	enum '}'
	{
		$$ = maxtype(maxenum);
	}
|	LTYPE
	{
		$$ = tcopy($1->type);
		if($1->ref)
			$1->ref->class = CTYPEDEF;
	}

tnlist:
	tname
|	tnlist tname
	{
		$$ = $1 | $2;
		if($1 & $2)
			if(($1 & $2) == BLONG)
				$$ |= BVLONG;		/* long long => vlong */
			else
				diag(Z, "once is enough: %Q", $1 & $2);
	}


enum:
	LNAME
	{
		doenum($1, Z);
		if($1->ref) {
			$1->ref->class = CENUM+CLAST;
			$1->varlineno = $1->ref->lineno;
		}
	}
|	LNAME
	{
		if($1->ref) {
			$1->ref->class = CENUM+CLAST;
			$1->varlineno = $1->ref->lineno;
		}
	}
	'=' expr
	{
		doenum($1, $4);
	}
|	enum ','
|	enum ',' enum

tname:
	LCHAR { $$ = BCHAR; }	/* type words */
|	LSHORT { $$ = BSHORT; }
|	LINT { $$ = BINT; }
|	LLONG { $$ = BLONG; }
|	LSIGNED { $$ = BSIGNED; }
|	LUNSIGNED { $$ = BUNSIGNED; }
|	LFLOAT { $$ = BFLOAT; }
|	LDOUBLE { $$ = BDOUBLE; }
|	LVOID { $$ = BVOID; }

|	LAUTO { $$ = BAUTO; }	/* class words */
|	LSTATIC { $$ = BSTATIC; }
|	LEXTERN { $$ = BEXTERN; }
|	LTYPEDEF { $$ = BTYPEDEF; }
|	LREGISTER { $$ = BREGISTER; }

|	LCONSTNT { $$ = 0; }	/* noise words */
|	LVOLATILE { $$ = 0; }

garbage:
|	garbage LCONSTNT
|	garbage LVOLATILE

name:
	LNAME
	{
		$$ = new(ONAME, Z, Z);
		if($1->class == CLOCAL)
			$1 = mkstatic($1);
		$$->sym = $1;
		$$->type = $1->type;
		$$->etype = TVOID;
		if($$->type != T)
			$$->etype = $$->type->etype;
		$$->offset = $1->offset;
		$$->class = $1->class;
		$$->ref = $1->ref;
		$1->aused = 1;
		if($$->ref) {
			$$->ref->class = $$->class;
			$$->ref->dlineno = $1->varlineno;
		}
	}
tag:
	ltag
	{
		$$ = new(ONAME, Z, Z);
		$$->sym = $1.sym;
		$$->type = $1.sym->type;
		$$->etype = TVOID;
		if($$->type != T)
			$$->etype = $$->type->etype;
		$$->offset = $1.sym->offset;
		$$->class = $1.sym->class;
		$$->ref = $1.ref;
	}
ltag:
	LNAME
	{
		$$.sym = $1;
		$$.ref = $1->ref;
	}
|	LTYPE
	{
		$$.sym = $1;
		$$.ref = $1->ref;
	}
%%
