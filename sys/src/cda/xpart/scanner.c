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
#include "part.h"
#include "parser.tab.h"

int	yyback(int *, int);
int	yylook(void);
int	yywrap(void);

char cbuf[MAXPIN];
int clen;
int index;

#undef input
#undef unput

#define normal 2
#define string 4
#define YYNEWLINE 10
yylex(void){
int nstr; extern int yyprevious;
switch (yybgin-yysvec-1) {	/* witchcraft */
	case 0:
		BEGIN normal;
		break;
	}
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
{ return(_HEX); }
break;
case 2:
{ return(_OCTAL); }
break;
case 3:
{ return(_DECIMAL); }
break;
case 4:
{ return(_BINARY); }
break;
case 5:
	{ BEGIN string; clen=0; }
break;
case 6:
	{ 	cbuf[clen]='\0';
		  		BEGIN normal;
		  		return(_STRING);
			}
break;
case 7:
	;
break;
case 8:
	;
break;
case 9:
	{ cbuf[clen++] = '"'; }
break;
case 10:
	{ cbuf[clen++] = '\\'; }
break;
case 11:
	{ cbuf[clen++] = yytext[0];
			  if(clen>=MAXPIN-1)
				{yyerror("string too long",cbuf); BEGIN normal;
			  }
			}
break;
case 12:
	;
break;
case 13:
	;
break;
case 14:
	;
break;
case 15:
	;
break;
case 16:
{ return(_PART); }
break;
case 17:
	return(_ARRAY);
break;
case 18:
	return(_INPUTS);
break;
case 19:
	return(_OUTPUTS);
break;
case 20:
	return(_SET);
break;
case 21:
	return(_RESET);
break;
case 22:
	return(_CLOCK);
break;
case 23:
return(_MACROCELL);
break;
case 24:
	return(_ENABLED);
break;
case 25:
	return(_ENABLES);
break;
case 26:
	return(_CLOCKED);
break;
case 27:
return(_INVERTED);
break;
case 28:
return(_INTERNAL);
break;
case 29:
return(_EXTERNAL);
break;
case 30:
	return(_INPUT);
break;
case 31:
	return(_OUTPUT);
break;
case 32:
	return(_FUSES);
break;
case 33:
return(_COMPLEMENTPLUS);
break;
case 34:
return(_COMPLEMENTMINUS);
break;
case 35:
	return(_PINS);
break;
case 36:
	return(_OFFSET);
break;
case 37:
	return(_DECLARE);
break;
case 38:
	return(_VCC);
break;
case 39:
	return(_GND);
break;
case 40:
return(_INVERTED);
break;
case 41:
	return(_PACKAGE);
break;
case 42:
	return(_INVERT);
break;
case 43:
	return(_ENABLE);
break;
case 44:
	return(_NOT);
break;
case 45:
return(_ID);
break;
case 46:
	return(yytext[0]);
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */

/* input() and unput() are transcriptions of the standard lex
   macros for input and output with additions for error message
   printing.  God help us all if someone changes how lex works.
*/

unsigned char ebuf[300];
unsigned char *ep = ebuf;
extern FILE * scan_fp;

int
input()
{
	register c;
	extern unsigned char *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr);
	else
		c = fgetc(scan_fp);
	if (c == '\n')
		yylineno++;
	else
	if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	return *ep++ = c;
}

void
unput(c)
{
	yytchar = c;
	if (yytchar == '\n')
		yylineno--;
	*yysptr++ = yytchar;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
}

