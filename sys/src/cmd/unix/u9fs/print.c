/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>

#define SIZE 4096
extern int printcol;

int
print(char* fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg, temp;
	int n;

	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf + SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	n = write(1, buf, (int32_t)(out - buf));
	return n;
}

int
fprint(int f, char* fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg, temp;
	int n;

	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf + SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	n = write(f, buf, (int32_t)(out - buf));
	return n;
}

int
sprint(char* buf, char* fmt, ...)
{
	char* out;
	va_list arg, temp;
	int scol;

	scol = printcol;
	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf + SIZE, fmt, &temp);
	va_end(temp);
	va_end(arg);
	printcol = scol;
	return out - buf;
}

int
snprint(char* buf, int len, char* fmt, ...)
{
	char* out;
	va_list arg, temp;
	int scol;

	scol = printcol;
	va_start(arg, fmt);
	va_copy(temp, arg);
	out = doprint(buf, buf + len, fmt, &temp);
	va_end(temp);
	va_end(arg);
	printcol = scol;
	return out - buf;
}

char*
seprint(char* buf, char* e, char* fmt, ...)
{
	char* out;
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
