#include <plan9.h>

#define	SIZE	4096
extern	int	printcol;

int
print(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg, temp;
	int n;

	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf+SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	n = write(1, buf, (long)(out-buf));
	return n;
}

int
fprint(int f, char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg, temp;
	int n;

	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf+SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	n = write(f, buf, (long)(out-buf));
	return n;
}

int
sprint(char *buf, char *fmt, ...)
{
	char *out;
	va_list arg, temp;
	int scol;

	scol = printcol;
	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf+SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	printcol = scol;
	return out-buf;
}

int
snprint(char *buf, int len, char *fmt, ...)
{
	char *out;
	va_list arg, temp;
	int scol;

	scol = printcol;
	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf+len, fmt, &temp);
	va_end(temp);
	va_end(arg);
	printcol = scol;
	return out-buf;
}

char*
seprint(char *buf, char *e, char *fmt, ...)
{
	char *out;
	va_list arg, temp;
	int scol;

	scol = printcol;
	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, e, fmt, &temp);
	va_end(temp);
	va_end(arg);
	printcol = scol;
	return out;
}
