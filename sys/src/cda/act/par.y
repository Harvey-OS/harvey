%{
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>
#include "symbols.h"
#include "dat.h"
Tree **ltree;		/* repository for input expressions */
Tree *ONE,*ZERO,*CLOK,*CLK0,*CLK1;
int sense;		/* of the incoming expression */
Term eqn[10000];
int neqn;
%}
%union {
	int code;
	int val;
	Sym *sym;
	Tree *tree;
}
%token	<code>	OUTPUT XOR OTHER
%token	<code>	INV DFF TOG INT ENAB CKEN RESET LATCH PRESET
%token	<val>	NUMBER
%token	<sym>	NAME
%type	<sym>	decl deplist imp implist
%type	<tree>	file s
%%
file:
	s {
	}
|	file s {
	}

s:
	OTHER NAME {
		$$ = 0;
	}
|	OTHER NUMBER {
		$$ = 0;
	}
|	OUTPUT decl deplist implist {
		Tree *t;
		bitreverse(eqn,neqn,dep,ndep);
		t = tree(eqn, neqn);
		if (sense)
			t = notnode(t);
		*ltree = t;
		neqn = 0;
	}
|	OUTPUT decl implist {
		$$ = 0;
		*ltree = ONE;
		if (sense) {
			*ltree = ZERO;
			$2->inv = 0;
		}
		neqn = 0;
	}
|	OUTPUT decl {
		$$ = 0;
		*ltree = ZERO;
		if (sense) {
			*ltree = ONE;
			$2->inv = 0;
		}
		neqn = 0;
	}
|	XOR decl deplist {
		Tree *t;
		t = xortree(dep, ndep);
		if (sense)
			t = notnode(t);
		*ltree = downprop(t, 0);
		neqn = 0;
	}

decl:
	NAME {
		ltree = &$1->exp;
		$1->lval = 1;
		sense = 0;
	}
|	decl INV {
		$1->inv = 1;
		sense = 1;
	}
|	decl DFF {
		ltree = &$1->clk;
	}
|	decl TOG {
		ltree = &$1->clk;
		$1->tog = 1;
	}
|	decl LATCH {
		ltree = &$1->clk;
		$1->latch = 1;
	}
|	decl INT {
		$1->internal = 1;
	}
|	decl ENAB {
		ltree = &$1->ena;
	}
|	decl CKEN {
		ltree = &$1->cke;
	}
|	decl RESET {
		ltree = &$1->rd;
	}
|	decl PRESET {
		ltree = &$1->pre;
	}

deplist:
	NAME {
		$1->rval = 1;
		dep[0] = $1;
		ndep = 1;
	}
|	deplist NAME {
		$2->rval = 1;
		dep[ndep++] = $2;
	}

imp:
	NUMBER ':' NUMBER {
		eqn[neqn].value = $1;
		eqn[neqn].mask = $3;
		neqn++;
	}

implist:
	imp {
	}
|	imp implist {
	}
%%

char yytext[40];
int lineno=1;

void
yyerror(char *s, ...)
{
	char *out,buf[80];
	static nerr;
	fprintf(stderr,"line %d: ",lineno);
	out = doprint(buf,&buf[80],s,&s+1);
	*out++ = '\n';
	fwrite(buf,1,out-buf,stderr);
	fflush(stderr);
	if (nerr++ > 10)
		exits("too many errors");
}

int
yylex(void)
{
	int c,i;
	char *p=yytext;
	static int peek=0;
	c = peek ? peek : fgetc(stdin);
	if (isalpha(c) || c == '_') {
		for (; c != '@' && !isspace(c); c = fgetc(stdin))
			*p++ = c;
		peek = c;
		*p = 0;
		yylval.sym = lookup(yytext);
		return NAME;
	}
	else if (isdigit(c)) {
		for (i = 0; isdigit(c); c = fgetc(stdin))
			i = i*10 + (c - '0');
		peek = c;
		yylval.val = i;
		return NUMBER;
	}
	else if (c == '@') {
		peek = 0;
		switch (fgetc(stdin)) {
		case 'i':
			return INV;
		case 'b':
			return INT;
		case 'd':
			return DFF;
		case 't':
			return TOG;
		case 'g':
			return CKEN;
		case 'e':
			return ENAB;
		case 's':
			return PRESET;
		case 'c':
			return RESET;
		case 'l':
			return LATCH;
		default:
			yyerror("unknown qualification");
		}
	}
	else if (c == '.') {
		peek = 0;
		switch (fgetc(stdin)) {
		case 'X':
		case 'I':
		case 'x':
			while ((c = fgetc(stdin)) != '.')
				if (c == -1)
					return c;
			peek = '.';
			return yylex();
		case 'o':
			return OUTPUT;
		case '^':
			return XOR;	/* special for aek */
		default:
			return OTHER;
		}
	}
	peek = 0;
	if (c == '\n')
		lineno++;
	if (isspace(c))
		return yylex();
	return c;
}

int aflag;	/* output adl format */
int bflag;	/* flush output after every tree */
int cflag;	/* output c code */
int dflag;	/* debug, prints input trees */
int kflag=1;	/* karplus' ITE idea */
int mflag=1;	/* aggressive mux flag 0:wimp, 1: normal, >1 wacko */
int nflag;	/* count gates */
int uflag;	/* uniq, hashes output trees */
int vflag;	/* verbose, output pin names */
int xflag;	/* output xnf format */
int maxload=8;	/* max fanout */
int levcrit=100;	/* lower bound for net criticality */
FILE *devnull;
FILE *critfile;

void
main(int argc, char *argv[])
{
	int i,yyparse(void);
	char *p,buf[40];
	
	terminit();
	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 'a':
				aflag = 1;
				break;
			case 'b':
				bflag = 1;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'd':
				dflag = strlen(argv[i])-1;
				break;
			case 'f':
				maxload = atoi(argv[++i]);
				break;
			case 'k':
				kflag = 1-kflag;
				break;
			case 'l':
				levcrit = atoi(argv[++i]);
				break;
			case 'm':
				mflag = atoi(argv[++i]);
				break;
			case 'n':
				nflag = 1-nflag;
				break;
			case 'u':
				uflag = 1;
				break;
			case 'v':
				vflag = 1;
				break;
			case 'x':
				xflag = 1;
				break;
			}
		else {
			if (freopen(argv[i],"r",stdin) == 0)
				yyerror("can't open %s",argv[i]);
			for (p = argv[i]; *p && *p != '.'; p++)
				;
			*p = 0;
			sprint(buf, "%s.crt", argv[i]);
			critfile = fopen(buf, "w");
			fprintf(critfile, "DEF %s.\n", argv[i]);
		}
	devnull = fopen("/dev/null","w");
	ZERO = constnode(0);
	ONE = constnode(1);
	CLOK = idnode(lookup("clk"),2);
	CLK0 = idnode(lookup("clk0"),2);
	CLK1 = idnode(lookup("clk1"),2);
	_matchinit();
	yyparse();
	symout();
	fprintf(critfile, "END.\n");
	exits(0);
}
