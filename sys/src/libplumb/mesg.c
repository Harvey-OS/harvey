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
#include "plumb.h"

int
plumbopen(char *name, int omode)
{
	int fd, f;
	char *s, *plumber;
	char buf[128], err[ERRMAX];

	if(name[0] == '/')
		return open(name, omode);

	/* find elusive plumber */
	if(access("/mnt/plumb/send", AWRITE) >= 0)
		plumber = "/mnt/plumb";
	else if(access("/mnt/term/mnt/plumb/send", AWRITE) >= 0)
		plumber = "/mnt/term/mnt/plumb";
	else{
		/* last resort: try mounting service */
		plumber = "/mnt/plumb";
		s = getenv("plumbsrv");
		if(s == nil)
			return -1;
		f = open(s, ORDWR);
		free(s);
		if(f < 0)
			return -1;
		if(mount(f, -1, "/mnt/plumb", MREPL, "", 'M') < 0){
			close(f);
			return -1;
		}
		if(access("/mnt/plumb/send", AWRITE) < 0)
			return -1;
	}

	snprint(buf, sizeof buf, "%s/%s", plumber, name);
	fd = open(buf, omode);
	if(fd >= 0)
		return fd;

	/* try creating port; used by non-standard plumb implementations */
	rerrstr(err, sizeof err);
	fd = create(buf, omode, 0600);
	if(fd >= 0)
		return fd;
	errstr(err, sizeof err);

	return -1;
}

static int
Strlen(char *s)
{
	if(s == nil)
		return 0;
	return strlen(s);
}

static char*
Strcpy(char *s, char *t)
{
	if(t == nil)
		return s;
	return strcpy(s, t) + strlen(t);
}

/* quote attribute value, if necessary */
static char*
quote(char *s, char *buf, char *bufe)
{
	char *t;
	int c;

	if(s == nil){
		buf[0] = '\0';
		return buf;
	}
	if(strpbrk(s, " '=\t") == nil)
		return s;
	t = buf;
	*t++ = '\'';
	while(t < bufe-2){
		c = *s++;
		if(c == '\0')
			break;
		*t++ = c;
		if(c == '\'')
			*t++ = c;
	}
	*t++ = '\'';
	*t = '\0';
	return buf;
}

char*
plumbpackattr(Plumbattr *attr)
{
	int n;
	Plumbattr *a;
	char *s, *t, *buf, *bufe;

	if(attr == nil)
		return nil;
	if((buf = malloc(4096)) == nil)
		return nil;
	bufe = buf + 4096;
	n = 0;
	for(a=attr; a!=nil; a=a->next)
		n += Strlen(a->name) + 1 + Strlen(quote(a->value, buf, bufe)) + 1;
	s = malloc(n);
	if(s == nil) {
		free(buf);
		return nil;
	}
	t = s;
	*t = '\0';
	for(a=attr; a!=nil; a=a->next){
		if(t != s)
			*t++ = ' ';
		strcpy(t, a->name);
		strcat(t, "=");
		strcat(t, quote(a->value, buf, bufe));
		t += strlen(t);
	}
	if(t > s+n)
		abort();
	free(buf);
	return s;
}

char*
plumblookup(Plumbattr *attr, char *name)
{
	while(attr){
		if(strcmp(attr->name, name) == 0)
			return attr->value;
		attr = attr->next;
	}
	return nil;
}

char*
plumbpack(Plumbmsg *m, int *np)
{
	int n, ndata;
	char *buf, *p, *attr;

	ndata = m->ndata;
	if(ndata < 0)
		ndata = Strlen(m->data);
	attr = plumbpackattr(m->attr);
	n = Strlen(m->src)+1 + Strlen(m->dst)+1 + Strlen(m->wdir)+1 +
		Strlen(m->type)+1 + Strlen(attr)+1 + 16 + ndata;
	buf = malloc(n+1);	/* +1 for '\0' */
	if(buf == nil){
		free(attr);
		return nil;
	}
	p = Strcpy(buf, m->src);
	*p++ = '\n';
	p = Strcpy(p, m->dst);
	*p++ = '\n';
	p = Strcpy(p, m->wdir);
	*p++ = '\n';
	p = Strcpy(p, m->type);
	*p++ = '\n';
	p = Strcpy(p, attr);
	*p++ = '\n';
	p += sprint(p, "%d\n", ndata);
	memmove(p, m->data, ndata);
	*np = (p-buf)+ndata;
	buf[*np] = '\0';	/* null terminate just in case */
	if(*np >= n+1)
		abort();
	free(attr);
	return buf;
}

int
plumbsend(int fd, Plumbmsg *m)
{
	char *buf;
	int n;

	buf = plumbpack(m, &n);
	if(buf == nil)
		return -1;
	n = write(fd, buf, n);
	free(buf);
	return n;
}

static int
plumbline(char **linep, char *buf, int i, int n, int *bad)
{
	int starti;
	char *p;

	starti = i;
	while(i<n && buf[i]!='\n')
		i++;
	if(i == n)
		*bad = 1;
	else{
		p = malloc((i-starti) + 1);
		if(p == nil)
			*bad = 1;
		else{
			memmove(p, buf+starti, i-starti);
			p[i-starti] = '\0';
		}
		*linep = p;
		i++;
	}
	return i;
}

