#include	"l.h"

void
noops(void)
{
	Prog *p, *q, *q1;
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
			if(p->link)
				p->link->mark |= LABEL;
			curtext = p;
			break;

		/* too hard, just leave alone */
		case AMOVW:
		case AMOV:
			if(p->to.type == D_CTLREG) {
				p->mark |= LABEL|SYNC;
				break;
			}
			if(p->from.type == D_CTLREG) {
				p->mark |= LABEL|SYNC;
				break;
			}
			break;

		/* too hard, just leave alone */
		case ASYS:
		case AWORD:
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

		case AJAL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case AJMP:
		case ABEQ:
		case ABGE:
		case ABGEU:
		case ABLT:
		case ABLTU:
		case ABNE:
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
		case AJAL:
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
			autosize = p->to.offset + ptrsize;
			if(autosize <= ptrsize) {
				if(curtext->mark & LEAF || autosize <= 0) {
					p->to.offset = -ptrsize;
					autosize = 0;
				} else if(ptrsize == 4) {
					p->to.offset = 4;
					autosize = 8;
				}
			}

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADD;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = -autosize;
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
			q1->as = thechar == 'j' ? AMOV : AMOVW;
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

				p->as = AADD;
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
			p->as = thechar == 'j' ? AMOV : AMOVW;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADD;
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
			q1->to.reg = REGLINK;
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

				p->as = AADD;
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
			q->as = AADD;
			q->from.type = D_CONST;
			q->from.offset = autosize;
			q->to.type = D_REG;
			q->to.reg = REGSP;
			q->link = p->link;
			p->link = q;

			p->as = thechar == 'j' ? AMOV : AMOVW;
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
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}
