typedef unsigned char Uchar;
# include <stdio.h>
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern char yytext[];
int yymorfg;
extern Uchar *yysptr, yysbuf[];
int yytchar;
FILE *yyin = {stdin}, *yyout = {stdout};
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
int yylook(void), yywrap(void), yyback(int *, int);
#define A 2
#define str 4
#define sc 6
#define reg 8
#define comment 10
/*
Copyright (c) 1989 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#undef	input	/* defeat lex */
#undef	unput

#include <stdlib.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"

extern YYSTYPE	yylval;
extern int	infunc;

int	lineno	= 1;
int	bracecnt = 0;
int	brackcnt  = 0;
int	parencnt = 0;
int	runecnt = 0;

#ifdef	DEBUG
#	define	RET(x)	{if(dbg)printf("lex %s [%s]\n", tokname(x), yytext); return(x); }
#else
#	define	RET(x)	return(x)
#endif

#define	CADD	cbuf[clen++] = yytext[0]; \
		if (clen >= CBUFLEN-1) { \
			ERROR "string/reg expr %.30s... too long", cbuf SYNTAX; \
			BEGIN A; \
		}

uchar	cbuf[CBUFLEN];
uchar	*s;
int	clen, cflag;
#define YYNEWLINE 10
yylex(void){
int nstr; extern int yyprevious;
switch (yybgin-yysvec-1) {	/* witchcraft */
	case 0:
		BEGIN A;
		break;
	case sc:
		BEGIN A;
		RET('}');
	}
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
	{ lineno++; RET(NL); }
break;
case 2:
	{ ; }
break;
case 3:
{ ; }
break;
case 4:
	{ RET(';'); }
break;
case 5:
{ lineno++; }
break;
case 6:
{ RET(XBEGIN); }
break;
case 7:
	{ RET(XEND); }
break;
case 8:
{ if (infunc) ERROR "illegal nested function" SYNTAX; RET(FUNC); }
break;
case 9:
{ if (!infunc) ERROR "return not in function" SYNTAX; RET(RETURN); }
break;
case 10:
	{ RET(AND); }
break;
case 11:
	{ RET(BOR); }
break;
case 12:
	{ RET(NOT); }
break;
case 13:
	{ yylval.i = NE; RET(NE); }
break;
case 14:
	{ yylval.i = MATCH; RET(MATCHOP); }
break;
case 15:
	{ yylval.i = NOTMATCH; RET(MATCHOP); }
break;
case 16:
	{ yylval.i = LT; RET(LT); }
break;
case 17:
	{ yylval.i = LE; RET(LE); }
break;
case 18:
	{ yylval.i = EQ; RET(EQ); }
break;
case 19:
	{ yylval.i = GE; RET(GE); }
break;
case 20:
	{ yylval.i = GT; RET(GT); }
break;
case 21:
	{ yylval.i = APPEND; RET(APPEND); }
break;
case 22:
	{ yylval.i = INCR; RET(INCR); }
break;
case 23:
	{ yylval.i = DECR; RET(DECR); }
break;
case 24:
	{ yylval.i = ADDEQ; RET(ASGNOP); }
break;
case 25:
	{ yylval.i = SUBEQ; RET(ASGNOP); }
break;
case 26:
	{ yylval.i = MULTEQ; RET(ASGNOP); }
break;
case 27:
	{ yylval.i = DIVEQ; RET(ASGNOP); }
break;
case 28:
	{ yylval.i = MODEQ; RET(ASGNOP); }
break;
case 29:
	{ yylval.i = POWEQ; RET(ASGNOP); }
break;
case 30:
{ yylval.i = POWEQ; RET(ASGNOP); }
break;
case 31:
	{ yylval.i = ASSIGN; RET(ASGNOP); }
break;
case 32:
	{ RET(POWER); }
break;
case 33:
	{ RET(POWER); }
break;
case 34:
{ yylval.cp = fieldadr(atoi(yytext+1)); RET(FIELD); }
break;
case 35:
{ unputstr("(NF)"); return(INDIRECT); }
break;
case 36:
{ int c, n;
		  c = input(); unput(c);
		  if (c == '(' || c == '[' || infunc && (n=isarg(yytext+1)) >= 0) {
			unputstr(yytext+1);
			return(INDIRECT);
		  } else {
			yylval.cp = setsymtab(yytext+1,"",0.0,STR|NUM,symtab);
			RET(IVAR);
		  }
		}
break;
case 37:
	{ RET(INDIRECT); }
break;
case 38:
	{ yylval.cp = setsymtab(yytext, "", 0.0, NUM, symtab); RET(VARNF); }
break;
case 39:
{
		  yylval.cp = setsymtab(yytext, tostring(yytext), atof(yytext), CON|NUM, symtab);
		/* should this also have STR set? */
		  RET(NUMBER); }
break;
case 40:
{ RET(WHILE); }
break;
case 41:
	{ RET(FOR); }
break;
case 42:
	{ RET(DO); }
break;
case 43:
	{ RET(IF); }
break;
case 44:
	{ RET(ELSE); }
break;
case 45:
	{ RET(NEXT); }
break;
case 46:
	{ RET(EXIT); }
break;
case 47:
{ RET(BREAK); }
break;
case 48:
{ RET(CONTINUE); }
break;
case 49:
{ yylval.i = PRINT; RET(PRINT); }
break;
case 50:
{ yylval.i = PRINTF; RET(PRINTF); }
break;
case 51:
{ yylval.i = SPRINTF; RET(SPRINTF); }
break;
case 52:
{ yylval.i = SPLIT; RET(SPLIT); }
break;
case 53:
{ RET(SUBSTR); }
break;
case 54:
	{ yylval.i = SUB; RET(SUB); }
break;
case 55:
	{ yylval.i = GSUB; RET(GSUB); }
break;
case 56:
{ RET(INDEX); }
break;
case 57:
{ RET(MATCHFCN); }
break;
case 58:
	{ RET(IN); }
break;
case 59:
{ RET(GETLINE); }
break;
case 60:
{ RET(CLOSE); }
break;
case 61:
{ RET(DELETE); }
break;
case 62:
{ yylval.i = FLENGTH; RET(BLTIN); }
break;
case 63:
	{ yylval.i = FLOG; RET(BLTIN); }
break;
case 64:
	{ yylval.i = FINT; RET(BLTIN); }
break;
case 65:
	{ yylval.i = FEXP; RET(BLTIN); }
break;
case 66:
	{ yylval.i = FSQRT; RET(BLTIN); }
break;
case 67:
	{ yylval.i = FSIN; RET(BLTIN); }
break;
case 68:
	{ yylval.i = FCOS; RET(BLTIN); }
break;
case 69:
{ yylval.i = FATAN; RET(BLTIN); }
break;
case 70:
{ yylval.i = FSYSTEM; RET(BLTIN); }
break;
case 71:
	{ yylval.i = FRAND; RET(BLTIN); }
break;
case 72:
{ yylval.i = FSRAND; RET(BLTIN); }
break;
case 73:
{ yylval.i = FTOUPPER; RET(BLTIN); }
break;
case 74:
{ yylval.i = FTOLOWER; RET(BLTIN); }
break;
case 75:
{ yylval.i = FFLUSH; RET(BLTIN); }
break;
case 76:
{ yylval.i = FUTF; RET(BLTIN); }
break;
case 77:
{ int n, c;
		  c = input(); unput(c);	/* look for '(' */
		  if (c != '(' && infunc && (n=isarg(yytext)) >= 0) {
			yylval.i = n;
			RET(ARG);
		  } else {
			yylval.cp = setsymtab(yytext,"",0.0,STR|NUM,symtab);
			if (c == '(') {
				RET(CALL);
			} else {
				RET(VAR);
			}
		  }
		}
break;
case 78:
	{ BEGIN str; clen = 0; }
break;
case 79:
	{ if (--bracecnt < 0) ERROR "extra }" SYNTAX; BEGIN sc; RET(';'); }
break;
case 80:
	{ if (--brackcnt < 0) ERROR "extra ]" SYNTAX; RET(']'); }
break;
case 81:
	{ if (--parencnt < 0) ERROR "extra )" SYNTAX; RET(')'); }
break;
case 82:
	{ if (yytext[0] == '{') bracecnt++;
		  else if (yytext[0] == '[') brackcnt++;
		  else if (yytext[0] == '(') parencnt++;
		  RET(yylval.i = yytext[0]); /* everything else */ }
break;
case 83:
{ cbuf[clen++] = '\\'; cbuf[clen++] = yytext[1]; }
break;
case 84:
	{ ERROR "newline in regular expression %.10s...", cbuf SYNTAX; lineno++; BEGIN A; }
break;
case 85:
{ BEGIN A;
		  cbuf[clen] = 0;
		  yylval.s = tostring(cbuf);
		  unput('/');
		  RET(REGEXPR); }
break;
case 86:
	{ int uc, i;
		  CADD;
		  uc = *((uchar *)yytext)&0xFF;
		  if (uc & 0x80) {
			i = 0x40;
			while (uc&i) {
				cbuf[clen++] = input();
				i >>= 1;
		  	}
		  }
		}
break;
case 87:
	{ BEGIN A;
		  cbuf[clen] = 0; s = tostring(cbuf);
		  cbuf[clen] = ' '; cbuf[++clen] = 0;
		  yylval.cp = setsymtab(cbuf, s, 0.0, CON|STR, symtab);
		  RET(STRING); }
break;
case 88:
	{ ERROR "newline in string %.10s...", cbuf SYNTAX; lineno++; BEGIN A; }
break;
case 89:
{ cbuf[clen++] = '"'; }
break;
case 90:
{ cbuf[clen++] = '\n'; }
break;
case 91:
{ cbuf[clen++] = '\t'; }
break;
case 92:
{ cbuf[clen++] = '\f'; }
break;
case 93:
{ cbuf[clen++] = '\r'; }
break;
case 94:
{ cbuf[clen++] = '\b'; }
break;
case 95:
{ cbuf[clen++] = '\v'; }
break;
case 96:
{ cbuf[clen++] = '\007'; }
break;
case 97:
{ cbuf[clen++] = '\\'; }
break;
case 98:
{ int n;
		  sscanf(yytext+1, "%o", &n); cbuf[clen++] = n; }
break;
case 99:
{ int n;	/* ANSI permits any number! */
		  sscanf(yytext+2, "%x", &n); cbuf[clen++] = n; }
break;
case 100:
{ cbuf[clen++] = yytext[1]; }
break;
case 101:
	{ int uc, i;
		  CADD;
		  uc = *((uchar *)yytext)&0xFF;
		  if (uc & 0x80) {
			i = 0x40;
			while (uc&i) {
				cbuf[clen++] = input();
				i >>= 1;
		  	}
		  }
		}
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */

void startreg(void)	/* start parsing a regular expression */
{
	BEGIN reg;
	clen = 0;
}

/* input() and unput() are transcriptions of the standard lex
   macros for input and output with additions for error message
   printing.  God help us all if someone changes how lex works.
*/

uchar	ebuf[300];
uchar	*ep = ebuf;

input(void)	/* get next lexical input character */
{
	register int c;
	extern uchar *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr)&0xff;
	else if (lexprog != NULL) {	/* awk '...' */
		if (c = (*lexprog&0xff))
			lexprog++;
	} else				/* awk -f ... */
		c = pgetc();
	if (c == '\n')
		yylineno++;
	else if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	c &= 0xFF;
	*ep++ = c;
	return c;
}

