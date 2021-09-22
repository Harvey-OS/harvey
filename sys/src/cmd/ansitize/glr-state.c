#include "a.h"

/*
 * state management
 *
 * pos lists are sorted, first decreasing by i, then increasing by r->n.
 */
void
yyfreestate(Ystate *z)
{
	free(z->pos);
	free(z);
}

int
yystatecmp(Ystate *z, Ystate *y)
{
	int i;

	if(z->npos != y->npos)
		return 1;
	for(i=0; i<z->npos; i++)
		if(z->pos[i].i != y->pos[i].i || z->pos[i].r != y->pos[i].r)
			return 1;
	return 0;
}


void
yycheckstate(Ystate *z)
{
	int i;
	
	for(i=0; i<z->npos-1; i++){
		if(z->pos[i].i < z->pos[i+1].i
		|| z->pos[i].r->n > z->pos[i+1].r->n){
			print("bad sort\n");
			abort();
		}
	}
}

static int
addtostate(Ystate *z, Yrule *r, int i)
{
	int j;
	Yrpos *p;
	
	for(j=z->npos-1; j>=0; j--){
		p = &z->pos[j];
		if(p->r == r && p->i == i)
			return 0;
		if(p->i > i || (p->i==i && p->r < r))
			break;
	}
	if(z->npos%32==0)
		z->pos = realloc(z->pos, (z->npos+32)*sizeof(z->pos[0]));
	p = &z->pos[++j];
	if(z->npos != j)
		memmove(p+1, p, (z->npos-j)*sizeof p[0]);
	z->npos++;
	p->r = r;
	p->i = i;
	return 1;
}

static void
closure(Ygram *g, Ystate *z)
{
	int did;
	Yrpos *p;
	Yrule *r;
	Ysym *s;

	USED(g);
	do{
		did = 0;
		foreachptr(pos, p, z){
			if(p->i == p->r->nright)
				continue;
			s = p->r->right[p->i];
			foreach(rule, r, s)
				did += addtostate(z, r, 0);
		}
	}while(did);
}

static uint
hashstate(Ystate *z)
{
	int i;
	uint n;
	
	n = 0;
	for(i=0; i<z->npos; i++)
		n += 37*z->pos[i].r->n+101*z->pos[i].i;
	return n%nelem(z->g->statehash);
}

Ystate*
yycanonstate(Ygram *g, Ystate *z)
{
	uint h;
	Ystate *y, **l, **a;

	/* checksort(z); */
	closure(g, z);
	/* checksort(z); */
	g = z->g;
	h = hashstate(z);
	for(l=&g->statehash[h]; y=*l; l=&y->next){
		if(yystatecmp(z, y) == 0){
			yyfreestate(z);
			if(l != &g->statehash[h]){
				*l = y->next;
				y->next = g->statehash[h];
				g->statehash[h] = y;
			}
			return y;
		}
//		// g->stats.hashwalk++;
	}

	if(g->nstate%32 == 0){
		a = realloc(g->state, (g->nstate+32)*sizeof g->state[0]);
		if(a == nil){
			yyfreestate(z);
			return nil;
		}
		g->state = a;
	}

	z->next = g->statehash[h];
	g->statehash[h] = z;

	z->n = g->nstate;
	g->state[g->nstate++] = z;
	return z;
}

int
yystatefmt(Fmt *fmt)
{
	int i, islong;
	Ystate *s;
	Yrpos *p;

	s = va_arg(fmt->args, Ystate*);
	if(s == nil)
		return fmtprint(fmt, "<nil>");

	islong = fmt->flags&FmtLong;
	for(i=0; i<s->npos; i++){
		p = &s->pos[i];
		if(!islong)
		if(i>0)
		if(p->i == 0)
		if(p->r->left != s->pos[0].r->left)
			break;
		if(i > 0)
			fmtprint(fmt, " || ");
		fmtprint(fmt, "%.*R", p->i, p->r);
	}
	return 0;
}

Ystate*
yycomputeshift(Ygram *g, Ystate *z, Ysym *s)
{
	int i, didshift;
	Yrpos *p;
	Ystate *y;

//	g->stats.yycomputeshift++;
	y = mallocz(sizeof *y, 1);
	y->g = z->g;
	didshift = 0;
again:
	for(i=0; i<z->npos; i++){
		p = &z->pos[i];
		if(p->i < p->r->nright && p->r->right[p->i]==s){
			didshift = 1;
			if(addtostate(y, p->r, p->i+1) < 0){
				yyfreestate(y);
				return nil;
			}
		}
	}
	if(!didshift && g->wildcard){
		didshift = 1;
		s = g->wildcard;
		goto again;
	}
	return yycanonstate(g, y);
}

int
yycanshift(Ygram *g, Ystate *z, Ysym *s, Ystate **nz)
{
	int i;
	Yshift *sh, *shwildcard;
	
	*nz = nil;
//	g->stats.canshift++;
	if(s == nil){
//		g->stats.canshiftzero++;
		return 0;
	}
	if(!z->compiled)
		if(yycompilestate(g, z) < 0)
			return -1;
	shwildcard = nil;
	for(i=0; i<z->nshift; i++){
		sh = &z->shift[i];
		if(sh->sym == g->wildcard){
			shwildcard = sh;
			continue;
		}
		if(sh->sym == s){
		havesh:
			if(!sh->state){
				sh->state = yycomputeshift(g, z, s);
				if(sh->state == nil)
					return -1;
			}
			*nz = sh->state;
			return 1;
		}
	}
	if(shwildcard && s != g->eof){
		sh = shwildcard;
		goto havesh;
	}
//	g->stats.canshiftzero++;
	return 0;
}

