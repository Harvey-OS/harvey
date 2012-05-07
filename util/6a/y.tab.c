
#line	2	"/Users/rminnich/src/nxm/util/6a/a.y"
#include "a.h"

#line	4	"/Users/rminnich/src/nxm/util/6a/a.y"
typedef union 	{
	Sym	*sym;
	vlong	lval;
	double	dval;
	char	sval[8];
	Gen	gen;
	Gen2	gen2;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define	LTYPE0	57346
#define	LTYPE1	57347
#define	LTYPE2	57348
#define	LTYPE3	57349
#define	LTYPE4	57350
#define	LTYPEC	57351
#define	LTYPED	57352
#define	LTYPEN	57353
#define	LTYPER	57354
#define	LTYPET	57355
#define	LTYPES	57356
#define	LTYPEM	57357
#define	LTYPEI	57358
#define	LTYPEXC	57359
#define	LTYPEX	57360
#define	LTYPERT	57361
#define	LCONST	57362
#define	LFP	57363
#define	LPC	57364
#define	LSB	57365
#define	LBREG	57366
#define	LLREG	57367
#define	LSREG	57368
#define	LFREG	57369
#define	LMREG	57370
#define	LXREG	57371
#define	LFCONST	57372
#define	LSCONST	57373
#define	LSP	57374
#define	LNAME	57375
#define	LLAB	57376
#define	LVAR	57377
#define YYEOFCODE 1
#define YYERRCODE 2
static	const	short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
};
#define	YYNPROD	120
#define	YYPRIVATE 57344
#define	yydebug	1
#define	YYLAST	515
static	const	short	yyact[] =
{
  47,  36,  59,  45,  46,   2, 116,  49,  78,  60,
 235, 192,  57, 234, 111,  68,  53, 126,  84,  82,
  79,  83,  80,  73,  97,  62,  99, 101, 105, 232,
 228, 105,  91,  93,  95,  67, 226,  50, 217, 215,
 160, 203, 104, 202, 193, 106, 161, 124, 123, 105,
  55, 191,  51, 113, 114, 115, 218, 125, 212, 194,
  54, 121, 164, 138, 130, 112, 108,  68, 122,  53,
  52, 209, 105, 208, 205, 204, 131, 132,  84,  82,
 159,  83,  80, 136, 144, 137, 135, 129, 143, 142,
  50, 141, 139,  58,  38,  40,  43,  39,  41,  44,
 140, 134,  42,  55,  35,  51, 128,  64, 145, 146,
  48, 127,  35,  54, 120,  32,  26,  30,  27, 166,
 167,  29,  28, 207, 206,  25, 105, 113, 107, 200,
  55, 198, 157, 172, 173, 175, 157, 174, 222, 158,
 199, 171, 225, 158, 221, 214, 163, 229, 105, 105,
 105, 105, 105, 172, 133, 105, 105, 105, 149, 150,
 151, 195, 181, 182, 183, 184, 185,  33,  37, 188,
 189, 190, 201, 156, 155, 154, 152, 153, 147, 148,
 149, 150, 151, 107,  31,  75,  88, 105, 105,  53,
  52,  81, 117, 213, 118, 119,   6, 216, 230, 227,
 196, 210, 211,  87, 118, 119, 187, 219, 220, 223,
  50, 224, 165, 186,  38,  40,  43,  39,  41,  44,
 162, 170,  42,  85,  70,  51, 103, 102, 231,  77,
  48, 233,   1,  54, 176, 177, 178, 179, 180,  53,
  52,  81,  38,  40,  43,  39,  41,  44,  98,  96,
  42, 147, 148, 149, 150, 151,  94,  53,  52,  92,
  50,  53,  52,  90,  38,  40,  43,  39,  41,  44,
  86,  76,  42,  85,  70,  51,  74,  72,  50, 100,
  48,  63,  50,  54,  61,  56,  38,  40,  43,  39,
  41,  44,  65,  51,  42,  55,   7,  51,  71, 197,
  60,  54,  48,   0,  60,  54,   0,   0,   9,  10,
  11,  12,  13,  17,  15,  18,  14,  16,  19,  20,
  21,  22,  23,  24,  53,  52, 156, 155, 154, 152,
 153, 147, 148, 149, 150, 151,   0,   4,   3,   8,
   0,   5,   0,   0,   0,  50,  53,  52,   0,  38,
  40,  43,  39,  41,  44,   0,   0,  42,  55,   0,
  51,   0,   0,   0,  89,  48,   0,  50,  54,   0,
   0,  38,  40,  43,  39,  41,  44,  53,  52,  42,
  55,   0,  51,   0,   0,   0,  34,  48,  53,  52,
  54, 152, 153, 147, 148, 149, 150, 151,  50,  53,
  52,   0,  38,  40,  43,  39,  41,  44,   0,  50,
  42,   0,   0,  51,   0,  53,  52,   0,  48,   0,
  50,  54,  69,  70,  51,   0,  53,  52,  66,  71,
   0,   0,  54,  69,  70,  51,  50,  53,  52,   0,
  71, 109,   0,  54,  53,  52,   0,  50, 110,   0,
   0,  51,   0,  53,  52,   0,  71, 169,  50,  54,
   0,   0,  51,   0,   0,  50,   0,  71, 168,   0,
  54,   0,   0,  51,  50,   0,   0,   0,  71,   0,
  51,  54,   0,   0,   0,  71,   0,  55,  54,  51,
   0,   0,   0,   0,  48,   0,   0,  54, 155, 154,
 152, 153, 147, 148, 149, 150, 151, 154, 152, 153,
 147, 148, 149, 150, 151
};
static	const	short	yypact[] =
{
-1000, 294,-1000,  79,  70,-1000,  75,  74,  69,  66,
 337, 252, 252,  60, 379,  87, 444, 180, 315, 252,
 252, 252, 208, -43, -43,-1000,-1000, 435,-1000,-1000,
 435,-1000,-1000,-1000,  60,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,  16, 406,  15,
-1000,-1000, 435, 435, 435, 185,-1000,  65,-1000,-1000,
   7,-1000,  62,-1000,  57,-1000, 390,-1000,  14, 195,
 195, 435,-1000, 142,-1000,  52,-1000, 230,-1000,-1000,
-1000, 368,-1000,-1000,  13, 185,-1000,-1000,-1000,  60,
-1000,  51,-1000,  42,-1000,  40,-1000,  39,-1000,  35,
-1000,-1000, 294, 294, 322,-1000, 322,-1000,  97,  29,
  -5, 169, 111,-1000,-1000,-1000,  12, 204, 435, 435,
-1000,-1000,-1000,-1000,-1000, 428, 417,  60, 252,-1000,
 121,-1000,-1000, 435, 248,-1000,-1000,-1000, 101,  12,
  60,  60,  60,  60,  60,-1000,-1000, 435, 435, 435,
 435, 435, 206, 198, 435, 435, 435,   0,  -7,   9,
 435,-1000,-1000, 189,  98, 195,-1000,-1000,  -8,-1000,
-1000,-1000, -10,  26,-1000,  25,  78,  77,-1000,  24,
  22, 147, 147,-1000,-1000,-1000, 435, 435, 384, 501,
 493,   8, 435,-1000, 110, -12, 435, -13,-1000,-1000,
-1000,   6,-1000,-1000, -43, -43, 109, 102, 435, 208,
 242, 242, 107, -15, 188,-1000, -21,-1000, 114,-1000,
-1000,-1000,-1000,-1000,-1000, 187,-1000, 435,-1000, -22,
 435, -38,-1000, -41,-1000,-1000
};
static	const	short	yypgo[] =
{
   0,   0,  14, 299,   6, 168,   2,   1,   7,  20,
  93,  12,   8,   3,   4, 184, 292, 167, 285, 284,
 281, 279, 277, 276, 271, 270, 263, 259, 256, 249,
 248, 232,   5, 227, 226, 196
};
static	const	short	yyr1[] =
{
   0,  31,  31,  33,  32,  34,  32,  32,  32,  32,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  15,  15,
  19,  20,  18,  18,  17,  17,  16,  16,  22,  23,
  23,  24,  24,  25,  25,  26,  26,  27,  27,  28,
  28,  28,  29,  30,  21,  21,  10,  10,  12,  12,
  12,  12,  12,  12,  11,  11,   9,   9,   9,   7,
   7,   7,   7,   7,   7,   7,   6,   6,   6,   6,
   6,   6,   5,   5,  13,  13,  13,  13,  13,  13,
  13,  13,  13,  14,  14,   8,   8,   4,   4,   4,
   3,   3,   3,   1,   1,   1,   1,   1,   1,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2
};
static	const	short	yyr2[] =
{
   0,   0,   2,   0,   4,   0,   4,   1,   2,   2,
   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   0,   1,
   3,   3,   2,   1,   2,   1,   2,   1,   5,   3,
   5,   2,   1,   1,   1,   3,   5,   3,   5,   2,
   1,   3,   5,   5,   0,   1,   1,   1,   1,   1,
   2,   2,   1,   1,   1,   1,   4,   2,   2,   1,
   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
   4,   3,   1,   1,   1,   4,   4,   6,   9,   3,
   3,   5,   8,   1,   6,   5,   7,   0,   2,   2,
   1,   1,   1,   1,   1,   2,   2,   2,   3,   1,
   3,   3,   3,   3,   3,   4,   4,   3,   3,   3
};
static	const	short	yychk[] =
{
-1000, -31, -32,  44,  43,  47, -35,   2,  45,  14,
  15,  16,  17,  18,  22,  20,  23,  19,  21,  24,
  25,  26,  27,  28,  29,  46,  46,  48,  47,  47,
  48, -15,  49, -17,  49, -10,  -7,  -5,  34,  37,
  35,  38,  42,  36,  39, -13, -14,  -1,  50,  -8,
  30,  45,  10,   9,  53,  43, -18, -11, -10,  -6,
  52, -19, -11, -20, -10, -16,  49,  -9,  -1,  43,
  44,  50, -22,  -8, -23,  -5, -24,  49, -12,  -9,
 -14,  11,  -7, -13,  -1,  43, -25, -15, -17,  49,
 -26, -11, -27, -11, -28, -11, -29,  -7, -30,  -6,
 -21,  -6, -33, -34,  -2,  -1,  -2, -10,  50,  35,
  42,  -2,  50,  -1,  -1,  -1,  -4,   7,   9,  10,
  49,  -1,  -8,  41,  40,  50,  10,  49,  49,  -9,
  50,  -4,  -4,  12,  49, -12,  -7, -13,  50,  -4,
  49,  49,  49,  49,  49, -32, -32,   9,  10,  11,
  12,  13,   7,   8,   6,   5,   4,  35,  42,  51,
  11,  51,  51,  35,  50,   8,  -1,  -1,  40,  40,
 -10, -11,  32,  -1,  -6,  -1, -10, -10, -10, -10,
 -10,  -2,  -2,  -2,  -2,  -2,   7,   8,  -2,  -2,
  -2,  51,  11,  51,  50,  -1,  11,  -3,  33,  42,
  31,  -4,  51,  51,  49,  49,  46,  46,  49,  49,
  -2,  -2,  50,  -1,  35,  51,  -1,  51,  50,  -6,
  -6,  35,  36,  -1,  -7,  35,  51,  11,  51,  33,
  11,  -1,  51,  -1,  51,  51
};
static	const	short	yydef[] =
{
   1,  -2,   2,   0,   0,   7,   0,   0,   0,  28,
   0,   0,   0,   0,   0,   0,   0,   0,  28,   0,
   0,   0,   0,   0,  54,   3,   5,   0,   8,   9,
   0,  12,  29,  13,   0,  35,  56,  57,  69,  70,
  71,  72,  73,  74,  75,  82,  83,  84,   0,  93,
 103, 104,   0,   0,   0,  97,  14,  33,  64,  65,
   0,  15,   0,  16,   0,  17,   0,  37,   0,  97,
  97,   0,  18,   0,  19,   0,  20,   0,  42,  58,
  59,   0,  62,  63,  84,  97,  21,  43,  44,  29,
  22,   0,  23,   0,  24,  50,  25,   0,  26,   0,
  27,  55,   0,   0,  10, 109,  11,  34,   0,   0,
   0,   0,   0, 105, 106, 107,   0,   0,   0,   0,
  32,  76,  77,  78,  79,   0,   0,   0,   0,  36,
   0,  67,  68,   0,   0,  41,  60,  61,   0,  67,
   0,   0,  49,   0,   0,   4,   6,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  89,
   0,  90, 108,   0,   0,  97,  98,  99,   0,  81,
  30,  31,   0,   0,  39,   0,  45,  47,  51,   0,
   0, 110, 111, 112, 113, 114,   0,   0, 117, 118,
 119,  85,   0,  86,   0,   0,   0,   0, 100, 101,
 102,   0,  80,  66,   0,   0,   0,   0,   0,   0,
 115, 116,   0,   0,   0,  91,   0,  95,   0,  38,
  40,  46,  48,  52,  53,   0,  87,   0,  94,   0,
   0,   0,  96,   0,  92,  88
};
static	const	short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  52,  13,   6,   0,
  50,  51,  11,   9,  49,  10,   0,  12,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  46,  47,
   7,  48,   8,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   4,   0,  53
};
static	const	short	yytok2[] =
{
   2,   3,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
  42,  43,  44,  45
};
static	const	long	yytok3[] =
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
static	const	char*	yytoknames[1];		/* for debugging */
static	const	char*	yystates[1];		/* for debugging */
#endif

