#include "gc.h"

void
peep(void)
{
	Reg *r, *r1;
	Prog *p;
	int fr, f1r, tr;

	/*
	 * massage instructions to make peep easier
	 */
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		if(p->as == AFMOVF || p->as == AFMOVFN){
			if(p->to.type == D_NONE){
				p->to.type = D_FREG;
				p->to.reg = p->reg;
				p->reg = NREG;
			}
			if(p->from.type == D_NAME)
				addmovea(r, &p->from);
			else if(p->to.type == D_NAME)
				addmovea(r, &p->to);
		}
		if(p->as == AMOVW && regzer(&p->from)){
			p->from.type = D_REG;
			p->from.reg = 0;
		}
		if(p->as == ASUB && contyp(&p->from)){
			p->as = AADD;
			p->from.offset = -p->from.offset;
		}
		a2type(p);
	}

	while(propcopies())
		;

	/*
	 * remove extra stack adjustments
	 * rewrite adds to make (A)+ easier
	 */
	for(r = firstr; r; r = r->link){
		p = r->prog;
		if(p->as == AADD && contyp(&p->from)){
			addprop(r);
			if(0 && isadj(p))
				killadj(r);
		}
	}

	/*
	 * try to push adds forward so
	 * we'll have a better chance of getting (A)+ later
	 */
	for(r = firstr; r; r = r->link){
		p = r->prog;
		if(p->as == AADD && contyp(&p->from))
			forward(r);
	}

	while(propcopies())
		;

	/*
	 * loops
	 */
	if(!debug['l']){
		for(r = firstr; r; r = r1){
			r1 = r->link;
			p = r->prog;
			if(p->as == ABRA)
				optloop(r);
		}
	}

	/*
	 * look for MOVB x,R; MOVB R,R
	 * convert (A) ... A++ into (A)++
	 */
	for(r = firstr; r; r = r->link){
		p = r->prog;
		switch(p->as){
		case AMOVW:
			if(p->from.reg != p->to.reg){
				mergeinc(r, &p->from, 4);
				mergeinc(r, &p->to, 4);
			}
			break;
		case AMOVB:
		case AMOVBU:
			if(p->to.type == D_REG)
				killrmove(r);
			if(p->from.reg != p->to.reg){
				mergeinc(r, &p->from, 1);
				mergeinc(r, &p->to, 1);
			}
			break;
		case AMOVH:
		case AMOVHU:
			if(p->to.type == D_REG)
				killrmove(r);
			if(p->from.reg != p->to.reg){
				mergeinc(r, &p->from, 2);
				mergeinc(r, &p->to, 2);
			}
			break;
		case AFMOVFH:
			mergeinc(r, &p->to, 2);
			if(indreg(&p->to) != indreg(&p->from))
				mergeinc(r, &p->from, 4);
			break;
		case AFMOVHF:
			mergeinc(r, &p->to, 4);
			if(indreg(&p->to) != indreg(&p->from))
				mergeinc(r, &p->from, 2);
			break;
		case AFADD:
		case AFSUB:
		case AFMUL:
		case AFMOVF:
		case AFMOVFW:
		case AFMOVWF:
		case AFMOVFN:
			tr = indreg(&p->to);
			fr = indreg(&p->from);
			f1r = indreg(&p->from1);
			mergeinc(r, &p->to, 4);
			if(f1r != tr)
				mergeinc(r, &p->from1, 4);
			if(fr != tr && fr != f1r)
				mergeinc(r, &p->from, 4);
			break;
		}
	}

	/*
	 * merge FMUL ... FADD and FMUL .. FSUB
	 */
	for(r = firstr; r; r = r1){
		r1 = r->link;
		if(r->prog->as == AFMUL)
			mergemuladd(r);
	}

	/*
	 * merge FADD x,y,f FMOVF f,f,mem
	 */
	for(r = firstr; r; r = r->link){
		p = r->prog;
		switch(p->as){
		case AFMOVF:
		case AFMOVFN:
			if(p->to.type != D_FREG)
				continue;
			mergestore(r);
			break;
		case AFADD:
		case AFSUB:
		case AFMUL:
		case AFMADD:
		case AFMSUB:
			if(p->to.type != D_NONE)
				continue;
			mergestore(r);
			break;
		}
	}

	/*
	 * kill extraneous tests
	 */
	for(r = firstr; r; r = r->link){
		p = r->prog;
		if(p->as == ACMP && findtst(r, p, 0, regzer(&p->to)))
			excise(r);
	}

	/*
	 * last pass:
	 * insert dolocks in basic blocks
	 */
	if(debug['l'])
		return;
	for(r = firstr; r; r = r->link)
		r->active = 0;
	insertlock(firstr);

	/*
	 * ideas:
	 * need to check no memory refs between instructions when merging 
	 *
	 * need to do a better job of putting float pointers
	 * in registers: addrs, floating consts really worth it.
	 * possibly reserve a register?
	 * also could try to merge adject address calculations
	 *
	 * looks like i could use a different algorithm
	 * to load constant addresses, or remove them when common.
	 *
	 * try moving addresses calculations backwards so can merge
	 * stores with calculations better
	 * might need to relabel the pointer register to avoid conflicts
	 */
}

