/*
Copyright (c) 1989 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

%{
#include <stdio.h>
#include "awk.h"
yywrap(void) { return(1); }

Node	*beginloc = 0;
Node	*endloc = 0;
int	infunc	= 0;	/* = 1 if in arglist or body of func */
int	inloop	= 0;	/* = 1 if in while, for, do */
uchar	*curfname = 0;
Node	*arglist = 0;	/* list of args for current function */
%}

%union {
	Node	*p;
	Cell	*cp;
	int	i;
	uchar	*s;
}

%token	<i>	FIRSTTOKEN	/* must be first */
%token	<p>	PROGRAM PASTAT PASTAT2 XBEGIN XEND
%token	<i>	NL ',' '{' '(' '|' ';' '/' ')' '}' '[' ']'
%token	<i>	ARRAY
%token	<i>	MATCH NOTMATCH MATCHOP
%token	<i>	FINAL DOT ALL CCL NCCL CHAR OR STAR QUEST PLUS
%token	<i>	AND BOR APPEND EQ GE GT LE LT NE IN
%token	<i>	ARG BLTIN BREAK CLOSE CONTINUE DELETE DO EXIT FOR FUNC 
%token	<i>	SUB GSUB IF INDEX LSUBSTR MATCHFCN NEXT
%token	<i>	ADD MINUS MULT DIVIDE MOD
%token	<i>	ASSIGN ASGNOP ADDEQ SUBEQ MULTEQ DIVEQ MODEQ POWEQ
%token	<i>	PRINT PRINTF SPRINTF
%token	<p>	ELSE INTEST CONDEXPR
%token	<i>	POSTINCR PREINCR POSTDECR PREDECR
%token	<cp>	VAR IVAR VARNF CALL NUMBER STRING FIELD
%token	<s>	REGEXPR

%type	<p>	pas pattern ppattern plist pplist patlist prarg term re
%type	<p>	pa_pat pa_stat pa_stats
%type	<s>	reg_expr
%type	<p>	simple_stmt opt_simple_stmt stmt stmtlist
%type	<p>	var varname funcname varlist
%type	<p>	for if while
%type	<i>	pst opt_pst lbrace rparen comma nl opt_nl and bor
%type	<i>	subop print

%right	ASGNOP
%right	'?'
%right	':'
%left	BOR
%left	AND
%left	GETLINE
%nonassoc APPEND EQ GE GT LE LT NE MATCHOP IN '|'
%left	ARG BLTIN BREAK CALL CLOSE CONTINUE DELETE DO EXIT FOR FIELD FUNC 
%left	GSUB IF INDEX LSUBSTR MATCHFCN NEXT NUMBER
%left	PRINT PRINTF RETURN SPLIT SPRINTF STRING SUB SUBSTR
%left	REGEXPR VAR VARNF IVAR WHILE '('
%left	CAT
%left	'+' '-'
%left	'*' '/' '%'
%left	NOT UMINUS
%right	POWER
%right	DECR INCR
%left	INDIRECT
%token	LASTTOKEN	/* must be last */

%%

program:
	  pas	{ if (errorflag==0)
			winner = (Node *)stat3(PROGRAM, beginloc, $1, endloc); }
	| error	{ yyclearin; bracecheck(); ERROR "bailing out" SYNTAX; }
	;

and:
	  AND | and NL
	;

bor:
	  BOR | bor NL
	;

comma:
	  ',' | comma NL
	;

do:
	  DO | do NL
	;

else:
	  ELSE | else NL
	;

for:
	  FOR '(' opt_simple_stmt ';' opt_nl pattern ';' opt_nl opt_simple_stmt rparen {inloop++;} stmt
		{ --inloop; $$ = stat4(FOR, $3, notnull($6), $9, $12); }
	| FOR '(' opt_simple_stmt ';'  ';' opt_nl opt_simple_stmt rparen {inloop++;} stmt
		{ --inloop; $$ = stat4(FOR, $3, NIL, $7, $10); }
	| FOR '(' varname IN varname rparen {inloop++;} stmt
		{ --inloop; $$ = stat3(IN, $3, makearr($5), $8); }
	;

funcname:
	  VAR	{ setfname($1); }
	| CALL	{ setfname($1); }
	;

if:
	  IF '(' pattern rparen		{ $$ = notnull($3); }
	;

lbrace:
	  '{' | lbrace NL
	;

nl:
	  NL | nl NL
	;

opt_nl:
	  /* empty */	{ $$ = 0; }
	| nl
	;

opt_pst:
	  /* empty */	{ $$ = 0; }
	| pst
	;


opt_simple_stmt:
	  /* empty */			{ $$ = 0; }
	| simple_stmt
	;

