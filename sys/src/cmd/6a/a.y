%{
#include "a.h"
%}
%union	{
	Sym	*sym;
	vlong	lval;
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
%token	<lval>	LTYPE0 LTYPE1 LTYPE2 LTYPE3 LTYPE4
%token	<lval>	LTYPEC LTYPED LTYPEN LTYPER LTYPET LTYPES LTYPEM LTYPEI LTYPEG LTYPEXC LTYPEX LTYPEY LTYPERT
%token	<lval>	LCONST LFP LPC LSB
%token	<lval>	LBREG LLREG LSREG LFREG LMREG LXREG LYREG
%token	<dval>	LFCONST
%token	<sval>	LSCONST LSP
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset
%type	<gen>	mem imm reg nam rel rem rim rom omem nmem
%type	<gen2>	nonnon nonrel nonrem rimnon rimrem remrim
%type	<gen2>	spec1 spec2 spec3 spec4 spec5 spec6 spec7 spec8 spec9 spec10 spec11 spec12
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
|	LTYPE0 nonnon	{ outcode($1, &$2); }
|	LTYPE1 nonrem	{ outcode($1, &$2); }
|	LTYPE2 rimnon	{ outcode($1, &$2); }
|	LTYPE3 rimrem	{ outcode($1, &$2); }
|	LTYPE4 remrim	{ outcode($1, &$2); }
|	LTYPER nonrel	{ outcode($1, &$2); }
|	LTYPED spec1	{ outcode($1, &$2); }
|	LTYPET spec2	{ outcode($1, &$2); }
|	LTYPEC spec3	{ outcode($1, &$2); }
|	LTYPEN spec4	{ outcode($1, &$2); }
|	LTYPES spec5	{ outcode($1, &$2); }
|	LTYPEM spec6	{ outcode($1, &$2); }
|	LTYPEI spec7	{ outcode($1, &$2); }
|	LTYPEXC spec8	{ outcode($1, &$2); }
|	LTYPEX spec9	{ outcode($1, &$2); }
|	LTYPEG spec10	{ outcode($1, &$2); }
|	LTYPEY spec11	{ outcode($1, &$2); }
|	LTYPERT spec12	{ outcode($1, &$2); }

nonnon:
	{
		$$.from = nullgen;
		$$.to = nullgen;
	}
|	','
	{
		$$.from = nullgen;
		$$.to = nullgen;
	}

rimrem:
	rim ',' rem
	{
		$$.from = $1;
		$$.to = $3;
	}

remrim:
	rem ',' rim
	{
		$$.from = $1;
		$$.to = $3;
	}

rimnon:
	rim ','
	{
		$$.from = $1;
		$$.to = nullgen;
	}
|	rim
	{
		$$.from = $1;
		$$.to = nullgen;
	}

nonrem:
	',' rem
	{
		$$.from = nullgen;
		$$.to = $2;
	}
|	rem
	{
		$$.from = nullgen;
		$$.to = $1;
	}

nonrel:
	',' rel
	{
		$$.from = nullgen;
		$$.to = $2;
	}
|	rel
	{
		$$.from = nullgen;
		$$.to = $1;
	}

spec1:	/* DATA */
	nam '/' con ',' imm
	{
		$$.from = $1;
		$$.from.scale = $3;
		$$.to = $5;
	}

spec2:	/* TEXT */
	mem ',' imm
	{
		$$.from = $1;
		$$.to = $3;
	}
|	mem ',' con ',' imm
	{
		$$.from = $1;
		$$.from.scale = $3;
		$$.to = $5;
	}

spec3:	/* JMP/CALL */
	',' rom
	{
		$$.from = nullgen;
		$$.to = $2;
	}
|	rom
	{
		$$.from = nullgen;
		$$.to = $1;
	}

spec4:	/* NOP */
	nonnon
|	nonrem

spec5:	/* SHL/SHR */
	rim ',' rem
	{
		$$.from = $1;
		$$.to = $3;
	}
|	rim ',' rem ':' LLREG
	{
		$$.from = $1;
		$$.to = $3;
		if($$.from.index != D_NONE)
			yyerror("dp shift with lhs index");
		$$.from.index = $5;
	}

spec6:	/* MOVW/MOVL */
	rim ',' rem
	{
		$$.from = $1;
		$$.to = $3;
	}
|	rim ',' rem ':' LSREG
	{
		$$.from = $1;
		$$.to = $3;
		if($$.to.index != D_NONE)
			yyerror("dp move with lhs index");
		$$.to.index = $5;
	}

spec7:
	rim ','
	{
		$$.from = $1;
		$$.to = nullgen;
	}
|	rim
	{
		$$.from = $1;
		$$.to = nullgen;
	}
|	rim ',' rem
	{
		$$.from = $1;
		$$.to = $3;
	}

spec8:	/* CMPPS/CMPPD */
	reg ',' rem ',' con
	{
		$$.from = $1;
		$$.to = $3;
		$$.from.offset = $5;
	}
|	reg ',' reg ',' rem ',' con	/* VCMPPS/VCMPPD */
	{
		$$.from = $1;
		if(!isxyreg($3.type))
			yyerror("second source operand must be X/Y register");
		$$.from.index = $3.type;
		$$.to = $5;
		$$.from.offset = $7;
	}

spec9:	/* SHUFL */
	imm ',' rem ',' reg
	{
		$$.from = $3;
		$$.to = $5;
		if($1.type != D_CONST)
			yyerror("illegal constant");
		$$.to.offset = $1.offset;
	}
|	imm ',' rem ',' reg ',' reg
	{
		$$.from = $3;
		$$.to = $7;
		if($1.type != D_CONST)
			yyerror("illegal constant");
		$$.to.offset = $1.offset;
		if(!isxyreg($5.type))
			yyerror("second source operand must be X/Y register");
		$$.to.index = $5.type;
	}

spec10:	/* GLOBL */
	mem ',' imm
	{
		$$.from = $1;
		$$.to = $3;
	}
|	mem ',' con ',' imm
	{
		$$.from = $1;
		$$.from.scale = $3;
		$$.to = $5;
	}

spec11:
	rimrem
|	rim ',' reg ',' rem
	{
		$$.from = $1;
		$$.to = $5;
		if(isxyreg($3.type)) {
			if(isxyreg($1.type))
				$$.from.index = $3.type;
			else if(isxyreg($5.type))
				$$.to.index = $3.type;
		} else
			yyerror("second source operand must be X or Y register");
	}

spec12:	/* RET/RETF */
	{
		$$.from = nullgen;
		$$.to = nullgen;
	}
|	imm
	{
		$$.from = $1;
		$$.to = nullgen;
	}

rem:
	reg
|	mem

rom:
	rel
|	nmem
|	'*' reg
	{
		$$ = $2;
	}
|	'*' omem
	{
		$$ = $2;
	}
|	reg
|	omem
|	imm

rim:
	rem
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

reg:
	LBREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LFREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LLREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LMREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LSP
	{
		$$ = nullgen;
		$$.type = D_SP;
	}
|	LSREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LXREG
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	LYREG
	{
		$$ = nullgen;
		$$.type = $1;
	}

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
		/*
		if($2.type == D_AUTO || $2.type == D_PARAM)
			yyerror("constant cannot be automatic: %s",
				$2.sym->name);
		 */
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
|	'$' '(' LFCONST ')'
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = $3;
	}
