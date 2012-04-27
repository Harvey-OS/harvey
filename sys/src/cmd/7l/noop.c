#include	"l.h"

Prog	*divuconst(Prog *, uvlong, int, int, int);
Prog	*divconst(Prog *, vlong, int, int, int);
Prog	*modconst(Prog *, vlong, int, int, int);
void	excise(Prog *);

void
noops(void)
{
	Prog *p, *p1, *q, *q1, *q2;
	int o, curframe, curbecome, maxbecome, shift;

	/*
	 * find leaf subroutines
	 * become sizes
	 * frame sizes
	 * strip NOPs
	 * expand RET and other macros
	 * expand BECOME pseudo
	 * use conditional moves where appropriate
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

		/* don't mess with what we don't understand */
		case AWORD:
		case ACALL_PAL:
		/* etc. */
			p->mark |= LABEL;
			for(q1=p->link; q1 != P; q1 = q1->link) {
				q1->mark |= LABEL;
				if(q1->as != AXORNOT)		/* used as NOP in PALcode */
					break;
			}
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

		case AJSR:
			if(curtext != P)
				curtext->mark &= ~LEAF;
		case ABEQ:
		case ABNE:
		case ABGE:
		case ABGT:
		case ABLE:
		case ABLT:
		case ABLBC:
		case ABLBS:
		case AFBEQ:
		case AFBNE:
		case AFBGE:
		case AFBGT:
		case AFBLE:
		case AFBLT:
		case AJMP:
			p->mark |= BRANCH;
			q1 = p->cond;
			if(q1 != P) {
				while(q1->as == ANOP) {
					q1 = q1->link;
					p->cond = q1;
				}
				if(!(q1->mark & LEAF)) {
					if (q1->mark & LABEL)
						q1->mark |= LABEL2;
					else
						q1->mark |= LABEL;
				}
			} else
				p->mark |= LABEL;
			q1 = p->link;
			if(q1 != P) {
				if (q1->mark & LABEL)
					q1->mark |= LABEL2;
				else
					q1->mark |= LABEL;
			}
			else
				p->mark |= LABEL;	/* ??? */
			break;

		case ADIVQ:
		case ADIVQU:
		case AMODQ:
		case AMODQU:
		case ADIVL:
		case ADIVLU:
		case AMODL:
		case AMODLU:
			if(p->from.type == D_CONST /*&& !debug['d']*/)
				continue;
			if(prog_divq == P)
				initdiv();
			if(curtext != P)
				curtext->mark &= ~LEAF;
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
		case AJSR:
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
			autosize = p->to.offset + 8;
			if(autosize <= 8)
			if(curtext->mark & LEAF) {
				p->to.offset = -8;
				autosize = 0;
			}
			if (autosize & 4)
				autosize += 4;

			q = p;
			if(autosize)
				q = genIRR(p, ASUBQ, autosize, NREG, REGSP);
			else if(!(curtext->mark & LEAF)) {
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

			genstore(q, AMOVL, REGLINK, 0LL, REGSP);
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
					break;
				}

				p->as = AADDQ;
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
			p->to.reg = REGLINK;

			q = p;
			if(autosize)
				q = genIRR(p, AADDQ, autosize, NREG, REGSP);

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

				p->as = AADDQ;
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

			q = genIRR(p, AADDQ, autosize, NREG, REGSP);

			p->as = AMOVL;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;
			

		/* All I wanted was a MOVB... */
		case AMOVB:
		case AMOVW:
			/* rewrite sign extend; could use v3 extension in asmout case 1 */
			if (p->to.type == D_REG) {
				nocache(p);
				shift = (p->as == AMOVB) ? (64-8) : (64-16);
				if (p->from.type == D_REG) {
					p->as = ASLLQ;
					p->reg = p->from.reg;
					p->from.type = D_CONST;
					p->from.offset = shift;
					q = genIRR(p, ASRAQ, shift, p->to.reg, p->to.reg);
					break;
				}
				else {
					p->as = (p->as == AMOVB) ? AMOVBU : AMOVWU;
					q = genIRR(p, ASLLQ, shift, p->to.reg, p->to.reg);
					q = genIRR(q, ASRAQ, shift, p->to.reg, p->to.reg);
				}
			}
			/* fall through... */
		case AMOVBU:
		case AMOVWU:
			if(!debug['x'])
				break;		/* use BWX extension */
			o = p->as;
			nocache(p);
			if (p->from.type == D_OREG) {
				if (p->to.type != D_REG)
					break;
				p->as = AMOVQU;
				q = genXXX(p, AEXTBL, &p->to, REGTMP2, &p->to);
				if (o == AMOVW || o == AMOVWU)
					q->as = AEXTWL;
				p->to.reg = REGTMP2;
				if ((p->from.offset & 7) != 0 || aclass(&p->from) != C_SOREG) {
					q1 = genXXX(p, AMOVA, &p->from, NREG, &q->to);
					q1->from.offset &= 7;
					q->from = q->to;
				}
				else
					q->from.reg = p->from.reg;
				if (o == AMOVB || o == AMOVW)
					genXXX(q, o, &q->to, NREG, &q->to);
			}
			else if (p->to.type == D_OREG) {
				if (aclass(&p->from) == C_ZCON) {
					p->from.type = D_REG;
					p->from.reg = REGZERO;
				}
				else if (p->from.type != D_REG)
					break;
				p->as = AMOVQU;
				q = genRRR(p, AMSKBL, p->to.reg, REGTMP2, REGTMP2);
				q1 = genRRR(q, AINSBL, p->to.reg, p->from.reg, REGTMP);
				if (o == AMOVW || o == AMOVWU) {
					q->as = AMSKWL;
					q1->as = AINSWL;
				}
				q2 = genXXX(q1, AOR, &q->to, REGTMP, &q->to);
				genXXX(q2, AMOVQU, &q->to, NREG, &p->to);
				p->from = p->to;
				p->to = q->to;
				if ((p->from.offset & 7) != 0 || aclass(&p->from) != C_SOREG) {
					q->from.reg = REGTMP;
					q1->from.reg = REGTMP;
					q = genXXX(p, AMOVA, &p->from, NREG, &q->from);
					q->from.offset &= 7;
				}
			}
			break;

		case ASLLL:
			p->as = ASLLQ;
			p = genXXX(p, AADDL, &p->to, REGZERO, &p->to);
			break;

		case ASRLL:
			if (p->to.type != D_REG) {
				diag("illegal dest type in %P", p);
				break;
			}
			if (p->reg == NREG)
				p->reg = p->to.reg;

			q = genXXX(p, ASRLQ, &p->from, REGTMP, &p->to);

			p->as = AZAP;
			p->from.type = D_CONST;
			p->from.offset = 0xf0;
			p->to.reg = REGTMP;
			p = q;

			p = genXXX(p, AADDL, &p->to, REGZERO, &p->to);
			break;

		case ASRAL:
			p->as = ASRAQ;
			break;

		case ADIVQ:
		case ADIVQU:
		case AMODQ:
		case AMODQU:
		case ADIVL:
		case ADIVLU:
		case AMODL:
		case AMODLU:
			/* if (debug['d'])
				print("%P\n", p); */
			if(p->to.type != D_REG)
				break;
			/*if(debug['d'] && p->from.type == D_CONST) {
				q = genRRR(p, p->as, REGTMP, p->reg, p->to.reg);
				p->as = AMOVQ;
				p->reg = NREG;
				p->to.reg = REGTMP;
				p = q;
			}*/
			if(p->from.type == D_CONST) {
				if (p->reg == NREG)
					p->reg = p->to.reg;
				switch (p->as) {
				case ADIVQ:
					q = divconst(p, p->from.offset, p->reg, p->to.reg, 64);
					break;
				case ADIVQU:
					q = divuconst(p, p->from.offset, p->reg, p->to.reg, 64);
					break;
				case AMODQ:
					q = modconst(p, p->from.offset, p->reg, p->to.reg, 64);
					break;
				case AMODQU:
					q = divuconst(p, p->from.offset, p->reg, REGTMP2, 64);
					q = genIRR(q, AMULQ, p->from.offset, REGTMP2, REGTMP2);
					q = genRRR(q, ASUBQ, REGTMP2, p->reg, p->to.reg);
					break;
				case ADIVL:
					q = divconst(p, p->from.offset, p->reg, p->to.reg, 32);
					break;
				case ADIVLU:
					q = divuconst(p, p->from.offset, p->reg, p->to.reg, 32);
					break;
				case AMODL:
					q = modconst(p, p->from.offset, p->reg, p->to.reg, 32);
					break;
				case AMODLU:
					q = divuconst(p, p->from.offset, p->reg, REGTMP2, 32);
					q = genIRR(q, AMULQ, p->from.offset, REGTMP2, REGTMP2);
					q = genRRR(q, ASUBQ, REGTMP2, p->reg, p->to.reg);
					break;
				}
				excise(p);
				p = q;
				break;
			}
			if(p->from.type != D_REG){
				diag("bad instruction %P", p);
				break;
			}
			o = p->as;
			q = genIRR(p, ASUBQ, 16LL, NREG, REGSP);
			q = genstore(q, AMOVQ, p->from.reg, 8LL, REGSP);
			if (o == ADIVL || o == ADIVL || o == AMODL || o == AMODLU)
				q->as = AMOVL;

			q = genRRR(q, AMOVQ, p->reg, NREG, REGTMP);
			if (p->reg == NREG)
				q->from.reg = p->to.reg;

			/* CALL appropriate */
			q1 = prg();
			q1->link = q->link;
			q->link = q1;

			q1->as = AJSR;
			q1->line = p->line;
			q1->to.type = D_BRANCH;
			q1->cond = divsubr(o);
			q1->mark |= BRANCH;
			q = q1;

			q = genRRR(q, AMOVQ, REGTMP, NREG, p->to.reg);
			q = genIRR(q, AADDQ, 16LL, NREG, REGSP);
			excise(p);
			p = q;
			break;

		/* Attempt to replace {cond. branch, mov} with a cmov */
		/* XXX warning: this is all a bit experimental */
		case ABEQ:
		case ABNE:
		case ABGE:
		case ABGT:
		case ABLE:
		case ABLT:
		case ABLBC:
		case ABLBS:
			q = p->link;
			if (q == P)
				break;
			q1 = q->link;
			if (q1 != p->cond || q1 == P)
				break;
/*print("%P\n", q); /* */
			if (q->to.type != D_REG)
				break;
			if (q->from.type != D_REG && (q->from.type != D_CONST || q->from.name != D_NONE))
				break;
			if (q->mark&LABEL2)
				break;
/* print("%P\n", q); /* */
			if (q->as != AMOVQ)		/* XXX can handle more than this! */
				break;
			q->as = (p->as^1) + ACMOVEQ-ABEQ;	/* sleazy hack */
			q->reg = p->from.reg;		/* XXX check CMOVx operand order! */
			excise(p);		/* XXX p's LABEL? */
			if (!(q1->mark&LABEL2))
				q1->mark &= ~LABEL;
			break;
		case AFBEQ:
		case AFBNE:
		case AFBGE:
		case AFBGT:
		case AFBLE:
		case AFBLT:
			q = p->link;
			if (q == P)
				break;
			q1 = q->link;
			if (q1 != p->cond || q1 == P)
				break;
			if (q->from.type != D_FREG || q->to.type != D_FREG)
				break;
/* print("%P\n", q); /* */
			if (q->mark&LABEL2)
				break;
			if (q->as != AMOVT)		/* XXX can handle more than this! */
				break;
			q->as = (p->as^1) + AFCMOVEQ-AFBEQ;	/* sleazy hack */
			q->reg = p->from.reg;		/* XXX check CMOVx operand order! */
			excise(p);		/* XXX p's LABEL? */
			if (!(q1->mark&LABEL2))
				q1->mark &= ~LABEL;
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
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}