/*
 * check if a is modified after r and before r1
 */
int
setbetween(Reg *r, Reg *r1, Adr *a)
{
	Adr a1;
	int n;

	if(indreg(a)){
		a1 = *a;
		a1.type = D_REG;
		a = &a1;
	}
	for(r = uniqs(r); r && r != r1; r = uniqs(r)){
		n = copyu(r->prog, a, A, 0);
		if(n != 0 && n != 1)
			return 1;
	}
	if(!r){
		diag(Z, "setbetween: end not uniq\n");
		return 1;
	}
	return 0;
}

Reg*
findused(Reg *r, Adr *a)
{
	for(r = uniqs(r); r; r = uniqs(r)){
		if(!uniqp(r))
			return R;
		if(copyu(r->prog, a, A, 0))
			return r;
	}
	return R;
}

Reg*
findusedb(Reg *r, Adr *a)
{
	for(r = uniqp(r); r; r = uniqp(r)){
		if(!uniqs(r))
			return R;
		if(copyu(r->prog, a, A, 0))
			return r;
	}
	return R;
}

/*
 * check if the value in register a is dead after this instruction
 * don't mark this instruction as check, so we'll pick it up on a loop
 */
int
notused(Reg *r0, Adr *a)
{
	Reg *r;
	int n;

	n = copyu(r0->prog, a, A, 0);
	if(n == 2)
		return 0;
	if(n == 3 || n == 4)
		return 1;
	for(r = firstr; r; r = r->link)
		r->active = 0;
	if(r0->s1 && !notused1(r0->s1, a))
		return 0;
	if(r0->s2 && !notused1(r0->s2, a))
		return 0;
	return 1;
}

/*
 * same as notused, but check if dead when we take the branch
 */
int
notusedbra(Reg *r0, Adr *a)
{
	Reg *r;

	if(!r0->s2)
		return 0;
	for(r = firstr; r; r = r->link)
		r->active = 0;
	return notused1(r0->s2, a);
}

int
notused1(Reg *r, Adr *a)
{
	int t;

	for(; r; r = r->s1){
		if(r->active)
			return 1;
		r->active = 1;
		t = copyu(r->prog, a, A, 0);
		if(t == 3)
			return 1;
		if(t)
			return 0;
		if(r->s2 && !notused1(r->s2, a))
			return 0;
	}
	return 1;
}

int
blocklen(Reg *r)
{
	int i;

	i = 0;
	for(; r; r = uniqs(r)){
		if(!uniqp(r))
			break;
		if(r->prog->as != ANOP)
			i++;
	}
	return i;
}

/*
 * walk all basic blocks rooted at r0, insterting locks if large enough
 */
