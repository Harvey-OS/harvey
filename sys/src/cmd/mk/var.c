/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"mk.h"

void
setvar(char *name, void *value)
{
	symlook(name, S_VAR, value)->u.ptr = value;
	symlook(name, S_MAKEVAR, (void*)"");
}

static void
print1(Symtab *s)
{
	Word *w;

	Bprint(&bout, "\t%s=", s->name);
	for (w = s->u.ptr; w; w = w->next)
		Bprint(&bout, "'%s'", w->s);
	Bprint(&bout, "\n");
}

void
dumpv(char *s)
{
	Bprint(&bout, "%s:\n", s);
	symtraverse(S_VAR, print1);
}

char *
shname(char *a)
{
	Rune r;
	int n;

	while (*a) {
		n = chartorune(&r, a);
		if (!WORDCHR(r))
			break;
		a += n;
	}
	return a;
}
