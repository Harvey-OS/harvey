#include <u.h>
#include <libc.h>
#include <regexp.h>
#include <String.h>
#include "glob.h"

/*
 *  I wrote this glob so that there would be no limit
 *  on element or path size.  The one in rc is probably
 *  better, certainly faster. - presotto
 */

static Glob*
globnew(void)
{
	Glob *g;

	g = mallocz(sizeof(*g), 1);
	if(g == nil)
		sysfatal("globnew: %r");
	return g;
}

static void
globfree1(Glob *g)
{
	s_free(g->glob);
	free(g);
}

static void
globfree(Glob *g)
{
	Glob *next;

	for(; g != nil; g = next){
		next = g->next;
		globfree1(g);
	}
}

static Globlist*
globlistnew(char *x)
{
	Globlist *gl;

	gl = mallocz(sizeof *gl, 1);
	if(gl == nil)
		sysfatal("globlistnew: %r");
	gl->first = globnew();
	gl->first->glob = s_copy(x);
	gl->l = &gl->first->next;
	return gl;
}

void
globlistfree(Globlist *gl)
{
	if(gl == nil)
		return;
	globfree(gl->first);
	free(gl);
}

void
globadd(Globlist *gl, char *dir, char *file)
{
	Glob *g;

	g = globnew();
	g->glob = s_copy(dir);
	if(strcmp(dir, "/") != 0 && *dir != 0)
		s_append(g->glob, "/");
	s_append(g->glob, file);
	*(gl->l) = g;
	gl->l = &(g->next); 
}

static void
globdir(Globlist *gl, char *dir, Reprog *re)
{
	Dir *d;
	int i, n, fd;

	if(*dir == 0)
		fd = open(".", OREAD);
	else
		fd = open(dir, OREAD);
	if(fd < 0)
		return;
	n = dirreadall(fd, &d);
	if(n == 0)
		return;
	close(fd);
	for(i = 0; i < n; i++)
		if(regexec(re, d[i].name, nil, 0))
			globadd(gl, dir, d[i].name);
	free(d);
}

static void
globdot(Globlist *gl, char *dir)
{
	Dir *d;

	if(*dir == 0){
		globadd(gl, "", ".");
		return;
	}
	d = dirstat(dir);
	if(d == nil)
		return;
	if(d->qid.type & QTDIR)
		globadd(gl, dir, ".");
	free(d);
}

static void
globnext(Globlist *gl, char *pattern)
{
	String *np;
	Glob *g, *inlist;
	Reprog *re;
	int c;

	/* nothing left */
	if(*pattern == 0)
		return;

	inlist = gl->first;
	gl->first = nil;
	gl->l = &gl->first;

	/* pick off next pattern and turn into a reg exp */
	np = s_new();
	s_putc(np, '^');
	for(; c = *pattern; pattern++){
		if(c == '/'){
			pattern++;
			break;
		}
		switch(c){
		case '|':
		case '+':
		case '.':
		case '^':
		case '$':
		case '(':
		case ')':
			s_putc(np, '\\');
			s_putc(np, c);
			break;
		case '?':
			s_putc(np, '.');
			break;
		case '*':
			s_putc(np, '.');
			s_putc(np, '*');
			break;
		default:
			s_putc(np, c);
			break;
		}
	}
	s_putc(np, '$');
	s_terminate(np);
	if(strcmp(s_to_c(np), "^\\.$") == 0){
		/* anything that's a directory works */
		for(g = inlist; g != nil; g = g->next)
			globdot(gl, s_to_c(g->glob));
	} else {
		re = regcomp(s_to_c(np));

		/* run input list as directories */
		for(g = inlist; g != nil; g = g->next)
			globdir(gl, s_to_c(g->glob), re);
		free(re);
	}
	s_free(np);
	globfree(inlist);

	if(gl->first != nil)
		globnext(gl, pattern);
}

char *
globiter(Globlist *gl)
{
	Glob *g;
	char *s;

	if(gl->first == nil)
		return nil;
	g = gl->first;
	gl->first = g->next;
	if(gl->first == nil)
		gl->l = &gl->first;
	s = strdup(s_to_c(g->glob));
	if(s == nil)
		sysfatal("globiter: %r");
	globfree1(g);
	return s;
}

Globlist*
glob(char *pattern)
{
	Globlist *gl;

	if(pattern == nil || *pattern == 0)
		return nil;
	if(*pattern == '/'){
		pattern++;
		gl = globlistnew("/");
	} else
		gl = globlistnew("");
	globnext(gl, pattern);
	return gl;
}
