%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../zc/z.out.h"
#include "a.h"
%}
%union
{
	Sym	*sym;
	long	lval;
	double	dval;
	char	sval[NSNAME+1];
	Gen	gen;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'

%token	<lval>	LTYPED LTYPET
%token	<lval>	LTYPE1 LTYPE2 LTYPE3 LTYPE4 LTYPE5 LTYPE6 LTYPE7
%token	<lval>	LCONST LSP LSB LFP LPC LREG LWID LDQWID LR
%token	<sym>	LNAME LLAB LVAR
%token	<sval>	LSCONST
%token	<dval>	LFCONST
%type	<lval>	con expr pointer offset
%type	<gen>	gen dqgen d1 rel

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
|	LNAME '=' expr ';'
	{
		$1->type = LVAR;
		$1->value = $3;
	}
|	LVAR '=' expr ';'
	{
		if($1->value != $3)
			yyerror("redeclaration of %s", $1->name);
		$1->value = $3;
	}
|	';'
|	inst ';'
|	error ';'

inst:
	LTYPED gen '/' con ',' gen
	{
		switch($4) {
		default:
			print("unknown DATA width: %d\n", $4);
			$2.width = W_GOK;
			break;
		case 1:
			$2.width = W_B;
			break;
		case 2:
			$2.width = W_H;
			break;
		case 4:
			$2.width = W_W;
			break;
		case 8:
			$2.width = W_D;
			break;
		case 12:
			$2.width = W_E;
			break;
		}
		outcode($1, &$2, &$6);
	}
|	LTYPED gen ',' gen
	{
		outcode($1, &$2, &$4);
	}
|	LTYPET gen ',' con ',' gen
	{
		if($4)
			$2.width = W_B;
		outcode($1, &$2, &$6);
	}
|	LTYPET gen ',' gen
	{
		outcode($1, &$2, &$4);
	}
|	LTYPE1 gen ',' gen
	{
		outcode($1, &$2, &$4);
	}
|	LTYPE2
	{
		outcode($1, &nullgen, &nullgen);
	}
|	LTYPE2 ','
	{
		outcode($1, &nullgen, &nullgen);
	}
|	LTYPE3 gen
	{
		outcode($1, &nullgen, &$2);
	}
|	LTYPE3 ',' gen
	{
		outcode($1, &nullgen, &$3);
	}
|	LTYPE4 gen
	{
		outcode($1, &$2, &nullgen);
	}
|	LTYPE4 gen ','
	{
		outcode($1, &$2, &nullgen);
	}
|	LTYPE5 rel
	{
		outcode($1, &nullgen, &$2);
	}
|	LTYPE5 ',' rel
	{
		outcode($1, &nullgen, &$3);
	}
|	LTYPE6
	{
		outcode($1, &nullgen, &nullgen);
	}
|	LTYPE6 ','
	{
		outcode($1, &nullgen, &nullgen);
	}
|	LTYPE6 gen
	{
		outcode($1, &nullgen, &$2);
	}
|	LTYPE6 ',' gen
	{
		outcode($1, &nullgen, &$3);
	}
|	LTYPE7 gen ',' gen
	{
		outcode($1, &$2, &$4);
	}
|	LTYPE7 gen ',' dqgen
	{
		outcode(ADQM, &$2, &$4);
	}

dqgen:
	d1 LDQWID
	{
		$$ = $1;
		$$.width = $2;
	}
|	'*' d1 LDQWID
	{
		$$ = $2;
		$$.type |= D_INDIR;
		$$.width = $3;
	}
|	'*' '$' con LDQWID
	{
		$$ = nullgen;
		$$.type = D_CONST|D_INDIR;
		$$.offset = $3;
		$$.width = $4;
	}

gen:
	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
		$$.width = W_NONE;
	}
|	'$' LSCONST
	{
		$$ = nullgen;
		$$.type = D_SCONST;
		memcpy($$.sval, $2, sizeof($$.sval));
		$$.width = sizeof($$.sval);
	}
|	'$' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = $2;
		$$.width = W_NONE;
	}
|	'%' con
	{
		$$ = nullgen;
		$$.type = D_CPU;
		$$.offset = $2;
		$$.width = W_UH;
	}
|	'$' d1
	{
		$$ = $2;
		$$.width = $$.type - D_NONE;
		$$.type = D_ADDR;
	}
|	d1
	{
		$$ = $1;
		$$.width = W_NONE;
	}
|	d1 LWID
	{
		$$ = $1;
		$$.width = $2;
	}
|	'*' d1
	{
		$$ = $2;
		$$.type |= D_INDIR;
		$$.width = W_NONE;
	}
|	'*' d1 LWID
	{
		$$ = $2;
		$$.type |= D_INDIR;
		$$.width = $3;
	}
|	'*' '$' con
	{
		$$ = nullgen;
		$$.type = D_CONST|D_INDIR;
		$$.offset = $3;
		$$.width = W_NONE;
	}
|	'*' '$' con LWID
	{
		$$ = nullgen;
		$$.type = D_CONST|D_INDIR;
		$$.offset = $3;
		$$.width = $4;
	}

d1:
	LREG
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.offset = $1;
	}
|	LR con
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.offset = $2;
	}
|	LNAME offset '(' pointer ')'
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

rel:
	gen
|	con '(' LPC ')'
	{
		$$ = nullgen;
		$$.sym = S;
		$$.offset = $1 + pc;
		$$.type = $3;
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

pointer:
	LSB
|	LSP
|	LFP
