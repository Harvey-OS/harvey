#include	"l.h"

void
expand(void)
{
	Prog *p, *q, *q1;
	long s;
	int o, fr, r, tr;

	/*
	 * 1. find leaf subroutines
	 * 2. strip NOPs
	 * 3. expand RETURN pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f mark\n", cputime());
	Bflush(&bso);

	/*
	 * label loads, brances, labels, and synchronizing ops
	 * for instruction scheduling
	 * find leaf routines
	 */
	q = P;
	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		/* too hard, just leave alone */
		case ATEXT:
			q = p;
			p->mark |= LABEL|LEAF|SYNC;
			if(p->link)
				p->link->mark |= LABEL;
			curtext = p;
			break;

		case ASFTRST:
		case AWAITI:
		case AWORD:
			q = p;
			p->mark |= SYNC;
			continue;

		case AIRET:
			q = p;
			p->mark |= SYNC;
			if(p->link != P)
				p->link->mark |= LABEL;
			continue;

		case ABMOVW:
		case AFADD:
		case AFADDN:
		case AFADDT:
		case AFADDTN:
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
			q = p;
			continue;

		case AMUL:
			q = p;
			if(prog_mul == P)
				initmul();
			if(curtext != P)
				curtext->mark &= ~LEAF;
			continue;
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			q = p;
			if(prog_div == P)
				initdiv();
			if(curtext != P)
				curtext->mark &= ~LEAF;
			continue;
		case AFDIV:
			q = p;
			if(prog_fdiv == P)
				initfdiv();
			if(curtext != P)
				curtext->mark &= ~LEAF;
			continue;

		/* special sync code */
		case AADD:
			q = p;
			if(p->cc == CCFALSE){
				p->mark |= SYNC;
				continue;
			}
			continue;


		case ACALL:
			if(curtext != P)
				curtext->mark &= ~LEAF;
			goto branches;

		/* check for special sync code */
		case ABRA:
			if(p->cc == CCFALSE){
				p->mark |= SYNC;
				continue;
			}
			goto branches;

		branches:
		case ADBRA:
		case AJMP:
			p->mark |= BRANCH;
			q = p;
			q1 = p->cond;
			if(q1 != P) {
				while(q1->as == ANOP) {
					q1 = q1->link;
					p->cond = q1;
				}
				if(!(q1->mark & LEAF))
					q1->mark |= LABEL;
			} else
				p->mark |= LABEL;
			q1 = p->link;
			if(q1 != P)
				q1->mark |= LABEL;
			continue;

		case ADO:		/* funny */
		case ADOLOCK:
		case ADOEND:
			q = p;
			p->mark |= SYNC;
			if(p->cond){
				if(p->as != ADOEND && p->cond->as != ADOEND)
					diag("unmatched DO %P %P\n", p, p->cond);
				if(p->cond->cond != p)
					diag("unlinked DO %P %P\n", p, p->cond);
				p->cond->mark |= SYNC;
			}
			continue;

		case ARETURN:
			q = p;
			if(p->link != P)
				p->link->mark |= LABEL;
			continue;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		default:
			q = p;
			continue;
		}
	}

	dostkoff();

	if(debug['v'])
		Bprint(&bso, "%5.2f expand\n", cputime());
	Bflush(&bso);

	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			curtext = p;
			s = p->to.offset + 4;

			/*
			 * supress saves for interrupt routines, etc
			 */
			if(!s && !(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				curtext->mark |= LEAF;
			}
			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
			}else{
				p = appendp(p);
				p->as = AMOVW;
				p->from.type = D_REG;
				p->from.reg = REGLINK;
				p->to.type = D_INDREG;
				p->from.offset = 0;
				p->to.reg = REGSP;
			}
			if(s){
				p = appendp(p);
				p->as = ASUB;
				p->from.type = D_CONST;
				p->from.offset = s;
				p->to.type = D_REG;
				p->to.reg = REGSP;
			}
			break;

		case ARETURN:
			if(p->stkoff){
				p->as = AADD;
				p->from.type = D_CONST;
				p->from.offset = p->stkoff;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				if(!(curtext->mark & LEAF)){
					p = appendp(p);
					p->as = AMOVW;
					p->from.type = D_INDREG;
					p->from.reg = REGSP;
					p->to.type = D_REG;
					p->to.reg = REGLINK;
				}
				p = appendp(p);
				p->stkoff = 0;
			}
			p->as = AJMP;
			p->from = zprg.from;
			p->to.type = D_OREG;
			p->to.offset = 0;
			p->to.reg = REGLINK;
			p->mark |= BRANCH;
			break;

		/*
		 * generate up to one address per move
		 */
		case AMOVW:
		case AMOVH:
		case AMOVHU:
		case AMOVB:
		case AMOVBU:
		case AMOVHB:
			if(p->from.type == D_NAME && !isextern(&p->from)){
				genaddr(p, &p->from);
				p = p->link;
			}else if(p->to.type == D_NAME && !isextern(&p->to)){
				genaddr(p, &p->to);
				p = p->link;
			}
			break;

		case AFMOVF:
		case AFMOVFN:
		case AFMOVFW:
		case AFMOVFH:
		case AFMOVWF:
		case AFMOVHF:
			if(p->reg == NREG){
				if(p->to.type == D_FREG){
					p->reg = p->to.reg;
					p->to.type = D_NONE;
				}else if(p->from.type == D_FREG)
					p->reg = p->from.reg;
			}
			if(p->from.type == D_FCONST){
				p->from.name = D_EXTERN;
				p->from.offset = 0;
				genaddr(p, &p->from);
				p = p->link;
			}else if(p->from.type == D_NAME){
				genaddr(p, &p->from);
				p = p->link;
			}else if(p->to.type == D_NAME){
				genaddr(p, &p->to);
				p = p->link;
			}
			break;

		case AFDIV:
			if(p->from.type != D_FREG
			|| p->from1.type != D_FREG
			|| p->to.type != D_NONE)
				break;

			fr = p->from.reg;
			r = p->from1.reg;
			tr = p->reg;

			/* FMOVF f, f, (SP)- */
			p->as = AFMOVF;
			p->isf = 1;
			p->reg = fr;
			p->from1.type = D_NONE;
			p->to.type = D_DEC;
			p->to.reg = REGSP;

			/* FMOVF f1, f1, (SP)- */
			p = appendp(p);
			p->as = AFMOVF;
			p->isf = 1;
			p->from.type = D_FREG;
			p->from.reg = r;
			p->reg = r;
			p->to.type = D_DEC;
			p->to.reg = REGSP;

			/* CALL */
			p = appendp(p);
			p->as = ACALL;
			p->to.type = D_BRANCH;
			p->mark |= BRANCH;
			p->cond = prog_fdiv;
			if(prog_fdiv)
				p->to.sym = prog_fdiv->from.sym;
			/*
			 * fdiv pops an extra cell, so we only need to add 4
			 * FMOVF (SP)+, t
			 */
			p = appendp(p);
			p->as = AFMOVF;
			p->isf = 1;
			p->reg = tr;
			p->from.type = D_INC;
			p->from.reg = REGSP;
			break;

		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			if(p->from.type != D_REG)
				break;
			if(p->to.type != D_REG)
				break;

			tr = p->to.reg;
			r = p->reg;

			/* MOVW a,(SP)- */
			p->as = AMOVW;
			p->reg = NREG;
			p->to.type = D_DEC;
			p->to.reg = REGSP;

			/* MOVW b,REGTMP */
			p = appendp(p);
			p->as = AMOVW;
			p->from.type = D_REG;
			p->from.reg = r;
			if(r == NREG)
				p->from.reg = tr;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			/* CALL appropriate */
			p = appendp(p);
			p->as = ACALL;
			p->to.type = D_BRANCH;
			p->cond = p;
			p->mark |= BRANCH;
			switch(o) {
			case AMUL:
				p->cond = prog_mul;
				break;
			case ADIV:
				p->cond = prog_div;
				break;
			case ADIVL:
				p->cond = prog_divl;
				break;
			case AMOD:
				p->cond = prog_mod;
				break;
			case AMODL:
				p->cond = prog_modl;
				break;
			}

			/* ADD $4,SP */
			p = appendp(p);
			p->as = AADD;
			p->from.type = D_CONST;
			p->from.reg = NREG;
			p->from.offset = 4;
			p->reg = NREG;
			p->to.type = D_REG;
			p->to.reg = REGSP;

			/* MOVW REGTMP, b */
			p = appendp(p);
			p->as = AMOVW;
			p->from.type = D_REG;
			p->from.reg = REGTMP;
			p->to.type = D_REG;
			p->to.reg = tr;
			break;
		}
	}
}

