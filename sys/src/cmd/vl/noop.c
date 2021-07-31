#include	"l.h"

int	nbrnoop;
int	nldnoop;
int	ncmnoop;
int	nmfnoop;
int	nbrtot;
int	nldtot;
int	ncmtot;
int	nmagex;
int	nmfrom;

void
noops(void)
{
	Prog *p, *q, *q1, *m;
	int f;

	/*
	 * 1. find leaf subroutines
	 * 2. strip NOPs
	 * 3. mark instruction that might need delays
	 * 4. expand RET pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);
	nbrnoop = 0;
	nbrtot = 0;
	nldnoop = 0;
	nldtot = 0;
	ncmnoop = 0;
	ncmtot = 0;
	nmagex = 0;
	nmfrom = 0;
	nmfnoop = 0;

	q = P;
	for(p = firstp; p != P; p = p->link) {

		switch(p->as) {
		case ATEXT:
			q = p;
			p->mark |= LEAF;
			if(p->link)
				p->link->mark |= LABEL;
			curtext = p;
			continue;

		/* too hard, just leave alone */
		case AMOVW:
			if(p->to.type != D_FCREG &&
			   p->to.type != D_MREG) {
				q = p;
				continue;
			}

		/* too hard, just leave alone */
		case ASYSCALL:
		case AWORD:
		case ATLBWR:
		case ATLBWI:
		case ATLBP:
		case ATLBR:
			q = p;
			p->mark |= LABEL;
			for(q1=p->link; q1 != P; q1 = q1->link) {
				q1->mark |= LABEL;
				if(q1->as != ANOR)
					break;
			}
			continue;

		case ARET:
			q = p;
			if(p->link != P)
				p->link->mark |= LABEL;
			continue;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case ABGEZAL:
		case ABLTZAL:
		case AJAL:
			if(curtext != P)
				curtext->mark &= ~LEAF;
		case ABEQ:
		case ABGEZ:
		case ABGTZ:
		case ABLEZ:
		case ABLTZ:
		case ABNE:
		case ABFPT:
		case ABFPF:
		case AJMP:
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

		default:
			q = p;
			continue;
		}
	}

	m = P;
	f = 0;
	for(q = firstp; q != P; q = p->link) {
		if(q->mark & LABEL) {
			if(f)
				sched(m, p);
			m = q;
			f = 0;
		}
		p = q;
	loop:
		switch(p->as) {
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

		case ARET:
			nocache(p);
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AJMP;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.offset = 0;
					p->to.reg = REGLINK;
					goto loop;
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

				q->link = p->link;
				p->link = q;
				goto loop;
			}
			p->as = AMOVW;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = 2;

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
			q1->to.reg = 2;

			q1->link = q->link;
			q->link = q1;
			goto loop;

		case ACMPEQF:
		case ACMPEQD:
		case ACMPGTF:
		case ACMPGTD:
		case ACMPGEF:
		case ACMPGED:
			f++;
			p->mark |= COMPARE;
			break;

		case AMOVB:
		case AMOVBU:
		case AMOVH:
		case AMOVHU:
		case AMOVW:
		case AMOVF:
		case AMOVD:
			if(p->from.type == D_HI || p->from.type == D_LO) {
				p->mark |= MFROM;
				f++;
				break;
			}
			if(p->from.type == D_OREG)
				goto tst;
			if(p->to.type == D_FREG) {
				if(p->from.type == D_FCONST)
					goto tst;
				if(p->from.type == D_CONST)
					goto tst;
				if(p->from.type == D_REG)
					goto tst;
			}
			if(p->to.type == D_REG) {
				if(p->from.type == D_FREG)
					goto tst;
				if(p->from.type == D_FCREG)
					goto tst;
				if(p->from.type == D_MREG)
					goto tst;
			}
			break;
		tst:
			f++;
			p->mark |= LOAD;
			break;

		case ABEQ:
		case ABGEZ:
		case ABGTZ:
		case ABLEZ:
		case ABLTZ:
		case ABNE:
		case ABFPT:
		case ABFPF:
		case AJMP:
		case ABGEZAL:
		case ABLTZAL:
		case AJAL:
			p->mark |= BRANCH;
			sched(m, p);
			f = 0;
			break;
		}
	}
	if(debug['v'])
	Bprint(&bso, "br=%d/%d; ld=%d/%d; cm=%d/%d; mf=%d/%d; ex=%d\n",
		nbrnoop, nbrtot,
		nldnoop, nldtot,
		ncmnoop, ncmtot,
		nmfnoop, nmfrom,
		nmagex);
	Bflush(&bso);
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}