void
plumbfree(Plumbmsg *m)
{
	Plumbattr *a, *next;

	free(m->src);
	free(m->dst);
	free(m->wdir);
	free(m->type);
	for(a=m->attr; a!=nil; a=next){
		next = a->next;
		free(a->name);
		free(a->value);
		free(a);
	}
	free(m->data);
	free(m);
}

Plumbattr*
plumbunpackattr(char *p)
{
	Plumbattr *attr, *prev, *a;
	char *q, *v, *buf, *bufe;
	int c, quoting;

	buf = malloc(4096);
	if(buf == nil)
		return nil;
	bufe = buf + 4096;
	attr = prev = nil;
	while(*p!='\0' && *p!='\n'){
		while(*p==' ' || *p=='\t')
			p++;
		if(*p == '\0')
			break;
		for(q=p; *q!='\0' && *q!='\n' && *q!=' ' && *q!='\t'; q++)
			if(*q == '=')
				break;
		if(*q != '=')
			break;	/* malformed attribute */
		a = malloc(sizeof(Plumbattr));
		if(a == nil)
			break;
		a->name = malloc(q-p+1);
		if(a->name == nil){
			free(a);
			break;
		}
		memmove(a->name, p, q-p);
		a->name[q-p] = '\0';
		/* process quotes in value */
		q++;	/* skip '=' */
		v = buf;
		quoting = 0;
		while(*q!='\0' && *q!='\n'){
			if(v >= bufe)
				break;
			c = *q++;
			if(quoting){
				if(c == '\''){
					if(*q == '\'')
						q++;
					else{
						quoting = 0;
						continue;
					}
				}
			}else{
				if(c==' ' || c=='\t')
					break;
				if(c == '\''){
					quoting = 1;
					continue;
				}
			}
			*v++ = c;
		}
		a->value = malloc(v-buf+1);
		if(a->value == nil){
			free(a->name);
			free(a);
			break;
		}
		memmove(a->value, buf, v-buf);
		a->value[v-buf] = '\0';
		a->next = nil;
		if(prev == nil)
			attr = a;
		else
			prev->next = a;
		prev = a;
		p = q;
	}
	free(buf);
	return attr;
}

Plumbattr*
plumbaddattr(Plumbattr *attr, Plumbattr *new)
{
	Plumbattr *l;

	l = attr;
	if(l == nil)
		return new;
	while(l->next != nil)
		l = l->next;
	l->next = new;
	return attr;
}

Plumbattr*
plumbdelattr(Plumbattr *attr, char *name)
{
	Plumbattr *l, *prev;

	prev = nil;
	for(l=attr; l!=nil; l=l->next){
		if(strcmp(name, l->name) == 0)
			break;
		prev = l;
	}
	if(l == nil)
		return nil;
	if(prev)
		prev->next = l->next;
	else
		attr = l->next;
	free(l->name);
	free(l->value);
	free(l);
	return attr;
}

Plumbmsg*
plumbunpackpartial(char *buf, int n, int *morep)
{
	Plumbmsg *m;
	int i, bad;
	char *ntext, *attr;

	m = malloc(sizeof(Plumbmsg));
	if(m == nil)
		return nil;
	memset(m, 0, sizeof(Plumbmsg));
	if(morep != nil)
		*morep = 0;
	bad = 0;
	i = plumbline(&m->src, buf, 0, n, &bad);
	i = plumbline(&m->dst, buf, i, n, &bad);
	i = plumbline(&m->wdir, buf, i, n, &bad);
	i = plumbline(&m->type, buf, i, n, &bad);
	i = plumbline(&attr, buf, i, n, &bad);
	i = plumbline(&ntext, buf, i, n, &bad);
	if(bad){
		plumbfree(m);
		return nil;
	}
	m->attr = plumbunpackattr(attr);
	free(attr);
	m->ndata = atoi(ntext);
	if(m->ndata != n-i){
		bad = 1;
		if(morep!=nil && m->ndata>n-i)
			*morep = m->ndata - (n-i);
	}
	free(ntext);
	if(!bad){
		m->data = malloc(n-i+1);	/* +1 for '\0' */
		if(m->data == nil)
			bad = 1;
		else{
			memmove(m->data, buf+i, m->ndata);
			m->ndata = n-i;
			/* null-terminate in case it's text */
			m->data[m->ndata] = '\0';
		}
	}
	if(bad){
		plumbfree(m);
		m = nil;
	}
	return m;
}

Plumbmsg*
plumbunpack(char *buf, int n)
{
	return plumbunpackpartial(buf, n, nil);
}

Plumbmsg*
plumbrecv(int fd)
{
	char *buf;
	Plumbmsg *m;
	int n, more;

	buf = malloc(8192);
	if(buf == nil)
		return nil;
	n = read(fd, buf, 8192);
	m = nil;
	if(n > 0){
		m = plumbunpackpartial(buf, n, &more);
		if(m==nil && more>0){
			/* we now know how many more bytes to read for complete message */
			buf = realloc(buf, n+more);
			if(buf == nil)
				return nil;
			if(readn(fd, buf+n, more) == more)
				m = plumbunpackpartial(buf, n+more, nil);
		}
	}
	free(buf);
	return m;
}
