/***** spin: spin.y *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

%{
#include "spin.h"
#include <sys/types.h>
#ifndef PC
#include <unistd.h>
#endif
#include <stdarg.h>

#define YYDEBUG	0
#define Stop	nn(ZN,'@',ZN,ZN)
#define PART0	"place initialized var decl of "
#define PART1	"place initialized chan decl of "
#define PART2	" at start of proctype "

static	Lextok *ltl_to_string(Lextok *);

extern  Symbol	*context, *owner;
extern	Lextok *for_body(Lextok *, int);
extern	void for_setup(Lextok *, Lextok *, Lextok *);
extern	Lextok *for_index(Lextok *, Lextok *);
extern	Lextok *sel_index(Lextok *, Lextok *, Lextok *);
extern  int	u_sync, u_async, dumptab, scope_level;
extern	int	initialization_ok, split_decl;
extern	short	has_sorted, has_random, has_enabled, has_pcvalue, has_np;
extern	short	has_code, has_state, has_io;
extern	void	count_runs(Lextok *);
extern	void	no_internals(Lextok *);
extern	void	any_runs(Lextok *);
extern	void	ltl_list(char *, char *);
extern	void	validref(Lextok *, Lextok *);
extern	char	yytext[];

int	Mpars = 0;	/* max nr of message parameters  */
int	nclaims = 0;	/* nr of never claims */
int	ltl_mode = 0;	/* set when parsing an ltl formula */
int	Expand_Ok = 0, realread = 1, IArgs = 0, NamesNotAdded = 0;
int	in_for = 0;
char	*claimproc = (char *) 0;
char	*eventmap = (char *) 0;
static	char *ltl_name;

static	int	Embedded = 0, inEventMap = 0, has_ini = 0;

%}

%token	ASSERT PRINT PRINTM
%token	C_CODE C_DECL C_EXPR C_STATE C_TRACK
%token	RUN LEN ENABLED EVAL PC_VAL
%token	TYPEDEF MTYPE INLINE LABEL OF
%token	GOTO BREAK ELSE SEMI
%token	IF FI DO OD FOR SELECT IN SEP DOTDOT
%token	ATOMIC NON_ATOMIC D_STEP UNLESS
%token  TIMEOUT NONPROGRESS
%token	ACTIVE PROCTYPE D_PROCTYPE
%token	HIDDEN SHOW ISLOCAL
%token	PRIORITY PROVIDED
%token	FULL EMPTY NFULL NEMPTY
%token	CONST TYPE XU			/* val */
%token	NAME UNAME PNAME INAME		/* sym */
%token	STRING CLAIM TRACE INIT	LTL	/* sym */

%right	ASGN
%left	SND O_SND RCV R_RCV /* SND doubles as boolean negation */
%left	IMPLIES EQUIV			/* ltl */
%left	OR
%left	AND
%left	ALWAYS EVENTUALLY		/* ltl */
%left	UNTIL WEAK_UNTIL RELEASE	/* ltl */
%right	NEXT				/* ltl */
%left	'|'
%left	'^'
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

/** PROMELA Grammar Rules **/

program	: units		{ yytext[0] = '\0'; }
	;

units	: unit
	| units unit
	;

unit	: proc		/* proctype { }       */
	| init		/* init { }           */
	| claim		/* never claim        */
	| ltl		/* ltl formula        */
	| events	/* event assertions   */
	| one_decl	/* variables, chans   */
	| utype		/* user defined types */
	| c_fcts	/* c functions etc.   */
	| ns		/* named sequence     */
	| SEMI		/* optional separator */
	| error
	;

proc	: inst		/* optional instantiator */
	  proctype NAME	{ 
			  setptype($3, PROCTYPE, ZN);
			  setpname($3);
			  context = $3->sym;
			  context->ini = $2; /* linenr and file */
			  Expand_Ok++; /* expand struct names in decl */
			  has_ini = 0;
			}
	  '(' decl ')'	{ Expand_Ok--;
			  if (has_ini)
			  fatal("initializer in parameter list", (char *) 0);
			}
	  Opt_priority
	  Opt_enabler
	  body		{ ProcList *rl;
			  if ($1 != ZN && $1->val > 0)
			  {	int j;
				rl = ready($3->sym, $6, $11->sq, $2->val, $10, A_PROC);
			  	for (j = 0; j < $1->val; j++)
				{	runnable(rl, $9?$9->val:1, 1);
				}
				announce(":root:");
				if (dumptab) $3->sym->ini = $1;
			  } else
			  {	rl = ready($3->sym, $6, $11->sq, $2->val, $10, P_PROC);
			  }
			  if (rl && has_ini == 1)	/* global initializations, unsafe */
			  {	/* printf("proctype %s has initialized data\n",
					$3->sym->name);
				 */
				rl->unsafe = 1;
			  }
			  context = ZS;
			}
	;