void
sched(Prog *p0, Prog *p1)
{
	Prog *p, *q, *t;
	int r, a;

	if(debug['X'])
		Bprint(&bso, "\n");
	for(p=p0;; p=p->link) {
		/*
		 * magic optimization #1
		 * add $c1,r1; sgt $c2,r1,r2
		 * 	is converted to
		 * sgt $c2-c1,r1,r2; add $c1,r1
		 */
		if(p->as == AADDU)
		if(p != p1)
		if(p->from.type == D_CONST)
		if(p->from.offset >= 0)
		if(p->to.type == D_REG) {
			q = p->link;
			a = q->as;
			r = p->to.reg;
			if(p->reg == r || p->reg == NREG)
			if(a == ASGT)
			if(q->from.type == D_CONST)
			if(q->from.offset >= 0)
			if(q->from.offset-p->from.offset >= 0)
			if(q->reg == r)
			if(q->to.type == D_REG)
			if(q->to.reg != r) {
				nocache(q);
				q->from.offset -= p->from.offset;
				nmagex++;
				exchange(p);
			}
		}
		/*
		 * magic optimization #2
		 * sub r1,r2,r3; bne r3,place
		 * 	is converted to
		 * sub r1,r2,r3; bne r1,r2,place
		 */
		if(p->as == ASUBU)
		if(p != p1)
		if(p->from.type == D_REG)
		if(p->to.type == D_REG)
		if(p->to.reg != p->from.reg)
		if(p->to.reg != p->reg)
		if(p->reg != NREG) {
			q = p->link;
			if(q->as == ABNE || q->as == ABEQ)
			if(q->from.type == D_REG)
			if(q->from.reg == p->to.reg)
			if(q->reg == NREG) {
				nocache(q);
				q->from.reg = p->from.reg;
				q->reg = p->reg;
				nmagex++;
			}
		}

		p->regused = regused(p);
		p->mark |= ACTIVE1|ACTIVE2;
		if(compound(p))
			p->mark &= ~(ACTIVE1|ACTIVE2);
		if(debug['X'])
			Bprint(&bso, "%2x	%.8lux%P\n", p->mark, p->regused, p);
		for(q=p0; q!=p; q=q->link)
			if(depend(q->regused, p->regused))
				q->mark &= ~ACTIVE1;
		if(p->mark & BRANCH) {
			nbrtot++;
			if(!debug['C'])
			for(q=p0; q!=p; q=q->link) {
				if(q->mark & (BRANCH|LOAD|COMPARE))
					continue;
				if(!(q->mark & ACTIVE1))
					continue;
				if(debug['X'])
					Bprint(&bso, "\t\t\t!!!%P\n", q);
				append(q, p);
				goto cont1;
			}
			addnop(p);
			nbrnoop++;
			goto cont1;
		}
	cont1:
		if(p == p1)
			break;
	}

	for(p=p0;; p=p->link) {
		for(q=p0; q!=p; q=q->link)
			if(depend(q->regused, p->regused))
				q->mark &= ~ACTIVE2;
		if(p->mark & MFROM) {
			nmfrom++;
			p->mark &= ~ACTIVE2;
			if(p == p1) {
				addnop(p);
				addnop(p);
				nmfnoop += 2;
				goto cont;
			}

			q = p->link;
			switch(q->as) {
			case AMOVW:
				if(q->to.type != p->from.type)
					break;
			case AMUL:
			case AMULU:
			case ADIV:
			case ADIVU:
				addnop(p);
				addnop(p);
				nmfnoop += 2;
				goto cont;
			}
			q->mark &= ~ACTIVE2;

			if(q == p1) {
				addnop(p);
				nmfnoop++;
				goto cont;
			}
			q = q->link;
			switch(q->as) {
			case AMOVW:
				if(q->to.type != p->from.type)
					break;
			case AMUL:
			case AMULU:
			case ADIV:
			case ADIVU:
				addnop(p);
				nmfnoop++;
				goto cont;
			}
			q->mark &= ~ACTIVE2;
		}
		if(p->mark & BRANCH)
			break;
		if(p->mark & COMPARE) {	/* FCMP to BFPF */
			ncmtot++;
			p->mark &= ~ACTIVE2;
			if(p == p1) {
				ncmnoop++;
				goto nop;
			}
			q = p->link;
			if(q->as == ABFPF || q->as == ABFPT) {
				ncmnoop++;
				goto nop;
			}
			q->mark &= ~ACTIVE2;
			goto cont;
		}
		if(p->mark & LOAD) {	/* mem load to use */
			p->mark &= ~ACTIVE2;
			nldtot++;
			if(debug['C']) {
				q = p->link;
				if(p == p1 || conflict(p->regused, q->regused)) {
					nldnoop++;
					goto nop;
				}
				q->mark &= ~ACTIVE2;
				goto cont;
			}
			for(q=p0; q!=p; q=q->link) {
				if(q->mark & (BRANCH|LOAD|COMPARE))
					continue;
				if(!(q->mark & ACTIVE2))
					continue;
				if(debug['Y']) {
					Bprint(&bso, "%P !!!\n", q);
					for(t=q->link; t!=p; t=t->link)
						Bprint(&bso, "%P\n", t);
					Bprint(&bso, "%P\n\n", p);
				}
				q->mark &= ~ACTIVE2;
				append(q, p);
				goto cont;
			}
			for(q=p->link; q!=p1->link; q=q->link) {
				if(q->mark & BRANCH) {
					if(conflict(p->regused, q->regused))
						break;
					if(q != p->link)
						break;
					q->mark &= ~ACTIVE2;
					goto cont;
				}
				if(q != p->link && !(q->mark & ACTIVE2))
					continue;
				if(conflict(p->regused, q->regused))
					continue;
				for(t=p->link; t!=q; t=t->link)
					if(depend(t->regused, q->regused))
						goto brk;
				t = p->link;
				if(t != q)
					prepend(t, q);
				t->mark &= ~ACTIVE2;
				goto cont;
			brk:;
			} 
			nldnoop++;
			goto nop;
		}
	cont:
		if(p == p1)
			break;
		continue;
	nop:
		addnop(p);
		if(p == p1)
			break;
		p = p->link;
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

#define	E_REG	(0)
#define	E_FREG	(E_REG+32)
#define	E_MEM	(E_FREG+32)
#define	E_HILO	(E_MEM+1)	/* cpu hi+lo registers */

int
depend(long a, long b)
{
	int ra, rb;

	ra = a & 0xff;
	rb = b & 0xff;

	if(ra) {
		if(rb) {
			if(ra == rb)
				return 1;
			if(ra == ((b>>8)&0xff))
				return 1;
			if(rb == ((a>>8)&0xff))
				return 1;
			if(ra == ((b>>16)&0xff))
				return 1;
			if(rb == ((a>>16)&0xff))
				return 1;
			if(ra == ((b>>24)&0xff))
				return 1;
			if(rb == ((a>>24)&0xff))
				return 1;
			return 0;
		}
		if(ra == ((b>>8)&0xff))
			return 1;
		if(ra == ((b>>16)&0xff))
			return 1;
		if(ra == ((b>>24)&0xff))
			return 1;
		return 0;
	}
	if(rb) {
		if(rb == ((a>>8)&0xff))
			return 1;
		if(rb == ((a>>16)&0xff))
			return 1;
		if(rb == ((a>>24)&0xff))
			return 1;
		return 0;
	}
	return 0;
}

int
conflict(long a, long b)
{

	a &= 0xff;
	if(a) {
		if(a == (b&0xff))
			return 1;
		if(a == ((b>>8)&0xff))
			return 1;
		if(a == ((b>>16)&0xff))
			return 1;
		if(a == ((b>>24)&0xff))
			return 1;
	}
	return 0;
}

int
compound(Prog *p)
{
	Optab *o;

	o = oplook(p);
	if(o->size != 4)
		return 1;
	if(p->to.type == D_REG && p->to.reg == REGSB)
		return 1;
	return 0;
}

void
prepend(Prog *p, Prog *q)
{

	if(p != q) {
		prepend(p->link, q);
		exchange(p);
	}
}

void
append(Prog *p, Prog *q)
{
	int m;

	m = p->mark & (LABEL|LEAF);
	if(m) {
		p->mark &= m;
		p->link->mark |= m;
	}
	for(;;) {
		exchange(p);
		p = p->link;
		if(p == q)
			break;
	}
}

void
exchange(Prog *p)
{
	Prog g, *q;

	q = p->link;
	g = *p;
	*p = *q;
	*q = g;
	q->link = p->link;
	p->link = q;
}

long
regused(Prog *p)
{
	long r;
	int c, a;

	r = 0;
	a = p->as;
	switch(a) {
	case AMUL:
	case AMULU:
	case ADIV:
	case ADIVU:
		r |= E_HILO << 0;
		break;
	}
	c = p->to.class;
	if(c == 0) {
		c = aclass(&p->to) + 1;
		p->to.class = c;
	}
	c--;
	switch(c) {
	default:
		print("aclass to %d%P\n", c, p);
		break;
	case C_NONE:
	case C_ZCON:
	case C_SCON:
	case C_ADD0CON:
	case C_AND0CON:
	case C_ADDCON:
	case C_ANDCON:
	case C_UCON:
	case C_LCON:
	case C_MREG:
	case C_FCREG:
		break;
	case C_SBRA:
	case C_LBRA:
		if(a == AJAL)
			r |= (E_REG+REGLINK) << 0;
		break;
	case C_REG:
		r |= (E_REG+p->to.reg) << 0;
		break;
	case C_FREG:
		r |= (E_FREG+p->to.reg) << 0;
		break;
	case C_HI:
	case C_LO:
		r |= E_HILO << 0;
		break;
	case C_SACON:
	case C_LACON:
		r |= (E_REG+REGSP) << 8;
		break;
	case C_SAUTO:
	case C_LAUTO:
		r |= (E_MEM << 0) | ((E_REG+REGSP) << 8);
		break;
	case C_SECON:
	case C_LECON:
		r |= (E_REG+REGSB) << 8;
		break;
	case C_SEXT:
	case C_LEXT:
		r |= (E_MEM << 0) | ((E_REG+REGSB) << 8);
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
		if(p->as == AJMP)
			r |= (E_REG+p->to.reg) << 8;
		else
		if(p->as == AJAL)
			r |= ((E_REG+REGLINK) << 0) | ((E_REG+p->to.reg) << 8);
		else
			r |= (E_MEM << 0) | ((E_REG+p->to.reg) << 8);
		break;
	}

	c = p->from.class;
	if(c == 0) {
		c = aclass(&p->from) + 1;
		p->from.class = c;
	}
	c--;
	switch(c) {
	default:
		print("aclass fr %d%P\n", c, p);
	case C_NONE:
	case C_ZCON:
	case C_SCON:
	case C_ADD0CON:
	case C_AND0CON:
	case C_ADDCON:
	case C_ANDCON:
	case C_UCON:
	case C_LCON:
	case C_SBRA:
	case C_LBRA:
	case C_MREG:
	case C_FCREG:
		break;
	case C_REG:
		r |= (E_REG+p->from.reg) << 16;
		break;
	case C_FREG:
		r |= (E_FREG+p->from.reg) << 16;
		break;
	case C_HI:
	case C_LO:
		r |= E_HILO << 16;
		break;
	case C_SACON:
	case C_LACON:
		r |= (E_REG+REGSP) << 16;
		break;
	case C_SAUTO:
	case C_LAUTO:
		r |= (E_MEM << 24) | ((E_REG+REGSP) << 16);
		break;
	case C_SECON:
	case C_LECON:
		r |= (E_REG+REGSB) << 16;
		break;
	case C_SEXT:
	case C_LEXT:
		r |= (E_MEM << 24) | ((E_REG+REGSB) << 16);
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
		r |= (E_MEM << 24) | ((E_REG+p->from.reg) << 16);
		break;
	}
	c = p->reg;
	if(c != NREG) {
		c += E_REG;
		if(p->from.type == D_FREG || p->to.type == D_FREG)
			c += E_FREG-E_REG;
		if(!(r & (0xff<<8)))
			r |= c << 8;
		else
		if(!(r & (0xff<<16)))
			r |= c << 16;
		else
		if(!(r & (0xff<<24)))
			r |= c << 24;
		else
			print("no room %P %x\n", p, r);
	}
	return r;
}
