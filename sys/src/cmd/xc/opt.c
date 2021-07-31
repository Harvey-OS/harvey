#include "gc.h"

void
mergemuladd(Reg *r)
{
	Reg *r1;
	Prog *p, *p1;
	Adr a;
	int f, f1, n;

	p = r->prog;
	if(p->to.type != D_NONE)
		return;
	mkfreg(&a, p->reg);
	r1 = findused(r, &a);
	if(!r1 || !notused(r1, &a))
		return;
	p1 = r1->prog;
	if(p1->as != AFADD && p1->as != AFSUB)
		return;

	/*
	 * check operands of add
	 */
	f = 0;
	f1 = 0;
	if(p1->from.type == D_FREG && p1->from.reg == p->reg)
		f = 1;
	if(p1->from1.type == D_FREG && p1->from1.reg == p->reg)
		f1 = 1;
	if(f && f1 || !f && !f1 || p1->as == AFSUB && f1)
		return;

	/*
	 * check number of memory references
	 */
	n = 0;
	if(p->from.type != D_FREG)
		n++;
	if(p->from1.type != D_FREG)
		n++;
	if(p1->from.type != D_FREG)
		n++;
	if(p1->from1.type != D_FREG)
		n++;
	if(n > 2)
		return;

	/*
	 * check setting of operands
	 */
	if(setbetween(r, r1, &p->from) || setbetween(r, r1, &p->from1))
		return;

	if(p1->as == AFADD)
		p1->as = AFMADD;
	else
		p1->as = AFMSUB;
	if(f)
		p1->from2 = p1->from1;
	else
		p1->from2 = p1->from;
	p1->from1 = p->from1;
	p1->from = p->from;
	excise(r);
}

/*
 * merge floating stores with preceding calculations
 */
void
mergestore(Reg *r)
{
	Reg *r1;
	Prog *p, *p1;
	Adr a;

	p = r->prog;
	if(p->to.type == D_FREG)
		a = p->to;
	else if(p->to.type == D_NONE)
		mkfreg(&a, p->reg);
	else
		return;
	r1 = r;
	for(;;){
		r1 = findused(r1, &a);
		if(!r1)
			return;
		p1 = r1->prog;
		if(p1->as == AFMOVF)
			break;
		if(copyu(p1, &a, A, 0) != 1)
			return;
	}

	/*
	 * check that this is a store of the result of the op
	 */
	if(p1->from.type != D_FREG || p1->from.reg != a.reg
	|| p1->reg != NREG && p1->reg != a.reg
	|| !indreg(&p1->to))
		return;

	/*
	 * check to see if we can move the store to the op
	 */
	a = p1->to;
	a.type = D_REG;
	if(!setbetween(r, r1, &a)){
		if(p->to.type == D_FREG)
			p->reg = p->to.reg;
		p->to = p1->to;
		excise(r1);
		return;
	}

	/*
	 * check to see if we can move the op to the store
	 */
	if(setbetween(r, r1, &p->from)
	|| setbetween(r, r1, &p->from1)
	|| setbetween(r, r1, &p->from2))
		return;
	if(p->to.type == D_FREG)
		p->reg = p->to.reg;
	p1->as = p->as;
	p1->from = p->from;
	p1->from1 = p->from1;
	p1->from2 = p->from2;
	p1->reg = p->reg;
	excise(r);
}

void
mergeinc(Reg *r, Adr *a, int inc)
{
	Reg *r1;
	Prog *p;
	Adr a1;

	if(a->type != D_INDREG)
		return;
	a1 = *a;
	a1.type = D_REG;
	r1 = findused(r, &a1);
	if(!r1)
		return;
	p = r1->prog;
	if(p->as != AADD || !contyp(&p->from))
		return;
	if(p->from.offset < 0)
		inc = -inc;
	if(p->from.offset != inc)
		return;

	/*
	 * try to convert ADD $c, r1, r2 into
	 * r1+; MOVW r1,r2 and then propagate the move away
	 */
	if(p->reg == NREG || p->reg == p->to.reg)
		excise(r1);
	else{
		if(p->reg != a->reg)
			return;
		if(!notused(r1, &a1))
			return;
		p->as = AMOVW;
		p->from = a1;
		p->reg = NREG;
		if(copyprop(r1))
			excise(r1);
	}
	a->type = D_INC;
	if(inc < 0)
		a->type = D_DEC;
}

/*
 * move r past as many memory references as possible
 */
