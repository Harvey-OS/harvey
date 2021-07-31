%{
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
%}
%union
{
	long	code;
	Value	val;
	Hshtab	*hp;
	Node	*node;
}
%token	<hp>	INPUTS OUTPUTS EXTERNS FIELDS INPUT OUTPUT
%token	<hp>	EQNS EQN TYPES CEQN BOTH BOTHS BURIEDS BURIED ITER USED
%token	<hp>	NAME EXTERN FIELD
%token	<code>	DCOUTPUT DCFIELD AEQN AOUTPUT CNDEQN XNOR
%token	<code>	GOTO END POP2 NOCASE DNOCASE DSWITCH EQX NEX
%token	<code>	IF SELECT CASE SWITCH WHILE
%token	<code>	CND ALT PERM
%token	<code>	NOT
%token	<code>	'=' COMEQU ALTEQU SUBEQU ADDEQU MULEQU XOREQU MODEQU PNDEQU
%token	<code>	DC
%token	<code>	COM NEG GREY FLONE FRONE
%token	<code>	ADD AND DIV EQ GE GT LAND LE LOR
%token	<code>	LS LT MOD MUL NE OR RS SUB XOR PND
%token	<code>	ELIST ASSIGN DONTCARE
%token	<val>	NUMBER
%right		CND ALT '{'
%left		LOR
%left		LAND
%left		OR XOR
%left		AND
%left		EQ NE LE GE LT GT
%left		ADD SUB
%left		MUL DIV MOD
%left		LS RS
%right		NOT COM
%type	<code>	equop laddop compop addop mulop shiftop
%type	<node>	e eqn eqnexpr swit selem eqns elist
%type	<hp>	anypin anyin anyout
%%
s:
|	{ clearmem(); } prom s

prom:
	dcls eqns
	{
		if(errcount == 0) {
			checkdefns();
			out($2);
		}
	}

dcls:
|	dcl dcls

eqns:
	EQNS { mode=EQN; } elist
	{
		$$ = $3;
	}

dcl:
	externs
|	ttypes
|	iters
|	inputs
|	boths
|	burieds
|	outputs
|	fields
|	used

externs:
	EXTERNS { mode=EXTERN; } NAME chip
	{
		chiptype = $3;
	}

ttypes:	
	TYPES { mode=EXTERN; } NAME
	{
		chipttype = $3;
	}

iters:	
	ITER { mode=EXTERN; } NAME
	{
		fprint(1, ".r %s\n", $3->name);
	}

chip:
|	NAME
	{
		chipname = $1;
	}

inputs:
	INPUTS { mode=INPUT; } alist

boths:
	BOTHS { mode=BOTH; } alist

burieds:
	BURIEDS { mode=BURIED; } alist

outputs:
	OUTPUTS { mode=OUTPUT; } alist

alist:
|	assign alist

assign:
	NAME '=' NUMBER
	{
		dclbit($1, $3);
		setctl($1, CTLNCN);
	}
|	NAME PND
	{
		dclbit($1, 0L);
		setctl($1, CTLNCN);
	}
|	NAME
	{
		dclbit($1, 0L);
	}
|	NAME '[' NUMBER ']'
	{
		dclvector($1, $3, 0);
	}
|	NAME '[' NUMBER ']' SUB
	{
		dclvector($1, $3, 1);
	}
|	NUMBER
|	NAME ALT NUMBER
	{
		dclbit($1, $3);
	}

fields:
	FIELDS { mode=FIELD; } fdcls

fdcls:
|	fdcl fdcls

fdcl:
	NAME '=' { SETLO($1, nextfield); $1->code = FIELD; } flist
	{
		SETHI($1, nextfield);
	}

flist:
|	felem flist

felem:
	anypin
	{
		switch($1->code){
		default:
			yyerror("");
			break;
		case 0:
			yyundef($1);
			break;
		case INPUT:
		case OUTPUT:
		case BOTH:
			fields[nextfield++] = $1;
			if(nextfield >= NFIELDS){
				fprint(2, "too many fields\n");
#ifdef PLAN9
				exits("too many fields");
#else
				exit(1);
#endif
			}
		}
	}

anypin:
	INPUT | OUTPUT | BOTH

used:	
	USED { mode=INPUT; } ulist

ulist:
|	NAME {
		switch($1->code){
		default:
			yywarn("\"%s\" not an input", $1->name);
			break;
		case 0:
			yyundef($1);
			break;
		case INPUT:
		case BOTH:
		case FIELD:
			checkio($1, INPUT);
			setiused($1,ALLBITS);
			break;
		}
	} ulist

