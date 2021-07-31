#include	"l.h"


/*
 *	4.170		start
 *	4.103		copy/invert branches
 *	3.903		annul branches
 *	3.865		jmpl/bra delay copy
 *	3.774		load+fcmp
 *	3.476		REGSB double aligned
 */

enum
{
	E_REG	= 0,
	E_FREG	= E_REG+32,

	E_CC	= 1,
	E_FCC,
	E_MEM,
	E_PREG,

	A_SET	= 0,
	A_USE1	= 8,
	A_USE2	= 16,
	A_SETCC	= 24,
	A_USECC	= 28,
};

typedef	struct	Sch	Sch;
struct	Sch
{
	Prog	p;
	long	dep;
	char	nop;
	char	comp;
};

long	regused(Prog*);
int	depend(long, long);
int	conflict(long, long);

void
sched(Prog *p0, Prog *pe)
{
	Prog *p, *q;
	Sch sch[NSCHED], *s, *t, *u, *se, stmp;

	/*
	 * build side structure
	 */
	s = sch;
	for(p=p0;; p=p->link) {
		s->dep = regused(p);
		s->comp = compound(p);
		s->nop = 0;
		s->p = *p;
		if(debug['X'])
			Bprint(&bso, "%.8ux%P\n", s->dep, &s->p);
		s++;
		if(p == pe)
			break;
	}
	se = s;

	for(s=se-1; s>=sch; s--) {
		/*
		 * branch delay slot
		 */
		if(s->p.mark & BRANCH) {
			/* t is the trial instruction to use */
			for(t=s-1; t>=sch; t--) {
				if(t->comp || (t->p.mark & FCMP))
					goto no1;
				for(u=t+1; u<=s; u++)
					if(depend(u->dep, t->dep))
						goto no1;
				goto out1;
			no1:;
			}
			if(debug['X'])
				Bprint(&bso, "?b%P\n", &s->p);
			s->nop = 1;
			continue;

		out1:
			if(debug['X']) {
				Bprint(&bso, "!b%P\n", &t->p);
				Bprint(&bso, "%P\n", &s->p);
			}
			stmp = *t;
			memmove(t, t+1, (uchar*)s - (uchar*)t);
			*s = stmp;
			s--;
			continue;
		}

		/*
		 * load delay. interlocked.
		 */
		if(s->p.mark & LOAD) {
			if(s >= se-1)
				continue;
			if(!conflict(s->dep, (s+1)->dep))
				continue;
			/* t is the trial instruction to use */
			for(t=s-1; t>=sch; t--) {
				if(t->p.mark & BRANCH)
					goto no2;
				if(t->p.mark & FCMP)
					if((s+1)->p.mark & BRANCH)
						goto no2;
				if(t->p.mark & LOAD)
					if(conflict(t->dep, (s+1)->dep))
						goto no2;
				for(u=t+1; u<=s; u++)
					if(depend(u->dep, t->dep))
						goto no2;
				goto out2;
			no2:;
			}
			if(debug['X'])
				Bprint(&bso, "?l%P\n", &s->p);
			continue;
		out2:
			if(debug['X']) {
				Bprint(&bso, "!l%P\n", &t->p);
				Bprint(&bso, "%P\n", &s->p);
			}
			stmp = *t;
			memmove(t, t+1, (uchar*)s - (uchar*)t);
			*s = stmp;
			s--;
			continue;
		}

		/*
		 * fop2 delay.
		 */
		if(s->p.mark & FCMP) {
			if(s >= se-1) {
				s->nop = 1;
				continue;
			}
			if(!((s+1)->p.mark & BRANCH))
				continue;
			/* t is the trial instruction to use */
			for(t=s-1; t>=sch; t--) {
				for(u=t+1; u<=s; u++)
					if(depend(u->dep, t->dep))
						goto no3;
				goto out3;
			no3:;
			}
			if(debug['X'])
				Bprint(&bso, "?f%P\n", &s->p);
			s->nop = 1;
			continue;
		out3:
			if(debug['X']) {
				Bprint(&bso, "!f%P\n", &t->p);
				Bprint(&bso, "%P\n", &s->p);
			}
			stmp = *t;
			memmove(t, t+1, (uchar*)s - (uchar*)t);
			*s = stmp;
			s--;
			continue;
		}
	}

	/*
	 * put it all back
	 */
	for(s=sch, p=p0; s<se; s++, p=q) {
		q = p->link;
		if(q != s->p.link) {
			*p = s->p;
			p->link = q;
		}
		if(s->nop)
			addnop(p);
	}
	if(debug['X'])
		Bprint(&bso, "\n");
}

