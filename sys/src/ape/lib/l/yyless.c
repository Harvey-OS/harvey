/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<libl.h>
#include	<stdio.h>

extern	char	yytext[];
extern	int	yyleng;
extern	int	yyprevious;

void	yyunput(int c);

void
yyless(int x)
{
	char *lastch, *ptr;

	lastch = yytext+yyleng;
	if(x>=0 && x <= yyleng)
		ptr = x + yytext;
	else
		ptr = (char*)x;
	while(lastch > ptr)
		yyunput(*--lastch);
	*lastch = 0;
	if (ptr >yytext)
		yyprevious = *--lastch;
	yyleng = ptr-yytext;
}
