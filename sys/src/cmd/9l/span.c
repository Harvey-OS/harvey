#include	"l.h"


#define	BIG	((1<<17)-10)

void
span(void)
{
	Prog *p;
	Sym *setext;
	Optab *o;
	int m;
	long c, oc, v;

	oc = 0;

loop:
	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);

	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(oc != 0 && p->to.type == D_BRANCH && p->to.class == C_SBRA+1) {
			v = 0;
			if(p->cond != P)
				if(p->cond->pc > c)
					v = p->cond->pc - p->pc;	/* foreward */
				else
					v = p->cond->pc - c;		/* backward */
			if(v < -BIG || v > BIG) {
				p->to.class = C_LBRA+1;
				p->optab = 0;
			}
		}
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
			diag("zero-width instruction\n%P\n", p);
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
	if(oc != c) {
		oc = c;
		goto loop;
	}
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

	offset = 0;
	aclass(a);
	return offset;
}

int
cclass(void)
{
	long o;

	o = offset;
	if(o == 0)
		return C_ZCON-C_ZCON;
	if((o & ~0xff) == 0)
		return C_SCON-C_ZCON;
	if((o & ~0xff) == ~0xff)
		return C_MCON-C_ZCON;
	if((o & ~0xffff) == 0)
		return C_PCON-C_ZCON;
	if((o & ~0xffff) == ~0xffff)
		return C_NCON-C_ZCON;
	return C_LCON-C_ZCON;
}

int
aclass(Adr *a)
{
	Sym *s;
	int t;

	noacache = 0;
	switch(a->type) {
	case D_NONE:
		return C_NONE;

	case D_REG:
		return C_REG;

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
				diag("undefined external: %s in %s\n",
					a->sym->name, TNAME);
				a->sym->type = SDATA;
			}
			offset = a->sym->value + a->offset + INITDAT;
			noacache = 1;
			if(offset >= INITDAT && offset <= INITDAT+0xff+0xff)
				return C_SEXT;
			if(offset >= 0 && offset <= 0xffff)
				return C_SEXT;
			return C_LEXT;
		case D_AUTO:
			offset = autosize + a->offset;
			return C_ZAUTO + cclass();
		case D_PARAM:
			offset = autosize + a->offset + 4L;
			return C_ZAUTO + cclass();
		case D_NONE:
			offset = a->offset;
			t = C_ZOREG + cclass();
			if(a->reg == NREG) {
				if(t == C_LOREG)
					return C_LEXT;
/*
				if(t == C_POREG)
					return C_SEXT;
 */
			}
			return t;
		}
		return C_GOK;

	case D_OCONST:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s\n",
					s->name, TNAME);
				s->type = SDATA;
			}
			offset = s->value + a->offset;
			if(s->type != STEXT && s->type != SLEAF) {
				noacache = 1;
				offset += INITDAT;
			}
			return C_ZCON + cclass();
		}
		return C_GOK;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			offset = a->offset;
			if(a->reg != NREG)
				return C_ZACON + cclass();
			return C_ZCON + cclass();

		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			switch(t) {
			case 0:
			case SXREF:
				diag("undefined external: %s in %s\n",
					s->name, TNAME);
				s->type = SDATA;
				break;
			case STEXT:
			case SLEAF:
				offset = s->value + a->offset;
				return C_LCON;
			}
			offset = s->value + a->offset + INITDAT;
			noacache = 1;
			return C_ZCON + cclass();

		case D_AUTO:
			offset = autosize + a->offset;
			return C_ZACON + cclass();

		case D_PARAM:
			offset = autosize + a->offset + 4L;
			return C_ZACON + cclass();
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
	int a1, a2, a3, r, noopcache;
	char *c1, *c3;
	Optab *o, *e;

	a1 = p->optab;
	if(a1)
		return optab+(a1-1);
	noopcache = 0;
	a1 = p->from.class;
	if(a1 == 0) {
		a1 = aclass(&p->from) + 1;
		if(!noacache)
			p->from.class = a1;
		else
			noopcache = 1;
	}
	a1--;
	a3 = p->to.class;
	if(a3 == 0) {
		a3 = aclass(&p->to) + 1;
		if(!noacache)
			p->to.class = a3;
		else
			noopcache = 1;
	}
	a3--;
	a2 = C_NONE;
	if(p->reg != NREG)
		a2 = C_REG;
	r = p->as;
	o = oprange[r].start;
	if(o == 0) {
		r = opcross[repop[r]][a1][a2][a3];
		if(r) {
			if(!noopcache)
				p->optab = r;
			return optab+r-1;
		}
		diag("no combination %A %d %d %d\n%P\n",
			p->as, a1, a2, a3, p);
		return optab + opcross[repop[AMOVL]][C_REG][C_NONE][C_REG] - 1;
	}
	e = oprange[r].stop;
	c1 = xcmp[a1];
	c3 = xcmp[a3];
	for(; o<e; o++)
		if(o->a2 == a2)
		if(c1[o->a1])
		if(c3[o->a3]) {
			if(!noopcache)
				p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %d %d %d\n%P\n",
		p->as, a1, a2, a3, p);
	return o;
}

