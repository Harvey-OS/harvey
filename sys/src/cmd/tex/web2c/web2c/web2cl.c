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
#include "web2c.h"
#include "web2cy.h"
#ifdef	ANSI
int yylex(void);
#ifndef unput
static void unput(char);
#endif
#ifndef input
static int input(void);
#endif
#endif
char conditional[20], negbuf[2], temp[20];
extern boolean doing_statements;
int yywrap(void) { return 1; }
#define YYNEWLINE 10
yylex(void){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
			;
break;
case 2:
			{while (input() != '}');
				}
break;
case 3:
			{
				    register int c;
				    (void) putc('#', std);
				    while ((c = input()) && c != ';')
					(void) putc(c, std);
				    (void) putc('\n', std);
				}
break;
case 4:
		{register int c;
				 extern char my_routine[];
				 register char *cp=conditional;
				 new_line();
				 (void) input();
				 while ((c = input()) != '\'')
#ifdef	MS_DOS
				    *cp++ = (char) c;
#else
				    *cp++ = c;
#endif
				 *cp = '\0';
				 (void) input();
				 if (doing_statements) fputs("\t;\n", std);
				 (void) fprintf(std,
					"#ifdef %s\n", conditional);
				}
break;
case 5:
		{register int c;
				 new_line();
				 fputs("#endif /* ", std);
				 (void) input();
				 while ((c = input()) != '\'')
					(void) putc(c, std);
				 (void) input();
				 conditional[0] = '\0';
				 fputs(" */\n", std);
				}
break;
case 6:
;
break;
case 7:
;
break;
case 8:
		return(last_tok=define_tok);
break;
case 9:
		return(last_tok=field_tok);
break;
case 10:
			return(last_tok=and_tok) ;
break;
case 11:
			return(last_tok=array_tok) ;
break;
case 12:
			return(last_tok=begin_tok) ;
break;
case 13:
			return(last_tok=case_tok) ;
break;
case 14:
			return(last_tok=const_tok) ;
break;
case 15:
			return(last_tok=div_tok) ;
break;
case 16:
			return(last_tok=break_tok);
break;
case 17:
			return(last_tok=do_tok) ;
break;
case 18:
		return(last_tok=downto_tok) ;
break;
case 19:
			return(last_tok=else_tok) ;
break;
case 20:
			return(last_tok=end_tok) ;
break;
case 21:
			return(last_tok=file_tok) ;
break;
case 22:
			return(last_tok=for_tok) ;
break;
case 23:
		return(last_tok=function_tok) ;
break;
case 24:
			return(last_tok=goto_tok) ;
break;
case 25:
			return(last_tok=if_tok) ;
break;
case 26:
			return(last_tok=label_tok) ;
break;
case 27:
			return(last_tok=mod_tok) ;
break;
case 28:
			return(last_tok=not_tok) ;
break;
case 29:
			return(last_tok=of_tok) ;
break;
case 30:
			return(last_tok=or_tok) ;
break;
case 31:
		return(last_tok=procedure_tok) ;
break;
case 32:
		return(last_tok=program_tok) ;
break;
case 33:
		return(last_tok=record_tok) ;
break;
case 34:
		return(last_tok=repeat_tok) ;
break;
case 35:
			return(last_tok=hhb0_tok) ;
break;
case 36:
			return(last_tok=hhb1_tok) ;
break;
case 37:
			return(last_tok=then_tok) ;
break;
case 38:
			return(last_tok=to_tok) ;
break;
case 39:
			return(last_tok=type_tok) ;
break;
case 40:
			return(last_tok=until_tok) ;
break;
case 41:
			return(last_tok=var_tok) ;
break;
case 42:
			return(last_tok=while_tok) ;
break;
case 43:
		return(last_tok=others_tok) ;
break;
case 44:
			{		
				  (void) sprintf(temp, "%s%s", negbuf, yytext);
				  negbuf[0] = '\0';
				  return(last_tok=r_num_tok) ;
				}
break;
case 45:
		{
				  (void) sprintf(temp, "%s%s", negbuf, yytext);
				  negbuf[0] = '\0';
				  return(last_tok=i_num_tok) ;
				}
break;
case 46:
	return(last_tok=single_char_tok) ;
break;
case 47:
	return(last_tok=string_literal_tok) ;
break;
case 48:
			{ if ((last_tok>=undef_id_tok &&
				      last_tok<=field_id_tok) ||
				      last_tok==i_num_tok ||
				      last_tok==r_num_tok ||
				      last_tok==')' ||
				      last_tok==']')
				   return(last_tok='+') ;
				else return(last_tok=unary_plus_tok) ; }
break;
case 49:
			{ if ((last_tok>=undef_id_tok &&
				      last_tok<=field_id_tok) ||
				      last_tok==i_num_tok ||
				      last_tok==r_num_tok ||
				      last_tok==')' ||
				      last_tok==']')
				   return(last_tok='-') ;
				else {
				  int c;
				  while ((c = input()) == ' ' || c == '\t');
#ifdef	MS_DOS
				  unput((char) c);
#else
				  unput(c);
#endif
				  if (c < '0' || c > '9') {
					return(last_tok = unary_minus_tok);
				  }
				  negbuf[0] = '-';
				}}
break;
case 50:
			return(last_tok='*') ;
break;
case 51:
			return(last_tok='/') ;
break;
case 52:
			return(last_tok='=') ;
break;
case 53:
			return(last_tok=not_eq_tok) ;
break;
case 54:
			return(last_tok='<') ;
break;
case 55:
			return(last_tok='>') ;
break;
case 56:
			return(last_tok=less_eq_tok) ;
break;
case 57:
			return(last_tok=great_eq_tok) ;
break;
case 58:
			return(last_tok='(') ;
break;
case 59:
			return(last_tok=')') ;
break;
case 60:
			return(last_tok='[') ;
break;
case 61:
			return(last_tok=']') ;
break;
case 62:
			return(last_tok=assign_tok) ;
break;
case 63:
			return(last_tok=two_dots_tok) ;
break;
case 64:
			return(last_tok='.') ;
break;
case 65:
			return(last_tok=',') ;
break;
case 66:
			return(last_tok=';') ;
break;
case 67:
			return(last_tok=':') ;
break;
case 68:
			return(last_tok='^') ;
break;
case 69:
		{ (void) strcpy(last_id,yytext) ; 
				  l_s=search_table(last_id) ;
				  if (l_s == -1) return(last_tok=undef_id_tok);
				  return(last_tok=sym_table[l_s].typ);
				}
break;
case 70:
			return(last_tok=unknown_tok) ;
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
int yyvstop[] = {
0,

70,
0,

1,
70,
0,

1,
0,

3,
70,
0,

70,
0,

58,
70,
0,

59,
70,
0,

50,
70,
0,

48,
70,
0,

65,
70,
0,

49,
70,
0,

64,
70,
0,

51,
70,
0,

45,
70,
0,

67,
70,
0,

66,
70,
0,

54,
70,
0,

52,
70,
0,

55,
70,
0,

70,
0,

69,
70,
0,

60,
70,
0,

61,
70,
0,

68,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

69,
70,
0,

2,
70,
0,

47,
0,

63,
0,

45,
0,

62,
0,

56,
0,

53,
0,

57,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

17,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

25,
69,
0,

69,
0,

69,
0,

69,
0,

29,
69,
0,

30,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

38,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

46,
47,
0,

44,
0,

44,
0,

10,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

15,
69,
0,

69,
0,

69,
0,

20,
69,
0,

69,
0,

22,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

27,
69,
0,

28,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

41,
69,
0,

69,
0,

47,
0,

69,
0,

69,
0,

69,
0,

13,
69,
0,

69,
0,

69,
0,

19,
69,
0,

69,
0,

21,
69,
0,

69,
0,

24,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

37,
69,
0,

39,
69,
0,

69,
0,

69,
0,

44,
0,

11,
69,
0,

12,
69,
0,

16,
69,
0,

14,
69,
0,

69,
0,

69,
0,

69,
0,

35,
0,

36,
0,

69,
0,

69,
0,

26,
69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

69,
0,

40,
69,
0,

42,
69,
0,

9,
0,

18,
69,
0,

5,
0,

69,
0,

69,
0,

4,
0,

43,
69,
0,

69,
0,

69,
0,

69,
0,

33,
69,
0,

34,
69,
0,

8,
0,

69,
0,

69,
0,

69,
0,

32,
69,
0,

23,
69,
0,

69,
0,

69,
0,

31,
69,
0,

7,
0,

6,
0,
0};
# define YYTYPE Uchar
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,4,	1,5,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	4,5,	
4,5,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
1,6,	0,0,	0,0,	0,0,	
1,7,	1,8,	1,9,	1,10,	
1,11,	1,12,	1,13,	1,14,	
1,15,	1,16,	4,5,	14,50,	
49,48,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	1,17,	
1,18,	1,19,	1,20,	1,21,	
17,54,	1,22,	1,23,	19,55,	
19,56,	21,57,	0,0,	0,0,	
16,51,	0,0,	16,52,	16,52,	
16,52,	16,52,	16,52,	16,52,	
16,52,	16,52,	16,52,	16,52,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	93,134,	94,93,	
1,24,	0,0,	1,25,	1,26,	
0,0,	0,0,	1,27,	1,28,	
1,29,	1,30,	1,31,	1,32,	
1,33,	1,34,	1,35,	1,23,	
1,23,	1,36,	1,37,	1,38,	
1,39,	1,40,	1,23,	1,41,	
1,23,	1,42,	1,43,	1,44,	
1,45,	1,23,	1,23,	1,23,	
1,46,	2,6,	1,3,	16,53,	
27,61,	22,58,	4,47,	22,59,	
27,62,	2,11,	2,12,	2,13,	
2,14,	2,15,	30,67,	33,74,	
28,63,	29,65,	34,75,	31,69,	
30,68,	31,70,	35,76,	36,77,	
2,17,	2,18,	2,19,	2,20,	
2,21,	28,64,	2,22,	29,66,	
32,71,	37,78,	38,79,	39,80,	
41,85,	40,83,	32,72,	42,86,	
43,89,	44,90,	45,91,	47,92,	
32,73,	58,98,	42,87,	39,81,	
59,99,	39,82,	61,100,	62,101,	
63,102,	7,48,	40,84,	64,103,	
42,88,	2,24,	65,104,	2,25,	
2,26,	7,48,	7,48,	66,105,	
2,28,	2,29,	2,30,	2,31,	
2,32,	2,33,	2,34,	2,35,	
2,23,	2,23,	2,36,	2,37,	
2,38,	2,39,	2,40,	2,23,	
2,41,	2,23,	2,42,	2,43,	
2,44,	2,45,	2,23,	2,23,	
2,23,	2,46,	67,106,	7,49,	
7,48,	7,48,	7,48,	68,107,	
69,108,	70,109,	71,110,	72,111,	
7,48,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	73,112,	
74,113,	76,119,	77,120,	78,121,	
79,122,	7,48,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
82,123,	83,124,	84,125,	86,128,	
88,129,	7,48,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
23,60,	23,60,	23,60,	23,60,	
48,93,	7,48,	89,130,	90,131,	
91,132,	92,133,	95,135,	98,136,	
48,93,	48,93,	51,95,	51,95,	
51,95,	51,95,	51,95,	51,95,	
51,95,	51,95,	51,95,	51,95,	
53,96,	99,137,	53,96,	75,114,	
75,114,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	85,126,	
101,138,	102,139,	48,94,	48,93,	
48,93,	48,93,	103,140,	104,141,	
105,142,	107,143,	75,114,	48,93,	
85,127,	108,144,	109,145,	110,146,	
112,147,	113,148,	75,115,	114,114,	
114,114,	115,150,	116,116,	116,116,	
75,116,	117,155,	119,156,	120,157,	
48,93,	96,97,	96,97,	96,97,	
96,97,	96,97,	96,97,	96,97,	
96,97,	96,97,	96,97,	118,118,	
123,158,	124,159,	114,114,	126,162,	
127,163,	116,116,	128,164,	118,118,	
118,118,	129,165,	114,115,	125,160,	
130,166,	116,151,	132,167,	125,161,	
114,116,	133,168,	136,171,	135,169,	
48,93,	135,169,	137,172,	138,173,	
135,170,	135,170,	135,170,	135,170,	
135,170,	135,170,	135,170,	135,170,	
135,170,	135,170,	139,174,	140,175,	
142,176,	118,118,	118,118,	118,118,	
118,118,	143,177,	145,178,	147,179,	
149,180,	151,182,	118,118,	153,185,	
48,93,	155,186,	75,117,	150,150,	
152,183,	152,184,	156,187,	157,188,	
158,189,	159,190,	160,191,	150,150,	
150,150,	75,118,	161,192,	118,118,	
154,154,	162,193,	163,194,	166,195,	
167,196,	168,197,	171,198,	116,152,	
154,154,	154,154,	172,199,	177,200,	
178,201,	179,202,	180,203,	181,114,	
181,150,	185,205,	114,149,	186,206,	
187,207,	116,153,	189,208,	190,209,	
191,210,	150,150,	150,150,	150,150,	
150,181,	114,118,	192,211,	118,118,	
116,154,	193,212,	150,150,	194,213,	
197,214,	182,182,	154,154,	154,154,	
154,154,	154,154,	198,215,	202,216,	
203,217,	182,182,	182,182,	154,154,	
204,116,	204,182,	205,218,	150,150,	
206,219,	209,5,	210,220,	211,221,	
214,5,	216,222,	217,223,	118,114,	
218,224,	219,225,	220,226,	222,227,	
154,154,	169,170,	169,170,	169,170,	
169,170,	169,170,	169,170,	169,170,	
169,170,	169,170,	169,170,	182,182,	
182,182,	182,182,	182,204,	223,228,	
224,229,	225,114,	226,230,	150,150,	
182,182,	228,114,	229,116,	230,232,	
231,233,	234,236,	235,237,	237,239,	
238,240,	239,241,	240,242,	241,243,	
154,154,	233,233,	233,233,	236,236,	
236,236,	182,182,	242,244,	243,245,	
244,246,	227,231,	227,231,	245,247,	
246,248,	227,231,	247,249,	150,150,	
248,250,	250,251,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
233,233,	0,0,	236,236,	227,231,	
154,116,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	182,182,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
232,234,	232,234,	232,234,	232,234,	
0,0,	182,182,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
227,231,	227,231,	227,231,	227,231,	
0,0,	0,0,	233,235,	0,0,	
236,238,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+-1,	0,		0,	
yycrank+-90,	yysvec+1,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+18,	0,		yyvstop+3,
yycrank+0,	yysvec+4,	yyvstop+6,
yycrank+0,	0,		yyvstop+8,
yycrank+-176,	0,		yyvstop+11,
yycrank+0,	0,		yyvstop+13,
yycrank+0,	0,		yyvstop+16,
yycrank+0,	0,		yyvstop+19,
yycrank+0,	0,		yyvstop+22,
yycrank+0,	0,		yyvstop+25,
yycrank+0,	0,		yyvstop+28,
yycrank+5,	0,		yyvstop+31,
yycrank+0,	0,		yyvstop+34,
yycrank+26,	0,		yyvstop+37,
yycrank+3,	0,		yyvstop+40,
yycrank+0,	0,		yyvstop+43,
yycrank+6,	0,		yyvstop+46,
yycrank+0,	0,		yyvstop+49,
yycrank+8,	0,		yyvstop+52,
yycrank+29,	0,		yyvstop+55,
yycrank+177,	0,		yyvstop+57,
yycrank+0,	0,		yyvstop+60,
yycrank+0,	0,		yyvstop+63,
yycrank+0,	0,		yyvstop+66,
yycrank+18,	yysvec+23,	yyvstop+69,
yycrank+39,	yysvec+23,	yyvstop+72,
yycrank+44,	yysvec+23,	yyvstop+75,
yycrank+33,	yysvec+23,	yyvstop+78,
yycrank+35,	yysvec+23,	yyvstop+81,
yycrank+51,	yysvec+23,	yyvstop+84,
yycrank+28,	yysvec+23,	yyvstop+87,
yycrank+38,	yysvec+23,	yyvstop+90,
yycrank+44,	yysvec+23,	yyvstop+93,
yycrank+50,	yysvec+23,	yyvstop+96,
yycrank+46,	yysvec+23,	yyvstop+99,
yycrank+47,	yysvec+23,	yyvstop+102,
yycrank+57,	yysvec+23,	yyvstop+105,
yycrank+64,	yysvec+23,	yyvstop+108,
yycrank+59,	yysvec+23,	yyvstop+111,
yycrank+59,	yysvec+23,	yyvstop+114,
yycrank+54,	yysvec+23,	yyvstop+117,
yycrank+68,	yysvec+23,	yyvstop+120,
yycrank+62,	yysvec+23,	yyvstop+123,
yycrank+0,	0,		yyvstop+126,
yycrank+70,	0,		0,	
yycrank+-299,	0,		0,	
yycrank+13,	0,		yyvstop+129,
yycrank+0,	0,		yyvstop+131,
yycrank+262,	0,		0,	
yycrank+0,	yysvec+16,	yyvstop+133,
yycrank+277,	0,		0,	
yycrank+0,	0,		yyvstop+135,
yycrank+0,	0,		yyvstop+137,
yycrank+0,	0,		yyvstop+139,
yycrank+0,	0,		yyvstop+141,
yycrank+68,	0,		0,	
yycrank+67,	0,		0,	
yycrank+0,	yysvec+23,	yyvstop+143,
yycrank+74,	yysvec+23,	yyvstop+145,
yycrank+61,	yysvec+23,	yyvstop+147,
yycrank+73,	yysvec+23,	yyvstop+149,
yycrank+78,	yysvec+23,	yyvstop+151,
yycrank+67,	yysvec+23,	yyvstop+153,
yycrank+77,	yysvec+23,	yyvstop+155,
yycrank+96,	yysvec+23,	yyvstop+157,
yycrank+100,	yysvec+23,	yyvstop+159,
yycrank+105,	yysvec+23,	yyvstop+162,
yycrank+121,	yysvec+23,	yyvstop+164,
yycrank+114,	yysvec+23,	yyvstop+166,
yycrank+109,	yysvec+23,	yyvstop+168,
yycrank+125,	yysvec+23,	yyvstop+170,
yycrank+120,	yysvec+23,	yyvstop+172,
yycrank+314,	yysvec+23,	yyvstop+174,
yycrank+137,	yysvec+23,	yyvstop+176,
yycrank+140,	yysvec+23,	yyvstop+179,
yycrank+139,	yysvec+23,	yyvstop+181,
yycrank+124,	yysvec+23,	yyvstop+183,
yycrank+0,	yysvec+23,	yyvstop+185,
yycrank+0,	yysvec+23,	yyvstop+188,
yycrank+164,	yysvec+23,	yyvstop+191,
yycrank+170,	yysvec+23,	yyvstop+193,
yycrank+159,	yysvec+23,	yyvstop+195,
yycrank+236,	yysvec+23,	yyvstop+197,
yycrank+170,	yysvec+23,	yyvstop+199,
yycrank+0,	yysvec+23,	yyvstop+201,
yycrank+160,	yysvec+23,	yyvstop+204,
yycrank+186,	yysvec+23,	yyvstop+206,
yycrank+189,	yysvec+23,	yyvstop+208,
yycrank+199,	yysvec+23,	yyvstop+210,
yycrank+206,	0,		0,	
yycrank+-51,	yysvec+48,	0,	
yycrank+52,	0,		yyvstop+212,
yycrank+205,	yysvec+51,	yyvstop+215,
yycrank+317,	0,		0,	
yycrank+0,	yysvec+96,	yyvstop+217,
yycrank+205,	0,		0,	
yycrank+220,	0,		0,	
yycrank+0,	yysvec+23,	yyvstop+219,
yycrank+239,	yysvec+23,	yyvstop+222,
yycrank+232,	yysvec+23,	yyvstop+224,
yycrank+245,	yysvec+23,	yyvstop+226,
yycrank+242,	yysvec+23,	yyvstop+228,
yycrank+229,	yysvec+23,	yyvstop+230,
yycrank+0,	yysvec+23,	yyvstop+232,
yycrank+235,	yysvec+23,	yyvstop+235,
yycrank+248,	yysvec+23,	yyvstop+237,
yycrank+245,	yysvec+23,	yyvstop+239,
yycrank+250,	yysvec+23,	yyvstop+242,
yycrank+0,	yysvec+23,	yyvstop+244,
yycrank+253,	yysvec+23,	yyvstop+247,
yycrank+242,	yysvec+23,	yyvstop+249,
yycrank+346,	0,		0,	
yycrank+315,	0,		0,	
yycrank+349,	0,		0,	
yycrank+264,	yysvec+23,	yyvstop+251,
yycrank+-374,	0,		0,	
yycrank+261,	yysvec+23,	yyvstop+253,
yycrank+262,	yysvec+23,	yyvstop+255,
yycrank+0,	yysvec+23,	yyvstop+257,
yycrank+0,	yysvec+23,	yyvstop+260,
yycrank+275,	yysvec+23,	yyvstop+263,
yycrank+270,	yysvec+23,	yyvstop+265,
yycrank+288,	yysvec+23,	yyvstop+267,
yycrank+268,	yysvec+23,	yyvstop+269,
yycrank+279,	yysvec+23,	yyvstop+271,
yycrank+272,	yysvec+23,	yyvstop+273,
yycrank+284,	yysvec+23,	yyvstop+275,
yycrank+283,	yysvec+23,	yyvstop+277,
yycrank+0,	yysvec+23,	yyvstop+279,
yycrank+282,	yysvec+23,	yyvstop+282,
yycrank+286,	0,		0,	
yycrank+0,	yysvec+94,	yyvstop+284,
yycrank+352,	0,		0,	
yycrank+289,	0,		0,	
yycrank+290,	0,		0,	
yycrank+278,	yysvec+23,	yyvstop+286,
yycrank+300,	yysvec+23,	yyvstop+288,
yycrank+304,	yysvec+23,	yyvstop+290,
yycrank+0,	yysvec+23,	yyvstop+292,
yycrank+296,	yysvec+23,	yyvstop+295,
yycrank+301,	yysvec+23,	yyvstop+297,
yycrank+0,	yysvec+23,	yyvstop+299,
yycrank+316,	yysvec+23,	yyvstop+302,
yycrank+0,	yysvec+23,	yyvstop+304,
yycrank+303,	yysvec+23,	yyvstop+307,
yycrank+0,	yysvec+23,	yyvstop+309,
yycrank+323,	0,		0,	
yycrank+-426,	0,		0,	
yycrank+379,	0,		0,	
yycrank+380,	0,		0,	
yycrank+326,	0,		0,	
yycrank+-439,	0,		0,	
yycrank+326,	yysvec+23,	yyvstop+312,
yycrank+328,	yysvec+23,	yyvstop+314,
yycrank+323,	yysvec+23,	yyvstop+316,
yycrank+318,	yysvec+23,	yyvstop+318,
yycrank+332,	yysvec+23,	yyvstop+320,
yycrank+333,	yysvec+23,	yyvstop+322,
yycrank+324,	yysvec+23,	yyvstop+324,
yycrank+327,	yysvec+23,	yyvstop+326,
yycrank+345,	yysvec+23,	yyvstop+328,
yycrank+0,	yysvec+23,	yyvstop+330,
yycrank+0,	yysvec+23,	yyvstop+333,
yycrank+335,	yysvec+23,	yyvstop+336,
yycrank+343,	yysvec+23,	yyvstop+338,
yycrank+344,	0,		0,	
yycrank+457,	0,		0,	
yycrank+0,	yysvec+169,	yyvstop+340,
yycrank+336,	0,		0,	
yycrank+350,	0,		0,	
yycrank+0,	yysvec+23,	yyvstop+342,
yycrank+0,	yysvec+23,	yyvstop+345,
yycrank+0,	yysvec+23,	yyvstop+348,
yycrank+0,	yysvec+23,	yyvstop+351,
yycrank+340,	yysvec+23,	yyvstop+354,
yycrank+412,	yysvec+23,	yyvstop+356,
yycrank+348,	yysvec+23,	yyvstop+358,
yycrank+355,	0,		0,	
yycrank+-414,	yysvec+150,	0,	
yycrank+-476,	0,		0,	
yycrank+0,	0,		yyvstop+360,
yycrank+0,	0,		yyvstop+362,
yycrank+358,	0,		0,	
yycrank+352,	yysvec+23,	yyvstop+364,
yycrank+420,	yysvec+23,	yyvstop+366,
yycrank+0,	yysvec+23,	yyvstop+368,
yycrank+347,	yysvec+23,	yyvstop+371,
yycrank+363,	yysvec+23,	yyvstop+373,
yycrank+364,	yysvec+23,	yyvstop+375,
yycrank+373,	yysvec+23,	yyvstop+377,
yycrank+373,	yysvec+23,	yyvstop+379,
yycrank+359,	yysvec+23,	yyvstop+381,
yycrank+0,	yysvec+23,	yyvstop+383,
yycrank+0,	yysvec+23,	yyvstop+386,
yycrank+376,	0,		0,	
yycrank+381,	0,		0,	
yycrank+0,	0,		yyvstop+389,
yycrank+0,	yysvec+23,	yyvstop+391,
yycrank+0,	0,		yyvstop+394,
yycrank+372,	yysvec+23,	yyvstop+396,
yycrank+377,	0,		0,	
yycrank+-447,	yysvec+182,	0,	
yycrank+383,	0,		0,	
yycrank+391,	yysvec+23,	yyvstop+398,
yycrank+0,	0,		yyvstop+400,
yycrank+0,	yysvec+23,	yyvstop+402,
yycrank+461,	yysvec+23,	yyvstop+405,
yycrank+377,	yysvec+23,	yyvstop+407,
yycrank+386,	yysvec+23,	yyvstop+409,
yycrank+0,	yysvec+23,	yyvstop+411,
yycrank+0,	yysvec+23,	yyvstop+414,
yycrank+464,	0,		0,	
yycrank+0,	0,		yyvstop+417,
yycrank+387,	yysvec+23,	yyvstop+419,
yycrank+397,	0,		0,	
yycrank+399,	0,		0,	
yycrank+401,	yysvec+23,	yyvstop+421,
yycrank+388,	yysvec+23,	yyvstop+423,
yycrank+0,	yysvec+23,	yyvstop+425,
yycrank+471,	yysvec+23,	yyvstop+428,
yycrank+419,	0,		0,	
yycrank+420,	0,		0,	
yycrank+489,	yysvec+23,	yyvstop+431,
yycrank+421,	yysvec+23,	yyvstop+433,
yycrank+505,	0,		0,	
yycrank+493,	0,		0,	
yycrank+494,	0,		0,	
yycrank+495,	yysvec+23,	yyvstop+435,
yycrank+469,	yysvec+227,	0,	
yycrank+477,	0,		0,	
yycrank+528,	0,		0,	
yycrank+470,	yysvec+232,	0,	
yycrank+419,	0,		0,	
yycrank+530,	0,		0,	
yycrank+417,	0,		0,	
yycrank+421,	0,		0,	
yycrank+414,	0,		0,	
yycrank+420,	0,		0,	
yycrank+438,	0,		0,	
yycrank+423,	0,		0,	
yycrank+429,	0,		0,	
yycrank+447,	0,		0,	
yycrank+447,	0,		0,	
yycrank+434,	0,		0,	
yycrank+491,	0,		0,	
yycrank+452,	0,		0,	
yycrank+0,	0,		yyvstop+438,
yycrank+494,	0,		0,	
yycrank+0,	0,		yyvstop+440,
0,	0,	0};
struct yywork *yytop = yycrank+632;
struct yysvf *yybgin = yysvec+1;
Uchar yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,047 ,
'(' ,')' ,'*' ,01  ,'(' ,01  ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,'(' ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,01  ,
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
