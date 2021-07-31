#include "gc.h"

int	emptyloop(Reg*, Reg*, Reg*);
/*
 * bra is a conditional branch which may be the start of a loop
 * try to change the loop into a do loop
 */
void
optloop(Reg *bra)
{
	Reg *entry, *cmp, *inc, *e, *tail, *loop;
	Prog *p;
	Adr *v, *t, a, b;
	int off, cc;

	e = bra->s2;
	if(!e || e->loop >= bra->loop)
		return;

	if(!onlybra(bra))
		return;
	entry = findentry(bra);
	cmp = findcmp(bra);
	if(!entry || !cmp)
		return;
	v = &cmp->prog->from;
	t = &cmp->prog->to;
	inc = indvarinc(bra, cmp, v);
	if(!inc || !loopinvar(bra, t))
		return;
	if(!notusedbra(bra, v))
		return;
	off = inc->prog->from.offset;
	if(off != -1 && off != 1)
		return;
	cc = bra->prog->cc;
	switch(cc){
	case CCEQ:
		break;
	case CCLT:
	case CCLE:
		if(off > 0)
			return;
		break;
	case CCGT:
		if(off < 1)
			return;
		cc = CCLT;
		break;
	case CCGE:
		if(off < 1)
			return;
		cc = CCLE;
		break;
	default:
		return;
	}
	if(!isfor(entry, cmp, t) || blocklen(bra->s1) > MAXLOOP || emptyloop(bra, inc, cmp))
		return;
	if(debug['P'])
		print("found inner loop: entry %P cmp %P inc %P exit %P\n",
			entry->prog, cmp->prog, inc->prog, bra->prog);

	/*
	 * have
	 *	top
	 * entry:
	 *	...
	 * 	CMP	v, t
	 *	BRA	CC,e
	 * want
	 *	top
	 * doend:
	 *	DOEND	,do		DOEND	,do
	 *	ADD	$-2048, v	MOVW	$3, REGTMP
	 *	BRA	GE, do		DBRA	v, do
	 *	JMP	e		JMP	e
	 * entry:
	 *	...
	 *	SUB	t, v	or SUBR t, v	if incrementing
	 *	BRA	CC, e	or BRA	CC', e	if incrementing
	 *	ADD	$-1, v	or nothing	if CC == CCLT
	 *				MOVW	v, REGTMP
	 *				AND	$3, REGTMP
	 *				SRA	$2, v
	 *				ADD	$-1, v
	 * do:
	 *	DO	v,doend		DO	R2,doend
	 */
	excise(inc);
	tail = entry->p1;
	adddo(ADOLOCK, bra, tail);
	loop = bra->s1;
	tail = tail->s1;
	mkreg(&b, REGTMP);
	loop->prog->from = b;
	tail = addinst(tail, AMOVW, mkconst(&a, 3), NREG, &b);
	tail = adddbra(tail, v, loop);
	addjmp(tail, AJMP, 0, e);

	bra->loop -= LOOP;
	bra->prog->cc = cc;
	if(cc != CCLT)
		bra = addinst(bra, AADD, mkconst(&a, -1), v->reg, v);
	bra = addinst(bra, AMOVW, v, NREG, mkreg(&b, REGTMP));
	bra = addinst(bra, AAND, mkconst(&a, 3), NREG, &b);
	bra = addinst(bra, ASRA, mkconst(&a, 2), NREG, v);
	addinst(bra, AADD, mkconst(&a, -1), v->reg, v);
	if(regzer(t))
		return;
	p = cmp->prog;
	a = *t;
	p->to = *v;
	p->from = a;
	p->as = ASUB;
	if(off > 0)
		p->as = ASUBR;
}

int
onlybra(Reg *r0)
{
	Reg *r;

	for(r = r0->s1; r; r = uniqs(r)){
		switch(r->prog->as){
		case ACALL:
		case ARETURN:
		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
		case AFDIV:
			return 0;
		}
		if(r == r0)
			return 1;
	}
	return 0;
}

/*
 * r0 is the only exit from the loop
 * find the only entry
 */
Reg *
findentry(Reg *r0)
{
	Reg *entry, *r;

	entry = 0;
	for(r = r0->s1; r; r = uniqs(r)){
		if(!uniqp(r)){
			if(entry)
				return 0;
			entry = r;
		}
		if(r == r0)
			return entry;
	}
	return 0;
}

/*
 * r is a branch
 * find the comparison to go along with it
 */
Reg *
findcmp(Reg *r)
{
	for(r = uniqp(r); r; r = uniqp(r)){
		if(!uniqs(r))
			return 0;
		if(r->prog->as == ACMP)
			return r;
		if(r->prog->as != ANOP)
			return 0;
	}
	return 0;
}

/*
 * is the loop empty?
 */
int
emptyloop(Reg *r0, Reg *inc, Reg *cmp)
{
	Reg *r;

	for(r = r0->s1; r && r != r0; r = uniqs(r)){
		if(r == inc || r == cmp)
			continue;
		if(r->prog->as != ANOP && r->prog->as != AJMP){
			return 0;
		}
	}
	return 1;
}

/*
 * is the loop a for loop?
 */
int
isfor(Reg *entry, Reg *cmp, Adr *v)
{
	Reg *r;
	Prog *p;
	int cmped;

	r = entry->p1;
	if(!r || r->loop != entry->loop || r->s2)
		return 0;
	r = entry->p2;
	if(entry->s2 || !r || r->prog->as != AJMP || r->p2link)
		return 0;
	cmped = 0;
	for(r = entry; r; r = uniqs(r)){
		p = r->prog;
		switch(p->as){
		case ABRA:
			return cmped;
		case ACMP:
			if(r != cmp)
				return 0;
			cmped = 1;
			break;
		case ANOP:
			break;
		case AMOVW:
			if(p->to.type != D_REG || p->to.reg != v->reg)
				return 0;
			break;
		default:
			return 0;
		}
	}
	return 0;
}

/*
 * return the increment if a is a simple induction variable
 * for the loop with exit r
 */
Reg *
indvarinc(Reg *r, Reg *cmp, Adr *a)
{
	Reg *inc;
	Prog *p;
	int u;

	if(a->type != D_REG)
		return 0;
	inc = 0;
	for(r = r->s1; r; r = uniqs(r)){
		p = r->prog;
		u = copyu(p, a, 0, 0);
		switch(u){
		case 0:
			break;
		case 1:
			if(r != cmp || !inc)
				return 0;
			break;
		case 2:
		case 3:
			return 0;
		case 4:
			if(inc || p->as != AADD || !contyp(&p->from))
				return 0;
			inc = r;
			break;
		}
	}
	return inc;
}

/*
 * does v have the same value for every iteration of the loop
 * r is the exit from the loop
 */
int
loopinvar(Reg *r, Adr *v)
{
	int u, entered;

	if(v->type == D_CONST)
		return 1;
	if(v->type != D_REG)
		return 0;
	entered = 0;
	for(r = r->s1; r; r = uniqs(r)){
		if(!uniqp(r))
			entered = 1;
		p = r->prog;
		u = copyu(p, v, 0, 0);
		switch(u){
		case 0:
			break;
		case 1:
			if(p->as != ACMP && p->as != AMOVW)
				return 0;
			break;
		case 2:
		case 4:
			return 0;
		case 3:
			if(!entered || p->as != AMOVW
			|| p->from.type != D_NAME && p->from.type != D_CONST
			|| !unaliased(&p->from))
				return 0;
			break;
		}
	}
	return 1;
}
