#include "a.h"

/*
 * Compute whether each symbol can produce an empty string.
 */
static void
empty(Ygram *g)
{
	int did, flag;
	Yrule *r;
	Ysym *s, *ss;
	
	foreach(sym, s, g)
		s->flags &= ~YsymEmpty;

	/* could keep a queue for speed, but not a bottleneck */
	do{
		did = 0;
		foreach(sym, s, g){
			if(s->flags&YsymEmpty)
				continue;
			foreach(rule, r, s){
				flag = YsymEmpty;
				foreach(right, ss, r)
					flag &= ss->flags;
				if(flag){
					s->flags |= flag;
					did = 1;
				}
			}
		}
	}while(did);
}

/*
 * Compute first set for each symbol.
 */
static void
first(Ygram *g)
{
	int did;
	Yrule *r;
	Ysym *s, *ss;

	/* allocate sets, initialize terminals */
	foreach(sym, s, g){
		yyfreeset(s->first);
		s->first = yynewset(g->nsym);
		yyaddbit(s->first, s->n);
	}

	/* iterate for non-terminals */
	do{
		did = 0;
		foreach(sym, s, g){
			foreach(rule, r, s){
				foreach(right, ss, r){
					did += yyaddset(s->first, ss->first);
					if(!(ss->flags&YsymEmpty))
						break;
				}
			}
		}
	}while(did);
}

/*
 * Compute first set for each symbol.
 */
static void
starts(Ygram *g)
{
	int did;
	Yrule *r;
	Ysym *s;

	/* allocate sets, initialize terminals */
	foreach(sym, s, g){
		yyfreeset(s->starts);
		s->starts = yynewset(g->nsym);
		yyaddbit(s->starts, s->n);
	}

	/* iterate for non-terminals */
	do{
		did = 0;
		foreach(sym, s, g)
			foreach(rule, r, s)
				if(r->nright)
					did += yyaddset(s->starts, r->right[0]->starts);
	}while(did);
}

/*
 * Compute follow set for each symbol.
 */
static void
follow(Ygram *g)
{
	int did, k, lastnonempty, l;
	Yrule *r;
	Ysym **rhs, *s;

	foreach(sym, s, g){
		yyfreeset(s->follow);
		s->follow = yynewset(g->nsym);
	}

	do{
		did = 0;
		foreach(rule, r, g){
			s = r->left;
			rhs = r->right;
			lastnonempty = 0;
			for(k=0; k<r->nright; k++){
				/* X => ... Y Z ..., so follow(Y) += first(Z) */
				for(l=lastnonempty; l<k; l++)
					did += yyaddset(rhs[l]->follow, rhs[k]->first);
				if(!(rhs[k]->flags&YsymEmpty))
					lastnonempty = k;
			}
			/* X => ... Y, so follow(Y) += follow(X) */
			for(l=lastnonempty; l<r->nright; l++)
				did += yyaddset(rhs[l]->follow, s->follow);
		}
	}while(did);
}

void
yycfgcomp(Ygram *g)
{
	empty(g);
	first(g);
	starts(g);
	follow(g);
}
