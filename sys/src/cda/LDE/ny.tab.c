
#line	2	"nlde.y"
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"

#line	8	"nlde.y"
typedef union 
{
	int	code;
	Value	val;
	Hshtab	*hp;
	Node	*node;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define	INPUTS	57346
#define	OUTPUTS	57347
#define	EXTERNS	57348
#define	FIELDS	57349
#define	FUNCS	57350
#define	INPUT	57351
#define	OUTPUT	57352
#define	EQNS	57353
#define	EQN	57354
#define	TYPES	57355
#define	CEQN	57356
#define	BOTH	57357
#define	BOTHS	57358
#define	BURIEDS	57359
#define	BURIED	57360
#define	ITER	57361
#define	YUSED	57362
#define	NAME	57363
#define	EXTERN	57364
#define	FIELD	57365
#define	FNCS	57366
#define	DCOUTPUT	57367
#define	DCFIELD	57368
#define	AEQN	57369
#define	AOUTPUT	57370
#define	CNDEQN	57371
#define	XNOR	57372
#define	GOTO	57373
#define	END	57374
#define	POP2	57375
#define	NOCASE	57376
#define	DNOCASE	57377
#define	DSWITCH	57378
#define	EQX	57379
#define	NEX	57380
#define	IF	57381
#define	SELECT	57382
#define	CASE	57383
#define	SWITCH	57384
#define	WHILE	57385
#define	CND	57386
#define	ALT	57387
#define	PERM	57388
#define	NOT	57389
#define	COMEQU	57390
#define	ALTEQU	57391
#define	SUBEQU	57392
#define	ADDEQU	57393
#define	MULEQU	57394
#define	XOREQU	57395
#define	MODEQU	57396
#define	PNDEQU	57397
#define	DC	57398
#define	COM	57399
#define	NEG	57400
#define	GREY	57401
#define	FLONE	57402
#define	FRONE	57403
#define	ADD	57404
#define	AND	57405
#define	DIV	57406
#define	EQ	57407
#define	GE	57408
#define	GT	57409
#define	LAND	57410
#define	LE	57411
#define	LOR	57412
#define	LS	57413
#define	LT	57414
#define	MOD	57415
#define	MUL	57416
#define	NE	57417
#define	OR	57418
#define	RS	57419
#define	SUB	57420
#define	XOR	57421
#define	PND	57422
#define	ELIST	57423
#define	ASSIGN	57424
#define	DONTCARE	57425
#define	NUMBER	57426
#define YYERRCODE 57344

#line	426	"nlde.y"


static void
yymsg(int exitflag, char *s, void *args)
{
	extern char yytext[];
	extern yylineno;
	char buf[1024], *out;

	errcount++;
	fprintf(stderr, "%s:%d ", yyfile, yylineno);
	if(yytext[0])
		fprintf(stderr, "sym \"%s\": ", yytext);
	if(s==0 || *s==0)
		fprintf(stderr, "syntax error\n");
	else{
#ifdef PLAN9
		out = doprint(buf, buf+sizeof buf, s, args);
		*out++ = '\n';
		fwrite(buf, 1, out-buf, stderr);
#else
		fprintf(stderr, s);
#endif
		*out++ = '\n';
		fwrite(buf, 1, out-buf, stderr);
	}
	fflush(stderr);
	if(exitflag)
		exits("error");;
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
			fprintf(stderr, "too many inputs\n");
			exits("error");;
		}
		leaves[nleaves] = h;
		break;

	case OUTPUT:
		h->oref = ++noutputs;
		if(noutputs >= NLEAVES) {
			fprintf(stderr, "too many outputs\n");
			exits("error");;
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
			fprintf(stderr, "too many inputs\n");
			exits("error");;
		}
		leaves[nleaves] = h;

		h->oref = ++noutputs;
		if(noutputs >= NLEAVES) {
			fprintf(stderr, "too many outputs\n");
			exits("error");;
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
			fprintf(stderr, "too many fields\n");
			exits("error");;
		}
	}
	h->code = FIELD;
	SETLO(h, nextfield-n);
	SETHI(h, nextfield);
