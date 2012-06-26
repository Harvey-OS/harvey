#include	<libl.h>
#include	<stdio.h>

extern	FILE*	yyout;

int
printable(int c)
{
	return 040 < c && c < 0177;
}

void
allprint(char c)
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
