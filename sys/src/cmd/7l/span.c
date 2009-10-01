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
				autosize = p->to.offset + 8;
				if(p->from.sym != S)
					p->from.sym->value = c;
				continue;
			}
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

vlong		/* BUG? */
regoff(Adr *a)
{

	offset = 0;
	aclass(a);
	return offset;
}

/*
 * note that we can't generate every 32 bit constant with MOVQA+MOVQAH, hence the
 * comparison with 0x7fff8000.  offset >= this value gets incorrectly sign extended in
 * the 64 bit register.
 */
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

	case D_FCREG:
		return C_FCREG;

	case D_PREG:
		return C_PREG;

	case D_PCC:
		return C_PCC;

	case D_OREG:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			if(a->sym == 0) {
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
			offset = a->sym->value + a->offset - BIG;
			if(offset >= -BIG && offset < BIG)
				return C_SEXT;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LEXT;
		badoff:
			diag("offset out of range: %#llux", offset);
			return C_GOK;
		case D_AUTO:
			offset = autosize + a->offset;
			if(offset >= -BIG && offset < BIG)
				return C_SAUTO;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LAUTO;
			goto badoff;
		case D_PARAM:
			offset = autosize + a->offset + 8L;
			if(offset >= -BIG && offset < BIG)
				return C_SAUTO;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LAUTO;
			goto badoff;
		case D_NONE:
			offset = a->offset;
			if(offset == 0)
				return C_ZOREG;
			if(offset >= -BIG && offset < BIG)
				return C_SOREG;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LOREG;
			goto badoff;
		}
		return C_GOK;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			offset = a->offset;
			if(offset > 0) {
				if(offset <= 0xffLL)
					return C_BCON;
				if(offset <= 0x7fffLL)
					return C_SCON;
				if((offset & 0xffffLL) == 0 && offset <= 0x7fff0000LL)
					return C_UCON;
				if (offset < 0x7fff8000LL)
					return C_LCON;
				return C_QCON;
			}
			if(offset == 0)
				return C_ZCON;
			if(offset >= -0x100LL)
				return C_NCON;
			if(offset >= -0x8000LL)
				return C_SCON;
			if((offset & 0xffffLL) == 0 && offset >= -0x80000000LL)
				return C_UCON;
			if (offset >= -0x80000000LL)
				return C_LCON;
			return C_QCON;

		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == 0) {
				print("null sym const\n");
				print("%D\n", a);
				return C_GOK;
			}
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			if(s->type == STEXT || s->type == SLEAF) {
				offset = s->value + a->offset;
				return C_LCON;
			}
			offset = s->value + a->offset - BIG;
			if (offset == 0L) {
				offset = s->value + a->offset + INITDAT;
				return C_LCON;		/* botch */
			}
			if(offset >= -BIG && offset < BIG && offset != 0L)
				return C_SECON;
			if (offset >= -0x80000000LL && offset < 0x7fff8000L && offset != 0LL /*&& offset >= -INITDAT*/)
				return C_LECON;
			/*offset = s->value + a->offset + INITDAT;*/
/*			if (offset >= -BIG && offset < BIG)
				return C_SCON;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LCON; */
			goto badoff;
			/*return C_QCON;*/

		case D_AUTO:
			offset = autosize + a->offset;
			if(offset >= -BIG && offset < BIG)
				return C_SACON;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LACON;
			goto badoff;

		case D_PARAM:
			offset = autosize + a->offset + 8L;
			if(offset >= -BIG && offset < BIG)
				return C_SACON;
			if (offset >= -0x80000000LL && offset < 0x7fff8000LL)
				return C_LACON;
			goto badoff;
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
		a1 = opcross[repop[r]][a1][a2][a3];
		if(a1) {
			p->optab = a1+1;
			return optab+a1;
		}
		o = oprange[r].stop; /* just generate an error */
	}
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
	diag("illegal combination %A %d %d %d (opcross %d)",
		p->as, p->from.class-1, a2, a3, a1);
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
	case C_QCON:
		if(b == C_ZCON || b == C_BCON || b == C_NCON || b == C_SCON || b == C_UCON || b == C_LCON)
			return 1;
		break;
	case C_LCON:
		if(b == C_ZCON || b == C_BCON || b == C_NCON || b == C_SCON || b == C_UCON)
			return 1;
		break;
	case C_UCON:
		if(b == C_ZCON)
			return 1;
		break;
	case C_SCON:
		if(b == C_ZCON || b == C_BCON || b == C_NCON)
			return 1;
		break;
	case C_BCON:
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
	case C_LEXT:
		if(b == C_SEXT)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_SAUTO)
			return 1;
		break;
	case C_REG:
