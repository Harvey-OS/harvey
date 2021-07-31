#include <alef.h>

Arg *arg;

enum
{
	SIZE=	256
};

void
fatal(byte *fmt, ...)
{
	byte buf[SIZE], buf2[NAMELEN+SIZE+4], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, ...);
	*out = 0;
	if(arg != nil && arg->arg0 != nil)
		n = snprint(buf2, sizeof(buf2), "%s: %s\n", arg->arg0, buf);
	else
		n = snprint(buf2, sizeof(buf2), "fatal: %s\n", buf);
	write(2, buf2, n);
	exits(buf);
}
