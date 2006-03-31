#include "gc.h"
#include "bound.h"

static	BB*	bbfree;
static	BBset*	bbsfree;
static	int	bballoc;
static	int	bbsalloc;
static	BB	bbz;
static	BBset	bbsz;
static	BB*	firstbb;
static	BB*	lastbb;
static	BB*	wounded;
static	BB*	bbaux;
static	BBset*	recalc;
static	BBset*	bbhash[BBHASH];
static	BB**	ordered;
static	int	bcount;
static	BBset**	heap;
static	int	heapn;
static	int	bsize;
static	char	bbbuff[BBBSIZE];
static	int	bchange;

#define	bdebug	(debug['v'])
#define	dbg	0
#define	bcheck	0

static long
Rn(Reg *r)
{
	if(r == R)
		return -1;
	return r->rpo;
}

static BB*
bba(void)
{
	BB *b;

	bballoc++;
	b = bbfree;
	if(b == nil) {
		b = alloc(sizeof(*b));
	} else
		bbfree = b->link;

	*b = bbz;
	return b;
}

static void
bfree(BB *b)
{
	bballoc--;
	b->link = bbfree;
	bbfree = b;
}

static BBset*
bbsa(void)
{
	BBset *b;

	bballoc++;
	b = bbsfree;
	if(b == nil) {
		b = alloc(sizeof(*b));
	} else
		bbsfree = b->link;

	*b = bbsz;
	return b;
}

static void
bsfree(BBset *b)
{
	bballoc--;
	b->link = bbsfree;
	bbsfree = b;
}

static void
dumpheap(void)
{
	int i;

	for(i = 1; i <= heapn; i++)
		print(" %d", heap[i]->damage);
}

static void
checkheap(void)
{
	int N, N2, n, c;

	N = heapn;
	N2 = N >> 1;
	for(n = 1; n <= N2; n++) {
		c = n << 1;
		if((heap[c]->damage > heap[n]->damage)
		|| ((c < N) && (heap[c + 1]->damage > heap[n]->damage))) {
			print("bad heap (%d:%d) %d [", n, heap[n]->damage, heapn);
			dumpheap();
			print(" ]\n");
			abort();
		}
	}
}

static void
downheap(int n)
{
	int N, N2, d, c;
	BBset *s, *t, *u;

	s = heap[n];
	d = s->damage;
//print("down %d %d", n, d);
	N = heapn;
	N2 = N >> 1;
	while(n <= N2) {
		c = n << 1;
		t = heap[c];
		if(c < N) {
			u = heap[c + 1];
			if(t->damage < u->damage) {
				t = u;
				c++;
			}
		}
//print(" [%d %d]", c, t->damage);
		if(t->damage < d)
			break;
		heap[n] = t;
		t->index = n;
		n = c;
	}
	heap[n] = s;
	s->index = n;
//print("\n");
//checkheap();
}

static void
upheap(int n)
{
	int f, d;
	BBset *s, *t;

	s = heap[n];
	d = s->damage;
//print("up %d %d", n, d);
	while(n > 1) {
		f = n >> 1;
		t = heap[f];
//print(" [%d %d]", f, t->damage);
		if(t->damage >= d)
			break;
		heap[n] = t;
		t->index = n;
		n = f;
	}
	heap[n] = s;
	s->index = n;
//print("\n");
//checkheap();
}

static void
heapremove(BBset *s)
{
	int x;
	BBset *t;

	x = s->index;
	s->index = 0;
	if(x == 0)
		return;
	if(x == heapn) {
		heapn--;
		return;
	}
	t = heap[heapn--];
	heap[x] = t;
	t->index = x;
	if(s->damage < t->damage)
		upheap(x);
	else
		downheap(x);
}

static void
heapadd(BBset *s)
{
	int n;

	n = heapn + 1;
	heap[n] = s;
	s->index = n;
	heapn = n;
	upheap(n);

}

