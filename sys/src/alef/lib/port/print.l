#include <alef.h>

#define	SIZE	1024
extern	int	printcol;
intern	int	errcount;

int
print(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, ...);
	n = write(1, buf, (int)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits("print errors");
	return n;
}

int
fprint(int f, char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, ...);
	n = write(f, buf, (int)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits("print errors");
	return n;
}

int
sprint(char *buf, char *fmt, ...)
{
	char *out;
	int scol;

	scol = printcol;
	out = doprint(buf, buf+SIZE, fmt, ...);
	printcol = scol;
	return out-buf;
}