/* 	nextfield += n; */
}
short	yyexca[] =
{-1, 0,
	1, 1,
	-2, 2,
-1, 1,
	1, -1,
	-2, 0,
-1, 3,
	1, 1,
	-2, 2,
-1, 96,
	48, 60,
	49, 60,
	50, 60,
	51, 60,
	52, 60,
	53, 60,
	54, 60,
	55, 60,
	56, 60,
	57, 60,
	-2, 81,
-1, 106,
	48, 61,
	49, 61,
	50, 61,
	51, 61,
	52, 61,
	53, 61,
	54, 61,
	55, 61,
	56, 61,
	57, 61,
	-2, 58,
-1, 107,
	48, 62,
	49, 62,
	50, 62,
	51, 62,
	52, 62,
	53, 62,
	54, 62,
	55, 62,
	56, 62,
	57, 62,
	-2, 59,
};
#define	YYNPROD	123
#define	YYPRIVATE 57344
#define	YYLAST	472
short	yyact[] =
{
  91, 117, 163,  55, 118,  89, 169, 170,  67, 115,
  51,  64, 175,  43, 153,  86,  85,  84, 151,  54,
 136, 122, 139, 130, 133, 135, 120, 132, 119, 141,
 134, 140, 138, 131, 128, 142, 137, 129, 117, 179,
 118, 118, 168, 127,  65,  83,  48, 166, 109,  69,
  66,  53, 173,  50, 118,  62,  40, 136, 122, 139,
 130, 133, 135, 120, 132, 119, 141, 134, 140, 138,
 131, 128, 142, 137, 129,  71,  90,  44,  39, 141,
 127,  88,  38,  26, 110, 142,  74,  75,  78,  77,
  76,  80,  79,  81,  82,  73,  68,  70, 116, 143,
 144, 145, 146, 147, 148, 149,   1,  36,  87,  49,
  24, 117, 171,  90, 118, 150,  41,  35, 152,  42,
 154, 155, 156, 157, 158, 159, 160, 161, 164,  34,
 136, 122, 139, 130, 133, 135, 120, 132, 119, 141,
 134, 140, 138, 131, 128, 142, 137, 129,   4,  45,
  46,  47,  33, 127,  27,  32,  58,  31,  30,  63,
  29,  59, 117, 167,  61, 118, 172,  57, 174,  60,
  28, 164, 177, 176, 178,  14,  13,  12,  11,  10,
 180, 136, 122, 139, 130, 133, 135, 120, 132, 119,
 141, 134, 140, 138, 131, 128, 142, 137, 129, 117,
   9,   8, 118,   7, 127,   6,  37,   5,   3,   2,
  52,  56,  93,  25, 162, 126, 125, 124, 136, 122,
 139, 130, 133, 135, 120, 132, 119, 141, 134, 140,
 138, 131, 128, 142, 137, 129, 105, 123, 121,  72,
   0, 127,   0, 108, 112,   0,  94,   0,  95, 113,
   0,   0,   0,   0, 105, 111,   0, 114,   0,   0,
   0, 108, 112,   0,  94,   0,  95, 113,   0,   0,
   0,   0,   0, 111,   0, 114, 105,   0, 118, 165,
   0,  99,   0, 108,  96,   0,  94,   0,  95, 106,
   0,   0, 100,   0,   0,  92, 139, 107,   0,  99,
   0,   0, 102, 141,   0, 140, 138, 101, 103, 142,
 100,   0,   0,  98,   0,   0,   0,   0,   0,  97,
 102,  99,   0, 104,   0, 101, 103,   0,   0,   0,
   0,  98, 100,   0,   0,   0,   0,  97,   0,   0,
   0, 104, 102,   0,   0,   0,   0, 101, 103, 118,
   0,   0,   0,  98,   0,   0,   0,   0,   0,  97,
   0,   0,   0, 104,   0, 136, 122, 139, 130, 133,
 135, 120, 132, 118, 141, 134, 140, 138, 131, 128,
 142, 137, 129,   0,   0,   0,   0,   0,   0, 136,
 122, 139, 130, 133, 135, 118, 132,   0, 141, 134,
 140, 138, 131, 128, 142, 137, 129,   0,   0,   0,
   0, 136, 122, 139, 130, 133, 135, 118, 132,   0,
 141, 134, 140, 138, 131,   0, 142, 137,   0,   0,
   0,   0,   0, 136, 118, 139, 130, 133, 135,   0,
 132,   0, 141, 134, 140, 138, 131,   0, 142, 137,
 136,   0, 139,   0,   0,  18,  21,  15,  22, 141,
   0, 140, 138,   0,  16, 142, 137,  19,  20,   0,
  17,  23
};
short	yypact[] =
{
-1000,-1000, 451,-1000,  72, 451,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  61,  57,
  35,  -8,  -8,  -8,  -8,  32,  30, 146,  34,-1000,
-1000,-1000,  -8, -37,-1000,-1000,-1000,-1000,-1000,  32,
   1,-1000,-1000,-1000,-1000, 146,  38,  -3,-1000,-1000,
-1000,-1000,-1000,-1000, -68,-1000, -69, -70,-1000,-1000,
  30,-1000, 274,   0,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000, 252,-1000, -79,-1000,  30,-1000,-1000,
-1000, 155,  -3,-1000,-1000,-1000,-1000,-1000, 252, 252,
 252, 252, 252, 252, 252,-1000,-1000,-1000,-1000, 274,
 155,-1000,-1000,-1000,-1000, -61,-1000, 252, -71, 252,
 252, 252, 252, 252, 252, 252, 252, 234,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000, 231,  -7,  -7, 387, 387,   7, -43,
-1000,-1000, 118,  -5, 302, 326, 348, 370, 387, 231,
   7,  -7, -85,-1000,  67, 252,   8, 252, -73,-1000,
 234, 252, 155, 252, 155,-1000,-1000, 155,  -6, 252,
 155
};
short	yypgo[] =
{
   0, 239, 238, 237, 217, 216, 215,   0,   3,   5,
 214,   2, 213,  19, 212, 211, 210, 106, 209, 208,
 148, 207, 206, 205, 203, 201, 200, 179, 178, 177,
 176, 175, 170, 164, 160, 158, 157, 116, 155, 152,
 129, 119, 117,  46, 109, 108,  10, 107,  97
};
short	yyr1[] =
{
   0,  17,  18,  17,  19,  20,  20,  22,  12,  21,
  21,  21,  21,  21,  21,  21,  21,  21,  32,  23,
  34,  24,  35,  25,  33,  33,  36,  26,  38,  27,
  39,  28,  40,  29,  37,  37,  41,  41,  41,  41,
  41,  41,  41,  42,  30,  43,  43,  45,  44,  47,
  31,  46,  48,  46,  16,  13,  13,  14,  14,  14,
  15,  15,  15,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   8,   8,   8,   9,   9,   7,   7,   7,
   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
   7,   7,   7,   2,   2,   3,   3,   3,   3,   3,
   3,   4,   4,   5,   5,   5,   6,   6,  10,  10,
  11,  11,  11
};
short	yyr2[] =
{
   0,   0,   0,   3,   2,   0,   2,   0,   3,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   0,   4,
   0,   3,   0,   3,   0,   1,   0,   3,   0,   3,
   0,   3,   0,   3,   0,   2,   3,   2,   1,   4,
   5,   1,   3,   0,   3,   0,   2,   0,   4,   0,
   3,   1,   0,   3,   1,   0,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   3,   4,   3,   1,   1,   1,   1,   1,
   1,   1,   1,   2,   2,   2,   2,   2,   2,   5,
   7,   5,   3,   3,   3,   3,   3,   3,   3,   3,
   4,   3,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   3,
   1,   2,   3
};
short	yychk[] =
{
-1000, -17, -18, -19, -20, -21, -23, -24, -25, -26,
 -27, -28, -29, -30, -31,   6,  13,  19,   4,  16,
  17,   5,   7,  20, -17, -12,  11, -20, -32, -34,
 -35, -36, -38, -39, -40, -42, -47, -22,  21,  21,
  21, -37, -41,  21,  85, -37, -37, -37, -43, -44,
  21, -46, -16,  21, -13,  -8, -15,  21,  10,  15,
  23, -33,  21, -37,  48,  81,  87,  45, -43,  48,
 -48, -13,  -1,  57,  48,  49,  52,  51,  50,  54,
  53,  55,  56,  48,  85,  85,  85, -45, -46,  -9,
  -8,  -7,  21, -14,  12,  14,  10,  85,  79,  47,
  58,  73,  68,  74,  89,   2,  15,  23,   9,  48,
  -7,  21,  10,  15,  23,  88, -46,  44,  47,  71,
  69,  -2,  64,  -3,  -4,  -5,  -6,  86,  77,  80,
  66,  76,  70,  67,  73,  68,  63,  79,  75,  65,
  74,  72,  78,  -7,  -7,  -7,  -7,  -7,  -7,  -7,
  -9,  79,  -7,  85,  -7,  -7,  -7,  -7,  -7,  -7,
  -7,  -7, -10, -11,  -7,  45,  90,  45,  47,  91,
  92,  45,  -7,  44,  -7,  85, -11,  -7,  -7,  45,
  -7
};
short	yydef[] =
{
  -2,  -2,   5,  -2,   0,   5,   9,  10,  11,  12,
  13,  14,  15,  16,  17,  18,  20,  22,  26,  28,
  30,  32,  43,  49,   3,   4,   7,   6,   0,   0,
   0,  34,  34,  34,  34,  45,   0,  55,  24,  21,
  23,  27,  34,  38,  41,  29,  31,  33,  44,  45,
   0,  50,  51,  54,   8,  55,   0,   0,  60,  61,
  62,  19,  25,  35,   0,  37,   0,   0,  46,  47,
   0,  56,   0,   0,  63,  64,  65,  66,  67,  68,
  69,  70,  71,   0,  36,   0,  42,   0,  53,  72,
  75,  76,  78,  77,  79,  80,  -2,  82,   0,   0,
   0,   0,   0,   0,   0, 102,  -2,  -2,  57,   0,
  74,  78,  81,  58,  59,  39,  48,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 103, 104,
 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
 115, 116, 117,  83,  84,  85,  86,  87,  88,   0,
  73,  40,   0,   0,  92,  93,  94,  95,  96,  97,
  98,  99,   0, 118, 120,   0, 101,   0,   0, 100,
   0,   0, 121,   0,  89,  91, 119, 122,   0,   0,
  90
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  89,  90,   0,   0,  92,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  48,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  87,   0,  88,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  86,   0,  91
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
  42,  43,  44,  45,  46,  47,  49,  50,  51,  52,
  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,
  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,
  83,  84,  85
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
	int i, c;

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
			if(yychar == 0)
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
		
case 2:
#line	45	"nlde.y"
{ clearmem(); } break;
case 4:
#line	49	"nlde.y"
{
		if(errcount == 0) {
			checkdefns();
			out(yypt[-0].yyv.node);
		}
	} break;
case 7:
#line	60	"nlde.y"
{ mode=EQN; } break;
case 8:
#line	61	"nlde.y"
{
		yyval.node = yypt[-0].yyv.node;
	} break;
case 18:
#line	77	"nlde.y"
{ mode=EXTERN; } break;
case 19:
#line	78	"nlde.y"
{
		chiptype = yypt[-1].yyv.hp;
	} break;
case 20:
#line	83	"nlde.y"
{ mode=EXTERN; } break;
case 21:
#line	84	"nlde.y"
{
		chipttype = yypt[-0].yyv.hp;
	} break;
case 22:
#line	89	"nlde.y"
{ mode=EXTERN; } break;
case 23:
#line	90	"nlde.y"
{
		fprintf(stdout, ".r %s\n", yypt[-0].yyv.hp->name);
	} break;
case 25:
#line	96	"nlde.y"
{
		chipname = yypt[-0].yyv.hp;
	} break;
case 26:
#line	101	"nlde.y"
{ mode=INPUT; } break;
case 28:
#line	104	"nlde.y"
{ mode=BOTH; } break;
case 30:
#line	107	"nlde.y"
{ mode=BURIED; } break;
case 32:
#line	110	"nlde.y"
{ mode=OUTPUT; } break;
case 36:
#line	117	"nlde.y"
{
		dclbit(yypt[-2].yyv.hp, yypt[-0].yyv.val);
		setctl(yypt[-2].yyv.hp, CTLNCN);
	} break;
case 37:
#line	122	"nlde.y"
{
		dclbit(yypt[-1].yyv.hp, 0L);
		setctl(yypt[-1].yyv.hp, CTLNCN);
	} break;
case 38:
#line	127	"nlde.y"
{
		dclbit(yypt[-0].yyv.hp, 0L);
	} break;
case 39:
#line	131	"nlde.y"
{
		dclvector(yypt[-3].yyv.hp, yypt[-1].yyv.val, 0);
	} break;
case 40:
#line	135	"nlde.y"
{
		dclvector(yypt[-4].yyv.hp, yypt[-2].yyv.val, 1);
	} break;
case 42:
#line	140	"nlde.y"
{
		dclbit(yypt[-2].yyv.hp, yypt[-0].yyv.val);
	} break;
case 43:
#line	145	"nlde.y"
{ mode=FIELD; } break;
case 47:
#line	151	"nlde.y"
{ SETLO(yypt[-1].yyv.hp, nextfield); yypt[-1].yyv.hp->code = FIELD; } break;
case 48:
#line	152	"nlde.y"
{
		SETHI(yypt[-3].yyv.hp, nextfield);
	} break;
case 49:
#line	158	"nlde.y"
{ mode=INPUT; } break;
case 51:
#line	161	"nlde.y"
{
		switch(yypt[-0].yyv.hp->code){
		default:
			yywarn("\"%s\" not an input", yypt[-0].yyv.hp->name);
			break;
		case 0:
			yyundef(yypt[-0].yyv.hp);
			break;
		case INPUT:
		case BOTH:
		case FIELD:
			checkio(yypt[-0].yyv.hp, INPUT);
			setiused(yypt[-0].yyv.hp,ALLBITS);
			break;
		}
	} break;
case 52:
#line	177	"nlde.y"
{
		switch(yypt[-0].yyv.hp->code){
		default:
			yywarn("\"%s\" not an input", yypt[-0].yyv.hp->name);
			break;
		case 0:
			yyundef(yypt[-0].yyv.hp);
			break;
		case INPUT:
		case BOTH:
		case FIELD:
			checkio(yypt[-0].yyv.hp, INPUT);
			setiused(yypt[-0].yyv.hp,ALLBITS);
			break;
		}
	} break;
case 55:
#line	197	"nlde.y"
{
		yyval.node = build(ELIST, 0, 0);
	} break;
case 56:
#line	201	"nlde.y"
{
		yyval.node = build(ELIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 72:
#line	212	"nlde.y"
{
		checkio(yypt[-2].yyv.hp, OUTPUT);
		switch(yypt[-1].yyv.code){
		case COMEQU:
			setctl(yypt[-2].yyv.hp, CTLINV); break;
		case ADDEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 's'); break;
		case SUBEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 'c'); break;
		case ALTEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 'd'); break;
		case XOREQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 't'); break;
		case MULEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 'e'); break;
		case MODEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 'g'); break;
		case PNDEQU:
			yypt[-2].yyv.hp = addctl(yypt[-2].yyv.hp, 'l'); break;
		}
		yyval.node = build(ASSIGN, (Node*)yypt[-2].yyv.hp, yypt[-0].yyv.node);
		setoused(yypt[-2].yyv.hp,yyval.node);
	} break;