/*		if(b == C_ZCON)
			return 1; */
		break;
	case C_LOREG:
		if(b == C_ZOREG || b == C_SOREG)
			return 1;
		break;
	case C_SOREG:
		if(b == C_ZOREG)
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

	p1 = a1;
	p2 = a2;
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
		
		switch(r)
		{
		default:
			diag("unknown op in build: %A", r);
			errorexit();
		case AADDQ:
			oprange[AS4ADDQ] = oprange[r];
			oprange[AS8ADDQ] = oprange[r];
			oprange[ASUBQ] = oprange[r];
			oprange[AS4SUBQ] = oprange[r];
			oprange[AS8SUBQ] = oprange[r];
			break;
		case AADDL:
			oprange[AADDLV] = oprange[r];
			oprange[AS4ADDL] = oprange[r];
			oprange[AS8ADDL] = oprange[r];
			oprange[AADDQV] = oprange[r];
			oprange[ASUBQV] = oprange[r];
			oprange[ASUBL] = oprange[r];
			oprange[ASUBLV] = oprange[r];
			oprange[AS4SUBL] = oprange[r];
			oprange[AS8SUBL] = oprange[r];
			break;
		case AAND:
			oprange[AANDNOT] = oprange[r];
			oprange[AOR] = oprange[r];
			oprange[AORNOT] = oprange[r];
			oprange[AXOR] = oprange[r];
			oprange[AXORNOT] = oprange[r];
			break;
		case AMULQ:
			oprange[ACMPEQ] = oprange[r];
			oprange[ACMPGT] = oprange[r];
			oprange[ACMPGE] = oprange[r];
			oprange[ACMPUGT] = oprange[r];
			oprange[ACMPUGE] = oprange[r];
			oprange[ACMPBLE] = oprange[r];
			oprange[ACMOVEQ] = oprange[r];
			oprange[ACMOVNE] = oprange[r];
			oprange[ACMOVLT] = oprange[r];
			oprange[ACMOVGE] = oprange[r];
			oprange[ACMOVLE] = oprange[r];
			oprange[ACMOVGT] = oprange[r];
			oprange[ACMOVLBS] = oprange[r];
			oprange[ACMOVLBC] = oprange[r];
			oprange[AMULL] = oprange[r];
			oprange[AMULLV] = oprange[r];
			oprange[AMULQV] = oprange[r];
			oprange[AUMULH] = oprange[r];
			oprange[ASLLQ] = oprange[r];
			oprange[ASRLQ] = oprange[r];
			oprange[ASRAQ] = oprange[r];
			oprange[AEXTBL] = oprange[r];
			oprange[AEXTWL] = oprange[r];
			oprange[AEXTLL] = oprange[r];
			oprange[AEXTQL] = oprange[r];
			oprange[AEXTWH] = oprange[r];
			oprange[AEXTLH] = oprange[r];
			oprange[AEXTQH] = oprange[r];
			oprange[AINSBL] = oprange[r];
			oprange[AINSWL] = oprange[r];
			oprange[AINSLL] = oprange[r];
			oprange[AINSQL] = oprange[r];
			oprange[AINSWH] = oprange[r];
			oprange[AINSLH] = oprange[r];
			oprange[AINSQH] = oprange[r];
			oprange[AMSKBL] = oprange[r];
			oprange[AMSKWL] = oprange[r];
			oprange[AMSKLL] = oprange[r];
			oprange[AMSKQL] = oprange[r];
			oprange[AMSKWH] = oprange[r];
			oprange[AMSKLH] = oprange[r];
			oprange[AMSKQH] = oprange[r];
			oprange[AZAP] = oprange[r];
			oprange[AZAPNOT] = oprange[r];
			break;
		case ABEQ:
			oprange[ABNE] = oprange[r];
			oprange[ABGE] = oprange[r];
			oprange[ABLE] = oprange[r];
			oprange[ABGT] = oprange[r];
			oprange[ABLT] = oprange[r];
			break;
		case AFBEQ:
			oprange[AFBNE] = oprange[r];
			oprange[AFBGE] = oprange[r];
			oprange[AFBLE] = oprange[r];
			oprange[AFBGT] = oprange[r];
			oprange[AFBLT] = oprange[r];
			break;
		case AMOVQ:
			oprange[AMOVL] = oprange[r];
			oprange[AMOVLU] = oprange[r];
			oprange[AMOVQU] = oprange[r];
			oprange[AMOVA] = oprange[r];
			oprange[AMOVAH] = oprange[r];

			oprange[AMOVBU] = oprange[r];
			oprange[AMOVWU] = oprange[r];
			oprange[AMOVB] = oprange[r];
			oprange[AMOVW] = oprange[r];
			break;
		case AMOVT:
			oprange[AMOVS] = oprange[r];
			break;
		case AADDT:
			oprange[AADDS] = oprange[r];
			oprange[ACMPTEQ] = oprange[r];
			oprange[ACMPTGT] = oprange[r];
			oprange[ACMPTGE] = oprange[r];
			oprange[ACMPTUN] = oprange[r];
			oprange[ADIVS] = oprange[r];
			oprange[ADIVT] = oprange[r];
			oprange[AMULS] = oprange[r];
			oprange[AMULT] = oprange[r];
			oprange[ASUBS] = oprange[r];
			oprange[ASUBT] = oprange[r];
			oprange[ACPYS] = oprange[r];
			oprange[ACPYSN] = oprange[r];
			oprange[ACPYSE] = oprange[r];
			oprange[ACVTLQ] = oprange[r];
			oprange[ACVTQL] = oprange[r];
			oprange[AFCMOVEQ] = oprange[r];
			oprange[AFCMOVNE] = oprange[r];
			oprange[AFCMOVLT] = oprange[r];
			oprange[AFCMOVGE] = oprange[r];
			oprange[AFCMOVLE] = oprange[r];
			oprange[AFCMOVGT] = oprange[r];
			break;
		case ACVTTQ:
			oprange[ACVTQT] = oprange[r];
			oprange[ACVTTS] = oprange[r];
			oprange[ACVTQS] = oprange[r];
			break;
		case AJMP:
		case AJSR:
			break;
		case ACALL_PAL:
			break;
		case AMOVQL:
			oprange[AMOVLL] = oprange[r];
			break;
		case AMOVQC:
			oprange[AMOVLC] = oprange[r];
			break;
		case AMOVQP:
			oprange[AMOVLP] = oprange[r];
			break;
		case AREI:
			oprange[AMB] = oprange[r];
			oprange[ATRAPB] = oprange[r];
			break;
		case AFETCH:
			oprange[AFETCHM] = oprange[r];
			break;
		case AWORD:
		case ATEXT:
			break;
		}
	}
}

void
buildrep(int x, int as)
{
	Opcross *p;
	Optab *e, *s, *o;
	int a1, a2, a3, n;

	if(C_NONE != 0 || C_REG != 1 || C_GOK >= 32 || x >= nelem(opcross)) {
		diag("assumptions fail in buildrep");
		errorexit();
	}
	repop[as] = x;
	p = (opcross + x);
	s = oprange[as].start;
	e = oprange[as].stop;
	for(o=e-1; o>=s; o--) {
		n = o-optab;
		for(a2=0; a2<2; a2++) {
			if(a2) {
				if(o->a2 == C_NONE)
					continue;
			} else
				if(o->a2 != C_NONE)
					continue;
			for(a1=0; a1<32; a1++) {
				if(!xcmp[a1][o->a1])
					continue;
				for(a3=0; a3<32; a3++)
					if(xcmp[a3][o->a3])
						(*p)[a1][a2][a3] = n;
			}
		}
	}
	oprange[as].start = 0;
}
