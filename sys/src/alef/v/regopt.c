#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

#define	COL1	32

Node dummy;

int
sval(long v)
{
	if(v >= -32766L && v < 32766L)
		return 1;
	return 0;
}

Reg*
rega(void)
{
	Reg *r;

	r = freer;
	if(r == R) {
		r = malloc(sizeof(Reg));
	} else
		freer = r->next;

	*r = zreg;
	return r;
}

int
rcmp(void *a1, void *a2)
{
	Rgn *p1, *p2;
	int c1, c2;

	p1 = a1;
	p2 = a2;
	c1 = p2->cost;
	c2 = p1->cost;
	if(c1 -= c2)
		return c1;
	return p2->varno - p1->varno;
}

void
regopt(Inst *p)
{
	Reg *r, *r1, *r2;
	int i, z;
	long initpc, val;
	ulong vreg;
	Bits bit;
	struct
	{
		long	m;
		long	c;
		Reg*	p;
	} log5[6], *lp;

	initpc = 0;
	firstr = R;
	lastr = R;
	nvar = 0;
	regbits = 0;
	for(z=0; z<BITS; z++) {
		externs.b[z] = 0;
		param.b[z] = 0;
		consts.b[z] = 0;
		addrs.b[z] = 0;
	}

	/*
	 * pops 1
	 * build aux data structure
	 * allocate pcs
	 * find use and set of variables
	 */
	val = 5L * 5L * 5L * 5L * 5L;
	lp = log5;
	for(i=0; i<5; i++) {
		lp->m = val;
		lp->c = 0;
		lp->p = R;
		val /= 5L;
		lp++;
	}

	for(; p != P; p = p->next) {
		switch(p->op) {
		case ADATA:
		case AGLOBL:
		case ANAME:
			continue;
		}
		r = rega();
		if(firstr == R) {
			initpc = p->pc;
			firstr = r;
			lastr = r;
		} else {
			lastr->next = r;
			r->p1 = lastr;
			lastr->s1 = r;
			lastr = r;
		}
		r->prog = p;
		r->pc = p->pc;

		lp = log5;
		for(i=0; i<5; i++) {
			lp->c--;
			if(lp->c <= 0) {
				lp->c = lp->m;
				if(lp->p != R)
					lp->p->log5 = r;
				lp->p = r;
				(lp+1)->c = 0;
				break;
			}
			lp++;
		}

		r1 = r->p1;
		if(r1 != R)
		switch(r1->prog->op) {
		case ARET:
		case AJMP:
		case ARFE:
			r->p1 = R;
			r1->s1 = R;
		}

		/*
		 * left side always read
		 */
		bit = mkvar(&p->src1, p->op==AMOVW);
		for(z=0; z<BITS; z++)
			r->use1.b[z] |= bit.b[z];

		/*
		 * right side depends on opcode
		 */
		bit = mkvar(&p->dst, 0);
		if(bany(&bit))
		switch(p->op) {
		default:
			diag(ZeroN, "reg: unknown opop: %A", p->op);
			break;

		/*
		 * right side write
		 */
		case ANOP:
		case AMOVB:
		case AMOVBU:
		case AMOVH:
		case AMOVHU:
		case AMOVW:
		case AMOVF:
		case AMOVD:
			for(z=0; z<BITS; z++)
				r->set.b[z] |= bit.b[z];
			break;

		/*
		 * funny
		 */
		case AJAL:
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			break;
		}
	}
	if(firstr == R)
		return;

	/*
	 * pops 2
	 * turn branch references to pointers
	 * build back pointers
	 */
	for(r = firstr; r != R; r = r->next) {
		p = r->prog;
		if(p->dst.type == A_BRANCH) {
			val = p->dst.ival;
			r1 = firstr;
			while(r1 != R) {
				r2 = r1->log5;
				if(r2 != R && val >= r2->pc) {
					r1 = r2;
					continue;
				}
				if(r1->pc == val)
					break;
				r1 = r1->next;
			}
			if(r1 == R) {
				dummy.srcline = p->lineno;
				diag(&dummy, "ref not found\n%i", p);
				continue;
			}
			if(r1 == r) {
				dummy.srcline = p->lineno;
				diag(&dummy, "ref to self\n%i", p);
				continue;
			}
			r->s2 = r1;
			r->p2next = r1->p2;
			r1->p2 = r;
		}
	}
	if(opt('R')) {
		p = firstr->prog;
		print("\n%L %a\n", p->lineno, &p->src1);
	}

	/*
	 * pops 2.5
	 * find looping structure
	 */
	for(r = firstr; r != R; r = r->next)
		r->active = 0;
	change = 0;
	loopit(firstr);
	if(opt('R') && opt('v')) {
		print("\nlooping structure:\n");
		for(r = firstr; r != R; r = r->next) {
			print("%d:%i", r->loop, r->prog);
			for(z=0; z<BITS; z++)
				bit.b[z] = r->use1.b[z] |
					r->use2.b[z] | r->set.b[z];
			if(bany(&bit)) {
				print("%|", COL1);
				if(bany(&r->use1))
					print(" u1=%B", r->use1);
				if(bany(&r->use2))
					print(" u2=%B", r->use2);
				if(bany(&r->set))
					print(" st=%B", r->set);
			}
			print("\n");
		}
	}

	/*
	 * pops 3
	 * iterate propagating usage
	 * 	back until flow graph is complete
	 */
loop1:
	change = 0;
	for(r = firstr; r != R; r = r->next)
		r->active = 0;
	for(r = firstr; r != R; r = r->next)
		if(r->prog->op == ARET)
			prop(r, zbits, zbits);
loop11:
	/* pick up unreachable code */
	i = 0;
	for(r = firstr; r != R; r = r1) {
		r1 = r->next;
		if(r1 && r1->active && !r->active) {
			prop(r, zbits, zbits);
			i = 1;
		}
	}
	if(i)
		goto loop11;
	if(change)
		goto loop1;


	/*
	 * pops 4
	 * iterate propagating register/variable synchrony
	 * 	forward until graph is complete
	 */
loop2:
	change = 0;
	for(r = firstr; r != R; r = r->next)
		r->active = 0;
	synch(firstr, zbits);
	if(change)
		goto loop2;


	/*
	 * pops 5
	 * isolate regions
	 * calculate costs (paint1)
	 */
	r = firstr;
	if(r) {
		for(z=0; z<BITS; z++)
			bit.b[z] = (r->refahead.b[z] | r->calahead.b[z]) &
			  ~(externs.b[z] | param.b[z] | addrs.b[z] | consts.b[z]);
		if(bany(&bit)) {
			dummy.srcline = r->prog->lineno;
			warn(&dummy, "used and not set: %B", bit);
			if(opt('R') && !opt('w'))
				print("used and not set: %B\n", bit);
		}
	}
	if(opt('R') && opt('v'))
		print("\nprop structure:\n");
	for(r = firstr; r != R; r = r->next)
		r->act = zbits;
	rgp = region;
	nregion = 0;
	for(r = firstr; r != R; r = r->next) {
		if(opt('R') && opt('v'))
			print("%i\n	set = %B; rah = %B; cal = %B\n",
				r->prog, r->set, r->refahead, r->calahead);
		for(z=0; z<BITS; z++)
			bit.b[z] = r->set.b[z] &
			  ~(r->refahead.b[z] | r->calahead.b[z] | addrs.b[z]);
		if(bany(&bit)) {
			dummy.srcline = r->prog->lineno;
			warn(&dummy, "set and not used: %B", bit);
			if(opt('R'))
				print("set an not used: %B\n", bit);
			excise(r);
		}
		for(z=0; z<BITS; z++)
			bit.b[z] = LOAD(r) & ~(r->act.b[z] | addrs.b[z]);
		while(bany(&bit)) {
			i = bnum(bit);
			rgp->enter = r;
			rgp->varno = i;
			change = 0;
			if(opt('R') && opt('v'))
				print("\n");
			paint1(r, i);
			bit.b[i/32] &= ~(1L<<(i%32));
			if(change <= 0) {
				if(opt('R'))
					print("%L $%d: %B\n",
						r->prog->lineno, change, blsh(i));
				continue;
			}
			rgp->cost = change;
			nregion++;
			if(nregion >= NRGN) {
				warn(ZeroN, "too many regions");
				goto brk;
			}
			rgp++;
		}
	}
brk:
	qsort(region, nregion, sizeof(region[0]), rcmp);

	/*
	 * pops 6
	 * determine used registers (paint2)
	 * replace code (paint3)
	 */
	rgp = region;
	for(i=0; i<nregion; i++) {
		bit = blsh(rgp->varno);
		vreg = paint2(rgp->enter, rgp->varno);
		vreg = allreg(vreg, rgp);
		if(opt('R')) {
			if(rgp->regno >= Nreg)
				print("%L $%d F%d: %B\n",
					rgp->enter->prog->lineno,
					rgp->cost,
					rgp->regno-Nreg,
					bit);
			else
				print("%L $%d R%d: %B\n",
					rgp->enter->prog->lineno,
					rgp->cost,
					rgp->regno,
					bit);
		}
		if(rgp->regno != 0)
			paint3(rgp->enter, rgp->varno, vreg, rgp->regno);
		rgp++;
	}
	/*
	 * pops 7
	 * peep-hole on bopic block
	 */
	if(!opt('R') || opt('P'))
		peep();

	/*
	 * pops 8
	 * recalculate pc
	 */
	val = initpc;
	for(r = firstr; r != R; r = r1) {
		r->pc = val;
		p = r->prog;
		r1 = r->next;
		if(r1 == R)
			break;
		while(p != r1->prog)
		switch(p->op) {

		default:
			p->pc = val++;

		case ADATA:
		case AGLOBL:
		case ANAME:
			p = p->next;
		}
	}
	pc = val + 1;

	/*
	 * lopt pops
	 * fix up branches
	 * free aux structures
	 */
	if(opt('R'))
		if(bany(&addrs))
			print("addrs: %B\n", addrs);

	r1 = 0; /* set */
	for(r = firstr; r != R; r = r->next) {
		p = r->prog;
		if(p->dst.type == A_BRANCH)
			p->dst.ival = r->s2->pc;
		r1 = r;
	}
	if(r1 != R) {
		r1->next = freer;
		freer = firstr;
	}
}

