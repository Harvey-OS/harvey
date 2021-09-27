#include "gc.h"

Reg*
rega(void)
{
	Reg *r;

	r = freer;
	if(r == R) {
		r = alloc(sizeof(*r));
	} else
		freer = r->link;

	*r = zreg;
	return r;
}

int
rcmp(const void *a1, const void *a2)
{
	Rgn *p1, *p2;
	int c1, c2;

	p1 = (Rgn*)a1;
	p2 = (Rgn*)a2;
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
	long initpc, val, npc;
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
		case ASIGNAME:
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

		bit = mkvar(r, &p->from, p->as==AMOVL);
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

		bit = mkvar(r, &p->to, 0);
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
		case AIMULL:
		case AIMULW:
		case ANEGL:
		case ANOTL:
		case AADCL:
		case ASBBL:
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
		case AFMOVLP:
		case AFMOVVP:
		case AFMOVWP:
		case ACALL:
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			break;
		}

		switch(p->as) {
		case AIMULL:
		case AIMULW:
			if(p->to.type != D_NONE)
				break;

		case AIDIVB:
		case AIDIVL:
		case AIDIVW:
		case AIMULB:
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
	npc = val;

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
	loopit(firstr, npc);
	if(debug['R'] && debug['v']) {
		print("\nlooping structure:\n");
		for(r = firstr; r != R; r = r->link) {
			print("%ld:%P", r->loop, r->prog);
			for(z=0; z<BITS; z++)
				bit.b[z] = r->use1.b[z] |
					   r->use2.b[z] |
					   r->set.b[z];
			if(bany(&bit)) {
				print("\t");
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
			print("%P\t", r->prog);
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
				print("set and not used: %B\n", bit);
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
			case ASIGNAME:
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

	p1 = alloc(sizeof(*p1));
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
		print("%P\t.a%P\n", p, p1);
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
mkvar(Reg *r, Adr *a, int isro)
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
	et = a->etype;

	switch(t) {
	default:
		goto none;
	case D_INDIR+D_GS:
		if(!isro || 1)
			goto none;
		n = t;
		{static Sym er; a->sym = &er;}
		a->sym->name = "$extreg";
		break;
	case D_ADDR:
		a->type = a->index;
		bit = mkvar(r, a, 0);
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
		if(debug['w'] > 1 && s)
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

/*
 * find looping structure
 *
 * 1) find reverse postordering
 * 2) find approximate dominators,
 *	the actual dominators if the flow graph is reducible
 *	otherwise, dominators plus some other non-dominators.
 *	See Matthew S. Hecht and Jeffrey D. Ullman,
 *	"Analysis of a Simple Algorithm for Global Data Flow Problems",
 *	Conf.  Record of ACM Symp. on Principles of Prog. Langs, Boston, Massachusetts,
 *	Oct. 1-3, 1973, pp.  207-217.
 * 3) find all nodes with a predecessor dominated by the current node.
 *	such a node is a loop head.
 *	recursively, all preds with a greater rpo number are in the loop
 */
long
postorder(Reg *r, Reg **rpo2r, long n)
{
	Reg *r1;

	r->rpo = 1;
	r1 = r->s1;
	if(r1 && !r1->rpo)
		n = postorder(r1, rpo2r, n);
	r1 = r->s2;
	if(r1 && !r1->rpo)
		n = postorder(r1, rpo2r, n);
	rpo2r[n] = r;
	n++;
	return n;
}

long
rpolca(long *idom, long rpo1, long rpo2)
{
	long t;

	if(rpo1 == -1)
		return rpo2;
	while(rpo1 != rpo2){
		if(rpo1 > rpo2){
			t = rpo2;
			rpo2 = rpo1;
			rpo1 = t;
		}
		while(rpo1 < rpo2){
			t = idom[rpo2];
			if(t >= rpo2)
				fatal(Z, "bad idom");
			rpo2 = t;
		}
	}
	return rpo1;
}

int
doms(long *idom, long r, long s)
{
	while(s > r)
		s = idom[s];
	return s == r;
}

int
loophead(long *idom, Reg *r)
{
	long src;

	src = r->rpo;
	if(r->p1 != R && doms(idom, src, r->p1->rpo))
		return 1;
	for(r = r->p2; r != R; r = r->p2link)
		if(doms(idom, src, r->rpo))
			return 1;
	return 0;
}

void
loopmark(Reg **rpo2r, long head, Reg *r)
{
	if(r->rpo < head || r->active == head)
		return;
	r->active = head;
	r->loop += LOOP;
	if(r->p1 != R)
		loopmark(rpo2r, head, r->p1);
	for(r = r->p2; r != R; r = r->p2link)
		loopmark(rpo2r, head, r);
}

void
loopit(Reg *r, long nr)
{
	Reg *r1;
	long i, d, me;

	if(nr > maxnr) {
		rpo2r = alloc(nr * sizeof(Reg*));
		idom = alloc(nr * sizeof(long));
		maxnr = nr;
	}

	d = postorder(r, rpo2r, 0);
	if(d > nr)
		fatal(Z, "too many reg nodes");
	nr = d;
	for(i = 0; i < nr / 2; i++){
		r1 = rpo2r[i];
		rpo2r[i] = rpo2r[nr - 1 - i];
		rpo2r[nr - 1 - i] = r1;
	}
	for(i = 0; i < nr; i++)
		rpo2r[i]->rpo = i;

	idom[0] = 0;
	for(i = 0; i < nr; i++){
		r1 = rpo2r[i];
		me = r1->rpo;
		d = -1;
		if(r1->p1 != R && r1->p1->rpo < me)
			d = r1->p1->rpo;
		for(r1 = r1->p2; r1 != nil; r1 = r1->p2link)
			if(r1->rpo < me)
				d = rpolca(idom, d, r1->rpo);
		idom[i] = d;
	}

	for(i = 0; i < nr; i++){
		r1 = rpo2r[i];
		r1->loop++;
		if(r1->p2 != R && loophead(idom, r1))
			loopmark(rpo2r, i, r1);
	}
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
	case TINT:
	case TUINT:
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
			print("%ld%P\tld %B $%d\n", r->loop,
				r->prog, blsh(bn), change);
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			change += CREF * r->loop;
			if(p->as == AFMOVL || p->as == AFMOVW)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%ld%P\tu1 %B $%d\n", r->loop,
					p, blsh(bn), change);
		}

		if((r->use2.b[z]|r->set.b[z]) & bb) {
			change += CREF * r->loop;
			if(p->as == AFMOVL || p->as == AFMOVW)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%ld%P\tu2 %B $%d\n", r->loop,
					p, blsh(bn), change);
		}

		if(STORE(r) & r->regdiff.b[z] & bb) {
			change -= CLOAD * r->loop;
			if(p->as == AFMOVL || p->as == AFMOVW)
				if(BtoR(bb) != D_F0)
					change = -CINF;
			if(debug['R'] && debug['v'])
				print("%ld%P\tst %B $%d\n", r->loop,
					p, blsh(bn), change);
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
				print("\t.c%P\n", p);
		}
		if((r->use2.b[z]|r->set.b[z]) & bb) {
			if(debug['R'])
				print("%P", p);
			addreg(&p->to, rn);
			if(debug['R'])
				print("\t.c%P\n", p);
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

	if(r < D_AX || r > D_DI)
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