proctype: PROCTYPE	{ $$ = nn(ZN,CONST,ZN,ZN); $$->val = 0; }
	| D_PROCTYPE	{ $$ = nn(ZN,CONST,ZN,ZN); $$->val = 1; }
	;

inst	: /* empty */	{ $$ = ZN; }
	| ACTIVE	{ $$ = nn(ZN,CONST,ZN,ZN); $$->val = 1; }
	| ACTIVE '[' CONST ']' {
			  $$ = nn(ZN,CONST,ZN,ZN); $$->val = $3->val;
			  if ($3->val > 255)
				non_fatal("max nr of processes is 255\n", "");
			}
	| ACTIVE '[' NAME ']' {
			  $$ = nn(ZN,CONST,ZN,ZN);
			  $$->val = 0;
			  if (!$3->sym->type)
				fatal("undeclared variable %s",
					$3->sym->name);
			  else if ($3->sym->ini->ntyp != CONST)
				fatal("need constant initializer for %s\n",
					$3->sym->name);
			  else
				$$->val = $3->sym->ini->val;
			}
	;

init	: INIT		{ context = $1->sym; }
	  Opt_priority
	  body		{ ProcList *rl;
			  rl = ready(context, ZN, $4->sq, 0, ZN, I_PROC);
			  runnable(rl, $3?$3->val:1, 1);
			  announce(":root:");
			  context = ZS;
        		}
	;

ltl	: LTL optname2		{ ltl_mode = 1; ltl_name = $2->sym->name; }
	  ltl_body		{ if ($4) ltl_list($2->sym->name, $4->sym->name);
			  ltl_mode = 0;
			}
	;

ltl_body: '{' full_expr OS '}' { $$ = ltl_to_string($2); }
	| error		{ $$ = NULL; }
	;

claim	: CLAIM	optname	{ if ($2 != ZN)
			  {	$1->sym = $2->sym;	/* new 5.3.0 */
			  }
			  nclaims++;
			  context = $1->sym;
			  if (claimproc && !strcmp(claimproc, $1->sym->name))
			  {	fatal("claim %s redefined", claimproc);
			  }
			  claimproc = $1->sym->name;
			}
	  body		{ (void) ready($1->sym, ZN, $4->sq, 0, ZN, N_CLAIM);
        		  context = ZS;
        		}
	;

optname : /* empty */	{ char tb[32];
			  memset(tb, 0, 32);
			  sprintf(tb, "never_%d", nclaims);
			  $$ = nn(ZN, NAME, ZN, ZN);
			  $$->sym = lookup(tb);
			}
	| NAME		{ $$ = $1; }
	;

optname2 : /* empty */ { char tb[32]; static int nltl = 0;
			  memset(tb, 0, 32);
			  sprintf(tb, "ltl_%d", nltl++);
			  $$ = nn(ZN, NAME, ZN, ZN);
			  $$->sym = lookup(tb);
			}
	| NAME		{ $$ = $1; }
	;

events : TRACE		{ context = $1->sym;
			  if (eventmap)
				non_fatal("trace %s redefined", eventmap);
			  eventmap = $1->sym->name;
			  inEventMap++;
			}
	  body		{
			  if (strcmp($1->sym->name, ":trace:") == 0)
			  {	(void) ready($1->sym, ZN, $3->sq, 0, ZN, E_TRACE);
			  } else
			  {	(void) ready($1->sym, ZN, $3->sq, 0, ZN, N_TRACE);
			  }
        		  context = ZS;
			  inEventMap--;
			}
	;

utype	: TYPEDEF NAME		{ if (context)
				   fatal("typedef %s must be global",
						$2->sym->name);
				   owner = $2->sym;
				}
	  '{' decl_lst '}'	{ setuname($5); owner = ZS; }
	;