/*
 * add mov b,rn
 * just after r
 */
void
addmove(Reg *r, int bn, int rn, int f)
{
	Inst *p, *p1;
	Adres *a;
	Var *v;

	p1 = ai();
	*p1 = zprog;
	p = r->prog;

	p1->next = p->next;
	p->next = p1;
	p1->lineno = p->lineno;

	v = var + bn;

	a = &p1->dst;
	a->sym = v->sym;
	a->class = v->class;
	a->ival = v->ival;
	a->etype = v->etype;
	a->type = A_INDREG;
	if(a->etype == TARRAY || a->sym == ZeroS)
		a->type = A_CONST;

	p1->op = AMOVW;
	if(v->etype == TCHAR)
		p1->op = AMOVBU;
	if(v->etype == TSINT || v->etype == TSUINT)
		p1->op = AMOVH;
	if(v->etype == TFLOAT)
		p1->op = AMOVD;

	p1->src1.type = A_REG;
	p1->src1.reg = rn;
	if(rn >= Nreg) {
		p1->src1.type = A_FREG;
		p1->src1.reg = rn-Nreg;
	}
	if(!f) {
		p1->src1 = *a;
		*a = zprog.src1;
		a->type = A_REG;
		a->reg = rn;
		if(rn >= Nreg) {
			a->type = A_FREG;
			a->reg = rn-Nreg;
		}
		if(v->etype == TCHAR)
			p1->op = AMOVBU;
		if(v->etype == TSUINT)
			p1->op = AMOVHU;
	}
	if(opt('R'))
		print("%i%|.a%i\n", p, COL1, p1);
}

