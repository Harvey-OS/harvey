#include "a.h"

void
yycomp(Ygram *g)
{
	int i;

	if(g->dirty == 0)
		return;

	for(i=0; i<g->nstate; i++)
		yyfreestate(g->state[i]);
	free(g->state);
	g->state = nil;
	g->nstate = 0;
	memset(g->statehash, 0, sizeof g->statehash);

	yycfgcomp(g);

	g->seqnum++;
	g->dirty = 0;
}

void
yycompile(Ygram *g)
{
	int i, j;
	Ystate *z;
	
	yycomp(g);

	yyinitstate(g, nil);
	for(i=0; i<g->nstate; i++){
		z = g->state[i];
		if(!z->compiled)
			yycompilestate(g, z);
		for(j=0; j<z->nshift; j++)
			if(!z->shift[j].state)
				z->shift[j].state = yycomputeshift(g, z, z->shift[j].sym);
	}
}

int
yycompilestate(Ygram *g, Ystate *z)
{
	int i, n;
	Yrpos *p;
	Yset *set;

assert(!z->compiled);
//	g->stats.yycompilestate++;
	if(z->shift || z->nshift){
		free(z->shift);
		z->shift = nil;
		z->nshift = 0;
	}
	if(z->reduce || z->nreduce){
		free(z->reduce);
		z->reduce = nil;
		z->nreduce = 0;
	}

	/*
	 * Compute shift set
	 */
	set = yynewset(g->nsym);
	if(set == nil)
		return -1;
	for(i=0; i<z->npos; i++){
		p = &z->pos[i];
		if(p->i < p->r->nright)
			yyaddbit(set, p->r->right[p->i]->n);
	}
	n = yycountset(set);
	z->shift = malloc(n*sizeof(z->shift[0]));
	if(z->shift == nil)
		return -1;
	z->nshift = n;
	n = 0;
	for(i=0; i<g->nsym; i++){
		if(yyinset(set, i)){
			z->shift[n].sym = g->sym[i];
			z->shift[n].state = nil;
			n++;
		}
	}
	yyfreeset(set);

	/*
	 * Compute reduce set
	 */
	n = 0;
	for(i=0; i<z->npos; i++){
		p = &z->pos[i];
		if(p->i == p->r->nright)
			n++;
	}
	z->reduce = malloc(n*sizeof z->reduce[0]);
	if(z->reduce == nil)
		return -1;
	z->nreduce = n;
	n = 0;
	for(i=0; i<z->npos; i++){
		p = &z->pos[i];
		if(p->i == p->r->nright)
			z->reduce[n++] = p->r;
	}

	z->compiled = 1;
	return 0;
}

/*
 * on-the-fly compilation
 */
Ystate*
yyinitstate(Ygram *g, char *start)
{
	Ystate *z;
	Ysym *s, *s0;
	
	if(start)
		s = yylooksym(g, start);
	else{
		if(g->nrule == 0){
		//	werrstr("no rules");
			return nil;
		}
		s = g->rule[0]->left;
	}
	
	if(s->parseme == nil){
		s0 = yynewsym(g, smprint("$%s", s->name), nil, 0, Ynonterm);
		yyrulesym(g, nil, s0, nil, s, yylooksym(g, "EOF"), nil);
		s->parseme = s0->rule[0];
		s->parseme->toplevel = 1;
		yycomp(g);
	}
	
	z = mallocz(sizeof *z, 1);
	z->g = g;
	z->pos = mallocz(32*sizeof z->pos[0], 1);
	z->pos[0].r = s->parseme;
	z->npos = 1;
//	fprint(2, "state0 %s(%d; %d) %lY\n", s->name, s->nrule, z->npos, z);
	z = yycanonstate(g, z);
//	fprint(2, "canonstate %lY\n", z);
	return z;
}

