#include	<libl.h>
#include	<stdlib.h>

int	yylex(void);

void
main(int argc, char *argv[])
{
	USED(argc);
	yylex();
	exit(0);
}
