#include <u.h>
#include <libc.h>

uchar debug[255];

int
dprint(char d, char *fmt, ...)
{
	int n;
	va_list args;

	if(!debug['A'] && !debug[d])
		return -1;
	va_start(args, fmt);
	n = vfprint(2, fmt, args);
	va_end(args);
	return n;
}


void
dumpbuf(char d, uchar *buf, int bufsz)
{
	int i;

	if(d != 0 && !debug[d])
		return;
	for(i = 0; i < bufsz; i++){
		fprint(2, "%#2.2x ", buf[i]);
		if(i != 0 && (i + 1) % 8 == 0)	
			fprint(2, "\n");
	}
	if(i %16 != 0)
		fprint(2, "\n");
}