case 73:
#line	236	"nlde.y"
{
		checkio(yypt[-3].yyv.hp, OUTPUT);
		yyval.node = build(DONTCARE, (Node*)yypt[-3].yyv.hp, yypt[-0].yyv.node);
		setdcused(yypt[-3].yyv.hp,yyval.node);
	} break;
case 74:
#line	242	"nlde.y"
{
		if(yypt[-2].yyv.hp != comname(yypt[-2].yyv.hp))
			fprintf(stderr, "line %d warning: ambiguous lhs with compliment\n",
				yylineno);
		if(vconst(yypt[-0].yyv.node)) {
			yypt[-2].yyv.hp->code = CEQN;
			yypt[-2].yyv.hp->val = eval(yypt[-0].yyv.node);
			yyval.node = 0;
		} else {
			yypt[-2].yyv.hp->code = EQN;
			yypt[-2].yyv.hp->oref = ++neqns;
			if(neqns >= NLEAVES) {
				fprintf(stderr, "too many eqns\n");
				exits("error");;
			}
			eqns[neqns] = yypt[-2].yyv.hp;
			yyval.node = build(ASSIGN, (Node*)yypt[-2].yyv.hp, yypt[-0].yyv.node);
		}
		setoused(yypt[-2].yyv.hp,yyval.node);
	} break;
