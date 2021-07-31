%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#define Extern extern
#include "globl.h"
%}

%union
{
	Node	*node;
	Sym	*sym;
	Type	*type;
	ulong	ival;
	double	fval;
	int	clas;
	String	*string;
	Tysym	ltype;
}

%type <node> zinit ivardecl zelist prog decllist decl vardecllist vardecl ztag
%type <node> arglist arrayspec indsp block slist stmnt zexpr 
%type <node> term zarlist elist typespec memberlist name autolist 
%type <node> arg bufdim clist case rbody zlab sname adtfunc nlstmnt
%type <node> cbody expr castexpr monexpr autodecl setlist zargs
%type <node> telist tcomp ztelist tuplearg ctlist tcase tbody
%type <node> zpolytype polytype info

%type <type> typecast xtname tname tlist ztname variant
%type <clas> sclass
%type <ival> zconst
%type <sym>  stag

%left	';'
%left	','
%right	'=' Tvasgn Taddeq Tsubeq Tmuleq Tdiveq Tmodeq Trsheq Tlsheq Tandeq Toreq Txoreq 
%right	'?'
%left	Toror
%left	Tandand
%left	'|'
%left	'^'
%left	'&'
%left	Teq Tneq
%left	'<' '>' Tleq Tgeq
%right	Titer
%left	Tlsh Trsh
%left	'+' '-'
%left	'*' '/' '%'
%right	Tdec Tinc Tcomm Tindir '.' '[' '('

%token <ltype>	Ttypename
%token <sym>	Tid
%token <ival>	Tconst
%token <ival>	Tsname
%token <fval>	Tfconst
%token <string>	Tstring

%token Tint Tuint Tsint Tsuint Tuchar Tchar Tfloat Tchannel Tvoid Tcase Tcheck
%token Tbreak Taggregate Tunion Tstorage Treturn Tswitch Twhile Tfor Tpar Telse 
%token Tandand Tdo Tcomm Toror Tneq Teq Tadt Ttask Tprocess Tselect Tif Traise
%token Tleq Tgeq Tindir Tlsh Trsh Tset Tsname Trescue Tintern Textern Tdefault
%token Taddeq Tsubeq Tmuleq Tdiveq Tmodeq Trsheq Tlsheq Tcontinue Tnewtype
%token Tandeq Toreq Txoreq Tgoto Tdec Tinc Tnil Talloc Tunalloc Tid
%token Tguard Tprivate Ttuple Tbecome Titer Tzerox Tvasgn Ttypeof

%%
prog:		decllist
		{
			polyasgn();
			packdepend();
			strop();
		}
		;

decllist	:
		{ $$ = nil; }
		| decllist decl
		{
			$$ = an(OLIST, $1, $2);
			pushdcl($2, Global);
			gendata($2);
		}
		;

decl		: tname vardecllist ';'
		{
			$$ = simpledecl($1, $2);
		}
		| tname vardecl '(' arglist ')'
		{
			fundecl($1, $2, $4);
		}
		block
		{
			fungen($7, $2);
			$$ = $2->left;
		}
		| tname adtfunc '(' arglist ')'
		{
			if($1->class != External && $1->class != Global)
				diag($2, "%s illegal storage class for adt function",
						sclass[$1->class]);
			fundecl($1, $2, $4);
			adtchk($2->init, $2);
			$4->init = nil;
		}
		block
		{
			fungen($7, $2);
			adtbfun = nil;
			$$ = $2->left;
		}
		| tname vardecl '(' arglist ')' ';'
		{
			applytype($1, $2);
			funproto($2, $4);
			$$ = $2;
		}
		| typespec ';'
		{
			if($1)
				buildtype($1);
		}
		| Tnewtype ztname vardecl zargs ';'
		{
			newtype($2, $3, $4);
			$$ = nil;
		}
		| Tnewtype Tid ';'
		{
			$2->lexval = Ttypename;
			$2->ltype = at(TPOLY, nil);
			$2->ltype->sym = $2;
			$$ = nil;
		}
		;

zargs		:
		{ $$ = nil; }
		| '(' arglist ')'
		{ $$ = $2; }
		;

ztname		: tname
		| Taggregate
		{ $$ = nil; }
		| Tadt
		{ $$ = nil; }
		| Tunion
		{ $$ = nil; }
		;