elist:
	{
		$$ = build(ELIST, 0, 0);
	}
|	eqn elist
	{
		$$ = build(ELIST, $1, $2);
	}

anyin:	INPUT | BOTH | FIELD
anyout:	OUTPUT | BOTH | FIELD

equop:	'=' | COMEQU | ADDEQU | SUBEQU | ALTEQU | XOREQU | MULEQU | MODEQU | PNDEQU

eqn:
	anyout equop eqnexpr
	{
		checkio($1, OUTPUT);
		switch($2){
		case COMEQU:
			setctl($1, CTLINV); break;
		case ADDEQU:
			$1 = addctl($1, 's'); break;
		case SUBEQU:
			$1 = addctl($1, 'c'); break;
		case ALTEQU:
			$1 = addctl($1, 'd'); break;
		case XOREQU:
			$1 = addctl($1, 't'); break;
		case MULEQU:
			$1 = addctl($1, 'e'); break;
		case MODEQU:
			$1 = addctl($1, 'g'); break;
		case PNDEQU:
			$1 = addctl($1, 'l'); break;
		}
		$$ = build(ASSIGN, (Node*)$1, $3);
		setoused($1,$$);
	}
|	anyout DC '=' eqnexpr
	{
		checkio($1, OUTPUT);
		$$ = build(DONTCARE, (Node*)$1, $4);
		setdcused($1,$$);
	}
|	NAME '=' e
	{
		if($1 != comname($1))
			fprint(2, "line %d warning: ambiguous lhs with compliment\n",
				yylineno);
		if(vconst($3)) {
			$1->code = CEQN;
			$1->val = eval($3);
			$$ = 0;
		} else {
			$1->code = EQN;
			$1->oref = ++neqns;
			if(neqns >= NLEAVES) {
				fprint(2, "too many eqns\n");
#ifdef PLAN9
				exits("too many eqns");
#else
				exit(1);
#endif
			}
			eqns[neqns] = $1;
			$$ = build(ASSIGN, (Node*)$1, $3);
		}
		setoused($1,$$);
	}

eqnexpr:	eqn | e

e:
	anyin
	{
		checkio($1, INPUT);
		$$ = build($1->code==FIELD ? FIELD : INPUT, (Node*)$1, 0);
		setiused($1,ALLBITS);
	}
|	NAME
	{
		$1 = comname($1);
		switch($1->code) {
		default:
			yyundef($1);
		case INPUT:
		case BOTH:
			$$ = build(INPUT, (Node*)$1, 0);
			$$ = build(NOT, $$, 0);
			setiused($1,ALLBITS);
			break;
		case FIELD:
			checkio($1, INPUT);
			$$ = build(FIELD,(Node*)$1,0);
			$$ = build(COM, $$, 0);
			setiused($1,ALLBITS);
			break;
		}
	}
|	EQN
	{
		$$ = build(EQN, (Node*)$1, 0);
		setiused($1,ALLBITS);
	}
|	CEQN
	{
		$$ = build(NUMBER, (Node*)$1->val, 0);
		setiused($1,ALLBITS);
	}
|	OUTPUT
	{
		yyerror("output used as input");
		$$ = build(NUMBER, 0, 0);
	}
|	NUMBER
	{
		$$ = build(NUMBER, (Node*)$1, 0);
	}
|	SUB e
	{
		$$ = build(NEG, $2, 0);
	}
|	NOT e
	{
		$$ = build(NOT, $2, 0);
	}
|	COM e
	{
		$$ = build(COM, $2, 0);
	}
|	LT e
	{
		$$ = build(FLONE, $2, 0);
	}
|	GT e
	{
		$$ = build(FRONE, $2, 0);
	}
|	MOD e
	{
		$$ = build(GREY, $2, 0);
	}
|	e CND e ALT e
	{
		$$ = build(CND, $1, build(ALT, $3, $5));
	}
|	'(' e ')' CND e ALT e
	{
		$$ = build(CND, $2, build(ALT, $5, $7));
	}
|	e NOT NUMBER NOT NUMBER
	{
		$$ = build(ALT, (Node*)$3, (Node*)$5);
		$$ = build(PERM, $1, $$);
	}
|	e LOR e
	{
	binop:
		$$ = build($2,$1,$3);
	}
|	e LAND e
	{
		goto binop;
	}
|	e laddop e	%prec OR
	{
		goto binop;
	}
|	e AND e
	{
		goto binop;
	}
|	e compop e	%prec EQ
	{
		goto binop;
	}
|	e addop e	%prec ADD
	{
		goto binop;
	}
