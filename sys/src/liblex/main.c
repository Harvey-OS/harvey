#include	<u.h>
#include	<libc.h>
#include	<stdio.h>

int	yylex(void);

void
main(int argc, char *argv[])
{
	USED(argc, argv);
	yylex();
	exits(0);
}