/* XXX use of this may lose important LABEL flags, check that this isn't happening (or fix) */
void
excise(Prog *p)
{
	Prog *q;

	q = p->link;
	*p = *q;
}

void
initdiv(void)
{
	Sym *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8;
	Prog *p;

	s1 = lookup("_divq", 0);
	s2 = lookup("_divqu", 0);
	s3 = lookup("_modq", 0);
	s4 = lookup("_modqu", 0);
	s5 = lookup("_divl", 0);
	s6 = lookup("_divlu", 0);
	s7 = lookup("_modl", 0);
	s8 = lookup("_modlu", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s1)
				prog_divq = p;
			if(p->from.sym == s2)
				prog_divqu = p;
			if(p->from.sym == s3)
				prog_modq = p;
			if(p->from.sym == s4)
				prog_modqu = p;
			if(p->from.sym == s5)
				prog_divl = p;
			if(p->from.sym == s6)
				prog_divlu = p;
			if(p->from.sym == s7)
				prog_modl = p;
			if(p->from.sym == s8)
				prog_modlu = p;
		}
	if(prog_divq == P) {
		diag("undefined: %s", s1->name);
		prog_divq = curtext;
	}
	if(prog_divqu == P) {
		diag("undefined: %s", s2->name);
		prog_divqu = curtext;
	}
	if(prog_modq == P) {
		diag("undefined: %s", s3->name);
		prog_modq = curtext;
	}
	if(prog_modqu == P) {
		diag("undefined: %s", s4->name);
		prog_modqu = curtext;
	}
	if(prog_divl == P) {
		diag("undefined: %s", s5->name);
		prog_divl = curtext;
	}
	if(prog_divlu == P) {
		diag("undefined: %s", s6->name);
		prog_divlu = curtext;
	}
	if(prog_modl == P) {
		diag("undefined: %s", s7->name);
		prog_modl = curtext;
	}
	if(prog_modlu == P) {
		diag("undefined: %s", s8->name);
		prog_modlu = curtext;
	}
}

