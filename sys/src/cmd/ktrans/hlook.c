
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "ktrans.h"

Rune *
hlook(Rune *key)
{
	Rune *h, *l, *m;
	Rune *v;
	int r;

	l = &dict[0];
	h = &dict[ndict];
	while (l<=h) {
		m = l + (h-l)/2;
		if ((r = hcmp(key, m, &v)) < 0)
			h = m-1;
		else if (r>0)
			l = m+1;
		else
			return v;
	}
	return 0;
}

int
hcmp(Rune *k, Rune *t, Rune **v)
{
	Rune c;

	while (t>dict && *t!=L'\n')
		--t;
	++t;
	*v = t;
	while (*k == *t && *k != L'\0')
		++k, ++t;
	c = *t;
	if (c==L' ' || c==L'\t')
		c = L'\0';
	return (*k - c);
}
	
int
nrune(char *p)
{
	int n = 0;
	Rune r;

	while (*p) {
		p += chartorune(&r, p);
		n++;
	}
	return n;
}

void
unrune(char *p, Rune *r)
{
	while (*r) {
		p += runetochar(p, r);
		r++;
	}
	*p = '\0';
}
