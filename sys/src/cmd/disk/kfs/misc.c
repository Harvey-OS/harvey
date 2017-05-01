/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

extern int cmdfd;

Float
famd(Float a, int b, int c, int d)
{
	uint32_t x, m;

	x = (a + b) * c;
	m = x % d;
	x /= d;
	if(m >= d / 2)
		x++;
	return x;
}

uint32_t
fdf(Float a, int d)
{
	uint32_t x, m;

	m = a % d;
	x = a / d;
	if(m >= d / 2)
		x++;
	return x;
}

int32_t
beint32(char *s)
{
	uint8_t *x;

	x = (uint8_t *)s;
	return (x[0] << 24) + (x[1] << 16) + (x[2] << 8) + x[3];
}

void
panic(char *fmt, ...)
{
	char buf[8192], *s;
	va_list arg;


	s = buf;
	s += sprint(s, "%s %s %d: ", progname, procname, getpid());
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
abort();
	exits(buf);
}

#define	SIZE	4096

void
cprint(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;

	va_start(arg, fmt);
	out = vseprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	write(cmdfd, buf, (int32_t)(out-buf));
}

/*
 * print goes to fd 2 [sic] because fd 1 might be
 * otherwise preoccupied when the -s flag is given to kfs.
 */
int
print(char *fmt, ...)
{
	va_list arg;
	int n;

	va_start(arg, fmt);
	n = vfprint(2, fmt, arg);
	va_end(arg);
	return n;
}