pas:
	  opt_pst			{ $$ = 0; }
	| opt_pst pa_stats opt_pst	{ $$ = $2; }
	;

pa_pat:
	  pattern	{ $$ = notnull($1); }
	;

pa_stat:
	  pa_pat			{ $$ = stat2(PASTAT, $1, stat2(PRINT, rectonode(), NIL)); }
	| pa_pat lbrace stmtlist '}'	{ $$ = stat2(PASTAT, $1, $3); }
	| pa_pat ',' pa_pat		{ $$ = pa2stat($1, $3, stat2(PRINT, rectonode(), NIL)); }
	| pa_pat ',' pa_pat lbrace stmtlist '}'	{ $$ = pa2stat($1, $3, $5); }
	| lbrace stmtlist '}'		{ $$ = stat2(PASTAT, NIL, $2); }
	| XBEGIN lbrace stmtlist '}'
		{ beginloc = linkum(beginloc, $3); $$ = 0; }
	| XEND lbrace stmtlist '}'
		{ endloc = linkum(endloc, $3); $$ = 0; }
	| FUNC funcname '(' varlist rparen {infunc++;} lbrace stmtlist '}'
		{ infunc--; curfname=0; defn((Cell *)$2, $4, $8); $$ = 0; }
	;

pa_stats:
	  pa_stat
	| pa_stats opt_pst pa_stat	{ $$ = linkum($1, $3); }
	;

patlist:
	  pattern
	| patlist comma pattern	{ $$ = linkum($1, $3); }
	;

ppattern:
	  var ASGNOP ppattern		{ $$ = op2($2, $1, $3); }
	| ppattern '?' ppattern ':' ppattern %prec '?'
	 	{ $$ = op3(CONDEXPR, notnull($1), $3, $5); }
	| ppattern bor ppattern %prec BOR
		{ $$ = op2(BOR, notnull($1), notnull($3)); }
	| ppattern and ppattern %prec AND
		{ $$ = op2(AND, notnull($1), notnull($3)); }
	| ppattern MATCHOP reg_expr	{ $$ = op3($2, NIL, $1, (Node*)compre($3)); }
	| ppattern MATCHOP ppattern
		{ if (constnode($3))
			$$ = op3($2, NIL, $1, (Node*)compre(strnode($3)));
		  else
			$$ = op3($2, (Node *)1, $1, $3); }
	| ppattern IN varname		{ $$ = op2(INTEST, $1, makearr($3)); }
	| '(' plist ')' IN varname	{ $$ = op2(INTEST, $2, makearr($5)); }
	| ppattern term %prec CAT	{ $$ = op2(CAT, $1, $2); }
	| re
	| term
	;

pattern:
	  var ASGNOP pattern		{ $$ = op2($2, $1, $3); }
	| pattern '?' pattern ':' pattern %prec '?'
	 	{ $$ = op3(CONDEXPR, notnull($1), $3, $5); }
	| pattern bor pattern %prec BOR
		{ $$ = op2(BOR, notnull($1), notnull($3)); }
	| pattern and pattern %prec AND
		{ $$ = op2(AND, notnull($1), notnull($3)); }
	| pattern EQ pattern		{ $$ = op2($2, $1, $3); }
	| pattern GE pattern		{ $$ = op2($2, $1, $3); }
	| pattern GT pattern		{ $$ = op2($2, $1, $3); }
	| pattern LE pattern		{ $$ = op2($2, $1, $3); }
	| pattern LT pattern		{ $$ = op2($2, $1, $3); }
	| pattern NE pattern		{ $$ = op2($2, $1, $3); }
	| pattern MATCHOP reg_expr	{ $$ = op3($2, NIL, $1, (Node*)compre($3)); }
	| pattern MATCHOP pattern
		{ if (constnode($3))
			$$ = op3($2, NIL, $1, (Node*)compre(strnode($3)));
		  else
			$$ = op3($2, (Node *)1, $1, $3); }
	| pattern IN varname		{ $$ = op2(INTEST, $1, makearr($3)); }
	| '(' plist ')' IN varname	{ $$ = op2(INTEST, $2, makearr($5)); }
	| pattern '|' GETLINE var	{ $$ = op3(GETLINE, $4, (Node*)$2, $1); }
	| pattern '|' GETLINE		{ $$ = op3(GETLINE, (Node*)0, (Node*)$2, $1); }
	| pattern term %prec CAT	{ $$ = op2(CAT, $1, $2); }
	| re
	| term
	;

plist:
	  pattern comma pattern		{ $$ = linkum($1, $3); }
	| plist comma pattern		{ $$ = linkum($1, $3); }
	;

pplist:
	  ppattern
	| pplist comma ppattern		{ $$ = linkum($1, $3); }
	;

