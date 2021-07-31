#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "dat.h"

void*
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		error("can't malloc: %r");
	memset(p, 0, n);
	return p;
}

char*
estrdup(char *s)
{
	char *t;

	t = emalloc(strlen(s)+1);
	strcpy(t, s);
	return t;
}

char*
estrstrdup(char *s, char *t)
{
	char *u;

	u = emalloc(strlen(s)+strlen(t)+1);
	sprint(u, "%s%s", s, t);
	return u;
}

char*
eappend(char *s, char *sep, char *t)
{
	char *u;

	if(t == nil)
		u = estrstrdup(s, sep);
	else{
		u = emalloc(strlen(s)+strlen(sep)+strlen(t)+1);
		sprint(u, "%s%s%s", s, sep, t);
	}
	free(s);
	return u;
}

char*
egrow(char *s, char *sep, char *t)
{
	s = eappend(s, sep, t);
	free(t);
	return s;
}

void
error(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[256];

	threadprint(2, "win: ");
	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof buf, fmt, arg) - buf;
	va_end(arg);
	write(2, buf, n);
	write(2, "\n", 1);
	threadexitsall(fmt);
}

void
ctlprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[256];

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof buf, fmt, arg) - buf;
	va_end(arg);
	if(write(fd, buf, n) != n)
		error("control file write error: %r");
}
