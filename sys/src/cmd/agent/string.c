#include "agent.h"

static void
grow(String *s, int n)
{
	if(n > s->m) {
		s->s = erealloc(s->s, n);
		s->m = n;
	}
}

void
append(String *s, char *b)
{
	int on;

	on = s->n;
	s->n += strlen(b);
	grow(s, s->n);
	strcpy(s->s+on-1, b);
}

void
appendn(String *s, char *b, int n)
{
	int on;

	on = s->n;
	s->n += n;
	grow(s, s->n);
	memmove(s->s+on-1, b, n);
	s->s[s->n-1] = '\0';
}

void
resetstring(String *s)
{
	s->n = 1;
	grow(s, s->n);
	s->s[0] = '\0';
}

String*
mkstring(char *a)
{
	String *s;

	s = emalloc(sizeof(*s));
	s->n = strlen(a)+1;
	grow(s, s->n);
	strcpy(s->s, a);
	s->ref = 1;
	return s;
}

void
freestring(String *s)
{
	if(s == nil)
		return;

	memset(s->s, 0xFF, s->m);
	free(s->s);
	s->ref = 0;
	s->s = (char*)0xDeadBeef;
	s->n = 100;
	s->m = 100;
	free(s);
}

String*
copystring(String *s)
{
	String *t;

	t = emalloc(sizeof(*t));
	t->n = s->n;
	grow(t, t->n);
	t->ref = 1;
	strcpy(t->s, s->s);
	return t;
}