prarg:
	  /* empty */			{ $$ = rectonode(); }
	| pplist
	| '(' plist ')'			{ $$ = $2; }
	;

print:
	  PRINT | PRINTF
	;

pst:
	  NL | ';' | pst NL | pst ';'
	;

rbrace:
	  '}' | rbrace NL
	;

re:
	   reg_expr
		{ $$ = op3(MATCH, NIL, rectonode(), (Node*)compre($1)); }
	| NOT re			{ $$ = op1(NOT, notnull($2)); }
	;

reg_expr:
	  '/' {startreg();} REGEXPR '/'		{ $$ = $3; }
	;

rparen:
	  ')' | rparen NL
	;

simple_stmt:
	  print prarg '|' term		{ $$ = stat3($1, $2, (Node *) $3, $4); }
	| print prarg APPEND term	{ $$ = stat3($1, $2, (Node *) $3, $4); }
	| print prarg GT term		{ $$ = stat3($1, $2, (Node *) $3, $4); }
	| print prarg			{ $$ = stat3($1, $2, NIL, NIL); }
	| DELETE varname '[' patlist ']' { $$ = stat2(DELETE, makearr($2), $4); }
	| DELETE varname		{ yyclearin; ERROR "you can only delete array[element]" SYNTAX; $$ = stat1(DELETE, $2); }
	| pattern			{ $$ = exptostat($1); }
	| error				{ yyclearin; ERROR "illegal statement" SYNTAX; }
	;

st:
	  nl | ';' opt_nl
	;

stmt:
	  BREAK st		{ if (!inloop) ERROR "break illegal outside of loops" SYNTAX;
				  $$ = stat1(BREAK, NIL); }
	| CLOSE pattern st	{ $$ = stat1(CLOSE, $2); }
	| CONTINUE st		{  if (!inloop) ERROR "continue illegal outside of loops" SYNTAX;
				  $$ = stat1(CONTINUE, NIL); }
	| do {inloop++;} stmt {--inloop;} WHILE '(' pattern ')' st
		{ $$ = stat2(DO, $3, notnull($7)); }
	| EXIT pattern st	{ $$ = stat1(EXIT, $2); }
	| EXIT st		{ $$ = stat1(EXIT, NIL); }
	| for
	| if stmt else stmt	{ $$ = stat3(IF, $1, $2, $4); }
	| if stmt		{ $$ = stat3(IF, $1, $2, NIL); }
	| lbrace stmtlist rbrace { $$ = $2; }
	| NEXT st	{ if (infunc)
				ERROR "next is illegal inside a function" SYNTAX;
			  $$ = stat1(NEXT, NIL); }
	| RETURN pattern st	{ $$ = stat1(RETURN, $2); }
	| RETURN st		{ $$ = stat1(RETURN, NIL); }
	| simple_stmt st
	| while {inloop++;} stmt	{ --inloop; $$ = stat2(WHILE, $1, $3); }
	| ';' opt_nl		{ $$ = 0; }
	;

stmtlist:
	  stmt
	| stmtlist stmt		{ $$ = linkum($1, $2); }
	;

subop:
	  SUB | GSUB
	;

