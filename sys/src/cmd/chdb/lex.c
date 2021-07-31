#include	"ddb.h"
#include	"y.tab.h"

int
yylex(void)
{
	int c, e;
	Rune r;
	long n;

	if(yysyntax || yyterminal)
		return -1;

	if(yystring) {
		yylval.str.beg = cmdi;
		n = 0;	/* inside [] */
		e = 0;	/* escape */
		for(;;) {
			c = chartorune(&r, &cmd[cmdi]);
			if(e == 0 && n == 0 && r == '[')
				n = 1;
			if(e == 0 && n == 1 && r == ']')
				n = 0;
			if(e == 0 && n == 0 && r == yystring) {
				yylval.str.end = cmdi;
				cmdi += c;
				break;
			}
			if(e == 0 && r == '\\')
				e = 1;
			else
				e = 0;
			if(r == 0) {
				yylval.str.end = cmdi;
				break;
			}
			cmdi += c;
		}
		yystring = 0;
		return STRING;
	}

loop:
	cmdi += chartorune(&r, &cmd[cmdi]);
	c = r;
	yylval.lval = c;

	switch(c) {
	case 0:
		yyterminal = 1;
		return ';';

	case ' ':
	case '\t':
		goto loop;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		n = c - '0';
		for(;;) {
			c = cmd[cmdi];
			if(c >= '0' && c <= '9') {
				cmdi++;
				n = n*10 + c-'0';
				continue;
			}
			yylval.lval = n;
			return NUMB;
		}
	}
	return c;
}

void
yyerror(char *fmt, ...)
{
	char *p1, *p2;

	if(yysyntax)
		return;
	yysyntax = 1;
	strcpy(chars, cmd);
	p1 = chars+cmdi;
	p2 = strchr(p1, 0)+1;
	memmove(p1+1, p1, p2-p1);
	*p1 = '\\';
	prline(7);
	doprint(chars, chars+sizeof(chars), fmt, &fmt+1);
	prline(8);
}
