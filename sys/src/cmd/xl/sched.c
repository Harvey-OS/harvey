#include "l.h"

enum
{
	Now,			/* different phases of instruction */
	Load,
	Mul,
	Acc,
	Store,
	Ncycle	= 5,		/* max cycles till done */
};

enum
{
	E_ICC	= 1<<0,
	E_FCC	= 1<<1,
	E_OCC	= 1<<2,		/* io condition codes */
	E_CC	= E_ICC|E_FCC|E_OCC,
	E_MEM	= 1<<3,
	E_MEMSP	= 1<<4,		/* uses offset and size */
	E_MEMSB	= 1<<5,		/* uses offset and size */
	ANYMEM	= E_MEM|E_MEMSP|E_MEMSB,
	ALL	= ~0,
};

typedef	struct	Sch	Sch;
typedef	struct	Dep	Dep;
typedef struct	Cycle	Cycle;

struct	Dep
{
	ulong	ireg;
	ulong	freg;
	ulong	cc;
};
struct	Cycle
{
	Dep	set;
	Dep	used;
	long	offset;
	char	size;
};
struct	Sch
{
	Prog	p;
	Cycle	cyc[Ncycle];
	Dep	set;
	Dep	used;
	char	isy;		/* y mem ref */
	char	store;		/* integer store */
	char	ccused;		/* are the condition codes actually used? */
	char	nop;
	char	comp;
	char	isf;
	char	sched;		/* true if already fills a delay slot */
};

int	memref(Adr*);
void	regused(Sch*, Prog*);
int	depend(Sch*, Sch*);
int	conflict(Sch*, Sch*, int);
int	offoverlap(Cycle*, Cycle*);
void	dumpbits(Sch*);
void	dumpdep(Dep*, int);

