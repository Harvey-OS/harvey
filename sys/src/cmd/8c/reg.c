#include "gc.h"

#define	COL1	40

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
	c2 = p1->cost;
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
	regbits = RtoB(D_SP) | RtoB(D_AX);
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
		case ARET:
		case AJMP:
		case AIRETL:
			r->p1 = R;
			r1->s1 = R;
		}

		bit = mkvar(r, &p->from);
		if(bany(&bit))
		switch(p->as) {
		/*
		 * funny
		 */
		case ALEAL:
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			break;

		/*
		 * left side read
		 */
		default:
			for(z=0; z<BITS; z++)
				r->use1.b[z] |= bit.b[z];
			break;
		}

		bit = mkvar(r, &p->to);
		if(bany(&bit))
		switch(p->as) {
		default:
			diag(Z, "reg: unknown op: %A", p->as);
			break;

		/*
		 * right side read
		 */
		case ACMPB:
		case ACMPL:
		case ACMPW:
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];
			break;

		/*
		 * right side write
		 */
		case ANOP:
		case AMOVL:
		case AMOVB:
		case AMOVW:
		case AMOVBLSX:
		case AMOVBLZX:
		case AMOVWLSX:
		case AMOVWLZX:
			for(z=0; z<BITS; z++)
				r->set.b[z] |= bit.b[z];
			break;

		/*
		 * right side read+write
		 */
		case AADDB:
		case AADDL:
		case AADDW:
		case AANDB:
		case AANDL:
		case AANDW:
		case ASUBB:
		case ASUBL:
		case ASUBW:
		case AORB:
		case AORL:
		case AORW:
		case AXORB:
		case AXORL:
		case AXORW:
		case ASALB:
		case ASALL:
		case ASALW:
		case ASARB:
		case ASARL:
		case ASARW:
		case AROLB:
		case AROLL:
		case AROLW:
		case ARORB:
		case ARORL:
		case ARORW:
		case ASHLB:
		case ASHLL:
		case ASHLW:
		case ASHRB:
		case ASHRL:
		case ASHRW:
			for(z=0; z<BITS; z++) {
				r->set.b[z] |= bit.b[z];
				r->use2.b[z] |= bit.b[z];
			}
			break;

		/*
		 * funny
		 */
		case AFMOVDP:
		case AFMOVFP:
		case ACALL:
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			break;
		}

		switch(p->as) {
		case AIDIVB:
		case AIDIVL:
		case AIDIVW:
		case AIMULB:
		case AIMULL:
		case AIMULW:
		case ADIVB:
		case ADIVL:
		case ADIVW:
		case AMULB:
		case AMULL:
		case AMULW:

		case ACWD:
		case ACDQ:
			r->regu |= RtoB(D_AX) | RtoB(D_DX);
			break;

		case AREP:
		case AREPN:
		case ALOOP:
		case ALOOPEQ:
		case ALOOPNE:
			r->regu |= RtoB(D_CX);
			break;

		case AMOVSB:
		case AMOVSL:
		case AMOVSW:
		case ACMPSB:
		case ACMPSL:
		case ACMPSW:
			r->regu |= RtoB(D_SI) | RtoB(D_DI);
			break;

		case ASTOSB:
		case ASTOSL:
		case ASTOSW:
		case ASCASB:
		case ASCASL:
		case ASCASW:
			r->regu |= RtoB(D_AX) | RtoB(D_DI);
			break;

		case AINSB:
		case AINSL:
		case AINSW:
		case AOUTSB:
		case AOUTSL:
		case AOUTSW:
			r->regu |= RtoB(D_DI) | RtoB(D_DX);
			break;

		case AFSTSW:
		case ASAHF:
			r->regu |= RtoB(D_AX);
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
					   r->use2.b[z] |
					   r->set.b[z];
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
		if(r->prog->as == ARET)
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
		if(debug['R'] && debug['v']) {
			print("%P%|", r->prog, COL1);
			if(bany(&r->set))
				print("s:%B ", r->set);
			if(bany(&r->refahead))
				print("ra:%B ", r->refahead);
			if(bany(&r->calahead))
				print("ca:%B ", r->calahead);
			print("\n");
		}
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
			bit.b[z] = LOAD(r) & ~(r->act.b[z] | addrs.b[z]);
		while(bany(&bit)) {
			i = bnum(bit);
			rgp->enter = r;
			rgp->varno = i;
			change = 0;
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
			print("%L$%d %R: %B\n",
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

/*
 * add mov b,rn
 * just after r
 */
void
addmove(Reg *r, int bn, int rn, int f)
{
	Prog *p, *p1;
	Adr *a;
	Var *v;

	ALLOC(p1,Prog);
	*p1 = zprog;
	p = r->prog;

	p1->link = p->link;
	p->link = p1;
	p1->lineno = p->lineno;

	v = var + bn;

	a = &p1->to;
	a->sym = v->sym;
	a->offset = v->offset;
	a->etype = v->etype;
	a->type = v->name;

	p1->as = AMOVL;
	if(v->etype == TCHAR || v->etype == TUCHAR)
		p1->as = AMOVB;
	if(v->etype == TSHORT || v->etype == TUSHORT)
		p1->as = AMOVW;

	p1->from.type = rn;
	if(!f) {
		p1->from = *a;
		*a = zprog.from;
		a->type = rn;
		if(v->etype == TUCHAR)
			p1->as = AMOVB;
		if(v->etype == TUSHORT)
			p1->as = AMOVW;
	}
	if(debug['R'])
		print("%P%|.a%P\n", p, COL1, p1);
}

ulong
doregbits(int r)
{
	ulong b;

	b = 0;
	if(r >= D_INDIR)
		r -= D_INDIR;
	if(r >= D_AX && r <= D_DI)
		b |= RtoB(r);
	else
	if(r >= D_AL && r <= D_BL)
		b |= RtoB(r-D_AL+D_AX);
	else
	if(r >= D_AH && r <= D_BH)
		b |= RtoB(r-D_AH+D_AX);
	return b;
}

Bits
mkvar(Reg *r, Adr *a)
{
	Var *v;
	int i, t, n, et, z;
	long o;
	Bits bit;
	Sym *s;

	/*
	 * mark registers used
	 */
	t = a->type;
	r->regu |= doregbits(t);
	r->regu |= doregbits(a->index);

	switch(t) {
	default:
		goto none;
	case D_ADDR:
		a->type = a->index;
		bit = mkvar(r, a);
		for(z=0; z<BITS; z++)
			addrs.b[z] |= bit.b[z];
		a->type = t;
		goto none;
	case D_EXTERN:
	case D_STATIC:
	case D_PARAM:
	case D_AUTO:
		n = t;
		break;
	}
	s = a->sym;
	if(s == S)
		goto none;
	if(s->name[0] == '.')
		goto none;
	et = a->etype;
	o = a->offset;
	v = var;
	for(i=0; i<nvar; i++) {
		if(s == v->sym)
		if(n == v->name)
		if(o == v->offset)
			goto out;
		v++;
	}
	if(nvar >= NVAR) {
		warn(Z, "variable not optimized: %s", s->name);
		goto none;
	}
	i = nvar;
	nvar++;
	v = &var[i];
	v->sym = s;
	v->offset = o;
	v->name = n;
	v->etype = et;
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

ulong
allreg(ulong b, Rgn *r)
{
	Var *v;
	int i;

	v = var + r->varno;
	r->regno = 0;
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
	case TIND:
	case TARRAY:
		i = BtoR(~b);
		if(i && r->cost > 0) {
			r->regno = i;
			return RtoB(i);
		}
		break;

	case TVLONG:
	case TDOUBLE:
	case TFLOAT:
		break;
	}
	return 0;
}

void
paint1(Reg *r, int bn)
{
	Reg *r1;
	Prog *p;
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
		if(debug['R'] && debug['v'])
			print("%d%P%|ld %B $%d\n", r->loop,
				r->prog, COL1, blsh(bn), change);
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			change += CREF * r->loop;
			if(p->as == AFMOVL)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%d%P%|u1 %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
		}

		if((r->use2.b[z]|r->set.b[z]) & bb) {
			change += CREF * r->loop;
			if(p->as == AFMOVL)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%d%P%|u2 %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
		}

		if(STORE(r) & r->regdiff.b[z] & bb) {
			change -= CLOAD * r->loop;
			if(p->as == AFMOVL)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%d%P%|st %B $%d\n", r->loop,
					p, COL1, blsh(bn), change);
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
regset(Reg *r, ulong bb)
{
	ulong b, set;
	Adr v;
	int c;

	set = 0;
	v = zprog.from;
	while(b = bb & ~(bb-1)) {
		v.type = BtoR(b);
		c = copyu(r->prog, &v, A);
		if(c == 3)
			set |= b;
		bb &= ~b;
	}
	return set;
}

ulong
reguse(Reg *r, ulong bb)
{
	ulong b, set;
	Adr v;
	int c;

	set = 0;
	v = zprog.from;
	while(b = bb & ~(bb-1)) {
		v.type = BtoR(b);
		c = copyu(r->prog, &v, A);
		if(c == 1 || c == 2 || c == 4)
			set |= b;
		bb &= ~b;
	}
	return set;
}

ulong
paint2(Reg *r, int bn)
{
	Reg *r1;
	int z;
	ulong bb, vreg, x;

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

	bb = vreg;
	for(; r; r=r->s1) {
		x = r->regu & ~bb;
		if(x) {
			vreg |= reguse(r, x);
			bb |= regset(r, x);
		}
	}
	return vreg;
}

void
paint3(Reg *r, int bn, long rb, int rn)
{
	Reg *r1;
	Prog *p;
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
			if(debug['R'])
				print("%P", p);
			addreg(&p->from, rn);
			if(debug['R'])
				print("%|.c%P\n", COL1, p);
		}
		if((r->use2.b[z]|r->set.b[z]) & bb) {
			if(debug['R'])
				print("%P", p);
			addreg(&p->to, rn);
			if(debug['R'])
				print("%|.c%P\n", COL1, p);
		}

		if(STORE(r) & r->regdiff.b[z] & bb)
			addmove(r, bn, rn, 1);
		r->regu |= rb;

		if(r->refbehind.b[z] & bb)
			for(r1 = r->p2; r1 != R; r1 = r1->p2link)
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
addreg(Adr *a, int rn)
{

	a->sym = 0;
	a->offset = 0;
	a->type = rn;
}

long
RtoB(int r)
{

	switch(r < D_AX || r > D_DI || r == D_SP)
		return 0;
	return 1L << (r-D_AX);
}

int
BtoR(long b)
{

	b &= 0xffL;
	if(b == 0)
		return 0;
	return bitno(b) + D_AX;
}
