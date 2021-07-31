
#line	2	"par.y"
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

#line	14	"par.y"
typedef union  {
	int code;
	int val;
	Sym *sym;
	Tree *tree;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define	OUTPUT	57346
#define	XOR	57347
#define	OTHER	57348
#define	INV	57349
#define	DFF	57350
#define	TOG	57351
#define	INT	57352
#define	ENAB	57353
#define	CKEN	57354
#define	RESET	57355
#define	LATCH	57356
#define	PRESET	57357
#define	NUMBER	57358
#define	NAME	57359
#define YYERRCODE 57344

#line	136	"par.y"


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
int mflag=1;	/* aggressive mux flag */
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
	int yyparse(void);
	int i;
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
				mflag = 1-mflag;
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
				yyerror("can't open %d",argv[i]);
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
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
};
#define	YYNPROD	24
#define	YYPRIVATE 57344
#define	YYLAST	46
short	yyact[] =
{
  14,  15,  16,  18,  19,  20,  21,  17,  22,  25,
  23,  14,  15,  16,  18,  19,  20,  21,  17,  22,
  13,  23,  31,  30,  25,  28,   8,   7,  28,  10,
  25,  12,   9,  27,   4,   5,   3,   2,  11,   6,
   1,  24,   0,  26,   0,  29
};
short	yypact[] =
{
  30,  30,-1000,  10,  12,  12,-1000,-1000,-1000,  -7,
-1000,   4,   8,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,  14,   5,  11,-1000,-1000,-1000,
   6,-1000
};
short	yypgo[] =
{
   0,  32,  31,  41,  20,  40,  37
};
short	yyr1[] =
{
   0,   5,   5,   6,   6,   6,   6,   6,   6,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   2,   3,   4,   4
};
short	yyr2[] =
{
   0,   1,   2,   2,   2,   4,   3,   2,   3,   1,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   1,
   2,   3,   1,   2
};
short	yychk[] =
{
-1000,  -5,  -6,   6,   4,   5,  -6,  17,  16,  -1,
  17,  -1,  -2,  -4,   7,   8,   9,  14,  10,  11,
  12,  13,  15,  17,  -3,  16,  -2,  -4,  17,  -4,
  18,  16
};
short	yydef[] =
{
   0,  -2,   1,   0,   0,   0,   2,   3,   4,   7,
   9,   0,   0,   6,  10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  22,   0,   8,   5,  20,  23,
   0,  21
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  18
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
#endif

/*	parser for yacc output	*/

int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */

char*	yytoknames[1];		/* for debugging */
char*	yystates[1];		/* for debugging */

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprintf(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprintf(x, "<%d>\n", yys);
	return x;
}

long
yylex1(void)
{
	long yychar;
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		if(yychar <= 0) {
			c = yytok1[0];
			goto out;
		}
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = yytok2[1];	/* unknown char */

out:
	if(yydebug >= 3)
		printf("lex %.4X %s\n", yychar, yytokname(c));
	return c;
}

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		printf("char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) { 
		yyerror("yacc stack overflow"); 
		return 1; 
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
		yychar = yylex1();
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
			yychar = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			return 0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 1) {
				printf("%s", yystatname(yystate));
				printf("saw %s\n", yytokname(yychar));
			}
			yynerrs++;

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					printf("error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			return 1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				printf("error recovery discards %s\n", yytokname(yychar));
			if(yychar == 0)
				return 1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		printf("reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 1:
#line	28	"par.y"
{
	} break;
case 2:
#line	30	"par.y"
{
	} break;
case 3:
#line	34	"par.y"
{
		yyval.tree = 0;
	} break;
case 4:
#line	37	"par.y"
{
		yyval.tree = 0;
	} break;
case 5:
#line	40	"par.y"
{
		Tree *t;
		bitreverse(eqn,neqn,dep,ndep);
		t = tree(eqn, neqn);
		if (sense)
			t = notnode(t);
		*ltree = t;
		neqn = 0;
	} break;
case 6:
#line	49	"par.y"
{
		yyval.tree = 0;
		*ltree = ONE;
		if (sense) {
			*ltree = ZERO;
			yypt[-1].yyv.sym->inv = 0;
		}
		neqn = 0;
	} break;
case 7:
#line	58	"par.y"
{
		yyval.tree = 0;
		*ltree = ZERO;
		if (sense) {
			*ltree = ONE;
			yypt[-0].yyv.sym->inv = 0;
		}
		neqn = 0;
	} break;
case 8:
#line	67	"par.y"
{
		Tree *t;
		t = xortree(dep, ndep);
		if (sense)
			t = notnode(t);
		*ltree = downprop(t, 0);
		neqn = 0;
	} break;
case 9:
#line	77	"par.y"
{
		ltree = &yypt[-0].yyv.sym->exp;
		yypt[-0].yyv.sym->lval = 1;
		sense = 0;
	} break;
case 10:
#line	82	"par.y"
{
		yypt[-1].yyv.sym->inv = 1;
		sense = 1;
	} break;
case 11:
#line	86	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
	} break;
case 12:
#line	89	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
		yypt[-1].yyv.sym->tog = 1;
	} break;
case 13:
#line	93	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
		yypt[-1].yyv.sym->latch = 1;
	} break;
case 14:
#line	97	"par.y"
{
		yypt[-1].yyv.sym->internal = 1;
	} break;
case 15:
#line	100	"par.y"
{
		ltree = &yypt[-1].yyv.sym->ena;
	} break;
case 16:
#line	103	"par.y"
{
		ltree = &yypt[-1].yyv.sym->cke;
	} break;
case 17:
#line	106	"par.y"
{
		ltree = &yypt[-1].yyv.sym->rd;
	} break;
case 18:
#line	109	"par.y"
{
		ltree = &yypt[-1].yyv.sym->pre;
	} break;
case 19:
#line	114	"par.y"
{
		yypt[-0].yyv.sym->rval = 1;
		dep[0] = yypt[-0].yyv.sym;
		ndep = 1;
	} break;
case 20:
#line	119	"par.y"
{
		yypt[-0].yyv.sym->rval = 1;
		dep[ndep++] = yypt[-0].yyv.sym;
	} break;
case 21:
#line	125	"par.y"
{
		eqn[neqn].value = yypt[-2].yyv.val;
		eqn[neqn].mask = yypt[-0].yyv.val;
		neqn++;
	} break;
case 22:
#line	132	"par.y"
{
	} break;
case 23:
#line	134	"par.y"
{
	} break;
	}
	goto yystack;  /* stack new state and value */
}