Bits
mkvar(Adres *a, int docon)
{
	Var *v;
	int i, t, n, et, z;
	long o;
	Bits bit;
	Sym *s;

	t = a->type;
	if(t == A_REG && a->reg != Nreg)
		regbits |= RtoB(a->reg);
	if(t == A_FREG && a->reg != Nreg)
		regbits |= FtoB(a->reg);
	s = a->sym;
	o = a->ival;
	et = a->etype;
	if(s == ZeroS) {
		if(t != A_CONST || !docon || a->reg != Nreg)
			goto none;
		et = TINT;
	}
	if(t == A_CONST) {
		if(s == ZeroS && sval(o))
			goto none;
	}

	n = a->class;
	v = var;
	for(i=0; i<nvar; i++) {
		if(s == v->sym)
		if(n == v->class)
		if(o == v->ival)
			goto out;
		v++;
	}
	if(s)
		if(s->name[0] == '.')
			goto none;
	if(nvar >= NVAR) {
		if(s)
		warn(ZeroN, "variable not optimized: %s", s->name);
		goto none;
	}
	i = nvar;
	nvar++;
	v = &var[i];
	v->sym = s;
	v->ival = o;
	v->etype = et;
	v->class = n;
	if(opt('R'))
		print("bit=%2d et=%2d %a\n", i, et, a);
out:
	bit = blsh(i);
	if(n == External || n == Internal)
		for(z=0; z<BITS; z++)
			externs.b[z] |= bit.b[z];
	if(n == Parameter)
		for(z=0; z<BITS; z++)
			param.b[z] |= bit.b[z];
	if(v->etype != et || !(MARITH&(1<<et)))	/* funny punning */
		for(z=0; z<BITS; z++)
			addrs.b[z] |= bit.b[z];
	if(t == A_CONST) {
		if(s == ZeroS) {
			for(z=0; z<BITS; z++)
				consts.b[z] |= bit.b[z];
			return bit;
		}
		if(et != TARRAY)
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
		for(z=0; z<BITS; z++)
			param.b[z] |= bit.b[z];
		return bit;
	}
	if(t == A_INDREG)
		return bit;

none:
	return zbits;
}

