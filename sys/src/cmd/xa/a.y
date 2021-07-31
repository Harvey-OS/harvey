%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../xc/x.out.h"
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
%token	<lval>	LMOVW LMOVH LMOVHU
%token	<lval>	LFMOV LMOVA LBMOVW LDO LDOEND LJMP LBRA LDBRA LCALL
%token	<lval>	LADD LSH LRO LCMP LOP
%token	<lval>	LFADD LFMADD LFADDT LFMADDT LFOP LFDSP LFDIV
%token	<lval>	LWAITI LSFTRST
%token	<lval>	LNOP LEND LWORD LTEXT LDATA LIRET LRETRN LSCHED
%token	<lval>	LCONST LSP LSB LFP LPC LCREG LPCSH LCC
%token	<lval>	LREG LFREG LR LC LF
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg esreg fsreg
%type	<gen>	addr faddr rreg name freg ereg creg
%type	<gen>	one imm ximm fimm rel x
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
 * load integer instructions
 */
	LMOVW addr ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH addr ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU addr ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVW name ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH name ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU name ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH creg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * load immediate and address pseudo move
 */
|	LMOVW imm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVW ximm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * store integer instructions
 */
|	LMOVW ereg ',' addr
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH ereg ',' addr
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU ereg ',' addr
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVW ereg ',' name
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH ereg ',' name
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU ereg ',' name
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH ereg ',' creg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

|	LMOVW imm ',' addr
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH imm ',' addr
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU imm ',' addr
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVW imm ',' name
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH imm ',' name
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU imm ',' name
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH imm ',' creg
	{
		if($2.offset != 0)
			yyerror("constant must be zero");
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * move between registers
 */
|	LMOVW ereg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH ereg ',' rreg
	{
		if($2.reg == REGPC || $2.reg == REGPCSH)
			yyerror("register can't be PC or PCSH");
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVHU ereg ',' rreg
	{
		if($2.reg == REGPC || $2.reg == REGPCSH)
			yyerror("register can't be PC or PCSH");
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * load and store control
 */
|	LMOVH addr ',' creg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LMOVH creg ',' addr
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * add
 */
|	LADD rreg ',' sreg ',' rreg
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LADD LCC ',' rreg ',' sreg ',' rreg
	{
		outcode($1, $2, &$4, $6, &$8);
	}
|	LADD rreg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LADD LCC ',' rreg ',' rreg
	{
		outcode($1, $2, &$4, NREG, &$6);
	}
|	LADD LCC ',' one ',' sreg ',' rreg
	{
		outcode($1, $2, &$4, $6, &$8);
	}
|	LADD imm ',' esreg ',' rreg
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LADD imm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * shift left & right
 */
|	LSH rreg ',' sreg ',' rreg
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LSH LCC ',' rreg ',' sreg ',' rreg
	{
		outcode($1, $2, &$4, $6, &$8);
	}
|	LSH rreg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LSH LCC ',' rreg ',' rreg 
	{
		outcode($1, $2, &$4, NREG, &$6);
	}
|	LSH imm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LSH imm ',' sreg ',' rreg
	{
		if($2.offset != 1 && $4 != $6.reg)
			yyerror("constant must be 1");
		outcode($1, 0, &$2, $4, &$6);
	}
|	LSH LCC ',' one ',' sreg ',' rreg
	{
		outcode($1, $2, &$4, $6, &$8);
	}
|	LSH LCC ',' one ',' rreg
	{
		outcode($1, $2, &$4, NREG, &$6);
	}

/*
 * rotate left & right
 */
|	LRO rreg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LRO LCC ',' rreg ',' rreg
	{
		outcode($1, $2, &$4, NREG, &$6);
	}

/*
 * tests
 */
|	LCMP rreg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LCMP LCC ',' rreg ',' rreg
	{
		outcode($1, $2, &$4, NREG, &$6);
	}
|	LCMP imm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LCMP rreg ',' imm
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * most integer ops
 */
|	LOP rreg ',' sreg ',' rreg
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LOP LCC ',' rreg ',' sreg ',' rreg
	{
		outcode($1, $2, &$4, $6, &$8);
	}
|	LOP rreg ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LOP LCC ',' rreg ',' rreg
	{
		outcode($1, $2, &$4, NREG, &$6);
	}
|	LOP imm ',' rreg
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * branches, jumps, loop
 */
|	LJMP comma rel
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}
|	LBRA LCC ',' comma rel
	{
		outcode($1, $2, &nullgen, NREG, &$5);
	}
|	LDBRA rreg ',' rel
	{
		outcode($1, 0, &$2, NREG, &$4);
	}

/*
 * call
 */
|	LCALL comma rel
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}
|	LCALL comma name
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}
|	LCALL comma sreg ',' rel
	{
		outcode($1, 0, &nullgen, $3, &$5);
	}
