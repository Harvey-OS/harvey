#include <alef.h>

#define	SIZE	1024
extern	int	printcol;
intern	int	errcount;

int
print(byte *fmt, ...)
{
	int n;
	byte buf[SIZE], *out;

	out = doprint(buf, buf+SIZE, fmt, ...);
	n = write(1, buf, (int)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits("print errors");
	return n;
}

int
fprint(int f, byte *fmt, ...)
{
	int n;
	byte buf[SIZE], *out;

	out = doprint(buf, buf+SIZE, fmt, ...);
	n = write(f, buf, (int)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits("print errors");
	return n;
}

int
sprint(byte *buf, byte *fmt, ...)
{
	byte *out;
	int scol;

	scol = printcol;
	out = doprint(buf, buf+SIZE, fmt, ...);
	printcol = scol;
	return out-buf;
}

int
snprint(byte *buf, int len, byte *fmt, ...)
{
	byte *out;
	int scol;

	scol = printcol;
	out = doprint(buf, buf+len, fmt, ...);
	printcol = scol;
	return out-buf;
}
