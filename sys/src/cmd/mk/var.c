#include	"mk.h"

void
setvar(char *name, char *value)
{
	symlook(name, S_VAR, value)->value = value;
	symlook(name, S_MAKEVAR, "");
}

static Envy *nextv;

static void
vcopy(Symtab *s)
{
	char **p;
	extern char *myenv[];

	if(symlook(s->name, S_NOEXPORT, (char *)0))
		return;
	for(p = myenv; *p; p++)
		if(strcmp(*p, s->name) == 0)
			return;
	envinsert(s->name, (Word *) s->value);
}

void
vardump(void)
{
	symtraverse(S_VAR, vcopy);
	envinsert(0, 0);
}

static void
print1(Symtab *s)
{
	Word *w;

	Bprint(&stdout, "\t%s=", s->name);
	for (w = (Word *) s->value; w; w = w->next)
		Bprint(&stdout, "'%s'", w->s);
	Bprint(&stdout, "\n");
}

void
dumpv(char *s)
{
	Bprint(&stdout, "%s:\n", s);
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