static void
bbsrecalc(BBset *s)
{
	if(s->recalc)
		return;
	s->recalc = 1;
	s->link = recalc;
	recalc = s;
	heapremove(s);
}

static void
bbadd(BB *b, Hval h)
{
	int k;
	BBset *s;

	k = h[0] & BBMASK;
	for(s = bbhash[k]; s != nil; s = s->next) {
		if(BBEQ(s->hash, h)) {
			b->set = s;
			b->link = s->ents;
			s->ents = b;
			bbsrecalc(s);
			return;
		}
	}
	s = bbsa();
	s->next = bbhash[k];
	bbhash[k] = s;
	b->set = s;
	b->link = nil;
	s->ents = b;
	BBCP(s->hash, h);
	bbsrecalc(s);
}

static int
hashbb(BB *b, Hval h)
{
	Reg *r;
	Prog *p;
	char *s;
	int c, f, i, n;

	r = b->first;
	s = bbbuff;
	i = 0;
	n = BBBSIZE;
	for(;;) {
		p = r->prog;
		if(p->as != ANOP) {
			if(p->to.type == D_BRANCH)
				p->to.offset = r->s2->rpo;
			c = snprint(s, n, "%P", p);
			s += c;
			n -= c;
			i++;
		}
		if(r == b->last)
			break;
		r = r->link;
	}
	if(n == 0)
		return Bbig;
	b->len = i;
	BBMKHASH(bbbuff, BBBSIZE - n, h);
	f = b->flags;
	if(i == 1 && r->prog->as == AJMP && b->first->p1 == R)
		f = Bjo;
	else if(b->first->p1 != R)
		f |= Bpre;
	if(bdebug)
		print("A %x %s %ux %ux\n", f, bbbuff, h[0], h[1]);
	return f;
}

static void
enterbb(BB *b)
{
	Hval h;

	b->flags = hashbb(b, h);
	if(b->flags != Bbig)
		bbadd(b, h);
}

static void
preproc(BB *b, int x)
{
	BB *n;
	Reg *r;

	ordered[x] = b;
	if(b->last->rpo - b->first->rpo > BBBIG) {
		b->flags = Bbig;
		return;
	}
	if(b->first->p2 == nil) {
		b->flags = Bdel;
		return;
	}
	switch(b->last->prog->as) {
	case ARET:
	case AJMP:
	case AIRETL:
		break;

	default:
		b->flags = Bdel;
		n = bba();
		n->first = b->first;
		for(r = b->last->link; r != R; r = r->link) {
			switch(r->prog->as) {
			case ARET:
			case AJMP:
			case AIRETL:
				n->last = r;
				n->flags = Bpin;
				enterbb(n);
				if(n->flags & Bpin) {
					n->aux = bbaux;
					bbaux = n;
				}
				else
					bfree(n);
				return;
			}
		}
		bfree(n);
		return;
	}
	enterbb(b);
}

static int
p2len(Reg *r)
{
	int c;

	c = 0;
	for(r = r->p2; r != nil; r = r->p2link)
		c++;
	return c;
}

static void
calcdamage(BBset *s)
{
	BB *f;
	int d, t;

	s->recalc = 0;
	f = s->ents;
	if(f == nil)
		return;
	if(f->flags & Bjo) {
		if(bdebug)
			print("add %ld jo\n", f->first->rpo);
		s->damage = COSTJO;
		heapadd(s);
		return;
	}
	if(f->link == nil) {
		if(bdebug)
			print("solo %x %x\n", s->hash[0], s->hash[1]);
		return;
	}

	d = 0;
	t = 0;
	while(f != nil) {
		if((f->flags & (Bpre|Bpin)) == 0 && f->last->link != R) {
			t = 1;
			d += (f->last->rpo - f->first->rpo) >> 1;
		}
		d += p2len(f->first);
		f = f->link;
	}

	if(t == 0) {
		if(bdebug)
			print("all pre %ld\n", s->ents->first->rpo);
		return;
	}

	if(bdebug)
		print("add %ld %d\n", s->ents->first->rpo, d);
	if(d > COSTHI)
		d = COSTHI;
	s->damage = d;
	heapadd(s);
}

