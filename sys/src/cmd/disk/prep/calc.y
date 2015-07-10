%{
#include <u.h>

#define YYSIZE_T size_t

typedef struct Exp Exp;
enum {
	NUM,
	DOT,
	DOLLAR,
	ADD,
	SUB,
	MUL,
	DIV,
	FRAC,
	NEG,
};

struct Exp {
	int ty;
	long long n;
	Exp *e1;
	Exp *e2;
};

typedef Exp* Expptr;
#define YYSTYPE Expptr
Exp *yyexp;

static Exp* mkOP(int ty, Exp *e1, Exp *e2);
static Exp* mkNUM(int64_t x);
static int yylex(void);
static void yyerror(char *s);

%}

%token NUMBER

%left '+' '-'
%left '*' '/'
%left UNARYMINUS '%'
%%
top:	expr	{ yyexp = $1; return 0; }

expr:	NUMBER
	| '.'	{ $$ = mkOP(DOT, nil, nil); }
	| '$'	{ $$ = mkOP(DOLLAR, nil, nil); }
	| '(' expr ')'	{ $$ = $2; }
	| expr '+' expr	{ $$ = mkOP(ADD, $1, $3); }
	| expr '-' expr 	{ $$ = mkOP(SUB, $1, $3); }
	| expr '*' expr	{ $$ = mkOP(MUL, $1, $3); }
	| expr '/' expr	{ $$ = mkOP(DIV, $1, $3); }
	| expr '%'		{ $$ = mkOP(FRAC, $1, nil); }
	| '-' expr %prec UNARYMINUS	{ $$ = mkOP(NEG, $2, nil); }
	;

%%

#include <libc.h>
#include <ctype.h>
#include "disk.h"
#include "edit.h"


static Exp*
mkNUM(int64_t x)
{
	Exp *n;

	n = emalloc(sizeof *n);

	n->ty = NUM;
	n->n = x;
	return n;
}

static Exp*
mkOP(int ty, Exp *e1, Exp *e2)
{
	Exp *n;

	n = emalloc(sizeof *n);
	n->ty = ty;
	n->e1 = e1;
	n->e2 = e2;

	return n;
}

static char *inp;
static jmp_buf jmp;
static int64_t dot, size, dollar;
static char** errp;

static int
yylex(void)
{
	int c;
	uint64_t n;

	while(isspace(*inp))
		inp++;

	if(*inp == 0)
		return 0;

	if(isdigit(*inp)) {
		n = strtoull(inp, &inp, 0);	/* default unit is sectors */
		c = *inp++;
		if(isascii(c) && isupper(c))
			c = tolower(c);
		switch(c) {
		case 't':
			n *= 1024;
			/* fall through */
		case 'g':
			n *= 1024;
			/* fall through */
		case 'm':
			n *= 1024;
			/* fall through */
		case 'k':
			n *= 2;
			break;
		default:
			--inp;
			break;
		}
		yylval = mkNUM(n);
		return NUMBER;
	}
	return *inp++;
}

static void
yyerror(char *s)
{
	*errp = s;
	longjmp(jmp, 1);
}

static int64_t
eval(Exp *e)
{
	int64_t i;

	switch(e->ty) {
	case NUM:
		return e->n;
	case DOT:
		return dot;
	case DOLLAR:
		return dollar;
	case ADD:
		return eval(e->e1)+eval(e->e2);
	case SUB:
		return eval(e->e1)-eval(e->e2);
	case MUL:
		return eval(e->e1)*eval(e->e2);
	case DIV:
		i = eval(e->e2);
		if(i == 0)
			yyerror("division by zero");
		return eval(e->e1)/i;
	case FRAC:
		return (size*eval(e->e1))/100;
	case NEG:
		return -eval(e->e1);
	}
	assert(0);
	return 0;
}

int yyparse(void);

char*
parseexpr(char *s, int64_t xdot, int64_t xdollar, int64_t xsize, int64_t *result)
{
	char *err;

	errp = &err;
	if(setjmp(jmp))
		return err;

	inp = s;
	dot = xdot;
	size = xsize;
	dollar = xdollar;
	yyparse();
	if(yyexp == nil)
		return "nil yylval?";
	*result = eval(yyexp);
	return nil;
}

#ifdef TEST
void
main(int argc, char **argv)
{
	int i;
	int64_t r;
	char *e;

	for(i=1; i<argc; i++)
		if(e = parseexpr(argv[i], 1000, 1000000, 1000000, &r))
			print("%s\n", e);
		else
			print("%lld\n", r);
}
#endif