nm	: NAME			{ $$ = $1; }
	| INAME			{ $$ = $1;
				  if (IArgs)
				  fatal("invalid use of '%s'", $1->sym->name);
				}
	;

ns	: INLINE nm '('		{ NamesNotAdded++; }
	  args ')'		{ prep_inline($2->sym, $5);
				  NamesNotAdded--;
				}
	;

c_fcts	: ccode			{ /* leaves pseudo-inlines with sym of
				   * type CODE_FRAG or CODE_DECL in global context
				   */
				}
	| cstate
	;

cstate	: C_STATE STRING STRING	{
				  c_state($2->sym, $3->sym, ZS);
				  has_code = has_state = 1;
				}
	| C_TRACK STRING STRING {
				  c_track($2->sym, $3->sym, ZS);
				  has_code = has_state = 1;
				}
	| C_STATE STRING STRING	STRING {
				  c_state($2->sym, $3->sym, $4->sym);
				  has_code = has_state = 1;
				}
	| C_TRACK STRING STRING STRING {
				  c_track($2->sym, $3->sym, $4->sym);
				  has_code = has_state = 1;
				}
	;

ccode	: C_CODE		{ Symbol *s;
				  NamesNotAdded++;
				  s = prep_inline(ZS, ZN);
				  NamesNotAdded--;
				  $$ = nn(ZN, C_CODE, ZN, ZN);
				  $$->sym = s;
				  has_code = 1;
				}
	| C_DECL		{ Symbol *s;
				  NamesNotAdded++;
				  s = prep_inline(ZS, ZN);
				  NamesNotAdded--;
				  s->type = CODE_DECL;
				  $$ = nn(ZN, C_CODE, ZN, ZN);
				  $$->sym = s;
				  has_code = 1;
				}
	;
cexpr	: C_EXPR		{ Symbol *s;
				  NamesNotAdded++;
				  s = prep_inline(ZS, ZN);
				  NamesNotAdded--;
				  $$ = nn(ZN, C_EXPR, ZN, ZN);
				  $$->sym = s;
				  no_side_effects(s->name);
				  has_code = 1;
				}
	;

body	: '{'			{ open_seq(1); }
          sequence OS		{ add_seq(Stop); }
          '}'			{ $$->sq = close_seq(0);
				  if (scope_level != 0)
				  {	non_fatal("missing '}' ?", 0);
					scope_level = 0;
				  }
				}
	;

sequence: step			{ if ($1) add_seq($1); }
	| sequence MS step	{ if ($3) add_seq($3); }
	;

step    : one_decl		{ $$ = ZN; }
	| XU vref_lst		{ setxus($2, $1->val); $$ = ZN; }
	| NAME ':' one_decl	{ fatal("label preceding declaration,", (char *)0); }
	| NAME ':' XU		{ fatal("label predecing xr/xs claim,", 0); }
	| stmnt			{ $$ = $1; }
	| stmnt UNLESS stmnt	{ $$ = do_unless($1, $3); }
	;

vis	: /* empty */		{ $$ = ZN; }
	| HIDDEN		{ $$ = $1; }
	| SHOW			{ $$ = $1; }
	| ISLOCAL		{ $$ = $1; }
	;

asgn:	/* empty */
	| ASGN
	;

