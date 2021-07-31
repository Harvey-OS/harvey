#include	"l.h"

void
span(void)
{
	Prog *p;
	Sym *setext;
	Optab *o;
	int m;
	long c;

	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		p->pc = c;
		o = oplook(p);
		m = o->size;
		if(m == 0) {
			if(p->as == ATEXT) {
				curtext = p;
				autosize = p->to.offset + 4;
				if(p->from.sym != S)
					p->from.sym->value = c;
				continue;
			}
			if(p->as != ANOP)
				diag("zero-width instruction\n%P", p);
			continue;
		}
		c += m;
	}
	c = rnd(c, 8);

	setext = lookup("etext", 0);
	if(setext != S) {
		setext->value = c;
		textsize = c - INITTEXT;
	}
	if(INITRND)
		INITDAT = rnd(c, INITRND);
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
	aclass(a);
	return instoffset;
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

	case D_FREG:
		return C_FREG;

	case D_CREG:
		return C_CREG;

	case D_PREG:
		if(a->reg == D_FSR)
			return C_FSR;
		if(a->reg == D_FPQ)
			return C_FQ;
		return C_PREG;

	case D_OREG:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			if(a->sym == S)
				break;
			t = a->sym->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					a->sym->name, TNAME);
				a->sym->type = SDATA;
			}
			instoffset = a->sym->value + a->offset - BIG;
			if(instoffset >= -BIG && instoffset < BIG) {
				if(instoffset & 7)
					return C_OSEXT;
				return C_ESEXT;
			}
			if(instoffset & 7)
				return C_OLEXT;
			return C_ELEXT;
		case D_AUTO:
			instoffset = autosize + a->offset;
			goto dauto;

		case D_PARAM:
			instoffset = autosize + a->offset + 4L;
		dauto:
			if(instoffset >= -BIG && instoffset < BIG) {
				if(instoffset & 7)
					return C_OSAUTO;
				return C_ESAUTO;
			}
			if(instoffset & 7)
				return C_OLAUTO;
			return C_ELAUTO;
		case D_NONE:
			instoffset = a->offset;
			if(instoffset == 0)
				return C_ZOREG;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SOREG;
			return C_LOREG;
		}
		return C_GOK;

	case D_ASI:
		if(a->name == D_NONE)
			return C_ASI;
		return C_GOK;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			instoffset = a->offset;
		consize:
			if(instoffset == 0)
				return C_ZCON;
			if(instoffset >= -0x1000 && instoffset <= 0xfff)
				return C_SCON;
			if((instoffset & 0x3ff) == 0)
				return C_UCON;
			return C_LCON;

		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			if(s->type == STEXT || s->type == SLEAF) {
				instoffset = s->value + a->offset;
				return C_LCON;
			}
			if(s->type == SCONST) {
				instoffset = s->value + a->offset;
				goto consize;
			}
			instoffset = s->value + a->offset - BIG;
			if(instoffset >= -BIG && instoffset < BIG && instoffset != 0)
				return C_SECON;
			instoffset = s->value + a->offset + INITDAT;
/* not sure why this barfs */
return C_LCON;
			if(instoffset == 0)
				return C_ZCON;
			if(instoffset >= -0x1000 && instoffset <= 0xfff)
				return C_SCON;
			if((instoffset & 0x3ff) == 0)
				return C_UCON;
			return C_LCON;

		case D_AUTO:
			instoffset = autosize + a->offset;
			if(instoffset >= -BIG && instoffset < BIG)
				return C_SACON;
			return C_LACON;

		case D_PARAM:
			instoffset = autosize + a->offset + 4L;
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
	if(o == 0)
		o = oprange[r].stop; /* just generate an error */
	e = oprange[r].stop;
	c1 = xcmp[a1];
	c3 = xcmp[a3];
	for(; o<e; o++)
		if(o->a2 == a2)
		if(c1[o->a1])
		if(c3[o->a3]) {
			p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %d %d %d",
		p->as, a1, a2, a3);
	if(1||!debug['a'])
		prasm(p);
	if(o == 0)
		errorexit();
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
	case C_LBRA:
		if(b == C_SBRA)
			return 1;
		break;
	case C_ELEXT:
		if(b == C_ESEXT)
			return 1;
		break;
	case C_LEXT:
		if(b == C_SEXT ||
		   b == C_ESEXT || b == C_OSEXT ||
		   b == C_ELEXT || b == C_OLEXT)
			return 1;
		break;
	case C_SEXT:
		if(b == C_ESEXT || b == C_OSEXT)
			return 1;
		break;
	case C_ELAUTO:
		if(b == C_ESAUTO)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_SAUTO ||
		   b == C_ESAUTO || b == C_OSAUTO ||
		   b == C_ELAUTO || b == C_OLAUTO)
			return 1;
		break;
	case C_SAUTO:
		if(b == C_ESAUTO || b == C_OSAUTO)
			return 1;
		break;
	case C_REG:
		if(b == C_ZCON)
			return 1;
		break;
	case C_LOREG:
		if(b == C_ZOREG || b == C_SOREG)
			return 1;
		break;
	case C_SOREG:
		if(b == C_ZOREG)
			return 1;
		break;

	case C_ANY:
		return 1;
	}
	return 0;
}

