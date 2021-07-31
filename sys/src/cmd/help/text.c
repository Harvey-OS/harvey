#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

void
textdraw(Text *t)
{
	int g;

	frinit(t, t->r, font, &screen);
	g = t->outline;
	t->outline = 0;
	frinsert(t, t->s+t->org, t->s+t->n, 0);
	highlight(t);
	setoutline(t, g);
}

void
textinsert(Text *t, Rune *s, ulong q0, int full, int sel)
{
	long p0;
	ulong n;

	if(sel)
		newsel(t);
	n = rstrlen(s);
	Strinsert(t, s, n, q0);
	p0 = q0-t->org;
	if(p0 < 0)
		t->org += n;
	else if(p0 <= t->nchars)
		frinsert(t, s, s+n, p0);
	t->q0 = q0;
	if(!full)
		t->q0 += n;
	t->q1 = q0+n;
	highlight(t);
}

Rune*
rstrchr(Rune *s, Rune c)
{
	Rune c1;

	if(c == 0) {
		while(*s++)
			;
		return s-1;
	}

	while(c1 = *s++)
		if(c1 == c)
			return s-1;
	return 0;
}

int
rstrncmp(Rune *s1, Rune *s2, int n)
{

	while(--n >= 0 && *s1 == *s2++)
		if(*s1++ == 0)
			return 0;
	if(n < 0)
		return 0;
	return *s1 - *(s2-1);
}

int
rstrlen(Rune *s)
{
	int i;

	i = 0;
	while(*s++)
		i++;
	return i;
}

Rune*
tmprstr(char *s)
{
	int j, w;
	Rune *r;

	r = emalloc((utflen(s)+1)*sizeof(Rune));
	for(j=0; *s; s+=w, j++)
		w = chartorune(r+j, s);
	r[j] = 0;
	return r;
}



char*
cstr(Rune *r, long n)
{
	int i, w;
	char *c, *d;

	w = 0;
	for(i=0; i<n; i++)
		w += runelen(r[i]);
	c = emalloc(w+1);
	d = c;
	for(i=0; i<n; i++)
		d += runetochar(d, r+i);
	*d = 0;
	return c;
}
