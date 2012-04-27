#include	"l.h"

enum
{
	E_HILO	= 1<<0,
	E_FCR	= 1<<1,
	E_MCR	= 1<<2,
	E_MEM	= 1<<3,
	E_MEMSP	= 1<<4,	/* uses offset and size */
	E_MEMSB	= 1<<5,	/* uses offset and size */
	ANYMEM	= E_MEM|E_MEMSP|E_MEMSB,
	DELAY	= BRANCH|LOAD|FCMP,
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
			Bprint(&bso, "%P\t\tset", &s->p);
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
		if(p == pe)
			break;
		s++;
	}
	se = s;

	/*
	 * prepass to move things around
	 * does nothing, but tries to make
	 * the actual scheduler work better
	 */
	for(s=sch; s<=se; s++) {
		if(!(s->p.mark & LOAD))
			continue;
		/* always good to put nonconflict loads together */
		for(t=s+1; t<=se; t++) {
			if(!(t->p.mark & LOAD))
				continue;
			if(t->p.mark & BRANCH)
				break;
			if(conflict(s, t))
				break;
			for(u=t-1; u>s; u--)
				if(depend(u, t))
					goto no11;
			u = s+1;
			stmp = *t;
			memmove(s+2, u, (uchar*)t - (uchar*)u);
			*u = stmp;
			break;
		}
	no11:

		/* put schedule fodder above load */
		for(t=s+1; t<=se; t++) {
			if(t->p.mark & BRANCH)
				break;
			if(s > sch && conflict(s-1, t))
				continue;
			for(u=t-1; u>=s; u--)
				if(depend(t, u))
					goto no1;
			stmp = *t;
			memmove(s+1, s, (uchar*)t - (uchar*)s);
			*s = stmp;
			if(!(s->p.mark & LOAD))
				break;
		no1:;
		}
	}

	for(s=se; s>=sch; s--) {
		if(!(s->p.mark & DELAY))
			continue;
		if(s < se)
			if(!conflict(s, s+1))
				goto out3;
		/*
		 * s is load, s+1 is immediate use of result or end of block
		 * t is the trial instruction to insert between s and s+1
		 */
		if(!debug['Y'])
		for(t=s-1; t>=sch; t--) {
			if(t->comp)
				if(s->p.mark & BRANCH)
					goto no2;
			if(t->p.mark & DELAY)
				if(s >= se || conflict(t, s+1))
					goto no2;
			for(u=t+1; u<=s; u++)
				if(depend(u, t))
					goto no2;
			goto out2;
		no2:;
		}
		if(debug['X'])
			Bprint(&bso, "?l%P\n", &s->p);
		s->nop = 1;
		if(debug['v']) {
			if(s->p.mark & LOAD) {
				nop.load.count++;
				nop.load.outof++;
			}
			if(s->p.mark & BRANCH) {
				nop.branch.count++;
				nop.branch.outof++;
			}
			if(s->p.mark & FCMP) {
				nop.fcmp.count++;
				nop.fcmp.outof++;
			}
		}
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

	out3:
		if(debug['v']) {
			if(s->p.mark & LOAD)
				nop.load.outof++;
			if(s->p.mark & BRANCH)
				nop.branch.outof++;
			if(s->p.mark & FCMP)
				nop.fcmp.outof++;
		}
	}

	/* Avoid HI/LO use->set */
	t = sch+1;
	for(s=sch; s<se-1; s++, t++) {
		if((s->used.cc & E_HILO) == 0)
			continue;
		if(t->set.cc & E_HILO)
			s->nop = 2;
	}

	/*
	 * put it all back
	 */
	for(s=sch, p=p0; s<=se; s++, p=q) {
		q = p->link;
		if(q != s->p.link) {
			*p = s->p;
			p->link = q;
		}
		while(s->nop--)
			addnop(p);
	}
	if(debug['X']) {
		Bprint(&bso, "\n");
		Bflush(&bso);
	}
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
	sz = 20;	/* size of load/store for overlap computation */

