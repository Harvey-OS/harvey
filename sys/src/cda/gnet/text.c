#include "geom.h"
#include "thing.h"
#include "text.h"

void TextList::walk(Vector *v, int (*f)(...))
{
	register i;
	register Text *t;
	for (i = 0; i < n; i++)
		if ((t = text(i))->used == 0)
			if ((*f)(v,t))
				t->used = 1;
}

void Text::merge(Text *x)
{
	if (p.y < x->p.y)
		t = x->s;
	else {
		t = s;
		s = x->s;
	}
}

int TextList::prop(Text *s)
{
	register i;
	register Text *t;
	for (i = 0; i < n; i++) {
		t = text(i);
		if (t->p.x == s->p.x && t->p.y < s->p.y && (t->p.y + 16) >= s->p.y) {
			t->t = s->s;
			return 1;
		}
	}
	return 0;
}