/*	parser for yacc output	*/
#ifdef YYARG
#define	yynerrs		yyarg->yynerrs
#define	yyerrflag	yyarg->yyerrflag
#define yyval		yyarg->yyval
#define yylval		yyarg->yylval
#else
int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */
#endif

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

static const char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprint(x, "<%d>", yyc);
	return x;
}

static const char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
	return x;
}

static long
#ifdef YYARG
yylex1(struct Yyarg *yyarg)
#else
yylex1(void)
#endif
{
	long yychar;
	const long *t3p;
	int c;

#ifdef YYARG	
	yychar = yylex(yyarg);
#else
	yychar = yylex();
#endif
	if(yychar <= 0) {
		c = yytok1[0];
		goto out;
	}
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
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
		fprint(2, "lex %.4lux %s\n", yychar, yytokname(c));
	return c;
}

int
#ifdef YYARG
yyparse(struct Yyarg *yyarg)
#else
yyparse(void)
#endif
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	const short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;
#ifndef YYARG
	YYSTYPE save1, save2;
	int save3, save4;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;
#endif

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];
	goto yystack;

ret0:
	yyn = 0;
	goto ret;

ret1:
	yyn = 1;
	goto ret;

ret:
#ifndef YYARG
	yylval = save1;
	yyval = save2;
	yynerrs = save3;
	yyerrflag = save4;