int
cmp(int a, int b)
{

	if(a == b)
		return 1;
	switch(a) {
	case C_LCON:
		if(b == C_ZCON || b == C_SCON || b == C_MCON ||
		   b == C_PCON || b == C_NCON)
			return 1;
		break;
	case C_PCON:
		if(b == C_ZCON || b == C_SCON)
			return 1;
		break;
	case C_SCON:
		if(b == C_ZCON)
			return 1;
		break;
	case C_NCON:
		if(b == C_MCON)
			return 1;
		break;
	case C_LACON:
		if(b == C_ZACON || b == C_SACON || b == C_MACON ||
		   b == C_PACON || b == C_NACON)
			return 1;
		break;
	case C_PACON:
		if(b == C_ZACON || b == C_SACON)
			return 1;
		break;
	case C_SACON:
		if(b == C_ZACON)
			return 1;
		break;
	case C_NACON:
		if(b == C_MACON)
			return 1;
		break;
	case C_LBRA:
		if(b == C_SBRA)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_ZAUTO || b == C_SAUTO || b == C_MAUTO ||
		   b == C_PAUTO || b == C_NAUTO)
			return 1;
		break;
	case C_PAUTO:
		if(b == C_ZAUTO || b == C_SAUTO)
			return 1;
		break;
	case C_SAUTO:
		if(b == C_ZAUTO)
			return 1;
		break;
	case C_NAUTO:
		if(b == C_MAUTO)
			return 1;
		break;
	case C_LOREG:
		if(b == C_ZOREG || b == C_SOREG || b == C_MOREG ||
		   b == C_POREG || b == C_NOREG)
			return 1;
		break;
	case C_POREG:
		if(b == C_ZOREG || b == C_SOREG)
			return 1;
		break;
	case C_SOREG:
		if(b == C_ZOREG)
			return 1;
		break;
	case C_NOREG:
		if(b == C_MOREG)
			return 1;
		break;
	case C_LEXT:
		if(b == C_SEXT)
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
			diag("unknown op in build: %A\n", r);
			errorexit();
		case AMOVL:
			buildrep(1, r);
			oprange[AMOVH] = oprange[r];
			repop[AMOVH] = 1;
			oprange[AMOVHU] = oprange[r];
			repop[AMOVHU] = 1;
			oprange[AMOVB] = oprange[r];
			repop[AMOVB] = 1;
			oprange[AMOVBU] = oprange[r];
			repop[AMOVBU] = 1;
			oprange[ALOADSET] = oprange[r];
			repop[ALOADSET] = 1;
			oprange[ALOADM] = oprange[r];
			repop[ALOADM] = 1;
			oprange[ASTOREM] = oprange[r];
			repop[ASTOREM] = 1;

			oprange[AMOVF] = oprange[r];
			repop[AMOVF] = 1;
			oprange[AMOVLD] = oprange[r];
			repop[AMOVLD] = 1;
			oprange[AMOVLF] = oprange[r];
			repop[AMOVLF] = 1;
			oprange[AMOVFD] = oprange[r];
			repop[AMOVFD] = 1;
			oprange[AMOVDF] = oprange[r];
			repop[AMOVDF] = 1;
			oprange[AMOVFL] = oprange[r];
			repop[AMOVFL] = 1;
			oprange[AMOVDL] = oprange[r];
			repop[AMOVDL] = 1;
			break;
		case AADDL:
			buildrep(2, r);
			oprange[ASUBL] = oprange[r];
			repop[ASUBL] = 2;
			oprange[AMULL] = oprange[r];
			repop[AMULL] = 2;
			oprange[AMULUL] = oprange[r];
			repop[AMULUL] = 2;
			oprange[ASUBL] = oprange[r];
			repop[ASUBL] = 2;
			oprange[AANDL] = oprange[r];
			repop[AANDL] = 2;
			oprange[AORL] = oprange[r];
			repop[AORL] = 2;
			oprange[AXORL] = oprange[r];
			repop[AXORL] = 2;
			oprange[AISUBL] = oprange[r];
			repop[AISUBL] = 2;
			oprange[ANANDL] = oprange[r];
			repop[ANANDL] = 2;
			oprange[ANORL] = oprange[r];
			repop[ANORL] = 2;
			oprange[AXNORL] = oprange[r];
			repop[AXNORL] = 2;
			oprange[ASRAL] = oprange[r];
			repop[ASRAL] = 2;
			oprange[ASRLL] = oprange[r];
			repop[ASRLL] = 2;
			oprange[ASLLL] = oprange[r];
			repop[ASLLL] = 2;

			oprange[ACPNEQL] = oprange[r];
			repop[ACPNEQL] = 2;
			oprange[ACPLTUL] = oprange[r];
			repop[ACPLTUL] = 2;
			oprange[ACPLTL] = oprange[r];
			repop[ACPLTL] = 2;
			oprange[ACPLEUL] = oprange[r];
			repop[ACPLEUL] = 2;
			oprange[ACPLEL] = oprange[r];
			repop[ACPLEL] = 2;
			oprange[ACPGTUL] = oprange[r];
			repop[ACPGTUL] = 2;
			oprange[ACPGTL] = oprange[r];
			repop[ACPGTL] = 2;
			oprange[ACPGEUL] = oprange[r];
			repop[ACPGEUL] = 2;
			oprange[ACPGEL] = oprange[r];
			repop[ACPGEL] = 2;
			oprange[ACPEQL] = oprange[r];
			repop[ACPEQL] = 2;

			oprange[AMSTEPL] = oprange[r];
			repop[AMSTEPL] = 2;
			oprange[AMSTEPUL] = oprange[r];
			repop[AMSTEPUL] = 2;
			oprange[AMSTEPLL] = oprange[r];
			repop[AMSTEPLL] = 2;
			oprange[AMULL] = oprange[r];
			repop[AMULL] = 2;
			oprange[AMULUL] = oprange[r];
			repop[AMULUL] = 2;
			oprange[AMULML] = oprange[r];
			repop[AMULML] = 2;
			oprange[AMULMUL] = oprange[r];
			repop[AMULMUL] = 2;

			oprange[ADSTEPL] = oprange[r];
			repop[ADSTEPL] = 2;
			oprange[ADSTEP0L] = oprange[r];
			repop[ADSTEP0L] = 2;
			oprange[ADSTEPLL] = oprange[r];
			repop[ADSTEPLL] = 2;
			oprange[ADSTEPRL] = oprange[r];
			repop[ADSTEPRL] = 2;
			break;
		case AJMPF:
			buildrep(3, r);
			oprange[AJMPT] = oprange[r];
			repop[AJMPT] = 3;
			break;
		case AINV:
			buildrep(4, r);
			oprange[AIRETINV] = oprange[r];
			repop[AIRETINV] = 4;
			break;
		case AIRET:
			buildrep(5, r);
			oprange[AHALT] = oprange[r];
			repop[AHALT] = 5;
			break;
		case ALOADM:
			buildrep(6, r);
			oprange[ALOADSET] = oprange[r];
			repop[ALOADSET] = 6;
			break;
		case AADDD:
			buildrep(7, r);
			oprange[ASUBD] = oprange[r];
			repop[ASUBD] = 7;
			oprange[AMULD] = oprange[r];
			repop[AMULD] = 7;
			oprange[ADIVD] = oprange[r];
			repop[ADIVD] = 7;
			oprange[AADDF] = oprange[r];
			repop[AADDF] = 7;
			oprange[ASUBF] = oprange[r];
			repop[ASUBF] = 7;
			oprange[AMULF] = oprange[r];
			repop[AMULF] = 7;
			oprange[ADIVF] = oprange[r];
			repop[ADIVF] = 7;
			oprange[AGTD] = oprange[r];
			repop[AGTD] = 7;
			oprange[AGED] = oprange[r];
			repop[AGED] = 7;
			oprange[AEQD] = oprange[r];
			repop[AEQD] = 7;
			oprange[AGTF] = oprange[r];
			repop[AGTF] = 7;
			oprange[AGEF] = oprange[r];
			repop[AGEF] = 7;
			oprange[AEQF] = oprange[r];
			repop[AEQF] = 7;
			break;
		case ASTOREM:
		case AJMP:
		case AWORD:
		case ACALL:
		case AJMPFDEC:
		case ATEXT:
		case ADELAY:
		case AEMULATE:
		case ASETIP:
		case AMTSR:
		case AMFSR:
		case AMOVD:
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
						(*p)[a1][a2][a3] = n+1;
			}
		}
	}
	oprange[as].start = 0;
}
