#include	"l.h"

void
noops(void)
{
	Prog *p, *p1, *q, *q1;
	int o, curframe, curbecome, maxbecome;

	/*
	 * find leaf subroutines
	 * become sizes
	 * frame sizes
	 * strip NOPs
	 * expand RET
	 * expand BECOME pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);

	curframe = 0;
	curbecome = 0;
	maxbecome = 0;
	curtext = 0;

	q = P;
	for(p = firstp; p != P; p = p->link) {

		/* find out how much arg space is used in this TEXT */
		if(p->to.type == D_OREG && p->to.reg == REGSP)
			if(p->to.offset > curframe)
				curframe = p->to.offset;

		switch(p->as) {
		case ATEXT:
			if(curtext && curtext->from.sym) {
				curtext->from.sym->frame = curframe;
				curtext->from.sym->become = curbecome;
				if(curbecome > maxbecome)
					maxbecome = curbecome;
			}
			curframe = 0;
			curbecome = 0;

			p->mark |= LABEL|LEAF|SYNC;
			curtext = p;
			break;

		/* too hard, just leave alone */
		case AWORD:
		case ADELAY:
			p->mark |= LABEL|SYNC;
			break;

		case ARET:
			/* special form of RET is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.offset > curbecome)
					curbecome = p->from.offset;

			if(p->link != P)
				p->link->mark |= LABEL;
			break;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case AMULL:
		case AMULUL:
		case AMULML:
		case AMULMUL:
			if(debug['m'])
				break;
			if(prog_mull == P)
				initmul();
			goto muldiv;

		case ADIVL:
		case ADIVUL:
			if(debug['d'])
				break;
			if(prog_divl == P)
				initdiv();
			goto muldiv;

		muldiv:
			if(curtext != P)
				curtext->mark &= ~LEAF;
			q = prg();
			q->link = p->link;
			p->link = q;

			q->as = ACALL;
			q->line = p->line;
			q->to.type = D_BRANCH;
			q->cond = p;
			q->mark |= BRANCH;
			switch(p->as) {
			case AMULL:
				q->cond = prog_mull;
				break;
			case AMULUL:
				q->cond = prog_mulul;
				break;
			case AMULML:
				q->cond = prog_mulml;
				break;
			case AMULMUL:
				q->cond = prog_mulmul;
				break;
			case ADIVL:
				q->cond = prog_divl;
				break;
			case ADIVUL:
				q->cond = prog_divul;
				break;
			}
			p->as = ASETIP;
			continue;

		case ACALL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case AJMP:
		case AJMPT:
		case AJMPF:
			p->mark |= BRANCH;
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
			break;
		}
		q = p;
	}

	if(curtext && curtext->from.sym) {
		curtext->from.sym->frame = curframe;
		curtext->from.sym->become = curbecome;
		if(curbecome > maxbecome)
			maxbecome = curbecome;
	}

	if(debug['b'])
		print("max become = %d\n", maxbecome);
	xdefine("ALEFbecome", STEXT, maxbecome);

	curtext = 0;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		case ATEXT:
			curtext = p;
			break;
		case ACALL:
			if(curtext != P && curtext->from.sym != S && curtext->to.offset >= 0) {
				o = maxbecome - curtext->from.sym->frame;
				if(o <= 0)
					break;
				/* calling a become or calling a variable */
				if(p->to.sym == S || p->to.sym->become) {
					curtext->to.offset += o;
					if(debug['b']) {
						curp = p;
						print("%D calling %D increase %d\n",
							&curtext->from, &p->to, o);
					}
				}
			}
			break;
		}
	}

	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			curtext = p;
			autosize = p->to.offset + 4;
			if(autosize <= 4)
			if(curtext->mark & LEAF) {
				p->to.offset = -4;
				autosize = 0;
			}

			q = p;
			if(autosize) {
				q = prg();
				q->as = ASUBL;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			} else
			if(!(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				Bflush(&bso);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = AMOVL;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_OREG;
			q1->from.offset = 0;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARET:
			nocache(p);
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AJMP;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.offset = 0;
					p->to.reg = REGLINK;
					p->mark |= BRANCH;
					break;
				}

				p->as = AADDL;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				q = prg();
				q->as = AJMP;
				q->line = p->line;
				q->to.type = D_OREG;
				q->to.offset = 0;
				q->to.reg = REGLINK;
				q->mark |= BRANCH;

				q->link = p->link;
				p->link = q;
				break;
			}
			p->as = AMOVL;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = REGRET+1;

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADDL;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			}

			q1 = prg();
			q1->as = AJMP;
			q1->line = p->line;
			q1->to.type = D_OREG;
			q1->to.offset = 0;
			q1->to.reg = REGRET+1;
			q1->mark |= BRANCH;

			q1->link = q->link;
			q->link = q1;
			break;

		become:
			if(curtext->mark & LEAF) {

				q = prg();
				q->line = p->line;
				q->as = AJMP;
				q->from = zprg.from;
				q->to = p->to;
				q->cond = p->cond;
				q->link = p->link;
				q->mark |= BRANCH;
				p->link = q;

				p->as = AADDL;
				p->from = zprg.from;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to = zprg.to;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				break;
			}
			q = prg();
			q->line = p->line;
			q->as = AJMP;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			q->mark |= BRANCH;
			p->link = q;

			q = prg();
			q->line = p->line;
			q->as = AADDL;
			q->from.type = D_CONST;
			q->from.offset = autosize;
			q->to.type = D_REG;
			q->to.reg = REGSP;
			q->link = p->link;
			p->link = q;

			p->as = AMOVL;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;
		}
	}

	curtext = P;
	q = P;		/* p - 1 */
	q1 = firstp;	/* top of block */
	o = 0;		/* count of instructions */
	for(p = firstp; p != P; p = p1) {
		p1 = p->link;
		o++;
		if(p->mark & NOSCHED){
			if(q1 != p){
				sched(q1, q);
			}
			for(; p != P; p = p->link){
				if(!(p->mark & NOSCHED))
					break;
				q = p;
			}
			p1 = p;
			q1 = p;
			o = 0;
			continue;
		}
		if(p->mark & (LABEL|SYNC)) {
			if(q1 != p)
				sched(q1, q);
			q1 = p;
			o = 1;
		}
		if(p->mark & (BRANCH|SYNC)) {
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		if(o >= NSCHED) {
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		q = p;
	}
}

void
addnop(Prog *p)
{
	Prog *q;

	q = prg();
	q->as = ADELAY;
	q->line = p->line;
	q->from = zprg.from;
	q->to = zprg.from;

	q->link = p->link;
	p->link = q;
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}

void
initmul(void)
{
	Sym *s1, *s2, *s3, *s4;
	Prog *p;

	s1 = lookup("_mull", 0);
	s2 = lookup("_mulul", 0);
	s3 = lookup("_mulml", 0);
	s4 = lookup("_mulmul", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s1)
				prog_mull = p;
			if(p->from.sym == s2)
				prog_mulul = p;
			if(p->from.sym == s3)
				prog_mulml = p;
			if(p->from.sym == s4)
				prog_mulmul = p;
		}
	if(prog_mull == P) {
		diag("undefined: %s\n", s1->name);
		prog_mull = curtext;
	}
	if(prog_mulul == P) {
		diag("undefined: %s\n", s2->name);
		prog_mulul = curtext;
	}
	if(prog_mulml == P) {
		diag("undefined: %s\n", s3->name);
		prog_mulml = curtext;
	}
	if(prog_mulmul == P) {
		diag("undefined: %s\n", s4->name);
		prog_mulmul = curtext;
	}
}

void
initdiv(void)
{
	Sym *s5, *s6;
	Prog *p;

	s5 = lookup("_divl", 0);
	s6 = lookup("_divul", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s5)
				prog_divl = p;
			if(p->from.sym == s6)
				prog_divul = p;
		}
	if(prog_divl == P) {
		diag("undefined: %s\n", s5->name);
		prog_divl = curtext;
	}
	if(prog_divul == P) {
		diag("undefined: %s\n", s6->name);
		prog_divul = curtext;
	}
}
