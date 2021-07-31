#include <u.h>
#include <libc.h>

#define	DOTDOT	(&fmt+1)

void
werrstr(char *fmt, ...)
{
	int psave;
	char buf[ERRLEN];

	extern int printcol;

	psave = printcol;
	doprint(buf, buf+ERRLEN, fmt, DOTDOT);
	errstr(buf);
	printcol = psave;
}
