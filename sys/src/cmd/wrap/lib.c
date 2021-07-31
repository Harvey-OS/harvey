#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

int
waserrstr(void)
{
	char err[ERRLEN];
	int rv;

	err[0] = '\0';
	errstr(err);
	rv = err[0] != '\0';
	errstr(err);
	return rv;
}

char*
mkpath(char *a, char *b)
{
	char *p;
	int l;

	l = strlen(a)+1+strlen(b)+1;
	p = emalloc(l);
	snprint(p, l, "%s/%s", a, b);
	cleanname(p);
	return p;
}

char*
mkpath3(char *a, char *b, char *c)
{
	char *p;
	int l;

	l = strlen(a)+1+strlen(b)+1+strlen(c)+1;
	p = emalloc(l);
	snprint(p, l, "%s/%s/%s", a, b, c);
	cleanname(p);
	return p;
}

char*
readfile(char *s)
{
	Dir d;
	char *p;
	int fd, n;

	if(dirstat(s, &d) < 0)
		return nil;

	p = emalloc(d.length+1);

	if((fd = open(s, OREAD)) < 0
	|| (n = readn(fd, p, d.length)) < 0) {
		free(p);
		return nil;
	}

	p[n] = '\0';
	return p;
}

int
strprefix(char *s, char *pre)
{
	return strncmp(s, pre, strlen(pre));
}

int
match(char *s, char **pre, int npre)
{
	int i;

	if(strcmp(s, "/wrap") == 0 || strprefix(s, "/wrap/") == 0 || npre == 0)
		return 1;

	for(i=0; i<npre; i++)
		if(strprefix(s, pre[i]) == 0)
			return 1;
	return 0;
}

int
genopentemp(char *template, int mode, int perm)
{
	int fd, i;
	char *p;	

	p = estrdup(template);
	fd = -1;
	for(i=0; i<10; i++){
		mktemp(p);
		if(access(p, 0) < 0 && (fd=create(p, mode, perm)) >= 0)
			break;
		strcpy(p, template);
	}
	if(fd < 0)
		sysfatal("could not create temporary file");

	strcpy(template, p);
	free(p);

	return fd;
}

int
opentemp(char *template)
{
	return genopentemp(template, ORDWR|ORCLOSE, 0400);
}

int
fsort(int fd, char *fn)
{
	Waitmsg w;

	switch(fork()){
	case -1:
		sysfatal("fork %r");
	case 0:
		dup(fd, 1);
		execl("/bin/sort", "sort", fn, nil);
		sysfatal("sort %r");
	default:
		wait(&w);
		if(w.msg[0]) {
			errstr(w.msg);
			return -1;
		}
		return 0;
	}
}

vlong
filelength(char *fn)
{
	Dir d;

	if(dirstat(fn, &d) < 0)
		return -1;

	return d.length;
}

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil)
		sysfatal("realloc %lud fails\n", sz);
	return v;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("strdup (%.10s) fails\n", s);
	return s;
}

