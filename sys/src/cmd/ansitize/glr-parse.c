/*
 * GLR parser. 
 *
 * Thanks to Scott McPeak for writing UCB TR CSD-2-1214.
 */

#include "a.h"

typedef struct Ypathelem Ypathelem;
struct Ypathelem
{
	Yedge *edge;
	Ypathelem *next;
};

static void
die(char *fmt, ...)
{
	va_list arg;
	
	fprint(2, "fatal: ");
	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	fprint(2, "\n");
	exits("dead");
}

static Yedge*
mkedge(Ygram *g, Ysym *sym, void *val, Ystack *down, Yedge *sib)
{
	Yedge *e;
	
	e = mallocz(sizeof *e+g->valsize, 1);
	e->sib = sib;
	e->down = down;
	e->sym = sym;
	memmove(e->val, val, g->valsize);
	return e;
}

static Ystack*
mkstack(Ygram *g, Ystate *state, Yedge *edge)
{
	Ystack *s;
	
	USED(g);
	s = mallocz(sizeof *s, 1);
	s->edge = edge;
	s->state = state;
	return s;
}

static Ypath*
mkpath(Yrule *r, Ystack *sp, Ypathelem *pe)
{
	int i;
	Ypath *path;
	
	path = mallocz(sizeof *path+r->nright*sizeof path->edge[0], 1);
	path->sp = sp;
	path->rule = r;
	for(i=0; i<r->nright; i++){
		path->edge[i] = pe->edge;
		pe = pe->next;
	}
	return path;
}

static void
freepath(Ypath *path)
{
	free(path);
}

static void
addpath(Yparse *p, Ypath *path)
{
	if(p->nqueue >= p->mqueue){
		p->mqueue += 100;
		p->queue = realloc(p->queue, p->mqueue*sizeof p->queue[0]);
	}
	p->queue[p->nqueue++] = path;
}

Yparse*
yynewparse(void)
{
	return mallocz(sizeof(Yparse), 1);
}

static Yparse*
yyparseinit(Ygram *g, Yparse *p, char *start)
{
	char *val;
	
	if(p == nil)
		p = yynewparse();
	
	p->mtop = 32;
	p->top = malloc(p->mtop*sizeof p->top[0]);
	p->top[0] = mkstack(g, yyinitstate(g, start), nil);	
	p->ntop = 1;
	val = mallocz(20*g->valsize, 1);
	p->tokval = val;
	p->tmpval = val+g->valsize;
	p->tok = nil;
	p->seqnum = g->seqnum;
	return p;
}

int
yycanreduce(Ygram *g, Ystate *z, Ysym *s, Yrule **nr)
{
	int i;
	Yrule *r;
	Yset *follow;

	*nr = nil;
	r = nil;
	assert(z->compiled);
	for(i=0; i<z->nreduce; i++){
		follow = z->reduce[i]->left->follow;
		if(yyinset(follow, s->n)
		|| (s!=g->eof && g->wildcard && yyinset(follow, g->wildcard->n))){
			if(r == nil)
				r = z->reduce[i];
			else{
				fprint(2, "reduce/reduce conflict\n");
				abort();
			}
		}
	}
	*nr = r;
	return r ? 1 : 0;
}

#if 0
/*
 * Figure out what to do next, given current parse state and lookahead token.
 */
static int
action(Ygram *g, Yparse *p, Ystate *z, Ystate **nz, Yrule **r, Ysym *tok)
{
	*nz = nil;
	*r = nil;
	if(!z->compiled)
		yycompilestate(g, z);
	if(z->nshift==0 && z->nreduce==1){
		*r = z->reduce[0];
		return 1;
	}
	if(tok == nil)
		return 0;
	yycanshift(g, z, tok, nz);
	yycanreduce(g, z, tok, r);
	return 1;
}

/*
 * Recompute the parse states on the stack by re-shifting
 * if the grammar has been changed.
 */
static void reshift0(Ygram*, Ystack*, char*);
static void
reshift(Ygram *g, char *start, Yparse *p)
{
	int i;
	
	if(!g->dirty && g->seqnum == p->seqnum)
		return;
	yycomp(g);
	yyinitstate(g, start);
	for(i=0; i<p->ntop; i++)
		reshift0(g, p->top[i], start);
	p->seqnum = g->seqnum;
}
static void
reshift0(Ygram *g, Ystack *sp, char *start)
{
	Yedge *e;
	
	for(e=sp->edge; e; e=e->sib)
		reshift0(g, e->down, start);
	if(sp->edge == nil)
		sp->state = yyinitstate(g, start);
	else{
		if(!yycanshift(g, sp->edge->down->state, sp->edge->sym, &sp->state))
			die("lost parse");
	}
}
#endif

static void
printinput(Ystack *sp)
{
	if(sp->edge == nil)
		return;
	printinput(sp->edge->down);
	print(" %s", sp->edge->sym->name);
}

/*
 * Parse
 */
static void walkpaths(Ypathelem*, Yparse*, Ystack*, uint, Yrule*);
static void reducepath(Ygram*, Yparse*, Ypath*, Ysym*);

static void
doreduce(Ygram *g, Yparse *p, Ysym *lookahead)
{
	Yrule *r;
	Ystack *sp;
	Ypath *path;

	foreach(top, sp, p){
		yycanshift(g, sp->state, lookahead, &sp->doshift);
		if(!sp->state->compiled)
			yycompilestate(g, sp->state);
		foreach(reduce, r, sp->state){
			if(!yyinset(r->left->follow, lookahead->n))
				continue;
			if(sp->doshift && lookahead->prec && r->prec){
				if(lookahead->prec >= r->prec)
					continue;
				else
					sp->doshift = nil;
			}
			walkpaths(nil, p, sp, r->nright, r);
		}
	}

	/*
	 * Should keep work queue sorted in general case.  See section 4.2
	 * of McPeak's technical report.
	 */
	while(p->nqueue > 0){
		path = p->queue[--p->nqueue];
		reducepath(g, p, path, lookahead);
		freepath(path);
	}
}