Reg*
insertlock(Reg *r0)
{
	Reg *r, *r1, *next;
	int n;

	n = 0;
	r1 = r0;
	for(r = r0; r; r = next){
		if(r->active){
			if(n >= MINBLOCK)
				addlock(r0, r1);
			return 0;
		}
		next = uniqs(r);
		r->active = 1;
		if(r->loop < LOOP)
			;
		switch(r->prog->as){
		case ATEXT:
		case ARETURN:
		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
		case AFDIV:
		case AEND:
			if(n >= MINBLOCK)
				addlock(r0, r1);
			if(r->s2)
				insertlock(r->s2);
			n = 0;
			r0 = next = r->s1;
			break;
		case ACALL:
		case ABRA:
		case ADBRA:
			if(uniqp(r)){
				r1 = r;
				n++;
			}
			if(n >= MINBLOCK)
				addlock(r0, r1);
			if(r->s2)
				insertlock(r->s2);
			n = 0;
			r0 = next = r->s1;
			break;
		case ADO:
		case ADOLOCK:
			if(n >= MINBLOCK)
				addlock(r0, r1);
			n = 0;
			r0 = next = r->s2->s1;
			for(r = r->s1; r; r = uniqs(r))
				r->active = 1;
			break;
		case ADOEND:
			print("doend without seeing do\n");
			break;
		case ANOP:
		case AJMP:
			if(!uniqp(r)){
				if(n >= MINBLOCK)
					addlock(r0, r1);
				n = 0;
				r0 = next;
			}
			if(r0 == r)
				r0 = next;
			break;
		default:
			if(!uniqp(r)){
				if(n >= MINBLOCK)
					addlock(r0, r1);
				n = 0;
				r0 = r;
			}
			n++;
			if(n >= MAXBLOCK){
				addlock(r0, r1);
				r0 = next;
				n = 0;
			}
			r1 = r;
			break;
		}
	}
	return 0;
}

void
excise(Reg *r)
{
	Prog *p;

	p = r->prog;
	p->as = ANOP;
	p->isf = 0;
	p->from = zprog.from;
	p->to = zprog.to;
	p->reg = zprog.reg;
}

Reg*
uniqp(Reg *r)
{
	Reg *r1;

	r1 = r->p1;
	if(r1 == R) {
		r1 = r->p2;
		if(r1 == R || r1->p2link != R)
			return R;
	} else
		if(r->p2 != R)
			return R;
	return r1;
}

Reg*
uniqs(Reg *r)
{
	Reg *r1;

	r1 = r->s1;
	if(r1 == R)
		r1 = r->s2;
	else if(r->s2 != R)
		return R;
	return r1;
}

/*
 * is this add instruction a stack adjustment?
 */
int
isadj(Prog *p)
{
	if(p->reg != NREG && p->reg != REGSP)
		return 0;
	if(p->to.type != D_REG || p->to.reg != REGSP)
		return 0;
	if(p->from.offset < 0)
		return 0;
	return 1;
}

int
unaliased(Adr *a)
{
	Var *v;
	int i, t, n, z;
	long o;
	Bits bit;
	Sym *s;

	t = a->type;
	if(t == D_CONST)
		return 1;
	if(t != D_NAME)
		return 0;
	s = a->sym;
	o = a->offset;
	if(s == S || s->name[0] == '.')
		return 0;
	n = a->name;
	v = var;
	for(i=0; i<nvar; i++) {
		if(s == v->sym)
		if(n == v->name)
		if(o == v->offset){
			bit = blsh(i);
			for(z=0; z<BITS; z++)
				if(bit.b[z] & addrs.b[z])
					return 0;
			return 1;
		}
		v++;
	}
	return 0;
}

int
regzer(Adr *a)
{
	if(a->type == D_CONST && a->sym == S && a->offset == 0)
		return 1;
	if(a->type == D_REG && a->reg == 0)
		return 1;
	return 0;
}

int
regtyp(Adr *a)
{
	if(a->type == D_REG && a->reg != 0)
		return 1;
	if(a->type == D_FREG)
		return 1;
	return 0;
}

int
contyp(Adr *a)
{
	if(a->type == D_CONST && a->sym == S)
		return 1;
	return 0;
}

/*
 * return if references memory
 */
