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
	float	fval;
	int	clas;
	String	*string;
	Tysym	ltype;
}

%type <node> zinit ivardecl zelist prog decllist decl vardecllist vardecl ztag
%type <node> arglist arrayspec indsp block slist stmnt zexpr expr castexpr monexpr
%type <node> term zarlist elist typespec memberlist name autolist autodecl setlist
%type <node> arg bufdim convexpr clist case rbody zlab sname adtfunc
%type <node> cbody
%type <type> typecast xtname tname ztname
%type <clas> sclass
%type <ival> zconst
%type <sym>  stag

%left	';'
%left	','
%right	'=' Taddeq Tsubeq Tmuleq Tdiveq Tmodeq Trsheq Tlsheq Tandeq Toreq Txoreq
%right	'?'
%left	Toror
%left	Tandand
%left	'|'
%left	'^'
%left	'&'
%left	Teq Tneq
%left	'<' '>' Tleq Tgeq
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
%token Tandeq Toreq Txoreq Tcomeq Tgoto Tdec Tinc Tnil Talloc Tunalloc Tid
%token Tguard Tprivate

%%
prog:		decllist
		{
			strop();
		}
		;

decllist	:
		{ $$ = ZeroN; }
		| decllist decl
		{
			$$ = an(OLIST, $1, $2);
			pushdcl($2);
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
			fundecl($1, $2, $4);
			adtchk($2->init, $2);
			$4->init = ZeroN;
		}
		block
		{
			fungen($7, $2);
			adtbfun = ZeroT;
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
			buildtype($1);
		}
		| Tnewtype ztname name ';'
		{
			newtype($2, $3);
			$$ = ZeroN;
		}
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

ztname		:
		{ $$ = 0; }
		| tname
		| Taggregate
		{ $$ = ZeroT; }
		| Tadt
		{ $$ = ZeroT; }
		| Tunion
		{ $$ = ZeroT; }
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
		| Tadt name
		{
			$2->sym->lexval = Ttypename;
			$2->right = buildadt;
			buildadt = $2;
		} 
		'{' memberlist '}' ztag
		{
			buildadt = $2->right;
			$2->right = ZeroN;
			$$ = an(OADTDECL, an(OLIST, $2, $7), $5);
		}
		| Tset ztag
		{
			setbase = 0;
			if($2) {
				$2->sym->lexval = Ttypename;
				newtype(builtype[TINT], $2);
			}
		}
		'{' setlist '}'
		{
			$$ = an(OSETDECL, ZeroN, ZeroN);
		}
		;

ztag		:
		{ $$ = ZeroN; }
		| name
		| Ttypename
		{
			$$ = an(ONAME, ZeroN, ZeroN);
			$$->sym = $1.s;
		}
		;

setlist		: sname
		| setlist ',' setlist
		{
			$$ = an(OLIST, $1, $3);
		}
		;

sname		:
		{ $$ = ZeroN; }
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
			$$ = an(ONAME, ZeroN, ZeroN);
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
		{ $$ = ZeroN; }
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
		{ $$ = ZeroN; }
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
		| '{' zelist '}'
		{
			$$ = an(OILIST, $2, ZeroN);
		}
		| zelist ',' zelist
		{
			$$ = an(OLIST, $1, $3);
		}
		;

