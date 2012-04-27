#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <ctype.h>
#include <thread.h>
#include "wiki.h"

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory reallocating %lud", n);
	setmalloctag(v, getcallerpc(&v));
	return v;
}

void*
emalloc(ulong n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory allocating %lud", n);
	memset(v, 0, n);
	setmalloctag(v, getcallerpc(&n));
	return v;
}

char*
estrdup(char *s)
{
	int l;
	char *t;

	if (s == nil)
		return nil;
	l = strlen(s)+1;
	t = emalloc(l);
	memmove(t, s, l);
	setmalloctag(t, getcallerpc(&s));
	return t;
}

char*
estrdupn(char *s, int n)
{
	int l;
	char *t;

	l = strlen(s);
	if(l > n)
		l = n;
	t = emalloc(l+1);
	memmove(t, s, l);
	t[l] = '\0';
	setmalloctag(t, getcallerpc(&s));
	return t;
}

char*
strlower(char *s)
{
	char *p;

	for(p=s; *p; p++)
		if('A' <= *p && *p <= 'Z')
			*p += 'a'-'A';
	return s;
}

String*
s_appendsub(String *s, char *p, int n, Sub *sub, int nsub)
{
	int i, m;
	char *q, *r, *ep;

	ep = p+n;
	while(p<ep){
		q = ep;
		m = -1;
		for(i=0; i<nsub; i++){
			if(sub[i].sub && (r = strstr(p, sub[i].match)) && r < q){
				q = r;
				m = i;
			}
		}
		s = s_nappend(s, p, q-p);
		p = q;
		if(m >= 0){
			s = s_append(s, sub[m].sub);
			p += strlen(sub[m].match);
		}
	}
	return s;
}

String*
s_appendlist(String *s, ...)
{
	char *x;
	va_list arg;

	va_start(arg, s);
	while(x = va_arg(arg, char*))
		s = s_append(s, x);
	va_end(arg);
	return s;
}

int
opentemp(char *template)
{
	int fd, i;
	char *p;

	p = estrdup(template);
	fd = -1;
	for(i=0; i<10; i++){
		mktemp(p);
		if(access(p, 0) < 0 && (fd=create(p, ORDWR|ORCLOSE, 0444)) >= 0)
			break;
		strcpy(p, template);
	}
	if(fd >= 0)
		strcpy(template, p);
	free(p);

	return fd;
}