Prog *
divsubr(int o)
{
	switch(o) {
	case ADIVQ:
		return prog_divq;
	case ADIVQU:
		return prog_divqu;
	case AMODQ:
		return prog_modq;
	case AMODQU:
		return prog_modqu;
	case ADIVL:
		return prog_divl;
	case ADIVLU:
		return prog_divlu;
	case AMODL:
		return prog_modl;
	case AMODLU:
		return prog_modlu;
	default:
		diag("bad op %A in divsubr", o);
		return prog_modlu;
	}
}

Prog*
divuconst(Prog *p, uvlong y, int num, int quot, int bits)
{
	int logy, i, shift;
	uvlong k, m, n, mult, tmp, msb;

	if(num == NREG)
		num = quot;
	if(y == 0) {
		diag("division by zero");
		return p;
	}
	if(y == 1)
		return genRRR(p, AMOVQ, num, NREG, quot);

	if(num == REGTMP || quot == REGTMP)
		diag("bad register in divuconst");

	tmp = y;
	for(logy = -1; tmp != 0; logy++)
		tmp >>= 1;

	msb = (1LL << (bits-1));
	if((y & (y-1)) == 0)		/* power of 2 */
		return genIRR(p, ASRLQ, logy, num, quot);
	if(y > msb)
		return genIRR(p, ACMPUGE, y, num, quot);

	/* k = (-2^(bits+logy)) % y */
	m = msb/y;
	n = msb%y;
	if(debug['d'])
		Bprint(&bso, "divuconst: y=%lld msb=%lld m=%lld n=%lld\n",
			y, msb, m, n);
	for(i = 0; i <= logy; i++) {
		m *= 2LL;
		n *= 2LL;
		if(n > y) {
			m += 1LL;
			n -= y;
		}
	}
	if(debug['d'])
		Bprint(&bso, "divuconst: y=%lld msb=%lld m=%lld n=%lld\n",
			y, msb, m, n);
	k = y - n;
	if(k > (1LL << logy)) {
		mult = 2LL*m + 1LL;
		bits++;
	} else
		mult = m + 1LL;

	shift = bits + logy;
	if(debug['d'])
		Bprint(&bso, "divuconst: y=%lld mult=%lld shift=%d bits=%d k=%lld\n",
			y, mult, shift, bits, k);
	if(bits <= 32) {
		p = genIRR(p, AMOVQ, mult, NREG, REGTMP);
		p = genRRR(p, AEXTLL, REGZERO, num, quot);
		p = genRRR(p, AMULQ, REGTMP, quot, quot);
		p = genIRR(p, ASRLQ, shift, quot, quot);
		p = genRRR(p, AADDL, quot, REGZERO, quot);
		return p;
	}
	if(bits == 33) {
		if(shift < 64) {
			mult <<= (64-shift);
			shift = 64;
		}
		p = genIRR(p, AMOVQ, mult, NREG, REGTMP);
		p = genRRR(p, AEXTLL, REGZERO, num, quot);
		p = genRRR(p, AUMULH, REGTMP, quot, quot);
		if(shift != 64)
			p = genIRR(p, ASRLQ, shift-64, quot, quot);
		p = genRRR(p, AADDL, quot, REGZERO, quot);
		return p;
	}
	if(bits <= 64) {
		if(shift < 64) {
			mult <<= (64-shift);
			shift = 64;
		}
		p = genIRR(p, AMOVQ, mult, NREG, REGTMP);
		p = genRRR(p, AUMULH, REGTMP, num, quot);
		if(shift != 64)
			p = genIRR(p, ASRLQ, shift-64, quot, quot);
		return p;
	}

	p = genIRR(p, AMOVQ, mult, NREG, REGTMP);
	p = genRRR(p, AUMULH, REGTMP, num, REGTMP);
	p = genRRR(p, AADDQ, num, REGTMP, quot);
	p = genRRR(p, ACMPUGT, REGTMP, quot, REGTMP);
	p = genIRR(p, ASLLQ, 128-shift, REGTMP, REGTMP);
	p = genIRR(p, ASRLQ, shift-64, quot, quot);
	p = genRRR(p, AADDQ, REGTMP, quot, quot);
	return p;
}

