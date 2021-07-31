%{
#include "a.h"
%}
%union	{
	Sym	*sym;
	long	lval;
	double	dval;
	char	sval[8];
	Addr	addr;
	Gen	gen;
	Gen2	gen2;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LTYPE1 LTYPE2 LTYPE3 LTYPE4 LTYPE5
%token	<lval>	LTYPE6 LTYPE7 LTYPE8 LTYPE9 LTYPEA LTYPEB
%token	<lval>	LCONST LSP LSB LFP LPC LTOS LAREG LDREG LFREG
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr type pointer reg offset
%type	<addr>	name areg
%type	<gen>	gen rel
%type	<gen2>	noaddr gengen dstgen spec1 spec2 spec3 srcgen dstrel genrel
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
|	LTYPE1 gengen	{ outcode($1, &$2); }
|	LTYPE2 noaddr	{ outcode($1, &$2); }
|	LTYPE3 dstgen	{ outcode($1, &$2); }
|	LTYPE4 spec1	{ outcode($1, &$2); }
|	LTYPE5 srcgen	{ outcode($1, &$2); }
|	LTYPE6 dstrel	{ outcode($1, &$2); }
|	LTYPE7 genrel	{ outcode($1, &$2); }
|	LTYPE8 dstgen	{ outcode($1, &$2); }
|	LTYPE8 gengen	{ outcode($1, &$2); }
|	LTYPE9 noaddr	{ outcode($1, &$2); }
|	LTYPE9 dstgen	{ outcode($1, &$2); }
|	LTYPEA spec2	{ outcode($1, &$2); }
|	LTYPEB spec3	{ outcode($1, &$2); }

noaddr:
	{
		$$.from = nullgen;
		$$.to = nullgen;
	}
|	','
	{
		$$.from = nullgen;
		$$.to = nullgen;
	}

srcgen:
	gen
	{
		$$.from = $1;
		$$.to = nullgen;
	}
|	gen ','
	{
		$$.from = $1;
		$$.to = nullgen;
	}

dstgen:
	gen
	{
		$$.from = nullgen;
		$$.to = $1;
	}
|	',' gen
	{
		$$.from = nullgen;
		$$.to = $2;
	}

gengen:
	gen ',' gen
	{
		$$.from = $1;
		$$.to = $3;
	}

dstrel:
	rel
	{
		$$.from = nullgen;
		$$.to = $1;
	}
|	',' rel
	{
		$$.from = nullgen;
		$$.to = $2;
	}

genrel:
	gen ',' rel
	{
		$$.from = $1;
		$$.to = $3;
	}

spec1:	/* DATA opcode */
	gen '/' con ',' gen
	{
		$1.displace = $3;
		$$.from = $1;
		$$.to = $5;
	}

spec2:	/* bit field opcodes */
	gen ',' gen ',' con ',' con
	{
		$1.field = $7;
		$3.field = $5;
		$$.from = $1;
		$$.to = $3;
	}

spec3:	/* TEXT opcode */
	gengen
|	gen ',' con ',' gen
	{
		$1.displace = $3;
		$$.from = $1;
		$$.to = $5;
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

gen:
	type
	{
		$$ = nullgen;
		$$.type = $1;
	}
|	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}
|	'$' name
	{
		$$ = nullgen;
		{
			Addr *a;
			a = &$$;
			*a = $2;
		}
		if($2.type == D_AUTO || $2.type == D_PARAM)
			yyerror("constant cannot be automatic: %s",
				$2.sym->name);
		$$.type = $2.type | I_ADDR;
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
|	LTOS '+' con
	{
		$$ = nullgen;
		$$.type = D_STACK;
		$$.offset = $3;
	}
|	LTOS '-' con
	{
		$$ = nullgen;
		$$.type = D_STACK;
		$$.offset = -$3;
	}
|	con
	{
		$$ = nullgen;
		$$.type = D_CONST | I_INDIR;
		$$.offset = $1;
	}
|	'-' '(' LAREG ')'
	{
		$$ = nullgen;
		$$.type = $3 | I_INDDEC;
	}
|	'(' LAREG ')' '+'
	{
		$$ = nullgen;
		$$.type = $2 | I_INDINC;
	}
|	areg
	{
		$$ = nullgen;
		$$.type = $1.type;
		{
			Addr *a;
			a = &$$;
			*a = $1;
		}
	}

type:
	reg
|	LFREG

reg:
	LAREG
|	LDREG
|	LTOS

areg:
	'(' LAREG ')'
	{
		$$.type = $2 | I_INDIR;
		$$.sym = S;
		$$.offset = 0;
	}
|	con '(' LAREG ')'
	{
		$$.type = $3 | I_INDIR;
		$$.sym = S;
		$$.offset = $1;
	}
|	'(' ')'
	{
		$$.type = D_NONE | I_INDIR;
		$$.sym = S;
		$$.offset = 0;
	}
|	con '(' ')'
	{
		$$.type = D_NONE | I_INDIR;
		$$.sym = S;
		$$.offset = $1;
	}
|	name

name:
	LNAME offset '(' pointer ')'
	{
		$$.type = $4;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
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
