/***** spin: spin.y *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

%{
#include "spin.h"
#include <stdarg.h>

#define YYDEBUG	0
#define Stop	nn(ZN,'@',ZN,ZN)

extern  Symbol	*context, *owner;
extern  int	u_sync, u_async, dataflow;
extern	int	has_sorted, has_random, has_enabled;

static	int	Embedded = 0, Expand_Ok = 0;
int	Mpars = 0;		/* max nr of message parameters  */
char	*claimproc = (char *) 0;
%}

%token	ASSERT PRINT
%token	RUN LEN ENABLED PC_VAL
%token	TYPEDEF MTYPE LABEL OF
%token	GOTO BREAK ELSE SEMI
%token	IF FI DO OD SEP
%token	ATOMIC NON_ATOMIC D_STEP UNLESS
%token	SND O_SND RCV R_RCV TIMEOUT
%token	ACTIVE PROCTYPE HIDDEN
%token	FULL EMPTY NFULL NEMPTY

%token	CONST TYPE XU				/* val */
%token	NAME UNAME PNAME CLAIM STRING INIT	/* sym */

/*
fields in struct Lextok used by non-terminals:
	sym:	vardcl ivar
	seq:	option body
	seql:	options
	node:	basetype expr Probe Expr var_list vref_lst stmnt
		nlst args arg typ_list decl decl_lst one_decl
		prargs margs varref step vis ch_init sfld	
*/

%right	ASGN
%left	SND O_SND RCV R_RCV
%left	OR
%left	AND
%left	'|'
%left	'&'
%left	EQ NE
%left	GT LT GE LE
%left	LSHIFT RSHIFT
%left	'+' '-'
%left	'*' '/' '%'
%left	INCR DECR
%right	'~' UMIN NEG
%left	DOT
%%

/** PROMELA2 Grammar Rules **/

program	: units		{ extern char yytext[];
			  yytext[0] = '\0';
			  if (!dataflow)
				sched();
			}
	;

units	: unit
	| units unit
	;

unit	: proc		/* proctype { }       */
	| init		/* init { }           */
	| claim		/* never claim        */
	| one_decl	/* variables, chans   */
	| mtype		/* message names      */
	| utype		/* user defined types */
	| SEMI		/* optional separator */
	| error
	;

proc	: inst		/* optional instantiator */
	  PROCTYPE NAME	{ setptype($3, PROCTYPE, ZN);
			  setpname($3);
			  context = $3->sym;
			  Expand_Ok++; /* expand struct names in decl */
			}
	  '(' decl ')'	{ Expand_Ok--; }
	  body		{ ProcList *rl = ready($3->sym, $6, $9->sq);
			  if ($1 != ZN && $1->val > 0)
			  {	int j;
			  	for (j = 0; j < $1->val; j++)
				runnable(rl);
			  }
			  varcheck($9->sq->frst, $9->sq->frst->nxt);
			  context = ZS;
			}
	;

inst	: /* empty */	{ $$ = ZN; }
	| ACTIVE	{ $$ = nn(ZN,CONST,ZN,ZN); $$->val = 1; }
	| ACTIVE '[' CONST ']' {
			  $$ = nn(ZN,CONST,ZN,ZN); $$->val = $3->val;
			}
	;

init	: INIT		{ context = $1->sym; }
	  body		{ runnable(ready(context, ZN, $3->sq));
			  varcheck($3->sq->frst, $3->sq->frst->nxt);
			  context = ZS;
        		}
	;

claim	: CLAIM		{ context = $1->sym;
			  if (claimproc)
				non_fatal("claim %s redefined", claimproc);
			  claimproc = $1->sym->name;
			}
	  body		{ (void) ready($1->sym, ZN, $3->sq);
        		  context = ZS;
        		}
	;

mtype   : MTYPE ASGN '{' nlst '}' { setmtype($4); }
	| MTYPE '{' nlst '}'	  { setmtype($3); /* missing '=' */ }
	;
utype	: TYPEDEF NAME		{ if (context)
				   fatal("typedef %s must be global",
						$2->sym->name);
				   owner = $2->sym;
				}
	  '{' decl_lst '}'	{ setuname($5); owner = ZS; }
	;

body	: '{'			{ open_seq(1); }
          sequence OS		{ add_seq(Stop); }
          '}'			{ $$->sq = close_seq(0); }
	;

sequence: step			{ if ($1) add_seq($1); }
	| sequence MS step	{ if ($3) add_seq($3); }
	;

step    : one_decl		{ $$ = ZN; }
	| XU vref_lst		{ setxus($2, $1->val); $$ = ZN; }
	| stmnt			{ $$ = $1; }
	| stmnt UNLESS stmnt	{ $$ = do_unless($1, $3); }
	;

