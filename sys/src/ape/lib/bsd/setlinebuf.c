#include <stdio.h>

void
setlinebuf(FILE *f)
{
	static char buf[BUFSIZ];

	setvbuf (f, buf, _IOLBF, BUFSIZ);
}