static Reg*
findjump(BB *b)
{
	Reg *r, *l;

	r = b->first;
	l = b->last;

	for(;;) {
		if(r->prog->as == AJMP)
			break;
		if(r == l) {
			diag(Z, "findjump botch");
			break;
		}
		r = r->link;
	}
	return r;
}

static BB*
findset(int r)
{
	BB *s, **p;
	int n, n2;

	if(r < ordered[0]->first->rpo)
		return nil;
	n = bcount;
	p = ordered;
	while(n > 0) {
		n2 = n >> 1;
		s = p[n2];
		if(r < s->first->rpo) {
			n = n2;
			continue;
		}
		if(r > s->last->rpo) {
			n2++;
			p += n2;
			n -= n2;
			continue;
		}
		return s;
	}
	diag(Z, "findset botch");
	return nil;
}

static void
wound(Reg *r)
{
	BB *b, *p, **n;
	BBset *s;

	b = findset(r->rpo);
	if(b == nil)
		return;
	s = b->set;
	if(s == nil)
		return;
	for(n = &s->ents; (p = *n) != nil; n = &(*n)->link) {
		if(p == b) {
			*n = b->link;
			b->link = wounded;
			wounded = b;
			bbsrecalc(s);
			return;
		}
	}
}

static void
printbl(Reg *l)
{
	if(l == nil) {
		print("Z");
		return;
	}

	print("%ld", l->rpo);
	while((l = l->p2link) != nil)
		print(" %ld", l->rpo);
}

static void
appset(Reg *e, Reg *s)
{
	for(;;) {
		if(s->p2link == R) {
			s->p2link = e;
			return;
		}
		s = s->p2link;
	}
}

static Reg*
delset(Reg *e, Reg *s)
{
	Reg *c, *l;

	c = s;
	l = nil;
	for(;;) {
		if(e == c) {
			if(l == nil)
				return s->p2link;
			l->p2link = c->p2link;
			return s;
		}
		l = c;
		c = c->p2link;
		if(c == nil)
			return s;
	}
}

static void
redest(Reg *s, Reg *d)
{
	while(s != R) {
		s->s2 = d;
		s = s->p2link;
	}
}

static void
changedest(Reg *s, Reg *d, int x)
{
	Reg *l;

	if(bdebug) {
		print("change %ld [", s->rpo);
		printbl(s->p2);
		print("] -> %ld [", d->rpo);
		printbl(d->p2);
		print("]\n");
	}

	if(s->p2 == nil) {
//		print("deadjmp\n");
		return;
	}

	l = s->p2;
	for(;;) {
		if(bdebug)
			print("s2 %ld = %ld\n", l->rpo, d->rpo);
		l->s2 = d;
		wound(l);
		if(l->p2link == nil)
			break;
		l = l->p2link;
	}

	if(x) {
		l->p2link = delset(s, d->p2);
		d->p2 = s->p2;
	}
	else {
		l->p2link = d->p2;
		d->p2 = s->p2;
		s->p2 = nil;
	}

	if(bdebug) {
		print("result [");
		printbl(d->p2);
		print("]\n");
	}

	bchange = 1;
}

static void
bexcise(BB *b)
{
	Reg *r, *l;

	l = b->last;
	r = b->first;
	if(bdebug)
		print("excise %ld to %ld\n", r->rpo, l->rpo);
	for(;;) {
		r->prog->as = ANOP;
		r->prog->to.type = D_NONE;
		r->p2 = R;
		if(r->s2 != R) {
			r->s2->p2 = delset(r, r->s2->p2);
			r->s2 = R;
		}
		if(r == l)
			break;
		r = r->link;
	}
}