void
sched(Prog *p0, Prog *pe)
{
	Prog *p, *q;
	Sch sch[NSCHED], *s, *t, *u, *se, stmp;
	int i, nops, needcc;

	/*
	 * build side structure
	 */
	s = sch;
	for(p=p0;; p=p->link) {
		memset(s, 0, sizeof(*s));
		s->p = *p;
		regused(s, p);
		if(debug['X']) {
			Bprint(&bso, "%P", &s->p);
			dumpbits(s);
			Bprint(&bso, "\n");
		}
		s++;
		if(p == pe)
			break;
	}
	se = s;

	/*
	 * flow condition codes
	 */
	needcc = 1;
	for(s=se-1; s>=sch; s--){
		if(needcc && (s->set.cc & E_CC)){
			s->ccused = 1;
			needcc = 0;
		}
		if(s->used.cc & E_CC)
			needcc = 1;
	}

	for(s=se-1; s>=sch; s--) {
		/*
		 * branch delay slot
		 */
		if(s->p.mark & BRANCH) {
			/* t is the trial instruction to use */
			for(t=s-1; t>=sch; t--) {
				if(t->comp || t->isf || (t->p.mark & LOAD))
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
			stmp.sched = 1;
			memmove(t, t+1, (uchar*)s - (uchar*)t);
			*s = stmp;
			s--;
			continue;
		}

		/*
		 * load delay
		 */
		if(s->p.mark & LOAD) {
			if(s >= se-1){
				s->nop = 1;
				continue;
			}
			if(!conflict(s, s+1, 0)){
				s[1].sched = 1;
				continue;
			}
			/*
			 * s is load t is the trial instruction to use
			 */
			for(t=s-1; t>=sch; t--) {
				if(t->isf || (t->p.mark & LOAD))
					goto no2;
				for(u=t+1; u<=s; u++)
					if(depend(u, t))
						goto no2;
				goto out2;
			no2:;
			}
			/*
			 * try the instructions following s
			 */
			for(t=s+1; t<se; t++) {
				if(t->sched || t->isf || (t->p.mark & (BRANCH|LOAD)))
					continue;
				if(conflict(s, t, 0))
					continue;
				for(u=s+1; u<t; u++)
					if(depend(t, u))
						goto no3;
				goto out3;
			no3:;
			}
			if(debug['X'])
				Bprint(&bso, "?l%P\n", &s->p);
			s->nop = 1;
			continue;
		out3:
			if(debug['X']) {
				Bprint(&bso, "!l%P\n", &t->p);
				Bprint(&bso, "%P\n", &s->p);
			}
			stmp = *t;
			stmp.sched = 1;
			memmove(s+1, s, (uchar*)t - (uchar*)s);
			s[1] = stmp;
			continue;
		out2:
			if(debug['X']) {
				Bprint(&bso, "!l%P\n", &t->p);
				Bprint(&bso, "%P\n", &s->p);
			}
			stmp = *t;
			stmp.sched = 1;
			memmove(t, t+1, (uchar*)s - (uchar*)t);
			*s = stmp;
			s--;
			continue;
		}

		/*
		 * fop delay.
		 * just pack for now
		 */
		if(s->p.isf){
			t = s + 1;
			nops = 0;
			if(debug['X'])
				Bprint(&bso, "!f%P ", s);
			for(i = 0; i < 3; i++){
				if(t >= se)
					nops++;
				else{
					nops += conflict(s, t, i);
					i += nops;
					/*
					 * we might fill these nops with other instructions
					 * don't work about BRA, because we don't
					 * conflict with any conditional instructions
					 */
					if(t->p.as != ACALL && t->p.as != AJMP)
						i += t->nop;
					if(debug['X'])
						Bprint(&bso, "%P.%d %d ", t, t->nop, nops);
					t->sched = 1;
				}
				t++;
			}
			if(debug['X'])
				Bprint(&bso, "add %d nops\n", nops);
			s->nop = nops;
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
		for(; s->nop; s->nop--)
			addnop(p);
	}
	if(debug['X'])
		Bprint(&bso, "\n");
}

void
argregused(Sch *s, Cycle *cyc, Cycle *inc, Adr *a, int ar, int sz)
{
	int c;
	ulong m;

	c = a->class;
	c--;
	switch(c) {
	default:
		print("unknown class %d %D\n", c, a);
	case C_ZCON:
	case C_SCON:
	case C_MCON:
	case C_HCON:
	case C_LCON:
		break;
	case C_NONE:
	case C_SBRA:
	case C_MBRA:
	case C_LBRA:
		break;
	case C_CREG:
		cyc->set.ireg = ALL;
		cyc->set.freg = ALL;
		cyc->set.cc = ALL;
		break;
	case C_SOREG:
	case C_MOREG:
	case C_LOREG:
		cyc->used.ireg |= 1 << a->reg;
		break;
	case C_INDR:
	case C_INDFR:
		inc->used.ireg |= 1 << a->increg;
		/* fall through */
	case C_INDM:
	case C_INDFM:
		inc->set.ireg |= 1 << a->reg;
		/* fall through */
	case C_IND:
	case C_INDF:
		c = a->reg;
		inc->used.ireg |= 1 << c;
		cyc->size = sz;
		cyc->offset = 0;
		m = ANYMEM;
		if(c == REGSP)
			m = E_MEMSP;
		if(ar)
			cyc->used.cc |= m;
		else
			cyc->set.cc |= m;
		break;
	case C_SACON:
	case C_LACON:
		cyc->used.ireg |= 1 << REGSP;
		break;
	case C_REG:
	case C_REGPC:
		if(ar)
			cyc->used.ireg |= 1 << a->reg;
		else
			cyc->set.ireg |= 1 << a->reg;
		break;
	case C_FREG:
		if(ar)
			cyc->used.freg |= 1 << a->reg;
		else
			cyc->set.freg |= 1 << a->reg;
		break;
	case C_SEXT:
	case C_MEXT:
	case C_LEXT:
		cyc->size = sz;
		cyc->offset = regoff(&s->p, a);
		if(ar)
			cyc->used.cc |= E_MEMSB;
		else
			cyc->set.cc |= E_MEMSB;
		break;
	}
}

void
regused(Sch *s, Prog *realp)
{
	Prog *p;
	Cycle *now, *acc;
	int i, c, ar, sz, tsz;

	p = &s->p;
	s->comp = compound(p);
	s->nop = 0;
	s->isf = p->isf;
	now = &s->cyc[Now];
	acc = &s->cyc[Acc];

	ar = 0;		/* dest is really reference */
	sz = 20;	/* size of load/store for overlap computation */

	/*
	 * flags based on condition code
	 */
	switch(p->cc){
	case CCEQ:
	case CCNE:
	case CCMI:
	case CCPL:
	case CCGT:
	case CCLE:
	case CCLT:
	case CCGE:
	case CCHI:
	case CCLS:
	case CCCC:
	case CCCS:
	case CCOC:
	case CCOS:
		now->used.cc |= E_ICC;
		break;
	case CCFNE:
	case CCFEQ:
	case CCFGE:
	case CCFLT:
	case CCFOC:
	case CCFOS:
	case CCFUC:
	case CCFUS:
	case CCFGT:
	case CCFLE:
		now->used.cc |= E_FCC;
		break;
	case CCIBE:
	case CCIBF:
	case CCOBE:
	case CCOBF:
	case CCSYC:
	case CCSYS:
	case CCFBC:
	case CCFBS:
	case CCIR0C:
	case CCIR0S:
	case CCIR1C:
	case CCIR1S:
		now->used.cc |= E_OCC;
		break;
	}

	/*
	 * flags based on opcode
	 */
	c = p->reg;
	tsz = 4;
	switch(p->as) {
	case ATEXT:
		curtext = realp;
		break;
	case ACALL:
		c = p->reg;
		if(c == NREG)
			c = REGLINK;
		now->set.ireg |= 1<<c;
		ar = 1;
		break;
	case AJMP:
		ar = 1;
		break;
	case ABRA:
		now->used.cc |= E_ICC;
		ar = 1;
		break;
	case ADBRA:
		now->set.ireg |= 1<<p->from.reg;
		ar = 1;
		break;
	case ADO:				/* should all be SYNC */
	case ADOLOCK:
	case ADOEND:
		now->set.cc = ALL;
		now->set.freg = ALL;
		now->set.ireg = ALL;
		break;
	case ABMOVW:
		now->set.freg |= 1<<0;		/* kills F0 */
		now->set.cc |= E_FCC;
		sz = 4;
		break;
	case AFADD:
	case AFADDN:
	case AFADDT:
	case AFADDTN:
	case AFSUB:
	case AFSUBN:
	case AFSUBT:
	case AFSUBTN:
		argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		if(memref(&p->from1)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from1, 1, 4);
		}else
			argregused(s, acc, now, &p->from1, 1, 4);
		acc->set.cc |= E_FCC;
		break;
	case AFDSP:
	case AFMOVF:
	case AFMOVFN:
	case AFRND:
	case AFSEED:
	case AFMOVWF:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		}else
			argregused(s, acc, now, &p->from, 1, 4);
		acc->set.cc |= E_FCC;
		break;
	case AFMADD:
	case AFMADDN:
	case AFMADDT:
	case AFMADDTN:
	case AFMSUB:
	case AFMSUBN:
	case AFMSUBT:
	case AFMSUBTN:
	case AFMUL:
	case AFMULN:
	case AFMULT:
	case AFMULTN:
		argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		argregused(s, &s->cyc[Load], now, &p->from1, 1, 4);
		if(memref(&p->from1) || memref(&p->from2))
			s->isy = 1;
		if(memref(&p->from2))
			argregused(s, &s->cyc[Load], now, &p->from2, 1, 4);
		else
			argregused(s, acc, now, &p->from2, 1, 4);
		acc->set.cc |= E_FCC;
		break;
	case AFMOVHF:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 2);
		}else
			argregused(s, acc, now, &p->from, 1, 2);
		acc->set.cc |= E_FCC;
		break;
	case AFMOVBF:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 1);
		}else
			argregused(s, acc, now, &p->from, 1, 1);
		acc->set.cc |= E_FCC;
		break;
	case AFIFEQ:
	case AFIFGT:
	case AFIFLT:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		}else
			argregused(s, acc, now, &p->from, 1, 4);
		acc->used.cc |= E_FCC;
		break;
	case AFMOVFW:
	case AFIEEE:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		}else
			argregused(s, acc, now, &p->from, 1, 4);
		break;
	case AFMOVFH:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		}else
			argregused(s, acc, now, &p->from, 1, 4);
		tsz = 2;
		break;
	case AFMOVFB:
		if(memref(&p->from)){
			s->isy = 1;
			argregused(s, &s->cyc[Load], now, &p->from, 1, 4);
		}else
			argregused(s, acc, now, &p->from, 1, 4);
		tsz = 1;
		break;
	case AMOVB:
	case AMOVBU:
	case AMOVHB:
		if(memref(&p->from))
			p->mark |= LOAD;
		else if(memref(&p->to))
			s->store = 1;
		else
			now->set.cc |= E_ICC;
		sz = 1;
		break;
	case AMOVH:
	case AMOVHU:
		if(memref(&p->from))
			p->mark |= LOAD;
		else if(memref(&p->to))
			s->store = 1;
		else
			now->set.cc |= E_ICC;
		sz = 2;
		break;
	case AMOVW:
		if(memref(&p->from))
			p->mark |= LOAD;
		else if(memref(&p->to))
			s->store = 1;
		else
			now->set.cc |= E_ICC;
		sz = 4;
		break;
	case AADD:
	case AADDH:
	case AADDCR:
	case AADDCRH:
	case AAND:
	case AANDH:
	case AANDN:
	case AANDNH:
	case AOR:
	case AORH:
	case AROL:
	case AROLH:
	case AROR:
	case ARORH:
	case ASLL:
	case ASLLH:
	case ASRA:
	case ASRAH:
	case ASRL:
	case ASRLH:
	case ASUB:
	case ASUBH:
	case ASUBR:
	case ASUBRH:
	case AXOR:
	case AXORH:
		if(p->reg == NREG)
			now->used.ireg |= 1 << p->to.reg;
		now->set.cc |= E_ICC;
		break;
	case ABIT:
	case ABITH:
	case ACMP:
	case ACMPH:
		now->set.cc |= E_ICC;
		ar = 1;
		break;
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case AMUL:
		now->set.ireg = ALL;
		now->set.freg = ALL;
		now->set.cc = ALL;
		break;
	}

	if(p->isf){
		argregused(s, &s->cyc[Store], &s->cyc[Load], &p->to, ar, tsz);
		if(c != NREG)
			s->cyc[Acc].set.freg |= 1 << c;
	}else{
		if(p->mark & LOAD){
			argregused(s, &s->cyc[Load], now, &p->from, 1, sz);
			argregused(s, &s->cyc[Load], now, &p->to, 0, sz);
		}else{
			argregused(s, now, now, &p->from, 1, sz);
			argregused(s, now, now, &p->to, ar, sz);
		}
		if(c != NREG)
			now->used.ireg |= 1 << c;
	}

	/*
	 * assume all multi op instructions
	 * make big constants
	 */
	if(s->comp){
		now->set.ireg |= 1 << REGTMP;
		now->set.cc |= E_ICC;
	}
	for(i = 0; i < Ncycle; i++){
		s->set.ireg |= now[i].set.ireg;
		s->used.ireg |= now[i].used.ireg;
		s->set.freg |= now[i].set.freg;
		s->used.freg |= now[i].used.freg;
		s->set.cc |= now[i].set.cc;
		s->used.cc |= now[i].used.cc;
	}
	now->set.ireg &= ~(1<<0);		/* R0 can't be set */
}