int
forward(Reg *r0)
{
	Prog *p;
	Reg *r, *r1;
	Adr *a;
	Adr a1;

	a = &r0->prog->to;
	mkreg(&a1, r0->prog->reg);
	if(a1.reg == NREG)
		a1.reg = a->reg;
	r = r1 = r0;
	for(r = uniqs(r); r; r = uniqs(r)){
		if(!uniqp(r))
			break;
		p = r->prog;
		if(copyu(p, a, A, 0) || copyu(p, &a1, A, 0))
			break;
		if(memref(&p->from) || memref(&p->to)
		|| p->isf && (memref(&p->from1) || memref(&p->from2)))
			r1 = r;
	}
	if(!r || r1 == r0)
		return 0;
	if(r1->s2)
		diag(Z, "forward fail\n");

	ALLOC(p,Prog);
	*p = *r0->prog;
	p->link = r1->prog->link;
	r1->prog->link = p;
	excise(r0);
	newreg(r1);

	return 1;
}

/*
 * we have add $c, rx, ry
 * try to make add $c, ry, ry
 * look backwards for add $d, ry, rz
 * try to rewrite as add $d, ry, ry; ...; add $(c-d), ry, ry and then repeat
 */
void
addprop(Reg *r)
{
	Reg *r1;
	Prog *p, *p1;
	Adr a;

	p = r->prog;
	if(p->reg == NREG)
		p->reg = p->to.reg;
	if(p->reg == REGZERO)
		return;
	if(debug['P'])
		print("addprop: %P\n", p);
	if(p->reg != p->to.reg){
		if(debug['P'])
			print("\ttrying to rewrite\n");
		if(!notused(r, mkreg(&a, p->reg)))
			return;
		if(debug['P'])
			print("\t.\n");
		r1 = findused(r, &p->to);
		if(!r1)
			return;
		p1 = r1->prog;
		if(debug['P'])
			print("\tused %P\n", p1);
		if(!notused(r1, &p->to))
			return;
		if(setbetween(r, r1, &a))
			return;
		if(debug['P'])
			print("\t.\n");
		if(copyu(p1, &p->to, &a, 0))
			return;
		copyu(p1, &p->to, &a, 1);
		p->to.reg = p->reg;
		if(debug['P'])
			print("\tgot it %P; %P\n", p, p1);
	}
	for(;;){
		r1 = findusedb(r, &p->to);
		if(!r1)
			return;
		p1 = r1->prog;
		if(debug['P'])
			print("check %P\n", p1);
		if(p1->as != AADD || !contyp(&p1->from)
		|| p1->reg != p->reg || p1->to.reg == p->reg)
			return;
		if(debug['P'])
			print("\t.\n");
		if(!subreg(r1, r, &p1->to, &p->to))
			return;
		p->from.offset -= p1->from.offset;
		if(debug['P'])
			print("got it: %P %P\n", p, p1);
		r = r1;
		p = p1;
	}
}

/*
 * replace rf with rt between r0 ... r1
 */
int
subreg(Reg *r0, Reg *r1, Adr *rf, Adr *rt)
{
	Reg *r;
	int u;

	if(r0 == r1)
		return 0;
	r = r0;
	u = 1;
	for(r=uniqs(r); r!=r1; r=uniqs(r)){
		/*
		 * check if we can do the copy
		 * also check set/used/rar
		 */
		if(copyu(r->prog, rf, rt, 0))
			return 0;
		u = copyu(r->prog, rf, A, 0);
		if(u == 3 || u == 4){
			r1 = r;
			break;
		}
		if(u == 2)
			return 0;
	}
	if(u != 3 && u != 4 && !notused(r1, rf))
		return 0;
	r = r0;
	for(r=uniqs(r); r!=r1; r=uniqs(r))
		copyu(r->prog, rf, rt, 1);
	copyu(r1->prog, rf, rt, 1);
	rf->reg = rt->reg;
	return 1;
}

void
killrmove(Reg *r)
{
	Reg *r1;
	Prog *p, *p1;

	p = r->prog;
	r1 = findused(r, &p->to);
	if(!r1)
		return;
	p1 = r1->prog;
	if(p1->as != p->as)
		return;
	if(p1->from.type != D_REG || p1->from.reg != p->to.reg)
		return;
	if(p1->to.type != D_REG || p1->to.reg != p->to.reg)
		return;
	excise(r1);
}

void
killadj(Reg *r0)
{
	Prog *p;
	Reg *r;

	for(r = uniqs(r0); r; r = uniqs(r)){
		if(!uniqp(r))
			return;
		p = r->prog;
		if(p->as == AADD && isadj(p)){
			p->from.offset += r0->prog->from.offset;
			excise(r0);
			return;
		}
		if(p->as == ARETURN){
			excise(r0);
			return;
		}
	}
}

