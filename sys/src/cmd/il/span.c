#include	"l.h"

void
span(void)
{
	Prog *p, *q;
	Sym *setext, *s;
	Optab *o;
	int m, bflag, i, spass;
	long c, otxt, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);

	bflag = 0;
	c = 0;
	otxt = c;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			c = (c + 3) & ~3;
		p->pc = c;
		o = oplook(p);
		m = o->size;
		if(!debug['c']){
			if(o->ctype && asmout(p, o, 2) == 2){
				bflag = 1;
				p->mark |= COMPR;
				m = 2;
			}
		}
		if(m == 0) {
			if(p->as == ATEXT) {
				curtext = p;
				autosize = p->to.offset + ptrsize;
				if(p->from.sym != S)
					p->from.sym->value = c;
				/* need passes to resolve branches */
				if(c-otxt >= 0x1000)
					bflag = 1;
				otxt = c;
				continue;
			}
			diag("zero-width instruction\n%P", p);
			continue;
		}
		c += m;
	}

	/*
	 * Multi-pass expansion of span dependent instructions
	 *   Bcond JAL C.Bcond C.JAL C.JMP
	 */
	spass = 0;
	while(bflag) {
		if(debug['v'])
			Bprint(&bso, "%5.2f span1\n", cputime());
		bflag = 0;
		spass ^= SPASS;
		c = 0;
		for(p = firstp; p != P; p = p->link) {
			o = oplook(p);
			m = o->size;
			if(p->mark&COMPR)
				m = 2;
			if((o->type == 3 || o->type == 4) && p->cond) {
				if((p->cond->mark&SPASS) == spass)
					p->pc = c;
				if(m == 2){
					/*
					 * If instruction was compressed, check again in case
					 * branch range is now too large.
					 */
					m = asmout(p, o, 3);
					if(m != 2){
						p->mark &= ~COMPR;
						bflag = 1;
					}
				}
				otxt = p->cond->pc - p->pc;
				if(otxt < 0)
					otxt = -otxt;
				if(o->type == 3){
					/*
					 * If Bcond branch range exceeds 4K, replace it by the
					 * logically negated branch around a JMP.
					*/
					if(otxt >= 0x1000) {
						q = prg();
						q->link = p->link;
						q->line = p->line;
						q->as = AJMP;
						q->to.type = D_BRANCH;
						q->cond = p->cond;
						p->link = q;
						p->as = relinv(p->as);
						p->cond = q->link;
						p->optab = 0;
						o = oplook(p);
						q->mark = spass ^ SPASS;
						m = asmout(p, o, 2);
						if(m == 2)
							p->mark |= COMPR;
						q->pc = p->pc + m;
						bflag = 1;
					}
				}else{
					/*
					 * If JAL branch range exceeds 1M, change address class
					 * and recalculate instruction length.
					 */
					if(otxt >= 0x100000) {
						p->to.class = C_LBRA + 1;
						p->optab = 0;
						o = oplook(p);
						m = asmout(p, o, 3);
						p->mark &= ~COMPR;
					}
				}
			}
			if(p->as == ATEXT)
				c = (c + 3) & ~3;
			p->pc = c;
			p->mark ^= SPASS;
			if(m == 0) {
				if(p->as == ATEXT) {
					curtext = p;
					autosize = p->to.offset + ptrsize;
					if(p->from.sym != S)
						p->from.sym->value = c;
					continue;
				}
				diag("zero-width instruction\n%P", p);
				continue;
			}
			c += m;
		}
	}

	if(debug['t']) {
		/* 
		 * add strings to text segment
		 */
		c = rnd(c, 8);
		for(i=0; i<NHASH; i++)
		for(s = hash[i]; s != S; s = s->link) {
			if(s->type != SSTRING)
				continue;
			v = s->value;
			while(v & 3)
				v++;
			s->value = c;
			c += v;
		}
	}

	c = rnd(c, 8);

	setext = lookup("etext", 0);
	if(setext != S) {
		setext->value = c;
		textsize = c;
	}
	if(INITRND)
		INITDAT = rnd(INITTEXT + c, INITRND);
	if(debug['v'])
		Bprint(&bso, "tsize = %lux\n", textsize);
	Bflush(&bso);
}
		