int
memref(Adr *a)
{
	switch(a->type){
	case D_NAME:
	case D_INDREG:
	case D_INC:
	case D_INCREG:
	case D_OREG:
	case D_DEC:
		return 1;
	}
	return 0;
}

/*
 * return if moving b after a will change semantics
 */
int
depend(Sch *a, Sch *b)
{
	ulong x;

	if(a->set.ireg & (b->set.ireg|b->used.ireg))
		return 1;
	if(b->set.ireg & a->used.ireg)
		return 1;

	if(a->set.freg & (b->set.freg|b->used.freg))
		return 1;
	if(b->set.freg & a->used.freg)
		return 1;

	x = a->set.cc & (b->set.cc|b->used.cc);
	if(!a->ccused)
		x = a->set.cc & ((b->set.cc&~E_CC)|b->used.cc);
	x |= b->set.cc & a->used.cc;
	if(x){
		/*
		 * allow SB and SP to pass each other.
		 * allow SB to pass SB iff doffsets are ok
		 * anything else conflicts
		 */
		if(x != E_MEMSP && x != E_MEMSB)
			return 1;
		x = a->set.cc | b->set.cc | a->used.cc | b->used.cc;
		if(x & E_MEM)
			return 1;
		return 1;
	}

	return 0;
}

int
offoverlap(Cycle *ca, Cycle *cb)
{
	if(ca->offset < cb->offset) {
		if(ca->offset+ca->size > cb->offset)
			return 1;
		return 0;
	}
	if(cb->offset+cb->size > ca->offset)
		return 1;
	return 0;
}