void
prop(Reg *r, Bits ref, Bits cal)
{
	Reg *r1, *r2;
	int z;

	for(r1 = r; r1 != R; r1 = r1->p1) {
		for(z=0; z<BITS; z++) {
			ref.b[z] |= r1->refahead.b[z];
			if(ref.b[z] != r1->refahead.b[z]) {
				r1->refahead.b[z] = ref.b[z];
				change++;
			}
			cal.b[z] |= r1->calahead.b[z];
			if(cal.b[z] != r1->calahead.b[z]) {
				r1->calahead.b[z] = cal.b[z];
				change++;
			}
		}
		switch(r1->prog->op) {
		case AJAL:
			for(z=0; z<BITS; z++) {
				cal.b[z] |= ref.b[z] | externs.b[z];
				ref.b[z] = 0;
			}
			break;

		case ATEXT:
			for(z=0; z<BITS; z++) {
				cal.b[z] = 0;
				ref.b[z] = 0;
			}
			break;

		case ARET:
			for(z=0; z<BITS; z++) {
				cal.b[z] = externs.b[z];
				ref.b[z] = 0;
			}
		}
		for(z=0; z<BITS; z++) {
			ref.b[z] = (ref.b[z] & ~r1->set.b[z]) |
				r1->use1.b[z] | r1->use2.b[z];
			cal.b[z] &= ~(r1->set.b[z] | r1->use1.b[z] | r1->use2.b[z]);
			r1->refbehind.b[z] = ref.b[z];
			r1->calbehind.b[z] = cal.b[z];
		}
		if(r1->active)
			break;
		r1->active = 1;
	}
	for(; r != r1; r = r->p1)
		for(r2 = r->p2; r2 != R; r2 = r2->p2next)
			prop(r2, r->refbehind, r->calbehind);
}