void
xdefine(char *p, int t, long v)
{
	Sym *s;

	s = lookup(p, 0);
	if(s->type == 0 || s->type == SXREF) {
		s->type = t;
		s->value = v;
	}
}

long
regoff(Adr *a)
{

	instoffset = 0;
	a->class = aclass(a) + 1;
	return instoffset;
}

int
classreg(Adr *a)
{
	if(a->reg == NREG) {
		switch(a->class - 1) {
		case C_SEXT:
		case C_SECON:
		case C_LECON:
			return REGSB;
		case C_SAUTO:
		case C_LAUTO:
		case C_SACON:
		case C_LACON:
			return REGSP;
		}
	}
	return a->reg;
}

int
aclass(Adr *a)
{
	Sym *s;
	int t;

	switch(a->type) {
	case D_NONE:
		return C_NONE;

	case D_REG:
		return C_REG;

	case D_CTLREG:
		return C_CTLREG;

	case D_FREG:
		return C_FREG;

	case D_OREG:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			if(a->sym == 0 || a->sym->name == 0) {
				print("null sym external\n");
				print("%D\n", a);
				return C_GOK;
			}
			t = a->sym->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					a->sym->name, TNAME);
				a->sym->type = SDATA;
			}
			instoffset = a->sym->value + a->offset - BIG;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SEXT;
			return C_LEXT;
		case D_AUTO:
			instoffset = autosize + a->offset;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SAUTO;
			return C_LAUTO;

		case D_PARAM:
			instoffset = autosize + a->offset + ptrsize;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SAUTO;
			return C_LAUTO;
		case D_NONE:
			instoffset = a->offset;
			if(instoffset == 0)
				return C_ZOREG;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SOREG;
			return C_LOREG;
		}
		return C_GOK;

	case D_FCONST:
		return C_FCON;

	case D_VCONST:
		return C_VCON;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			instoffset = a->offset;
			if(a->reg != NREG && a->reg != REGZERO){
				if(instoffset >= -BIG && instoffset < BIG)
					return C_SRCON;
				return C_LRCON;
			}
		consize:
			if(instoffset == 0)
				return C_ZCON;
			if(instoffset >= -0x800 && instoffset <= 0x7ff)
				return C_SCON;
			if((instoffset & 0xfff) == 0)
				return C_UCON;
			return C_LCON;

		case D_EXTERN:
		case D_STATIC:
			instoffx = 0;
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			switch(t) {
			case 0:
			case SXREF:
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
				break;
			case SCONST:
				instoffset = s->value + a->offset;
				goto consize;
			case STEXT:
			case SLEAF:
			case SSTRING:
				instoffset = s->value + a->offset;
				instoffx = INITTEXT;
				return C_LECON;
			}
			instoffset = s->value + a->offset - BIG;
			if(instoffset >= -BIG && instoffset < BIG && instoffset != 0L)
				return C_SECON;
			instoffset = s->value + a->offset;
			instoffx = INITDAT;
			return C_LECON;

		case D_AUTO:
			instoffset = autosize + a->offset;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SACON;
			return C_LACON;

		case D_PARAM:
			instoffset = autosize + a->offset + ptrsize;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SACON;
			return C_LACON;
		}
		return C_GOK;

	case D_BRANCH:
		return C_SBRA;
	}
	return C_GOK;
}

