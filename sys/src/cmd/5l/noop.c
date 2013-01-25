#include	"l.h"

static	Sym*	sym_div;
static	Sym*	sym_divu;
static	Sym*	sym_mod;
static	Sym*	sym_modu;

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

			p->mark |= LEAF;
			curtext = p;
			break;

		case ARET:
			/* special form of RET is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.offset > curbecome)
					curbecome = p->from.offset;
			break;

		case ADIV:
		case ADIVU:
		case AMOD:
		case AMODU:
			q = p;
			if(prog_div == P)
				initdiv();
			if(curtext != P)
				curtext->mark &= ~LEAF;
			continue;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case ABL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

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
			autosize = p->to.offset + 4;
			if(autosize <= 4)
			if(curtext->mark & LEAF) {
				p->to.offset = -4;
				autosize = 0;
			}

			if(!autosize && !(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				Bflush(&bso);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
#ifdef optimise_time
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
				}
				break;
#else
				if(!autosize)
					break;
#endif
			}

			q1 = prg();
			q1->as = AMOVW;
			q1->scond |= C_WBIT;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_OREG;
			q1->to.offset = -autosize;
			q1->to.reg = REGSP;

			q1->link = p->link;
			p->link = q1;
			break;

		case ARET:
			nocache(p);
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AB;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.offset = 0;
					p->to.reg = REGLINK;
					break;
				}

#ifdef optimise_time
				p->as = AADD;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				q = prg();
				q->as = AB;
				q->scond = p->scond;
				q->line = p->line;
				q->to.type = D_OREG;
				q->to.offset = 0;
				q->to.reg = REGLINK;

				q->link = p->link;
				p->link = q;

				break;
#endif
			}
			p->as = AMOVW;
			p->scond |= C_PBIT;
			p->from.type = D_OREG;
			p->from.offset = autosize;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = REGPC;
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
				q->scond = p->scond;
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
			q->scond = p->scond;
			q->line = p->line;
			q->as = AB;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			p->link = q;

			p->as = AMOVW;
			p->scond |= C_PBIT;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.offset = autosize;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;

		/*
		 * 5c code generation for unsigned -> double made the
		 * unfortunate assumption that single and double floating
		 * point registers are aliased - true for emulated 7500
		 * but not for vfp.  Now corrected, but this test is
		 * insurance against old 5c compiled code in libraries.
		 */
		case AMOVWD:
			if((q = p->link) != P && q->as == ACMP)
			if((q = q->link) != P && q->as == AMOVF)
			if((q1 = q->link) != P && q1->as == AADDF)
			if(q1->to.type == D_FREG && q1->to.reg == p->to.reg) {
				q1->as = AADDD;
				q1 = prg();
				q1->scond = q->scond;
				q1->line = q->line;
				q1->as = AMOVFD;
				q1->from = q->to;
				q1->to = q1->from;
				q1->link = q->link;
				q->link = q1;
			}
			break;

		case ADIV:
		case ADIVU:
		case AMOD:
		case AMODU:
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

			p->as = ABL;
			p->line = q1->line;
			p->to.type = D_BRANCH;
			p->cond = p;
			switch(o) {
			case ADIV:
				p->cond = prog_div;
				p->to.sym = sym_div;
				break;
			case ADIVU:
				p->cond = prog_divu;
				p->to.sym = sym_divu;
				break;
			case AMOD:
				p->cond = prog_mod;
				p->to.sym = sym_mod;
				break;
			case AMODU:
				p->cond = prog_modu;
				p->to.sym = sym_modu;
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
		}
	}
}

static void
sigdiv(char *n)
{
	Sym *s;

	s = lookup(n, 0);
	if(s->type == STEXT){
		if(s->sig == 0)
			s->sig = SIGNINTERN;
	}
	else if(s->type == 0 || s->type == SXREF)
		s->type = SUNDEF;
}

void
divsig(void)
{
	sigdiv("_div");
	sigdiv("_divu");
	sigdiv("_mod");
	sigdiv("_modu");
}

static void
sdiv(Sym *s)
{
	if(s->type == 0 || s->type == SXREF){
		/* undefsym(s); */
		s->type = SXREF;
		if(s->sig == 0)
			s->sig = SIGNINTERN;
		s->subtype = SIMPORT;
	}
	else if(s->type != STEXT)
		diag("undefined: %s", s->name);
}

void
initdiv(void)
{
	Sym *s2, *s3, *s4, *s5;
	Prog *p;

	if(prog_div != P)
		return;
	sym_div = s2 = lookup("_div", 0);
	sym_divu = s3 = lookup("_divu", 0);
	sym_mod = s4 = lookup("_mod", 0);
	sym_modu = s5 = lookup("_modu", 0);
	if(dlm) {
		sdiv(s2); if(s2->type == SXREF) prog_div = UP;
		sdiv(s3); if(s3->type == SXREF) prog_divu = UP;
		sdiv(s4); if(s4->type == SXREF) prog_mod = UP;
		sdiv(s5); if(s5->type == SXREF) prog_modu = UP;
	}
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s2)
				prog_div = p;
			if(p->from.sym == s3)
				prog_divu = p;
			if(p->from.sym == s4)
				prog_mod = p;
			if(p->from.sym == s5)
				prog_modu = p;
		}
	if(prog_div == P) {
		diag("undefined: %s", s2->name);
		prog_div = curtext;
	}
	if(prog_divu == P) {
		diag("undefined: %s", s3->name);
		prog_divu = curtext;
	}
	if(prog_mod == P) {
		diag("undefined: %s", s4->name);
		prog_mod = curtext;
	}
	if(prog_modu == P) {
		diag("undefined: %s", s5->name);
		prog_modu = curtext;
	}
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}