adtfunc		: Ttypename '.' name
		{
			$3->init = adtfunc($1, $3);
			$$ = $3;
		}
		| indsp Ttypename '.' name
		{
			$4->init = adtfunc($2, $4);
			$4->right = $1;
			$$ = $4;
		}
		;

typespec	: Taggregate ztag
		{
			if($2)
				$2->sym->lexval = Ttypename;
		} 
		'{' memberlist '}' ztag
		{
			$$ = an(OAGDECL, an(OLIST, $2, $7), $5);
		}
		| Tunion ztag
		{
			if($2)
				$2->sym->lexval = Ttypename; 
		} 
		'{' memberlist  '}' ztag
		{
			$$ = an(OUNDECL, an(OLIST, $2, $7), $5);
		}
		| Tadt ztag zpolytype
		{
			if($2) {
				$2->sym->lexval = Ttypename;
				$2->right = buildadt;
				buildadt = $2;
			}
			else
				diag(nil, "adt decl needs type name");
		} 
		'{' memberlist '}' ztag
		{
			if($2) {
				buildadt = $2->right;
				$2->right = nil;
				$$ = an(OADTDECL, an(OLIST, $2, $8), $6);
				$$->poly = $3;
			}
			else
				$$ = nil;
		}
		| Tset ztag
		{
			setbase = 0;
			if($2) {
				$2->sym->lexval = Ttypename;
				newtype(builtype[TINT], $2, nil);
			}
		}
		'{' setlist '}'
		{
			$$ = an(OSETDECL, nil, nil);
		}
		;

ztag		:
		{ $$ = nil; }
		| name
		| Ttypename
		{
			$$ = an(ONAME, nil, nil);
			$$->sym = $1.s;
		}
		;

zpolytype	:
		{ $$ = nil; }
		| '[' polytype ']'
		{
			$$ = $2;
		}
		;

polytype	: name
		{
			polytyp($1);
		}
		| polytype ',' polytype
		{
			$1->left = $3;
			$$ = $1;
		}
		;

setlist		: sname
		| setlist ',' setlist
		{
			$$ = an(OLIST, $1, $3);
		}
		;

sname		:
		{ $$ = nil; }
		| name
		{
			$1->sym->lexval = Tsname;
			coverset($1);
		}
		| name '=' expr
		{
			$1->sym->lexval = Tsname;
			$1->left = $3;
			coverset($1);
		}
		;

name		: Tid
		{
			$$ = an(ONAME, nil, nil);
			$$->sym = $1;
		}
		;

memberlist	: decl
		| memberlist decl
		{
			$$ = an(OLIST, $1, $2);
		}
		;

vardecllist	:
		{ $$ = nil; }
		| ivardecl
		| vardecllist ',' ivardecl
		{
			$$ = an(OLIST, $1, $3);
		}
		;

ivardecl	: vardecl zinit
		{
			$$->init = $2;
			typechk($2, 0);
			rewrite($2);
			$$ = $1;
		}
		;

zinit		:
		{ $$ = nil; }
		| '=' zelist
		{
			$$ = $2;
		}
		;

zelist		: zexpr
		| '[' expr ']' expr
		{
			$$ = an(OINDEX, $2, $4);
		}
		| '.' stag expr
		{
			$$ = an(OINDEX, $3, nil);
			$$->sym = $2;
		}
		| '{' zelist '}'
		{
			$$ = an(OILIST, $2, nil);
		}
		| '[' expr ']' '{' zelist '}'
		{
			$$ = an(OINDEX, $2, an(OILIST, $5, nil));
		}
		| zelist ',' zelist
		{
			$$ = an(OLIST, $1, $3);
		}
		;

vardecl		: Tid arrayspec
		{
			$$ = an(ONAME, $2, nil);
			$$->sym = $1;
		}
		| indsp Tid arrayspec
		{
			$$ = an(ONAME, $3, $1);
			$$->sym = $2;
		}
		| '(' indsp Tid arrayspec ')' '(' arglist ')'
		{
			$$ = an(OFUNC, an(OLIST, $4, $2), nil);
			$$->sym = $3;
			$$->proto = $7;
		}
		| indsp '(' indsp Tid arrayspec ')' '(' arglist ')'
		{
			$$ = an(OFUNC, an(OLIST, $5, $3), $1);
			$$->sym = $4;
			$$->proto = $8;
		}
		;