vis	: /* empty */		{ $$ = ZN; }
	| HIDDEN		{ $$ = $1; }
	;

one_decl: vis TYPE var_list	{ setptype($3, $2->val, $1); $$ = $3; }
	| vis UNAME var_list	{ setutype($3, $2->sym, $1);
				  $$ = expand($3, Expand_Ok);
				}
	;

decl_lst: one_decl       	{ $$ = nn(ZN, ',', $1, ZN); }
	| one_decl SEMI
	  decl_lst		{ $$ = nn(ZN, ',', $1, $3); }
	;

decl    : /* empty */		{ $$ = ZN; }
	| decl_lst      	{ $$ = $1; }
	;

vref_lst: varref		{ $$ = nn($1, XU, $1, ZN); }
	| varref ',' vref_lst	{ $$ = nn($1, XU, $1, $3); }
	;

var_list: ivar           	{ $$ = nn($1, TYPE, ZN, ZN); }
	| ivar ',' var_list	{ $$ = nn($1, TYPE, ZN, $3); }
	;

ivar    : vardcl           	{ $$ = $1; }
	| vardcl ASGN expr   	{ $1->sym->ini = $3; $$ = $1; }
	| vardcl ASGN ch_init	{ $1->sym->ini = $3; $$ = $1; }
	;

ch_init : '[' CONST ']' OF
	  '{' typ_list '}'	{ if ($2->val) u_async++;
				  else u_sync++;
        			  {	int i = cnt_mpars($6);
					Mpars = max(Mpars, i);
				  }
        			  $$ = nn(ZN, CHAN, ZN, $6);
				  $$->val = $2->val;
        			}
	;

vardcl  : NAME  		{ $1->sym->nel =  1; $$ = $1; }
	| NAME '[' CONST ']'	{ $1->sym->nel = $3->val; $$ = $1; }
	;

varref	: cmpnd			{ $$ = mk_explicit($1, Expand_Ok, NAME); }
	;

pfld	: NAME			{ $$ = nn($1, NAME, ZN, ZN); }
	| NAME			{ owner = ZS; }
	  '[' expr ']'		{ $$ = nn($1, NAME, $4, ZN); }
	;

cmpnd	: pfld			{ Embedded++;
				  if ($1->sym->type == STRUCT)
					owner = $1->sym->Snm;
				}
	  sfld			{ $$ = $1; $$->rgt = $3;
				  if ($3 && $1->sym->type != STRUCT)
					$1->sym->type = STRUCT;
				  if (!Embedded && !$1->sym->type)
				   non_fatal("undeclared variable: %s",
						$1->sym->name);
				  Embedded--;
				  owner = ZS; 
				}
	;

sfld	: /* empty */		{ $$ = ZN; }
	| '.' cmpnd %prec DOT	{ $$ = nn(ZN, '.', $2, ZN); }
	;

stmnt	: varref ASGN expr	{ $$ = nn($1, ASGN, $1, $3); }
	| varref INCR		{ $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
				  $$ = nn(ZN,  '+', $1, $$);
				  $$ = nn($1, ASGN, $1, $$);
				}
	| varref DECR		{ $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
				  $$ = nn(ZN,  '-', $1, $$);
				  $$ = nn($1, ASGN, $1, $$);
				}
	| varref RCV		{ Expand_Ok++; }
	  rargs			{ Expand_Ok--;
				  $$ = nn($1,  'r', $1, $4);
				}
	| varref SND		{ Expand_Ok++; }
	  margs			{ Expand_Ok--;
				  $$ = nn($1, 's', $1, $4); $$->val=0;
				}
	| varref R_RCV		{ Expand_Ok++; }
	  rargs			{ Expand_Ok--;
				  $$ = nn($1,  'r', $1, $4);
				  $$->val = has_random = 1;
				}
	| varref O_SND		{ Expand_Ok++; }
	  margs			{ Expand_Ok--;
				  $$ = nn($1, 's', $1, $4);
				  $$->val = has_sorted = 1;
				}
	| PRINT '(' STRING prargs ')' { $$ = nn($3, PRINT, $4, ZN); }
	| ASSERT Expr    	{ $$ = nn(ZN, ASSERT, $2, ZN); }
	| ASSERT expr    	{ $$ = nn(ZN, ASSERT, $2, ZN); }
	| GOTO NAME		{ $$ = nn($2, GOTO, ZN, ZN);
				  if ($2->sym->type != 0
				  &&  $2->sym->type != LABEL) {
				  non_fatal("bad label-name %s", $2->sym->name);
				  }
				  $2->sym->type = LABEL;
				}
	| Expr			{ $$ = nn(ZN, 'c', $1, ZN); }
	| expr			{ $$ = nn(ZN, 'c', $1, ZN); }
	| NAME ':' stmnt	{ $$ = nn($1, ':',$3, ZN);
				  if ($1->sym->type != 0
				  &&  $1->sym->type != LABEL) {
				  non_fatal("bad label-name %s", $1->sym->name);
				  }
				  $1->sym->type = LABEL;
				}
	| IF options FI 	{ $$ = nn($1, IF, ZN, ZN);
        			  $$->sl = $2->sl;
        			}
	| DO    		{ pushbreak(); }
          options OD    	{ $$ = nn($1, DO, ZN, ZN);
        			  $$->sl = $3->sl;
        			}
	| ELSE  		{ $$ = nn(ZN,ELSE,ZN,ZN);
				}
	| BREAK  		{ $$ = nn(ZN,GOTO,ZN,ZN);
				  $$->sym = break_dest();
				}
	| ATOMIC   '{'   	{ open_seq(0); }
          sequence OS '}'   	{ $$ = nn($1, ATOMIC, ZN, ZN);
        			  $$->sl = seqlist(close_seq(3), 0);
        			  make_atomic($$->sl->this, 0);
        			}
	| D_STEP '{'		{ open_seq(0); rem_Seq(); }
          sequence OS '}'   	{ $$ = nn($1, D_STEP, ZN, ZN);
        			  $$->sl = seqlist(close_seq(4), 0);
        			  make_atomic($$->sl->this, D_ATOM);
				  unrem_Seq();
        			}
	| '{'			{ open_seq(0); }
	  sequence OS '}'	{ $$ = nn(ZN, NON_ATOMIC, ZN, ZN);
        			  $$->sl = seqlist(close_seq(5), 0);
        			 }
	;

