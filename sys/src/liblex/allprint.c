/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"/sys/src/cmd/lex/ldefs.h"
#include	<stdio.h>

extern	FILE*	yyout;

int
printable(int c)
{
	return 040 < c && c < 0177;
}

void
allprint(int c)
{

	switch(c) {
	case '\n':
		fprintf(yyout,"\\n");
		break;
	case '\t':
		fprintf(yyout,"\\t");
		break;
	case '\b':
		fprintf(yyout,"\\b");
		break;
	case ' ':
		fprintf(yyout,"\\\bb");
		break;
	default:
		if(!printable(c))
			fprintf(yyout,"\\%-3o",c);
		else 
			c = putc(c,yyout);
			USED(c);
		break;
	}
	return;
}
