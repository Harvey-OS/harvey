
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
#define	MACRO	57349
#define	INV	57350
#define	DFF	57351
#define	TOG	57352
#define	INT	57353
#define	ENAB	57354
#define	CKEN	57355
#define	RESET	57356
#define	LATCH	57357
#define	PRESET	57358
#define	NUMBER	57359
#define	NAME	57360
#define	MACPIN	57361
#define YYEOFCODE 1
#define YYERRCODE 2

#line	131	"par.y"


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
	char *pin=0;
	char *p=yytext;
	static int peek=0;
	c = peek ? peek : fgetc(stdin);
	if (isalpha(c) || c == '_') {
		for (; c != '@' && !isspace(c); c = fgetc(stdin))
			if ((*p++ = c) == '.')
				pin = p;
		peek = c;
		*p = 0;
		if (pin) {
			pin[-1] = 0;
			yylval.sym = looktup(yytext, pin);
			return MACPIN;
		}
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
		case 'm':
			return MACRO;
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

int aflag=1;	/* avenge node invisibilities */
int cflag;	/* output c code */
int dflag;	/* debug, prints input trees */
int kflag=1;	/* karplus' ITE idea */
int mflag=1;	/* aggressive mux flag */
int nflag;	/* count gates */
int uflag;	/* uniq, hashes output trees */
int vflag;	/* verbose, output pin names */
int xflag;	/* output xnf format */
int fflag;	/* flatten */
int levcrit=100;	/* lower bound for net criticality */
FILE *devnull;
FILE *netfile;

int
binconv(void *o, Fconv *fp)
{
	ulong m, i=fp->f1;
	char buf[50];
	char *p=&buf[sizeof buf];
	m = *((ulong *) o);
	*--p = 0;
	for (; i > 0; i--, m >>= 1)
		*--p = m&1 ? '1' : '0';
	strconv(p, fp);
	return sizeof(ulong);
}

char *boilerplate = "LCANET, 4\n\
USER,\n\
PROG, XIL\n\
PWR, 0, GND, VSS\n\
PWR, 1, VCC, VDD\n";

void
main(int argc, char *argv[])
{
	int yyparse(void);
	int i;
	char *p,buf[40];
	char *part = "4005PC84";
	
	terminit();
	fmtinstall('b', binconv);
	fmtinstall('P', pinconv);
	fmtinstall('T', treeconv);
	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 'a':
				aflag = 1-aflag;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'd':
				dflag = strlen(argv[i])-1;
				break;
			case 'f':
				fflag = 1-fflag;
				break;
			case 'k':
				kflag = 1;
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
			case 'p':
				part = argv[++i];
				break;
			case 'u':
				uflag = 1;
				break;
			case 'v':
				vflag = 1;
				break;
			case 'x':
				xflag = fflag = 1;
				break;
			}
		else {
			if (freopen(argv[i],"r",stdin) == 0)
				yyerror("can't open %d",argv[i]);
			for (p = argv[i]; *p && *p != '.'; p++)
				;
			*p = 0;
			sprint(buf, "%s.crt", argv[i]);
			netfile = fopen(buf, "w");
			fprintf(netfile, "DEF %s.\n", argv[i]);
		}
	ZERO = constnode(0);
	ONE = constnode(1);
	yyparse();
	apply(merge);
	apply(dovis);
	if (xflag) {
		print(boilerplate);
		print("PART, %s\n", part);
		apply(outsym);
		print("EOF\n");
	}
	else
		apply(prsym);
	exits(0);
}
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
};
#define	YYNPROD	27
#define	YYPRIVATE 57344
#define	YYLAST	56
short	yyact[] =
{
  12,  18,  36,  19,  20,  21,  23,  24,  25,  26,
  22,  27,  28,  13,  14,  32,  28,  10,  34,  33,
  30,  13,  14,  13,  14,   9,   8,  37,  30,  16,
  17,  35,  34,  19,  20,  21,  23,  24,  25,  26,
  22,  27,  30,  13,  14,  11,  31,   5,   6,   3,
   4,   2,  15,   7,   1,  29
};
short	yypact[] =
{
  43,  43,-1000,   8,  -1,   5,   5,-1000,-1000,-1000,
  12,  25,-1000,-1000,-1000,  -5,  -3,   3,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  11,
 -18,   5,-1000,-1000,-1000,-1000,  10,-1000
};
short	yypgo[] =
{
   0,  45,  30,  55,   1,   0,  54,  51
};
short	yyr1[] =
{
   0,   6,   6,   7,   7,   7,   7,   7,   7,   7,
   5,   5,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   2,   2,   3,   4,   4
};
short	yyr2[] =
{
   0,   1,   2,   2,   2,   4,   4,   3,   2,   3,
   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   1,   2,   3,   1,   2
};
short	yychk[] =
{
-1000,  -6,  -7,   6,   7,   4,   5,  -7,  18,  17,
  18,  -1,  -5,  18,  19,  -1,  17,  -2,  -4,   8,
   9,  10,  15,  11,  12,  13,  14,  16,  -5,  -3,
  17,  -2,  18,  -4,  -5,  -4,  20,  17
};
short	yydef[] =
{
   0,  -2,   1,   0,   0,   0,   0,   2,   3,   4,
   0,   8,  12,  10,  11,   0,   0,   0,   7,  13,
  14,  15,  16,  17,  18,  19,  20,  21,  22,  25,
   0,   9,   5,   6,  23,  26,   0,  24
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  20
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19
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
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
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
yyerrlab:
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
			if(yydebug >= YYEOFCODE)
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
		definst(yypt[-2].yyv.sym, yypt[-1].yyv.val, yypt[-0].yyv.sym);
	} break;