static int
backtrack(Reg *s, Reg *d)
{
	int i;
	char l[BINST], r[BINST];

//print("backtrack %ld %ld\n", Rn(s), Rn(d));
	i = 0;
	while(s != nil && d != nil) {
		if(snprint(l, BINST, "%P", s->prog) == BINST)
			break;
		if(snprint(r, BINST, "%P", d->prog) == BINST)
			break;
//print("%s\t%s\n", l, r);
		if(strcmp(l, r) != 0)
			break;
		i++;
		s = s->p2link;
		d = d->p2link;
	}
	return i;
}

static void
checktails(void)
{
	int c;
	Reg *r;

	c = 0;
	for(r = firstr; r->link != R; r = r->link) {
		if(r->prog->as == AJMP && r->s2 != nil)
			c += backtrack(r->p1, r->s2->p1);
	}

	if(c > 0)
		print("tails %s %d\n", firstr->prog->from.sym->name, c);
}

static void
process(BBset *s)
{
	Reg *h;
	BB *f, *o, *p, *t;

	if(bdebug)
		print("process %d %x %x\n", s->damage, s->hash[0], s->hash[1]);
	f = s->ents;
	if(f->flags & Bjo) {
		s->ents = nil;
		h = findjump(f)->s2;
		o = nil;
		while(f != nil) {
			t = f->link;
			if((f->flags & Bjo) != 0 && f->first->s2 != f->first) {
				changedest(f->first, h, 1);
				bexcise(f);
			}
			else {
				f->link = o;
				o = f;
			}
			f = t;
		}
		s->ents = o;
	}
	else {
		o = nil;
		p = nil;
		while(f != nil) {
			t = f->link;
			if((f->flags & (Bpre|Bpin)) != 0 || (f->last->link == R)) {
				f->link = p;
				p = f;
			}
			else {
				f->link = o;
				o = f;
			}
			f = t;
		}
		if(o == nil) {
			diag(Z, "all Bpre");
			return;
		}
		if(p == nil) {
			p = o;
			o = p->link;
			p->link = nil;
			s->ents = p;
		}
		else
			s->ents = p;

		h = p->first;
		// oblit o list repl with jmp to h
		while(o != nil) {
			changedest(o->first, h, 1);
			bexcise(o);
			o = o->link;
		}

		bbsrecalc(s);
	}
}

static void
iterate(void)
{
	BBset *s;
	BB *b, *t;

	heapn = 0;

	for(;;) {
		for(b = wounded; b != nil; b = t) {
			t = b->link;
			enterbb(b);
		}
		wounded = nil;

		for(s = recalc; s != nil; s = s->link)
			calcdamage(s);
		recalc = nil;

		if(heapn == 0)
			return;

		s = heap[1];
		heapremove(s);
		process(s);
	}
}

static void
cleanup(void)
{
	int i;
	BB *l, *n;
	BBset *b, *t;

	for(i = 0; i < BBHASH; i++) {
		b = bbhash[i];
		bbhash[i] = nil;
		while(b != nil) {
			t = b->next;
			bsfree(b);
			b = t;
		}
	}
	for(i = 0; i < bcount; i++)
		bfree(ordered[i]);
	for(l = bbaux; l != nil; l = n) {
		n = l->aux;
		bfree(l);
	}
	bbaux = nil;
}

static void
prreg(Reg *r)
{
	Prog *p;

	p = r->prog;
	if(p->to.type == D_BRANCH)
		p->to.offset = r->s2->rpo;
	print("%ld:%P\tr %lX ", r->rpo, r->prog, r->regu);
	print("p1 %ld p2 %ld p2l %ld s1 %ld s2 %ld link %ld",
		Rn(r->p1), Rn(r->p2), Rn(r->p2link),
		Rn(r->s1), Rn(r->s2), Rn(r->link));
	if(!r->active)
		print(" d");
//	print(" %p %p\n", r->prog, r->prog->link);
	print("\n");
}

static	void	prfunc(char*);

