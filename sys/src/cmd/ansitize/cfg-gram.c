#include "a.h"

Ygram*
yynewgram(void)
{
	Ygram *g;
	
	g = mallocz(sizeof(Ygram), 1);
	g->valsize = -1;
	yynewsym(g, "EOF", nil, 0, 0);
	yynewsym(g, "error", nil, 0, 0);
	return g;
}

void
yyfreegram(Ygram *g)
{
	Yrule *r;
	Ysym *s;
	Ystate *z;

	foreach(state, z, g)
		yyfreestate(z);
	free(g->state);

	foreach(sym, s, g){
		yyfreeset(s->first);
		yyfreeset(s->starts);
		free(s->name);
		free(s->type);
		free(s->rule);
		free(s);
	}
	free(g->sym);
	
	foreach(rule, r, g){
		free(r->right);
		free(r);
	}
	free(g->rule);

	free(g);
}

