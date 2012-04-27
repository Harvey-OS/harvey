#include	"l.h"

enum
{
	E_ICC	= 1<<0,
	E_FCC	= 1<<1,
	E_MEM	= 1<<2,
	E_MEMSP	= 1<<3,	/* uses offset and size */
	E_MEMSB	= 1<<4,	/* uses offset and size */
	ANYMEM	= E_MEM|E_MEMSP|E_MEMSB,
	ALL	= ~0
};

typedef	struct	Sch	Sch;
typedef	struct	Dep	Dep;

struct	Dep
{
	ulong	ireg;
	ulong	freg;
	ulong	cc;
};
struct	Sch
{
	Prog	p;
	Dep	set;
	Dep	used;
	long	soffset;
	char	size;
	char	nop;
	char	comp;
};

void	regsused(Sch*, Prog*);
int	depend(Sch*, Sch*);
int	conflict(Sch*, Sch*);
int	offoverlap(Sch*, Sch*);
void	dumpbits(Sch*, Dep*);

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
		memset(s, 0, sizeof(*s));
		s->p = *p;
		regsused(s, p);
		if(debug['X']) {
			Bprint(&bso, "%P\tset", &s->p);
			dumpbits(s, &s->set);
			Bprint(&bso, "; used");
			dumpbits(s, &s->used);
			if(s->comp)
				Bprint(&bso, "; compound");
			if(s->p.mark & LOAD)
				Bprint(&bso, "; load");
			if(s->p.mark & BRANCH)
				Bprint(&bso, "; branch");
			if(s->p.mark & FCMP)
				Bprint(&bso, "; fcmp");
			Bprint(&bso, "\n");
		}
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
					if(depend(u, t))
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
			if(!conflict(s, (s+1)))
				continue;
			/*
			 * s is load, s+1 is immediate use of result
			 * t is the trial instruction to insert between s and s+1
			 */
			for(t=s-1; t>=sch; t--) {
				if(t->p.mark & BRANCH)
					goto no2;
				if(t->p.mark & FCMP)
					if((s+1)->p.mark & BRANCH)
						goto no2;
				if(t->p.mark & LOAD)
					if(conflict(t, (s+1)))
						goto no2;
				for(u=t+1; u<=s; u++)
					if(depend(u, t))
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
					if(depend(u, t))
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

void
regsused(Sch *s, Prog *realp)
{
	int c, ar, ad, ld, sz;
	ulong m;
	Prog *p;

	p = &s->p;
	s->comp = compound(p);
	s->nop = 0;
	if(s->comp) {
		s->set.ireg |= 1<<REGTMP;
		s->used.ireg |= 1<<REGTMP;
	}
	ar = 0;		/* dest is really reference */
	ad = 0;		/* source/dest is really address */
	ld = 0;		/* opcode is load instruction */
	sz = 20;		/* size of load/store for overlap computation */

/*
 * flags based on opcode
 */
	switch(p->as) {
	case ATEXT:
		curtext = realp;
		autosize = p->to.offset + 4;
		ad = 1;
		break;
	case AJMPL:
		c = p->reg;
		if(c == NREG)
			c = REGLINK;
		s->set.ireg |= 1<<c;
		ar = 1;
		ad = 1;
		break;
	case AJMP:
		ar = 1;
		ad = 1;
		break;
	case ACMP:
		s->set.cc |= E_ICC;
		ar = 1;
		break;
	case AFCMPD:
	case AFCMPED:
	case AFCMPEF:
	case AFCMPEX:
	case AFCMPF:
	case AFCMPX:
		s->set.cc |= E_FCC;
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
		s->used.cc |= E_ICC;
		ar = 1;
		break;
	case AFBE:
	case AFBLG:
	case AFBG:
	case AFBLE:
	case AFBGE:
	case AFBL:
		s->used.cc |= E_FCC;
		ar = 1;
		break;
	case AMOVB:
	case AMOVBU:
		sz = 1;
		ld = 1;
		break;
	case AMOVH:
	case AMOVHU:
		sz = 2;
		ld = 1;
		break;
	case AFMOVF:
	case AMOVW:
		sz = 4;
		ld = 1;
		break;
	case AMOVD:
	case AFMOVD:
		sz = 8;
		ld = 1;
		break;
	case AFMOVX:	/* gok */
		sz = 16;
		ld = 1;
		break;
	case AADDCC:
	case AADDXCC:
	case AANDCC:
	case AANDNCC:
	case AORCC:
	case AORNCC:
	case ASUBCC:
	case ASUBXCC:
	case ATADDCC:
	case ATADDCCTV:
	case ATSUBCC:
	case ATSUBCCTV:
	case AXNORCC:
	case AXORCC:
		s->set.cc |= E_ICC;
		break;
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case AMUL:
	case AMULSCC:
	case ATAS:
		s->set.ireg = ALL;
		s->set.freg = ALL;
		s->set.cc = ALL;
		break;
	}

/*
 * flags based on 'to' field
 */
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
		s->set.ireg = ALL;
		s->set.freg = ALL;
		s->set.cc = ALL;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
	case C_ASI:
		c = p->to.reg;
		s->used.ireg |= 1<<c;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->to);

		m = ANYMEM;
		if(c == REGSB)
			m = E_MEMSB;
		if(c == REGSP)
			m = E_MEMSP;

		if(ar)
			s->used.cc |= m;
		else
			s->set.cc |= m;
		break;
	case C_SACON:
	case C_LACON:
		s->used.ireg |= 1<<REGSP;
		break;
	case C_SECON:
	case C_LECON:
		s->used.ireg |= 1<<REGSB;
		break;
	case C_REG:
		if(ar)
			s->used.ireg |= 1<<p->to.reg;
		else
			s->set.ireg |= 1<<p->to.reg;
		break;
	case C_FREG:
		/* do better -- determine double prec */
		if(ar) {
			s->used.freg |= 1<<p->to.reg;
			s->used.freg |= 1<<(p->to.reg|1);
		} else {
			s->set.freg |= 1<<p->to.reg;
			s->set.freg |= 1<<(p->to.reg|1);
		}
		break;
	case C_SAUTO:
	case C_LAUTO:
	case C_ESAUTO:
	case C_OSAUTO:
	case C_ELAUTO:
	case C_OLAUTO:
		s->used.ireg |= 1<<REGSP;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->to);

		if(ar)
			s->used.cc |= E_MEMSP;
		else
			s->set.cc |= E_MEMSP;
		break;
	case C_SEXT:
	case C_LEXT:
	case C_ESEXT:
	case C_OSEXT:
	case C_ELEXT:
	case C_OLEXT:
		s->used.ireg |= 1<<REGSB;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->to);

		if(ar)
			s->used.cc |= E_MEMSB;
		else
			s->set.cc |= E_MEMSB;
		break;
	}