one_decl: vis TYPE var_list	{ setptype($3, $2->val, $1);
				  $$ = $3;
				}
	| vis UNAME var_list	{ setutype($3, $2->sym, $1);
				  $$ = expand($3, Expand_Ok);
				}
	| vis TYPE asgn '{' nlst '}' {
				  if ($2->val != MTYPE)
					fatal("malformed declaration", 0);
				  setmtype($5);
				  if ($1)
					non_fatal("cannot %s mtype (ignored)",
						$1->sym->name);
				  if (context != ZS)
					fatal("mtype declaration must be global", 0);
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

ivar    : vardcl           	{ $$ = $1;
				  $1->sym->ini = nn(ZN,CONST,ZN,ZN);
				  $1->sym->ini->val = 0;
				}
	| vardcl ASGN expr   	{ $$ = $1;
				  $1->sym->ini = $3;
				  trackvar($1,$3);
				  if ($3->ntyp == CONST
				  || ($3->ntyp == NAME && $3->sym->context))
				  {	has_ini = 2; /* local init */
				  } else
				  {	has_ini = 1; /* possibly global */
				  }
				  if (!initialization_ok && split_decl)
				  {	nochan_manip($1, $3, 0);
				  	no_internals($1);
					non_fatal(PART0 "'%s'" PART2, $1->sym->name);
				  }
				}
	| vardcl ASGN ch_init	{ $1->sym->ini = $3;
				  $$ = $1; has_ini = 1;
				  if (!initialization_ok && split_decl)
				  {	non_fatal(PART1 "'%s'" PART2, $1->sym->name);
				  }
				}
	;

ch_init : '[' CONST ']' OF
	  '{' typ_list '}'	{ if ($2->val)
					u_async++;
				  else
					u_sync++;
        			  {	int i = cnt_mpars($6);
					Mpars = max(Mpars, i);
				  }
        			  $$ = nn(ZN, CHAN, ZN, $6);
				  $$->val = $2->val;
        			}
	;

vardcl  : NAME  		{ $1->sym->nel = 1; $$ = $1; }
	| NAME ':' CONST	{ $1->sym->nbits = $3->val;
				  if ($3->val >= 8*sizeof(long))
				  {	non_fatal("width-field %s too large",
						$1->sym->name);
					$3->val = 8*sizeof(long)-1;
				  }
				  $1->sym->nel = 1; $$ = $1;
				}
	| NAME '[' CONST ']'	{ $1->sym->nel = $3->val; $1->sym->isarray = 1; $$ = $1; }
	;

varref	: cmpnd			{ $$ = mk_explicit($1, Expand_Ok, NAME); }
	;

pfld	: NAME			{ $$ = nn($1, NAME, ZN, ZN);
				  if ($1->sym->isarray && !in_for)
				  {	non_fatal("missing array index for '%s'",
						$1->sym->name);
				  }
				}
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
				  Embedded--;
				  if (!Embedded && !NamesNotAdded
				  &&  !$1->sym->type)
				   fatal("undeclared variable: %s",
						$1->sym->name);
				  if ($3) validref($1, $3->lft);
				  owner = ZS;
				}
	;

sfld	: /* empty */		{ $$ = ZN; }
	| '.' cmpnd %prec DOT	{ $$ = nn(ZN, '.', $2, ZN); }
	;

stmnt	: Special		{ $$ = $1; initialization_ok = 0; }
	| Stmnt			{ $$ = $1; initialization_ok = 0;
				  if (inEventMap)
				   non_fatal("not an event", (char *)0);
				}
	;

for_pre : FOR '('				{ in_for = 1; }
	  varref			{ $$ = $4; }
	;

for_post: '{' sequence OS '}' ;

Special : varref RCV		{ Expand_Ok++; }
	  rargs			{ Expand_Ok--; has_io++;
				  $$ = nn($1,  'r', $1, $4);
				  trackchanuse($4, ZN, 'R');
				}
	| varref SND		{ Expand_Ok++; }
	  margs			{ Expand_Ok--; has_io++;
				  $$ = nn($1, 's', $1, $4);
				  $$->val=0; trackchanuse($4, ZN, 'S');
				  any_runs($4);
				}
	| for_pre ':' expr DOTDOT expr ')'	{
				  for_setup($1, $3, $5); in_for = 0;
				}
	  for_post			{ $$ = for_body($1, 1);
				}
	| for_pre IN varref ')'	{ $$ = for_index($1, $3); in_for = 0;
				}
	  for_post			{ $$ = for_body($5, 1);
				}
	| SELECT '(' varref ':' expr DOTDOT expr ')' {
				  $$ = sel_index($3, $5, $7);
				}
	| IF options FI 	{ $$ = nn($1, IF, ZN, ZN);
        			  $$->sl = $2->sl;
				  prune_opts($$);
        			}
	| DO    		{ pushbreak(); }
          options OD    	{ $$ = nn($1, DO, ZN, ZN);
        			  $$->sl = $3->sl;
				  prune_opts($$);
        			}
	| BREAK  		{ $$ = nn(ZN, GOTO, ZN, ZN);
				  $$->sym = break_dest();
				}
	| GOTO NAME		{ $$ = nn($2, GOTO, ZN, ZN);
				  if ($2->sym->type != 0
				  &&  $2->sym->type != LABEL) {
				  	non_fatal("bad label-name %s",
					$2->sym->name);
				  }
				  $2->sym->type = LABEL;
				}
	| NAME ':' stmnt	{ $$ = nn($1, ':',$3, ZN);
				  if ($1->sym->type != 0
				  &&  $1->sym->type != LABEL) {
				  	non_fatal("bad label-name %s",
					$1->sym->name);
				  }
				  $1->sym->type = LABEL;
				}
	;

