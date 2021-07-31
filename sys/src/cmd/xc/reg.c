#include "gc.h"

#define	COL1	32

Reg*
rega(void)
{
	Reg *r;

	r = freer;
	if(r == R) {
		ALLOC(r, Reg);
	} else
		freer = r->link;

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
	if(c1 < p2->costa)
		c1 = p2->costa;
	c2 = p1->cost;
	if(c2 < p1->costa)
		c2 = p1->costa;
	if(c1 -= c2)
		return c1;
	return p2->varno - p1->varno;
}

void
regopt(Prog *p)
{
	Reg *r, *r1, *r2;
	Prog *p1;
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

	firstr = R;
	lastr = R;
	nvar = 0;
	regbits = RtoB(REGLINK) | RtoB(14);
	for(z=0; z<BITS; z++) {
		externs.b[z] = 0;
		params.b[z] = 0;
		consts.b[z] = 0;
		addrs.b[z] = 0;
	}

	/*
	 * pass 1
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
	val = 0;
	for(; p != P; p = p->link) {
		switch(p->as) {
		case ADATA:
		case AGLOBL:
		case ANAME:
			continue;
		}
		r = rega();
		if(firstr == R) {
			firstr = r;
			lastr = r;
		} else {
			lastr->link = r;
			r->p1 = lastr;
			lastr->s1 = r;
			lastr = r;
		}
		r->prog = p;
		r->pc = val;
		val++;

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
		switch(r1->prog->as) {
		case ARETURN:
		case AJMP:
		case AIRET:
			r->p1 = R;
			r1->s1 = R;
		}

		/*
		 * left side always read
		 */
		bit = mkvar(&p->from, p->as==AMOVW);
		for(z=0; z<BITS; z++)
			r->use1.b[z] |= bit.b[z];

		/*
		 * other left side always read
		 * if float, reg gets clobbered
		 */
		if(p->isf){
			bit = mkvar(&p->from1, 0);
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];
			if(p->reg != NREG)
				regbits |= FtoB(p->reg);
		}

		/*
		 * right side depends on opcode
		 */
		bit = mkvar(&p->to, 0);
		if(bany(&bit))
		switch(p->as) {
		default:
			diag(Z, "reg: unknown asop: %A", p->as);
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
		case AFMOVF:
		case AFMOVFW:
		case AFMOVFH:
		case AFMOVHF:
		case AFMOVWF:
			for(z=0; z<BITS; z++)
				r->set.b[z] |= bit.b[z];
			break;

		/*
		 * funny
		 */
		case ACALL:
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			break;
		}
	}
	if(firstr == R)
		return;
	initpc = pc - val;

	/*
	 * pass 2
	 * turn branch references to pointers
	 * build back pointers
	 */
	for(r = firstr; r != R; r = r->link) {
		p = r->prog;
		if(p->to.type == D_BRANCH) {
			val = p->to.offset - initpc;
			r1 = firstr;
			while(r1 != R) {
				r2 = r1->log5;
				if(r2 != R && val >= r2->pc) {
					r1 = r2;
					continue;
				}
				if(r1->pc == val)
					break;
				r1 = r1->link;
			}
			if(r1 == R) {
				nearln = p->lineno;
				diag(Z, "ref not found\n%P", p);
				continue;
			}
			if(r1 == r) {
				nearln = p->lineno;
				diag(Z, "ref to self\n%P", p);
				continue;
			}
			r->s2 = r1;
			r->p2link = r1->p2;
			r1->p2 = r;
		}
	}
	if(debug['R']) {
		p = firstr->prog;
		print("\n%L %D\n", p->lineno, &p->from);
	}

	/*
	 * pass 2.5
	 * find looping structure
	 */
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	change = 0;
	loopit(firstr);
	if(debug['R'] && debug['v']) {
		print("\nlooping structure:\n");
		for(r = firstr; r != R; r = r->link) {
			print("%d:%P", r->loop, r->prog);
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
	 * pass 3
	 * iterate propagating usage
	 * 	back until flow graph is complete
	 */
loop1:
	change = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	for(r = firstr; r != R; r = r->link)
		if(r->prog->as == ARETURN)
			prop(r, zbits, zbits);
loop11:
	/* pick up unreachable code */
	i = 0;
	for(r = firstr; r != R; r = r1) {
		r1 = r->link;
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
	 * pass 4
	 * iterate propagating register/variable synchrony
	 * 	forward until graph is complete
	 */
loop2:
	change = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	synch(firstr, zbits);
	if(change)
		goto loop2;


	/*
	 * pass 5
	 * isolate regions
	 * calculate costs (paint1)
	 */
	r = firstr;
	if(r) {
		for(z=0; z<BITS; z++)
			bit.b[z] = (r->refahead.b[z] | r->calahead.b[z]) &
			  ~(externs.b[z] | params.b[z] | addrs.b[z] | consts.b[z]);
		if(bany(&bit)) {
			nearln = r->prog->lineno;
			warn(Z, "used and not set: %B", bit);
			if(debug['R'] && !debug['w'])
				print("used and not set: %B\n", bit);
		}
	}
	if(debug['R'] && debug['v'])
		print("\nprop structure:\n");
	for(r = firstr; r != R; r = r->link)
		r->act = zbits;
	rgp = region;
	nregion = 0;
	for(r = firstr; r != R; r = r->link) {
		if(debug['R'] && debug['v'])
			print("%P\n	set = %B; rah = %B; cal = %B\n",
				r->prog, r->set, r->refahead, r->calahead);
		for(z=0; z<BITS; z++)
			bit.b[z] = r->set.b[z] &
			  ~(r->refahead.b[z] | r->calahead.b[z] | addrs.b[z]);
		if(bany(&bit)) {
			nearln = r->prog->lineno;
			warn(Z, "set and not used: %B", bit);
			if(debug['R'])
				print("set an not used: %B\n", bit);
			excise(r);
		}
		for(z=0; z<BITS; z++)
			bit.b[z] = LOAD(r, z) & ~(r->act.b[z] | addrs.b[z]);
		while(bany(&bit)) {
			i = bnum(bit);
			rgp->enter = r;
			rgp->varno = i;
			change = 0;
			changea = 0;
			if(debug['R'] && debug['v'])
				print("\n");
			paint1(r, i);
			bit.b[i/32] &= ~(1L<<(i%32));
			if(change <= 0) {
				if(debug['R'])
					print("%L$%d: %B\n",
						r->prog->lineno, change, blsh(i));
				continue;
			}
			rgp->cost = change;
			rgp->costa = changea;
			nregion++;
			if(nregion >= NRGN) {
				warn(Z, "too many regions");
				goto brk;
			}
			rgp++;
		}
	}
brk:
	qsort(region, nregion, sizeof(region[0]), rcmp);

	/*
	 * pass 6
	 * determine used registers (paint2)
	 * replace code (paint3)
	 */
	rgp = region;
	for(i=0; i<nregion; i++) {
		bit = blsh(rgp->varno);
		vreg = paint2(rgp->enter, rgp->varno);
		vreg = allreg(vreg, rgp);
		if(debug['R']) {
			if(rgp->regno >= NREG)
				print("%L$%d $%d F%d: %B\n",
					rgp->enter->prog->lineno,
					rgp->cost, rgp->costa,
					rgp->regno-NREG,
					bit);
			else
				print("%L$%d $%d R%d: %B\n",
					rgp->enter->prog->lineno,
					rgp->cost, rgp->costa,
					rgp->regno,
					bit);
		}
		if(rgp->regno != 0)
			paint3(rgp->enter, rgp->varno, vreg, rgp->regno, rgp->isa);
		rgp++;
	}
	/*
	 * pass 7
	 * peep-hole on basic block
	 */
	if(!debug['R'] || debug['P'])
		peep();

	/*
	 * pass 8
	 * recalculate pc
	 */
	val = initpc;
	for(r = firstr; r != R; r = r1) {
		r->pc = val;
		p = r->prog;
		p1 = P;
		r1 = r->link;
		if(r1 != R)
			p1 = r1->prog;
		for(; p != p1; p = p->link) {
			switch(p->as) {
			default:
				val++;
				break;

			case ANOP:
			case ADATA:
			case AGLOBL:
			case ANAME:
				break;
			}
		}
	}
	pc = val;

	/*
	 * pass 9
	 * fix up branches
	 */
	if(debug['R'])
		if(bany(&addrs))
			print("addrs: %B\n", addrs);

	r1 = 0; /* set */
	for(r = firstr; r != R; r = r->link) {
		p = r->prog;
		if(p->to.type == D_BRANCH)
			p->to.offset = r->s2->pc;
		r1 = r;
	}

	/*
	 * last pass
	 * eliminate nops
	 * free aux structures
	 */
	for(p = firstr->prog; p != P; p = p->link){
		while(p->link && p->link->as == ANOP)
			p->link = p->link->link;
	}
	if(r1 != R) {
		r1->link = freer;
		freer = firstr;
	}
}

Bits
mkvar(Adr *a, int docon)
{
	Var *v;
	int i, t, n, et, z;
	long o;
	Bits bit;
	Sym *s;

	t = a->type;
	if(t == D_REG && a->reg != NREG)
		regbits |= RtoB(a->reg);
	if(t == D_FREG && a->reg != NREG)
		regbits |= FtoB(a->reg);
	s = a->sym;
	o = a->offset;
	et = a->etype;
	if(s == S) {
		if(t != D_CONST || !docon || a->reg != NREG)
			goto none;
		et = TLONG;
	}
	if(t == D_CONST) {
		if(s == S && sval(o))
			goto none;
	}
	n = a->name;
	v = var;
	for(i=0; i<nvar; i++) {
		if(s == v->sym)
		if(n == v->name)
		if(o == v->offset)
			goto out;
		v++;
	}
	if(s)
		if(s->name[0] == '.')
			goto none;
	if(nvar >= NVAR) {
		if(s)
		warn(Z, "variable not optimized: %s", s->name);
		goto none;
	}
	i = nvar;
	nvar++;
	v = &var[i];
	v->sym = s;
	v->offset = o;
	v->etype = et;
	v->name = n;
	if(debug['R'])
		print("bit=%2d et=%2d %D\n", i, et, a);
out:
	bit = blsh(i);
	if(n == D_EXTERN || n == D_STATIC)
		for(z=0; z<BITS; z++)
			externs.b[z] |= bit.b[z];
	if(n == D_PARAM)
		for(z=0; z<BITS; z++)
			params.b[z] |= bit.b[z];
	if(v->etype != et || !typechlpfd[et])	/* funny punning */
		for(z=0; z<BITS; z++)
			addrs.b[z] |= bit.b[z];
	if(t == D_CONST) {
		if(s == S) {
			for(z=0; z<BITS; z++)
				consts.b[z] |= bit.b[z];
			return bit;
		}
		if(et != TARRAY)
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
		for(z=0; z<BITS; z++)
			params.b[z] |= bit.b[z];
		return bit;
	}
	if(t == D_NAME)
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
		switch(r1->prog->as) {
		case ACALL:
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

		case ARETURN:
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
		for(r2 = r->p2; r2 != R; r2 = r2->p2link)
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

/*
 * select a register
 * b is the mask of used regs
 */
ulong
allreg(ulong b, Rgn *r)
{
	Var *v;
	int i, a;

	v = var + r->varno;
	r->regno = 0;
	r->isa = 0;
	switch(v->etype) {

	default:
		diag(Z, "unknown etype %d/%d", bitno(b), v->etype);
		break;

	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
		i = BtoR(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return RtoB(i);
		}
		break;
	case TIND:
	case TARRAY:
		i = BtoA(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return AtoB(i);
		}
		break;

	case TVLONG:
	case TDOUBLE:
	case TFLOAT:
		a = BtoA(~b);
		i = BtoF(~b);
		if(a && r->costa > 0 && (!i || r->costa > r->cost)) {
			r->regno = a;
			r->isa = 1;
			return AtoB(a);
		}
		if(i && r->cost > 0) {
			r->regno = i+NREG;
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
	Prog *p;
	ulong bb;
	int z;

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

	if(r->prog->isf && ((LOAD(r, z)|r->set.b[z]) & bb)){
		changea -= CLOADA * r->loop;
		if(debug['R'] && debug['v'])
			print("%d%P%|la %B $%d $%d\n", r->loop,
				r->prog, COL1, blsh(bn), change, changea);
	}
	if(LOAD(r, z) & ~(r->set.b[z]&~(r->use1.b[z]|r->use2.b[z])) & bb) {
		change -= CLOAD * r->loop;
		if(debug['R'] && debug['v'])
			print("%d%P%|ld %B $%d $%d\n", r->loop,
				r->prog, COL1, blsh(bn), change, changea);
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			change += CREF * r->loop;
			if(r->prog->isf)
				changea += CREFA * r->loop;
			if(p->as == AFMOVWF || p->as == AFMOVHF
			|| p->as == AFMOVFW || p->as == AFMOVFH)
				change = -CINF;		/* cant go Rreg to Freg */
			if(debug['R'] && debug['v'])
				print("%d%P%|u1 %B $%d $%d\n", r->loop,
					p, COL1, blsh(bn), change, changea);
		}
		if(r->use2.b[z] & bb) {
			change += CREF * r->loop;
			if(r->prog->isf)
				changea += CREFA * r->loop;
			if(p->as == AFMOVWF || p->as == AFMOVHF
			|| p->as == AFMOVFW || p->as == AFMOVFH)
				change = -CINF;		/* cant go Rreg to Freg */
			if(debug['R'] && debug['v'])
				print("%d%P%|u2 %B $%d $%d\n", r->loop,
					p, COL1, blsh(bn), change, changea);
		}
		if(r->set.b[z] & bb) {
			change += CREF * r->loop;
			if(r->prog->isf)
				changea += CREFA * r->loop;
			if(p->as == AFMOVWF || p->as == AFMOVHF
			|| p->as == AFMOVFW || p->as == AFMOVFH)
				change = -CINF;		/* cant go Rreg to Freg */
			if(debug['R'] && debug['v'])
				print("%d%P%|se %B $%d $%d\n", r->loop,
					p, COL1, blsh(bn), change, changea);
		}

		if(STORE(r, z) & r->regdiff.b[z] & bb) {
			change -= CLOAD * r->loop;
			if(debug['R'] && debug['v'])
				print("%d%P%|st %B $%d $%d\n", r->loop,
					p, COL1, blsh(bn), change, changea);
		}

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
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
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
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
paint3(Reg *r, int bn, long rb, int rn, int isa)
{
	Reg *r1;
	Prog *p;
	ulong bb;
	int z;

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

	if(isa && (r->set.b[z] & bb)){
		addmove(r, bn, rn, 2, 0);
		r->act.b[z] |= bb;
		r->regu |= rb;
		r = r->link;
	}else if(isa && (LOAD(r, z) & bb))
		addmove(r, bn, rn, 2, 1);
	else if(LOAD(r, z) & ~(r->set.b[z] & ~(r->use1.b[z]|r->use2.b[z])) & bb)
		addmove(r, bn, rn, 1, 1);

	for(;;) {
		p = r->prog;

		if(r->use1.b[z] & bb) {
			if(debug['R'])
				print("%P", p);
			if(isa)
				addref(&p->from, rn);
			else
				addreg(&p->from, rn);
			if(debug['R'])
				print("%|.c%P\n", COL1, p);
		}
		if(r->use2.b[z] & bb) {
			if(debug['R'])
				print("%P", p);
			if(isa)
				addref(&p->from1, rn);
			else
				addreg(&p->from1, rn);
			if(debug['R'])
				print("%|.c%P\n", COL1, p);
		}
		if(r->set.b[z] & bb) {
			if(debug['R'])
				print("%P", p);
			if(isa)
				addref(&p->to, rn);
			else
				addreg(&p->to, rn);
			if(debug['R'])
				print("%|.c%P\n", COL1, p);
		}

		if(STORE(r, z) & r->regdiff.b[z] & bb)
		if(!isa)
			addmove(r, bn, rn, 0, 1);

		r->act.b[z] |= bb;
		r->regu |= rb;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
				if(r1->refahead.b[z] & bb)
					paint3(r1, bn, rb, rn, isa);

		if(!(r->refahead.b[z] & bb))
			break;
		r1 = r->s2;
		if(r1 != R)
			if(r1->refbehind.b[z] & bb)
				paint3(r1, bn, rb, rn, isa);
		r = r->s1;
		if(r == R)
			break;
		if(r->act.b[z] & bb)
			break;
		if(!(r->refbehind.b[z] & bb))
			break;
	}
}

/*
 * add mov b,rn
 */
void
addmove(Reg *r, int bn, int rn, int kind, int after)
{
	Prog *p, *p1;
	Adr *a;
	Var *v;

	ALLOC(p1,Prog);
	p = r->prog;
	if(after){
		*p1 = zprog;
		p1->link = p->link;
		p->link = p1;
	}else{
		p = p1;
		p1 = r->prog;
		*p = *p1;
		*p1 = zprog;
		p1->link = p;
	}
	p1->lineno = p->lineno;

	v = var + bn;

	a = &p1->to;
	a->sym = v->sym;
	a->name = v->name;
	a->offset = v->offset;
	a->etype = v->etype;
	a->type = D_NAME;
	if(kind == 2 || a->etype == TARRAY || a->sym == S)
		a->type = D_CONST;

	p1->as = AMOVW;
	if(v->etype == TCHAR || v->etype == TUCHAR)
		p1->as = AMOVB;
	if(v->etype == TSHORT || v->etype == TUSHORT)
		p1->as = AMOVH;
	if(v->etype == TFLOAT || v->etype == TDOUBLE)
		p1->as = AFMOVF;

	p1->from.type = D_REG;
	p1->from.reg = rn;
	if(rn >= NREG) {
		p1->from.type = D_FREG;
		p1->from.reg = rn-NREG;
		p1->isf = 1;
	}
	if(kind){
		p1->from = *a;
		*a = zprog.from;
		a->type = D_REG;
		a->reg = rn;
		if(rn >= NREG) {
			a->type = D_FREG;
			a->reg = rn-NREG;
		}
		if(v->etype == TUCHAR)
			p1->as = AMOVBU;
		if(v->etype == TUSHORT)
			p1->as = AMOVHU;
		if(kind == 2)
			p1->as = AMOVW;
	}
	if(debug['R'])
		if(after)
			print("%P%|.a%P\n", p, COL1, p1);
		else
			print("%P%|.b%P\n", p, COL1, p1);
	newreg(r);
	if(after)
		regafter(r);
	else
		regbefore(r);
}

/*
 * add a reg for the instruction after r
 * neither r's original prog nor the inserted prog are jumps
 * don't set the opt bits
 */
void
newreg(Reg *r)
{
	Reg *r1;

	r1 = rega();
	r1->act = r->act;
	r1->regu = r->regu;
	r1->loop = r->loop;

	r1->link = r->link;
	r->link = r1;

	r1->s1 = r->s1;
	if(r1->s1)
		r1->s1->p1 = r1;
	r->s1 = r1;
	r1->p1 = r;
	r1->s2 = r1->p2 = r1->p2link = 0;
	r1->prog = r->prog->link;
}

/*
 * set opt bits for
 * r->prog->link, which doesn't modify any bits
 */
void
regafter(Reg *r)
{
	Reg *r1;

	r1 = r->s1;
	r1->refahead = r->refahead;
	r1->refbehind = r->refahead;
	r1->calahead = r->calahead;
	r1->calbehind = r->calahead;
	r1->set = zbits;
	r1->use1 = zbits;
	r1->use2 = zbits;
}

/*
 * set optimization bits
 * r has bits for r->prog->link
 * and r->prog doesn't modify and bits
 */
void
regbefore(Reg *r)
{
	Reg *r1;

	r1 = r->s1;
	r1->refahead = r->refahead;
	r1->refbehind = r->refbehind;
	r1->calahead = r->calahead;
	r1->calbehind = r->calbehind;
	r1->set = r->set;
	r1->use1 = r->use1;
	r1->use2 = r->use2;

	r->set = zbits;
	r->use1 = zbits;
	r->use2 = zbits;
	r->refahead = r->refbehind;
	r->calahead = r->calbehind;
}

void
addreg(Adr *a, int rn)
{

	a->sym = 0;
	a->name = D_NONE;
	a->type = D_REG;
	a->reg = rn;
	if(rn >= NREG) {
		a->type = D_FREG;
		a->reg = rn-NREG;
	}
}

void
addref(Adr *a, int rn)
{
	a->sym = 0;
	a->name = D_NONE;
	a->type = D_INDREG;
	a->offset = 0;
	a->reg = rn;
	if(rn >= NREG)
		diag(Z, "bad refernce pointer %d\n", rn);
}

/*
 * integer registers are split as follows:
 * R0		$0
 * R1		sp
 * R2		tmp
 * R3		argument
 * R4-R14	int vals or pointers
 * R15-R17,R19	int vals
 * R18		link reg
 *	bit	reg
 *	0	R4
 *	1	R5
 *	...	...
 *	15	R19
 */
long
RtoB(int r)
{
	if(r >= 4 && r <= 19)
		return 1L << (r - 4);
	return 0;
}

int
BtoR(long b)
{
	long a;

	a = b & 0x0000f800L;
	if(a)
		return bitno(a) + 4;
	b &= 0x000007ffL;
	if(b)
		return bitno(b) + 4;
	return 0;
}

long
AtoB(int r)
{
	if(r >= 4 && r <= 14)
		return 1L << (r - 4);
	return 0;
}

int
BtoA(long b)
{
	b &= 0x000007ffL;
	if(b == 0)
		return 0;
	return bitno(b) + 4;
}

/*
 *	bit	reg
 *	16	F0
 *	17	F1
 *	18	F2
 *	19	F3
 */
long
FtoB(int f)
{

	if(f >= 0 && f <= 3)
		return 1L << (f + 16);
	return 0;
}

int
BtoF(long b)
{

	b &= 0x000f0000L;
	if(b == 0)
		return 0;
	return bitno(b) - 16;
}