vardecl		: Tid arrayspec
		{
			$$ = an(ONAME, $2, ZeroN);
			$$->sym = $1;
		}
		| indsp Tid arrayspec
		{
			$$ = an(ONAME, $3, $1);
			$$->sym = $2;
		}
		| '(' indsp Tid arrayspec ')' '(' arglist ')'
		{
			$$ = an(OFUNC, an(OLIST, $4, $2), ZeroN);
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
		{ $$ = ZeroN; }
		| arrayspec '[' zexpr ']'
		{
			$$ = an(OARRAY, $3, $1);
			typechk($3, 0);
			rewrite($3);
		}
		;

indsp		: '*'
		{
			$$ = an(OIND, ZeroN, ZeroN);
		}
		| indsp '*'
		{
			$$ = an(OIND, $1, ZeroN);
		}
		;


arglist		:
		{
			$$ = an(OPROTO, ZeroN, ZeroN);
			$$->t = builtype[TVOID];
		}
		| arg
		| '*' xtname
		{
			$$ = an(OADTARG, ZeroN, ZeroN);
			$$->t = at(TIND, $2);
		}
		| arglist ',' arg
		{
			$$ = an(OLIST, $1, $3);
		}
		;

arg		: xtname
		{
			$$ = an(OPROTO, ZeroN, ZeroN);
			$$->t = $1;
		}
		| xtname indsp
		{
			$$ = an(OPROTO, ZeroN, ZeroN);
			$$->t = mkcast($1, $2);
		}
		| xtname '(' '*' ')' '(' arglist ')'
		{
			$$= an(OPROTO, ZeroN, ZeroN);
			$$->t = $1;
			$$->t = at(TFUNC, $$->t);
			$$->t = at(TIND, $$->t);
			$$->proto = $6;
		}
		| xtname vardecl
		{
			applytype($1, $2);
			$$ = $2;
		}
		| '.' '.' '.'
		{
			$$ = an(OVARARG, ZeroN, ZeroN);
		}
		;

autolist	: 
		{ $$ = ZeroN; }
		| autolist autodecl
		{
			$$ = an(OLIST, $1, $2);
		}
		;

autodecl	: xtname vardecllist ';'
		{
			applytype($1, $2);
			pushdcl($2);
			$$ = $2;
		}
		;

block		: '{' 
			{ enterblock(); }
		autolist 
			{ scopeis(Automatic); }
		slist
			{ leaveblock(); }
		'}'
			{ $$ = an(OBLOCK, $5, ZeroN); }
		| Tguard
			{ enterblock(); }
		autolist 
			{ scopeis(Automatic); }
		slist
			{ leaveblock(); }
		'}'
			{ $$ = an(OLBLOCK, $5, ZeroN); }
		;

slist		:
		{ $$ = ZeroN; }
		| slist stmnt
		{
			$$ = an(OLIST, $1, $2);
		}
		;

cbody		: '{' clist '}'
		{
			$$ = $2;
		}
		| Tguard clist '}'
		{
			$$ = an(OLBLOCK, $2, ZeroN);
		}
		;

clist		:
		{ $$ = ZeroN; }
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
			$$ = an(ODEFAULT, ZeroN, $3);
		}
		;

rbody		: stmnt
		| Tid block
		{
			$$ = an(OLABEL, $2, ZeroN);
			$$->sym = $1;
		}
		;

zlab		:
		{ $$ = ZeroN; }
		| Tid
		{
			$$ = an(OLABEL, ZeroN, ZeroN);
			$$->sym = $1;
		}
		;

stmnt		: error ';'
		{ $$ = ZeroN; }
		| zexpr ';'
		| block
		| Tcheck expr ';'
		{
			$$ = an(OCHECK, $2, ZeroN);
		}
		| Talloc elist ';'
		{
			$$ = an(OALLOC, $2, ZeroN);
		}
		| Tunalloc elist ';'
		{
			$$ = an(OUALLOC, $2, ZeroN);
		}
		| Tid ':'
		{
			$$ = an(OLABEL, ZeroN, ZeroN);
			$$->sym = $1;
		}
		| Trescue rbody
		{
			$$ = an(ORESCUE, $2, ZeroN);
		}
		| Traise zlab ';'
		{
			$$ = an(ORAISE, $2, ZeroN);
		}
		| Tgoto Tid ';'
		{
			$$ = an(OGOTO, ZeroN, ZeroN);
			$$->sym = $2;
		}
		| Tprocess expr ';'
		{
			$$ = an(OPROCESS, $2, ZeroN);
		}
		| Ttask expr ';'
		{
			$$ = an(OTASK, $2, ZeroN);
		}
		| Tselect cbody
		{
			$$ = an(OSELECT, $2, ZeroN);
		}
		| Treturn zexpr ';'
		{
			$$ = an(ORET, $2, ZeroN);
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
			$$ = an(OPAR, $2, ZeroN);
		}
		| Tswitch '(' expr ')' cbody
		{
			$$ = an(OSWITCH, $3, $5);
		}
		| Tcontinue zconst ';'
		{
			$$ = an(OCONT, ZeroN, ZeroN);
			$$->ival = $2;
		}
		| Tbreak zconst ';'
		{
			$$ = an(OBREAK, ZeroN, ZeroN);
			$$->ival = $2;
		}
		;

zconst		:
		{ $$ = 1; }
		| Tconst
		;

zexpr		:
		{ $$ = ZeroN; }
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
		;

castexpr	: monexpr
		| '[' typecast ']' castexpr
		{
			$$ = an(OCAST, $4, ZeroN);
			$$->t = $2;
		}
		| '(' typecast ')' convexpr
		{
			$$ = $4;
			$$->t = $2;
		}
		;