/*
 * flags based on opcode
 */
	switch(p->as) {
	case ATEXT:
		curtext = realp;
		autosize = p->to.offset + 4;
		ad = 1;
		break;
	case AJAL:
		c = p->reg;
		if(c == NREG)
			c = REGLINK;
		s->set.ireg |= 1<<c;
		ar = 1;
		ad = 1;
		break;
	case ABGEZAL:
	case ABLTZAL:
		s->set.ireg |= 1<<REGLINK;
	case ABEQ:
	case ABGEZ:
	case ABGTZ:
	case ABLEZ:
	case ABLTZ:
	case ABNE:
		ar = 1;
		ad = 1;
		break;
	case ABFPT:
	case ABFPF:
		ad = 1;
		s->used.cc |= E_FCR;
		break;
	case ACMPEQD:
	case ACMPEQF:
	case ACMPGED:
	case ACMPGEF:
	case ACMPGTD:
	case ACMPGTF:
		ar = 1;
		s->set.cc |= E_FCR;
		p->mark |= FCMP;
		break;
	case AJMP:
		ar = 1;
		ad = 1;
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
	case AMOVF:
	case AMOVW:
	case AMOVWL:
	case AMOVWR:
		sz = 4;
		ld = 1;
		break;
	case AMOVD:
	case AMOVV:
	case AMOVVL:
	case AMOVVR:
		sz = 8;
		ld = 1;
		break;
	case ADIV:
	case ADIVU:
	case AMUL:
	case AMULU:
	case AREM:
	case AREMU:
		s->set.cc = E_HILO;
	case AADD:
	case AADDU:
	case AAND:
	case ANOR:
	case AOR:
	case ASGT:
	case ASGTU:
	case ASLL:
	case ASRA:
	case ASRL:
	case ASUB:
	case ASUBU:
	case AXOR:

	case AADDD:
	case AADDF:
	case AADDW:
	case ASUBD:
	case ASUBF:
	case ASUBW:
	case AMULF:
	case AMULD:
	case AMULW:
	case ADIVF:
	case ADIVD:
	case ADIVW:
		if(p->reg == NREG) {
			if(p->to.type == D_REG || p->to.type == D_FREG)
				p->reg = p->to.reg;
			if(p->reg == NREG)
				print("botch %P\n", p);
		}
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
	case C_ADD0CON:
	case C_AND0CON:
	case C_ADDCON:
	case C_ANDCON:
	case C_UCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;

	case C_HI:
	case C_LO:
		s->set.cc |= E_HILO;
		break;
	case C_FCREG:
		s->set.cc |= E_FCR;
		break;
	case C_MREG:
		s->set.cc |= E_MCR;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
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
		if(ld && p->from.type == D_REG)
			p->mark |= LOAD;
		break;
	case C_SAUTO:
	case C_LAUTO:
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
	case C_ADD0CON:
	case C_AND0CON:
	case C_ADDCON:
	case C_ANDCON:
	case C_UCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;
	case C_HI:
	case C_LO:
		s->used.cc |= E_HILO;
		break;
	case C_FCREG:
		s->used.cc |= E_FCR;
		break;
	case C_MREG:
		s->used.cc |= E_MCR;
		break;
	case C_ZOREG:
	case C_SOREG:
	case C_LOREG:
		c = p->from.reg;
		s->used.ireg |= 1<<c;
		if(ld)
			p->mark |= LOAD;
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
		if(ld && p->to.type == D_REG)
			p->mark |= LOAD;
		break;
	case C_SAUTO:
	case C_LAUTO:
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
	s->set.ireg &= ~(1<<REGZERO);		/* R0 cant be set */
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

	/*
	 * special case.
	 * loads from same address cannot pass.
	 * this is for hardware fifo's and the like
	 */
	if(sa->used.cc & sb->used.cc & E_MEM)
		if(sa->p.reg == sb->p.reg)
		if(regoff(&sa->p.from) == regoff(&sb->p.from))
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
 */
int
conflict(Sch *sa, Sch *sb)
{

	if(sa->set.ireg & sb->used.ireg)
		return 1;
	if(sa->set.freg & sb->used.freg)
		return 1;
	if(sa->set.cc & sb->used.cc)
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
		case E_HILO:
			Bprint(&bso, " HILO");
			break;
		case E_FCR:
			Bprint(&bso, " FCR");
			break;
		case E_MCR:
			Bprint(&bso, " MCR");
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