|	'$' '-' LFCONST
	{
		$$ = nullgen;
		$$.type = D_FCONST;
		$$.dval = -$3;
	}

mem:
	omem
|	nmem

omem:
	con
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.offset = $1;
	}
|	con '(' LLREG ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$3;
		$$.offset = $1;
	}
|	con '(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_SP;
		$$.offset = $1;
	}
|	con '(' LLREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.offset = $1;
		$$.index = $3;
		$$.scale = $5;
		checkscale($$.scale);
	}
|	con '(' LLREG ')' '(' LLREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$3;
		$$.offset = $1;
		$$.index = $6;
		$$.scale = $8;
		checkscale($$.scale);
	}
|	'(' LLREG ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$2;
	}
|	'(' LSP ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_SP;
	}
|	con '(' LSREG ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$3;
		$$.offset = $1;
	}
|	'(' LLREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+D_NONE;
		$$.index = $2;
		$$.scale = $4;
		checkscale($$.scale);
	}
|	'(' LLREG ')' '(' LLREG '*' con ')'
	{
		$$ = nullgen;
		$$.type = D_INDIR+$2;
		$$.index = $5;
		$$.scale = $7;
		checkscale($$.scale);
	}

nmem:
	nam
	{
		$$ = $1;
	}
|	nam '(' LLREG '*' con ')'
	{
		$$ = $1;
		$$.index = $3;
		$$.scale = $5;
		checkscale($$.scale);
	}

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
	{
		$$ = D_AUTO;
	}
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