int
loopit(Reg *r)
{
	Reg *r1;
	int l, m;

	l = 0;
	r->active = 1;
	r->loop = 0;
	if(r1 = r->s1)
	switch(r1->active) {
	case 0:
		l += loopit(r1);
		break;
	case 1:
		l += LOOP;
		r1->loop += LOOP;
	}
	if(r1 = r->s2)
	switch(r1->active) {
	case 0:
		l += loopit(r1);
		break;
	case 1:
		l += LOOP;
		r1->loop += LOOP;
	}
	r->active = 2;
	m = r->loop;
	r->loop = l + 1;
	return l - m;
}

void
synch(Reg *r, Bits dif)
{
	Reg *r1;
	int z;

	for(r1 = r; r1 != R; r1 = r1->s1) {
		for(z=0; z<BITS; z++) {
			dif.b[z] = (dif.b[z] &
				~(~r1->refbehind.b[z] & r1->refahead.b[z])) |
					r1->set.b[z] | r1->regdiff.b[z];
			if(dif.b[z] != r1->regdiff.b[z]) {
				r1->regdiff.b[z] = dif.b[z];
				change++;
			}
		}
		if(r1->active)
			break;
		r1->active = 1;
		for(z=0; z<BITS; z++)
			dif.b[z] &= ~(~r1->calbehind.b[z] & r1->calahead.b[z]);
		if(r1->s2 != R)
			synch(r1->s2, dif);
	}
}

ulong
allreg(ulong b, Rgn *r)
{
	Var *v;
	int i;

	v = var + r->varno;
	r->regno = 0;
	switch(v->etype) {

	default:
		diag(ZeroN, "unknown etype %d/%d", bitno(b), v->etype);
		break;

	case TCHAR:
	case TSINT:
	case TSUINT:
	case TINT:
	case TUINT:
	case TIND:
	case TARRAY:
		i = BtoR(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return RtoB(i);
		}
		break;

	case TFLOAT:
		i = BtoF(~b);
		if(i && r->cost > 0) {
			r->regno = i+Nreg;
			return FtoB(i);
		}
		break;
	}
	return 0;
}

void
paint1(Reg *r, int bn)
{
	Reg *r1;
	Inst *p;
	int z;
	ulong bb;

	z = bn/32;
	bb = 1L<<(bn%32);
	if(r->act.b[z] & bb)
		return;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(r1->act.b[z] & bb)
			break;
		r = r1;
	}

	if(LOAD(r) & ~(r->set.b[z]&~(r->use1.b[z]|r->use2.b[z])) & bb) {
		change -= CLOAD * r->loop;
		if(opt('R') && opt('v'))
			print("%d%i%|ld %B $%d\n", r->loop,
				r->prog, COL1, blsh(bn), change);
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			change += CREF * r->loop;
			if(opt('R') && opt('v'))
				print("%d%i%|u1 %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
		}

		if((r->use2.b[z]|r->set.b[z]) & bb) {
			change += CREF * r->loop;
			if(opt('R') && opt('v'))
				print("%d%i%|u2 %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
		}

		if(STORE(r) & r->regdiff.b[z] & bb) {
			change -= CLOAD * r->loop;
			if(opt('R') && opt('v'))
				print("%d%i%|st %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
		}

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2next)
				if(r1->refahead.b[z] & bb)
					paint1(r1, bn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				paint1(r1, bn);
		r = r->s1;
		if(r == R)
			break;
		if(r->act.b[z] & bb)
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
}

ulong
paint2(Reg *r, int bn)
{
	Reg *r1;
	int z;
	ulong bb, vreg;

	z = bn/32;
	bb = 1L << (bn%32);
	vreg = regbits;
	if(!(r->act.b[z] & bb))
		return vreg;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(!(r1->act.b[z] & bb))
			break;
		r = r1;
	}
	for(;;) {
		r->act.b[z] &= ~bb;

		vreg |= r->regu;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2next)
				if(r1->refahead.b[z] & bb)
					vreg |= paint2(r1, bn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				vreg |= paint2(r1, bn);
		r = r->s1;
		if(r == R)
			break;
		if(!(r->act.b[z] & bb))
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
	return vreg;
}

void
paint3(Reg *r, int bn, long rb, int rn)
{
	Reg *r1;
	Inst *p;
	int z;
	ulong bb;

	z = bn/32;
	bb = 1L << (bn%32);
	if(r->act.b[z] & bb)
		return;
	for(;;) {
		if(!(r->refbehind.b[z] & bb))
			break;
		r1 = r->p1;
		if(r1 == R)
			break;
		if(!(r1->refahead.b[z] & bb))
			break;
		if(r1->act.b[z] & bb)
			break;
		r = r1;
	}

	if(LOAD(r) & ~(r->set.b[z] & ~(r->use1.b[z]|r->use2.b[z])) & bb)
		addmove(r, bn, rn, 0);
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			if(opt('R'))
				print("%i", p);
			addreg(&p->src1, rn);
			if(opt('R'))
				print("%|.c%i\n", COL1, p);
		}
		if((r->use2.b[z]|r->set.b[z]) & bb) {
			if(opt('R'))
				print("%i", p);
			addreg(&p->dst, rn);
			if(opt('R'))
				print("%|.c%i\n", COL1, p);
		}

		if(STORE(r) & r->regdiff.b[z] & bb)
			addmove(r, bn, rn, 1);
		r->regu |= rb;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2next)
				if(r1->refahead.b[z] & bb)
					paint3(r1, bn, rb, rn);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				paint3(r1, bn, rb, rn);
		r = r->s1;
		if(r == R)
			break;
		if(r->act.b[z] & bb)
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
}