/*
 * flags based on 'from' field
 */
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
		c = p->from.reg;
		if(c != NREG)
			s->used.ireg |= 1<<c;
		break;
	case C_CREG:
	case C_FSR:
	case C_FQ:
	case C_PREG:
		s->set.ireg = ALL;
		s->set.freg = ALL;
		s->set.cc = ALL;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
	case C_ASI:
		c = p->from.reg;
		s->used.ireg |= 1<<c;
		if(ld)
			p->mark |= LOAD;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->from);

		m = ANYMEM;
		if(c == REGSB)
			m = E_MEMSB;
		if(c == REGSP)
			m = E_MEMSP;

		s->used.cc |= m;
		break;
	case C_SACON:
	case C_LACON:
		s->used.ireg |= 1<<REGSP;
		break;
	case C_SECON:
	case C_LECON:
		s->used.ireg |= 1<<REGSB;
		break;
	case C_REG:
		s->used.ireg |= 1<<p->from.reg;
		break;
	case C_FREG:
		/* do better -- determine double prec */
		s->used.freg |= 1<<p->from.reg;
		s->used.freg |= 1<<(p->from.reg|1);
		break;
	case C_SAUTO:
	case C_LAUTO:
	case C_ESAUTO:
	case C_ELAUTO:
	case C_OSAUTO:
	case C_OLAUTO:
		s->used.ireg |= 1<<REGSP;
		if(ld)
			p->mark |= LOAD;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->from);

		s->used.cc |= E_MEMSP;
		break;
	case C_SEXT:
	case C_LEXT:
	case C_ESEXT:
	case C_ELEXT:
	case C_OSEXT:
	case C_OLEXT:
		s->used.ireg |= 1<<REGSB;
		if(ld)
			p->mark |= LOAD;
		if(ad)
			break;
		s->size = sz;
		s->soffset = regoff(&p->from);

		s->used.cc |= E_MEMSB;
		break;
	}
	
	c = p->reg;
	if(c != NREG) {
		if(p->from.type == D_FREG || p->to.type == D_FREG) {
			s->used.freg |= 1<<c;
			s->used.freg |= 1<<(c|1);
		} else
			s->used.ireg |= 1<<c;
	}
	s->set.ireg &= ~(1<<0);		/* R0 cant be set */
}

