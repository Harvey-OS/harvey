%{
#include "a.h"
%}
%union
{
	Sym	*sym;
	vlong	lval;
	double	dval;
	char	sval[NSNAME];
	Gen	gen;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LTYPE0 LTYPE1 LTYPE2 LTYPE3 LTYPE4 LTYPE5
%token	<lval>	LTYPE6 LTYPE7 LTYPE8 LTYPE9 LTYPEA
%token	<lval>	LTYPEB LTYPEC LTYPED LTYPEE LTYPEF
%token	<lval>	LTYPEG LTYPEH LTYPEI LTYPEJ LTYPEK
%token	<lval>	LTYPEL LTYPEM LTYPEN LTYPEO LTYPEP LTYPEQ
%token	<lval>	LTYPER LTYPES LTYPET LTYPEU LTYPEV LTYPEW LTYPEX LTYPEY LTYPEZ
%token	<lval>	LMOVK LDMB LSTXR
%token	<lval>	LCONST LSP LSB LFP LPC
%token	<lval>	LR LREG LF LFREG LV LVREG LC LCREG LFCR LFCSEL
%token	<lval>	LCOND LS LAT LEXT LSPR LSPREG LVTYPE
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg spreg
%type	<lval>	scon indexreg vset vreglist
%type	<gen>	gen rel reg freg vreg shift fcon frcon extreg vlane vgen
%type	<gen>	imm ximm name oreg nireg ioreg imsr spr cond sysarg
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
 * ERET
 */
	LTYPE0 comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * ADD
 */
|	LTYPE1 imsr ',' spreg ',' reg
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTYPE1 imsr ',' spreg ','
	{
		outcode($1, &$2, $4, &nullgen);
	}
|	LTYPE1 imsr ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * CLS
 */
|	LTYPE2 imsr ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * MOV
 */
|	LTYPE3 gen ',' gen
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * MOVK
 */
|	LMOVK imm ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVK imm '<' '<' con ',' reg
	{
		Gen g;
		g = nullgen;
		g.type = D_CONST;
		g.offset = $5;
		outcode4($1, &$2, NREG, &g, &$7);
	}
/*
 * B/BL
 */
|	LTYPE4 comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE4 comma nireg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * BEQ
 */
|	LTYPE5 comma rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * SVC
 */
|	LTYPE6 comma gen
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPE6
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * CMP
 */
|	LTYPE7 imsr ',' spreg comma
	{
		outcode($1, &$2, $4, &nullgen);
	}
/*
 * CBZ
 */
|	LTYPE8 reg ',' rel
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * CSET
 */
|	LTYPER cond ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * CSEL/CINC/CNEG/CINV
 */
|	LTYPES cond ',' reg ',' reg ',' reg
	{
		outcode4($1, &$2, $6.reg, &$4, &$8);
	}
|	LTYPES cond ',' reg ',' reg
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * TBZ
 */
|	LTYPET imm ',' reg ',' rel
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * CCMN
 */
|	LTYPEU cond ',' imsr ',' reg ',' imm comma
	{
		outcode4($1, &$2, $6.reg, &$4, &$8);
	}
/*
 * ADR
 */
|	LTYPEV rel ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEV '$' name ',' reg
	{
		outcode($1, &$3, NREG, &$5);
	}
/*
 * BFM/BFI
 */
|	LTYPEY imm ',' imm ',' spreg ',' reg
	{
		outcode4($1, &$2, $6, &$4, &$8);
	}
/*
 * EXTR
 */
|	LTYPEP imm ',' reg ',' spreg ',' reg
	{
		outcode4($1, &$2, $6, &$4, &$8);
	}
/*
 * RET/RETURN
 */
|	LTYPEA comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LTYPEA reg
	{
		outcode($1, &nullgen, NREG, &$2);
	}
/*
 * NOP
 */
|	LTYPEQ comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LTYPEQ reg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LTYPEQ freg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LTYPEQ ',' reg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LTYPEQ ',' freg
	{
		outcode($1, &nullgen, NREG, &$3);
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
 * CASE
 */
|	LTYPED reg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * word
 */
|	LTYPEH comma ximm
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * floating-point
 */
|	LTYPEI freg ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * FADDD
 */
|	LTYPEK frcon ',' freg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEK frcon ',' freg ',' freg
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * FCMP
 */
|	LTYPEL frcon ',' freg comma
	{
		outcode($1, &$2, $4.reg, &nullgen);
	}
/*
 * FCCMP
 */
|	LTYPEF cond ',' freg ',' freg ',' imm comma
	{
		outcode4($1, &$2, $6.reg, &$4, &$8);
	}
/*
 * FMULA
 */
|	LTYPE9 freg ',' freg ', ' freg ',' freg comma
	{
		outcode4($1, &$2, $4.reg, &$6, &$8);
	}
/*
 * FCSEL
 */
|	LFCSEL cond ',' freg ',' freg ',' freg
	{
		outcode4($1, &$2, $6.reg, &$4, &$8);
	}
/*
 * SIMD
 */
|	LTYPEW vgen ',' vgen
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTYPEW vgen ',' vgen ',' vgen
	{
		outcode($1, &$2, $4.reg, &$6);
	}
/*
 * MOVP/MOVNP
 */
|	LTYPEJ gen ',' sreg ',' gen
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * MADD Rn,Rm,Ra,Rd
 */
|	LTYPEM reg ',' reg ',' sreg ',' reg
	{
		outcode4($1, &$2, $6, &$4, &$8);
	}
/*
 * SYS/SYSL
 */
|	LTYPEN sysarg
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LTYPEN reg ',' sysarg
	{
		outcode($1, &$4, $2.reg, &nullgen);
	}
|	LTYPEO sysarg ',' reg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * DMB, HINT
 */
|	LDMB imm
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * STXR
 */
|	LSTXR reg ',' gen ',' sreg
	{
		outcode($1, &$2, $6, &$4);
	}
/*
 * END
 */
|	LTYPEE comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}

cond:
	LCOND
	{
		$$ = nullgen;
		$$.type = D_COND;
		$$.reg = $1;
	}

comma:
|	',' comma

sysarg:
	con ',' con ',' con ',' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = SYSARG4($1, $3, $5, $7);
	}
