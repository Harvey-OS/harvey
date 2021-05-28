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
%token	<lval>	LADD LMUL LBEQ LBR LBRET LCALL LFLT2 LFLT3
%token	<lval>	LMOVB LMOVBU LMOVW LMOVF LLUI LSYS LSYS0 LCSR LSWAP LAMO
%token	<lval>	LCONST LSP LSB LFP LPC LREG LFREG LR FR LCTL
%token	<lval>	LDATA LTEXT LWORD
%token	<sval>	LSCONST
%token	<dval>	LFCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg freg oprrr
%type	<gen>	rreg dreg ctlreg drreg
%type	<gen>	addr name oreg rel imm ximm fimm
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
	LADD imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	oprrr rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LADD imm ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}
|	oprrr rreg ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}

|	LFLT2 drreg ',' drreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFLT3 drreg ',' freg ',' drreg
	{
		outcode($1, &$2, $4, &$6);
	}

|	LBEQ rreg ',' sreg ',' rel
	{
		outcode($1, &$2, $4, &$6);
	}

|	LBEQ rreg ',' rel
	{
		Gen regzero;
		regzero = nullgen;
		regzero.type = D_REG;
		regzero.reg = 0;
		outcode($1, &regzero, $2.reg, &$4);
	}

|	LBR	rel
	{
		outcode($1, &nullgen, NREG, &$2);
	}

|	LBR oreg
	{
		outcode($1, &nullgen, NREG, &$2);
	}


|	LBRET
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}

|	LCALL sreg ',' addr
	{
		outcode($1, &nullgen, $2, &$4);
	}
|	LCALL sreg ',' rel
	{
		outcode($1, &nullgen, $2, &$4);
	}

|	LMOVB addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVBU addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}

|	LMOVF addr ',' dreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVF dreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVF dreg ',' dreg
	{
		outcode($1, &$2, NREG, &$4);
	}

		
|	LMOVW imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW ximm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW rreg ',' ctlreg
	{
		Gen regzero;
		regzero = nullgen;
		regzero.type = D_REG;
		regzero.reg = 0;
		outcode(ACSRRW, &$4, $2.reg, &regzero);
	}
|	LMOVW imm ',' ctlreg
	{
		Gen regzero;
		int r = $2.offset;
		if(r < 0 || r >= NREG)
			yyerror("immediate value out of range");
		regzero = nullgen;
		regzero.type = D_REG;
		regzero.reg = 0;
		outcode(ACSRRWI, &$4, r, &regzero);
	}
|	LMOVW ctlreg ',' rreg
	{
		outcode(ACSRRS, &$2, REGZERO, &$4);
	}

|	LLUI name ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LLUI imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}


|	LSYS imm
	{
		outcode($1, &nullgen, NREG, &$2);
	}

|	LSYS0
	{
		Gen syscon;
		syscon = nullgen;
		syscon.type = D_CONST;
		syscon.offset = $1;
		outcode(ASYS, &nullgen, NREG, &syscon);
	}

|	LCSR ctlreg ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}

|	LCSR ctlreg ',' '$' con ',' rreg
	{
		if($5 < 0 || $5 >= NREG)
			yyerror("immediate value out of range");
		outcode($1 + (ACSRRWI-ACSRRW), &$2, $5, &$7);
	}

|	LSWAP rreg ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}

|	LAMO con ',' rreg ',' sreg ',' rreg
	{
		outcode($1, &$4, ($2<<16)|$6, &$8);
	}

|	LTEXT name ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTEXT name ',' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}

|	LDATA name '/' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' ximm
	{
		outcode($1, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' fimm
	{
		outcode($1, &$2, $4, &$6);
	}

|	LWORD imm
	{
		outcode($1, &nullgen, NREG, &$2);
	}

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

oprrr:
	LADD
|	LMUL

addr:
	oreg
|	name

oreg:
	'(' sreg ')'
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

rreg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

dreg:
	freg
	{
		$$ = nullgen;
		$$.type = D_FREG;
		$$.reg = $1;
	}

drreg:
	dreg
|	rreg
	
sreg:
	LREG
|	LR '(' expr ')'
	{
		if($3 < 0 || $3 >= NREG)
			yyerror("register value out of range");
		$$ = $3;
	}

freg:
	LFREG
|	FR '(' expr ')'
	{
		if($3 < 0 || $3 >= NREG)
			yyerror("register value out of range");
		$$ = $3;
	}

ctlreg:
	LCTL '(' expr ')'
	{
		if($3 < 0 || $3 >= 0xFFF)
			yyerror("CSR value out of range");
		$$ = nullgen;
		$$.type = D_CTLREG;
		$$.offset = $3;
	}

ximm:
	'$' addr
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

fimm:
	'$' LFCONST
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

imm:
	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
		if(thechar == 'j' && (vlong)$$.offset != $2){
			$$.type = D_VCONST;
			$$.vval = $2;
		}
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
