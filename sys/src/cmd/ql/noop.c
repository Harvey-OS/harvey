#include	"l.h"

void
noops(void)
{
	Prog *p, *p1, *q, *q1;
	int o, mov, aoffset, curframe, curbecome, maxbecome;

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
		/* too hard, just leave alone */
		case ATEXT:
			if(curtext && curtext->from.sym) {
				curtext->from.sym->frame = curframe;
				curtext->from.sym->become = curbecome;
				if(curbecome > maxbecome)
					maxbecome = curbecome;
			}
			curframe = 0;
			curbecome = 0;

			q = p;
			p->mark |= LABEL|LEAF|SYNC;
			if(p->link)
				p->link->mark |= LABEL;
			curtext = p;
			break;

		case ANOR:
			q = p;
			if(p->to.type == D_REG)
				if(p->to.reg == REGZERO)
					p->mark |= LABEL|SYNC;
			break;

		case ALWAR:
		case ASTWCCC:
		case AECIWX:
		case AECOWX:
		case AEIEIO:
		case AICBI:
		case AISYNC:
		case ATLBIE:
		case ADCBF:
		case ADCBI:
		case ADCBST:
		case ADCBT:
		case ADCBTST:
		case ADCBZ:
		case ASYNC:
		case ATW:
		case AWORD:
		case ARFI:
		case ARFCI:
			q = p;
			p->mark |= LABEL|SYNC;
			continue;

		case AMOVW:
			q = p;
			switch(p->from.type) {
			case D_MSR:
			case D_SREG:
			case D_SPR:
			case D_FPSCR:
			case D_CREG:
			case D_DCR:
				p->mark |= LABEL|SYNC;
			}
			switch(p->to.type) {
			case D_MSR:
			case D_SREG:
			case D_SPR:
			case D_FPSCR:
			case D_CREG:
			case D_DCR:
				p->mark |= LABEL|SYNC;
			}
			continue;

		case AFABS:
		case AFABSCC:
		case AFADD:
		case AFADDCC:
		case AFCTIW:
		case AFCTIWCC:
		case AFCTIWZ:
		case AFCTIWZCC:
		case AFDIV:
		case AFDIVCC:
		case AFMADD:
		case AFMADDCC:
		case AFMOVD:
		case AFMOVDU:
		/* case AFMOVDS: */
		case AFMOVS:
		case AFMOVSU:
		/* case AFMOVSD: */
		case AFMSUB:
		case AFMSUBCC:
		case AFMUL:
		case AFMULCC:
		case AFNABS:
		case AFNABSCC:
		case AFNEG:
		case AFNEGCC:
		case AFNMADD:
		case AFNMADDCC:
		case AFNMSUB:
		case AFNMSUBCC:
		case AFRSP:
		case AFRSPCC:
		case AFSUB:
		case AFSUBCC:
			q = p;
			p->mark |= FLOAT;
			continue;

		case ABL:
		case ABCL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case ABC:
		case ABEQ:
		case ABGE:
		case ABGT:
		case ABLE:
		case ABLT:
		case ABNE:
		case ABR:
		case ABVC:
		case ABVS:

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

		case AFCMPO:
		case AFCMPU:
			q = p;
			p->mark |= FCMP|FLOAT;
			continue;

		case ARETURN:
			/* special form of RETURN is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.offset > curbecome)
					curbecome = p->from.offset;

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

		case ABL:	/* ABCL? */
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

	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			mov = AMOVW;
			aoffset = 0;
			curtext = p;
			autosize = p->to.offset + 4;
			if((p->mark & LEAF) && autosize <= 4)
				autosize = 0;
			else
				if(autosize & 4)
					autosize += 4;
			p->to.offset = autosize - 4;

			q = p;
			if(autosize) {
				/* use MOVWU to adjust R1 when saving R31, if autosize is small */
				if(!(curtext->mark & LEAF) && autosize >= -BIG && autosize <= BIG) {
					mov = AMOVWU;
					aoffset = -autosize;
				} else {
					q = prg();
					q->as = AADD;
					q->line = p->line;
					q->from.type = D_CONST;
					q->from.offset = -autosize;
					q->to.type = D_REG;
					q->to.reg = REGSP;

					q->link = p->link;
					p->link = q;
				}
			} else
			if(!(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = mov;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGTMP;
			q1->to.type = D_OREG;
			q1->to.offset = aoffset;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;

			q1 = prg();
			q1->as = AMOVW;
			q1->line = p->line;
			q1->from.type = D_SPR;
			q1->from.offset = D_LR;
			q1->to.type = D_REG;
			q1->to.reg = REGTMP;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARETURN:
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = ABR;
					p->from = zprg.from;
					p->to.type = D_SPR;
					p->to.offset = D_LR;
					p->mark |= BRANCH;
					break;
				}

				p->as = AADD;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				q = prg();
				q->as = ABR;
				q->line = p->line;
				q->to.type = D_SPR;
				q->to.offset = D_LR;
				q->mark |= BRANCH;

				q->link = p->link;
				p->link = q;
				break;
			}

			p->as = AMOVW;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			q = prg();
			q->as = AMOVW;
			q->line = p->line;
			q->from.type = D_REG;
			q->from.reg = REGTMP;
			q->to.type = D_SPR;
			q->to.offset = D_LR;

			q->link = p->link;
			p->link = q;
			p = q;

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
			q1->as = ABR;
			q1->line = p->line;
			q1->to.type = D_SPR;
			q1->to.offset = D_LR;
			q1->mark |= BRANCH;

			q1->link = q->link;
			q->link = q1;
			break;

		become:
			if(curtext->mark & LEAF) {

				q = prg();
				q->line = p->line;
				q->as = ABR;
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
			q->as = ABR;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->mark |= BRANCH;
			q->link = p->link;
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

			q = prg();
			q->line = p->line;
			q->as = AMOVW;
			q->line = p->line;
			q->from.type = D_REG;
			q->from.reg = REGTMP;
			q->to.type = D_SPR;
			q->to.offset = D_LR;
			q->link = p->link;
			p->link = q;

			p->as = AMOVW;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			break;
		}
	}

	if(debug['Q'] == 0)
		return;

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
	q->as = ANOR;
	q->line = p->line;
	q->from.type = D_REG;
	q->from.reg = REGZERO;
	q->to.type = D_REG;
	q->to.reg = REGZERO;

	q->link = p->link;
	p->link = q;
}