Prog *
divconst(Prog *p, vlong y, int num, int quot, int bits)
{
	vlong yabs;
	Prog *q;

	yabs = y;
	if (y < 0)
		yabs = -y;
	q = genRRR(p, ASUBQ, num, REGZERO, REGTMP2);
	if (num != quot)
		q = genRRR(q, AMOVQ, num, NREG, quot);
	q = genRRR(q, ACMOVGT, REGTMP2, REGTMP2, quot);
	q = divuconst(q, yabs, quot, quot, bits-1);
	q = genRRR(q, ASUBQ, quot, REGZERO, REGTMP);
	q = genRRR(q, (y < 0)? ACMOVLT: ACMOVGT, REGTMP, REGTMP2, quot);
	return q;
}

Prog *
modconst(Prog *p, vlong y, int num, int quot, int bits)
{
	vlong yabs;
	Prog *q;

	yabs = y;
	if (y < 0)
		yabs = -y;
	q = genRRR(p, ASUBQ, num, REGZERO, REGTMP2);
	q = genRRR(q, ACMOVLT, num, REGTMP2, REGTMP2);
	q = divuconst(q, yabs, REGTMP2, REGTMP2, bits-1);
	q = genRRR(q, ASUBQ, REGTMP2, REGZERO, REGTMP);
	q = genRRR(q, ACMOVLT, REGTMP, num, REGTMP2);
	q = genIRR(q, AMULQ, yabs, REGTMP2, REGTMP2);
	q = genRRR(q, ASUBQ, REGTMP2, num, quot);
	return q;
}

