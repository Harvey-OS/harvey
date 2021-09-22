/* Yacc productions for "expr" command: */

%token OR AND ADD SUBT MULT DIV REM EQ GT GEQ LT LEQ NEQ
%token A_STRING SUBSTR LENGTH INDEX NOARG MATCH

/* operators listed below in increasing precedence: */
%left OR
%left AND
%left EQ LT GT GEQ LEQ NEQ
%left ADD SUBT
%left MULT DIV REM
%left MCH
%left MATCH
%left SUBSTR
%left LENGTH INDEX

%{
#define YYSTYPE charp

typedef char *charp;
%}

%%

/* a single `expression' is evaluated and printed: */

expression:	expr NOARG = {
			prt(1, $1);
			exit((!strcmp($1,"0")||!strcmp($1,"\0"))? 1: 0);
			}
	;


expr:	'(' expr ')' = { $$ = $2; }
	| expr OR expr   = { $$ = conj(OR, $1, $3); }
	| expr AND expr   = { $$ = conj(AND, $1, $3); }
	| expr EQ expr   = { $$ = rel(EQ, $1, $3); }
	| expr GT expr   = { $$ = rel(GT, $1, $3); }
	| expr GEQ expr   = { $$ = rel(GEQ, $1, $3); }
	| expr LT expr   = { $$ = rel(LT, $1, $3); }
	| expr LEQ expr   = { $$ = rel(LEQ, $1, $3); }
	| expr NEQ expr   = { $$ = rel(NEQ, $1, $3); }
	| expr ADD expr   = { $$ = arith(ADD, $1, $3); }
	| expr SUBT expr   = { $$ = arith(SUBT, $1, $3); }
	| expr MULT expr   = { $$ = arith(MULT, $1, $3); }
	| expr DIV expr   = { $$ = arith(DIV, $1, $3); }
	| expr REM expr   = { $$ = arith(REM, $1, $3); }
	| expr MCH expr	 = { $$ = match($1, $3); }
	| MATCH expr expr = { $$ = match($2, $3); }
	| SUBSTR expr expr expr = { $$ = substr($2, $3, $4); }
	| LENGTH expr       = { $$ = length($2); }
	| INDEX expr expr = { $$ = index($2, $3); }
	| A_STRING
	;
%%
/*	expression command */
#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* get rid of yacc debug printf's */
#define printf
#define ESIZE	512
#define error(c)	errxx(c)
#define EQL(x,y) !strcmp(x,y)

int	advance(char *lp, char *ep);
char	*arith(int op, char *r1, char *r2);
char	*compile(char *instring, char *ep, char *endbuf, int seof);
char	*conj(int op, char *r1, char *r2);
int	ecmp(char *a, char *b, int count);
int	ematch(char *s, char *p);
void	errxx(int c);
void	getrnge(char *str);
char	*index(char *s, char *t);
char	*length(char *s);
char	*ltoa(long l);
void	prt(int fd, char *s);
char	*rel(int op, char *r1, char *r2);
char	*substr(char *v, char *s, char *w);
void	yyerror(char *s);
int	yyparse(void);

char	**Av;
int	Ac;
int	Argi;
char Mstring[1][128];

extern int nbra;

void
main(int argc, char **argv)
{
	Ac = argc;
	Argi = 1;
	Av = argv;
	yyparse();
}

char *operator[] = { "|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", "substr", "length", "index", "\0" };
int op[] = { OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH, SUBSTR, LENGTH, INDEX };

int
yylex(void)
{
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi++];

	if(*p == '(' || *p == ')')
		return (int)*p;
	for(i = 0; *operator[i]; ++i)
		if(EQL(operator[i], p))
			return op[i];

	yylval = p;
	return A_STRING;
}

char *
rel(int op, char *r1, char *r2)
{
	int i;

	if(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$"))
		i = atol(r1) - atol(r2);
	else
		i = strcmp(r1, r2);
	switch(op) {
	case EQ: i = i==0; break;
	case GT: i = i>0; break;
	case GEQ: i = i>=0; break;
	case LT: i = i<0; break;
	case LEQ: i = i<=0; break;
	case NEQ: i = i!=0; break;
	}
	return i? "1": "0";
}

char *
arith(int op, char *r1, char *r2)
{
	long i1, i2;
	char *rv;

	if(!(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$")))
		yyerror("non-numeric argument");
	i1 = atol(r1);
	i2 = atol(r2);

	switch(op) {
	case ADD: i1 = i1 + i2; break;
	case SUBT: i1 = i1 - i2; break;
	case MULT: i1 = i1 * i2; break;
	case DIV: i1 = i1 / i2; break;
	case REM: i1 = i1 % i2; break;
	}
	rv = malloc(16);
	strcpy(rv, ltoa(i1));
	return rv;
}

char *
conj(int op, char *r1, char *r2)
{
	char *rv = "0";

	switch(op) {
	case OR:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
			if(EQL(r2, "0")
			|| EQL(r2, ""))
				rv = "0";
			else
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		|| EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *
substr(char *v, char *s, char *w)
{
	int si, wi;
	char *res;

	si = atol(s);
	wi = atol(w);
	while(--si) if(*v) ++v;

	res = v;

	while(wi--) if(*v) ++v;

	*v = '\0';
	return res;
}

char *
length(char *s)
{
	int i = 0;
	char *rv;

	while(*s++) ++i;

	rv = malloc(8);
	if (rv)
		strcpy(rv, ltoa((long)i));
	return rv;
}

char *
index(char *s, char *t)
{
	int i, j;
	char *rv;

	for(i = 0; s[i] ; ++i)
		for(j = 0; t[j] ; ++j)
			if(s[i]==t[j]) {
				strcpy(rv=malloc(8), ltoa((long)++i));
				return rv;
			}
	return "0";
}

char *
match(char *s, char *p)
{
	register char *rv;

	strcpy(rv=malloc(8), ltoa(ematch(s, p)));
	if(nbra) {
		rv = malloc(strlen(Mstring[0])+1);
		if (rv)
			strcpy(rv, Mstring[0]);
	}
	return rv;
}

#define INIT		char *sp = instring;
#define GETC()		(*sp++)
#define PEEKC()		(*sp)
#define UNGETC(c)	(--sp)		/* c must be char consumed */
#define RETURN(c)	return(c)
#define ERROR(c)	errxx(c)

int
ematch(char *s, char *p)
{
	static char expbuf[ESIZE];
	register num;
	extern char *braslist[], *braelist[], *loc2;

	compile(p, expbuf, &expbuf[ESIZE], 0);
	if(nbra > 1)
		yyerror("Too many '\\('s");
	if(advance(s, expbuf)) {
		if(nbra == 1) {
			p = braslist[0];
			num = braelist[0] - p;
			strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
		return(loc2-s);
	}
	return(0);
}

void
errxx(int)
{
	yyerror("RE error");
}

#include  "regexp.h"

void
yyerror(char *s)
{
	write(2, "expr: ", 6);
	prt(2, s);
	exit(2);
}

void
prt(int fd, char *s)
{
	write(fd, s, strlen(s));
	write(fd, "\n", 1);
}

char *
ltoa(long l)
{
	static char str[20];
	register char *sp = &str[18];
	register i;
	register neg = 0;

	if(l < 0)
		++neg, l *= -1;
	str[19] = '\0';
	do {
		i = l % 10;
		*sp-- = '0' + i;
		l /= 10;
	} while(l);
	if(neg)
		*sp-- = '-';
	return ++sp;
}