int
memref(Adr *a)
{
	switch(a->type){
	case D_INDREG:
	case D_INC:
	case D_DEC:
	case D_INCREG:
	case D_NAME:
		return 1;
	}
	return 0;
}

int
indreg(Adr *a)
{
	switch(a->type){
	case D_INDREG:
	case D_INC:
	case D_DEC:
	case D_INCREG:
		return a->reg;
	}
	return 0;
}

Adr*
mkfreg(Adr *a, int r)
{
	a->name = D_NONE;
	a->sym = 0;
	a->type = D_FREG;
	a->reg = r;
	return a;
}

Adr*
mkreg(Adr *a, int r)
{
	a->name = D_NONE;
	a->sym = 0;
	a->type = D_REG;
	a->reg = r;
	return a;
}

Adr*
mkconst(Adr *a, long off)
{
	a->name = D_NONE;
	a->sym = 0;
	a->reg = NREG;
	a->type = D_CONST;
	a->offset = off;
	return a;
}

/*
 * r is not a jump; add one after it with destination to
 */
Reg*
addjmp(Reg *r, int as, int cc, Reg *to)
{
	Prog *p;

	r = addinst(r, as, 0, NREG, 0);
	p = r->prog;
	p->to.type = D_BRANCH;
	p->cc = cc;
	r->s2 = to;
	r->p2link = to->p2;
	to->p2 = r->p2link;
	if(as == AJMP && r->s1){
		r->s1->p1 = 0;
		r->s1 = 0;
	}
	return r;
}

Reg*
adddbra(Reg *r, Adr *from, Reg *to)
{
	Prog *p;

	r = addinst(r, ADBRA, 0, NREG, 0);
	p = r->prog;
	p->to.type = D_BRANCH;
	if(from)
		p->from = *from;
	r->s2 = to;
	r->p2link = to->p2;
	to->p2 = r->p2link;
	return r;
}

/*
 * add a do instruction;
 * s is the beginning of the loop, e the end
 */
void
adddo(int as, Reg *s, Reg *e)
{
	Reg *d, *de;

	d = addinst(s, as, 0, NREG, 0);
	de = addinst(e, ADOEND, 0, NREG, 0);
	d->prog->to.type = D_BRANCH;
	de->prog->to.type = D_BRANCH;

	d->s2 = de;
	d->p2 = de;
	de->s2 = d;
	de->p2 = d;
}

/*
 * lock the basic block [s, e]
 */
void
addlock(Reg *s, Reg *e)
{
	Prog *sp, *ep;
	Reg *d, *de;

	d = insertinst(s, ADOLOCK, 0, NREG, 0);
	de = addinst(e, ADOEND, 0, NREG, 0);
	d->link->active = de->active = 1;
	sp = d->prog;
	ep = de->prog;
	sp->to.type = D_BRANCH;
	mkconst(&sp->from, 0);
	ep->to.type = D_BRANCH;

	d->s2 = de;
	d->p2 = de;
	de->s2 = d;
	de->p2 = d;
}

/*
 * add a inst after r
 */
Reg*
addinst(Reg *r, int as, Adr *from, int reg, Adr *to)
{
	Prog *p, *p1;

	ALLOC(p1,Prog);
	p = r->prog;
	*p1 = zprog;
	p1->link = p->link;
	p1->lineno = p->lineno;
	p->link = p1;

	p1->as = as;
	if(from)
		p1->from = *from;
	p1->reg = reg;
	if(to)
		p1->to = *to;

	newreg(r);
	return r->s1;
}

/*
 * insert an instruction before r
 * r is not a jump or a branch
 */
Reg*
insertinst(Reg *r, int as, Adr *from, int reg, Adr *to)
{
	Prog *p, *p1;

	ALLOC(p1,Prog);
	p = r->prog;
	*p1 = *p;
	*p = zprog;
	p->link = p1;
	p->lineno = p1->lineno;

	p->as = as;
	if(from)
		p->from = *from;
	p->reg = reg;
	if(to)
		p->to = *to;

	newreg(r);
	return r;
}

