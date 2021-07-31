#define	_ID	57346
#define	_STRING	57347
#define	_PART	57348
#define	_HEX	57349
#define	_OCTAL	57350
#define	_DECIMAL	57351
#define	_BINARY	57352
#define	_ARRAY	57353
#define	_INPUTS	57354
#define	_OUTPUTS	57355
#define	_INTERNAL	57356
#define	_EXTERNAL	57357
#define	_DECLARE	57358
#define	_PACKAGE	57359
#define	_COMPLEMENTPLUS	57360
#define	_COMPLEMENTMINUS	57361
#define	_PINS	57362
#define	_FUSES	57363
#define	_OFFSET	57364
#define	_NOT	57365
#define	_SET	57366
#define	_RESET	57367
#define	_CLOCK	57368
#define	_MACROCELL	57369
#define	_ENABLED	57370
#define	_ENABLES	57371
#define	_CLOCKED	57372
#define	_INVERTED	57373
#define	_ENABLE	57374
#define	_INVERT	57375
#define	_INPUT	57376
#define	_OUTPUT	57377
#define	_VCC	57378
#define	_GND	57379

#line	8	"paddle.y"

#include "part.h"
#include <stdio.h>

#define YYMAXDEPTH 5000

#define YYSTYPE yystype
typedef union yystype {
	int integer;
	float real;
	char *string;
	struct PIN pin;
} YYSTYPE;

int array_count = 0;
int input_count = 0;
int output_count = 0;
int cell_count = 0;
int max_pin;
int last_pin;
int match = -1;
int pintypes;
int side = 0;
int line;
int index;
int macroindex = 0;

struct CELL macrocells[MAXCELLS];
struct ARRAY arrays[MAXARRAY];

extern char yytext[];
extern char *extern_id;
extern char package[];
extern char cbuf[];
extern struct PIN pin_array[MAXPIN];
extern int max_fuse;
extern int found;

int lookup_array(char *ref, struct ARRAY array[], int count);
int lookup_cell(char *ref, struct CELL array[], int count);
int setpin(struct PIN pin);

#ifndef PLAN9
extern char *strdup();
extern int strcmp();
#endif

extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
#ifndef	YYSTYPE
#define	YYSTYPE	int
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define	UMINUS	57380
#define YYEOFCODE 1
#define YYERRCODE 2
short	yyexca[] =
{-1, 0,
	1, 118,
	-2, 3,
-1, 1,
	1, -1,
	-2, 0,
-1, 2,
	1, 118,
	-2, 3,
-1, 19,
	46, 13,
	-2, 101,
-1, 64,
	46, 118,
	-2, 48,
-1, 104,
	46, 118,
	-2, 48,
};
#define	YYNPROD	119
#define	YYPRIVATE 57344
#define	yydebug	1
#define	YYLAST	254
short	yyact[] =
{
   3, 181, 173, 171, 154, 178, 168, 155,  13, 119,
 151,  22,  35,  79, 115,  87,  19,  13,  82,  19,
  29,  85,  59,  29, 175, 183, 150, 149,  39,  40,
  38,  39,  40,  38, 182,  39,  40,  38,  57,  58,
 183, 148, 147,  54, 146,  28,  48,  49,  50,  51,
  52, 145, 187,  67,  68,  69,  70,  71, 188, 177,
  33,  75, 166,  33, 143,  81,  72,  33, 176, 172,
 114,  29,  36,  34, 156,  36,  34, 165,  83,  36,
  34, 111, 142,  77, 126, 117,  48,  49,  50,  51,
  52,  48,  49,  50,  51,  52, 113,  53, 103, 167,
  76,  42, 129,  84, 130,  81, 135,  78, 127,  48,
  49,  50,  51,  52,  66,  64,  52,  44, 132,  29,
  15,  50,  51,  52,  12, 140,  83,  55, 144,  56,
 153, 139, 152, 141, 136, 137,   7,  17, 118, 124,
 123, 121,  11, 131, 117, 122, 157, 120, 125,  16,
 159, 160, 161, 162, 163, 164,  43, 170, 158,  41,
  62,  63, 124, 123, 121,  74,  46, 153, 122, 152,
 120, 125,   1, 180, 184,   5, 179, 174, 170, 116,
 185,  39,  40,  38, 186,  73,   9,  31,   8, 180,
 184, 192, 179, 108, 191, 189, 190,  39,  40,  38,
 169,  27,  65, 112,  47, 134,  26,  32, 110, 138,
  30,  28, 109, 106, 133, 105, 104,  80,  45, 107,
  37,  99, 100,  88, 128,  86,  61,  60,  25,  33,
  24,  23,  21,  89,  90,  91,  92,  93,  94,  95,
  96,  36,  34,  97,  98, 101, 102,  20,  18,  14,
  10,   6,   4,   2
};
short	yypact[] =
{
-1000,-1000,-1000,-1000, 182,-1000,-1000,  80,-1000,-1000,
  75,-1000, 182,-1000,-1000, 190,  80,  55, 190,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  72, 162,-1000,  53,
 174, 100, 124,  28,  28,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 146,  70,-1000,  69,  28,  28,
  28,  28,  28, 174,-1000, 161,-1000,-1000,   8,  54,
 146,  62,-1000,-1000,-1000,-1000,  24,  81,  81,  74,
  74,-1000,-1000,  58,-1000,-1000,-1000,-1000, 209,  52,
 181,-1000,  50,  23, 115,  38, 209,  57, 209,-1000,
-1000,-1000, 139,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, 116, 118,-1000,-1000,-1000,
-1000,-1000, 174,-1000,  24,  36,  17,-1000, 138,-1000,
   1,  -6,  -8,  -9, -23, -24,-1000,-1000,-1000,  28,
-1000,-1000,-1000,  29,-1000,-1000,-1000,-1000,  29,-1000,
-1000,-1000,-1000, 115,-1000,  28,  28,  28,  28,  28,
  28,  31,  15,-1000,  48,-1000,  21,-1000,-1000,  71,
  71,  71,  71,  71,  71,-1000,  28, -27,  22,  12,
-1000,-1000,  28, -10,-1000,  28,-1000,  21,   3,  11,
-1000,-1000, 174, 174,-1000,  71,-1000,-1000,  28, -25,
-1000,-1000,-1000
};
short	yypgo[] =
{
   0, 172, 253,   0, 252, 251, 250, 249, 136, 142,
 137, 248, 247, 232,  11, 231, 230, 228,  22, 227,
 226,  21, 225,  15, 224, 223, 219,  12, 218,  13,
 217, 216, 215, 214,   7, 213, 209, 205, 204, 202,
  10,   2,   6, 200,   3,   5,   1,   4,  18, 187,
 185,  14, 179,   9
};
short	yyr1[] =
{
   0,   1,   1,   4,   6,   2,   5,   9,   9,   8,
   8,   7,  10,  10,  11,  11,  11,  11,  11,  11,
  12,  18,  18,  19,  21,  21,  22,  23,  23,  25,
  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,
  25,  25,  25,  20,  20,  26,  13,  28,  30,  29,
  29,  31,  31,  31,  32,  36,  35,  35,  35,  33,
  33,  37,  37,  38,  14,  15,  24,  40,  40,  40,
  34,  42,  42,  42,  43,  43,  45,  45,  45,  44,
  44,  46,  46,  41,  41,  47,  47,  47,  47,  47,
  47,  47,  47,  47,  39,  48,  48,  14,  14,  16,
  49,  49,  51,  51,  51,  52,  52,  53,  53,  53,
  53,  53,  53,  50,  17,  27,  27,  27,   3
};
short	yyr2[] =
{
   0,   2,   1,   0,   0,   4,   2,   3,   1,   1,
   1,   3,   2,   1,   1,   1,   1,   1,   1,   1,
   4,   2,   1,   4,   2,   1,   2,   2,   1,   1,
   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   5,   1,   0,   3,
   1,   3,   3,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   0,   3,   2,   3,   3,   1,   1,
   3,   3,   1,   1,   1,   3,   3,   1,   1,   2,
   4,   2,   1,   1,   4,   3,   3,   3,   3,   3,
   2,   3,   1,   1,   3,   3,   1,   3,   1,   6,
   1,   1,   3,   1,   1,   2,   1,   3,   3,   3,
   3,   3,   3,   1,   2,   1,   1,   1,   0
};
short	yychk[] =
{
-1000,  -1,  -2,  -3,  -4,  -1,  -5,  -8,   6,   4,
  -6,  -9,  44,  -3,  -7,  45,  -8, -10, -11,  -3,
 -12, -13, -14, -15, -16, -17,  16,  11,  21, -47,
  20, -49,  17,  39,  52, -27,  51,  30,   9,   7,
   8,  -9,  46, -10,  45, -28,   4, -38,  38,  39,
  40,  41,  42,  44, -27,  27,   5, -47, -47, -18,
 -19, -20,  14,  15,  45, -39,  45, -47, -47, -47,
 -47, -47, -27, -50,   4,  53,  46, -18,  45, -29,
 -30,  -3, -48, -14,  45, -21, -22, -23, -25,  24,
  25,  26,  27,  28,  29,  30,  31,  34,  35,  12,
  13,  36,  37,  46, -31, -32, -35, -26,  12,  31,
  27,  -3,  22,  46,  47, -51, -52,  -3,  23, -53,
  32,  26,  30,  25,  24,  33,  46, -21, -24,  45,
 -23,   4, -29, -33, -37,  -3,  18,  19, -36,  13,
 -27, -48,  46,  47, -53,  50,  50,  50,  50,  50,
  50, -40, -41,  -3, -47, -34,  45, -34, -51, -47,
 -47, -47, -47, -47, -47,  46,  47,  51, -42, -43,
  -3, -44,  48, -41, -40,  51,  46,  47, -45, -44,
  -3, -46,  44,  50,  -3, -47, -42,  49,  47, -27,
 -27, -45, -46
};
short	yydef[] =
{
  -2,  -2,  -2,   2,   0,   1,   4, 118,   9,  10,
   0,   6,   0,   8,   5, 118, 118,   0, 118,  -2,
  14,  15,  16,  17,  18,  19,   0,   0,  63,  98,
   0,   0,   0,   0,   0,  92,  93, 100, 115, 116,
 117,   7,  11,  12,   0,   0,  47,   0,   0,   0,
   0,   0,   0,   0,  65,   0, 114,  90,   0,   0,
  22,   0,  43,  44,  -2,  64,   0,  85,  86,  87,
  88,  89,  97,   0, 113,  91,  20,  21,   0,   0,
 118,  50,   0,  96, 118,   0,  25,   0,  28,  29,
  30,  31,   0,  33,  34,  35,  36,  37,  38,  39,
  40,  41,  42,  46,  -2, 118,   0,  53,  54,  56,
  57,  58,   0,  94,   0,   0, 103, 104,   0, 106,
   0,   0,   0,   0,   0,   0,  23,  24,  26, 118,
  27,  32,  49,   0,  59,  60,  61,  62,   0,  55,
  45,  95,  99, 118, 105,   0,   0,   0,   0,   0,
   0,   0,  68,  69,  83,  51, 118,  52, 102, 107,
 108, 109, 110, 111, 112,  66, 118,   0,   0,  72,
  73,  74, 118, 118,  67,   0,  70, 118,   0,  77,
  78,  79,   0,   0,  82,  84,  71,  75, 118, 118,
  81,  76,  80
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  41,   0,
  52,  53,   0,  38,  47,  39,  51,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  50,   0,
   0,  44,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  48,   0,  49,  42,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  45,  40,  46
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  43
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
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
	sprint(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
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
		fprint(2, "lex %.4lux %s\n", yychar, yytokname(c));
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
		fprint(2, "char %s in %s", yytokname(yychar), yystatname(yystate));

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
			yynerrs++;
			if(yydebug >= 1) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yychar));
			}

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
			return 1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				fprint(2, "error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				return 1;
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
#line	66	"paddle.y"
{ match = -1; array_count = 0; cell_count = 0; } break;
case 4:
#line	66	"paddle.y"
{ max_pin = MAXPIN; } break;
case 6:
#line	69	"paddle.y"
{ match = yypt[-1].yyv.integer && yypt[-0].yyv.integer; } break;
case 7:
#line	72	"paddle.y"
{ yyval.integer = yypt[-1].yyv.integer && yypt[-0].yyv.integer; } break;
case 8:
#line	74	"paddle.y"
{ yyval.integer = TRUE; } break;
case 9:
#line	77	"paddle.y"
{ yyval.integer = strcmp(yytext, extern_id); } break;
case 10:
#line	79	"paddle.y"
{ yyval.integer = strcmp(yytext, extern_id); } break;
case 26:
#line	104	"paddle.y"
{ if (!match) {
				declare_pin_stack(yypt[-1].yyv.integer | side);
				if (macroindex > 0) declare_macropins(macroindex);
				clear_pin_stack();
			  }
			  macroindex = 0;
			} break;
case 27:
#line	113	"paddle.y"
{ yyval.integer = yypt[-1].yyv.integer | yypt[-0].yyv.integer; } break;
case 28:
#line	115	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 29:
#line	118	"paddle.y"
{ yyval.integer = SET_TYPE; } break;
case 30:
#line	120	"paddle.y"
{ yyval.integer = OUTPUT_TYPE | RESET_TYPE; } break;
case 31:
#line	122	"paddle.y"
{ yyval.integer = OUTPUT_TYPE | CLOCK_TYPE; } break;
case 32:
#line	124	"paddle.y"
{ if (macroindex > 0)
				fprintf(stderr, "macrocell %s already selected\n", macrocells[macroindex].name);
			  else macroindex = lookup_macrocell(yytext);
			  yyval.integer = (macroindex > 0) ? (macrocells[macroindex].type & ~(RESET_TYPE|SET_TYPE)) : 0; } break;
case 33:
#line	129	"paddle.y"
{ yyval.integer = ENABLED_TYPE; } break;
case 34:
#line	131	"paddle.y"
{ yyval.integer = ENABLE_TYPE; } break;
case 35:
#line	133	"paddle.y"
{ yyval.integer = CLOCKED_TYPE; } break;
case 36:
#line	135	"paddle.y"
{ yyval.integer = INVERTED_TYPE; } break;
case 37:
#line	137	"paddle.y"
{ yyval.integer = INPUT_TYPE | COMB_TYPE; } break;
case 38:
#line	139	"paddle.y"
{ yyval.integer = OUTPUT_TYPE | NONINVERTED_TYPE | INVERTED_TYPE; } break;
case 39:
#line	141	"paddle.y"
{ yyval.integer = INPUT_TYPE | COMB_TYPE; } break;
case 40:
#line	143	"paddle.y"
{ yyval.integer = OUTPUT_TYPE | NONINVERTED_TYPE; } break;
case 41:
#line	145	"paddle.y"
{ yyval.integer = VCC_TYPE; } break;
case 42:
#line	147	"paddle.y"
{ yyval.integer = GND_TYPE; } break;
case 43:
#line	150	"paddle.y"
{ side = INTERNAL_TYPE; } break;
case 44:
#line	152	"paddle.y"
{ side = EXTERNAL_TYPE; } break;
case 45:
#line	155	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 46:
#line	157	"paddle.y"
{ if (!match) {
			  last_pin = input_count*output_count+arrays[array_count].offset;
			  array_count++; found++;
			  }
			} break;
case 47:
#line	164	"paddle.y"
{  if (!match) {
			if (lookup_array(yytext, arrays, array_count))
				yyerror("array name %s already declared\n", yytext);
			arrays[array_count].offset = 0;
			if (array_count < MAXARRAY) {
				arrays[array_count].name = strdup(yytext);
			        input_count = output_count = 0;
			}
			else
				fprintf(stderr, "too many arrays (MAXARRAY)\n");
		}
	    } break;
case 48:
#line	177	"paddle.y"
{ pintypes = 0; } break;
case 51:
#line	181	"paddle.y"
{ if (!match) arrays[array_count].max_inputs = input_count;
			  yyval.integer = yypt[-1].yyv.integer;
			} break;
case 52:
#line	185	"paddle.y"
{ if (!match) arrays[array_count].max_outputs = output_count;
			  yyval.integer = yypt[-1].yyv.integer;
			} break;
case 53:
#line	189	"paddle.y"
{ if (!match) {arrays[array_count].offset = yypt[-0].yyv.integer;  }
			} break;
case 54:
#line	193	"paddle.y"
{ pintypes |= INPUT_TYPE; line = 0; } break;
case 55:
#line	196	"paddle.y"
{ pintypes |= OUTPUT_TYPE; line = 0; } break;
case 56:
#line	199	"paddle.y"
{ pintypes = INVERTED_TYPE; } break;
case 57:
#line	201	"paddle.y"
{ pintypes = INVERTED_TYPE | NONINVERTED_TYPE; } break;
case 58:
#line	203	"paddle.y"
{ pintypes = NONINVERTED_TYPE; } break;
case 59:
#line	206	"paddle.y"
{ pintypes |= DOUBLE_FLAG; } break;
case 60:
#line	208	"paddle.y"
{ pintypes &= ~DOUBLE_FLAG; } break;
case 62:
#line	211	"paddle.y"
{ pintypes |= COMPLEMENT_FIRST; } break;
case 63:
#line	213	"paddle.y"
{ line = 0; } break;
case 65:
#line	216	"paddle.y"
{ max_pin = yypt[-0].yyv.integer; } break;
case 66:
#line	219	"paddle.y"
{ yyval.integer = match ? 0 : yypt[-1].yyv.integer; } break;
case 70:
#line	226	"paddle.y"
{ yyval.integer = match ? 0 : yypt[-1].yyv.integer; } break;
case 74:
#line	233	"paddle.y"
{ line += setpin(yypt[-0].yyv.pin); } break;
case 75:
#line	235	"paddle.y"
{ line++; } break;
case 76:
#line	238	"paddle.y"
{ (void) setpin(yypt[-2].yyv.pin); } break;
case 77:
#line	240	"paddle.y"
{ (void) setpin(yypt[-0].yyv.pin); } break;
case 79:
#line	244	"paddle.y"
{ 
			  if (pintypes & INPUT_TYPE)
			  	yyval.pin.input_line = line;
			  else
			  if (pintypes & OUTPUT_TYPE) {
			  	yyval.pin.output_line = line;
			  	yyval.pin.max_term = yypt[-0].yyv.integer;
			  }
			  yyval.pin.index = array_count;
			  yyval.pin.first = yypt[-1].yyv.integer;
			  yyval.pin.flags = pintypes;
			} break;
case 80:
#line	257	"paddle.y"
{
			  line = yypt[-1].yyv.integer;
			  if (pintypes & INPUT_TYPE)
			  	yyval.pin.input_line = line;
			  else
			  if (pintypes & OUTPUT_TYPE)
			  	yyval.pin.output_line = line;
			  yyval.pin.index = array_count;
			  yyval.pin.first = yypt[-3].yyv.integer;
			  yyval.pin.max_term = yypt[-0].yyv.integer;
			  yyval.pin.flags = pintypes;
			} break;
case 81:
#line	271	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 82:
#line	273	"paddle.y"
{ yyval.integer = 1; } break;
case 83:
#line	276	"paddle.y"
{ if (yypt[-0].yyv.integer > MAXPIN) yyerror("pin out of range");
			  if (!match) push_pins(yypt[-0].yyv.integer, 0);
			} break;
case 84:
#line	280	"paddle.y"
{ if (yypt[-3].yyv.integer > MAXPIN) yyerror("first pin out of range");
			  else
			  if (yypt[-0].yyv.integer > MAXPIN) yyerror("second pin out of range");
			  else
			  if (!match) push_pins(yypt[-3].yyv.integer, yypt[-0].yyv.integer);
			} break;
case 85:
#line	288	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer + yypt[-0].yyv.integer; } break;
case 86:
#line	290	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer - yypt[-0].yyv.integer; } break;
case 87:
#line	292	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer | yypt[-0].yyv.integer; } break;
case 88:
#line	294	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer & yypt[-0].yyv.integer; } break;
case 89:
#line	296	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer ^ yypt[-0].yyv.integer; } break;
case 90:
#line	298	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 91:
#line	300	"paddle.y"
{ yyval.integer = yypt[-1].yyv.integer; } break;
case 92:
#line	302	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 93:
#line	304	"paddle.y"
{ yyval.integer = last_pin; } break;
case 95:
#line	309	"paddle.y"
{ (void) setpin(yypt[-2].yyv.pin); } break;
case 96:
#line	311	"paddle.y"
{ (void) setpin(yypt[-0].yyv.pin); } break;
case 97:
#line	314	"paddle.y"
{ if (yypt[-2].yyv.integer > MAXPIN) yyerror("fuse pin out of range");
			  yyval.pin.index = -1;
			  yyval.pin.first = yypt[-2].yyv.integer;
			  yyval.pin.max_term = 1;
			  yyval.pin.flags = FUSE_FLAG;
			  line = yypt[-0].yyv.integer;
			  yyval.pin.output_line = line;
			  if (array_count && (line > max_fuse)) max_fuse = line;
			  line++;
			} break;
case 98:
#line	325	"paddle.y"
{
			  yyval.pin.index = -1;
			  yyval.pin.first = yypt[-0].yyv.integer;
			  yyval.pin.max_term = 1;
			  yyval.pin.flags = FUSE_FLAG;
			  yyval.pin.output_line = line;
			  if (array_count && (line > max_fuse)) max_fuse = line;
			  line++;
			} break;
case 99:
#line	336	"paddle.y"
{ 
				macrocells[cell_count].name = yypt[-3].yyv.string;
			  if (!match) {
				macrocells[cell_count].type |= INPUT_TYPE|OUTPUT_TYPE|COMB_TYPE|NONINVERTED_TYPE | yypt[-5].yyv.integer;
				macrocells[cell_count].mask = yypt[-1].yyv.integer;
				macrocells[cell_count].mask |= (yypt[-1].yyv.integer & INVERTED_TYPE) ? 0 : NONINVERTED_TYPE;
				macrocells[cell_count].mask |= (yypt[-1].yyv.integer & CLOCKED_TYPE) ? 0 : COMB_TYPE;
			  }
			} break;
case 100:
#line	347	"paddle.y"
{ yyval.integer = CLOCKED_TYPE; } break;
case 101:
#line	349	"paddle.y"
{ yyval.integer = 0; } break;
case 102:
#line	351	"paddle.y"
{ yyval.integer = yypt[-2].yyv.integer | yypt[-0].yyv.integer; } break;
case 103:
#line	353	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 104:
#line	355	"paddle.y"
{ yyval.integer = 0; } break;
case 105:
#line	358	"paddle.y"
{ yyval.integer = 0; } break;
case 106:
#line	360	"paddle.y"
{ yyval.integer = yypt[-0].yyv.integer; } break;
case 107:
#line	363	"paddle.y"
{ if (!match) {
				macrocells[cell_count].enable = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= ENABLED_TYPE;
				yyval.integer = ENABLED_TYPE;
			  }
			} break;
case 108:
#line	370	"paddle.y"
{ if (!match) {
				macrocells[cell_count].clock = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= CLOCK_TYPE;
				yyval.integer = CLOCK_TYPE;
			  }
			} break;
case 109:
#line	377	"paddle.y"
{ if (!match) {
				macrocells[cell_count].clocked = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= CLOCKED_TYPE;
				yyval.integer = CLOCKED_TYPE;
			  }
			} break;
case 110:
#line	384	"paddle.y"
{ if (!match) {
				macrocells[cell_count].reset = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= RESET_TYPE;
				yyval.integer = RESET_TYPE;
			  }
			} break;
case 111:
#line	391	"paddle.y"
{ if (!match) {
				macrocells[cell_count].set = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= SET_TYPE;
				yyval.integer = SET_TYPE;
			  }
			} break;
case 112:
#line	398	"paddle.y"
{ if (!match) {
				macrocells[cell_count].invert = yypt[-0].yyv.integer;
				macrocells[cell_count].type |= INVERTED_TYPE;
				yyval.integer = INVERTED_TYPE;
			  }
			} break;
case 113:
#line	406	"paddle.y"
{ yyval.string = strdup(yytext); cell_count++; } break;
case 114:
#line	408	"paddle.y"
{ if (!match) strcpy(package, cbuf); } break;
case 115:
#line	411	"paddle.y"
{ sscanf(yytext, "%d", &yyval.integer); } break;
case 116:
#line	413	"paddle.y"
{ sscanf(yytext, "%x", &yyval.integer); } break;
case 117:
#line	415	"paddle.y"
{ sscanf(yytext, "%o", &yyval.integer); } break;
	}
	goto yystack;  /* stack new state and value */
}