#endif
	return yyn;

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		fprint(2, "char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) {
		yyerror("yacc stack overflow");
		goto ret1;
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
#ifdef YYARG
		yychar = yylex1(yyarg);
#else
		yychar = yylex1();
#endif
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
#ifdef YYARG
		yychar = yylex1(yyarg);
#else
		yychar = yylex1();
#endif

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
			goto ret0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 1) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yychar));
			}
			goto yyerrlab;
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
					fprint(2, "error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			goto ret1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				fprint(2, "error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				goto ret1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		fprint(2, "reduce %d in:\n\t%s", yyn, yystatname(yystate));

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
		
case 3:
#line	35	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		if(yypt[-1].yyv.sym->value != pc)
			yyerror("redeclaration of %s", yypt[-1].yyv.sym->name);
		yypt[-1].yyv.sym->value = pc;
	} break;
case 5:
#line	42	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yypt[-1].yyv.sym->type = LLAB;
		yypt[-1].yyv.sym->value = pc;
	} break;
case 10:
#line	53	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yypt[-2].yyv.sym->type = LVAR;
		yypt[-2].yyv.sym->value = yypt[-0].yyv.lval;
	} break;
case 11:
#line	58	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		if(yypt[-2].yyv.sym->value != yypt[-0].yyv.lval)
			yyerror("redeclaration of %s", yypt[-2].yyv.sym->name);
		yypt[-2].yyv.sym->value = yypt[-0].yyv.lval;
	} break;