|	imm

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
		memmove($$.sval, $2, sizeof($$.sval));
	}
|	fcon

fcon:
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

gen:
	reg
|	ximm
|	LFCR
	{
		$$ = nullgen;
		$$.type = D_SPR;
		$$.offset = $1;
	}
|	con
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.offset = $1;
	}
|	oreg
|	freg
|	vreg
|	spr

nireg:
	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}
|	name
	{
		$$ = $1;
		if($1.name != D_EXTERN && $1.name != D_STATIC) {
		}
	}

oreg:
	name
|	name '(' sreg ')'
	{
		$$ = $1;
		$$.type = D_OREG;
		$$.reg = $3;
	}
|	ioreg

ioreg:
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
|	con '(' sreg ')' '!'
	{
		$$ = nullgen;
		$$.type = D_XPRE;
		$$.reg = $3;
		$$.offset = $1;
	}
|	'(' sreg ')' con '!'
	{
		$$ = nullgen;
		$$.type = D_XPOST;
		$$.reg = $2;
		$$.offset = $4;
	}
|	'(' sreg ')' '(' indexreg ')'
	{
		$$ = nullgen;
		$$.type = D_ROFF;
		$$.reg = $2;
		$$.xreg = $5 & 0x1f;
		$$.offset = $5;
	}
|	'(' sreg ')' '[' indexreg ']'
	{
		$$ = nullgen;
		$$.type = D_ROFF;
		$$.reg = $2;
		$$.xreg = $5 & 0x1f;
		$$.offset = $5 | (1<<16);
	}

imsr:
	imm
|	shift
|	extreg

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
|	LSP
	{
		$$ = nullgen;
		$$.type = D_SP;
		$$.reg = REGSP;
	}

shift:
	sreg '<' '<' scon
	{
		$$ = nullgen;
		$$.type = D_SHIFT;
		$$.offset = ($1 << 16) | ($4 << 10) | (0 << 22);
	}
|	sreg '>' '>' scon
	{
		$$ = nullgen;
		$$.type = D_SHIFT;
		$$.offset = (($1&0x1F) << 16) | ($4 << 10) | (1 << 22);
	}
|	sreg '-' '>' scon
	{
		$$ = nullgen;
		$$.type = D_SHIFT;
		$$.offset = ($1 << 16) | ($4 << 10) | (2 << 22);
	}
|	sreg LAT '>' scon
	{
		$$ = nullgen;
		$$.type = D_SHIFT;
		$$.offset = ($1 << 16) | ($4 << 10) | (3 << 22);
	}

extreg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}
|	sreg LEXT
	{
		$$ = nullgen;
		$$.type = D_EXTREG;
		$$.reg = $1;
		$$.offset = ($1 << 16) | ($2 << 13);
	}
|	sreg LEXT '<' '<' con
	{
		if($5 < 0 || $5 > 4)
			yyerror("shift value out of range");
		$$ = nullgen;
		$$.type = D_EXTREG;
		$$.reg = $1;
		$$.offset = ($1 << 16) | ($2 << 13) | ($5 << 10);
	}

indexreg:
	sreg
	{
		$$ = (3 << 8) | $1;
	}
|	sreg LEXT
	{
		$$ = ($2 << 8) | $1;
	}

scon:
	con
	{
		if($$ < 0 || $$ >= 64)
			yyerror("shift value out of range");
		$$ = $1&0x3F;
	}

sreg:
	LREG
|	LR '(' expr ')'
	{
		if($3 < 0 || $3 >= NREG)
			print("register value out of range\n");
		$$ = $3;
	}

spreg:
	sreg
|	LSP
	{
		$$ = REGSP;
	}

spr:
	LSPREG
	{
		$$ = nullgen;
		$$.type = D_SPR;
		$$.offset = $1;
	}
|	LSPR '(' con ')'
	{
		$$ = nullgen;
		$$.type = $1;
		$$.offset = $3;
	}

frcon:
	freg
|	fcon

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

vgen:
	oreg
|	vreg
|	vlane
|	vset
	{
		$$ = nullgen;
		$$.type = D_VSET;
		$$.offset = $1;
	}

vlane:
	vreg '[' con ']'
	{
		$$.type = D_VLANE;
		$$.offset = $3;
	}
|	vset '[' con ']'
	{
		$$.type = D_VLANE;
		$$.offset = $3;
	}

vset:
	'{' vreglist '}'
	{
		$$ = $2;
	}

vreglist:
	vreg
	{
		$$ = 1 << $1.reg;
	}
|	vreg '-' vreg
	{
		int i;
		$$=0;
		for(i=$1.reg; i<=$3.reg; i++)
			$$ |= 1<<i;
		for(i=$3.reg; i<=$1.reg; i++)
			$$ |= 1<<i;
	}
|	vreg comma vreglist
	{
		$$ = (1<<$1.reg) | $3;
	}

vreg:
	LVREG
	{
		$$ = nullgen;
		$$.type = D_VREG;
		$$.reg = $1;
		/* TO DO: slice */
	}
|	LV '(' con ')'
	{
		$$ = nullgen;
		$$.type = D_VREG;
		$$.reg = $3;
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