Stmnt	: varref ASGN full_expr	{ $$ = nn($1, ASGN, $1, $3);
				  trackvar($1, $3);
				  nochan_manip($1, $3, 0);
				  no_internals($1);
				}
	| varref INCR		{ $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
				  $$ = nn(ZN,  '+', $1, $$);
				  $$ = nn($1, ASGN, $1, $$);
				  trackvar($1, $1);
				  no_internals($1);
				  if ($1->sym->type == CHAN)
				   fatal("arithmetic on chan", (char *)0);
				}
	| varref DECR		{ $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
				  $$ = nn(ZN,  '-', $1, $$);
				  $$ = nn($1, ASGN, $1, $$);
				  trackvar($1, $1);
				  no_internals($1);
				  if ($1->sym->type == CHAN)
				   fatal("arithmetic on chan id's", (char *)0);
				}
	| PRINT	'(' STRING	{ realread = 0; }
	  prargs ')'		{ $$ = nn($3, PRINT, $5, ZN); realread = 1; }
	| PRINTM '(' varref ')'	{ $$ = nn(ZN, PRINTM, $3, ZN); }
	| PRINTM '(' CONST ')'	{ $$ = nn(ZN, PRINTM, $3, ZN); }
	| ASSERT full_expr    	{ $$ = nn(ZN, ASSERT, $2, ZN); AST_track($2, 0); }
	| ccode			{ $$ = $1; }
	| varref R_RCV		{ Expand_Ok++; }
	  rargs			{ Expand_Ok--; has_io++;
				  $$ = nn($1,  'r', $1, $4);
				  $$->val = has_random = 1;
				  trackchanuse($4, ZN, 'R');
				}
	| varref RCV		{ Expand_Ok++; }
	  LT rargs GT		{ Expand_Ok--; has_io++;
				  $$ = nn($1, 'r', $1, $5);
				  $$->val = 2;	/* fifo poll */
				  trackchanuse($5, ZN, 'R');
				}
	| varref R_RCV		{ Expand_Ok++; }
	  LT rargs GT		{ Expand_Ok--; has_io++;	/* rrcv poll */
				  $$ = nn($1, 'r', $1, $5);
				  $$->val = 3; has_random = 1;
				  trackchanuse($5, ZN, 'R');
				}
	| varref O_SND		{ Expand_Ok++; }
	  margs			{ Expand_Ok--; has_io++;
				  $$ = nn($1, 's', $1, $4);
				  $$->val = has_sorted = 1;
				  trackchanuse($4, ZN, 'S');
				  any_runs($4);
				}
	| full_expr		{ $$ = nn(ZN, 'c', $1, ZN); count_runs($$); }
	| ELSE  		{ $$ = nn(ZN,ELSE,ZN,ZN);
				}
	| ATOMIC   '{'   	{ open_seq(0); }
          sequence OS '}'   	{ $$ = nn($1, ATOMIC, ZN, ZN);
        			  $$->sl = seqlist(close_seq(3), 0);
        			  make_atomic($$->sl->this, 0);
        			}
	| D_STEP '{'		{ open_seq(0);
				  rem_Seq();
				}
          sequence OS '}'   	{ $$ = nn($1, D_STEP, ZN, ZN);
        			  $$->sl = seqlist(close_seq(4), 0);
        			  make_atomic($$->sl->this, D_ATOM);
				  unrem_Seq();
        			}
	| '{'			{ open_seq(0); }
	  sequence OS '}'	{ $$ = nn(ZN, NON_ATOMIC, ZN, ZN);
        			  $$->sl = seqlist(close_seq(5), 0);
        			}
	| INAME			{ IArgs++; }
	  '(' args ')'		{ pickup_inline($1->sym, $4); IArgs--; }
	  Stmnt			{ $$ = $7; }
	;

options : option		{ $$->sl = seqlist($1->sq, 0); }
	| option options	{ $$->sl = seqlist($1->sq, $2->sl); }
	;