Prog *
genXXX(Prog *q, int op, Adr *from, int reg, Adr *to)
{
	Prog *p;

	p = prg();
	p->as = op;
	p->line = q->line;
	p->from = *from;
	p->to = *to;
	p->reg = reg;
	p->link = q->link;
	q->link = p;
	return p;
}

Prog *
genRRR(Prog *q, int op, int from, int reg, int to)
{
	Prog *p;

	p = prg();
	p->as = op;
	p->line = q->line;
	p->from.type = D_REG;
	p->from.reg = from;
	p->to.type = D_REG;
	p->to.reg = to;
	p->reg = reg;
	p->link = q->link;
	q->link = p;
	return p;
}

Prog *
genIRR(Prog *q, int op, vlong v, int reg, int to)
{
	Prog *p;

	p = prg();
	p->as = op;
	p->line = q->line;
	p->from.type = D_CONST;
	p->from.offset = v;
	p->to.type = D_REG;
	p->to.reg = to;
	p->reg = reg;
	p->link = q->link;
	q->link = p;
	return p;
}

Prog *
genstore(Prog *q, int op, int from, vlong offset, int to)
{
	Prog *p;

	p = prg();
	p->as = op;
	p->line = q->line;
	p->from.type = D_REG;
	p->from.reg = from;
	p->to.type = D_OREG;
	p->to.reg = to;
	p->to.offset = offset;
	p->reg = NREG;
	p->link = q->link;
	q->link = p;
	return p;
}

Prog *
genload(Prog *q, int op, vlong offset, int from, int to)
{
	Prog *p;

	p = prg();
	p->as = op;
	p->line = q->line;
	p->from.type = D_OREG;
	p->from.offset = offset;
	p->from.reg = from;
	p->to.type = D_REG;
	p->to.reg = to;
	p->reg = NREG;
	p->link = q->link;
	q->link = p;
	return p;
}
