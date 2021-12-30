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
%token	<lval>	LMOVW LMOVD LMOVB LSWAP LADDW LCMP
%token	<lval>	LBRA LFMOV LFCONV LFADD LCPOP LTRAP LJMPL LXORW
%token	<lval>	LNOP LEND LRETT LUNIMP LTEXT LDATA LRETRN
%token	<lval>	LCONST LSP LSB LFP LPC LCREG LFLUSH
%token	<lval>	LREG LFREG LR LC LF
%token	<lval>	LFSR LFPQ LPSR LSCHED
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg
%type	<gen>	addr rreg name psr creg freg
%type	<gen>	imm ximm fimm rel fsr fpq
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
 * B.1 load integer instructions
 */
	LMOVW rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.2 load floating instructions
 *	includes CSR
 */
|	LMOVD addr ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFMOV addr ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFMOV fimm ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFMOV freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFMOV freg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW addr ',' fsr
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.3 load coprocessor instructions
 *	excludes CSR
 */
|	LMOVW addr ',' creg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD addr ',' creg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.4 store integer instructions
 */
|	LMOVW rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW imm ',' addr
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB imm ',' addr
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.5 store floating instructions
 *	includes CSR and CQ
 */
|	LMOVW freg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD freg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW fsr ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD fpq ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.6 store coprocessor instructions
 *	excludes CSR and CQ
 */
|	LMOVW creg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD creg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.7 atomic load unsigned byte (TAS)
 * B.8 swap
 */
|	LSWAP addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.9  add instructions
 * B.10 tagged add instructions
 * B.11 subtract instructions
 * B.12 tagged subtract instructions
 * B.13 multiply step instruction
 * B.14 logical instructions
 * B.15 shift instructions
 * B.17 save/restore
 */
|	LADDW rreg ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LADDW imm ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LADDW rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LADDW imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LXORW rreg ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LXORW imm ',' sreg ',' rreg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LXORW rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LXORW imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.16 set hi
 *	other pseudo moves
 */
|	LMOVW imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW ximm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVD ximm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.18 branch on integer condition
 * B.19 floating point branch on condition
 * B.20 coprocessor branch on condition
 */
|	LBRA comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * B.21 call instruction
 * B.22 jump and link instruction
 */
|	LJMPL comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LJMPL comma addr
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LJMPL comma sreg ',' rel
	{
		outcode($1, &nullgen, $3, &$5);
	}
|	LJMPL comma sreg ',' addr
	{
		outcode($1, &nullgen, $3, &$5);
	}
/*
 * B.23 return from trap
 */
|	LRETT rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.28 instruction cache flush
 */
|	LFLUSH rel comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LFLUSH addr comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * B.24 trap on condition
 */
|	LTRAP rreg ',' sreg
	{
		outcode($1, &$2, $4, &nullgen);
	}
|	LTRAP imm ',' sreg
	{
		outcode($1, &$2, $4, &nullgen);
	}
|	LTRAP rreg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LTRAP comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * B.25 read state register instructions
 */
|	LMOVW psr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * B.26 write state register instructions BOTCH XOR
 */
|	LMOVW rreg ',' psr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW imm ',' psr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LXORW rreg ',' sreg ',' psr
	{
		outcode($1, &$2, $4, &$6);
	}
|	LXORW imm ',' sreg ',' psr
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * B.27 unimplemented trap
 */
|	LUNIMP	comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LUNIMP imm comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * B.29 floating point operate
 */
|	LFCONV freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFADD freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LFADD freg ',' freg ',' freg
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * B.30 coprocessor operate
 */
|	LCPOP creg ',' creg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LCPOP creg ',' creg ',' creg
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * CMP
 */
|	LCMP rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LCMP rreg ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * NOP
 */
|	LNOP comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LNOP rreg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LNOP freg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LNOP ',' rreg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LNOP ',' freg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * END
 */
|	LEND comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * TEXT/GLOBL
 */
|	LTEXT name ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTEXT name ',' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * DATA
 */
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
/*
 * RETURN
 */
|	LRETRN	comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
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

rreg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

fsr:
	LFSR
	{
		$$ = nullgen;
		$$.type = D_PREG;
		$$.reg = $1;
	}

fpq:
	LFPQ
	{
		$$ = nullgen;
		$$.type = D_PREG;
		$$.reg = $1;
	}

psr:
	LPSR
	{
		$$ = nullgen;
		$$.type = D_PREG;
		$$.reg = $1;
	}

creg:
	LCREG
	{
		$$ = nullgen;
		$$.type = D_CREG;
		$$.reg = $1;
	}
|	LC '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_CREG;
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

imm:	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}

sreg:
	LREG
|	LR '(' con ')'
	{
		if($$ < 0 || $$ >= NREG)
			print("register value out of range\n");
		$$ = $3;
	}

addr:
	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}
|	'(' sreg ',' con ')'
	{
		$$ = nullgen;
		$$.type = D_ASI;
		$$.reg = $2;
		$$.offset = $4;
	}
|	'(' sreg '+' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.xreg = $4;
		$$.offset = 0;
	}
|	name
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

comma:
|	','

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
