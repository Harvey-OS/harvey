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
#define Display 2
#define Math 4
#define Normal 6
#define Tag 8
#define YYNEWLINE 10
yylex(void){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
{yyleng--; yymore(); /* ignore apostrophes */}
break;
case 2:
{yyleng-=2; yymore(); /* ignore hyphens */}
break;
case 3:
;
break;
case 4:
;
break;
case 5:
{printf("%s\n",yytext); /* any other letter seq is a word */}
break;
case 6:
;
break;
case 7:
;
break;
case 8:
BEGIN Tag;
break;
case 9:
BEGIN Tag;
break;
case 10:
BEGIN Tag;
break;
case 11:
BEGIN Tag;
break;
case 12:
BEGIN Tag;
break;
case 13:
	BEGIN Tag;
break;
case 14:
BEGIN Tag;
break;
case 15:
BEGIN Tag;
break;
case 16:
BEGIN Tag;
break;
case 17:
BEGIN Tag;
break;
case 18:
BEGIN Tag;
break;
case 19:
	BEGIN Tag;
break;
case 20:
	;
break;
case 21:
	BEGIN Normal;
break;
case 22:
;
break;
case 23:
	BEGIN Math;
break;
case 24:
	BEGIN Normal;
break;
case 25:
;
break;
case 26:
	BEGIN Display;
break;
case 27:
;
break;
case 28:
BEGIN Normal;
break;
case 29:
;
break;
case 30:
;
break;
case 31:
;
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
main(int argc, char **argv)
{
	int i;
	
	BEGIN Normal; /* Starts yylex off in the right state */
	if (argc==1) {
	    yyin = stdin;
	    yylex();
	    }
	else for (i=1; i<argc; i++) {
	    yyin = fopen(argv[i],"r");
	    if (yyin==NULL) {
		fprintf(stderr,"can't open %s\n",argv[i]);
		exit(1);
		}
	    yylex();
	    }
	exit(0);
}
yywrap(void)
{
	return 1;
}
int yyvstop[] = {
0,

27,
0,

27,
0,

25,
0,

25,
0,

31,
0,

6,
31,
0,

1,
31,
0,

22,
31,
0,

4,
31,
0,

31,
0,

31,
-3,
0,

20,
0,

21,
0,

28,
0,

24,
0,

6,
0,

22,
0,

4,
5,
0,

5,
0,

29,
0,

30,
0,

23,
29,
0,

7,
29,
0,

26,
29,
0,

7,
29,
0,

7,
29,
0,

7,
29,
0,

7,
29,
0,

7,
29,
0,

7,
29,
0,

7,
29,
0,

3,
0,

3,
0,

3,
0,

5,
0,

2,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

13,
0,

7,
0,

7,
0,

7,
0,

7,
0,

19,
0,

7,
0,

7,
0,

7,
0,

7,
0,

12,
0,

7,
0,

7,
0,

7,
0,

7,
0,

8,
0,

7,
0,

7,
0,

7,
0,

7,
0,

16,
0,

17,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

9,
0,

7,
0,

7,
0,

7,
0,

14,
0,

18,
0,

7,
0,

11,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

7,
0,

15,
0,

10,
0,
0};
# define YYTYPE Uchar
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	3,11,	0,0,	
0,0,	5,13,	0,0,	0,0,	
0,0,	0,0,	0,0,	3,11,	
0,0,	0,0,	5,13,	7,15,	
26,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	9,22,	
7,15,	12,11,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
9,22,	0,0,	12,11,	0,0,	
0,0,	3,0,	14,13,	4,0,	
5,13,	0,0,	3,11,	0,0,	
0,0,	5,13,	0,0,	14,13,	
0,0,	3,11,	7,15,	7,16,	
5,13,	7,17,	8,16,	7,15,	
8,17,	30,48,	9,22,	0,0,	
12,11,	0,0,	7,18,	9,22,	
0,0,	12,11,	3,11,	0,0,	
0,0,	5,13,	9,22,	0,0,	
12,11,	14,13,	0,0,	0,0,	
0,0,	0,0,	14,25,	7,19,	
0,0,	0,0,	0,0,	0,0,	
0,0,	14,13,	0,0,	9,22,	
0,0,	12,11,	0,0,	0,0,	
0,0,	3,12,	3,11,	4,12,	
5,14,	5,13,	3,11,	6,14,	
0,0,	5,13,	14,13,	16,26,	
41,55,	42,56,	7,20,	7,15,	
43,57,	8,20,	37,50,	7,21,	
16,0,	38,52,	37,51,	9,22,	
39,53,	12,24,	40,54,	9,22,	
50,58,	12,11,	51,59,	52,60,	
53,61,	55,64,	3,11,	54,62,	
56,65,	5,13,	14,13,	57,66,	
58,67,	60,71,	14,13,	61,72,	
62,73,	63,74,	16,26,	7,15,	
54,63,	64,75,	65,76,	16,26,	
66,77,	67,78,	68,79,	9,23,	
69,80,	12,11,	16,26,	18,27,	
18,27,	18,27,	18,27,	18,27,	
18,27,	18,27,	18,27,	18,27,	
18,27,	59,68,	14,13,	70,81,	
59,69,	71,82,	73,83,	16,26,	
74,84,	75,85,	76,86,	59,70,	
78,87,	79,88,	80,89,	19,28,	
81,90,	83,91,	84,92,	85,93,	
86,94,	88,95,	89,96,	90,97,	
91,98,	94,99,	95,100,	96,101,	
97,102,	99,105,	101,106,	102,107,	
103,108,	106,109,	108,110,	16,26,	
109,111,	110,112,	111,113,	16,26,	
112,114,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	19,29,	
19,29,	19,29,	19,29,	16,26,	
19,30,	113,115,	0,0,	0,0,	
0,0,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	19,31,	
19,31,	19,31,	19,31,	20,32,	
98,103,	0,0,	21,44,	0,0,	
0,0,	0,0,	0,0,	0,0,	
20,33,	0,0,	0,0,	21,44,	
98,104,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	20,32,	0,0,	
0,0,	21,44,	20,34,	20,32,	
21,45,	0,0,	21,44,	0,0,	
0,0,	0,0,	20,32,	0,0,	
0,0,	21,44,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	20,35,	
0,0,	0,0,	21,31,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	20,36,	0,0,	20,32,	
0,0,	21,46,	21,44,	20,35,	
20,37,	20,38,	21,31,	20,39,	
0,0,	28,28,	0,0,	20,40,	
0,0,	0,0,	20,41,	0,0,	
0,0,	0,0,	20,42,	0,0,	
20,43,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	20,32,	
0,0,	0,0,	21,44,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	28,47,	28,47,	28,47,	
28,47,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	31,31,	
31,31,	31,31,	31,31,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	35,49,	35,49,	35,49,	
35,49,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-1,	0,		0,	
yycrank+-3,	yysvec+3,	0,	
yycrank+-4,	0,		0,	
yycrank+-7,	yysvec+5,	0,	
yycrank+-14,	0,		0,	
yycrank+-17,	yysvec+7,	0,	
yycrank+-22,	0,		0,	
yycrank+0,	yysvec+9,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+-24,	0,		yyvstop+3,
yycrank+0,	0,		yyvstop+5,
yycrank+-37,	0,		yyvstop+7,
yycrank+0,	0,		yyvstop+9,
yycrank+-102,	0,		yyvstop+11,
yycrank+0,	0,		yyvstop+14,
yycrank+103,	0,		yyvstop+17,
yycrank+136,	0,		yyvstop+20,
yycrank+-258,	0,		yyvstop+23,
yycrank+-261,	0,		yyvstop+25,
yycrank+0,	0,		yyvstop+28,
yycrank+0,	0,		yyvstop+30,
yycrank+0,	0,		yyvstop+32,
yycrank+0,	0,		yyvstop+34,
yycrank+-6,	yysvec+16,	yyvstop+36,
yycrank+0,	yysvec+18,	yyvstop+38,
yycrank+322,	0,		0,	
yycrank+0,	yysvec+19,	yyvstop+40,
yycrank+12,	0,		0,	
yycrank+380,	yysvec+19,	yyvstop+43,
yycrank+0,	0,		yyvstop+45,
yycrank+0,	0,		yyvstop+47,
yycrank+0,	0,		yyvstop+49,
yycrank+406,	0,		yyvstop+52,
yycrank+0,	0,		yyvstop+55,
yycrank+9,	yysvec+35,	yyvstop+58,
yycrank+8,	yysvec+35,	yyvstop+61,
yycrank+6,	yysvec+35,	yyvstop+64,
yycrank+8,	yysvec+35,	yyvstop+67,
yycrank+7,	yysvec+35,	yyvstop+70,
yycrank+8,	yysvec+35,	yyvstop+73,
yycrank+7,	yysvec+35,	yyvstop+76,
yycrank+0,	0,		yyvstop+79,
yycrank+0,	yysvec+28,	yyvstop+81,
yycrank+0,	yysvec+30,	yyvstop+83,
yycrank+0,	yysvec+28,	yyvstop+85,
yycrank+0,	0,		yyvstop+87,
yycrank+0,	yysvec+35,	yyvstop+89,
yycrank+17,	yysvec+35,	yyvstop+91,
yycrank+24,	yysvec+35,	yyvstop+93,
yycrank+7,	yysvec+35,	yyvstop+95,
yycrank+24,	yysvec+35,	yyvstop+97,
yycrank+28,	yysvec+35,	yyvstop+99,
yycrank+27,	yysvec+35,	yyvstop+101,
yycrank+25,	yysvec+35,	yyvstop+103,
yycrank+29,	yysvec+35,	yyvstop+105,
yycrank+27,	yysvec+35,	yyvstop+107,
yycrank+56,	yysvec+35,	yyvstop+109,
yycrank+32,	yysvec+35,	yyvstop+111,
yycrank+12,	yysvec+35,	yyvstop+113,
yycrank+28,	yysvec+35,	yyvstop+115,
yycrank+20,	yysvec+35,	yyvstop+117,
yycrank+40,	yysvec+35,	yyvstop+119,
yycrank+41,	yysvec+35,	yyvstop+121,
yycrank+21,	yysvec+35,	yyvstop+123,
yycrank+35,	yysvec+35,	yyvstop+125,
yycrank+30,	yysvec+35,	yyvstop+127,
yycrank+43,	yysvec+35,	yyvstop+129,
yycrank+47,	yysvec+35,	yyvstop+131,
yycrank+42,	yysvec+35,	yyvstop+133,
yycrank+0,	0,		yyvstop+135,
yycrank+49,	yysvec+35,	yyvstop+137,
yycrank+52,	yysvec+35,	yyvstop+139,
yycrank+61,	yysvec+35,	yyvstop+141,
yycrank+56,	yysvec+35,	yyvstop+143,
yycrank+0,	0,		yyvstop+145,
yycrank+49,	yysvec+35,	yyvstop+147,
yycrank+72,	yysvec+35,	yyvstop+149,
yycrank+63,	yysvec+35,	yyvstop+151,
yycrank+55,	yysvec+35,	yyvstop+153,
yycrank+0,	0,		yyvstop+155,
yycrank+77,	yysvec+35,	yyvstop+157,
yycrank+55,	yysvec+35,	yyvstop+159,
yycrank+56,	yysvec+35,	yyvstop+161,
yycrank+79,	yysvec+35,	yyvstop+163,
yycrank+0,	0,		yyvstop+165,
yycrank+72,	yysvec+35,	yyvstop+167,
yycrank+79,	yysvec+35,	yyvstop+169,
yycrank+75,	yysvec+35,	yyvstop+171,
yycrank+83,	yysvec+35,	yyvstop+173,
yycrank+0,	0,		yyvstop+175,
yycrank+0,	0,		yyvstop+177,
yycrank+83,	yysvec+35,	yyvstop+179,
yycrank+63,	yysvec+35,	yyvstop+181,
yycrank+73,	yysvec+35,	yyvstop+183,
yycrank+87,	yysvec+35,	yyvstop+185,
yycrank+149,	yysvec+35,	yyvstop+187,
yycrank+66,	yysvec+35,	yyvstop+189,
yycrank+0,	0,		yyvstop+191,
yycrank+93,	yysvec+35,	yyvstop+193,
yycrank+68,	yysvec+35,	yyvstop+195,
yycrank+82,	yysvec+35,	yyvstop+197,
yycrank+0,	0,		yyvstop+199,
yycrank+0,	0,		yyvstop+201,
yycrank+81,	yysvec+35,	yyvstop+203,
yycrank+0,	0,		yyvstop+205,
yycrank+86,	yysvec+35,	yyvstop+207,
yycrank+92,	yysvec+35,	yyvstop+209,
yycrank+76,	yysvec+35,	yyvstop+211,
yycrank+77,	yysvec+35,	yyvstop+213,
yycrank+77,	yysvec+35,	yyvstop+215,
yycrank+106,	yysvec+35,	yyvstop+217,
yycrank+0,	0,		yyvstop+219,
yycrank+0,	0,		yyvstop+221,
0,	0,	0};
struct yywork *yytop = yycrank+528;
struct yysvf *yybgin = yysvec+1;
Uchar yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,'$' ,01  ,01  ,01  ,
01  ,')' ,01  ,01  ,01  ,01  ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,']' ,01  ,01  ,
01  ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,
'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,
'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,
'a' ,'a' ,'a' ,01  ,01  ,'}' ,01  ,01  ,
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
0,0,0,1,0,0,0,0,
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