case 6:
#line	43	"par.y"
{
		Tree *t;
		bitreverse(eqn,neqn,dep,ndep);
		t = tree(eqn, neqn);
		if (yypt[-2].yyv.sym->inv)
			t = notnode(t);
		*ltree = uflag ? uniq(t) : t;
		neqn = 0;
	} break;
case 7:
#line	52	"par.y"
{
		yyval.tree = 0;
		*ltree = yypt[-1].yyv.sym->inv ? ZERO : ONE;
		neqn = 0;
	} break;
case 8:
#line	57	"par.y"
{
		yyval.tree = 0;
		*ltree = yypt[-0].yyv.sym->inv ? ONE : ZERO;
		neqn = 0;
	} break;
case 9:
#line	62	"par.y"
{
		Tree *t;
		t = xortree(dep, 0, ndep);
		if (yypt[-1].yyv.sym->inv)
			t = notnode(t);
		*ltree = uflag ? uniq(t) : t;
		neqn = 0;
	} break;
case 12:
#line	74	"par.y"
{
		ltree = &yypt[-0].yyv.sym->exp;
		yypt[-0].yyv.sym->lval = 1;
	} break;
case 13:
#line	78	"par.y"
{
		yypt[-1].yyv.sym->inv = 1;
	} break;
case 14:
#line	81	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
	} break;
case 15:
#line	84	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
		yypt[-1].yyv.sym->tog = 1;
	} break;
case 16:
#line	88	"par.y"
{
		ltree = &yypt[-1].yyv.sym->clk;
		yypt[-1].yyv.sym->latch = 1;
	} break;
case 17:
#line	92	"par.y"
{
		yypt[-1].yyv.sym->internal = 1;
	} break;
case 18:
#line	95	"par.y"
{
		ltree = &yypt[-1].yyv.sym->ena;
	} break;
case 19:
#line	98	"par.y"
{
		ltree = &yypt[-1].yyv.sym->cke;
	} break;
case 20:
#line	101	"par.y"
{
		ltree = &yypt[-1].yyv.sym->rd;
	} break;
case 21:
#line	104	"par.y"
{
		ltree = &yypt[-1].yyv.sym->pre;
	} break;
case 22:
#line	109	"par.y"
{
		yypt[-0].yyv.sym->rval = 1;
		dep[0] = yypt[-0].yyv.sym;
		ndep = 1;
	} break;
case 23:
#line	114	"par.y"
{
		yypt[-0].yyv.sym->rval = 1;
		dep[ndep++] = yypt[-0].yyv.sym;
	} break;
case 24:
#line	120	"par.y"
{
		eqn[neqn].value = yypt[-2].yyv.val;
		eqn[neqn].mask = yypt[-0].yyv.val;
		neqn++;
	} break;
case 25:
#line	127	"par.y"
{
	} break;
case 26:
#line	129	"par.y"
{
	} break;
	}
	goto yystack;  /* stack new state and value */
}