/*
 * test to see if 2 instrictions can be
 * interchanged without changing semantics
 */
int
depend(Sch *sa, Sch *sb)
{
	ulong x;

	if(sa->set.ireg & (sb->set.ireg|sb->used.ireg))
		return 1;
	if(sb->set.ireg & sa->used.ireg)
		return 1;

	if(sa->set.freg & (sb->set.freg|sb->used.freg))
		return 1;
	if(sb->set.freg & sa->used.freg)
		return 1;

	x = (sa->set.cc & (sb->set.cc|sb->used.cc)) |
		(sb->set.cc & sa->used.cc);
	if(x) {
		/*
		 * allow SB and SP to pass each other.
		 * allow SB to pass SB iff doffsets are ok
		 * anything else conflicts
		 */
		if(x != E_MEMSP && x != E_MEMSB)
			return 1;
		x = sa->set.cc | sb->set.cc |
			sa->used.cc | sb->used.cc;
		if(x & E_MEM)
			return 1;
		if(offoverlap(sa, sb))
			return 1;
	}

	return 0; 
}

int
offoverlap(Sch *sa, Sch *sb)
{

	if(sa->soffset < sb->soffset) {
		if(sa->soffset+sa->size > sb->soffset)
			return 1;
		return 0;
	}
	if(sb->soffset+sb->size > sa->soffset)
		return 1;
	return 0;
}

/*
 * test 2 adjacent instructions
 * and find out if inserted instructions
 * are desired to prevent stalls.
 * first instruction is a load instruction.
 */
int
conflict(Sch *sa, Sch *sb)
{

	if(sa->set.ireg & sb->used.ireg)
		return 1;
	if(sa->set.freg & sb->used.freg)
		return 1;
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
dumpbits(Sch *s, Dep *d)
{
	int i;

	for(i=0; i<32; i++)
		if(d->ireg & (1<<i))
			Bprint(&bso, " R%d", i);
	for(i=0; i<32; i++)
		if(d->freg & (1<<i))
			Bprint(&bso, " F%d", i);
	for(i=0; i<32; i++)
		switch(d->cc & (1<<i)) {
		default:
			break;
		case E_ICC:
			Bprint(&bso, " ICC");
			break;
		case E_FCC:
			Bprint(&bso, " FCC");
			break;
		case E_MEM:
			Bprint(&bso, " MEM%d", s->size);
			break;
		case E_MEMSB:
			Bprint(&bso, " SB%d", s->size);
			break;
		case E_MEMSP:
			Bprint(&bso, " SP%d", s->size);
			break;
		}
}
