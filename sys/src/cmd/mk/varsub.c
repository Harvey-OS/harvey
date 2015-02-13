/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"mk.h"

static	Word		*subsub(Word*, int8_t*, int8_t*);
static	Word		*expandvar(int8_t**);
static	Bufblock	*varname(int8_t**);
static	Word		*extractpat(int8_t*, int8_t**, int8_t*,
					      int8_t*);
static	int		submatch(int8_t*, Word*, Word*, int*,
					  int8_t**);
static	Word		*varmatch(int8_t *);

Word *
varsub(int8_t **s)
{
	Bufblock *b;
	Word *w;

	if(**s == '{')		/* either ${name} or ${name: A%B==C%D}*/
		return expandvar(s);

	b = varname(s);
	if(b == 0)
		return 0;

	w = varmatch(b->start);
	freebuf(b);
	return w;
}

/*
 *	extract a variable name
 */
static Bufblock*
varname(int8_t **s)
{
	Bufblock *b;
	int8_t *cp;
	Rune r;
	int n;

	b = newbuf();
	cp = *s;
	for(;;){
		n = chartorune(&r, cp);
		if (!WORDCHR(r))
			break;
		rinsert(b, r);
		cp += n;
	}
	if (b->current == b->start){
		SYNERR(-1);
		fprint(2, "missing variable name <%s>\n", *s);
		freebuf(b);
		return 0;
	}
	*s = cp;
	insert(b, 0);
	return b;
}

static Word*
varmatch(int8_t *name)
{
	Word *w;
	Symtab *sym;
	
	sym = symlook(name, S_VAR, 0);
	if(sym){
			/* check for at least one non-NULL value */
		for (w = sym->u.ptr; w; w = w->next)
			if(w->s && *w->s)
				return wdup(w);
	}
	return 0;
}

static Word*
expandvar(int8_t **s)
{
	Word *w;
	Bufblock *buf;
	Symtab *sym;
	int8_t *cp, *begin, *end;

	begin = *s;
	(*s)++;						/* skip the '{' */
	buf = varname(s);
	if (buf == 0)
		return 0;
	cp = *s;
	if (*cp == '}') {				/* ${name} variant*/
		(*s)++;					/* skip the '}' */
		w = varmatch(buf->start);
		freebuf(buf);
		return w;
	}
	if (*cp != ':') {
		SYNERR(-1);
		fprint(2, "bad variable name <%s>\n", buf->start);
		freebuf(buf);
		return 0;
	}
	cp++;
	end = charin(cp , "}");
	if(end == 0){
		SYNERR(-1);
		fprint(2, "missing '}': %s\n", begin);
		Exit();
	}
	*end = 0;
	*s = end+1;
	
	sym = symlook(buf->start, S_VAR, 0);
	if(sym == 0 || sym->u.value == 0)
		w = newword(buf->start);
	else
		w = subsub(sym->u.ptr, cp, end);
	freebuf(buf);
	return w;
}

static Word*
extractpat(int8_t *s, int8_t **r, int8_t *term, int8_t *end)
{
	int save;
	int8_t *cp;
	Word *w;

	cp = charin(s, term);
	if(cp){
		*r = cp;
		if(cp == s)
			return 0;
		save = *cp;
		*cp = 0;
		w = stow(s);
		*cp = save;
	} else {
		*r = end;
		w = stow(s);
	}
	return w;
}

static Word*
subsub(Word *v, int8_t *s, int8_t *end)
{
	int nmid;
	Word *head, *tail, *w, *h;
	Word *a, *b, *c, *d;
	Bufblock *buf;
	int8_t *cp, *enda;

	a = extractpat(s, &cp, "=%&", end);
	b = c = d = 0;
	if(PERCENT(*cp))
		b = extractpat(cp+1, &cp, "=", end);
	if(*cp == '=')
		c = extractpat(cp+1, &cp, "&%", end);
	if(PERCENT(*cp))
		d = stow(cp+1);
	else if(*cp)
		d = stow(cp);

	head = tail = 0;
	buf = newbuf();
	for(; v; v = v->next){
		h = w = 0;
		if(submatch(v->s, a, b, &nmid, &enda)){
			/* enda points to end of A match in source;
			 * nmid = number of chars between end of A and start of B
			 */
			if(c){
				h = w = wdup(c);
				while(w->next)
					w = w->next;
			}
			if(PERCENT(*cp) && nmid > 0){	
				if(w){
					bufcpy(buf, w->s, strlen(w->s));
					bufcpy(buf, enda, nmid);
					insert(buf, 0);
					free(w->s);
					w->s = strdup(buf->start);
				} else {
					bufcpy(buf, enda, nmid);
					insert(buf, 0);
					h = w = newword(buf->start);
				}
				buf->current = buf->start;
			}
			if(d && *d->s){
				if(w){

					bufcpy(buf, w->s, strlen(w->s));
					bufcpy(buf, d->s, strlen(d->s));
					insert(buf, 0);
					free(w->s);
					w->s = strdup(buf->start);
					w->next = wdup(d->next);
					while(w->next)
						w = w->next;
					buf->current = buf->start;
				} else
					h = w = wdup(d);
			}
		}
		if(w == 0)
			h = w = newword(v->s);
	
		if(head == 0)
			head = h;
		else
			tail->next = h;
		tail = w;
	}
	freebuf(buf);
	delword(a);
	delword(b);
	delword(c);
	delword(d);
	return head;
}

static int
submatch(int8_t *s, Word *a, Word *b, int *nmid, int8_t **enda)
{
	Word *w;
	int n;
	int8_t *end;

	n = 0;
	for(w = a; w; w = w->next){
		n = strlen(w->s);
		if(strncmp(s, w->s, n) == 0)
			break;
	}
	if(a && w == 0)		/*  a == NULL matches everything*/
		return 0;

	*enda = s+n;		/* pointer to end a A part match */
	*nmid = strlen(s)-n;	/* size of remainder of source */
	end = *enda+*nmid;
	for(w = b; w; w = w->next){
		n = strlen(w->s);
		if(strcmp(w->s, end-n) == 0){
			*nmid -= n;
			break;
		}
	}
	if(b && w == 0)		/* b == NULL matches everything */
		return 0;
	return 1;
}
