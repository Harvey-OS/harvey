%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../9c/9.out.h"
#include "a.h"
%}
%union
{
	Sym	*sym;
	long	lval;
	double	dval;
	char	sval[8];
	Gen	gen;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LTYPE1 LTYPE3 LTYPE4 LTYPE5 LTYPE6 LTYPE7 LTYPE8 LTYPE9 LTYPE10
%token	<lval>	LTYPEA LTYPEB LTYPEC
%token	<lval>	LTYPEG LTYPEH LTYPEI LTYPEJ LTYPEK
%token	<lval>	LCONST LSP LSB LFP LPC LMREG 
%token	<lval>	LTYPEX LREG LFREG LR
%token	<lval>	LFCR LSCHED
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg
%type	<gen>	gen rel reg abs zoreg
%type	<gen>	imm ximm ireg name oreg imr nireg
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
/*
 * rrr
 */
	LTYPE1 imr ',' sreg ',' reg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTYPE1 imr ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE1 ',' reg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * LOAD/STORE
 */
|	LTYPE3 gen ',' gen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE3 ximm ',' reg
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * LOAD/STORE
 */
|	LTYPE4 reg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * JMP/CALL
 */
|	LTYPE5 comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE5 comma nireg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE5 reg comma rel
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE5 reg comma nireg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * LOAD special
 */
|	LTYPE6 zoreg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * STORE special
 */
|	LTYPE7 reg ',' zoreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * INV
 */
|	LTYPE8 imm
	{
		if((ulong)$2.offset > 3)
			yyerror("inv constant too large");
		outcode($1, &nullgen, NREG, &$2);
	}
/*
 * MTSR special
 */
|	LTYPE9 reg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE9 imm ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * MFSR special
 */
|	LTYPE10 reg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * emulate
 */
|	LTYPEA abs ',' abs
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEA abs
	{
		outcode($1, &nullgen, NREG, &$2);
	}
|	LTYPEA
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * TEXT/GLOBL
 */
|	LTYPEB name ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEB name ',' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * DATA
 */
|	LTYPEC name '/' con ',' ximm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * ret
 */
|	LTYPEG comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * word
 */
|	LTYPEH comma ximm
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * nop
 */
|	LTYPEI comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LTYPEI comma reg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * scheduling on/off
 */
|	LSCHED
	{
		outcode(ANOSCHED, &nullgen, $1, &nullgen);
	}

comma:
|	','

rel:
	con '(' LPC ')'
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

ximm:	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}
|	'$' oreg
	{
		$$ = $2;
		$$.type = D_CONST;
	}
|	'$' '*' '$' oreg
	{
		$$ = $4;
		$$.type = D_OCONST;
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

nireg:
	ireg
|	name
	{
		$$ = $1;
		if($1.name != D_EXTERN && $1.name != D_STATIC) {
		}
	}

ireg:
	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}

gen:
	reg
|	abs
|	oreg

abs:
	con
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.offset = $1;
	}

zoreg	: '(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}

oreg:
	name
|	name '(' sreg ')'
	{
		$$ = $1;
		$$.type = D_OREG;
		$$.reg = $3;
	}
|	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}
|	con '(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $3;
		$$.offset = $1;
	}

imr:
	reg
|	imm

imm:	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}

reg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

sreg:
	LREG
|	LR '(' expr ')'
	{
		if($$ < 0 || $$ >= NREG)
			print("register value out of range\n");
		$$ = $3;
	}

name:
	con '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = $3;
		$$.sym = S;
		$$.offset = $1;
	}
|	LNAME offset '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = $4;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_STATIC;
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
