#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "win.h"

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
	strcpy(u, s);
	strcat(u, t);
	return u;
}

char*
estrstrstrdup(char *r, char *s, char *t)
{
	char *u;

	u = emalloc(strlen(r)+strlen(s)+strlen(t)+1);
	strcpy(u, r);
	strcat(u, s);
	strcat(u, t);
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
		strcpy(u, s);
		strcat(u, sep);
		strcat(u, t);
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
	va_list arg;
	char buf[256];
	Fmt f;

	fmtfdinit(&f, 2, buf, sizeof buf);
	fmtprint(&f, "%s: ", argv0);
	va_start(arg, fmt);
	fmtprint(&f, fmt, arg);
	va_end(arg);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
	threadexitsall(fmt);
}

void
ctlprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[256];

	va_start(arg, fmt);
	n = vfprint(fd, fmt, arg);
	va_end(arg);
	if(n < 0)
		error("control file write(%s) error: %r", buf);
}