case 12:
#line	63	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 13:
#line	64	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 14:
#line	65	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 15:
#line	66	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 16:
#line	67	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 17:
#line	68	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 18:
#line	69	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 19:
#line	70	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 20:
#line	71	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 21:
#line	72	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 22:
#line	73	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 23:
#line	74	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 24:
#line	75	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 25:
#line	76	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 26:
#line	77	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 27:
#line	78	"/Users/rminnich/src/nxm/util/6a/a.y"
{ outcode(yypt[-1].yyv.lval, &yypt[-0].yyv.gen2); } break;
case 28:
#line	81	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = nullgen;
	} break;
case 29:
#line	86	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = nullgen;
	} break;
case 30:
#line	93	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 31:
#line	100	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 32:
#line	107	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-1].yyv.gen;
		yyval.gen2.to = nullgen;
	} break;
case 33:
#line	112	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-0].yyv.gen;
		yyval.gen2.to = nullgen;
	} break;
case 34:
#line	119	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 35:
#line	124	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 36:
#line	131	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 37:
#line	136	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 38:
#line	143	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-4].yyv.gen;
		yyval.gen2.from.scale = yypt[-2].yyv.lval;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 39:
#line	151	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 40:
#line	156	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-4].yyv.gen;
		yyval.gen2.from.scale = yypt[-2].yyv.lval;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 41:
#line	164	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 42:
#line	169	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 45:
#line	180	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 46:
#line	185	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-4].yyv.gen;
		yyval.gen2.to = yypt[-2].yyv.gen;
		if(yyval.gen2.from.index != D_NONE)
			yyerror("dp shift with lhs index");
		yyval.gen2.from.index = yypt[-0].yyv.lval;
	} break;
case 47:
#line	195	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 48:
#line	200	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-4].yyv.gen;
		yyval.gen2.to = yypt[-2].yyv.gen;
		if(yyval.gen2.to.index != D_NONE)
			yyerror("dp move with lhs index");
		yyval.gen2.to.index = yypt[-0].yyv.lval;
	} break;
case 49:
#line	210	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-1].yyv.gen;
		yyval.gen2.to = nullgen;
	} break;
case 50:
#line	215	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-0].yyv.gen;
		yyval.gen2.to = nullgen;
	} break;
case 51:
#line	220	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
	} break;
case 52:
#line	227	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-4].yyv.gen;
		yyval.gen2.to = yypt[-2].yyv.gen;
		yyval.gen2.from.offset = yypt[-0].yyv.lval;
	} break;
case 53:
#line	235	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-2].yyv.gen;
		yyval.gen2.to = yypt[-0].yyv.gen;
		if(yypt[-4].yyv.gen.type != D_CONST)
			yyerror("illegal constant");
		yyval.gen2.to.offset = yypt[-4].yyv.gen.offset;
	} break;
case 54:
#line	244	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = nullgen;
		yyval.gen2.to = nullgen;
	} break;
case 55:
#line	249	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen2.from = yypt[-0].yyv.gen;
		yyval.gen2.to = nullgen;
	} break;
case 60:
#line	262	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = yypt[-0].yyv.gen;
	} break;
case 61:
#line	266	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = yypt[-0].yyv.gen;
	} break;
case 66:
#line	278	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_BRANCH;
		yyval.gen.offset = yypt[-3].yyv.lval + pc;
	} break;
case 67:
#line	284	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		if(pass == 2)
			yyerror("undefined label: %s", yypt[-1].yyv.sym->name);
		yyval.gen.type = D_BRANCH;
		yyval.gen.sym = yypt[-1].yyv.sym;
		yyval.gen.offset = yypt[-0].yyv.lval;
	} break;
case 68:
#line	293	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_BRANCH;
		yyval.gen.sym = yypt[-1].yyv.sym;
		yyval.gen.offset = yypt[-1].yyv.sym->value + yypt[-0].yyv.lval;
	} break;
case 69:
#line	302	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 70:
#line	307	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 71:
#line	312	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 72:
#line	317	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 73:
#line	322	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_SP;
	} break;
case 74:
#line	327	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 75:
#line	332	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-0].yyv.lval;
	} break;
case 76:
#line	339	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_CONST;
		yyval.gen.offset = yypt[-0].yyv.lval;
	} break;
case 77:
#line	345	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = yypt[-0].yyv.gen;
		yyval.gen.index = yypt[-0].yyv.gen.type;
		yyval.gen.type = D_ADDR;
		/*
		if($2.type == D_AUTO || $2.type == D_PARAM)
			yyerror("constant cannot be automatic: %s",
				$2.sym->name);
		 */
	} break;
case 78:
#line	356	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_SCONST;
		memcpy(yyval.gen.sval, yypt[-0].yyv.sval, sizeof(yyval.gen.sval));
	} break;
case 79:
#line	362	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_FCONST;
		yyval.gen.dval = yypt[-0].yyv.dval;
	} break;