|	e mulop e	%prec MUL
	{
		goto binop;
	}
|	e shiftop e	%prec LS
	{
		goto binop;
	}
|	e '{' swit '}'
	{
		$$ = build(SWITCH, $1, $3);
	}
|	'(' e ')'
	{
		$$ = $2;
	}
|	error
	{
		yyerror("expression syntax");
	}

laddop:		OR | XOR
compop:		EQ | NE | LE | GE | LT | GT
addop:		ADD | SUB
mulop:		MUL | DIV | MOD
shiftop:	LS | RS

swit:
	selem
|	swit ',' selem
	{
		Node *t;
		for(t = $1; t->t2; t = t->t2)
			;
		t->t2 = $3;
		$$ = $1;
	}

selem:
	e
	{
		$$ = build(CASE, $1, 0);
	}
|	ALT e
	{
		$$ = build(CASE, $2, 0);
		$$ = build(ALT, 0, $$);
	}
|	e ALT e
	{
		$$ = build(CASE, $3, 0);
		$$ = build(ALT, $1, $$);
	}
%%

static void
yymsg(int exitflag, char *s, void *args)
{
	extern char yytext[];
	extern yylineno;
	char buf[1024];

	errcount++;
	fprint(2, "%s:%d ", yyfile, yylineno);
	if(yytext[0])
		fprint(2, "sym \"%s\": ", yytext);
	if(s==0 || *s==0)
		fprint(2, "syntax error\n");
	else{
#ifdef PLAN9
		doprint(buf, buf+sizeof buf, s, args);
		fprint(2, "%s\n", buf);
#else
		fprint(2, s);
#endif
	}
	
	if(exitflag)
#ifdef PLAN9
	exits("syntax error");
#else
	exit(1);
#endif
}

void
yywarn(char *s, ...)
{
	yymsg(0, s, &s+1);
}

void
yyerror(char *s, ...)
{
	yymsg(1, s, &s+1);
}

void
yyundef(Hshtab *h)
{
	yywarn("%s undefined", h->name);
}

void
dclbit(Hshtab *h, Value x)
{
	if(h->code){
		yywarn("%s redefined", h->name);
		return;
	}
	h->code = mode;
	h->pin = x;
	h->iused = 0;
	h->assign = 0;
	switch(mode){
	case INPUT:
		h->iref = ++nleaves;
		if(nleaves >= NLEAVES) {
			fprint(2, "too many inputs\n");
#ifdef PLAN9
			exits("too many inputs");
#else
			exit(1);
#endif
		}
		leaves[nleaves] = h;
		break;

	case OUTPUT:
		h->oref = ++noutputs;
		if(noutputs >= NLEAVES) {
			fprint(2, "too many outputs\n");
#ifdef PLAN9
			exits("too many outputs");
#else
			exit(1);
#endif
		}
		outputs[noutputs] = h;
		break;

	case BURIED:
		h->type |= CTLNCN;
		h->code = BOTH;
		/* fall through */
	case BOTH:
		h->iref = ++nleaves;
		if(nleaves >= NLEAVES) {
			fprint(2, "too many inputs\n");
#ifdef PLAN9
			exits("too many inputs");
#else
			exit(1);
#endif
		}
		leaves[nleaves] = h;

		h->oref = ++noutputs;
		if(noutputs >= NLEAVES) {
			fprint(2, "too many outputs\n");
#ifdef PLAN9
			exits("too many outputs");
#else
			exit(1);
#endif
		}
		outputs[noutputs] = h;
		break;
	}
}

void
dclvector(Hshtab *h, Value n, int minus)
{
	char buf[40],name[40],cvs[20];
	int i,x;
	Hshtab *hp;
	strcpy(name,h->name);		/* use a copy */
	if (minus) {			/* ugly, shameful */
		strcpy(buf,name);
		strcat(buf,"-");
		h->name[0] = 0;
		h = lookup(buf,1);
	}
	for (i = 0; i < n; i++) {
		x = (n>10) ? 2 : 1;
		sprint(cvs,"%%s%%%d.%dd%%s",x,x);
		sprint(buf,cvs,name,i,minus?"-":"");
		hp = lookup(buf,1);
		dclbit(hp,0L);
		fields[nextfield++] = hp;
		if (nextfield >= NFIELDS) {
			fprint(2, "too many fields\n");
#ifdef PLAN9
			exits("too many fields");
#else
			exit(1);
#endif
		}
	}
	h->code = FIELD;
	SETLO(h, nextfield-n);
	SETHI(h, nextfield);
/* 	nextfield += n; */
}