void
addreg(Adres *a, int rn)
{
	a->sym = 0;
	a->type = A_REG;
	a->class = 0;
	a->reg = rn;
	if(rn >= Nreg) {
		a->type = A_FREG;
		a->reg = rn-Nreg;
	}
}

/*
 *	bit	reg
 *	0	R3
 *	1	R4
 *	...	...
 *	19	R22
 *	20	R23
 */
long
RtoB(int r)
{

	if(r < 3 || r > 23)
		return 0;
	return 1L << (r-3);
}

BtoR(long b)
{

	b &= 0x001fffffL;
	if(b == 0)
		return 0;
	return bitno(b) + 3;
}

/*
 *	bit	reg
 *	22	F4
 *	23	F6
 *	...	...
 *	31	F22
 */
long
FtoB(int f)
{
	if(f < 4 || f > 22 || (f&1))
		return 0;
	return 1L << (f/2 + 20);
}

BtoF(long b)
{

	b &= 0xffc00000L;
	if(b == 0)
		return 0;
	return bitno(b)*2 - 40;
}

int
bany(Bits *a)
{
	int i;

	for(i=0; i<BITS; i++)
		if(a->b[i])
			return 1;
	return 0;
}

int
bnum(Bits a)
{
	int i;
	long b;

	for(i=0; i<BITS; i++)
		if(b = a.b[i])
			return 32*i + bitno(b);
	diag(ZeroN, "bad in bnum");
	return 0;
}

Bits
blsh(int n)
{
	Bits c;

	c = zbits;
	c.b[n/32] = 1L << (n%32);
	return c;
}

int
Bconv(void *o, Fconv *f)
{
	char str[128], ss[128], *s;
	Bits bits;
	int i;

	str[0] = 0;
	bits = *(Bits*)o;
	while(bany(&bits)) {
		i = bnum(bits);
		if(str[0])
			strcat(str, " ");
		if(var[i].sym == ZeroS) {
			sprint(ss, "$%ld", var[i].ival);
			s = ss;
		} else
			s = var[i].sym->name;
		if(strlen(str) + strlen(s) + 1 >= 128)
			break;
		strcat(str, s);
		bits.b[i/32] &= ~(1L << (i%32));
	}
	strconv(str, f);
	return sizeof(bits);
}

int
bitno(long b)
{
	int i;

	for(i=0; i<32; i++)
		if(b & (1L<<i))
			return i;
	diag(ZeroN, "bad in bitno");
	return 0;
}