arrayspec	:
		{ $$ = nil; }
		| arrayspec '[' zexpr ']'
		{
			$$ = an(OARRAY, $3, $1);
			typechk($3, 0);
			rewrite($3);
		}
		;

indsp		: '*'
		{
			$$ = an(OIND, nil, nil);
		}
		| indsp '*'
		{
			$$ = an(OIND, $1, nil);
		}
		;


arglist		:
		{
			$$ = an(OPROTO, nil, nil);
			$$->t = builtype[TVOID];
		}
		| arg
		| '*' xtname
		{
			$$ = an(OADTARG, nil, nil);
			$$->t = at(TIND, $2);
		}
		| '.' xtname
		{
			$$ = an(OADTARG, nil, nil);
			$$->t = $2;
		}
		| arglist ',' arg
		{
			$$ = an(OLIST, $1, $3);
		}
		;

arg		: xtname
		{
			$$ = an(OPROTO, nil, nil);
			$$->t = $1;
		}
		| xtname indsp arrayspec
		{
			$$ = argproto($1, $2, $3);
		}
		| xtname '(' indsp ')' '(' arglist ')'
		{
			$$= an(OPROTO, nil, nil);
			$$->t = $1;
			$$->t = at(TFUNC, $$->t);
			$$->t = mkcast($$->t, $3);
			$$->proto = $6;
		}
		| xtname indsp '(' indsp ')' '(' arglist ')'
		{
			$$= an(OPROTO, nil, nil);
			$$->t = mkcast($1, $2);
			$$->t = at(TFUNC, $$->t);
			$$->t = mkcast($$->t, $4);
			$$->proto = $7;
		}
		| Ttuple tuplearg
		{
			$$ = $2;
		}
		| xtname vardecl
		{
			applytype($1, $2);
			$$ = $2;
			if($$->t->type == TARRAY) {
				$$->t->type = TIND;
				$$->t->size = builtype[TIND]->size;
			}
		}
		| '.' '.' '.'
		{
			$$ = an(OVARARG, nil, nil);
		}
		;

tuplearg	: tname
		{
			$$ = an(OPROTO, nil, nil);
			$$->t = $1;
		}
		| tname '(' indsp ')' '(' arglist ')'
		{
			$$= an(OPROTO, nil, nil);
			$$->t = $1;
			$$->t = at(TFUNC, $$->t);
			$$->t = mkcast($$->t, $3);
			$$->proto = $6;
		}
		| tname vardecl
		{
			applytype($1, $2);
			$$ = $2;
			if($$->t->type == TARRAY) {
				$$->t->type = TIND;
				$$->t->size = builtype[TIND]->size;
			}
		}
		;

autolist	: 
		{ $$ = nil; }
		| autolist autodecl
		{
			$$ = an(OLIST, $1, $2);
		}
		;

autodecl	: xtname vardecllist ';'
		{
			applytype($1, $2);
			pushdcl($2, Automatic);
			$$ = $2;
		}
		| Ttuple tname vardecllist ';'
		{
			applytype($2, $3);
			pushdcl($3, Automatic);
			$$ = $3;
		}
		;

block		: '{' 
		{ enterblock(); }
		autolist 
		{ scopeis(Automatic); }
		slist
		{ leaveblock(); }
		'}'
		{ $$ = an(OBLOCK, $5, nil); }
		| Tguard
		{ enterblock(); }
		autolist 
		{ scopeis(Automatic); }
		slist
		{ leaveblock(); }
		'}'
		{ $$ = an(OLBLOCK, $5, nil); }
		;

slist		:
		{ $$ = nil; }
		| slist stmnt
		{
			$$ = an(OLIST, $1, $2);
		}
		;

tbody		: '{' ctlist '}'
		{
			$$ = $2;
		}
		| Tguard clist '}'
		{
			$$ = an(OLBLOCK, $2, nil);
		}
		;

ctlist		:
		{ $$ = nil; }
		| ctlist tcase
		{
			$$ = an(OLIST, $1, $2);
		}
		;

tcase		: Tcase typecast { settype($2); } ':' slist
		{
			unsettype();
			$$ = an(OCASE, con(typesig($2)), $5);
		}
		| Tdefault ':' slist
		{
			$$ = an(ODEFAULT, nil, $3);
		}
		;

cbody		: '{' clist '}'
		{
			$$ = $2;
		}
		| Tguard clist '}'
		{
			$$ = an(OLBLOCK, $2, nil);
		}
		;