int
yywrap(void)
{
	return(1);
}
int yyvstop[] = {
0,

46,
0,

15,
46,
0,

13,
0,

14,
46,
0,

5,
46,
0,

46,
-12,
0,

45,
46,
0,

3,
46,
0,

3,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

45,
46,
0,

11,
0,

7,
0,

8,
11,
0,

6,
11,
0,

11,
0,

-12,
0,

12,
0,

45,
0,

2,
3,
0,

3,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

9,
0,

10,
0,

16,
0,

4,
16,
0,

1,
16,
0,

1,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

44,
45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

20,
45,
0,

45,
0,

1,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

35,
45,
0,

45,
0,

45,
0,

17,
45,
0,

22,
45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

32,
45,
0,

45,
0,

30,
45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

21,
45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

43,
45,
0,

45,
0,

39,
45,
0,

18,
45,
0,

45,
0,

42,
45,
0,

45,
0,

36,
45,
0,

31,
45,
0,

45,
0,

38,
45,
0,

26,
45,
0,

45,
0,

37,
45,
0,

24,
45,
0,

25,
45,
0,

45,
0,

45,
0,

45,
0,

45,
0,

19,
45,
0,

41,
45,
0,

45,
0,

29,
45,
0,

28,
45,
0,

27,
40,
45,
0,

45,
0,

45,
0,

23,
45,
0,

45,
0,

33,
0,

34,
0,
0};
# define YYTYPE Uchar
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	3,7,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	3,8,	3,9,	
0,0,	0,0,	5,29,	0,0,	
0,0,	0,0,	12,34,	0,0,	
0,0,	0,0,	0,0,	5,30,	
0,0,	0,0,	0,0,	12,35,	
0,0,	0,0,	4,8,	0,0,	
0,0,	3,10,	6,31,	3,11,	
6,32,	33,60,	3,12,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	5,31,	0,0,	5,32,	
3,13,	3,14,	3,15,	3,15,	
0,0,	4,10,	0,0,	4,11,	
0,0,	3,15,	4,12,	0,0,	
5,29,	5,29,	0,0,	5,29,	
12,34,	12,34,	3,13,	12,34,	
0,0,	5,29,	4,15,	63,63,	
63,63,	12,34,	0,0,	0,0,	
0,0,	0,0,	5,29,	0,0,	
0,0,	0,0,	12,34,	62,62,	
62,62,	62,62,	62,62,	62,62,	
62,62,	62,62,	62,62,	62,62,	
62,62,	0,0,	6,33,	33,61,	
0,0,	0,0,	3,16,	23,51,	
3,17,	3,18,	3,19,	3,20,	
3,21,	5,33,	3,22,	18,45,	
27,57,	45,69,	3,23,	3,24,	
3,25,	3,26,	22,50,	3,27,	
3,28,	16,42,	4,16,	19,46,	
4,17,	4,18,	4,19,	4,20,	
4,21,	17,43,	4,22,	20,48,	
17,44,	19,47,	4,23,	4,24,	
4,25,	4,26,	21,49,	4,27,	
4,28,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
24,52,	42,66,	43,67,	44,68,	
46,70,	47,71,	48,72,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	49,73,	51,77,	52,78,	
53,79,	13,36,	54,80,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	13,36,	13,36,	13,36,	
13,36,	14,37,	14,38,	14,38,	
14,38,	14,38,	14,38,	14,38,	
14,38,	14,38,	14,39,	14,39,	
50,74,	55,81,	56,82,	57,83,	
50,75,	58,84,	50,76,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	59,85,	66,87,	67,88,	
68,89,	14,37,	69,90,	14,37,	
14,40,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,37,	14,37,	
14,37,	14,37,	14,41,	14,37,	
14,37,	15,39,	15,39,	15,39,	
15,39,	15,39,	15,39,	15,39,	
15,39,	25,53,	26,55,	28,58,	
70,91,	71,92,	72,93,	73,94,	
38,37,	74,95,	26,56,	75,96,	
76,97,	77,98,	79,99,	80,100,	
25,54,	81,101,	82,102,	28,59,	
37,62,	37,62,	37,62,	37,62,	
37,62,	37,62,	37,62,	37,62,	
37,62,	37,62,	38,37,	39,39,	
39,39,	39,39,	39,39,	39,39,	
39,39,	39,39,	39,39,	83,103,	
85,104,	87,105,	88,106,	15,37,	
40,63,	40,63,	40,62,	40,62,	
40,62,	40,62,	40,62,	40,62,	
40,62,	40,62,	89,107,	41,64,	
41,64,	41,64,	41,64,	41,64,	
41,64,	41,64,	41,64,	41,64,	
41,64,	15,37,	90,108,	91,109,	
92,110,	93,111,	37,37,	94,112,	
41,65,	41,65,	41,65,	41,65,	
41,65,	41,65,	95,113,	96,114,	
97,115,	39,37,	86,86,	86,86,	
86,86,	86,86,	86,86,	86,86,	
86,86,	86,86,	86,86,	86,86,	
37,37,	98,116,	40,37,	99,117,	
100,118,	101,119,	103,120,	104,121,	
106,122,	107,123,	108,124,	39,37,	
41,65,	41,65,	41,65,	41,65,	
41,65,	41,65,	109,125,	110,126,	
112,127,	113,128,	114,129,	115,130,	
40,37,	64,64,	64,64,	64,64,	
64,64,	64,64,	64,64,	64,64,	
64,64,	64,64,	64,64,	41,37,	
116,131,	117,132,	118,133,	119,134,	
121,135,	122,136,	64,86,	64,86,	
64,86,	64,86,	64,86,	64,86,	
65,64,	65,64,	65,64,	65,64,	
65,64,	65,64,	65,64,	65,64,	
65,64,	65,64,	123,137,	124,138,	
126,141,	129,142,	130,143,	131,144,	
133,145,	65,65,	65,65,	65,65,	
65,65,	65,65,	65,65,	125,139,	
134,146,	137,147,	64,86,	64,86,	
64,86,	64,86,	64,86,	64,86,	
141,148,	142,149,	143,150,	144,151,	
147,152,	151,153,	125,140,	152,154,	
154,155,	0,0,	154,156,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	65,65,	65,65,	65,65,	
65,65,	65,65,	65,65,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
65,37,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-1,	0,		0,	
yycrank+-21,	yysvec+3,	0,	
yycrank+-13,	0,		0,	
yycrank+-2,	yysvec+5,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+3,
yycrank+0,	0,		yyvstop+6,
yycrank+0,	0,		yyvstop+8,
yycrank+0,	0,		yyvstop+11,
yycrank+-17,	0,		yyvstop+14,
yycrank+90,	0,		yyvstop+17,
yycrank+166,	0,		yyvstop+20,
yycrank+241,	yysvec+14,	yyvstop+23,
yycrank+3,	yysvec+13,	yyvstop+26,
yycrank+17,	yysvec+13,	yyvstop+29,
yycrank+6,	yysvec+13,	yyvstop+32,
yycrank+9,	yysvec+13,	yyvstop+35,
yycrank+10,	yysvec+13,	yyvstop+38,
yycrank+20,	yysvec+13,	yyvstop+41,
yycrank+4,	yysvec+13,	yyvstop+44,
yycrank+2,	yysvec+13,	yyvstop+47,
yycrank+37,	yysvec+13,	yyvstop+50,
yycrank+195,	yysvec+13,	yyvstop+53,
yycrank+201,	yysvec+13,	yyvstop+56,
yycrank+7,	yysvec+13,	yyvstop+59,
yycrank+198,	yysvec+13,	yyvstop+62,
yycrank+0,	0,		yyvstop+65,
yycrank+0,	0,		yyvstop+67,
yycrank+0,	0,		yyvstop+69,
yycrank+0,	0,		yyvstop+72,
yycrank+3,	0,		yyvstop+75,
yycrank+0,	yysvec+12,	yyvstop+77,
yycrank+0,	0,		yyvstop+79,
yycrank+0,	yysvec+13,	yyvstop+81,
yycrank+268,	yysvec+14,	0,	
yycrank+206,	yysvec+14,	yyvstop+83,
yycrank+279,	yysvec+14,	yyvstop+86,
yycrank+292,	yysvec+14,	0,	
yycrank+303,	yysvec+14,	0,	
yycrank+35,	yysvec+13,	yyvstop+88,
yycrank+39,	yysvec+13,	yyvstop+90,
yycrank+42,	yysvec+13,	yyvstop+92,
yycrank+10,	yysvec+13,	yyvstop+94,
yycrank+55,	yysvec+13,	yyvstop+96,
yycrank+37,	yysvec+13,	yyvstop+98,
yycrank+39,	yysvec+13,	yyvstop+100,
yycrank+70,	yysvec+13,	yyvstop+102,
yycrank+112,	yysvec+13,	yyvstop+104,
yycrank+83,	yysvec+13,	yyvstop+106,
yycrank+67,	yysvec+13,	yyvstop+108,
yycrank+82,	yysvec+13,	yyvstop+110,
yycrank+70,	yysvec+13,	yyvstop+112,
yycrank+126,	yysvec+13,	yyvstop+114,
yycrank+116,	yysvec+13,	yyvstop+116,
yycrank+112,	yysvec+13,	yyvstop+118,
yycrank+113,	yysvec+13,	yyvstop+120,
yycrank+145,	yysvec+13,	yyvstop+122,
yycrank+0,	0,		yyvstop+124,
yycrank+0,	0,		yyvstop+126,
yycrank+35,	0,		yyvstop+128,
yycrank+23,	yysvec+62,	yyvstop+130,
yycrank+365,	0,		yyvstop+133,
yycrank+388,	yysvec+14,	yyvstop+136,
yycrank+161,	yysvec+13,	yyvstop+138,
yycrank+160,	yysvec+13,	yyvstop+140,
yycrank+148,	yysvec+13,	yyvstop+142,
yycrank+154,	yysvec+13,	yyvstop+144,
yycrank+202,	yysvec+13,	yyvstop+146,
yycrank+200,	yysvec+13,	yyvstop+148,
yycrank+201,	yysvec+13,	yyvstop+150,
yycrank+186,	yysvec+13,	yyvstop+152,
yycrank+188,	yysvec+13,	yyvstop+154,
yycrank+206,	yysvec+13,	yyvstop+156,
yycrank+207,	yysvec+13,	yyvstop+158,
yycrank+195,	yysvec+13,	yyvstop+160,
yycrank+0,	yysvec+13,	yyvstop+162,
yycrank+195,	yysvec+13,	yyvstop+165,
yycrank+199,	yysvec+13,	yyvstop+167,
yycrank+206,	yysvec+13,	yyvstop+169,
yycrank+199,	yysvec+13,	yyvstop+171,
yycrank+234,	yysvec+13,	yyvstop+173,
yycrank+0,	yysvec+13,	yyvstop+175,
yycrank+224,	yysvec+13,	yyvstop+178,
yycrank+330,	yysvec+64,	yyvstop+180,
yycrank+216,	yysvec+13,	yyvstop+182,
yycrank+231,	yysvec+13,	yyvstop+184,
yycrank+242,	yysvec+13,	yyvstop+186,
yycrank+265,	yysvec+13,	yyvstop+188,
yycrank+255,	yysvec+13,	yyvstop+190,
yycrank+250,	yysvec+13,	yyvstop+192,
yycrank+250,	yysvec+13,	yyvstop+194,
yycrank+257,	yysvec+13,	yyvstop+196,
yycrank+258,	yysvec+13,	yyvstop+198,
yycrank+261,	yysvec+13,	yyvstop+200,
yycrank+262,	yysvec+13,	yyvstop+202,
yycrank+278,	yysvec+13,	yyvstop+204,
yycrank+290,	yysvec+13,	yyvstop+206,
yycrank+275,	yysvec+13,	yyvstop+208,
yycrank+296,	yysvec+13,	yyvstop+210,
yycrank+0,	yysvec+13,	yyvstop+212,
yycrank+278,	yysvec+13,	yyvstop+215,
yycrank+287,	yysvec+13,	yyvstop+217,
yycrank+0,	yysvec+13,	yyvstop+219,
yycrank+295,	yysvec+13,	yyvstop+222,
yycrank+296,	yysvec+13,	yyvstop+225,
yycrank+284,	yysvec+13,	yyvstop+227,
yycrank+305,	yysvec+13,	yyvstop+229,
yycrank+297,	yysvec+13,	yyvstop+231,
yycrank+0,	yysvec+13,	yyvstop+233,
yycrank+308,	yysvec+13,	yyvstop+236,
yycrank+294,	yysvec+13,	yyvstop+238,
yycrank+300,	yysvec+13,	yyvstop+241,
yycrank+295,	yysvec+13,	yyvstop+243,
yycrank+325,	yysvec+13,	yyvstop+245,
yycrank+309,	yysvec+13,	yyvstop+247,
yycrank+310,	yysvec+13,	yyvstop+249,
yycrank+324,	yysvec+13,	yyvstop+251,
yycrank+0,	yysvec+13,	yyvstop+253,
yycrank+307,	yysvec+13,	yyvstop+256,
yycrank+329,	yysvec+13,	yyvstop+258,
yycrank+337,	yysvec+13,	yyvstop+260,
yycrank+346,	yysvec+13,	yyvstop+262,
yycrank+359,	yysvec+13,	yyvstop+264,
yycrank+351,	yysvec+13,	yyvstop+267,
yycrank+0,	yysvec+13,	yyvstop+269,
yycrank+0,	yysvec+13,	yyvstop+272,
yycrank+352,	yysvec+13,	yyvstop+275,
yycrank+349,	yysvec+13,	yyvstop+277,
yycrank+350,	yysvec+13,	yyvstop+280,
yycrank+0,	yysvec+13,	yyvstop+282,
yycrank+337,	yysvec+13,	yyvstop+285,
yycrank+359,	yysvec+13,	yyvstop+288,
yycrank+0,	yysvec+13,	yyvstop+290,
yycrank+0,	yysvec+13,	yyvstop+293,
yycrank+360,	yysvec+13,	yyvstop+296,
yycrank+0,	yysvec+13,	yyvstop+298,
yycrank+0,	yysvec+13,	yyvstop+301,
yycrank+0,	yysvec+13,	yyvstop+304,
yycrank+360,	yysvec+13,	yyvstop+307,
yycrank+361,	yysvec+13,	yyvstop+309,
yycrank+370,	yysvec+13,	yyvstop+311,
yycrank+363,	yysvec+13,	yyvstop+313,
yycrank+0,	yysvec+13,	yyvstop+315,
yycrank+0,	yysvec+13,	yyvstop+318,
yycrank+362,	yysvec+13,	yyvstop+321,
yycrank+0,	yysvec+13,	yyvstop+323,
yycrank+0,	yysvec+13,	yyvstop+326,
yycrank+0,	yysvec+13,	yyvstop+329,
yycrank+365,	yysvec+13,	yyvstop+333,
yycrank+359,	yysvec+13,	yyvstop+335,
yycrank+0,	yysvec+13,	yyvstop+337,
yycrank+433,	yysvec+13,	yyvstop+340,
yycrank+0,	0,		yyvstop+342,
yycrank+0,	0,		yyvstop+344,
0,	0,	0};
struct yywork *yytop = yycrank+508;
struct yysvf *yybgin = yysvec+1;
Uchar yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,'/' ,
'0' ,'0' ,'2' ,'2' ,'2' ,'2' ,'2' ,'2' ,
'8' ,'8' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'/' ,
'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,
'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,
'/' ,'/' ,'/' ,01  ,01  ,01  ,01  ,'/' ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'/' ,
'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,
'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,'/' ,
'/' ,'/' ,'/' ,01  ,01  ,01  ,01  ,01  ,
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
0,0,0,0,1,0,0,0,
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
yyoutput(int c)
{
	output(c);
}
yyunput(int c)
{
	unput(c);
}