long
regused(Prog *p)
{
	long r;
	int c, ar, ad, ld;

	r = 0;
	ar = 0;		/* dest is really reference */
	ad = 0;		/* dest is really ardress */
	ld = 0;		/* opcode is load instruction */

	switch(p->as) {
	case ATEXT:
		curtext = p;
		autosize = p->to.offset + 4;
		break;
	case AJMPL:
		c = p->reg;
		if(c == NREG)
			c = REGLINK;
		r |= (E_REG+c) << A_SET;
		ar = 1;
		break;
	case AJMP:
		ar = 1;
		ad = 1;
		break;
	case ACMP:
		r |= E_CC << A_SETCC;
		ar = 1;
		ad = 1;
		break;
	case AFCMPD:
	case AFCMPED:
	case AFCMPEF:
	case AFCMPEX:
	case AFCMPF:
	case AFCMPX:
		r |= E_FCC << A_SETCC;
		ar = 1;
		break;
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
		r |= E_CC << A_USECC;
		ar = 1;
		break;
	case AFBE:
	case AFBLG:
	case AFBG:
	case AFBLE:
	case AFBGE:
	case AFBL:
		r |= E_FCC << A_USECC;
		ar = 1;
		break;

	case AFMOVD:
	case AFMOVF:
	case AFMOVX:
	case AMOVB:
	case AMOVBU:
	case AMOVD:
	case AMOVH:
	case AMOVHU:
	case AMOVW:
		ld = 1;
		break;
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case AMUL:

	case AADDCC:
	case AADDXCC:
	case AANDCC:
	case AANDNCC:
	case AMULSCC:
	case AORCC:
	case AORNCC:
	case ASUBCC:
	case ASUBXCC:
	case ATADDCC:
	case ATCC:
	case ATSUBCC:
	case AXNORCC:
	case AXORCC:

	case ATADDCCTV:
	case ATSUBCCTV:
		r |= E_CC << A_SETCC;
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
		print("unknown class %d %D\n", c, &p->to);

	case C_ZCON:
	case C_SCON:
	case C_UCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;
	case C_CREG:
	case C_FSR:
	case C_FQ:
	case C_PREG:
		r |= E_CC << A_SETCC;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
	case C_ASI:
		if(ar) {
			if(!ad)
				r |= E_MEM << A_USECC;
			r |= (E_REG+p->to.reg) << A_USE1;
		} else
			r |= (E_MEM << A_SETCC) |
				((E_REG+p->to.reg) << A_USE1);
		break;
	case C_SACON:
	case C_LACON:
		r |= (E_REG+REGSP) << A_USE1;
		break;
	case C_SECON:
	case C_LECON:
		r |= (E_REG+REGSB) << A_USE1;
		break;
	case C_REG:
		if(ar)
			r |= (E_REG+p->to.reg) << A_USE1;
		else
			r |= (E_REG+p->to.reg) << A_SET;
		break;
	case C_FREG:
		if(ar)
			r |= (E_FREG+(p->to.reg&~1)) << A_USE1;
		else
			r |= (E_FREG+(p->to.reg&~1)) << A_SET;
		break;
	case C_SAUTO:
	case C_LAUTO:
	case C_ESAUTO:
	case C_OSAUTO:
	case C_ELAUTO:
	case C_OLAUTO:
		if(ar) {
			if(!ad)
				r |= E_MEM << A_USECC;
			r |= (E_REG+REGSP) << A_USE1;
		} else
			r |= (E_MEM << A_SETCC) |
				((E_REG+REGSP) << A_USE1);
		break;
	case C_SEXT:
	case C_LEXT:
	case C_ESEXT:
	case C_OSEXT:
	case C_ELEXT:
	case C_OLEXT:
		if(ar) {
			if(!ad)
				r |= E_MEM << A_USECC;
			r |= (E_REG+REGSB) << A_USE1;
		} else
			r |= (E_MEM << A_SETCC) |
				((E_REG+REGSB) << A_USE1);
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
		print("unknown class %d %D\n", c, &p->from);

	case C_ZCON:
	case C_SCON:
	case C_UCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;
	case C_CREG:
	case C_FSR:
	case C_FQ:
	case C_PREG:
		r |= E_CC << A_SETCC;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
	case C_ASI:
		r |= (E_MEM << A_USECC) |
			((E_REG+p->from.reg) << A_USE2);
		if(ld)
			p->mark |= LOAD;
		break;
	case C_SACON:
	case C_LACON:
		r |= (E_REG+REGSP) << A_USE2;
		break;
	case C_SECON:
	case C_LECON:
		r |= (E_REG+REGSB) << A_USE2;
		break;
	case C_REG:
		r |= (E_REG+p->from.reg) << A_USE2;
		break;
	case C_FREG:
		r |= (E_FREG+(p->from.reg&~1)) << A_USE2;
		break;
	case C_SAUTO:
	case C_LAUTO:
	case C_ESAUTO:
	case C_ELAUTO:
	case C_OSAUTO:
	case C_OLAUTO:
		r |= (E_MEM << A_USECC) |
			((E_REG+REGSP) << A_USE2);
		if(ld)
			p->mark |= LOAD;
		break;
	case C_SEXT:
	case C_LEXT:
	case C_ESEXT:
	case C_ELEXT:
	case C_OSEXT:
	case C_OLEXT:
		r |= (E_MEM << A_USECC) |
			((E_REG+REGSB) << A_USE2);
		if(ld)
			p->mark |= LOAD;
		break;
	}
	
	c = p->reg;
	if(c != NREG) {
		c += E_REG;
		if(p->from.type == D_FREG || p->to.type == D_FREG)
			c += E_FREG-E_REG;
		if(!(r & (0xff<<A_USE1)))
			r |= c << A_USE1;
		else
		if(!(r & (0xff<<A_USE2)))
			r |= c << A_USE2;
		else
		if(!(r & (0xff<<A_SET)))
			r |= c << A_SET;	/* case of R1,(R2+R3) */
		else
			print("no room %P %x\n", p, r);
	}

	return r;
}

int
depend(long a, long b)
{
	int ra, rb;

	ra = (a >> A_SET) & 0xff;
	rb = (b >> A_SET) & 0xff;

	if(ra) {
		if(ra == rb)
			return 1;
		if(ra == ((b>>A_USE1)&0xff))
			return 1;
		if(ra == ((b>>A_USE2)&0xff))
			return 1;
	}
	if(rb) {
		if(rb == ((a>>A_USE1)&0xff))
			return 1;
		if(rb == ((a>>A_USE2)&0xff))
			return 1;
	}

	ra = (a >> A_SETCC) & 0xf;
	rb = (b >> A_SETCC) & 0xf;

	if(ra) {
		if(ra == rb)
			return 1;
		if(ra == ((b>>A_USECC)&0xf))
			return 1;
	}
	if(rb)
		if(rb == ((a>>A_USECC)&0xf))
			return 1;

	return 0;
}

int
conflict(long a, long b)
{
	int ra;

	ra = (a >> A_SET) & 0xff;
	if(ra) {
		if(ra == ((b>>A_USE1)&0xff))
			return 1;
		if(ra == ((b>>A_USE2)&0xff))
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
