#include	"l.h"

void
dodata(void)
{
	int i, t;
	Sym *s;
	Prog *p;
	long orig, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(p->as == ADYNT || p->as == AINIT)
			s->value = dtype;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type != SDATA)
			diag("initialize non-data (%d): %s\n%P",
				s->type, s->name, p);
		v = p->from.offset + p->reg;
		if(v > s->value)
			diag("initialize bounds (%lld): %s\n%P",
				(vlong)s->value, s->name, p);
	}

	if(debug['t']) {
		/*
		 * pull out string constants
		 */
		for(p = datap; p != P; p = p->link) {
			s = p->from.sym;
			if(p->to.type == D_SCONST)
				s->type = SSTRING;
		}
	}

	/*
	 * pass 1
	 *	assign 'small' variables to data segment
	 *	(rationale is that data segment is more easily
	 *	 addressed through offset on REGSB)
	 */
	orig = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA && t != SBSS)
			continue;
		v = s->value;
		if(v == 0) {
			diag("%s: no size", s->name);
			v = 1;
		}
		v = rnd(v, 4);
		s->value = v;
		if(v > MINSIZ)
			continue;
		if(v >= 16)
			orig = rnd(orig, 16);
		else if(v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}

	/*
	 * pass 2
	 *	assign large 'data' variables to data segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA) {
			if(t == SDATA1)
				s->type = SDATA;
			continue;
		}
		v = s->value;
		if(v >= 16)
			orig = rnd(orig, 16);
		else if(v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
	}

	while(orig & 7)
		orig++;
	datsize = orig;

	/*
	 * pass 3
	 *	everything else to bss segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SBSS)
			continue;
		v = s->value;
		if(v >= 16)
			orig = rnd(orig, 16);
		else if(v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
	}
	while(orig & 7)
		orig++;
	bsssize = orig-datsize;

	xdefine("setSB", SDATA, 0L);
	xdefine("bdata", SDATA, 0L);
	xdefine("edata", SDATA, datsize);
	xdefine("end", SBSS, datsize+bsssize);
	xdefine("etext", STEXT, 0L);
}

Prog*
brchain(Prog *p)
{
	int i;

	for(i=0; i<20; i++) {
		if(p == P || !isbranch(p->as))
			return p;
		p = p->cond;
	}
	return P;
}

void
follow(void)
{
	if(debug['v'])
		Bprint(&bso, "%5.2f follow\n", cputime());
	Bflush(&bso);

	firstp = prg();
	lastp = firstp;
	xfol(textp);

	firstp = firstp->link;
	lastp->link = P;
}

void
xfol(Prog *p)
{
	Prog *q, *r;
	int a, i;

loop:
	if(p == P)
		return;
	a = p->as;
	if(a == ATEXT)
		curtext = p;
	if(isbranch(a)) {
		q = p->cond;
		if(q != P) {
			p->mark |= FOLL;
			p = q;
			if(!(p->mark & FOLL))
				goto loop;
		}
	}
	if(p->mark & FOLL) {
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == lastp)
				break;
			a = q->as;
			if(a == ANOP) {
				i--;
				continue;
			}
			if(isbranch(a) || isreturn(a))
				goto copy;
			if(q->cond == nil || (q->cond->mark&FOLL))
				continue;
			if(a != ABEQ && a != ABNE)
				continue;
		copy:
			for(;;) {
				r = prg();
				*r = *p;
				if(!(r->mark&FOLL))
					print("cant happen 1\n");
				r->mark |= FOLL;
				if(p != q) {
					p = p->link;
					lastp->link = r;
					lastp = r;
					continue;
				}
				lastp->link = r;
				lastp = r;
				if(isbranch(a) || isreturn(a))
					return;
				r->as = a == ABNE? ABEQ: ABNE;
				r->cond = p->link;
				r->link = p->cond;
				if(!(r->link->mark&FOLL))
					xfol(r->link);
				if(!(r->cond->mark&FOLL))
					print("cant happen 2\n");
				return;
			}
		}
		a = branchop();
		q = prg();
		q->as = a;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->cond = p;
		p = q;
	}
	p->mark |= FOLL;
	lastp->link = p;
	lastp = p;
	if(isbranch(a) || isreturn(a))
		return;
	if(p->cond != P)
	if(!iscall(a) && p->link != P) {
		q = brchain(p->link);
		if(canfollow(a))
		if(q != P && (q->mark&FOLL)) {
			p->as = relinv(a);
			p->link = p->cond;
			p->cond = q;
		}
		xfol(p->link);
		q = brchain(p->cond);
		if(q == P)
			q = p->cond;
		if(q->mark&FOLL) {
			p->cond = q;
			return;
		}
		p = q;
		goto loop;
	}
	p = p->link;
	goto loop;
}

void
patch(void)
{
	long c, vexit;
	Prog *p, *q;
	Sym *s;
	int a;

	if(debug['v'])
		Bprint(&bso, "%5.2f patch\n", cputime());
	Bflush(&bso);
	mkfwd();
	s = lookup("exit", 0);
	vexit = s->value;
	for(p = firstp; p != P; p = p->link) {
		a = p->as;
		if(a == ATEXT)
			curtext = p;
		if((iscall(a) || isbranch(a) || isreturn(a)) &&
		   p->to.type != D_BRANCH && p->to.sym != S) {
			s = p->to.sym;
			switch(s->type) {
			default:
				diag("undefined: %s\n%P", s->name, p);
				s->type = STEXT;
				s->value = vexit;
				break;
			case STEXT:
				p->to.offset = s->value;
				break;
			case SUNDEF:
				if(!iscall(p->as))
					diag("help: SUNDEF in AB || ARET");
				p->to.offset = 0;
				p->cond = UP;
				break;
			}
			p->to.type = D_BRANCH;
		}
		if(p->cond == UP)
			continue;
		if(p->to.type == D_BRANCH)
			c = p->to.offset;
		else if(p->from.type == D_BRANCH)
			c = p->from.offset;
		else
			continue;
		for(q = firstp; q != P;) {
			if(q->forwd != P)
			if(c >= q->forwd->pc) {
				q = q->forwd;
				continue;
			}
			if(c == q->pc)
				break;
			q = q->link;
		}
		if(q == P) {
			diag("branch out of range %ld\n%P", c, p);
			p->to.type = D_NONE;
		}
		p->cond = q;
	}

	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		if(p->cond != P && p->cond != UP) {
			p->cond = brloop(p->cond);
			if(p->cond != P){
				if(p->to.type == D_BRANCH)
					p->to.offset = p->cond->pc;
				if(p->from.type == D_BRANCH)
					p->from.offset = p->cond->pc;
			}
		}
	}
}

#define	LOG	6
void
mkfwd(void)
{
	Prog *p;
	long dwn[LOG], cnt[LOG], i;
	Prog *lst[LOG];

	for(i=0; i<LOG; i++) {
		if(i == 0)
			cnt[i] = 1; else
			cnt[i] = LOG * cnt[i-1];
		dwn[i] = 1;
		lst[i] = P;
	}
	i = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		i--;
		if(i < 0)
			i = LOG-1;
		p->forwd = P;
		dwn[i]--;
		if(dwn[i] <= 0) {
			dwn[i] = cnt[i];
			if(lst[i] != P)
				lst[i]->forwd = p;
			lst[i] = p;
		}
	}
}

Prog*
brloop(Prog *p)
{
	Prog *q;
	int c;

	for(c=0; p!=P;) {
		if(!isbranch(p->as))
			return p;
		q = p->cond;
		if(q <= p) {
			c++;
			if(q == p || c > 5000)
				break;
		}
		p = q;
	}
	return P;
}