term:
	  term '+' term			{ $$ = op2(ADD, $1, $3); }
	| term '-' term			{ $$ = op2(MINUS, $1, $3); }
	| term '*' term			{ $$ = op2(MULT, $1, $3); }
	| term '/' term			{ $$ = op2(DIVIDE, $1, $3); }
	| term '%' term			{ $$ = op2(MOD, $1, $3); }
	| term POWER term		{ $$ = op2(POWER, $1, $3); }
	| '-' term %prec UMINUS		{ $$ = op1(UMINUS, $2); }
	| '+' term %prec UMINUS		{ $$ = $2; }
	| NOT term %prec UMINUS		{ $$ = op1(NOT, notnull($2)); }
	| BLTIN '(' ')'			{ $$ = op2(BLTIN, (Node *) $1, rectonode()); }
	| BLTIN '(' patlist ')'		{ $$ = op2(BLTIN, (Node *) $1, $3); }
	| BLTIN				{ $$ = op2(BLTIN, (Node *) $1, rectonode()); }
	| CALL '(' ')'			{ $$ = op2(CALL, valtonode($1,CVAR), NIL); }
	| CALL '(' patlist ')'		{ $$ = op2(CALL, valtonode($1,CVAR), $3); }
	| DECR var			{ $$ = op1(PREDECR, $2); }
	| INCR var			{ $$ = op1(PREINCR, $2); }
	| var DECR			{ $$ = op1(POSTDECR, $1); }
	| var INCR			{ $$ = op1(POSTINCR, $1); }
	| GETLINE var LT term		{ $$ = op3(GETLINE, $2, (Node *)$3, $4); }
	| GETLINE LT term		{ $$ = op3(GETLINE, NIL, (Node *)$2, $3); }
	| GETLINE var			{ $$ = op3(GETLINE, $2, NIL, NIL); }
	| GETLINE			{ $$ = op3(GETLINE, NIL, NIL, NIL); }
	| INDEX '(' pattern comma pattern ')'
		{ $$ = op2(INDEX, $3, $5); }
	| INDEX '(' pattern comma reg_expr ')'
		{ ERROR "index() doesn't permit regular expressions" SYNTAX;
		  $$ = op2(INDEX, $3, (Node*)$5); }
	| '(' pattern ')'		{ $$ = $2; }
	| MATCHFCN '(' pattern comma reg_expr ')'
		{ $$ = op3(MATCHFCN, NIL, $3, (Node*)compre($5)); }
	| MATCHFCN '(' pattern comma pattern ')'
		{ if (constnode($5))
			$$ = op3(MATCHFCN, NIL, $3, (Node*)compre(strnode($5)));
		  else
			$$ = op3(MATCHFCN, (Node *)1, $3, $5); }
	| NUMBER			{ $$ = valtonode($1, CCON); }
	| SPLIT '(' pattern comma varname comma pattern ')'     /* string */
		{ $$ = op4(SPLIT, $3, makearr($5), $7, (Node*)STRING); }
	| SPLIT '(' pattern comma varname comma reg_expr ')'    /* const /regexp/ */
		{ $$ = op4(SPLIT, $3, makearr($5), (Node*)compre($7), (Node *)REGEXPR); }
	| SPLIT '(' pattern comma varname ')'
		{ $$ = op4(SPLIT, $3, makearr($5), NIL, (Node*)STRING); }  /* default */
	| SPRINTF '(' patlist ')'	{ $$ = op1($1, $3); }
	| STRING	 		{ $$ = valtonode($1, CCON); }
	| subop '(' reg_expr comma pattern ')'
		{ $$ = op4($1, NIL, (Node*)compre($3), $5, rectonode()); }
	| subop '(' pattern comma pattern ')'
		{ if (constnode($3))
			$$ = op4($1, NIL, (Node*)compre(strnode($3)), $5, rectonode());
		  else
			$$ = op4($1, (Node *)1, $3, $5, rectonode()); }
	| subop '(' reg_expr comma pattern comma var ')'
		{ $$ = op4($1, NIL, (Node*)compre($3), $5, $7); }
	| subop '(' pattern comma pattern comma var ')'
		{ if (constnode($3))
			$$ = op4($1, NIL, (Node*)compre(strnode($3)), $5, $7);
		  else
			$$ = op4($1, (Node *)1, $3, $5, $7); }
	| SUBSTR '(' pattern comma pattern comma pattern ')'
		{ $$ = op3(SUBSTR, $3, $5, $7); }
	| SUBSTR '(' pattern comma pattern ')'
		{ $$ = op3(SUBSTR, $3, $5, NIL); }
	| var
	;

var:
	  varname
	| varname '[' patlist ']'	{ $$ = op2(ARRAY, makearr($1), $3); }
	| FIELD				{ $$ = valtonode($1, CFLD); }
	| IVAR				{ $$ = op1(INDIRECT, valtonode($1, CVAR)); }
	| INDIRECT term	 		{ $$ = op1(INDIRECT, $2); }
	;	

varlist:
	  /* nothing */		{ arglist = $$ = 0; }
	| VAR			{ arglist = $$ = valtonode($1,CVAR); }
	| varlist comma VAR	{ arglist = $$ = linkum($1,valtonode($3,CVAR)); }
	;

varname:
	  VAR			{ $$ = valtonode($1, CVAR); }
	| ARG 			{ $$ = op1(ARG, (Node *) $1); }
	| VARNF			{ $$ = op1(VARNF, (Node *) $1); }
	;


while:
	  WHILE '(' pattern rparen	{ $$ = notnull($3); }
	;

%%

void setfname(Cell *p)
{
	if (isarr(p))
		ERROR "%s is an array, not a function", p->nval SYNTAX;
	else if (isfunc(p))
		ERROR "you can't define function %s more than once", p->nval SYNTAX;
	curfname = p->nval;
}

constnode(Node *p)
{
	return isvalue(p) && ((Cell *) (p->narg[0]))->csub == CCON;
}

uchar *strnode(Node *p)
{
	return ((Cell *)(p->narg[0]))->sval;
}

Node *notnull(Node *n)
{
	switch (n->nobj) {
	case LE: case LT: case EQ: case NE: case GT: case GE:
	case BOR: case AND: case NOT:
		return n;
	default:
		return op2(NE, n, nullnode);
	}
}