convexpr	: castexpr
		{
			$$ = an(OCONV, $1, ZeroN);
		}
		| '{' zelist '}'
		{
			$$ = an(OILIST, $2, ZeroN);
		}
		;

typecast	: xtname
		| xtname indsp
		{
			$$ = mkcast($1, $2);
		}
		;

monexpr		: term
		| '*' castexpr 
		{
			$$ = an(OIND, $2, ZeroN);
		}
		| '&' castexpr
		{
			$$ = an(OADDR, $2, ZeroN);
		}
		| '+' castexpr
		{
			$$ = con(0);
			$$ = an(OADD, $2, $$);
		}
		| '-' castexpr
		{
			$$ = con(0);
			$$ = an(OSUB, $$, $2);
		}
		| Tdec castexpr
		{
			$$ = an(OEDEC, $2, ZeroN);
		}
		| Tinc castexpr
		{
			$$ = an(OEINC, $2, ZeroN);
		}
		| '!' castexpr
		{
			$$ = an(ONOT, $2, ZeroN);
		}
		| '~' castexpr
		{
			$$ = an(OXOR, $2, con(-1));
		}
		| Tstorage monexpr
		{
			$$ = an(OSTORAGE, $2, ZeroN);
		}
		;

term		: '(' expr ')'
		{
			$$ = $2;
		}
		| Tstorage '(' typecast ')'
		{
			$$ = an(OSTORAGE, ZeroN, ZeroN);
			$$->t = $3;
		}
		| term '(' zarlist ')'
		{
			$$ = an(OCALL, $1, $3);
		}
		| term '[' expr ']'
		{
			$$ = an(OIND, an(OADD, $1, $3), ZeroN);
		}
		| Tcomm term
		{
			$$ = an(ORECV, $2, ZeroN);
		}
		| '?' term
		{
			$$ = an(OCRCV, $2, ZeroN);
		}
		| term Tcomm
		{
			$$ = an(OSEND, $1, ZeroN);
		}
		| term '?'
		{
			$$ = an(OCSND, $1, ZeroN);
		}
		| term '.' stag
		{
			$$ = an(ODOT, $1, ZeroN);
			$$->sym = $3;
		}
		| term Tindir stag
		{
			$$ = an(ODOT, an(OIND, $1, ZeroN), ZeroN);
			$$->sym = $3;
		}
		| term Tdec
		{
			$$ = an(OPDEC, $1, ZeroN);
		}
		| term Tinc
		{
			$$ = an(OPINC, $1, ZeroN);
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
			$$ = an(OCONST, ZeroN, ZeroN);
			$$->t = at(TVOID, 0);
			$$->t = at(TIND, $$->t);
			$$->ival = 0;
		}
		| Tsname
		{
			$$ = con($1);
		}
		| Tfconst
		{
			$$ = an(OCONST, ZeroN, ZeroN);
			$$->t = builtype[TFLOAT];
			$$->fval = $1;
		}
		| Tstring
		{
			$$ = strnode($1);
		}
		;

stag		: Tid
		| Ttypename
		{
			$$ = $1.s;
		}
		;

zarlist		:
		{ $$ = ZeroN; }
		| elist
		;

elist		: expr
		| elist ',' expr
		{
			$$ = an(OLIST, $1, $3);
		}
		;


tname		: sclass xtname
		{
			$2->class = $1;
			$$ = $2;
		}

xtname		: Ttypename	{ $$ = $1.t; }
		| Tint		{ $$ = at(TINT, 0); }
		| Tuint		{ $$ = at(TUINT, 0); }
		| Tsint		{ $$ = at(TSINT, 0); }
		| Tsuint	{ $$ = at(TSUINT, 0); }
		| Tchar		{ $$ = at(TCHAR, 0); }
		| Tfloat	{ $$ = at(TFLOAT, 0); }
		| Tvoid		{ $$ = at(TVOID, 0); }
		| Tchannel '(' typecast ')' bufdim
		{
			$$ = at(TCHANNEL, $3);
			if($5 != ZeroN)
				chani($$, $5);
		}
		;

bufdim		:
		{ $$ = ZeroN; }
		| '[' expr ']'
		{
			$$ = $2;
		}
		;

sclass		:
		{
			$$ = External;
			if(buildadt)
				$$ = Adtdeflt;
		}
		| Textern	{ $$ = External; }
		| Tintern	{ $$ = Internal; }
		| Tprivate	{ $$ = Private; }
		;
%%
