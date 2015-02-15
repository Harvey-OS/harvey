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

int	(*doquote)(int);

extern int _needsquotes(char*, int*);
extern int _runeneedsquotes(Rune*, int*);

char*
unquotestrdup(char *s)
{
	char *t, *ret;
	int quoting;

	ret = s = strdup(s);	/* return unquoted copy */
	if(ret == nil)
		return ret;
	quoting = 0;
	t = s;	/* s is output string, t is input string */
	while(*t!='\0' && (quoting || (*t!=' ' && *t!='\t'))){
		if(*t != '\''){
			*s++ = *t++;
			continue;
		}
		/* *t is a quote */
		if(!quoting){
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if(t[1] != '\''){
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t++;
		*s++ = *t++;
	}
	if(t != s)
		memmove(s, t, strlen(t)+1);
	return ret;
}

Rune*
unquoterunestrdup(Rune *s)
{
	Rune *t, *ret;
	int quoting;

	ret = s = runestrdup(s);	/* return unquoted copy */
	if(ret == nil)
		return ret;
	quoting = 0;
	t = s;	/* s is output string, t is input string */
	while(*t!='\0' && (quoting || (*t!=' ' && *t!='\t'))){
		if(*t != '\''){
			*s++ = *t++;
			continue;
		}
		/* *t is a quote */
		if(!quoting){
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if(t[1] != '\''){
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t++;
		*s++ = *t++;
	}
	if(t != s)
		memmove(s, t, (runestrlen(t)+1)*sizeof(Rune));
	return ret;
}

char*
quotestrdup(char *s)
{
	char *t, *u, *ret;
	int quotelen;
	Rune r;

	if(_needsquotes(s, &quotelen) == 0)
		return strdup(s);
	
	ret = malloc(quotelen+1);
	if(ret == nil)
		return nil;
	u = ret;
	*u++ = '\'';
	for(t=s; *t; t++){
		r = *t;
		if(r == L'\'')
			*u++ = r;	/* double the quote */
		*u++ = r;
	}
	*u++ = '\'';
	*u = '\0';
	return ret;
}

Rune*
quoterunestrdup(Rune *s)
{
	Rune *t, *u, *ret;
	int quotelen;
	Rune r;

	if(_runeneedsquotes(s, &quotelen) == 0)
		return runestrdup(s);
	
	ret = malloc((quotelen+1)*sizeof(Rune));
	if(ret == nil)
		return nil;
	u = ret;
	*u++ = '\'';
	for(t=s; *t; t++){
		r = *t;
		if(r == L'\'')
			*u++ = r;	/* double the quote */
		*u++ = r;
	}
	*u++ = '\'';
	*u = '\0';
	return ret;
}