static void
qreduce1(Ygram *g, Yparse *p, Ystack *sp, Yedge *e, Ysym *lookahead)
{
	Yrule *r;
	Ypathelem pe;
	
	if(!sp->state->compiled)
		yycompilestate(g, sp->state);
	foreach(reduce, r, sp->state){
		if(!yyinset(r->left->follow, lookahead->n))
			continue;
		if(sp->doshift && lookahead->prec && r->prec){
			if(lookahead->prec >= r->prec)
				continue;
			else
				sp->doshift = nil;
		}
		if(r->nright == 0){
			addpath(p, mkpath(r, sp, nil));
			continue;
		}
		pe.edge = e;
		pe.next = nil;
		walkpaths(&pe, p, e->down, r->nright-1, r);
	}
}

static void
walkpaths(Ypathelem *pep, Yparse *p, Ystack *sp, uint n, Yrule *r)
{
	Yedge *e;
	Ypathelem pe;
	
	if(n == 0){
		addpath(p, mkpath(r, sp, pep));
		return;
	}
	
	for(e=sp->edge; e; e=e->sib){
		pe.edge = e;
		pe.next = pep;
		walkpaths(&pe, p, e->down, n-1, r);
	}
}

static Ystack*
findtop(Yparse *p, Ystate *state)
{
	Ystack *sp;
	
	foreach(top, sp, p)
		if(sp->state == state)
			return sp;
	return nil;
}

static Ystack*
newtop(Yparse *p, Ystate *state)
{
	Ystack *sp;
	
	sp = mallocz(sizeof *sp, 1);
	sp->state = state;
	if(p->ntop >= p->mtop){
		p->mtop += 40;
		p->top = realloc(p->top, p->mtop*sizeof p->top[0]);
	}
	p->top[p->ntop++] = sp;
	return sp;
}

static Yedge*
findedge(Ystack *sp, Ystack *down)
{
	Yedge *e;
	
	for(e=sp->edge; e; e=e->sib)
		if(e->down == down)
			return e;
	return nil;
}

static void
reducepath(Ygram *g, Yparse *p, Ypath *path, Ysym *lookahead)
{
	int i;
	Ystack *sp;
	Yedge *e;
	Yrule *rule;
	Ystate *nextstate;

if(g->debug) print("reduce %R\n", path->rule);
	rule = path->rule;
	for(i=0; i<rule->nright; i++)
		memmove((char*)p->tmpval+(1+i)*g->valsize, path->edge[i]->val, g->valsize);
	memmove(p->tmpval, (char*)p->tmpval+g->valsize, g->valsize);
	if(rule->fn){
		rule->fn(g, rule, p->tmpval, (char*)p->tmpval+g->valsize);
	//	reshift(g, start, p);
	}
	*(Ysym**)p->tmpval = rule->left;
	if(!yycanshift(g, path->sp->state, rule->left, &nextstate))
		die("lost parse");

	if((sp = findtop(p, nextstate)) == nil){
		sp = newtop(p, nextstate);
		yycanshift(g, sp->state, lookahead, &sp->doshift);
	}
	if((e = findedge(sp, path->sp)) == nil){
		sp->edge = mkedge(g, rule->left, p->tmpval, path->sp, sp->edge);
		/* McPeak skips qreduce if we called newtop ? */
		qreduce1(g, p, sp, sp->edge, lookahead);
	}else if(p->merge)
		p->merge(e->val, p->tmpval);
}

static void
doshift(Ygram *g, Yparse *p, Ysym *sym, void *val)
{
	int i, ntop;
	Ystate *nstate;
	Ystack *sp, *nsp;

if(g->debug) print("shift %s\n", sym->name);	
	ntop = p->ntop;
	p->ntop = 0;
	for(i=0; i<ntop; i++){
		sp = p->top[i];
		if(!sp->doshift)
			continue;
		yycanshift(g, sp->state, sym, &nstate);
		if(nstate == nil)
			continue;
		if((nsp = findtop(p, nstate)) == nil)
			nsp = newtop(p, nstate);
		nsp->edge = mkedge(g, sym, val, sp, nsp->edge);
	}
}

void
printstate(Yparse *p)
{
	int i;
	
	for(i=0; i<p->ntop; i++)
		print("%d.%p.%p. %lY\n", i, p->top[i]->state, p->top[i]->doshift, p->top[i]->state);
}

void*
yyparse(Ygram *g, char *start, Yparse *p0, Ylex *yylex, void *arg)
{
	Yparse *p;
	Ysym *eofsym;

	fmtinstall('R', yyrulefmt);
	fmtinstall('$', yystatefmt);

	p = yyparseinit(g, p0, start);

	eofsym = yylooksym(g, "EOF");
if(g->debug>1) printstate(p);
	while((p->tok = yylex(g, p->tokval, arg)) != nil){
//print("%s ", p->tok->name);
if(g->debug>1) print("reduce\n");
		doreduce(g, p, p->tok);
if(g->debug>1) printstate(p);
if(g->debug>1) print("shift %s\n", p->tok->name);
		doshift(g, p, p->tok, p->tokval);
if(g->debug>1) printstate(p);
		if(p->ntop == 0){
			fprint(2, "parse error at %s\n", p->tok->name);
			return nil;
		}
		if(p->tok == eofsym)
			break;
	}
	return p->top[0]->edge->down->edge->val;
}