clist		:
		{ $$ = nil; }
		| clist case
		{
			$$ = an(OLIST, $1, $2);
		}
		;

case		: Tcase expr ':' slist
		{
			$$ = an(OCASE, $2, $4);
		}
		| Tdefault ':' slist
		{
			$$ = an(ODEFAULT, nil, $3);
		}
		;

rbody		: stmnt
		| Tid block
		{
			$$ = an(OLABEL, $2, nil);
			$$->sym = $1;
		}
		;

zlab		:
		{ $$ = nil; }
		| Tid
		{
			$$ = an(OLABEL, nil, nil);
			$$->sym = $1;
		}
		;

stmnt		: nlstmnt
		| Tid ':' stmnt
		{
			$$ = an(OLIST, an(OLABEL, nil, nil), $3);
			$$->left->sym = $1;
		}
		;

info		:
		{ $$ = nil; }
		| ',' Tstring
		{
			$$ = strnode($2, 0);
		}
		;

nlstmnt		: error ';'
		{ $$ = nil; }
		| zexpr ';'
		| block
		| Tcheck expr info ';'
		{
			$$ = an(OCHECK, $2, $3);
		}
		| Talloc elist ';'
		{
			$$ = an(OALLOC, $2, nil);
		}
		| Tunalloc elist ';'
		{
			$$ = an(OUALLOC, $2, nil);
		}
		| Trescue rbody
		{
			$$ = an(ORESCUE, $2, nil);
		}
		| Traise zlab ';'
		{
			$$ = an(ORAISE, $2, nil);
		}
		| Tgoto Tid ';'
		{
			$$ = an(OGOTO, nil, nil);
			$$->sym = $2;
		}
		| Tprocess elist ';'
		{
			$$ = an(OPROCESS, $2, nil);
		}
		| Ttask elist ';'
		{
			$$ = an(OTASK, $2, nil);
		}
		| Tbecome expr ';'
		{
			$$ = an(OBECOME, $2, nil);
		}
		| Tselect cbody
		{
			$$ = an(OSELECT, $2, nil);
		}
		| Treturn zexpr ';'
		{
			$$ = an(ORET, $2, nil);
		}
		| Tfor '(' zexpr ';' zexpr ';' zexpr ')' stmnt
		{
			$$ = an(OFOR, an(OLIST, $3, an(OLIST, $5, $7)), $9);
		}
		| Twhile '(' expr ')' stmnt
		{
			$$ = an(OWHILE, $3, $5);
		}
		| Tdo stmnt Twhile '(' expr ')'
		{
			$$ = an(ODWHILE, $5, $2);
		}
		| Tif '(' expr ')' stmnt
		{
			$$ = an(OIF, $3, $5);
		}
		| Tif '(' expr ')' stmnt Telse stmnt
		{
			$$ = an(OIF, $3, an(OELSE, $5, $7));
		}
		| Tpar block
		{
			$$ = an(OPAR, $2, nil);
		}
		| Tswitch expr cbody
		{
			$$ = an(OSWITCH, $2, $3);
		}
		| Ttypeof expr
		{
			$2->link = swstack;
			swstack = $2;
		} 
		tbody
		{
			swstack = $2->link;
			$$ = an(OSWITCH, $2, $4);
		}
		| Tcontinue zconst ';'
		{
			$$ = an(OCONT, nil, nil);
			$$->ival = $2;
		}
		| Tbreak zconst ';'
		{
			$$ = an(OBREAK, nil, nil);
			$$->ival = $2;
		}
		;

zconst		:
		{ $$ = 1; }
		| Tconst
		;

zexpr		:
		{ $$ = nil; }
		| expr
		;

