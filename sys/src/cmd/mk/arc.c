#include	"mk.h"

Arc *
newarc(Node *n, Rule *r, char *stem, Resub *match)
{
	Arc *a;

	a = (Arc *)Malloc(sizeof(Arc));
	a->n = n;
	a->r = r;
	a->stem = strdup(stem);
	memmove((char *)a->match, (char *)match, (COUNT)sizeof a->match);
	a->next = 0;
	a->flag = 0;
	a->prog = r->prog;
	return(a);
}

void
dumpa(char *s, Arc *a)
{
	char buf[1024];

	sprint(buf, "%s    ", (*s == ' ')? s:"");
	Bprint(&stdout, "%sArc@%ld: n=%ld r=%ld flag=0x%x stem='%s'",
		s, a, a->n, a->r, a->flag, a->stem);
	if(a->prog)
		Bprint(&stdout, " prog='%s'", a->prog);
	Bprint(&stdout, "\n");
	if(a->n)
		dumpn(buf, a->n);
}

void
nrep(void)
{
	Symtab *sym;
	Word *w;

	if(sym = symlook("NREP", S_VAR, (char *)0)) {
		w = (Word *) sym->value;
		if (w && w->s && *w->s)
			nreps = atoi(w->s);
	}
	if(nreps < 1)
		nreps = 1;
	if(DEBUG(D_GRAPH))
		Bprint(&stdout, "nreps = %d\n", nreps);
}