int
ocmp(const void *a1, const void *a2)
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
	n = p1->a2 - p2->a2;
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

	for(i=0; i<C_NCLASS; i++)
		for(n=0; n<C_NCLASS; n++)
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
		
		switch(r)
		{
		default:
			diag("unknown op in build: %A", r);
			errorexit();
		case AADD:
			oprange[AADDX] = oprange[r];
			oprange[ASUB] = oprange[r];
			oprange[ASUBX] = oprange[r];
			oprange[AMUL] = oprange[r];
			oprange[AXOR] = oprange[r];
			oprange[AXNOR] = oprange[r];
			oprange[AAND] = oprange[r];
			oprange[AANDN] = oprange[r];
			oprange[AOR] = oprange[r];
			oprange[AORN] = oprange[r];
			oprange[ASLL] = oprange[r];
			oprange[ASRL] = oprange[r];
			oprange[ASRA] = oprange[r];
			oprange[AADDCC] = oprange[r];
			oprange[AADDXCC] = oprange[r];
			oprange[ATADDCC] = oprange[r];
			oprange[ATADDCCTV] = oprange[r];
			oprange[ASUBCC] = oprange[r];
			oprange[ASUBXCC] = oprange[r];
			oprange[ATSUBCC] = oprange[r];
			oprange[ATSUBCCTV] = oprange[r];
			oprange[AXORCC] = oprange[r];
			oprange[AXNORCC] = oprange[r];
			oprange[AANDCC] = oprange[r];
			oprange[AANDNCC] = oprange[r];
			oprange[AORCC] = oprange[r];
			oprange[AORNCC] = oprange[r];
			oprange[AMULSCC] = oprange[r];
			oprange[ASAVE] = oprange[r];
			oprange[ARESTORE] = oprange[r];
			break;
		case AMOVB:
			oprange[AMOVH] = oprange[r];
			oprange[AMOVHU] = oprange[r];
			oprange[AMOVBU] = oprange[r];
			oprange[ASWAP] = oprange[r];
			oprange[ATAS] = oprange[r];
			break;
		case ABA:
			oprange[ABN] = oprange[r];
			oprange[AFBA] = oprange[r];
			oprange[AFBN] = oprange[r];
			break;
		case ABE:
			oprange[ABCC] = oprange[r];
			oprange[ABCS] = oprange[r];
			oprange[ABGE] = oprange[r];
			oprange[ABGU] = oprange[r];
			oprange[ABG] = oprange[r];
			oprange[ABLEU] = oprange[r];
			oprange[ABLE] = oprange[r];
			oprange[ABL] = oprange[r];
			oprange[ABNEG] = oprange[r];
			oprange[ABNE] = oprange[r];
			oprange[ABPOS] = oprange[r];
			oprange[ABVC] = oprange[r];
			oprange[ABVS] = oprange[r];

			oprange[AFBE] = oprange[r];
			oprange[AFBG] = oprange[r];
			oprange[AFBGE] = oprange[r];
			oprange[AFBL] = oprange[r];
			oprange[AFBLE] = oprange[r];
			oprange[AFBLG] = oprange[r];
			oprange[AFBNE] = oprange[r];
			oprange[AFBO] = oprange[r];
			oprange[AFBU] = oprange[r];
			oprange[AFBUE] = oprange[r];
			oprange[AFBUG] = oprange[r];
			oprange[AFBUGE] = oprange[r];
			oprange[AFBUL] = oprange[r];
			oprange[AFBULE] = oprange[r];
			break;
		case ATA:
			oprange[ATCC] = oprange[r];
			oprange[ATCS] = oprange[r];
			oprange[ATE] = oprange[r];
			oprange[ATGE] = oprange[r];
			oprange[ATGU] = oprange[r];
			oprange[ATG] = oprange[r];
			oprange[ATLEU] = oprange[r];
			oprange[ATLE] = oprange[r];
			oprange[ATL] = oprange[r];
			oprange[ATNEG] = oprange[r];
			oprange[ATNE] = oprange[r];
			oprange[ATN] = oprange[r];
			oprange[ATPOS] = oprange[r];
			oprange[ATVC] = oprange[r];
			oprange[ATVS] = oprange[r];
			break;
		case AFADDD:
			oprange[AFADDF] = oprange[r];
			oprange[AFADDX] = oprange[r];
			oprange[AFDIVD] = oprange[r];
			oprange[AFDIVF] = oprange[r];
			oprange[AFDIVX] = oprange[r];
			oprange[AFMULD] = oprange[r];
			oprange[AFMULF] = oprange[r];
			oprange[AFMULX] = oprange[r];
			oprange[AFSUBD] = oprange[r];
			oprange[AFSUBF] = oprange[r];
			oprange[AFSUBX] = oprange[r];
			break;
		case AFCMPD:
			oprange[AFCMPF] = oprange[r];
			oprange[AFCMPX] = oprange[r];
			oprange[AFCMPED] = oprange[r];
			oprange[AFCMPEF] = oprange[r];
			oprange[AFCMPEX] = oprange[r];
			break;
		case AFABSF:
			oprange[AFMOVDF] = oprange[r];
			oprange[AFMOVDW] = oprange[r];
			oprange[AFMOVFD] = oprange[r];
			oprange[AFMOVFW] = oprange[r];
			oprange[AFMOVWD] = oprange[r];
			oprange[AFMOVWF] = oprange[r];
			oprange[AFNEGF] = oprange[r];
			oprange[AFSQRTD] = oprange[r];
			oprange[AFSQRTF] = oprange[r];
			break;
		case AFMOVF:
		case AFMOVD:
		case AMOVW:
		case AMOVD:
		case AWORD:
		case ARETT:
		case AJMPL:
		case AJMP:
		case ACMP:
		case ANOP:
		case ATEXT:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			break;
		}
	}
}
