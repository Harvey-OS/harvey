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

		case AORN:
			q = p;
			if(p->to.type == D_REG)
				if(p->to.reg == REGZERO)
					p->mark |= LABEL|SYNC;
			break;

		case AUNIMP:
		case ATAS:
		case ASWAP:
		case ATA:
		case ATCC:
		case ATCS:
		case ATE:
		case ATG:
		case ATGE:
		case ATGU:
		case ATL:
		case ATLE:
		case ATLEU:
		case ATN:
		case ATNE:
		case ATNEG:
		case ATPOS:
		case ATVC:
		case ATVS:
		case AWORD:
			q = p;
			p->mark |= LABEL|SYNC;
			continue;

		case AFABSD:
		case AFABSF:
		case AFABSX:
		case AFADDD:
		case AFADDF:
		case AFADDX:
		case AFDIVD:
		case AFDIVF:
		case AFDIVX:
		case AFMOVD:
		case AFMOVDF:
		case AFMOVDW:
		case AFMOVDX:
		case AFMOVF:
		case AFMOVFD:
		case AFMOVFW:
		case AFMOVFX:
		case AFMOVWD:
		case AFMOVWF:
		case AFMOVWX:
		case AFMOVX:
		case AFMOVXD:
		case AFMOVXF:
		case AFMOVXW:
		case AFMULD:
		case AFMULF:
		case AFMULX:
		case AFNEGD:
		case AFNEGF:
		case AFNEGX:
		case AFSQRTD:
		case AFSQRTF:
		case AFSQRTX:
		case AFSUBD:
		case AFSUBF:
		case AFSUBX:
			q = p;
			p->mark |= FLOAT;
			continue;

		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			q = p;
			if(!debug['M']) {
				if(prog_mul == P)
					initmuldiv();
				if(curtext != P)
					curtext->mark &= ~LEAF;
			}
			continue;

		case AJMPL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case AJMP:

		case ABA:
		case ABN:
		case ABE:
		case ABNE:
		case ABLE:
		case ABG:
		case ABL:
		case ABGE:
		case ABLEU:
		case ABGU:
		case ABCS:
		case ABCC:
		case ABNEG:
		case ABPOS:
		case ABVC:
		case ABVS:

		case AFBN:
		case AFBO:
		case AFBE:
		case AFBLG:
		case AFBG:
		case AFBLE:
		case AFBGE:
		case AFBL:
		case AFBNE:
		case AFBUE:
		case AFBA:
		case AFBU:
		case AFBUG:
		case AFBULE:
		case AFBUGE:
		case AFBUL:
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

		case AFCMPD:
		case AFCMPED:
		case AFCMPEF:
		case AFCMPEX:
		case AFCMPF:
		case AFCMPX:
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

		case AJMPL:
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
				q = prg();
				q->as = ASUB;
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
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = AMOVW;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_OREG;
			q1->from.offset = 0;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;
			break;

		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			if(debug['M'])
				break;
			if(p->from.type != D_REG)
				break;
			if(p->to.type != D_REG)
				break;
			q1 = p;

			/* MOV a,4(SP) */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = q1->from.reg;
			p->to.type = D_OREG;
			p->to.reg = REGSP;
			p->to.offset = 4;

			/* MOV b,REGTMP */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = q1->reg;
			if(q1->reg == NREG)
				p->from.reg = q1->to.reg;
			p->to.type = D_REG;
			p->to.reg = REGTMP;
			p->to.offset = 0;

			/* CALL appropriate */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AJMPL;
			p->line = q1->line;
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

			/* MOV REGTMP, b */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = REGTMP;
			p->from.offset = 0;
			p->to.type = D_REG;
			p->to.reg = q1->to.reg;

			/* ADD $8,SP */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AADD;
			p->from.type = D_CONST;
			p->from.reg = NREG;
			p->from.offset = 8;
			p->reg = NREG;
			p->to.type = D_REG;
			p->to.reg = REGSP;

			/* SUB $8,SP */
			q1->as = ASUB;
			q1->from.type = D_CONST;
			q1->from.offset = 8;
			q1->from.reg = NREG;
			q1->reg = NREG;
			q1->to.type = D_REG;
			q1->to.reg = REGSP;
			break;

		case ARETURN:
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AJMP;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.offset = 8;
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
				q->to.offset = 8;
				q->to.reg = REGLINK;
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
			p->to.reg = REGRET+1;

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
			q1->to.offset = 8;
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

			p->as = AMOVW;
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
	q->as = AORN;
	q->line = p->line;
	q->from.type = D_REG;
	q->from.reg = REGZERO;
	q->to.type = D_REG;
	q->to.reg = REGZERO;

	q->link = p->link;
	p->link = q;
}

void
initmuldiv(void)
{
	Sym *s1, *s2, *s3, *s4, *s5;
	Prog *p;

	s1 = lookup("_mul", 0);
	s2 = lookup("_div", 0);
	s3 = lookup("_divl", 0);
	s4 = lookup("_mod", 0);
	s5 = lookup("_modl", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s1)
				prog_mul = p;
			if(p->from.sym == s2)
				prog_div = p;
			if(p->from.sym == s3)
				prog_divl = p;
			if(p->from.sym == s4)
				prog_mod = p;
			if(p->from.sym == s5)
				prog_modl = p;
		}
	if(prog_mul == P) {
		diag("undefined: %s", s1->name);
		prog_mul = curtext;
	}
	if(prog_div == P) {
		diag("undefined: %s", s2->name);
		prog_div = curtext;
	}
	if(prog_divl == P) {
		diag("undefined: %s", s3->name);
		prog_divl = curtext;
	}
	if(prog_mod == P) {
		diag("undefined: %s", s4->name);
		prog_mod = curtext;
	}
	if(prog_modl == P) {
		diag("undefined: %s", s5->name);
		prog_modl = curtext;
	}
}