/*
 * test 2 instructions with delay instructions between
 * return number of inserted instructions needed
 */
int
conflict(Sch *sa, Sch *sb, int delay)
{
	Cycle *a, *b, *e;
	int n;

	n = 1;
	if(sa->isf)
		n = 3;
	if(delay > n)
		return 0;
	e = &sa->cyc[Ncycle];
	for(; n > delay; n--){
		b = sb->cyc;
		for(a = &sa->cyc[n]; a < e; a++){
			if(a->set.ireg & b->used.ireg)
				return n - delay;
			if(a->set.freg & b->used.freg)
				return n - delay;
			if(a->set.cc & b->used.cc)
				return n - delay;
			b++;
		}
	}
	if(!delay && n - delay == 0 && sa->isy && sb->store)
		return 1;
	return n - delay;
}

/*
 * is p a multiple op instruction?
 */
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
dumpbits(Sch *s)
{
	int i, n;

	n = 2;
	if(s->isf)
		n = Ncycle;
	for(i = 0; i < n; i++){
		Bprint(&bso, " set");
		dumpdep(&s->cyc[i].set, s->cyc[i].size);
		Bprint(&bso, " used");
		dumpdep(&s->cyc[i].used, s->cyc[i].size);
	}
}

void
dumpdep(Dep *d, int size)
{
	int i;

	for(i=0; i<NREG; i++)
		if(d->ireg & (1<<i))
			Bprint(&bso, " R%d", i);
	for(i=0; i<NFREG; i++)
		if(d->freg & (1<<i))
			Bprint(&bso, " F%d", i);
	for(i=0; i<32; i++)
		switch(d->cc & (1<<i)) {
		default:
			break;
		case E_ICC:
			Bprint(&bso, " ICC");
			break;
		case E_OCC:
			Bprint(&bso, " OCC");
			break;
		case E_FCC:
			Bprint(&bso, " FCC");
			break;
		case E_MEM:
			Bprint(&bso, " MEM%d", size);
			break;
		case E_MEMSB:
			Bprint(&bso, " SB%d", size);
			break;
		case E_MEMSP:
			Bprint(&bso, " SP%d", size);
			break;
		}
}
