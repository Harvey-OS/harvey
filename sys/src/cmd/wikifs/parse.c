#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <ctype.h>
#include <thread.h>
#include "wiki.h"

static Wpage*
mkwtxt(int type, char *text)
{
	Wpage *w;

	w = emalloc(sizeof(*w));
	w->type = type;
	w->text = text;
	return w;
}

/*
 * turn runs of whitespace into single spaces,
 * eliminate whitespace at beginning and end.
 */
char*
strcondense(char *s, int cutbegin)
{
	char *r, *w, *es;
	int inspace;

	es = s+strlen(s);
	inspace = cutbegin;
	for(r=w=s; *r; r++){
		if(isspace(*r)){
			if(!inspace){
				inspace=1;
				*w++ = ' ';
			}
		}else{
			inspace=0;
			*w++ = *r;
		}
	}
	assert(w <= es);
	if(inspace && w>s){
		--w;
		*w = '\0';
	}
	else
		*w = '\0';
	return s;
}

/*
 * turn runs of Wplain into single Wplain.
 */
static Wpage*
wcondense(Wpage *wtxt)
{
	Wpage *ow, *w;

	for(w=wtxt; w; ){
		if(w->type == Wplain)
			strcondense(w->text, 1);

		if(w->type != Wplain || w->next==nil
		|| w->next->type != Wplain){
			w=w->next;
			continue;
		}

		w->text = erealloc(w->text, strlen(w->text)+1+strlen(w->next->text)+1);
		strcat(w->text, " ");
		strcat(w->text, w->next->text);
		
		free(w->next->text);
		ow = w->next;
		w->next = w->next->next;
		free(ow);
	}
	return wtxt;
}

/*
 * Parse a link, without the brackets.
 */
static Wpage*
mklink(char *s)
{
	char *q;
	Wpage *w;

	for(q=s; *q && *q != '|'; q++)
		;

	if(*q == '\0'){
		w = mkwtxt(Wlink, estrdup(strcondense(s, 1)));
		w->url = nil;
	}else{
		*q = '\0';
		w = mkwtxt(Wlink, estrdup(strcondense(s, 1)));
		w->url = estrdup(strcondense(q+1, 1));
	}
	return w;
}

/*
 * Parse Wplains, inserting Wlink nodes where appropriate.
 */
static Wpage*
wlink(Wpage *wtxt)
{
	char *p, *q, *r, *s;
	Wpage *w, *nw;

	for(w=wtxt; w; w=nw){
		nw = w->next;
		if(w->type != Wplain)
			continue;
		while(w->text[0]){
			p = w->text;
			for(q=p; *q && *q != '['; q++)
				;
			if(*q == '\0')
				break;
			for(r=q; *r && *r != ']'; r++)
				;
			if(*r == '\0')
				break;
			*q = '\0';
			*r = '\0';
			s = w->text;
			w->text = estrdup(w->text);
			w->next = mklink(q+1);
			w = w->next;
			w->next = mkwtxt(Wplain, estrdup(r+1));
			free(s);
			w = w->next;
			w->next = nw;
		}
		assert(w->next == nw);
	}
	return wtxt;	
}

static int
ismanchar(int c)
{
	return ('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| ('0' <= c && c <= '9')
		|| c=='_' || c=='-' || c=='.' || c=='/'
		|| (c < 0);	/* UTF */
}

static Wpage*
findmanref(char *p, char **beginp, char **endp)
{
	char *q, *r;
	Wpage *w;

	q=p;
	for(;;){
		for(; q[0] && (q[0] != '(' || !isdigit(q[1]) || q[2] != ')'); q++)
			;
		if(*q == '\0')
			break;
		for(r=q; r>p && ismanchar(r[-1]); r--)
			;
		if(r==q){
			q += 3;
			continue;
		}
		*q = '\0';
		w = mkwtxt(Wman, estrdup(r));
		*beginp = r;
		*q = '(';
		w->section = q[1]-'0';
		*endp = q+3;
		return w;
	}
	return nil;
}

/*
 * Parse Wplains, looking for man page references.
 * This should be done by using a plumb(6)-style 
 * control file rather than hard-coding things here.
 */
static Wpage*
wman(Wpage *wtxt)
{
	char *q, *r;
	Wpage *w, *mw, *nw;

	for(w=wtxt; w; w=nw){
		nw = w->next;
		if(w->type != Wplain)
			continue;
		while(w->text[0]){
			if((mw = findmanref(w->text, &q, &r)) == nil)
				break;
			*q = '\0';
			w->next = mw;
			w = w->next;
			w->next = mkwtxt(Wplain, estrdup(r));
			w = w->next;
			w->next = nw;
		}
		assert(w->next == nw);
	}
	return wtxt;	
}

static char *lower = "abcdefghijklmnopqrstuvwxyz";
static char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
Wpage*
Brdpage(char *(*rdline)(void*,int), void *b)
{
	char *p;
	int waspara;
	Wpage *w, **pw;

	w = nil;
	pw = &w;
	waspara = 1;
	while((p = rdline(b, '\n')) != nil){
		if(p[0] != '!')
			p = strcondense(p, 1);
		if(p[0] == '\0'){
			if(waspara==0){
				waspara=1;
				*pw = mkwtxt(Wpara, nil);
				pw = &(*pw)->next;
			}
			continue;
		}
		waspara = 0;
		switch(p[0]){
		case '*':
			*pw = mkwtxt(Wbullet, nil);
			pw = &(*pw)->next;
			*pw = mkwtxt(Wplain, estrdup(p+1));
			pw = &(*pw)->next;
			break;
		case '!':
			*pw = mkwtxt(Wpre, estrdup(p[1]==' '?p+2:p+1));
			pw = &(*pw)->next;
			break;
		default:
			if(strpbrk(p, lower)==nil && strpbrk(p, upper)){
				*pw = mkwtxt(Wheading, estrdup(p));
				pw = &(*pw)->next;
				continue;
			}
			*pw = mkwtxt(Wplain, estrdup(p));
			pw = &(*pw)->next;
			break;
		}
	}
	if(w == nil)
		werrstr("empty page");
	
	*pw = nil;
	w = wcondense(w);
	w = wlink(w);
	w = wman(w);

	return w;		
}

void
printpage(Wpage *w)
{
	for(; w; w=w->next){
		switch(w->type){
		case Wpara:
			print("para\n");
			break;
		case Wheading:
			print("heading '%s'\n", w->text);
			break;
		case Wbullet:
			print("bullet\n");
			break;
		case Wlink:
			print("link '%s' '%s'\n", w->text, w->url);
			break;
		case Wman:
			print("man %d %s\n", w->section, w->text);
			break;
		case Wplain:
			print("plain '%s'\n", w->text);
			break;
		case Wpre:
			print("pre '%s'\n", w->text);
			break;
		}
	}
}