static void
checkr(int d)
{
	Prog *p;
	Reg *r, *t;

	for(r = firstr; r->link != R; r = r->link) {
		for(p = r->prog->link; p != P && p != r->link->prog; p = p->link)
			;
		if(p == P) {
			print("%ld: bad prog link\n", r->rpo);
			if(d)
				prfunc(nil);
			abort();
		}
		if(r->s1 != R && (r->s1 != r->link || r->link->p1 != r)) {
			print("%ld: bad s1 p1\n", r->rpo);
			if(d)
				prfunc(nil);
			abort();
		}
		if(r->s2 != R && r->s2->p2 == nil) {
			print("%ld: no p2 for s2\n", r->rpo);
			if(d)
				prfunc(nil);
			abort();
		}
		if(r->p2 != R) {
			t = r->p2->s2;
			while(t != r) {
				t = t->p2link;
				if(t == R) {
					print("%ld: bad s2 for p2\n", r->rpo);
					if(d)
						prfunc(nil);
					abort();
				}
			}
		}
	}
}

static void
prfunc(char *s)
{
	Reg *r;

	if(s != nil)
		print("%s structure %s\n", s, firstr->prog->from.sym->name);
	for(r = firstr; r != R; r = r->link)
		prreg(r);
	if(s != nil) {
		print("end\n");
		checkr(0);
	}
}

/* find p in r's list and replace with l */
static void
adjprog(Reg *r, Prog *p, Prog *l)
{
	Prog *t, **n;

	for(n = &r->prog->link; (t = *n) != nil; n = &t->link) {
		if(t == p) {
			*n = l;
			return;
		}
	}
	print("adjprog botch\n");
	abort();
}

static void
jumptojump(void)
{
	Reg *r;

	for(r = firstr; r != R; r = r->link) {
		if(r->prog->as == AJMP && r->p2 != R && r->s2 != r) {
			if(bdebug)
				print("jump as dest %ld -> %ld\n", r->rpo, r->s2->rpo);
			changedest(r, r->s2, 0);
			bchange++;
		}
	}
}

/* drag a tail to replace a jump.  seems to be a bad idea. */
static void
rearrange(void)
{
	int i;
	Reg *j, *t;
	BB *b, *p, *s;

	for(i = 0; i < bcount; i++) {
		b = ordered[i];
		if(b->flags & Bdel)
			continue;
		j = b->last;
		if(j->prog->as == AJMP && j->s2->p1 == R) {
			t = j->s2;
			if(t == b->first)
				continue;
			s = findset(t->rpo);
			if(s == nil) {
				diag(Z, "no self");
				continue;
			}
			if(s == ordered[0])
				continue;
			if(s->flags & Bdel)
				continue;
			if(s->last->link == R)
				continue;
			if(bdebug)
				print("drag %ld to %ld\n", t->rpo, j->rpo);
			p = findset(t->rpo - 1);
			if(p == nil) {
				diag(Z, "no predec");
				continue;
			}
			if(p->last->link != t) {
				diag(Z, "bad predec %ld %ld", p->last->rpo, t->rpo);
				continue;
			}

			/* poison everything in sight */
			b->flags |= Bdel;
			s->flags |= Bdel;
			findset(j->link->rpo)->flags |= Bdel;
			findset(s->last->link->rpo)->flags |= Bdel;

			/* remove */
			adjprog(p->last, t->prog, s->last->link->prog);
			p->last->link = s->last->link;

			/* fix tail */
			adjprog(s->last, s->last->link->prog, j->link->prog);
			s->last->link = j->link;

			/* fix head */
			adjprog(j, j->link->prog, t->prog);
			j->link = t;

			/* nop the jump */
			j->prog->as = ANOP;
			j->prog->to.type = D_NONE;
			j->s2 = nil;
			j->link->p2 = delset(j, j->link->p2);
			j->s1 = t;
			t->p1 = j;
			if(bcheck)
				checkr(1);
			bchange++;
		}
	}
}