Optab*
oplook(Prog *p)
{
	int a1, a2, a3, r;
	char *c1, *c3;
	Optab *o, *e;

	a1 = p->optab;
	if(a1)
		return optab+(a1-1);
	a1 = p->from.class;
	if(a1 == 0) {
		a1 = aclass(&p->from) + 1;
		p->from.class = a1;
	}
	a1--;
	a3 = p->to.class;
	if(a3 == 0) {
		a3 = aclass(&p->to) + 1;
		p->to.class = a3;
	}
	a3--;
	a2 = C_NONE;
	if(p->reg != NREG)
		a2 = C_REG;
	r = p->as;
	o = oprange[r].start;
	if(o == 0) {
		a1 = opcross[repop[r]][a1][a3];
		if(a1) {
			p->optab = a1+1;
			return optab+a1;
		}
		o = oprange[r].stop; /* just generate an error */
		a1 = p->from.class - 1;
	}
	e = oprange[r].stop;

	c1 = xcmp[a1];
	c3 = xcmp[a3];
	for(; o<e; o++)
		if(c1[o->a1])
		if(c3[o->a3]) {
			p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %d %d %d",
		p->as, a1, a2, a3);
	if(!debug['a'])
		prasm(p);
	o = optab;
	p->optab = (o-optab)+1;
	return o;
}

int
cmp(int a, int b)
{

	if(a == b)
		return 1;
	switch(a) {
	case C_LCON:
		if(b == C_ZCON || b == C_SCON || b == C_UCON)
			return 1;
		break;
	case C_UCON:
		if(b == C_ZCON)
			return 1;
		break;
	case C_SCON:
		if(b == C_ZCON)
			return 1;
		break;
	case C_LACON:
		if(b == C_SACON)
			return 1;
		break;
	case C_LRCON:
		if(b == C_SRCON)
			return 1;
		break;
	case C_LBRA:
		if(b == C_SBRA)
			return 1;
		break;
	case C_LEXT:
		if(b == C_SEXT)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_SAUTO)
			return 1;
		break;
	case C_ZREG:
		if(b == C_REG || b == C_ZCON)
			return 1;
		break;
	case C_LOREG:
		if(b == C_ZOREG || b == C_SOREG || b == C_SAUTO || b == C_LAUTO)
			return 1;
		break;
	case C_SOREG:
		if(b == C_ZOREG || b == C_SAUTO || b == C_SEXT)
			return 1;
		break;
	}
	return 0;
}

int
ocmp(void *a1, void *a2)
{
	Optab *p1, *p2;
	int n;

	p1 = (Optab*)a1;
	p2 = (Optab*)a2;
	n = p1->as - p2->as;
	if(n)
		return n;
	n = p1->a1 - p2->a1;
	if(n)
		return n;
	n = p1->a3 - p2->a3;
	if(n)
		return n;
	return 0;
}

void
buildop(void)
{
	int i, n, r;

	for(i=0; i<32; i++)
		for(n=0; n<32; n++)
			xcmp[i][n] = cmp(n, i);
	for(n=0; optab[n].as != AXXX; n++)
		;
	qsort(optab, n, sizeof(optab[0]), ocmp);
	for(i=0; i<n; i++) {
		r = optab[i].as;
		oprange[r].start = optab+i;
		while(optab[i].as == r)
			i++;
		oprange[r].stop = optab+i;
		i--;
	}

	buildrep(1, AMOVW);
}

void
buildrep(int x, int as)
{
	Opcross *p;
	Optab *e, *s, *o;
	int a1, a3, n;

	if(C_GOK >= 32 || x >= nelem(opcross)) {
		diag("assumptions fail in buildrep");
		errorexit();
	}
	repop[as] = x;
	p = (opcross + x);
	s = oprange[as].start;
	e = oprange[as].stop;
	for(o=e-1; o>=s; o--) {
		n = o-optab;
		for(a1=0; a1<32; a1++) {
			if(!xcmp[a1][o->a1])
				continue;
			for(a3=0; a3<32; a3++)
				if(xcmp[a3][o->a3])
					(*p)[a1][a3] = n;
		}
	}
	oprange[as].start = 0;
}