/*
 * chase backward all cc setting.
 * returns 1 if all set same.
 */
int
findtst(Reg *r0, Prog *rp, int n, int zcmp)
{
	Reg *r;
	int c;

loop:
	n++;
	if(n >= 10)
		return 0;
	for(r=r0->p2; r!=R; r=r->p2link) {
		c = setcc(r->prog, rp, zcmp);
		if(c > 0)
			continue;
		if(c == 0)
			return 0;
		if(findtst(r, rp, n, zcmp) == 0)
			return 0;
	}
	r = r0->p1;
	if(r == R)
		return 1;
	c = setcc(r->prog, rp, zcmp);
	if(c > 0)
		return 1;
	if(c == 0)
		return 0;
	r0 = r;
	goto loop;
}

/*
 * return -1 if op can't set cc
 * return 1 if sets same as tst
 * return 0 otherwise
 */
int
setcc(Prog *p, Prog *tst, int zcmp)
{
	Adr a;

	switch(p->as){
	default:
		if(1 || debug['P'])
			print("unknown setcc %A\n", p->as);
		return 0;

	case ACALL:
	case ABIT:
	case AFDIV:
	case ATEXT:
		return 0;

	case ABRA:
	case AJMP:
	case ANOP:
		return -1;

	case AFADD:
	case AFMUL:
	case AFSUB:
	case AFMADD:
	case AFMSUB:
		if(copyau(&p->from, &tst->from) == 2
		|| copyau(&p->from1, &tst->from) == 2
		|| copyau(&p->from2, &tst->from) == 2
		|| copyau(&p->to, &tst->from) == 2)
			return 0;
		if(zcmp)
			return -1;
		if(copyau(&p->from, &tst->to) == 2
		|| copyau(&p->from1, &tst->to) == 2
		|| copyau(&p->from2, &tst->to) == 2
		|| copyau(&p->to, &tst->to) == 2)
			return 0;
		return -1;

	case AFMOVFH:
	case AFMOVFW:
	case AFMOVHF:
	case AFMOVWF:
	case AFMOVF:
	case AFMOVFN:
		if(p->from.type == D_NAME
		|| p->from.type == D_FCONST
		|| p->to.type == D_NAME)
			return 0;
		if(copyau(&p->from, &tst->from) == 2
		|| copyau(&p->to, &tst->from) == 2)
			return 0;
		if(zcmp)
			return -1;
		if(copyau(&p->from, &tst->to) == 2
		|| copyau(&p->to, &tst->to) == 2)
			return 0;
		return -1;

	case ACMP:
		if(!compat(&p->from, &tst->from))
			return 0;
		return compat(&p->to, &tst->to);

	case ASUB:
		if(zcmp && tst->from.reg == p->to.reg)
			return 1;
		if(!compat(&p->from, &tst->to))
			return 0;
		if(p->reg == NREG)
			return compat(&p->to, &tst->from);
		return compat(mkreg(&a, p->reg), &tst->from);

	/*
	 * these all end up doing an add of the result
	 */
	case AADD:
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case AMUL:

	/*
	 * set cc same as cmp x, $0
	 */
	case AAND:
	case AXOR:
	case AOR:
	case ASRA:
	case ASRL:
	case ASLL:
		if(zcmp && tst->from.reg == p->to.reg)
			return 1;
		return 0;

	case AMOVW:
	case AMOVB:
	case AMOVH:
	case AMOVBU:
	case AMOVHU:
		if(!zcmp || memref(&p->from))
			return 0;
		if(p->to.type != D_REG || p->to.reg != tst->from.reg)
			return 0;
		if(p->from.type == D_REG)
			return 1;
		if(consetscc(&p->from))
			return 1;
		return 0;
	}

	return 0;
}

int
compat(Adr *a, Adr *b)
{
	if(regzer(a) && regzer(b))
		return 1;
	if(a->type != b->type)
		return 0;
	if(a->type == D_CONST)
		return a->offset == b->offset;
	if(a->type == D_REG)
		return a->reg == b->reg;
	return 0;
}

int
consetscc(Adr *a)
{
	long v;

	if(a->type != D_CONST)
		return 0;
	if(a->sym || a->name != D_NONE)
		return 0;
	v = a->offset;
	if(v >= -32768 && v < 32768)
		return 1;
	return 0;
}