|	LCALL comma sreg ',' name
	{
		outcode($1, 0, &nullgen, $3, &$5);
	}

/*
 * floating point ops
 */
|	LFDIV freg ',' freg ',' fsreg
	{
		outfcode($1, &$2, &$4, &nullgen, $6, &nullgen);
	}
|	LFADD x ',' x ',' fsreg
	{
		outfcode($1, &$2, &$4, &nullgen, $6, &nullgen);
	}
|	LFADD x ',' x ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &nullgen, $6, &$8);
	}
|	LFMADD freg ',' freg ',' freg ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD faddr ',' freg ',' freg ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD freg ',' faddr ',' freg ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD faddr ',' faddr ',' freg ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD freg ',' freg ',' faddr ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD faddr ',' freg ',' faddr ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD freg ',' faddr ',' faddr ',' fsreg
	{
		outfcode($1, &$2, &$4, &$6, $8, &nullgen);
	}
|	LFMADD freg ',' freg ',' freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD faddr ',' freg ',' freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD freg ',' faddr ',' freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD faddr ',' faddr ',' freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD freg ',' freg ',' faddr ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD faddr ',' freg ',' faddr ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}
|	LFMADD freg ',' faddr ',' faddr ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}

/*
 * floating tapped ops
 */
|	LFADDT x ',' x ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &nullgen, $6, &$8);
	}
|	LFMADDT x ',' x ',' freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &$4, &$6, $8, &$10);
	}

/*
 * misc floating point ops
 */
|	LFOP x ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFOP x ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}

/*
 * floating moves
 * immediate load synthesized by the loader
 */
|	LFMOV freg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, NREG, &$4);
	}
|	LFMOV freg ',' name
	{
		outfcode($1, &$2, &nullgen, &nullgen, NREG, &$4);
	}
|	LFMOV freg ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFMOV fimm ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFMOV faddr ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFMOV name ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFMOV freg ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}
|	LFMOV freg ',' fsreg ',' name
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}
|	LFMOV fimm ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}
|	LFMOV faddr ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}
|	LFMOV name ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}
|	LFMOV faddr ',' fsreg ',' name
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}

/*
 * dsp conversion
 */
|	LFDSP faddr ',' fsreg
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &nullgen);
	}
|	LFDSP faddr ',' fsreg ',' faddr
	{
		outfcode($1, &$2, &nullgen, &nullgen, $4, &$6);
	}

/*
 * one of a kind
 */
|	LIRET comma
	{
		outcode($1, 0, &nullgen, NREG, &nullgen);
	}
|	LSFTRST comma
	{
		outcode($1, 0, &nullgen, NREG, &nullgen);
	}
|	LWAITI comma
	{
		outcode($1, 0, &nullgen, NREG, &nullgen);
	}
|	LWORD imm
	{
		outcode($1, 0, &$2, NREG, &nullgen);
	}

/*
 * block move; kills F0
 */
|	LBMOVW imm ',' faddr ',' faddr
	{
		outfcode($1, &$2, &$4, &nullgen, NREG, &$6);
	}
|	LBMOVW rreg ',' faddr ',' faddr
	{
		outfcode($1, &$2, &$4, &nullgen, NREG, &$6);
	}
