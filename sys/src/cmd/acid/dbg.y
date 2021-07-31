%{
#include <u.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
%}

%union
{
	Node	*node;
	Lsym	*sym;
	ulong	ival;
	float	fval;
	String	*string;
}

%type <node> expr monexpr term stmnt name args zexpr slist

%left	';'
%left	','
%right	'=' 
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
%right	Tdec Tinc Tindir '.' '[' '('

%token <sym>	Tid
%token <ival>	Tconst
%token <fval>	Tfconst
%token <string>	Tstring
%token Tif Tdo Tthen Telse Twhile Tloop Thead Ttail Tappend Tfn Tret Tlocal
%token Tcomplex Twhat Tdelete

%%

prog		: 
		| prog bigstmnt
		;

bigstmnt	: stmnt
		{
			execute($1);
			gc();
			if(interactive)
				Bprint(bout, "acid: ");
		}
		| Tfn Tid '(' args ')' stmnt
		{
			if($2->builtin)
				error("cannot redefine a builtin function");

			$2->proc = an(OLIST, $4, $6);
		}
		| Twhat Tid
		{
			whatis($2);
		}
		;

slist		: stmnt
		| slist stmnt
		{
			$$ = an(OLIST, $1, $2);
		}
		;

stmnt		: zexpr ';'
		| '{' slist '}'
		{
			$$ = $2;
		}
		| Tif expr Tthen stmnt
		{
			$$ = an(OIF, $2, $4);
		}
		| Tif expr Tthen stmnt Telse stmnt
		{
			$$ = an(OIF, $2, an(OELSE, $4, $6));
		}
		| Tloop expr ',' expr Tdo stmnt
		{
			$$ = an(ODO, an(OLIST, $2, $4), $6);
		}
		| Twhile expr Tdo stmnt
		{
			$$ = an(OWHILE, $2, $4);
		}
		| Tret expr ';'
		{
			$$ = an(ORET, $2, ZN);
		}
		| Tlocal Tid
		{
			$$ = an(OLOCAL, ZN, ZN);
			$$->sym = $2;
		}
		| Tcomplex Tid Tid ';'
		{
			$$ = an(OCOMPLEX, an(ONAME, ZN, ZN), ZN);
			$$->sym = $2;
			$$->left->sym = $3;
		}
		;

zexpr		:
		{ $$ = 0; }
		| expr
		;

expr		: monexpr
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
		;

monexpr		: term
		| '*' monexpr 
		{
			$$ = an(OINDM, $2, ZN);
		}
		| '@' monexpr 
		{
			$$ = an(OINDC, $2, ZN);
		}
		| '+' monexpr
		{
			$$ = con(0);
			$$ = an(OADD, $2, $$);
		}
		| '-' monexpr
		{
			$$ = con(0);
			$$ = an(OSUB, $$, $2);
		}
		| Tdec monexpr
		{
			$$ = an(OEDEC, $2, ZN);
		}
		| Tinc monexpr
		{
			$$ = an(OEINC, $2, ZN);
		}
		| Thead monexpr
		{
			$$ = an(OHEAD, $2, ZN);
		}
		| Ttail monexpr
		{
			$$ = an(OTAIL, $2, ZN);
		}
		| Tappend monexpr ',' monexpr
		{
			$$ = an(OAPPEND, $2, $4);
		}
		| Tdelete monexpr ',' monexpr
		{
			$$ = an(ODELETE, $2, $4);
		}
		| '!' monexpr
		{
			$$ = an(ONOT, $2, ZN);
		}
		| '~' monexpr
		{
			$$ = an(OXOR, $2, con(-1));
		}
		;

term		: '(' expr ')'
		{
			$$ = $2;
		}
		| '{' args '}'
		{
			$$ = an(OCTRUCT, $2, ZN);
		}
		| term '[' expr ']'
		{
			$$ = an(OINDEX, $1, $3);
		}
		| term Tdec
		{
			$$ = an(OPDEC, $1, ZN);
		}
		| term '.' Tid
		{
			$$ = an(ODOT, $1, ZN);
			$$->sym = $3;
		}
		| term Tindir Tid
		{
			$$ = an(ODOT, an(OINDM, $1, ZN), ZN);
			$$->sym = $3;
		}
		| term Tinc
		{
			$$ = an(OPINC, $1, ZN);
		}
		| name '(' args ')'
		{
			$$ = an(OCALL, $1, $3);
		}
		| name
		| Tconst
		{
			$$ = con($1);
		}
		| Tfconst
		{
			$$ = an(OCONST, ZN, ZN);
			$$->type = TFLOAT;
			$$->fval = $1;
		}
		| Tstring
		{
			$$ = an(OCONST, ZN, ZN);
			$$->type = TSTRING;
			$$->string = $1;
			$$->fmt = 's';
		}
		;

name		: Tid
		{
			$$ = an(ONAME, ZN, ZN);
			$$->sym = $1;
		}
		| Tid ':' name
		{
			$$ = an(OFRAME, $3, ZN);
			$$->sym = $1;
		}
		;

args		:
		{ $$ = 0; }
		| expr
		| expr ',' args
		{
			$$ = an(OLIST, $1, $3);
		}
		;