case 77:
#line	267	"nlde.y"
{
		checkio(yypt[-0].yyv.hp, INPUT);
		yyval.node = build(yypt[-0].yyv.hp->code==FIELD ? FIELD : INPUT, (Node*)yypt[-0].yyv.hp, 0);
		setiused(yypt[-0].yyv.hp,ALLBITS);
	} break;
case 78:
#line	273	"nlde.y"
{
		yypt[-0].yyv.hp = comname(yypt[-0].yyv.hp);
		switch(yypt[-0].yyv.hp->code) {
		default:
			yyundef(yypt[-0].yyv.hp);
		case INPUT:
		case BOTH:
			yyval.node = build(INPUT, (Node*)yypt[-0].yyv.hp, 0);
			yyval.node = build(NOT, yyval.node, 0);
			setiused(yypt[-0].yyv.hp,ALLBITS);
			break;
		case FIELD:
			checkio(yypt[-0].yyv.hp, INPUT);
			yyval.node = build(FIELD,(Node*)yypt[-0].yyv.hp,0);
			yyval.node = build(COM, yyval.node, 0);
			setiused(yypt[-0].yyv.hp,ALLBITS);
			break;
		}
	} break;
case 79:
#line	293	"nlde.y"
{
		yyval.node = build(EQN, (Node*)yypt[-0].yyv.hp, 0);
		setiused(yypt[-0].yyv.hp,ALLBITS);
	} break;
