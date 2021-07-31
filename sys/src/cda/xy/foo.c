#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void
main(int argc, char **argv)
{
	int i, n;

	fmtinstall('G', gerbconv);
	for(i=1; i<argc; i++){
		n = strtol(*++argv, 0, 0);
		print("X%G\t%d\n", n, n);
	}
	exits(0);
}

enum
{
	NONE	= -1000,
};

int
gerbconv(void *o, int f1, int f2, int f3, int chr)
{
	char buf[16];
	char *p = buf;

	p += sprint(p, "%.5d", *(long *)o);
	while(--p > buf && *p == '0')	/* sic dixit jhc */
		*p = 0;
	strconv(buf, 0, NONE, 0);
	return sizeof(long);
}
