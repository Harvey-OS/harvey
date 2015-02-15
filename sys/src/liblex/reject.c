/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<stdio.h>

extern	FILE*	yyout;
extern	FILE*	yyin;
extern	int	yyprevious, *yyfnd;
extern	char	yyextra[];
extern	char	yytext[];
extern	int	yyleng;

extern
struct
{
	int *yyaa, *yybb;
	int *yystops;
} *yylstate [], **yylsp, **yyolsp;

int	yyback(int *p, int m);
int	yyinput(void);
void	yyoutput(int c);
void	yyunput(int c);

int
yyracc(int m)
{

	yyolsp = yylsp;
	if(yyextra[m]) {
		while(yyback((*yylsp)->yystops, -m) != 1 && yylsp > yylstate) {
			yylsp--;
			yyunput(yytext[--yyleng]);
		}
	}
	yyprevious = yytext[yyleng-1];
	yytext[yyleng] = 0;
	return m;
}

int
yyreject(void)
{
	for(; yylsp < yyolsp; yylsp++)
		yytext[yyleng++] = yyinput();
	if(*yyfnd > 0)
		return yyracc(*yyfnd++);
	while(yylsp-- > yylstate) {
		yyunput(yytext[yyleng-1]);
		yytext[--yyleng] = 0;
		if(*yylsp != 0 && (yyfnd = (*yylsp)->yystops) && *yyfnd > 0)
			return yyracc(*yyfnd++);
	}
	if(yytext[0] == 0)
		return 0;
	yyoutput(yyprevious = yyinput());
	yyleng = 0;
	return -1;
}