expr		: castexpr
		| expr '*' expr
		{
			$$ = an(OMUL, $1, $3); 
		}
		| expr '/' expr
		{
			$$ = an(ODIV, $1, $3);
		}
		| expr '%' expr
		{
			$$ = an(OMOD, $1, $3);
		}
		| expr '+' expr
		{
			$$ = an(OADD, $1, $3);
		}
		| expr '-' expr
		{
			$$ = an(OSUB, $1, $3);
		}
		| expr Trsh expr
		{
			$$ = an(ORSH, $1, $3);
		}
		| expr Tlsh expr
		{
			$$ = an(OLSH, $1, $3);
		}
		| expr '<' expr
		{
			$$ = an(OLT, $1, $3);
		}
		| expr '>' expr
		{
			$$ = an(OGT, $1, $3);
		}
		| expr Tleq expr
		{
			$$ = an(OLEQ, $1, $3);
		}
		| expr Tgeq expr
		{
			$$ = an(OGEQ, $1, $3);
		}
		| expr Teq expr
		{
			$$ = an(OEQ, $1, $3);
		}
		| expr Tneq expr
		{
			$$ = an(ONEQ, $1, $3);
		}
		| expr '&' expr
		{
			$$ = an(OLAND, $1, $3);
		}
		| expr '^' expr
		{
			$$ = an(OXOR, $1, $3);
		}
		| expr '|' expr
		{
			$$ = an(OLOR, $1, $3);
		}
		| expr Tandand expr
		{
			$$ = an(OCAND, $1, $3);
		}
		| expr Toror expr
		{
			$$ = an(OCOR, $1, $3);
		}
		| expr '=' expr
		{
			$$ = an(OASGN, $1, $3);
		}
		| expr Tvasgn expr
		{
			$$ = an(OVASGN, $1, $3);
		}
		| expr Tcomm '=' expr
		{
			$$ = an(OASGN, an(OSEND, $1, nil), $4);
		}
		| expr Taddeq expr
		{
			$$ = an(OADDEQ, $1, $3);
		}
		| expr Tsubeq expr
		{
			$$ = an(OSUBEQ, $1, $3);
		}
		| expr Tmuleq expr
		{
			$$ = an(OMULEQ, $1, $3);
		}
		| expr Tdiveq expr
		{
			$$ = an(ODIVEQ, $1, $3);
		}
		| expr Tmodeq expr
		{
			$$ = an(OMODEQ, $1, $3);
		}
		| expr Trsheq expr
		{
			$$ = an(ORSHEQ, $1, $3);
		}
		| expr Tlsheq expr
		{
			$$ = an(OLSHEQ, $1, $3);
		}
		| expr Tandeq expr
		{
			$$ = an(OANDEQ, $1, $3);
		}
		| expr Toreq  expr
		{
			$$ = an(OOREQ, $1, $3);
		}
		| expr Txoreq expr
		{
			$$ = an(OXOREQ, $1, $3);
		}
		| expr Titer expr
		{
			$$ = an(OITER, $1, $3);
		}
		;

castexpr	: monexpr
		| '(' typecast ')' castexpr
		{
			$$ = an(OCONV, $4, nil);
			$$->t = $2;
		}
		| '(' Talloc typecast ')' castexpr
		{
			$$ = an(OALLOC, $5, nil);
			$$->t = $3;
		}
		;

typecast	: xtname
		| xtname indsp
		{
			$$ = mkcast($1, $2);
		}
		| xtname '(' indsp ')' '(' arglist ')'
		{
			$$ = mkcast(at(TFUNC, $1), $3);
			$$ = $$->next;
			$$->proto = an(OPROTO, $6, nil);
			$$ = at(TIND, $$);
		}
		| Ttuple tname
		{
			$$ = $2;
		}
		;

monexpr		: term
		| '*' castexpr 
		{
			$$ = an(OIND, $2, nil);
		}
		| '&' castexpr
		{
			$$ = an(OADDR, $2, nil);
		}
		| '+' castexpr
		{
			$$ = an(OADD, $2, con(0));
		}
		| '-' castexpr
		{
			$$ = an(OSUB, con(0), $2);
		}
		| Tdec castexpr
		{
			$$ = an(OEDEC, $2, nil);
		}
		| Tzerox castexpr
		{
			$$ = an(OXEROX, $2, nil);
		}
		| Tinc castexpr
		{
			$$ = an(OEINC, $2, nil);
		}
		| '!' castexpr
		{
			$$ = an(ONOT, $2, nil);
		}
		| '~' castexpr
		{
			$$ = an(OXOR, $2, con(-1));
		}
		| Tstorage monexpr
		{
			$$ = an(OSTORAGE, $2, nil);
		}
		| Tcomm castexpr
		{
			$$ = an(ORECV, $2, nil);
		}
		| '?' castexpr
		{
			$$ = an(OCRCV, $2, nil);
		}
		;

ztelist		: { $$ = nil; }
		| telist

tcomp		: expr
		| '{' ztelist '}'
		{
			$$ = an(OILIST, $2, nil);
			$$->t = at(TAGGREGATE, 0);
		}
		;

