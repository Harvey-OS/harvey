#include	"l.h"

enum
{
	E_MEM	= 1<<3,
	E_MEMSP	= 1<<4,	/* uses offset and size */
	E_MEMSB	= 1<<5,	/* uses offset and size */
	ANYMEM	= E_MEM|E_MEMSP|E_MEMSB,
	DELAY	= BRANCH|LOAD|FCMP,

	NRWORD	= 8,
	isdouble = 0,
};

typedef	struct	Sch	Sch;
typedef	struct	Dep	Dep;

struct	Dep
{
	ulong	ireg[NRWORD];
	ulong	cc;
};
struct	Sch
{
	Prog	p;
	Dep	set;
	Dep	used;
	long	offset;
	char	size;
	char	nop;
	char	comp;
};

void	bitset(ulong*, int);
int	bittest(ulong*, int);

void	regused(Sch*, Prog*);
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
		regused(s, p);
		if(debug['X']) {
			Bprint(&bso, "%P\t\tmark %x set", &s->p, s->p.mark);
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
	if(!debug['Y'])
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
		if(s->p.mark & BRANCH)
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
regused(Sch *s, Prog *realp)
{
	int c, ar, ad, ld, sz;
	ulong m;
	Prog *p;

	p = &s->p;
	s->comp = compound(p);
	s->nop = 0;
	if(s->comp) {
		bitset(s->set.ireg, REGTMP);
		bitset(s->used.ireg, REGTMP);
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
	case ACALL:
		c = p->reg;
		if(c == NREG)
			c = REGLINK;
		bitset(s->set.ireg, c);
		ar = 1;
		ad = 1;
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
	case AMOVL:
		sz = 4;
		ld = 1;
		break;
	case ADIVL:
	case ADIVUL:
	case AMULL:
	case AMULUL:
	case AMULML:
	case AMULMUL:
	case AMSTEPL:
	case AMSTEPUL:
	case AMSTEPLL:
	case ADSTEPL:
	case ADSTEP0L:
	case ADSTEPLL:
	case ADSTEPRL:
		bitset(s->set.ireg, REGQ);
		bitset(s->used.ireg, REGQ);
		break;
	case AMOVD:
		sz = 8;
		ld = 1;
		if(p->from.type == D_REG && p->to.type == D_REG)
			break;
	case ALOADM:
	case ASTOREM:
		bitset(s->set.ireg, REGC);
		bitset(s->used.ireg, REGC);
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
	case C_MCON:
	case C_PCON:
	case C_NCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;

	case C_ZOREG:
	case C_SOREG:
	case C_MOREG:
	case C_POREG:
	case C_NOREG:
	case C_LOREG:
		c = p->to.reg;
		bitset(s->used.ireg, c);
		if(ad)
			break;
		s->size = sz;
		s->offset = regoff(&p->to);

		m = ANYMEM;
		if(c == REGSP)
			m = E_MEMSP;

		if(ar)
			s->used.cc |= m;
		else
			s->set.cc |= m;
		break;

	case C_ZACON:
	case C_SACON:
	case C_MACON:
	case C_PACON:
	case C_NACON:
	case C_LACON:
		bitset(s->used.ireg, REGSP);
		break;

	case C_REG:
		if(ar)
			bitset(s->used.ireg, p->to.reg);
		else
			bitset(s->set.ireg, p->to.reg);
		break;

	case C_ZAUTO:
	case C_SAUTO:
	case C_MAUTO:
	case C_PAUTO:
	case C_NAUTO:
	case C_LAUTO:
		bitset(s->used.ireg, REGSP);
		if(ad)
			break;
		s->size = sz;
		s->offset = regoff(&p->to);

		if(ar)
			s->used.cc |= E_MEMSP;
		else
			s->set.cc |= E_MEMSP;
		break;
	case C_SEXT:
	case C_LEXT:
		if(ad)
			break;
		s->size = sz;
		s->offset = regoff(&p->to);

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
	case C_MCON:
	case C_PCON:
	case C_NCON:
	case C_LCON:
	case C_NONE:
	case C_SBRA:
	case C_LBRA:
		break;

	case C_ZOREG:
	case C_SOREG:
	case C_MOREG:
	case C_POREG:
	case C_NOREG:
	case C_LOREG:
		c = p->from.reg;
		bitset(s->used.ireg, c);
		if(ld)
			p->mark |= LOAD;
		s->size = sz;
		s->offset = regoff(&p->from);

		m = ANYMEM;
		if(c == REGSP)
			m = E_MEMSP;

		s->used.cc |= m;
		break;
	case C_ZACON:
	case C_SACON:
	case C_MACON:
	case C_PACON:
	case C_NACON:
	case C_LACON:
		bitset(s->used.ireg, REGSP);
		break;
	case C_REG:
		bitset(s->used.ireg, p->from.reg);
		if(isdouble)
			bitset(s->used.ireg, p->from.reg+1);
		break;
	case C_ZAUTO:
	case C_SAUTO:
	case C_MAUTO:
	case C_PAUTO:
	case C_NAUTO:
	case C_LAUTO:
		bitset(s->used.ireg, REGSP);
		if(ld)
			p->mark |= LOAD;
		if(ad)
			break;
		s->size = sz;
		s->offset = regoff(&p->from);

		s->used.cc |= E_MEMSP;
		break;
	case C_SEXT:
	case C_LEXT:
		if(ld)
			p->mark |= LOAD;
		if(ad)
			break;
		s->size = sz;
		s->offset = regoff(&p->from);

		s->used.cc |= E_MEMSB;
		break;
	}

	c = p->reg;
	if(c != NREG) {
		if(isdouble) {
			bitset(s->used.ireg, c);
			bitset(s->used.ireg, c+1);
		} else
			bitset(s->used.ireg, c);
	}
}

/*
 * test to see if 2 instrictions can be
 * interchanged without changing semantics
 */
int
depend(Sch *sa, Sch *sb)
{
	ulong x;
	int i;

	for(i=0; i<NRWORD; i++) {
		if(sa->set.ireg[i] & (sb->set.ireg[i]|sb->used.ireg[i]))
			return 1;
		if(sb->set.ireg[i] & sa->used.ireg[i])
			return 1;
	}

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

	if(sa->offset < sb->offset) {
		if(sa->offset+sa->size > sb->offset)
			return 1;
		return 0;
	}
	if(sb->offset+sb->size > sa->offset)
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
	int i;

	for(i=0; i<NRWORD; i++)
		if(sa->set.ireg[i] & sb->used.ireg[i])
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
	return 0;
}

void
dumpbits(Sch *s, Dep *d)
{
	int i;

	for(i=0; i<NRWORD*32; i++)
		if(bittest(d->ireg, i))
			Bprint(&bso, " R%d", i);
	for(i=0; i<32; i++)
		switch(d->cc & (1<<i)) {
		default:
			Bprint(&bso, " CC%d", i);
		case 0:
			break;
		case E_MEM:
			Bprint(&bso, " MEM.%d", s->size);
			break;
		case E_MEMSB:
			Bprint(&bso, " SB.%ld.%d", s->offset, s->size);
			break;
		case E_MEMSP:
			Bprint(&bso, " SP.%ld.%d", s->offset, s->size);
			break;
		}
}

void
bitset(ulong *b, int r)
{

	b[r>>5] |= 1<<(r&31);
}

int
bittest(ulong *b, int r)
{

	if(b[r>>5] & 1<<(r&31))
		return 1;
	return 0;
}
