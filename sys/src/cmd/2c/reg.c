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
	c1 = p2->costr;
	if(p2->costa > c1)
		c1 = p2->costa;
	c2 = p1->costr;
	if(p1->costa > c2)
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
	long val, initpc, npc;
	ulong vreg;
	Bits bit;
	Var *v;
	struct {
		long	m;
		long	c;
		Reg*	p;
	} log5[6], *lp;

	firstr = R;
	lastr = R;
	nvar = 0;
	for(z=0; z<BITS; z++) {
		externs.b[z] = 0;
		params.b[z] = 0;
		addrs.b[z] = 0;
	}
	regbits = RtoB(0) |		/* return reg */
		AtoB(6) | AtoB(7) |	/* sp and sb */
		FtoB(0) | FtoB(1);	/* floating return reg */
	for(i=0; i<NREG; i++) {
		if(regused[i])
			regbits |= RtoB(i);
		if(fregused[i])
			regbits |= FtoB(i);
		if(aregused[i])
			regbits |= AtoB(i);
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
		case ABRA:
		case ARTS:
		case ARTE:
			r->p1 = R;
			r1->s1 = R;
		}

		bit = mkvar(&p->from, AGOK);
		if(bany(&bit))
		switch(p->as) {
		case ALEA:
			if(!(mvbits & B_INDIR))
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];

		default:
			if(mvbits & B_ADDR)
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			for(z=0; z<BITS; z++)
				r->use1.b[z] |= bit.b[z];
		}

		bit = mkvar(&p->to, p->as);
		if(bany(&bit))
		switch(p->as) {
		case ABSR:	/* funny */
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			goto def;

		case APEA:
			if(!(mvbits & B_INDIR))
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];

		def:
		case ACMPB: case ACMPW: case ACMPL:
		case AFCMPF: case AFCMPD:
		case ATSTB: case ATSTW: case ATSTL:
		case AFTSTF: case AFTSTD:
		case ABFEXTU: case ABFEXTS:
			if(mvbits & B_ADDR)
			for(z=0; z<BITS; z++)
				addrs.b[z] |= bit.b[z];
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];
			break;

		default:
			diag(Z, "reg: unknown asop: %A", p->as);

		case AADDB: case AADDW: case AADDL:
		case ASUBB: case ASUBW: case ASUBL:
		case AANDB: case AANDW: case AANDL:
		case AORB: case AORW: case AORL:
		case AEORB: case AEORW: case AEORL:
		case ABFINS:
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];

		case ANOP:
		case AMOVB: case AMOVW: case AMOVL:
		case AFMOVEB: case AFMOVEW: case AFMOVEL:
		case ACLRB: case ACLRW: case ACLRL:
		case AFMOVEF: case AFMOVED:
			if(mvbits & B_INDIR)
			for(z=0; z<BITS; z++)
				r->use2.b[z] |= bit.b[z];
			else
			for(z=0; z<BITS; z++)
				r->set.b[z] |= bit.b[z];
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
				diag(Z, "ref not found\n%L:%P", p->lineno, p);
				continue;
			}
			if(r1 == r) {
				diag(Z, "ref to self");
				continue;
			}
			r->s2 = r1;
			r->p2link = r1->p2;
			r1->p2 = r;
		}
	}
	if(debug['R'])
		print("\n%L %D\n", firstr->prog->lineno, &firstr->prog->from);

	/*
	 * pass 2.5
	 * find looping structure
	 */
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	changer = 0;
	loopit(firstr, npc);
	if(debug['R'] && debug['v']) {
		print("\nlooping structure:\n");
		for(r = firstr; r != R; r = r->link) {
			print("%ld:%P", r->loop, r->prog);
			for(z=0; z<BITS; z++)
				bit.b[z] = r->use1.b[z] |
					r->use2.b[z] | r->set.b[z];
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
	changer = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	for(r = firstr; r != R; r = r->link)
		if(r->prog->as == ARTS)
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
	if(changer)
		goto loop1;

	/*
	 * pass 4
	 * iterate propagating register/variable synchrony
	 * 	forward until graph is complete
	 */
loop2:
	changer = 0;
	for(r = firstr; r != R; r = r->link)
		r->active = 0;
	synch(firstr, zbits);
	if(changer)
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
			  ~(externs.b[z] | params.b[z] | addrs.b[z]);
		if(bany(&bit)) {
			nearln = r->prog->lineno;
			warn(Z, "used and not set: %B", bit);
			if(debug['R'] && !debug['w'])
				print("used and not set: %B\n", bit);

			/*
			 * 68040 'feature':
			 *	load of a denormalized fp will trap
			 */
			while(bany(&bit)) {
				i = bnum(bit);
				bit.b[i/32] &= ~(1L << (i%32));
				v = var + i;
				if(v->type == D_AUTO) {
					r->set.b[i/32] |= (1L << (i%32));
					if(typefd[v->etype])
						addmove(r, i, NREG+NREG, 1);
				}
			}
		}
	}
	if(debug['R'] && debug['v'])
		print("\nprop structure:\n");
	for(r = firstr; r != R; r = r->link) {
		if(debug['R'] && debug['v'])
			print("%P\n	set = %B; rah = %B; cal = %B\n",
				r->prog, r->set, r->refahead, r->calahead);
		r->act = zbits;
	}
	rgp = region;
	nregion = 0;
	for(r = firstr; r != R; r = r->link) {
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
			changer = 0;
			changea = 0;
			if(debug['R'] && debug['v'])
				print("\n");
			paint1(r, i);
			bit.b[i/32] &= ~(1L<<(i%32));
			if(changer <= 0 && changea <= 0) {
				if(debug['R'])
					print("%L$%d.%d: %B\n",
						r->prog->lineno,
						changer, changea, blsh(i));
				continue;
			}
			rgp->costr = changer;
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
		if(debug['R'])
			print("%L$%d.%d %R: %B\n",
				rgp->enter->prog->lineno,
				rgp->costr, rgp->costa,
				rgp->regno,
				bit);
		if(rgp->regno != D_NONE)
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
	Var *v;
	int badccr;

	badccr = 0;
	p = r->prog;
	p1 = p->link;
	if(p1)
	switch(p1->as) {
	case AMOVW:
		if(p1->from.type == D_CCR)
			p = p1;
		break;

	case ABEQ:
	case ABNE:
	case ABLE:
	case ABLS:
	case ABLT:
	case ABMI:
	case ABGE:
	case ABPL:
	case ABGT:
	case ABHI:
	case ABCC:
	case ABCS:
		p1 = prg();
		p1->link = p->link;
		p->link = p1;
		p1->lineno = p->lineno;

		p1->from.type = D_CCR;
		p1->to.type = D_TOS;
		p1->as = AMOVW;
		p = p1;
		badccr = 1;
	}
	p1 = prg();
	p1->link = p->link;
	p->link = p1;
	p1->lineno = p->lineno;

	v = var + bn;
	p1->from.sym = v->sym;
	p1->from.type = v->type;
	p1->from.offset = v->offset;
	p1->from.etype = v->etype;
	p1->to.type = rn;
	if(f) {
		p1->to = p1->from;
		p1->from = zprog.from;
		p1->from.type = rn;
	}
	p1->as = opxt[OAS][v->etype];
	if(badccr) {
		p = p1;
		p1 = prg();
		p1->link = p->link;
		p->link = p1;
		p1->lineno = p->lineno;

		p1->from.type = D_TOS;
		p1->to.type = D_CCR;
		p1->as = AMOVW;
	}
	if(debug['R'])
		print("%P\t.a%P\n", p, p1);
}

Bits
mkvar(Adr *a, int as)
{
	Var *v;
	int i, t, z;
	long o;
	Bits bit;
	Sym *s;

	mvbits = 0;
	t = a->type & D_MASK;
	switch(t) {

	default:
		if(t >= D_R0 && t < D_R0+NREG) {
			regbits |= RtoB(t-D_R0);
			if(as == ADIVUL || as == ADIVSL)
				regbits |= RtoB(t-D_R0+1);
		}
		if(t >= D_A0 && t < D_A0+NREG)
			regbits |= AtoB(t-D_A0);
		if(t >= D_F0 && t < D_F0+NREG)
			regbits |= FtoB(t-D_F0);
		goto none;

	case D_EXTERN:
	case D_STATIC:
	case D_AUTO:
	case D_PARAM:
		break;
	}
	s = a->sym;
	if(s == S)
		goto none;

	if((a->type & I_MASK) == I_ADDR)
		mvbits |= B_ADDR;

	switch(a->index & I_MASK) {
	case I_INDEX1:
		mvbits |= B_ADDR;
		break;

	case I_INDEX2:
	case I_INDEX3:
		mvbits |= B_INDIR;
		break;
	}

	o = a->offset;
	v = var;
	for(i=0; i<nvar; i++) {
		if(s == v->sym)
		if(t == v->type)
		if(o == v->offset)
			goto out;
		v++;
	}
	if(s)
		if(s->name[0] == '.')
			goto none;
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
	v->etype = a->etype;
	v->type = t;
	if(debug['R'])
		print("bit=%2d et=%2d %s (%p,%d,%ld)\n",
			i, a->etype, s->name,
			v->sym, v->type, v->offset);

out:
	bit = blsh(i);
	if(t == D_EXTERN || t == D_STATIC)
		for(z=0; z<BITS; z++)
			externs.b[z] |= bit.b[z];
	if(t == D_PARAM)
		for(z=0; z<BITS; z++)
			params.b[z] |= bit.b[z];
	if(a->etype != v->etype || !typechlpfd[a->etype])
		for(z=0; z<BITS; z++)
			addrs.b[z] |= bit.b[z];	/* funny punning */
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
				changer++;
			}
			cal.b[z] |= r1->calahead.b[z];
			if(cal.b[z] != r1->calahead.b[z]) {
				r1->calahead.b[z] = cal.b[z];
				changer++;
			}
		}
		switch(r1->prog->as) {
		case ABSR:
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

		case ARTS:
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
				changer++;
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
	int i, j;

	v = var + r->varno;
	r->regno = D_NONE;
	switch(v->etype) {

	default:
		diag(Z, "unknown etype");
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
		i = BtoR(~b);
		j = BtoA(~b);
		if(r->costa == r->costr)
			if(i > j)
				i = NREG;
		if(j < NREG && r->costa > 0)
		if(r->costa > r->costr || i >= NREG) {
			r->regno = D_A0 + j;
			return AtoB(j);
		}
		if(i < NREG && r->costr > 0) {
			r->regno = D_R0 + i;
			return RtoB(i);
		}
		break;

	case TDOUBLE:
	case TFLOAT:
		i = BtoF(~b);
		if(i < NREG) {
			r->regno = D_F0 + i;
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
	int z;
	ulong bb;
	int x;

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
		changer -= CLOAD * r->loop;
		changea -= CLOAD * r->loop;
		if(debug['R'] && debug['v'])
			print("%ld%P\tld %B $%d.%d\n", r->loop,
				r->prog, blsh(bn), changer, changea);
	}
	for(;;) {
		r->act.b[z] |= bb;
		p = r->prog;

		if(r->use1.b[z] & bb) {
			changer += CREF * r->loop;
			changea += CREF * r->loop;
			x = p->from.index;
			if(x == D_NONE) {
				switch(p->as) {
				default:
					changea = -CINF;
				case AADDL:
				case ASUBL:
				case AMOVL:
				case ACMPL:
					break;
				}
			} else {
				changer += (CXREF-CREF) * r->loop;
				if(x != (I_INDEX3|D_NONE))
					changer = -CINF;
				if((x&I_MASK) == I_INDEX1)
					changea = -CINF;
			}
			if(p->as == AMOVL) {
				x = p->to.type;
				if(x >= D_R0 && x < D_R0+NREG)
					changer += r->loop;
				if(x >= D_A0 && x < D_A0+NREG)
					changea += r->loop;
			}
			if(debug['R'] && debug['v'])
				print("%ld%P\tu1 %B $%d.%d\n", r->loop,
					p, blsh(bn), changer, changea);
		}
		if((r->use2.b[z]|r->set.b[z]) & bb) {
			changer += CREF * r->loop;
			changea += CREF * r->loop;
			x = p->to.index;
			if(x == D_NONE)
				switch(p->as) {
				default:
					changea = -CINF;
					break;
				case AMOVL:
				case AADDL:
				case ACMPL:
				case ASUBL:
				case ACLRL:	/* can be faked */
				case ATSTL:	/* can be faked */
					break;
				}
			else {
				changer += (CXREF-CREF) * r->loop;
				if(x != (I_INDEX3|D_NONE))
					changer = -CINF;
				if((x&I_MASK) == I_INDEX1)
					changea = -CINF;
			}
			if(p->as == AMOVL) {
				x = p->from.type;
				if(x >= D_R0 && x < D_R0+NREG)
					changer += r->loop;
				if(x >= D_A0 && x < D_A0+NREG)
					changea += r->loop;
			}
			if(debug['R'] && debug['v'])
				print("%ld%P\tu2 %B $%d.%d\n", r->loop,
					p, blsh(bn), changer, changea);
		}
		if(STORE(r) & r->regdiff.b[z] & bb) {
			changer -= CLOAD * r->loop;
			changea -= CLOAD * r->loop;
			if(debug['R'] && debug['v'])
				print("%ld%P\tst %B $%d.%d\n", r->loop,
					p, blsh(bn), changer, changea);
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
paint3(Reg *r, int bn, ulong rb, int rn)
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
	int x;

	a->sym = 0;
	x = a->index;
	if(rn >= D_R0 && rn < D_R0+NREG)
		goto addr;
	if(x == (I_INDEX3|D_NONE)) {
		a->type = rn | I_INDIR;
		a->index = D_NONE;
		a->offset = a->displace;
		a->displace = 0;
		return;
	}
	if(x != D_NONE) {
		a->type = rn | I_INDIR;
		a->index += I_INDEX1 - I_INDEX2;
		a->offset = a->displace;
		a->displace = 0;
		return;
	}
	a->type = rn | (a->type & I_INDIR);
	return;

addr:
	if(x == (I_INDEX3|D_NONE)) {
		a->type = D_NONE|I_INDIR;
		a->index += I_INDEX1 + rn - D_NONE - I_INDEX3;
		a->scale = 4;	/* .L*1 */
		a->offset = a->displace;
		a->displace = 0;
		return;
	}
	a->type = rn | (a->type & I_INDIR);
}

/*
 *	bit	reg
 *	0-7	R0-R7
 *	8-15	A0-A7
 *	16-23	F0-F7
 */
ulong
RtoB(int r)
{

	if(r < 0 || r >= NREG)
		return 0;
	return 1L << (r + 0);
}

int
BtoR(ulong b)
{

	b &= 0x0000ffL;
	if(b == 0)
		return NREG;
	return bitno(b) - 0;
}

ulong
AtoB(int a)
{

	if(a < 0 || a >= NREG)
		return 0;
	return 1L << (a + NREG);
}

int
BtoA(ulong b)
{

	b &= 0x00ff00L;
	if(b == 0)
		return NREG;
	return bitno(b) - NREG;
}

ulong
FtoB(int f)
{

	if(f < 0 || f >= NREG)
		return 0;
	return 1L << (f + NREG+NREG);
}

int
BtoF(ulong b)
{

	b &= 0xff0000L;
	if(b == 0)
		return NREG;
	return bitno(b) - NREG-NREG;
}