options : option		{ $$->sl = seqlist($1->sq, 0); }
	| option options	{ $$->sl = seqlist($1->sq, $2->sl); }
	;

option  : SEP   		{ open_seq(0); }
          sequence OS		{ $$->sq = close_seq(6); }
	;

OS	: /* empty */
	| SEMI			{ /* redundant semi at end of sequence */ }
	;

MS	: SEMI			{ /* at least one semi-colon */ }
	| MS SEMI		{ /* but more are okay too   */ }
	;

aname	: NAME			{ $$ = $1; }
	| PNAME			{ $$ = $1; }
	;

expr    : '(' expr ')'		{ $$ = $2; }
	| expr '+' expr		{ $$ = nn(ZN, '+', $1, $3); }
	| expr '-' expr		{ $$ = nn(ZN, '-', $1, $3); }
	| expr '*' expr		{ $$ = nn(ZN, '*', $1, $3); }
	| expr '/' expr		{ $$ = nn(ZN, '/', $1, $3); }
	| expr '%' expr		{ $$ = nn(ZN, '%', $1, $3); }
	| expr '&' expr		{ $$ = nn(ZN, '&', $1, $3); }
	| expr '|' expr		{ $$ = nn(ZN, '|', $1, $3); }
	| expr GT expr		{ $$ = nn(ZN,  GT, $1, $3); }
	| expr LT expr		{ $$ = nn(ZN,  LT, $1, $3); }
	| expr GE expr		{ $$ = nn(ZN,  GE, $1, $3); }
	| expr LE expr		{ $$ = nn(ZN,  LE, $1, $3); }
	| expr EQ expr		{ $$ = nn(ZN,  EQ, $1, $3); }
	| expr NE expr		{ $$ = nn(ZN,  NE, $1, $3); }
	| expr AND expr		{ $$ = nn(ZN, AND, $1, $3); }
	| expr OR  expr		{ $$ = nn(ZN,  OR, $1, $3); }
	| expr LSHIFT expr	{ $$ = nn(ZN, LSHIFT,$1, $3); }
	| expr RSHIFT expr	{ $$ = nn(ZN, RSHIFT,$1, $3); }
	| '~' expr		{ $$ = nn(ZN, '~', $2, ZN); }
	| '-' expr %prec UMIN	{ $$ = nn(ZN, UMIN, $2, ZN); }
	| SND expr %prec NEG	{ $$ = nn(ZN, '!', $2, ZN); }

	| '(' expr SEMI expr ':' expr ')' {
				  $$ = nn(ZN,  OR, $4, $6);
				  $$ = nn(ZN, '?', $2, $$);
				}

	| RUN aname		{ Expand_Ok++; }
	  '(' args ')'		{ Expand_Ok--;
				  $$ = nn($2, RUN, $5, ZN);
				}
	| LEN '(' varref ')'	{ $$ = nn($3, LEN, $3, ZN); }
	| ENABLED '(' expr ')'	{ $$ = nn(ZN, ENABLED, $3, ZN); has_enabled++; }
	| varref RCV		{ Expand_Ok++; }
	  '[' rargs ']'		{ Expand_Ok--;
				  $$ = nn($1, 'R', $1, $5);
				}
	| varref R_RCV		{ Expand_Ok++; }
	  '[' rargs ']'		{ Expand_Ok--;
				  $$ = nn($1, 'R', $1, $5);
				  $$->val = has_random = 1;
				}
	| varref		{ $$ = $1; }
	| CONST 		{ $$ = nn(ZN,CONST,ZN,ZN);
				  $$->ismtyp = $1->ismtyp;
				  $$->val = $1->val;
				}
	| TIMEOUT		{ $$ = nn(ZN,TIMEOUT, ZN, ZN); }
	| PC_VAL '(' expr ')'	{ $$ = nn(ZN, PC_VAL, $3, ZN); }
	| PNAME '[' expr ']' '@'
	  NAME			{ $$ = rem_lab($1->sym, $3, $6->sym); }
	;

	/* an Expr cannot be negated - to protect Probe expressions */