option  : SEP   		{ open_seq(0); }
          sequence OS		{ $$ = nn(ZN,0,ZN,ZN);
				  $$->sq = close_seq(6); }
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
	| expr '^' expr		{ $$ = nn(ZN, '^', $1, $3); }
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

	| RUN aname		{ Expand_Ok++;
				  if (!context)
				   fatal("used 'run' outside proctype",
					(char *) 0);
				}
	  '(' args ')'
	  Opt_priority		{ Expand_Ok--;
				  $$ = nn($2, RUN, $5, ZN);
				  $$->val = ($7) ? $7->val : 1;
				  trackchanuse($5, $2, 'A'); trackrun($$);
				}
	| LEN '(' varref ')'	{ $$ = nn($3, LEN, $3, ZN); }
	| ENABLED '(' expr ')'	{ $$ = nn(ZN, ENABLED, $3, ZN);
				  has_enabled++;
				}
	| varref RCV		{ Expand_Ok++; }
	  '[' rargs ']'		{ Expand_Ok--; has_io++;
				  $$ = nn($1, 'R', $1, $5);
				}
	| varref R_RCV		{ Expand_Ok++; }
	  '[' rargs ']'		{ Expand_Ok--; has_io++;
				  $$ = nn($1, 'R', $1, $5);
				  $$->val = has_random = 1;
				}
	| varref		{ $$ = $1; trapwonly($1 /*, "varref" */); }
	| cexpr			{ $$ = $1; }
	| CONST 		{ $$ = nn(ZN,CONST,ZN,ZN);
				  $$->ismtyp = $1->ismtyp;
				  $$->val = $1->val;
				}
	| TIMEOUT		{ $$ = nn(ZN,TIMEOUT, ZN, ZN); }
	| NONPROGRESS		{ $$ = nn(ZN,NONPROGRESS, ZN, ZN);
				  has_np++;
				}
	| PC_VAL '(' expr ')'	{ $$ = nn(ZN, PC_VAL, $3, ZN);
				  has_pcvalue++;
				}
	| PNAME '[' expr ']' '@' NAME
	  			{ $$ = rem_lab($1->sym, $3, $6->sym); }
	| PNAME '[' expr ']' ':' pfld
	  			{ $$ = rem_var($1->sym, $3, $6->sym, $6->lft); }
	| PNAME '@' NAME	{ $$ = rem_lab($1->sym, ZN, $3->sym); }
	| PNAME ':' pfld	{ $$ = rem_var($1->sym, ZN, $3->sym, $3->lft); }
	| ltl_expr		{ $$ = $1; }
	;

Opt_priority:	/* none */	{ $$ = ZN; }
	| PRIORITY CONST	{ $$ = $2; }
	;

full_expr:	expr		{ $$ = $1; }
	| Expr		{ $$ = $1; }
	;

ltl_expr: expr UNTIL expr	{ $$ = nn(ZN, UNTIL,   $1, $3); }
	| expr RELEASE expr	{ $$ = nn(ZN, RELEASE, $1, $3); }
	| expr WEAK_UNTIL expr	{ $$ = nn(ZN, ALWAYS, $1, ZN);
				  $$ = nn(ZN, OR, $$, nn(ZN, UNTIL, $1, $3));
				}
	| expr IMPLIES expr	{
				$$ = nn(ZN, '!', $1, ZN);
				$$ = nn(ZN, OR,  $$, $3);
				}
	| expr EQUIV expr	{ $$ = nn(ZN, EQUIV,   $1, $3); }
	| NEXT expr       %prec NEG { $$ = nn(ZN, NEXT,  $2, ZN); }
	| ALWAYS expr     %prec NEG { $$ = nn(ZN, ALWAYS,$2, ZN); }
	| EVENTUALLY expr %prec NEG { $$ = nn(ZN, EVENTUALLY, $2, ZN); }
	;

	/* an Expr cannot be negated - to protect Probe expressions */
Expr	: Probe			{ $$ = $1; }
	| '(' Expr ')'		{ $$ = $2; }
	| Expr AND Expr		{ $$ = nn(ZN, AND, $1, $3); }
	| Expr AND expr		{ $$ = nn(ZN, AND, $1, $3); }
	| expr AND Expr		{ $$ = nn(ZN, AND, $1, $3); }
	| Expr OR  Expr		{ $$ = nn(ZN,  OR, $1, $3); }
	| Expr OR  expr		{ $$ = nn(ZN,  OR, $1, $3); }
	| expr OR  Expr		{ $$ = nn(ZN,  OR, $1, $3); }
	;