case 80:
#line	298	"nlde.y"
{
		yyval.node = build(NUMBER, (Node*)yypt[-0].yyv.hp->val, 0);
		setiused(yypt[-0].yyv.hp,ALLBITS);
	} break;
case 81:
#line	303	"nlde.y"
{
		yyerror("output used as input");
		yyval.node = build(NUMBER, 0, 0);
	} break;
case 82:
#line	308	"nlde.y"
{
		yyval.node = build(NUMBER, (Node*)yypt[-0].yyv.val, 0);
	} break;
case 83:
#line	312	"nlde.y"
{
		yyval.node = build(NEG, yypt[-0].yyv.node, 0);
	} break;
case 84:
#line	316	"nlde.y"
{
		yyval.node = build(NOT, yypt[-0].yyv.node, 0);
	} break;
case 85:
#line	320	"nlde.y"
{
		yyval.node = build(COM, yypt[-0].yyv.node, 0);
	} break;
case 86:
#line	324	"nlde.y"
{
		yyval.node = build(FLONE, yypt[-0].yyv.node, 0);
	} break;
case 87:
#line	328	"nlde.y"
{
		yyval.node = build(FRONE, yypt[-0].yyv.node, 0);
	} break;
case 88:
#line	332	"nlde.y"
{
		yyval.node = build(GREY, yypt[-0].yyv.node, 0);
	} break;