Expr	: Probe			{ $$ = $1; }
	| '(' Expr ')'		{ $$ = $2; }
	| Expr AND Expr		{ $$ = nn(ZN, AND, $1, $3); }
	| Expr AND expr		{ $$ = nn(ZN, AND, $1, $3); }
	| Expr OR  Expr		{ $$ = nn(ZN,  OR, $1, $3); }
	| Expr OR  expr		{ $$ = nn(ZN,  OR, $1, $3); }
	| expr AND Expr		{ $$ = nn(ZN, AND, $1, $3); }
	| expr OR  Expr		{ $$ = nn(ZN,  OR, $1, $3); }
	;

Probe	: FULL '(' varref ')'	{ $$ = nn($3,  FULL, $3, ZN); }
	| NFULL '(' varref ')'	{ $$ = nn($3, NFULL, $3, ZN); }
	| EMPTY '(' varref ')'	{ $$ = nn($3, EMPTY, $3, ZN); }
	| NEMPTY '(' varref ')'	{ $$ = nn($3,NEMPTY, $3, ZN); }
	;

basetype: TYPE			{ $$->sym = ZS;
				  $$->val = $1->val;
				}
	| MTYPE			{ $$->sym = ZS;
				  $$->val = MTYPE;
				}
	| UNAME			{ $$->sym = $1->sym;
				  $$->val = STRUCT;
				}
	;

typ_list: basetype		{ $$ = nn($1, $1->val, ZN, ZN); }
	| basetype ',' typ_list	{ $$ = nn($1, $1->val, ZN, $3); }
	;

args    : /* empty */		{ $$ = ZN; }
	| arg			{ $$ = $1; }
	;

prargs  : /* empty */		{ $$ = ZN; }
	| ',' arg		{ $$ = $2; }
	;

margs   : arg			{ $$ = $1; }
	| expr '(' arg ')'	{ if ($1->ntyp == ',')
					$$ = tail_add($1, $3);
				  else
				  	$$ = nn(ZN, ',', $1, $3);
				}
	;

arg     : expr			{ if ($1->ntyp == ',')
					$$ = $1;
				  else
				  	$$ = nn(ZN, ',', $1, ZN);
				}
	| expr ',' arg		{ if ($1->ntyp == ',')
					$$ = tail_add($1, $3);
				  else
				  	$$ = nn(ZN, ',', $1, $3);
				}
	;

rarg	: varref		{ $$ = $1; }
	| CONST 		{ $$ = nn(ZN,CONST,ZN,ZN);
				  $$->ismtyp = $1->ismtyp;
				  $$->val = $1->val;
				}
	| '-' CONST %prec UMIN	{ $$ = nn(ZN,CONST,ZN,ZN);
				  $$->val = - ($2->val);
				}
	;

rargs	: rarg			{ if ($1->ntyp == ',')
					$$ = $1;
				  else
				  	$$ = nn(ZN, ',', $1, ZN);
				}
	| rarg ',' rargs	{ if ($1->ntyp == ',')
					$$ = tail_add($1, $3);
				  else
				  	$$ = nn(ZN, ',', $1, $3);
				}
	| rarg '(' rargs ')'	{ if ($1->ntyp == ',')
					$$ = tail_add($1, $3);
				  else
				  	$$ = nn(ZN, ',', $1, $3);
				}
	| '(' rargs ')'		{ $$ = $2; }
	;

nlst	: NAME			{ $$ = nn($1, NAME, ZN, ZN);
				  $$ = nn(ZN, ',', $$, ZN); }
	| NAME ',' nlst		{ $$ = nn($1, NAME, ZN, ZN);
				  $$ = nn(ZN, ',', $$, $3);
				}
	;
%%

void
yyerror(char *fmt, ...)
{	va_list ap;

	va_start(ap, fmt);
	non_fatal(fmt, ap);
	va_end(ap);
}