case 80:
#line	368	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_FCONST;
		yyval.gen.dval = yypt[-1].yyv.dval;
	} break;
case 81:
#line	374	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_FCONST;
		yyval.gen.dval = -yypt[-0].yyv.dval;
	} break;
case 84:
#line	386	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+D_NONE;
		yyval.gen.offset = yypt[-0].yyv.lval;
	} break;
case 85:
#line	392	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+yypt[-1].yyv.lval;
		yyval.gen.offset = yypt[-3].yyv.lval;
	} break;
case 86:
#line	398	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+D_SP;
		yyval.gen.offset = yypt[-3].yyv.lval;
	} break;
case 87:
#line	404	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+D_NONE;
		yyval.gen.offset = yypt[-5].yyv.lval;
		yyval.gen.index = yypt[-3].yyv.lval;
		yyval.gen.scale = yypt[-1].yyv.lval;
		checkscale(yyval.gen.scale);
	} break;
case 88:
#line	413	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+yypt[-6].yyv.lval;
		yyval.gen.offset = yypt[-8].yyv.lval;
		yyval.gen.index = yypt[-3].yyv.lval;
		yyval.gen.scale = yypt[-1].yyv.lval;
		checkscale(yyval.gen.scale);
	} break;
case 89:
#line	422	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+yypt[-1].yyv.lval;
	} break;
case 90:
#line	427	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+D_SP;
	} break;
case 91:
#line	432	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+D_NONE;
		yyval.gen.index = yypt[-3].yyv.lval;
		yyval.gen.scale = yypt[-1].yyv.lval;
		checkscale(yyval.gen.scale);
	} break;
case 92:
#line	440	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_INDIR+yypt[-6].yyv.lval;
		yyval.gen.index = yypt[-3].yyv.lval;
		yyval.gen.scale = yypt[-1].yyv.lval;
		checkscale(yyval.gen.scale);
	} break;
case 93:
#line	450	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = yypt[-0].yyv.gen;
	} break;
case 94:
#line	454	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = yypt[-5].yyv.gen;
		yyval.gen.index = yypt[-3].yyv.lval;
		yyval.gen.scale = yypt[-1].yyv.lval;
		checkscale(yyval.gen.scale);
	} break;
case 95:
#line	463	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = yypt[-1].yyv.lval;
		yyval.gen.sym = yypt[-4].yyv.sym;
		yyval.gen.offset = yypt[-3].yyv.lval;
	} break;
case 96:
#line	470	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.gen = nullgen;
		yyval.gen.type = D_STATIC;
		yyval.gen.sym = yypt[-6].yyv.sym;
		yyval.gen.offset = yypt[-3].yyv.lval;
	} break;
case 97:
#line	478	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = 0;
	} break;
case 98:
#line	482	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-0].yyv.lval;
	} break;
case 99:
#line	486	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = -yypt[-0].yyv.lval;
	} break;
case 101:
#line	493	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = D_AUTO;
	} break;
case 104:
#line	501	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-0].yyv.sym->value;
	} break;
case 105:
#line	505	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = -yypt[-0].yyv.lval;
	} break;
case 106:
#line	509	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-0].yyv.lval;
	} break;
case 107:
#line	513	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = ~yypt[-0].yyv.lval;
	} break;
case 108:
#line	517	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-1].yyv.lval;
	} break;
case 110:
#line	524	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval + yypt[-0].yyv.lval;
	} break;
case 111:
#line	528	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval - yypt[-0].yyv.lval;
	} break;
case 112:
#line	532	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval * yypt[-0].yyv.lval;
	} break;
case 113:
#line	536	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval / yypt[-0].yyv.lval;
	} break;
case 114:
#line	540	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval % yypt[-0].yyv.lval;
	} break;
case 115:
#line	544	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-3].yyv.lval << yypt[-0].yyv.lval;
	} break;
case 116:
#line	548	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-3].yyv.lval >> yypt[-0].yyv.lval;
	} break;
case 117:
#line	552	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval & yypt[-0].yyv.lval;
	} break;
case 118:
#line	556	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval ^ yypt[-0].yyv.lval;
	} break;
case 119:
#line	560	"/Users/rminnich/src/nxm/util/6a/a.y"
{
		yyval.lval = yypt[-2].yyv.lval | yypt[-0].yyv.lval;
	} break;
	}
	goto yystack;  /* stack new state and value */
}