telist		: tcomp
		| telist ',' tcomp
		{
			$$ = an(OLIST, $1, $3);
		}

term		: '(' telist ')'
		{
			$$ = $2;
			if($2->type == OLIST) {
				$$ = an(OILIST, $2, nil);
				$$->t = at(TAGGREGATE, 0);
			}
		}
		| Tstorage '(' typecast ')'
		{
			$$ = an(OSTORAGE, nil, nil);
			$$->t = $3;
		}
		| term '(' zarlist ')'
		{
			$$ = an(OCALL, $1, $3);
		}
		| term '[' expr ']'
		{
			$$ = an(OIND, an(OADD, $1, $3), nil);
		}
		| term '.' stag
		{
			$$ = an(ODOT, $1, nil);
			$$->sym = $3;
		}
		| '.' Ttypename '.' stag
		{
			$$ = an(OCONST, nil, nil);
			$$->t = at(TVOID, 0);
			$$->t = at(TIND, $$->t);
			$$->ival = 0;
			$$ = an(OCONV, $$, nil);
			$$->t = at(TIND, $2.t);
			$$ = an(OIND, $$, nil);
			$$ = an(ODOT, $$, nil);
			$$->sym = $4;
		}
		| term Tindir stag
		{
			$$ = an(ODOT, an(OIND, $1, nil), nil);
			$$->sym = $3;
		}
		| term Tdec
		{
			$$ = an(OPDEC, $1, nil);
		}
		| term Tinc
		{
			$$ = an(OPINC, $1, nil);
		}
		| term '?'
		{
			$$ = an(OCSND, $1, nil);
		}
		| name
		{
			derivetype($1);
		}
		| '.' '.' '.'
		{
			$$ = vargptr();
		}
		| Tconst
		{
			$$ = con($1);
		}
		| Tnil
		{
			$$ = con(0);
			$$->t = at(TIND, builtype[TVOID]);
		}
		| Tsname
		{
			$$ = con($1);
		}
		| Tfconst
		{
			$$ = an(OCONST, nil, nil);
			$$->t = builtype[TFLOAT];
			$$->fval = $1;
		}
		| Tstring
		{
			$$ = strnode($1, 0);
		}
		| '$' Tstring
		{
			$$ = strnode($2, 1);
		}
		;

stag		: Tid
		| Ttypename
		{
			$$ = $1.s;
		}
		;

zarlist		:
		{ $$ = nil; }
		| elist
		;

elist		: expr
		| elist ',' expr
		{
			$$ = an(OLIST, $1, $3);
		}
		;

tlist		: typecast
		| typecast ',' tlist
		{
			$1->member = $3;
			$$ = $1;
		}
		;

tname		: sclass xtname
		{
			$2->class = $1;
			$$ = $2;
		}
		| sclass Ttuple '(' tlist ')'
		{
			$$ = unshape($4);
			$$->class = $1;
		}
		| sclass '(' tlist ')'
		{
			$$ = unshape($3);
			$$->class = $1;
		}
		;

variant		: typecast
		| typecast ',' variant
		{
			$1->variant = $3;
			$$ = $1;
		}
		;

xtname		: Tint		{ $$ = at(TINT, 0); }
		| Tuint		{ $$ = at(TUINT, 0); }
		| Tsint		{ $$ = at(TSINT, 0); }
		| Tsuint	{ $$ = at(TSUINT, 0); }
		| Tchar		{ $$ = at(TCHAR, 0); }
		| Tfloat	{ $$ = at(TFLOAT, 0); }
		| Tvoid		{ $$ = at(TVOID, 0); }
		| Ttypename
		{ $$ = $1.t; }
		| Ttypename '[' variant ']'
		{ $$ = polybuild($1.t, $3); }
		| Tchannel '(' variant ')' bufdim
		{
			$$ = at(TCHANNEL, $3);
			if($5 != nil)
				chani($$, $5);
		}
		;

bufdim		:
		{ $$ = nil; }
		| '[' expr ']'
		{
			$$ = $2;
		}
		;

sclass		:
		{
			$$ = Global;
			if(buildadt)
				$$ = Adtdeflt;
		}
		| Textern	{ $$ = External; }
		| Tintern	{ $$ = Internal; }
		| Tprivate	{ $$ = Private; }
		;
%%