void
addmovea(Reg *r, Adr *a1)
{
	Adr a, b;

	a = *a1;
	addref(a1, 14);

	a.type = D_CONST;
	insertinst(r, AMOVW, &a, NREG, mkreg(&b, 14));
}

int
a2type(Prog *p)
{
	switch(p->as) {
	case AADD:
	case AADDCR:
	case AADDCRH:
	case AADDH:
	case AAND:
	case AANDH:
	case AANDN:
	case AANDNH:
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case AMUL:
	case AOR:
	case AORH:
	case AROL:
	case AROLH:
	case AROR:
	case ARORH:
	case ASLL:
	case ASLLH:
	case ASRA:
	case ASRAH:
	case ASRL:
	case ASRLH:
	case ASUB:
	case ASUBH:
	case ASUBR:
	case ASUBRH:
	case AXOR:
	case AXORH:
		return D_REG;

	case AFADD:
	case AFADDN:
	case AFADDT:
	case AFADDTN:
	case AFDIV:
	case AFDSP:
	case AFIFEQ:
	case AFIFGT:
	case AFIEEE:
	case AFIFLT:
	case AFMOVF:
	case AFMOVFN:
	case AFMOVFB:
	case AFMOVFW:
	case AFMOVFH:
	case AFMOVBF:
	case AFMOVWF:
	case AFMOVHF:
	case AFMADD:
	case AFMADDN:
	case AFMADDT:
	case AFMADDTN:
	case AFMSUB:
	case AFMSUBN:
	case AFMSUBT:
	case AFMSUBTN:
	case AFMUL:
	case AFMULN:
	case AFMULT:
	case AFMULTN:
	case AFRND:
	case AFSEED:
	case AFSUB:
	case AFSUBN:
	case AFSUBT:
	case AFSUBTN:
		if(!p->isf)
			print("botch: !isf in atype\n");
		return D_FREG;
	}
	return D_NONE;
}

/*
 * direct reference,
 * could be set/use depending on
 * semantics
 */
int
copyas(Adr *a, Adr *v)
{

	if(regtyp(v)
	&& a->type == v->type
	&& a->reg == v->reg)
		return 1;
	return 0;
}

/*
 * either direct or indirect
 * return 2 if modifies reg
 */
int
copyau(Adr *a, Adr *v)
{
	if(copyas(a, v))
		return 1;
	if(v->type != D_REG)
		return 0;
	switch(a->type){
	case D_INDREG:
	case D_OREG:
		if(v->reg == a->reg)
			return 1;
		break;
	case D_INC:
	case D_DEC:
		if(v->reg == a->reg)
			return 2;
		break;
	}
	return 0;
}

int
copyau1(Prog *p, Adr *v)
{

	if(!regtyp(v))
		return 0;
	if(p->isf){
		if(v->type == D_FREG && p->reg == v->reg)
			return 1;
		return 0;
	}
	if((p->from.type == v->type || p->to.type == v->type)
	&& p->reg == v->reg) {
		if(a2type(p) != v->type)
			print("botch a2type %P\n", p);
		return 1;
	}
	return 0;
}

/*
 * substitute s for v in a
 * return failure to substitute
 */
int
copysub(Adr *a, Adr *v, Adr *s, int f)
{
	if(f && copyau(a, v))
		a->reg = s->reg;
	return 0;
}

int
copysubf(Adr *a, Adr *v, Adr *s, int f)
{
	int used;

	used = copyau(a, v);
	if(s->type == D_REG && !fpreg(s->reg) && used)
		return 1;
	if(f && used)
		a->reg = s->reg;
	return 0;
}

int
copysub1(Prog *p1, Adr *v, Adr *s, int f)
{

	if(f && copyau1(p1, v))
		p1->reg = s->reg;
	return 0;
}

/*
 * substitute indreg s for reg v in a
 * return failure to substitute
 */
int
copysubind(Adr *a, Adr *v, Adr *s, int f)
{
	int used;

	used = copyau(a, v);
	if(a->type == D_FREG && !fpreg(s->reg) && used)
		return 1;
	if(f && used){
		a->type = D_INDREG;
		a->reg = s->reg;
		a->offset = 0;
	}
	return 0;
}