case 89:
#line	336	"nlde.y"
{
		yyval.node = build(CND, yypt[-4].yyv.node, build(ALT, yypt[-2].yyv.node, yypt[-0].yyv.node));
	} break;
case 90:
#line	340	"nlde.y"
{
		yyval.node = build(CND, yypt[-5].yyv.node, build(ALT, yypt[-2].yyv.node, yypt[-0].yyv.node));
	} break;
case 91:
#line	344	"nlde.y"
{
		yyval.node = build(ALT, (Node*)yypt[-2].yyv.val, (Node*)yypt[-0].yyv.val);
		yyval.node = build(PERM, yypt[-4].yyv.node, yyval.node);
	} break;
case 92:
#line	349	"nlde.y"
{
	binop:
		yyval.node = build(yypt[-1].yyv.code,yypt[-2].yyv.node,yypt[-0].yyv.node);
	} break;
case 93:
#line	354	"nlde.y"
{
		goto binop;
	} break;
case 94:
#line	358	"nlde.y"
{
		goto binop;
	} break;
case 95:
#line	362	"nlde.y"
{
		goto binop;
	} break;
case 96:
#line	366	"nlde.y"
{
		goto binop;
	} break;
case 97:
#line	370	"nlde.y"
{
		goto binop;
	} break;
case 98:
#line	374	"nlde.y"
{
		goto binop;
	} break;
case 99:
#line	378	"nlde.y"
{
		goto binop;
	} break;
case 100:
#line	382	"nlde.y"
{
		yyval.node = build(SWITCH, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 101:
#line	386	"nlde.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 102:
#line	390	"nlde.y"
{
		yyerror("expression syntax");
	} break;
case 119:
#line	403	"nlde.y"
{
		Node *t;
		for(t = yypt[-2].yyv.node; t->t2; t = t->t2)
			;
		t->t2 = yypt[-0].yyv.node;
		yyval.node = yypt[-2].yyv.node;
	} break;
case 120:
#line	413	"nlde.y"
{
		yyval.node = build(CASE, yypt[-0].yyv.node, 0);
	} break;
case 121:
#line	417	"nlde.y"
{
		yyval.node = build(CASE, yypt[-0].yyv.node, 0);
		yyval.node = build(ALT, 0, yyval.node);
	} break;
case 122:
#line	422	"nlde.y"
{
		yyval.node = build(CASE, yypt[-0].yyv.node, 0);
		yyval.node = build(ALT, yypt[-2].yyv.node, yyval.node);
	} break;
	}
	goto yystack;  /* stack new state and value */
}
