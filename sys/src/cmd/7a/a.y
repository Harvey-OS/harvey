%{
#include "a.h"
%}
%union
{
	Sym	*sym;
	vlong	lval;
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
%token	<lval>	LTYPE1 LTYPE2 LTYPE3 LTYPE4 LTYPE5
%token	<lval>	LTYPE6 LTYPE7 LTYPE8 LTYPE9 LTYPEA
%token	<lval>	LTYPEB LTYPEC LTYPED LTYPEE LTYPEF
%token	<lval>	LTYPEG LTYPEH LTYPEI LTYPEJ LTYPEK
%token	<lval>	LCONST LSP LSB LFP LPC LPREG LPCC
%token	<lval>	LTYPEX LREG LFREG LFPCR LR LP LF
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg
%type	<gen>	gen vgen lgen vlgen flgen rel reg freg preg fcreg
%type	<gen>	imm ximm ireg name oreg imr nireg fgen pcc
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
 * Integer operates
 */
	LTYPE1 imr ',' sreg ',' reg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTYPE1 imr ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * floating-type
 */
|	LTYPE2 freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE3 freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE3 freg ',' LFREG ',' freg
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * MOVQ
 */
|	LTYPE4 vlgen ',' vgen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * integer LOAD/STORE, but not MOVQ
 */
|	LTYPE5 lgen ',' gen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * integer LOAD/STORE (only)
 */
|	LTYPE6 oreg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEK reg ',' oreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * floating LOAD/STORE
 */
|	LTYPE7 flgen ',' fgen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * JMP/JSR/RET
 */
|	LTYPE8 comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE8 comma nireg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE8 sreg ',' nireg
	{
		outcode($1, &nullgen, $2, &$4);
	}
|	LTYPE9 comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * integer conditional branches
 */
|	LTYPEA gen ',' rel
	{
		if(!isreg(&$2))
			print("left side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * floating conditional branches
 */
|	LTYPEB fgen ',' rel
	{
		if(!isreg(&$2))
			print("left side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * TRAPB/MB/REI
 */
|	LTYPEC comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * FETCH/FETCHM
 */
|	LTYPED ireg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * Call-pal
 */
|	LTYPEE imm
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * TEXT/GLOBL
 */
|	LTYPEF name ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEF name ',' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * DATA
 */
|	LTYPEG name '/' con ',' ximm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * word
 */
|	LTYPEH comma ximm
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * NOP
 */
|	LTYPEI comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LTYPEI ',' vgen
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPEI vgen comma
	{
		outcode($1, &$2, NREG, &nullgen);
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

vlgen:
	lgen
|	preg
|	pcc

vgen:
	gen
|	preg

lgen:
	gen
|	ximm

flgen:
	fgen
|	ximm

fgen:
	gen
|	freg
|	fcreg

preg:
	LPREG
	{
		$$ = nullgen;
		$$.type = D_PREG;
		$$.reg = $1;
	}
|	LP '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_PREG;
		$$.reg = $1+$3;
	}

fcreg:
	LFPCR
	{
		$$ = nullgen;
		$$.type = D_FCREG;
		$$.reg = $1;
	}

freg:
	LFREG
	{
		$$ = nullgen;
		$$.type = D_FREG;
		$$.reg = $1;
	}
|	LF '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_FREG;
		$$.reg = $3;
	}

pcc:
	LPCC
	{
		$$ = nullgen;
		$$.type = D_PCC;
		$$.reg = $1;
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
|	con
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.offset = $1;
	}
|	oreg

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
|	LR '(' con ')'
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
