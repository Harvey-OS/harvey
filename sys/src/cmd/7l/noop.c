#include	"l.h"

void
noops(void)
{
	Prog *p, *q, *q1;
	int o, aoffset, curframe, curbecome, maxbecome;

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

			p->mark |= LEAF;
			curtext = p;
			break;

		case ARETURN:
			/* special form of RETURN is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.offset > curbecome)
					curbecome = p->from.offset;
			break;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case ABL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case ACBNZ:
		case ACBZ:
		case ACBNZW:
		case ACBZW:
		case ATBZ:
		case ATBNZ:

		case ABCASE:
		case AB:

		case ABEQ:
		case ABNE:
		case ABCS:
		case ABHS:
		case ABCC:
		case ABLO:
		case ABMI:
		case ABPL:
		case ABVS:
		case ABVC:
		case ABHI:
		case ABLS:
		case ABGE:
		case ABLT:
		case ABGT:
		case ABLE:

		case AADR:		/* strange */
		case AADRP:

			q1 = p->cond;
			if(q1 != P) {
				while(q1->as == ANOP) {
					q1 = q1->link;
					p->cond = q1;
				}
			}
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
		case ABL:
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
			if(p->to.offset < 0)
				autosize = 0;
			else
				autosize = p->to.offset + PCSZ;
			if((curtext->mark & LEAF) && autosize <= PCSZ)
				autosize = 0;
			else if(autosize & (STACKALIGN-1))
				autosize += STACKALIGN - (autosize&(STACKALIGN-1));
			p->to.offset = autosize - PCSZ;

			if(autosize == 0 && !(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				Bflush(&bso);
				curtext->mark |= LEAF;
			}

			aoffset = autosize;
			if(aoffset > 0xF0)
				aoffset = 0xF0;

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				if(autosize == 0)
					break;
				aoffset = 0;
			}

			q = p;
			if(autosize > aoffset){
				q = prg();
				q->as = ASUB;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize - aoffset;
				q->to.type = D_REG;
				q->to.reg = REGSP;
				q->link = p->link;
				p->link = q;

				if(curtext->mark & LEAF)
					break;
			}

			q1 = prg();
			q1->as = AMOV;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_XPRE;
			q1->to.offset = -aoffset;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARETURN:
			nocache(p);
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(autosize != 0){
					p->as = AADD;
					p->from.type = D_CONST;
					p->from.offset = autosize;
					p->to.type = D_REG;
					p->to.reg = REGSP;
				}
			}else{
				/* want write-back pre-indexed SP+autosize -> SP, loading REGLINK*/
				aoffset = autosize;
				if(aoffset > 0xF0)
					aoffset = 0xF0;

				p->as = AMOV;
				p->from.type = D_XPOST;
				p->from.offset = aoffset;
				p->from.reg = REGSP;
				p->to.type = D_REG;
				p->to.reg = REGLINK;

				if(autosize > aoffset) {
					q = prg();
					q->as = AADD;
					q->from.type = D_CONST;
					q->from.offset = autosize - aoffset;
					q->to.type = D_REG;
					q->to.reg = REGSP;

					q->link = p->link;
					p->link = q;
					p = q;
				}
			}

			if(p->as != ARETURN) {
				q = prg();
				q->line = p->line;
				q->link = p->link;
				p->link = q;
				p = q;
			}

			p->as = ARET;
			p->line = p->line;
			p->to.type = D_OREG;
			p->to.offset = 0;
			p->to.reg = REGLINK;

			break;

		become:
			if(curtext->mark & LEAF) {

				if(!autosize) {
					p->as = AB;
					p->from = zprg.from;
					break;
				}

#ifdef optimise_time
				q = prg();
				q->line = p->line;
				q->as = AB;
				q->from = zprg.from;
				q->to = p->to;
				q->cond = p->cond;
				q->link = p->link;
				p->link = q;

				p->as = AADD;
				p->from = zprg.from;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to = zprg.to;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				break;
#endif
			}
			q = prg();
			q->line = p->line;
			q->as = AB;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			p->link = q;

			p->as = AMOV;
			p->from = zprg.from;
			p->from.type = D_XPRE;
			p->from.offset = -autosize;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;

		}
	}
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}
