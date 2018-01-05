/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"mk.h"

static	Word	*nextword(char**);

Word*
newword(char *s)
{
	Word *w;

	w = (Word *)Malloc(sizeof(Word));
	w->s = strdup(s);
	w->next = 0;
	return(w);
}

Word *
stow(char *s)
{
	Word *head, *w, *new;

	w = head = 0;
	while(*s){
		new = nextword(&s);
		if(new == 0)
			break;
		if (w)
			w->next = new;
		else
			head = w = new;
		while(w->next)
			w = w->next;

	}
	if (!head)
		head = newword("");
	return(head);
}

char *
wtos(Word *w, int sep)
{
	Bufblock *buf;
	char *cp;

	buf = newbuf();
	for(; w; w = w->next){
		for(cp = w->s; *cp; cp++)
			insert(buf, *cp);
		if(w->next)
			insert(buf, sep);
	}
	insert(buf, 0);
	cp = strdup(buf->start);
	freebuf(buf);
	return(cp);
}

Word*
wdup(Word *w)
{
	Word *v, *new, *base;

	v = base = 0;
	while(w){
		new = newword(w->s);
		if(v)
			v->next = new;
		else
			base = new;
		v = new;
		w = w->next;
	}
	return base;
}

void
delword(Word *w)
{
	Word *v;

	while(v = w){
		w = w->next;
		if(v->s)
			free(v->s);
		free(v);
	}
}

/*
 *	break out a word from a string handling quotes, executions,
 *	and variable expansions.
 */
static Word*
nextword(char **s)
{
	Bufblock *b;
	Word *head, *tail, *w;
	Rune r;
	char *cp;
	int empty;

	cp = *s;
	b = newbuf();
restart:
	head = tail = 0;
	while(*cp == ' ' || *cp == '\t')		/* leading white space */
		cp++;
	empty = 1;
	while(*cp){
		cp += chartorune(&r, cp);
		switch(r)
		{
		case ' ':
		case '\t':
		case '\n':
			goto out;
		case '\\':
		case '\'':
		case '"':
			empty = 0;
			cp = expandquote(cp, r, b);
			if(cp == 0){
				fprint(2, "missing closing quote: %s\n", *s);
				Exit();
			}
			break;
		case '$':
			w = varsub(&cp);
			if(w == 0){
				if(empty)
					goto restart;
				break;
			}
			empty = 0;
			if(b->current != b->start){
				bufcpy(b, w->s, strlen(w->s));
				insert(b, 0);
				free(w->s);
				w->s = strdup(b->start);
				b->current = b->start;
			}
			if(head){
				bufcpy(b, tail->s, strlen(tail->s));
				bufcpy(b, w->s, strlen(w->s));
				insert(b, 0);
				free(tail->s);
				tail->s = strdup(b->start);
				tail->next = w->next;
				free(w->s);
				free(w);
				b->current = b->start;
			} else
				tail = head = w;
			while(tail->next)
				tail = tail->next;
			break;
		default:
			empty = 0;
			rinsert(b, r);
			break;
		}
	}
out:
	*s = cp;
	if(b->current != b->start){
		if(head){
			cp = b->current;
			bufcpy(b, tail->s, strlen(tail->s));
			bufcpy(b, b->start, cp-b->start);
			insert(b, 0);
			free(tail->s);
			tail->s = strdup(cp);
		} else {
			insert(b, 0);
			head = newword(b->start);
		}
	}
	freebuf(b);
	return head;
}

void
dumpw(char *s, Word *w)
{
	Bprint(&bout, "%s", s);
	for(; w; w = w->next)
		Bprint(&bout, " '%s'", w->s);
	Bputc(&bout, '\n');
}