void
jumptodot(void)
{
	Reg *r;

	for(r = firstr; r != R; r = r->link) {
		if(r->prog->as == AJMP && r->s2 == r->link) {
			if(debug['v'])
				print("jump to next %ld\n", r->rpo);
			r->prog->as = ANOP;
			r->prog->to.type = D_NONE;
			r->s2 = nil;
			r->link->p2 = delset(r, r->link->p2);
			findset(r->rpo)->flags |= Bdel;
			findset(r->link->rpo)->flags |= Bdel;
			bchange++;
		}
	}
}

void
comtarg(void)
{
	int n;
	BB *b, *c;
	Reg *r, *l, *p, *t;

loop:
	bchange = 0;

	/* excise NOPS because they just get in the way */
	/* some have p2 because they are excised labelled moves */

	if(debug['v']) {
		n = 0;
		for(r = firstr; r != R; r = r->link)
			r->rpo = n++;
		prfunc("prenop");
	}

	r = firstr;
	l = r->link;
	while(l != R) {
		if(l->prog->as == ANOP) {
			t = l->p1;
			p = l->p2;
if(dbg) print("nop %ld [", l->rpo);
if(dbg) printbl(p);
			for(;;) {
				adjprog(r, l->prog, l->prog->link);
				r->link = l->link;
				l->link = freer;
				freer = l;
				l = r->link;
				if(l->prog->as != ANOP)
					break;
if(dbg) print("] %ld [", l->rpo);
if(dbg) printbl(l->p2);
				if(p == R)
					p = l->p2;
				else if(l->p2 != nil)
					appset(l->p2, p);
			}
if(dbg) print("] %ld [", l->rpo);
if(dbg) printbl(l->p2);
			if(p != R) {
				redest(p, l);
				if(l->p2 != R)
					appset(p, l->p2);
				else
					l->p2 = p;
			}
if(dbg) print("] -> [");
if(dbg) printbl(l->p2);
if(dbg) print("]\n");
			if(r->s1 != R)
				r->s1 = l;
			l->p1 = t;
		}
		r = l;
		l = r->link;
	}

	n = 0;
	for(r = firstr; r != R; r = r->link)
		r->rpo = n++;

	if(debug['v'])
		prfunc("input");

	firstbb = nil;
	lastbb = nil;

	if(debug['v'])
		print("bbstart\n");

	n = 0;
	r = firstr;
	do {
		b = bba();
		b->first = r;
		for(;;) {
			l = r;
			r = r->link;
			switch(l->prog->as) {
			case ARET:
			case AJMP:
			case AIRETL:
				goto out;
			}
			if(r->p2 != R)
				break;
		}
	out:
		b->last = l;
		if(lastbb == nil)
			firstbb = b;
		else
			lastbb->link = b;
		lastbb = b;
		if(bdebug)
			print("BB %ld %ld\n", b->first->rpo, b->last->rpo);
		n++;
	} while(r != R);

	if(debug['v'])
		print("end\n");

	if(n > bsize) {
		bsize = n * 3 / 2;
		if(bsize < BBINIT)
			bsize = BBINIT;
		ordered = alloc(bsize * sizeof(*ordered));
		heap = alloc((bsize + 1) * sizeof(*ordered));
	}

	if(debug['v'])
		print("preprocstart\n");

	n = 0;
	for(b = firstbb; b != nil; b = c) {
		c = b->link;
		preproc(b, n++);
	}

	if(debug['v'])
		print("end\n");

	bcount = n;

	jumptojump();

	if(debug['v'])
		print("iteratestart\n");

	iterate();
//checktails();

	if(debug['v'])
		print("end\n");

	if(debug['v'])
		print("extrastart\n");

	jumptodot();
//	rearrange();

	if(debug['v'])
		print("end\n");

	cleanup();
	if(bballoc || bbsalloc)
		diag(Z, "bballoc %d %d", bballoc, bbsalloc);

	if(debug['v'])
		prfunc("output");

	if(1 && bchange)
		goto loop;
}
