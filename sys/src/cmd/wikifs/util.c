/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <ctype.h>
#include <thread.h>
#include "wiki.h"

void*
erealloc(void *v, uint32_t n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory reallocating %lud", n);
	setmalloctag(v, getcallerpc(&v));
	return v;
}

void*
emalloc(uint32_t n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory allocating %lud", n);
	memset(v, 0, n);
	setmalloctag(v, getcallerpc(&n));
	return v;
}

int8_t*
estrdup(int8_t *s)
{
	int l;
	int8_t *t;

	if (s == nil)
		return nil;
	l = strlen(s)+1;
	t = emalloc(l);
	memmove(t, s, l);
	setmalloctag(t, getcallerpc(&s));
	return t;
}

int8_t*
estrdupn(int8_t *s, int n)
{
	int l;
	int8_t *t;

	l = strlen(s);
	if(l > n)
		l = n;
	t = emalloc(l+1);
	memmove(t, s, l);
	t[l] = '\0';
	setmalloctag(t, getcallerpc(&s));
	return t;
}

int8_t*
strlower(int8_t *s)
{
	int8_t *p;

	for(p=s; *p; p++)
		if('A' <= *p && *p <= 'Z')
			*p += 'a'-'A';
	return s;
}

String*
s_appendsub(String *s, int8_t *p, int n, Sub *sub, int nsub)
{
	int i, m;
	int8_t *q, *r, *ep;

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
	int8_t *x;
	va_list arg;

	va_start(arg, s);
	while(x = va_arg(arg, int8_t*))
		s = s_append(s, x);
	va_end(arg);
	return s;
}

int
opentemp(int8_t *template)
{
	int fd, i;
	int8_t *p;

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