void unput(int c)	/* put lexical character back on input */
{
	yytchar = c;
	if (yytchar == '\n')
		yylineno--;
	*yysptr++ = yytchar;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
}


void unputstr(char *s)	/* put a string back on input */
{
	int i;

	for (i = strlen(s)-1; i >= 0; i--)
		unput(s[i]);
}
int yyvstop[] = {
0,

82,
0,

3,
82,
0,

1,
0,

12,
82,
0,

78,
82,
0,

2,
82,
0,

37,
82,
0,

82,
0,

82,
0,

81,
82,
0,

82,
0,

82,
0,

82,
0,

82,
0,

82,
0,

39,
82,
0,

4,
82,
0,

16,
82,
0,

31,
82,
0,

20,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

82,
0,

80,
82,
0,

33,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

77,
82,
0,

82,
0,

79,
82,
0,

14,
82,
0,

101,
0,

88,
0,

87,
101,
0,

101,
0,

86,
0,

84,
0,

85,
86,
0,

86,
0,

3,
0,

13,
0,

15,
0,

2,
0,

34,
0,

36,
0,

36,
0,

28,
0,

10,
0,

32,
0,

26,
0,

22,
0,

24,
0,

23,
0,

25,
0,

39,
0,

27,
0,

39,
0,

39,
0,

17,
0,

18,
0,

19,
0,

21,
0,

77,
0,

77,
0,

77,
0,

38,
77,
0,

5,
0,

29,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

42,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

43,
77,
0,

58,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

11,
0,

100,
0,

89,
100,
0,

98,
100,
0,

97,
100,
0,

96,
100,
0,

94,
100,
0,

92,
100,
0,

90,
100,
0,

93,
100,
0,

91,
100,
0,

95,
100,
0,

100,
0,

83,
0,

35,
36,
0,

30,
0,

39,
0,

77,
0,

7,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

68,
77,
0,

77,
0,

77,
0,

77,
0,

65,
77,
0,

77,
0,

41,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

64,
77,
0,

77,
0,

63,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

67,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

54,
77,
0,

77,
0,

77,
0,

77,
0,

76,
77,
0,

77,
0,

98,
0,

99,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

44,
77,
0,

46,
77,
0,

77,
0,

8,
77,
0,

77,
0,

55,
77,
0,

77,
0,

77,
0,

77,
0,

45,
77,
0,

77,
0,

71,
77,
0,

77,
0,

77,
0,

77,
0,

66,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

98,
0,

6,
77,
0,

69,
77,
0,

47,
77,
0,

60,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

56,
77,
0,

77,
0,

57,
77,
0,

49,
77,
0,

77,
0,

52,
77,
0,

77,
0,

72,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

40,
77,
0,

77,
0,

61,
77,
0,

75,
77,
0,

77,
0,

77,
0,

62,
77,
0,

50,
77,
0,

9,
77,
0,

77,
0,

53,
77,
0,

70,
77,
0,

77,
0,

77,
0,

77,
0,

77,
0,

59,
77,
0,

51,
77,
0,

74,
77,
0,

73,
77,
0,

48,
77,
0,

8,
77,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	3,13,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	3,14,	3,15,	
0,0,	0,0,	0,0,	0,0,	
0,0,	14,68,	0,0,	0,0,	
0,0,	5,60,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	5,60,	5,61,	0,0,	
0,0,	0,0,	3,16,	3,17,	
3,18,	3,19,	3,20,	3,21,	
14,68,	37,96,	3,22,	3,23,	
3,24,	71,0,	3,25,	3,26,	
3,27,	3,28,	6,62,	0,0,	
0,0,	0,0,	5,62,	0,0,	
0,0,	3,28,	0,0,	0,0,	
3,29,	3,30,	3,31,	3,32,	
0,0,	21,76,	3,33,	3,34,	
5,60,	10,66,	3,35,	23,77,	
3,33,	0,0,	16,69,	0,0,	
5,60,	0,0,	0,0,	3,36,	
0,0,	0,0,	0,0,	0,0,	
0,0,	5,60,	0,0,	24,79,	
9,64,	20,75,	23,78,	5,60,	
27,84,	3,37,	3,38,	3,39,	
9,64,	9,65,	3,40,	3,41,	
3,42,	3,43,	3,44,	3,45,	
3,46,	24,80,	3,47,	25,81,	
6,63,	3,48,	3,49,	3,50,	
5,63,	3,51,	10,67,	3,52,	
3,53,	3,54,	3,55,	30,88,	
3,56,	31,89,	34,93,	25,82,	
35,94,	3,57,	3,58,	3,59,	
4,16,	4,17,	4,18,	4,19,	
4,20,	4,21,	9,66,	9,64,	
4,22,	4,23,	4,24,	16,70,	
4,25,	4,26,	4,27,	9,64,	
32,90,	32,91,	36,95,	39,97,	
40,98,	41,99,	42,100,	44,104,	
9,64,	42,101,	4,29,	4,30,	
4,31,	4,32,	9,64,	43,102,	
49,115,	4,34,	47,111,	44,105,	
4,35,	45,106,	50,116,	46,109,	
18,71,	43,103,	47,112,	51,117,	
48,113,	4,36,	45,107,	54,126,	
18,71,	18,0,	55,127,	9,67,	
45,108,	46,110,	48,114,	56,128,	
57,129,	77,144,	52,118,	4,37,	
4,38,	4,39,	52,119,	93,147,	
4,40,	4,41,	4,42,	4,43,	
4,44,	4,45,	4,46,	83,87,	
4,47,	94,148,	98,149,	4,48,	
4,49,	4,50,	99,150,	4,51,	
100,151,	4,52,	4,53,	4,54,	
4,55,	101,152,	4,56,	18,71,	
102,154,	53,120,	101,153,	4,57,	
4,58,	4,59,	104,155,	18,71,	
53,121,	53,122,	53,123,	105,156,	
106,158,	53,124,	107,159,	83,87,	
18,71,	53,125,	105,157,	108,160,	
109,161,	110,162,	18,71,	19,72,	
19,72,	19,72,	19,72,	19,72,	
19,72,	19,72,	19,72,	19,72,	
19,72,	113,165,	114,166,	115,167,	
116,168,	117,169,	118,170,	119,171,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,74,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	120,172,	122,175,	
123,176,	124,177,	19,73,	125,178,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	19,73,	19,73,	
19,73,	19,73,	26,83,	26,83,	
26,83,	26,83,	26,83,	26,83,	
26,83,	26,83,	26,83,	26,83,	
28,85,	127,181,	28,86,	28,86,	
28,86,	28,86,	28,86,	28,86,	
28,86,	28,86,	28,86,	28,86,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	121,173,	28,87,	
128,182,	147,185,	149,186,	150,187,	
121,174,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	28,87,	
151,188,	152,189,	154,190,	33,92,	
155,191,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	33,92,	
33,92,	33,92,	33,92,	63,130,	
156,192,	67,142,	158,193,	160,194,	
161,195,	162,196,	163,197,	63,130,	
63,0,	67,142,	67,0,	72,72,	
72,72,	72,72,	72,72,	72,72,	
72,72,	72,72,	72,72,	72,72,	
72,72,	73,73,	73,73,	73,73,	
73,73,	73,73,	73,73,	73,73,	
73,73,	73,73,	73,73,	112,163,	
63,131,	165,198,	167,199,	168,200,	
74,73,	74,73,	74,73,	74,73,	
74,73,	74,73,	74,73,	74,73,	
74,73,	74,73,	63,132,	112,164,	
67,142,	169,201,	126,179,	73,73,	
170,202,	171,203,	63,130,	173,204,	
67,142,	174,205,	74,143,	126,180,	
175,206,	176,207,	177,208,	63,130,	
178,209,	67,142,	74,73,	179,210,	
180,211,	63,130,	182,212,	67,142,	
85,85,	85,85,	85,85,	85,85,	
85,85,	85,85,	85,85,	85,85,	
85,85,	85,85,	132,183,	132,183,	
132,183,	132,183,	132,183,	132,183,	
132,183,	132,183,	63,133,	185,214,	
186,215,	85,87,	187,216,	63,134,	
63,135,	188,217,	189,218,	190,219,	
63,136,	193,220,	194,221,	195,222,	
197,223,	198,224,	199,225,	201,226,	
63,137,	203,227,	204,228,	205,229,	
63,138,	207,230,	63,139,	208,231,	
63,140,	209,232,	63,141,	210,233,	
211,234,	87,145,	212,235,	87,145,	
218,236,	85,87,	87,146,	87,146,	
87,146,	87,146,	87,146,	87,146,	
87,146,	87,146,	87,146,	87,146,	
141,184,	141,184,	141,184,	141,184,	
141,184,	141,184,	141,184,	141,184,	
141,184,	141,184,	219,237,	220,238,	
221,239,	222,240,	224,241,	226,242,	
227,243,	141,184,	141,184,	141,184,	
141,184,	141,184,	141,184,	229,244,	
231,245,	143,73,	143,73,	143,73,	
143,73,	143,73,	143,73,	143,73,	
143,73,	143,73,	143,73,	145,146,	
145,146,	145,146,	145,146,	145,146,	
145,146,	145,146,	145,146,	145,146,	
145,146,	232,246,	233,247,	234,248,	
236,249,	141,184,	141,184,	141,184,	
141,184,	141,184,	141,184,	143,73,	
183,213,	183,213,	183,213,	183,213,	
183,213,	183,213,	183,213,	183,213,	
239,250,	240,251,	244,252,	247,253,	
248,254,	249,255,	250,256,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-1,	0,		0,	
yycrank+-95,	yysvec+3,	0,	
yycrank+-20,	0,		0,	
yycrank+-16,	yysvec+5,	0,	
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-87,	0,		0,	
yycrank+-22,	yysvec+9,	0,	
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+0,	0,		yyvstop+1,
yycrank+8,	0,		yyvstop+3,
yycrank+0,	0,		yyvstop+6,
yycrank+13,	0,		yyvstop+8,
yycrank+0,	0,		yyvstop+11,
yycrank+-167,	0,		yyvstop+14,
yycrank+191,	0,		yyvstop+17,
yycrank+28,	0,		yyvstop+20,
yycrank+27,	0,		yyvstop+22,
yycrank+0,	0,		yyvstop+24,
yycrank+29,	0,		yyvstop+27,
yycrank+44,	0,		yyvstop+29,
yycrank+62,	0,		yyvstop+31,
yycrank+266,	0,		yyvstop+33,
yycrank+31,	0,		yyvstop+35,
yycrank+278,	0,		yyvstop+37,
yycrank+0,	0,		yyvstop+40,
yycrank+58,	0,		yyvstop+43,
yycrank+60,	0,		yyvstop+46,
yycrank+83,	0,		yyvstop+49,
yycrank+288,	0,		yyvstop+52,
yycrank+53,	yysvec+33,	yyvstop+55,
yycrank+46,	yysvec+33,	yyvstop+58,
yycrank+76,	yysvec+33,	yyvstop+61,
yycrank+31,	0,		yyvstop+64,
yycrank+0,	0,		yyvstop+66,
yycrank+86,	0,		yyvstop+69,
yycrank+32,	yysvec+33,	yyvstop+72,
yycrank+35,	yysvec+33,	yyvstop+75,
yycrank+42,	yysvec+33,	yyvstop+78,
yycrank+58,	yysvec+33,	yyvstop+81,
yycrank+43,	yysvec+33,	yyvstop+84,
yycrank+63,	yysvec+33,	yyvstop+87,
yycrank+66,	yysvec+33,	yyvstop+90,
yycrank+60,	yysvec+33,	yyvstop+93,
yycrank+71,	yysvec+33,	yyvstop+96,
yycrank+63,	yysvec+33,	yyvstop+99,
yycrank+65,	yysvec+33,	yyvstop+102,
yycrank+57,	yysvec+33,	yyvstop+105,
yycrank+89,	yysvec+33,	yyvstop+108,
yycrank+112,	yysvec+33,	yyvstop+111,
yycrank+64,	yysvec+33,	yyvstop+114,
yycrank+62,	yysvec+33,	yyvstop+117,
yycrank+79,	yysvec+33,	yyvstop+120,
yycrank+60,	0,		yyvstop+123,
yycrank+0,	0,		yyvstop+125,
yycrank+0,	0,		yyvstop+128,
yycrank+0,	0,		yyvstop+131,
yycrank+0,	0,		yyvstop+133,
yycrank+0,	0,		yyvstop+135,
yycrank+-410,	0,		yyvstop+138,
yycrank+0,	0,		yyvstop+140,
yycrank+0,	0,		yyvstop+142,
yycrank+0,	0,		yyvstop+144,
yycrank+-412,	0,		yyvstop+147,
yycrank+0,	yysvec+14,	yyvstop+149,
yycrank+0,	0,		yyvstop+151,
yycrank+0,	0,		yyvstop+153,
yycrank+-35,	yysvec+18,	yyvstop+155,
yycrank+375,	0,		yyvstop+157,
yycrank+385,	yysvec+19,	yyvstop+159,
yycrank+400,	yysvec+19,	yyvstop+161,
yycrank+0,	0,		yyvstop+163,
yycrank+0,	0,		yyvstop+165,
yycrank+124,	0,		yyvstop+167,
yycrank+0,	0,		yyvstop+169,
yycrank+0,	0,		yyvstop+171,
yycrank+0,	0,		yyvstop+173,
yycrank+0,	0,		yyvstop+175,
yycrank+0,	0,		yyvstop+177,
yycrank+130,	yysvec+26,	yyvstop+179,
yycrank+0,	0,		yyvstop+181,
yycrank+436,	0,		yyvstop+183,
yycrank+0,	yysvec+28,	yyvstop+185,
yycrank+490,	0,		0,	
yycrank+0,	0,		yyvstop+187,
yycrank+0,	0,		yyvstop+189,
yycrank+0,	0,		yyvstop+191,
yycrank+0,	0,		yyvstop+193,
yycrank+0,	yysvec+33,	yyvstop+195,
yycrank+120,	yysvec+33,	yyvstop+197,
yycrank+133,	yysvec+33,	yyvstop+199,
yycrank+0,	yysvec+33,	yyvstop+201,
yycrank+0,	0,		yyvstop+204,
yycrank+0,	0,		yyvstop+206,
yycrank+105,	yysvec+33,	yyvstop+208,
yycrank+105,	yysvec+33,	yyvstop+210,
yycrank+97,	yysvec+33,	yyvstop+212,
yycrank+103,	yysvec+33,	yyvstop+214,
yycrank+108,	yysvec+33,	yyvstop+216,
yycrank+0,	yysvec+33,	yyvstop+218,
yycrank+107,	yysvec+33,	yyvstop+221,
yycrank+122,	yysvec+33,	yyvstop+223,
yycrank+120,	yysvec+33,	yyvstop+225,
yycrank+116,	yysvec+33,	yyvstop+227,
yycrank+125,	yysvec+33,	yyvstop+229,
yycrank+120,	yysvec+33,	yyvstop+231,
yycrank+120,	yysvec+33,	yyvstop+233,
yycrank+0,	yysvec+33,	yyvstop+235,
yycrank+343,	yysvec+33,	yyvstop+238,
yycrank+139,	yysvec+33,	yyvstop+241,
yycrank+147,	yysvec+33,	yyvstop+243,
yycrank+135,	yysvec+33,	yyvstop+245,
yycrank+132,	yysvec+33,	yyvstop+247,
yycrank+148,	yysvec+33,	yyvstop+249,
yycrank+144,	yysvec+33,	yyvstop+251,
yycrank+139,	yysvec+33,	yyvstop+253,
yycrank+172,	yysvec+33,	yyvstop+255,
yycrank+238,	yysvec+33,	yyvstop+257,
yycrank+169,	yysvec+33,	yyvstop+259,
yycrank+187,	yysvec+33,	yyvstop+261,
yycrank+187,	yysvec+33,	yyvstop+263,
yycrank+172,	yysvec+33,	yyvstop+265,
yycrank+354,	yysvec+33,	yyvstop+267,
yycrank+223,	yysvec+33,	yyvstop+269,
yycrank+243,	yysvec+33,	yyvstop+271,
yycrank+0,	0,		yyvstop+273,
yycrank+0,	0,		yyvstop+275,
yycrank+0,	0,		yyvstop+277,
yycrank+446,	0,		yyvstop+280,
yycrank+0,	0,		yyvstop+283,
yycrank+0,	0,		yyvstop+286,
yycrank+0,	0,		yyvstop+289,
yycrank+0,	0,		yyvstop+292,
yycrank+0,	0,		yyvstop+295,
yycrank+0,	0,		yyvstop+298,
yycrank+0,	0,		yyvstop+301,
yycrank+0,	0,		yyvstop+304,
yycrank+500,	0,		yyvstop+307,
yycrank+0,	0,		yyvstop+309,
yycrank+525,	yysvec+19,	yyvstop+311,
yycrank+0,	0,		yyvstop+314,
yycrank+535,	0,		0,	
yycrank+0,	yysvec+145,	yyvstop+316,
yycrank+276,	yysvec+33,	yyvstop+318,
yycrank+0,	yysvec+33,	yyvstop+320,
yycrank+240,	yysvec+33,	yyvstop+323,
yycrank+254,	yysvec+33,	yyvstop+325,
yycrank+265,	yysvec+33,	yyvstop+327,
yycrank+265,	yysvec+33,	yyvstop+329,
yycrank+0,	yysvec+33,	yyvstop+331,
yycrank+281,	yysvec+33,	yyvstop+334,
yycrank+283,	yysvec+33,	yyvstop+336,
yycrank+296,	yysvec+33,	yyvstop+338,
yycrank+0,	yysvec+33,	yyvstop+340,
yycrank+297,	yysvec+33,	yyvstop+343,
yycrank+0,	yysvec+33,	yyvstop+345,
yycrank+316,	yysvec+33,	yyvstop+348,
yycrank+308,	yysvec+33,	yyvstop+350,
yycrank+319,	yysvec+33,	yyvstop+352,
yycrank+317,	yysvec+33,	yyvstop+354,
yycrank+0,	yysvec+33,	yyvstop+356,
yycrank+342,	yysvec+33,	yyvstop+359,
yycrank+0,	yysvec+33,	yyvstop+361,
yycrank+347,	yysvec+33,	yyvstop+364,
yycrank+331,	yysvec+33,	yyvstop+366,
yycrank+351,	yysvec+33,	yyvstop+368,
yycrank+364,	yysvec+33,	yyvstop+370,
yycrank+348,	yysvec+33,	yyvstop+372,
yycrank+0,	yysvec+33,	yyvstop+374,
yycrank+362,	yysvec+33,	yyvstop+377,
yycrank+364,	yysvec+33,	yyvstop+379,
yycrank+356,	yysvec+33,	yyvstop+381,
yycrank+363,	yysvec+33,	yyvstop+383,
yycrank+359,	yysvec+33,	yyvstop+385,
yycrank+360,	yysvec+33,	yyvstop+388,
yycrank+368,	yysvec+33,	yyvstop+390,
yycrank+368,	yysvec+33,	yyvstop+392,
yycrank+0,	yysvec+33,	yyvstop+394,
yycrank+374,	yysvec+33,	yyvstop+397,
yycrank+556,	0,		yyvstop+399,
yycrank+0,	yysvec+141,	yyvstop+401,
yycrank+425,	yysvec+33,	yyvstop+403,
yycrank+454,	yysvec+33,	yyvstop+405,
yycrank+399,	yysvec+33,	yyvstop+407,
yycrank+408,	yysvec+33,	yyvstop+409,
yycrank+405,	yysvec+33,	yyvstop+411,
yycrank+395,	yysvec+33,	yyvstop+413,
yycrank+0,	yysvec+33,	yyvstop+415,
yycrank+0,	yysvec+33,	yyvstop+418,
yycrank+398,	yysvec+33,	yyvstop+421,
yycrank+398,	yysvec+33,	yyvstop+423,
yycrank+410,	yysvec+33,	yyvstop+426,
yycrank+0,	yysvec+33,	yyvstop+428,
yycrank+396,	yysvec+33,	yyvstop+431,
yycrank+401,	yysvec+33,	yyvstop+433,
yycrank+414,	yysvec+33,	yyvstop+435,
yycrank+0,	yysvec+33,	yyvstop+437,
yycrank+403,	yysvec+33,	yyvstop+440,
yycrank+0,	yysvec+33,	yyvstop+442,
yycrank+407,	yysvec+33,	yyvstop+445,
yycrank+406,	yysvec+33,	yyvstop+447,
yycrank+413,	yysvec+33,	yyvstop+449,
yycrank+0,	yysvec+33,	yyvstop+451,
yycrank+425,	yysvec+33,	yyvstop+454,
yycrank+411,	yysvec+33,	yyvstop+456,
yycrank+428,	yysvec+33,	yyvstop+458,
yycrank+412,	yysvec+33,	yyvstop+460,
yycrank+420,	yysvec+33,	yyvstop+462,
yycrank+433,	yysvec+33,	yyvstop+464,
yycrank+0,	0,		yyvstop+466,
yycrank+0,	yysvec+33,	yyvstop+468,
yycrank+0,	yysvec+33,	yyvstop+471,
yycrank+0,	yysvec+33,	yyvstop+474,
yycrank+0,	yysvec+33,	yyvstop+477,
yycrank+426,	yysvec+33,	yyvstop+480,
yycrank+457,	yysvec+33,	yyvstop+482,
yycrank+455,	yysvec+33,	yyvstop+484,
yycrank+455,	yysvec+33,	yyvstop+486,
yycrank+451,	yysvec+33,	yyvstop+488,
yycrank+0,	yysvec+33,	yyvstop+490,
yycrank+458,	yysvec+33,	yyvstop+493,
yycrank+0,	yysvec+33,	yyvstop+495,
yycrank+461,	yysvec+33,	yyvstop+498,
yycrank+454,	yysvec+33,	yyvstop+501,
yycrank+0,	yysvec+33,	yyvstop+503,
yycrank+455,	yysvec+33,	yyvstop+506,
yycrank+0,	yysvec+33,	yyvstop+508,
yycrank+458,	yysvec+33,	yyvstop+511,
yycrank+484,	yysvec+33,	yyvstop+513,
yycrank+493,	yysvec+33,	yyvstop+515,
yycrank+494,	yysvec+33,	yyvstop+517,
yycrank+0,	yysvec+33,	yyvstop+519,
yycrank+479,	yysvec+33,	yyvstop+522,
yycrank+0,	yysvec+33,	yyvstop+524,
yycrank+0,	yysvec+33,	yyvstop+527,
yycrank+501,	yysvec+33,	yyvstop+530,
yycrank+512,	yysvec+33,	yyvstop+532,
yycrank+0,	yysvec+33,	yyvstop+534,
yycrank+0,	yysvec+33,	yyvstop+537,
yycrank+0,	yysvec+33,	yyvstop+540,
yycrank+512,	yysvec+33,	yyvstop+543,
yycrank+0,	yysvec+33,	yyvstop+545,
yycrank+0,	yysvec+33,	yyvstop+548,
yycrank+501,	yysvec+33,	yyvstop+551,
yycrank+502,	yysvec+33,	yyvstop+553,
yycrank+516,	yysvec+33,	yyvstop+555,
yycrank+508,	yysvec+33,	yyvstop+557,
yycrank+0,	yysvec+33,	yyvstop+559,
yycrank+0,	yysvec+33,	yyvstop+562,
yycrank+0,	yysvec+33,	yyvstop+565,
yycrank+0,	yysvec+33,	yyvstop+568,
yycrank+0,	yysvec+33,	yyvstop+571,
yycrank+0,	yysvec+33,	yyvstop+574,
0,	0,	0};
struct yywork *yytop = yycrank+618;
struct yysvf *yybgin = yysvec+1;
Uchar yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'8' ,'8' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,01  ,01  ,01  ,01  ,'G' ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
Uchar yyextra[] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
int yylineno =1;
# define YYU(x) x
char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
Uchar yysbuf[YYLMAX];
Uchar *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
# ifdef LEXDEBUG
extern void allprint(char);
# endif
yylook(void){
	struct yysvf *yystate, **lsp;
	struct yywork *yyt;
	struct yysvf *yyz;
	int yych;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	Uchar *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	if (!yymorfg)
		yylastch = (Uchar*)yytext;
	else {
		yymorfg=0;
		yylastch = (Uchar*)yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank){		/* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
			*yylastch++ = yych = input();
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-(Uchar*)yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch '%s'", yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = input();
		yytext[0] = yyprevious;
		if (yyprevious>0)
			output(yyprevious);
		yylastch = (Uchar*)yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	return(0);	/* shut up the compiler; i have no idea what should be returned */
	}
yyback(int *p, int m)
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(void){
	return(input());
}
void
yyoutput(int c)
{
	output(c);
}
void
yyunput(int c)
{
	unput(c);
}