/*
 * DO
 */
|	LDO imm ',' imm
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LDO rreg ',' imm
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LDO imm ',' rel
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LDO rreg ',' rel
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LDOEND comma rel
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}

/*
 * RETURN
 */
|	LRETRN	comma
	{
		outcode($1, 0, &nullgen, NREG, &nullgen);
	}

/*
 * NOP
 */
|	LNOP comma
	{
		outcode($1, 0, &nullgen, NREG, &nullgen);
	}
|	LNOP rreg comma
	{
		outcode($1, 0, &$2, NREG, &nullgen);
	}
|	LNOP freg comma
	{
		outcode($1, 0, &$2, NREG, &nullgen);
	}
|	LNOP ',' rreg
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}
|	LNOP ',' freg
	{
		outcode($1, 0, &nullgen, NREG, &$3);
	}

/*
 * END
 */
|	LEND comma

/*
 * TEXT/GLOBL
 */
|	LTEXT name ',' imm
	{
		outcode($1, 0, &$2, NREG, &$4);
	}
|	LTEXT name ',' con ',' imm
	{
		outcode($1, 0, &$2, $4, &$6);
	}

/*
 * DATA
 */
|	LDATA name '/' con ',' imm
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' ximm
	{
		outcode($1, 0, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' fimm
	{
		outcode($1, 0, &$2, $4, &$6);
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
		$$.offset = $1->value + $2;
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

rreg:	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

ereg:	esreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

esreg:	sreg
|	LPC
	{
		$$ = REGPC;
	}
|	LPCSH
	{
		$$ = REGPCSH;
	}

x:	freg
|	faddr

freg:	fsreg
	{
		$$ = nullgen;
		$$.type = D_FREG;
		$$.reg = $1;
	}

fsreg:
	LFREG
|	LF '(' con ')'
	{
		if($3 < 0 || $3 >= NFREG)
			yyerror("register value out of range");
		$$ = $3;
	}

creg:	LCREG
	{
		$$ = nullgen;
		$$.type = D_CREG;
		$$.reg = $1;
	}
|	LC '(' con ')'
	{
		if($3 < 0 || $3 >= NCREG)
			yyerror("control register value out of range");
		$$ = nullgen;
		$$.type = D_CREG;
		$$.reg = $3;
	}

ximm:
	'$' name
	{
		$$ = $2;
		$$.type = D_CONST;
	}
|	'$' '$' LFCONST
	{
		$$ = nullgen;
		$$.type = D_AFCONST;
		$$.dval = $3;
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

one:	'$' con
	{
		if($2 != 1)
			yyerror("constant must be one");
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = 1;
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
		if($3 < 0 || $3 >= NREG)
			yyerror("register value out of range");
		$$ = $3;
	}

faddr: addr
	{
		if($1.reg == 0 || $1.reg > 14)
			yyerror("register R%d used as a pointer", $1.reg);
		if($1.type == D_INCREG && ($1.offset <= 14 || $1.offset >= 20))
			yyerror("register R%d used for increment", $1.offset);
		$$ = $1;
	}

addr:
	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_INDREG;
		$$.reg = $2;
	}
|	con '(' sreg ')'
	{
		if($1 != 0)
			yyerror("constant must be zero");
		$$ = nullgen;
		$$.type = D_INDREG;
		$$.reg = $3;
	}
|	'(' sreg ')' '-'
	{
		$$ = nullgen;
		$$.type = D_DEC;
		$$.reg = $2;
	}
|	'(' sreg ')' '+'
	{
		$$ = nullgen;
		$$.type = D_INC;
		$$.reg = $2;
	}
|	'(' sreg ')' '+' sreg
	{
		$$ = nullgen;
		$$.type = D_INCREG;
		$$.reg = $2;
		$$.offset = $5;
	}

name:
	con '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_NAME;
		$$.name = $3;
		$$.sym = S;
		$$.offset = $1;
	}
|	LNAME offset '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_NAME;
		$$.name = $4;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_NAME;
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
