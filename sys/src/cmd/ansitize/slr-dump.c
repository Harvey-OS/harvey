#include "a.h"

void
yydumptables(Ygram *g)
{
	int i, j, k, first;
	Yrule *r;
	Ystate *z;
	Ysym *s;
	
	print("\n================================\n");

	for(i=0; i<g->nsym; i++){
		s = g->sym[i];
		print("%s:%s\n", s->name, 
			(s->flags&YsymEmpty) ? " canempty" : "");
		print("\tfirst:");
		for(j=0; j<g->nsym; j++)
			if(yyinset(s->first, g->sym[j]->n))
				print(" %s", g->sym[j]->name);
		print("\n");
		print("\tfollow:");
		for(j=0; j<g->nsym; j++)
			if(yyinset(s->follow, g->sym[j]->n))
				print(" %s", g->sym[j]->name);
		print("\n");
	}
	print("\n");

/*	compileall(g); */
	for(i=0; i<g->nstate; i++){
		z = g->state[i];
		print("#state %d: %d shift, %d reduce, %d pos\n",
			z->n, z->nshift, z->nreduce, z->npos);
		print("%d: %lY\n", i, z);
		for(j=0; j<z->nshift; j++){
			print("\t%s: shift %d\n", z->shift[j].sym ? z->shift[j].sym->name : "<>", z->shift[j].state ? z->shift[j].state->n : -1);
		}
		for(j=0; j<z->nreduce; j++){
			r = z->reduce[j];
			print("\t");
			first = 1;
			for(k=0; k<g->nsym; k++){
				if(yyinset(r->left->follow, k)){
					if(first)
						first = 0;
					else
						print(" ");
					print("%s", g->sym[k]->name);
				}
			}
			print(": reduce %R\n", r);
		}
	}
}