Probe	: FULL '(' varref ')'	{ $$ = nn($3,  FULL, $3, ZN); }
	| NFULL '(' varref ')'	{ $$ = nn($3, NFULL, $3, ZN); }
	| EMPTY '(' varref ')'	{ $$ = nn($3, EMPTY, $3, ZN); }
	| NEMPTY '(' varref ')'	{ $$ = nn($3,NEMPTY, $3, ZN); }
	;

Opt_enabler:	/* none */	{ $$ = ZN; }
	| PROVIDED '(' full_expr ')'	{ if (!proper_enabler($3))
				  {	non_fatal("invalid PROVIDED clause",
						(char *)0);
					$$ = ZN;
				  } else
					$$ = $3;
				 }
	| PROVIDED error	{ $$ = ZN;
				  non_fatal("usage: provided ( ..expr.. )",
					(char *)0);
				}
	;

basetype: TYPE			{ $$->sym = ZS;
				  $$->val = $1->val;
				  if ($$->val == UNSIGNED)
				  fatal("unsigned cannot be used as mesg type", 0);
				}
	| UNAME			{ $$->sym = $1->sym;
				  $$->val = STRUCT;
				}
	| error			/* e.g., unsigned ':' const */
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

rarg	: varref		{ $$ = $1; trackvar($1, $1);
				  trapwonly($1 /*, "rarg" */); }
	| EVAL '(' expr ')'	{ $$ = nn(ZN,EVAL,$3,ZN);
				  trapwonly($1 /*, "eval rarg" */); }
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
	| nlst NAME 		{ $$ = nn($2, NAME, ZN, ZN);
				  $$ = nn(ZN, ',', $$, $1);
				}
	| nlst ','		{ $$ = $1; /* commas optional */ }
	;
%%

#define binop(n, sop)	fprintf(fd, "("); recursive(fd, n->lft); \
			fprintf(fd, ") %s (", sop); recursive(fd, n->rgt); \
			fprintf(fd, ")");
#define unop(n, sop)	fprintf(fd, "%s (", sop); recursive(fd, n->lft); \
			fprintf(fd, ")");

static void
recursive(FILE *fd, Lextok *n)
{
	if (n)
	switch (n->ntyp) {
	case NEXT:
		unop(n, "X");
		break;
	case ALWAYS:
		unop(n, "[]");
		break;
	case EVENTUALLY:
		unop(n, "<>");
		break;
	case '!':
		unop(n, "!");
		break;
	case UNTIL:
		binop(n, "U");
		break;
	case WEAK_UNTIL:
		binop(n, "W");
		break;
	case RELEASE: /* see http://en.wikipedia.org/wiki/Linear_temporal_logic */
		binop(n, "V");
		break;
	case OR:
		binop(n, "||");
		break;
	case AND:
		binop(n, "&&");
		break;
	case IMPLIES:
		binop(n, "->");
		break;
	case EQUIV:
		binop(n, "<->");
		break;
	default:
		comment(fd, n, 0);
		break;
	}
}

#define TMP_FILE	"_S_p_I_n_.tmp"

extern int unlink(const char *);

static Lextok *
ltl_to_string(Lextok *n)
{	Lextok *m = nn(ZN, 0, ZN, ZN);
	char *retval;
	char formula[1024];
	FILE *tf = fopen(TMP_FILE, "w+"); /* tmpfile() fails on Windows 7 */

	/* convert the parsed ltl to a string
	   by writing into a file, using existing functions,
	   and then passing it to the existing interface for
	   conversion into a never claim
	  (this means parsing everything twice, which is
	   a little redundant, but adds only miniscule overhead)
	 */

	if (!tf)
	{	fatal("cannot create temporary file", (char *) 0);
	}
	recursive(tf, n);
	(void) fseek(tf, 0L, SEEK_SET);

	memset(formula, 0, sizeof(formula));
	retval = fgets(formula, sizeof(formula), tf);
	fclose(tf);
	(void) unlink(TMP_FILE);

	if (!retval)
	{	printf("%p\n", retval);
		fatal("could not translate ltl formula", 0);
	}

	if (1) printf("ltl %s: %s\n", ltl_name, formula);

	m->sym = lookup(formula);

	return m;
}

void
yyerror(char *fmt, ...)
{
	non_fatal(fmt, (char *) 0);
}
