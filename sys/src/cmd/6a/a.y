%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../6c/6.out.h"
#include "a.h"
%}
%union	{
	Sym	*sym;
	long	lval;
	double	dval;
	char	sval[8];
	Gen	gen;
	Gen2	gen2;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LTYPES LTYPED LTYPEB LTYPE1 LTYPE2
%token	<lval>	LCONST LSP LFP LPC LSB LTYPEX
%token	<lval>	LRREG LFREG
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset reg
%type	<gen>	addr
%type	<gen>	imm nam
%type	<gen2>	regs srrr2 drrr2 spec1 spec2
%%
prog:
|	prog line

line:
	LLAB ':'
	{
		if($1->value != pc)
			yyerror("redeclaration of %s", $1->name);
		$1->value = pc;
	}
	line
|	LNAME ':'
	{
		$1->type = LLAB;
		$1->value = pc;
	}
	line
|	';'
|	inst ';'
|	error ';'

inst:
	LNAME '=' expr
	{
		$1->type = LVAR;
		$1->value = $3;
	}
|	LVAR '=' expr
	{
		if($1->value != $3)
			yyerror("redeclaration of %s", $1->name);
		$1->value = $3;
	}
|	LTYPEB regs	{ outcode($1, &$2); }
|	LTYPES srrr2	{ outcode($1, &$2); }
|	LTYPED drrr2	{ outcode($1, &$2); }
|	LTYPE1 spec1	{ outcode($1, &$2); }
|	LTYPE2 spec2	{ outcode($1, &$2); }


srrr2:
	regs
|	addr
	{
		$$.from = $1;
		$$.to = nullgen;
		$$.type = D_NONE;
	}

drrr2:
	regs
|	addr
	{
		$$.from = nullgen;
		$$.to = $1;
		$$.type = D_NONE;
	}

regs:
	comma
	{
		$$.from = nullgen;
		$$.to = nullgen;
		$$.type = D_NONE;
	}
|	addr ','
	{
		$$.from = $1;
		$$.to = nullgen;
		$$.type = D_NONE;
	}
|	',' addr
	{
		$$.from = nullgen;
		$$.to = $2;
		$$.type = D_NONE;
	}
|	addr ',' addr
	{
		$$.from = $1;
		$$.to = $3;
		$$.type = D_NONE;
	}
|	addr ',' reg ',' addr
	{
		$$.from = $1;
		$$.to = $5;
		$$.type = $3;
	}

spec1:	/* DATA */
	addr '/' con ',' imm
	{
		$$.from = $1;
		$$.from.scale = $3;
		$$.to = $5;
		$$.type = D_NONE;
	}

spec2:	/* SHL/SHR/MOV */
	regs
|	regs ':' LRREG
	{
		$$ = $1;
		if($$.from.index != D_NONE)
			yyerror("dp shift with lhs index");
		$$.from.index = $3;
	}

addr:
	reg
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	con
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.offset = $1;
	}
|	con '(' LRREG ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$3;
		$$.offset = $1;
	}
|	con '(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+REGSP;
		$$.offset = $1;
	}
|	con '(' LRREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.offset = $1;
		$$.index = $3;
		$$.scale = $5;
		checkscale($$.scale);
	}
|	con '(' LRREG ')' '(' LRREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$3;
		$$.offset = $1;
		$$.index = $6;
		$$.scale = $8;
		checkscale($$.scale);
	}
|	'(' LRREG ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$2;
	}
|	'(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+REGSP;
	}
|	'(' LRREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.index = $2;
		$$.scale = $4;
		checkscale($$.scale);
	}
|	'(' LRREG ')' '(' LRREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$2;
		$$.index = $5;
		$$.scale = $7;
		checkscale($$.scale);
	}
|	nam
	{
		$$ = $1;
	}
|	nam '(' LRREG '*' con ')'
	{
		$$ = $1;
		$$.index = $3;
		$$.scale = $5;
		checkscale($$.scale);
	}
|	con '(' LPC ')'
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.offset = $1 + pc;
	}
|	LNAME offset
	{
		$$ = nullgen;
		if(pass == 2)
			yyerror("undefined label: %s", $1->name);
		$$.type = D_BRANCH;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LLAB offset
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.sym = $1;
		$$.offset = $1->value + $2;
	}
|	imm

imm:
	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}
|	'$' nam
	{
		$$ = $2;
		$$.index = $2.type;
		$$.type = D_ADDR;
		if($2.type == D_AUTO || $2.type == D_PARAM)
			yyerror("constant cannot be automatic: %s",
				$2.sym->name);
	}
|	'$' LSCONST
	{
		$$ = nullgen;
		$$.type = D_SCONST;
		memcpy($$.sval, $2, sizeof($$.sval));
	}
|	'$' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = $2;
	}
|	'$' '-' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = -$3;
	}
reg:
	LRREG
|	LFREG

nam:
	LNAME offset '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = $4;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_STATIC;
		$$.sym = $1;
		$$.offset = $4;
	}

offset:
	{
		$$ = 0;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'-' con
	{
		$$ = -$2;
	}

pointer:
	LSB
|	LSP
|	LFP

con:
	LCONST
|	LVAR
	{
		$$ = $1->value;
	}
|	'-' con
	{
		$$ = -$2;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'~' con
	{
		$$ = ~$2;
	}
|	'(' expr ')'
	{
		$$ = $2;
	}

expr:
	con
|	expr '+' expr
	{
		$$ = $1 + $3;
	}
|	expr '-' expr
	{
		$$ = $1 - $3;
	}
|	expr '*' expr
	{
		$$ = $1 * $3;
	}
|	expr '/' expr
	{
		$$ = $1 / $3;
	}
|	expr '%' expr
	{
		$$ = $1 % $3;
	}
|	expr '<' '<' expr
	{
		$$ = $1 << $4;
	}
|	expr '>' '>' expr
	{
		$$ = $1 >> $4;
	}
|	expr '&' expr
	{
		$$ = $1 & $3;
	}
|	expr '^' expr
	{
		$$ = $1 ^ $3;
	}
|	expr '|' expr
	{
		$$ = $1 | $3;
	}

comma:
	| ','
