%{
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
%token	<lval>	LTYPE1 LTYPE2 LTYPE3 LTYPE4 LTYPE5
%token	<lval>	LTYPE6 LTYPE7 LTYPE8 LTYPE9 LTYPEA
%token	<lval>	LTYPEB LTYPEC LTYPED LTYPEE LTYPEF
%token	<lval>	LTYPEG LTYPEH LTYPEI LTYPEJ LTYPEK
%token	<lval>	LCONST LSP LSB LFP LPC LHI LLO LMREG 
%token	<lval>	LTYPEX LREG LFREG LFCREG LR LM LF
%token	<lval>	LFCR LSCHED
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg
%type	<gen>	gen vgen lgen vlgen rel reg freg mreg fcreg
%type	<gen>	imm ximm ireg name oreg imr nireg fgen
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
|	LSCHED ';'
	{
		nosched = $1;
	}
|	';'
|	inst ';'
|	error ';'

inst:
/*
 * Immed-type
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
 * NOR
 */
|	LTYPE2 imr ',' sreg ',' imr
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTYPE2 imr ',' imr
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * LOAD/STORE, but not MOVW
 */
|	LTYPE3 lgen ',' gen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * SPECIAL
 */
|	LTYPE4 comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * MOVW
 */
|	LTYPE5 vlgen ',' vgen
	{
		if(!isreg(&$2) && !isreg(&$4))
			print("one side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * MUL/DIV
 */
|	LTYPE6 reg ',' sreg comma
	{
		outcode($1, &$2, $4, &nullgen);
	}
|	LTYPE6 reg ',' sreg ',' reg
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * JMP/JAL
 */
|	LTYPE7 comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE7 comma nireg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
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
/*
 * BEQ/BNE
 */
|	LTYPE9 gen ',' rel
	{
		if(!isreg(&$2))
			print("left side must be register\n");
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPE9 gen ',' sreg ',' rel
	{
		if(!isreg(&$2))
			print("left side must be register\n");
		outcode($1, &$2, $4, &$6);
	}
/*
 * B-other
 */
|	LTYPEA gen ',' rel
	{
		if(!isreg(&$2))
			print("left side must be register\n");
		outcode($1, &$2, NREG, &$4);
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
 * floating-type
 */
|	LTYPED freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEE freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEE freg ',' LFREG ',' freg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTYPEF freg ',' LFREG comma
	{
		outcode($1, &$2, $4, &nullgen);
	}
/*
 * coprocessor branch
 */
|	LTYPEG comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
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
/*
 * BREAK -- overloaded with CACHE opcode
 */
|	LTYPEJ comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LTYPEJ vgen ',' vgen
	{
		outcode($1, &$2, NREG, &$4);
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
|	fgen
|	mreg
|	fcreg
|	LHI
	{
		$$ = nullgen;
		$$.type = D_HI;
	}
|	LLO
	{
		$$ = nullgen;
		$$.type = D_LO;
	}

vgen:
	gen
|	fgen
|	mreg
|	fcreg
|	LHI
	{
		$$ = nullgen;
		$$.type = D_HI;
	}
|	LLO
	{
		$$ = nullgen;
		$$.type = D_LO;
	}

lgen:
	gen
|	ximm

fgen:
	freg

mreg:
	LMREG
	{
		$$ = nullgen;
		$$.type = D_MREG;
		$$.reg = $1;
	}
|	LM '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_MREG;
		$$.reg = $3;
	}

fcreg:
	LFCREG
	{
		$$ = nullgen;
		$$.type = D_FCREG;
		$$.reg = $1;
	}
|	LFCR '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_FCREG;
		$$.reg = $3;
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
|	con ireg
	{
		if($1 != 0)
			yyerror("offset must be zero");
		$$ = $2;
	}
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