void
genaddr(Prog *p, Adr *a)
{
	Prog *q;

	q = prg();
	*q = *p;
	p->link = q;

	if(a == &p->from){
		q->from.type = D_INDREG;
		q->from.reg = REGTMP;
		q->from.sym = S;
	}else{
		q->to.type = D_INDREG;
		q->to.reg = REGTMP;
		q->to.sym = S;
	}

	p->as = AMOVW;
	p->isf = 0;
	p->from = *a;
	p->from.type = D_CONST;
	p->to = zprg.to;
	p->to.type = D_REG;
	p->to.reg = REGTMP;
	p->reg = NREG;
}

int
isextern(Adr* a)
{
	return a->name == D_EXTERN || a->name == D_STATIC;
}

void
noops(void)
{
	Prog *p, *p1, *q, *q1;
	int o, n;

	if(debug['v'])
		Bprint(&bso, "%5.2f noop\n", cputime());
	Bflush(&bso);
	curtext = P;
	q = P;		/* p - 1 */
	q1 = firstp;	/* top of block */
	o = 0;		/* count of instructions */
	n = 0;
	for(p = firstp; p != P; p = p1) {
		p1 = p->link;
		o++;
		if(p->nosched){
			if(q1 != p){
				n++;
				sched(q1, q);
			}
			for(; p && p->nosched; p = p->link)
				q = p;
			p1 = p;
			q1 = p;
			o = 0;
			continue;
		}
		if(p->mark & (LABEL|SYNC)) {
			if(q1 != p){
				n++;
				sched(q1, q);
			}
			q1 = p;
			o = 1;
		}
		if(p->mark & (BRANCH|SYNC)) {
			n++;
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		if(o >= NSCHED) {
			n++;
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		q = p;
	}
	if(debug['v'])
		Bprint(&bso, "%5d sched regions\n", n);
}

void
addnop(Prog *p)
{
	static Prog *nop;
	Prog *q;

	if(!nop){
		nop = prg();
		nop->as = ABRA;
		nop->cc = CCFALSE;
		nop->to.type = D_OREG;
		nop->to.reg = REGZERO;
		nop->to.offset = 0;
		opsearch(nop);
	}
	q = prg();
	*q = *nop;
	q->link = p->link;
	p->link = q;
	q->line = p->line;
	q->stkoff = p->stkoff;
}

void
initmul(void)
{
	Sym *s1;
	Prog *p;

	s1 = lookup("_mul", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT && p->from.sym == s1){
			prog_mul = p;
			return;
		}
	diag("undefined: %s\n", s1->name);
	prog_mul = curtext;
}

void
initfdiv(void)
{
	Sym *s1;
	Prog *p;

	s1 = lookup("_fdiv", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT && p->from.sym == s1){
			prog_fdiv = p;
			return;
		}
	diag("undefined: %s\n", s1->name);
	prog_mul = curtext;
}

void
initdiv(void)
{
	Sym *s1, *s2, *s3, *s4;
	Prog *p;

	s1 = lookup("_div", 0);
	s2 = lookup("_divl", 0);
	s3 = lookup("_mod", 0);
	s4 = lookup("_modl", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s1)
				prog_div = p;
			if(p->from.sym == s2)
				prog_divl = p;
			if(p->from.sym == s3)
				prog_mod = p;
			if(p->from.sym == s4)
				prog_modl = p;
		}
	if(prog_div == P) {
		diag("undefined: %s\n", s1->name);
		prog_div = curtext;
	}
	if(prog_divl == P) {
		diag("undefined: %s\n", s2->name);
		prog_divl = curtext;
	}
	if(prog_mod == P) {
		diag("undefined: %s\n", s3->name);
		prog_mod = curtext;
	}
	if(prog_modl == P) {
		diag("undefined: %s\n", s4->name);
		prog_modl = curtext;
	}
}
