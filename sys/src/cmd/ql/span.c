#include	"l.h"
#define	r0iszero	1

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
				if(p->from3.type == D_CONST) {
					if(p->from3.offset & 3)
						diag("illegal origin\n%P", p);
					if(c > p->from3.offset)
						diag("passed origin (#%lux)\n%P", c, p);
					else
						c = p->from3.offset;
					p->pc = c;
				}
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
	c = rnd(c, 4);

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

	case D_SPR:
		if(a->offset == D_LR)
			return C_LR;
		if(a->offset == D_XER)
			return C_XER;
		if(a->offset == D_CTR)
			return C_CTR;
		return C_SPR;

	case D_DCR:
		return C_SPR;

	case D_SREG:
		return C_SREG;

	case D_FPSCR:
		return C_FPSCR;

	case D_MSR:
		return C_MSR;

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
			if(dlm){
				instoffset = a->sym->value + a->offset;
				switch(a->sym->type){
				case STEXT:
				case SLEAF:
				case SCONST:
				case SUNDEF:
					break;
				default:
					instoffset += INITDAT;
				}
				return C_ADDR;
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
			instoffset = autosize + a->offset + 4L;
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

	case D_OPT:
		instoffset = a->offset & 31L;
		if(a->name == D_NONE)
			return C_SCON;
		return C_GOK;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			instoffset = a->offset;
		consize:
			if(instoffset >= 0) {
				if(r0iszero && instoffset == 0)
					return C_ZCON;
				if(instoffset <= 0x7fff)
					return C_SCON;
				if(instoffset <= 0xffff)
					return C_ANDCON;
				if((instoffset & 0xffff) == 0)
					return C_UCON;
				return C_LCON;
			}
			if(instoffset >= -0x8000)
				return C_ADDCON;
			if((instoffset & 0xffff) == 0)
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
			if(s->type == STEXT || s->type == SLEAF || s->type == SUNDEF) {
				instoffset = s->value + a->offset;
				return C_LCON;
			}
			if(s->type == SCONST) {
				instoffset = s->value + a->offset;
				if(dlm)
					return C_LCON;
				goto consize;
			}
			if(!dlm){
				instoffset = s->value + a->offset - BIG;
				if(instoffset >= -BIG && instoffset < BIG && instoffset != 0)
					return C_SECON;
			}
			instoffset = s->value + a->offset + INITDAT;
			if(dlm)
				return C_LCON;
/* not sure why this barfs */
return C_LCON;
			if(instoffset == 0)
				return C_ZCON;
			if(instoffset >= -0x8000 && instoffset <= 0xffff)
				return C_SCON;
			if((instoffset & 0xffff) == 0)
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
	int a1, a2, a3, a4, r;
	char *c1, *c3, *c4;
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
	a3 = p->from3.class;
	if(a3 == 0) {
		a3 = aclass(&p->from3) + 1;
		p->from3.class = a3;
	}
	a3--;
	a4 = p->to.class;
	if(a4 == 0) {
		a4 = aclass(&p->to) + 1;
		p->to.class = a4;
	}
	a4--;
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
	c4 = xcmp[a4];
	for(; o<e; o++)
		if(o->a2 == a2)
		if(c1[o->a1])
		if(c3[o->a3])
		if(c4[o->a4]) {
			p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %R %R %R %R",
		p->as, a1, a2, a3, a4);
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
		if(b == C_ZCON || b == C_SCON || b == C_UCON || b == C_ADDCON || b == C_ANDCON)
			return 1;
		break;
	case C_ADDCON:
		if(b == C_ZCON || b == C_SCON)
			return 1;
		break;
	case C_ANDCON:
		if(b == C_ZCON || b == C_SCON)
			return 1;
		break;
	case C_SPR:
		if(b == C_LR || b == C_XER || b == C_CTR)
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
	case C_LEXT:
		if(b == C_SEXT)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_SAUTO)
			return 1;
		break;
	case C_REG:
		if(r0iszero && b == C_ZCON)
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
	n = p1->a4 - p2->a4;
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
		case ADCBF:	/* unary indexed: op (b+a); op (b) */
			oprange[ADCBI] = oprange[r];
			oprange[ADCBST] = oprange[r];
			oprange[ADCBT] = oprange[r];
			oprange[ADCBTST] = oprange[r];
			oprange[ADCBZ] = oprange[r];
			oprange[AICBI] = oprange[r];
			break;
		case AECOWX:	/* indexed store: op s,(b+a); op s,(b) */
			oprange[ASTWCCC] = oprange[r];
			break;
		case AREM:	/* macro */
			oprange[AREMCC] = oprange[r];
			oprange[AREMV] = oprange[r];
			oprange[AREMVCC] = oprange[r];
			oprange[AREMU] = oprange[r];
			oprange[AREMUCC] = oprange[r];
			oprange[AREMUV] = oprange[r];
			oprange[AREMUVCC] = oprange[r];
			break;
		case ADIVW:	/* op Rb[,Ra],Rd */
			oprange[AMULHW] = oprange[r];
			oprange[AMULHWCC] = oprange[r];
			oprange[AMULHWU] = oprange[r];
			oprange[AMULHWUCC] = oprange[r];
			oprange[AMULLWCC] = oprange[r];
			oprange[AMULLWVCC] = oprange[r];
			oprange[AMULLWV] = oprange[r];
			oprange[ADIVWCC] = oprange[r];
			oprange[ADIVWV] = oprange[r];
			oprange[ADIVWVCC] = oprange[r];
			oprange[ADIVWU] = oprange[r];
			oprange[ADIVWUCC] = oprange[r];
			oprange[ADIVWUV] = oprange[r];
			oprange[ADIVWUVCC] = oprange[r];
			oprange[AADDCC] = oprange[r];
			oprange[AADDCV] = oprange[r];
			oprange[AADDCVCC] = oprange[r];
			oprange[AADDV] = oprange[r];
			oprange[AADDVCC] = oprange[r];
			oprange[AADDE] = oprange[r];
			oprange[AADDECC] = oprange[r];
			oprange[AADDEV] = oprange[r];
			oprange[AADDEVCC] = oprange[r];
			oprange[ACRAND] = oprange[r];
			oprange[ACRANDN] = oprange[r];
			oprange[ACREQV] = oprange[r];
			oprange[ACRNAND] = oprange[r];
			oprange[ACRNOR] = oprange[r];
			oprange[ACROR] = oprange[r];
			oprange[ACRORN] = oprange[r];
			oprange[ACRXOR] = oprange[r];
			oprange[AMULCHW] = oprange[r];
			oprange[AMULCHWCC] = oprange[r];
			oprange[AMULCHWU] = oprange[r];
			oprange[AMULCHWUCC] = oprange[r];
			oprange[AMULHHW] = oprange[r];
			oprange[AMULHHWCC] = oprange[r];
			oprange[AMULHHWU] = oprange[r];
			oprange[AMULHHWUCC] = oprange[r];
			oprange[AMULLHW] = oprange[r];
			oprange[AMULLHWCC] = oprange[r];
			oprange[AMULLHWU] = oprange[r];
			oprange[AMULLHWUCC] = oprange[r];
			break;
		case AMACCHW:	/* strictly 3 registers */
			oprange[AMACCHWCC] = oprange[r];
			oprange[AMACCHWS] = oprange[r];
			oprange[AMACCHWSCC] = oprange[r];
			oprange[AMACCHWSU] = oprange[r];
			oprange[AMACCHWSUCC] = oprange[r];
			oprange[AMACCHWSUV] = oprange[r];
			oprange[AMACCHWSUVCC] = oprange[r];
			oprange[AMACCHWSV] = oprange[r];
			oprange[AMACCHWSVCC] = oprange[r];
			oprange[AMACCHWU] = oprange[r];
			oprange[AMACCHWUCC] = oprange[r];
			oprange[AMACCHWUV] = oprange[r];
			oprange[AMACCHWUVCC] = oprange[r];
			oprange[AMACCHWV] = oprange[r];
			oprange[AMACCHWVCC] = oprange[r];
			oprange[AMACHHW] = oprange[r];
			oprange[AMACHHWCC] = oprange[r];
			oprange[AMACHHWS] = oprange[r];
			oprange[AMACHHWSCC] = oprange[r];
			oprange[AMACHHWSU] = oprange[r];
			oprange[AMACHHWSUCC] = oprange[r];
			oprange[AMACHHWSUV] = oprange[r];
			oprange[AMACHHWSUVCC] = oprange[r];
			oprange[AMACHHWSV] = oprange[r];
			oprange[AMACHHWSVCC] = oprange[r];
			oprange[AMACHHWU] = oprange[r];
			oprange[AMACHHWUCC] = oprange[r];
			oprange[AMACHHWUV] = oprange[r];
			oprange[AMACHHWUVCC] = oprange[r];
			oprange[AMACHHWV] = oprange[r];
			oprange[AMACHHWVCC] = oprange[r];
			oprange[AMACLHW] = oprange[r];
			oprange[AMACLHWCC] = oprange[r];
			oprange[AMACLHWS] = oprange[r];
			oprange[AMACLHWSCC] = oprange[r];
			oprange[AMACLHWSU] = oprange[r];
			oprange[AMACLHWSUCC] = oprange[r];
			oprange[AMACLHWSUV] = oprange[r];
			oprange[AMACLHWSUVCC] = oprange[r];
			oprange[AMACLHWSV] = oprange[r];
			oprange[AMACLHWSVCC] = oprange[r];
			oprange[AMACLHWU] = oprange[r];
			oprange[AMACLHWUCC] = oprange[r];
			oprange[AMACLHWUV] = oprange[r];
			oprange[AMACLHWUVCC] = oprange[r];
			oprange[AMACLHWV] = oprange[r];
			oprange[AMACLHWVCC] = oprange[r];
			oprange[ANMACCHW] = oprange[r];
			oprange[ANMACCHWCC] = oprange[r];
			oprange[ANMACCHWS] = oprange[r];
			oprange[ANMACCHWSCC] = oprange[r];
			oprange[ANMACCHWSV] = oprange[r];
			oprange[ANMACCHWSVCC] = oprange[r];
			oprange[ANMACCHWV] = oprange[r];
			oprange[ANMACCHWVCC] = oprange[r];
			oprange[ANMACHHW] = oprange[r];
			oprange[ANMACHHWCC] = oprange[r];
			oprange[ANMACHHWS] = oprange[r];
			oprange[ANMACHHWSCC] = oprange[r];
			oprange[ANMACHHWSV] = oprange[r];
			oprange[ANMACHHWSVCC] = oprange[r];
			oprange[ANMACHHWV] = oprange[r];
			oprange[ANMACHHWVCC] = oprange[r];
			oprange[ANMACLHW] = oprange[r];
			oprange[ANMACLHWCC] = oprange[r];
			oprange[ANMACLHWS] = oprange[r];
			oprange[ANMACLHWSCC] = oprange[r];
			oprange[ANMACLHWSV] = oprange[r];
			oprange[ANMACLHWSVCC] = oprange[r];
			oprange[ANMACLHWV] = oprange[r];
			oprange[ANMACLHWVCC] = oprange[r];
			break;
/* floating point move *//*
			oprange[AFMR] = oprange[r];
			oprange[AFMRCC] = oprange[r];
*/
/**/
		case AMOVBZ:	/* lbz, stz, rlwm(r/r), lhz, lha, stz, and x variants */
			oprange[AMOVH] = oprange[r];
			oprange[AMOVHZ] = oprange[r];
			break;
		case AMOVBZU:	/* lbz[x]u, stb[x]u, lhz[x]u, lha[x]u, sth[u]x */
			oprange[AMOVHU] = oprange[r];
			oprange[AMOVHZU] = oprange[r];
			oprange[AMOVWU] = oprange[r];
			oprange[AMOVMW] = oprange[r];
			break;
		case AAND:	/* logical op Rb,Rs,Ra; no literal */
			oprange[AANDN] = oprange[r];
			oprange[AANDNCC] = oprange[r];
			oprange[AEQV] = oprange[r];
			oprange[AEQVCC] = oprange[r];
			oprange[ANAND] = oprange[r];
			oprange[ANANDCC] = oprange[r];
			oprange[ANOR] = oprange[r];
			oprange[ANORCC] = oprange[r];
			oprange[AORCC] = oprange[r];
			oprange[AORN] = oprange[r];
			oprange[AORNCC] = oprange[r];
			oprange[AXORCC] = oprange[r];
			break;
		case AADDME:	/* op Ra, Rd */
			oprange[AADDMECC] = oprange[r];
			oprange[AADDMEV] = oprange[r];
			oprange[AADDMEVCC] = oprange[r];
			oprange[AADDZE] = oprange[r];
			oprange[AADDZECC] = oprange[r];
			oprange[AADDZEV] = oprange[r];
			oprange[AADDZEVCC] = oprange[r];
			oprange[ASUBME] = oprange[r];
			oprange[ASUBMECC] = oprange[r];
			oprange[ASUBMEV] = oprange[r];
			oprange[ASUBMEVCC] = oprange[r];
			oprange[ASUBZE] = oprange[r];
			oprange[ASUBZECC] = oprange[r];
			oprange[ASUBZEV] = oprange[r];
			oprange[ASUBZEVCC] = oprange[r];
			break;
		case AADDC:
			oprange[AADDCCC] = oprange[r];
			break;
		case ABEQ:
			oprange[ABGE] = oprange[r];
			oprange[ABGT] = oprange[r];
			oprange[ABLE] = oprange[r];
			oprange[ABLT] = oprange[r];
			oprange[ABNE] = oprange[r];
			oprange[ABVC] = oprange[r];
			oprange[ABVS] = oprange[r];
			break;
		case ABR:
			oprange[ABL] = oprange[r];
			break;
		case ABC:
			oprange[ABCL] = oprange[r];
			break;
		case AEXTSB:	/* op Rs, Ra */
			oprange[AEXTSBCC] = oprange[r];
			oprange[AEXTSH] = oprange[r];
			oprange[AEXTSHCC] = oprange[r];
			oprange[ACNTLZW] = oprange[r];
			oprange[ACNTLZWCC] = oprange[r];
			break;
		case AFABS:	/* fop [s,]d */
			oprange[AFABSCC] = oprange[r];
			oprange[AFNABS] = oprange[r];
			oprange[AFNABSCC] = oprange[r];
			oprange[AFNEG] = oprange[r];
			oprange[AFNEGCC] = oprange[r];
			oprange[AFRSP] = oprange[r];
			oprange[AFRSPCC] = oprange[r];
			oprange[AFCTIW] = oprange[r];
			oprange[AFCTIWCC] = oprange[r];
			oprange[AFCTIWZ] = oprange[r];
			oprange[AFCTIWZCC] = oprange[r];
			break;
		case AFADD:
			oprange[AFADDS] = oprange[r];
			oprange[AFADDCC] = oprange[r];
			oprange[AFADDSCC] = oprange[r];
			oprange[AFDIV] = oprange[r];
			oprange[AFDIVS] = oprange[r];
			oprange[AFDIVCC] = oprange[r];
			oprange[AFDIVSCC] = oprange[r];
			oprange[AFSUB] = oprange[r];
			oprange[AFSUBS] = oprange[r];
			oprange[AFSUBCC] = oprange[r];
			oprange[AFSUBSCC] = oprange[r];
			break;
		case AFMADD:
			oprange[AFMADDCC] = oprange[r];
			oprange[AFMADDS] = oprange[r];
			oprange[AFMADDSCC] = oprange[r];
			oprange[AFMSUB] = oprange[r];
			oprange[AFMSUBCC] = oprange[r];
			oprange[AFMSUBS] = oprange[r];
			oprange[AFMSUBSCC] = oprange[r];
			oprange[AFNMADD] = oprange[r];
			oprange[AFNMADDCC] = oprange[r];
			oprange[AFNMADDS] = oprange[r];
			oprange[AFNMADDSCC] = oprange[r];
			oprange[AFNMSUB] = oprange[r];
			oprange[AFNMSUBCC] = oprange[r];
			oprange[AFNMSUBS] = oprange[r];
			oprange[AFNMSUBSCC] = oprange[r];
			break;
		case AFMUL:
			oprange[AFMULS] = oprange[r];
			oprange[AFMULCC] = oprange[r];
			oprange[AFMULSCC] = oprange[r];
			break;
		case AFCMPO:
			oprange[AFCMPU] = oprange[r];
			break;
		case AMTFSB0:
			oprange[AMTFSB0CC] = oprange[r];
			oprange[AMTFSB1] = oprange[r];
			oprange[AMTFSB1CC] = oprange[r];
			break;
		case ANEG:	/* op [Ra,] Rd */
			oprange[ANEGCC] = oprange[r];
			oprange[ANEGV] = oprange[r];
			oprange[ANEGVCC] = oprange[r];
			break;
		case AOR:	/* or/xor Rb,Rs,Ra; ori/xori $uimm,Rs,Ra; oris/xoris $uimm,Rs,Ra */
			oprange[AXOR] = oprange[r];
			break;
		case ASLW:
			oprange[ASLWCC] = oprange[r];
			oprange[ASRW] = oprange[r];
			oprange[ASRWCC] = oprange[r];
			break;
		case ASRAW:	/* sraw Rb,Rs,Ra; srawi sh,Rs,Ra */
			oprange[ASRAWCC] = oprange[r];
			break;
		case ASUB:	/* SUB Ra,Rb,Rd => subf Rd,ra,rb */
			oprange[ASUB] = oprange[r];
			oprange[ASUBCC] = oprange[r];
			oprange[ASUBV] = oprange[r];
			oprange[ASUBVCC] = oprange[r];
			oprange[ASUBCCC] = oprange[r];
			oprange[ASUBCV] = oprange[r];
			oprange[ASUBCVCC] = oprange[r];
			oprange[ASUBE] = oprange[r];
			oprange[ASUBECC] = oprange[r];
			oprange[ASUBEV] = oprange[r];
			oprange[ASUBEVCC] = oprange[r];
			break;
		case ASYNC:
			oprange[AISYNC] = oprange[r];
			break;
		case ARLWMI:
			oprange[ARLWMICC] = oprange[r];
			oprange[ARLWNM] = oprange[r];
			oprange[ARLWNMCC] = oprange[r];
			break;
		case AFMOVD:
			oprange[AFMOVDCC] = oprange[r];
			oprange[AFMOVDU] = oprange[r];
			oprange[AFMOVS] = oprange[r];
			oprange[AFMOVSU] = oprange[r];
			break;
		case AECIWX:
			oprange[ALWAR] = oprange[r];
			break;
		case ASYSCALL:	/* just the op; flow of control */
			oprange[ARFI] = oprange[r];
			oprange[ARFCI] = oprange[r];
			break;
		case AMOVHBR:
			oprange[AMOVWBR] = oprange[r];
			break;
		case AADD:
		case AANDCC:	/* and. Rb,Rs,Ra; andi. $uimm,Rs,Ra; andis. $uimm,Rs,Ra */
		case ACMP:
		case ACMPU:
		case AEIEIO:
		case ALSW:
		case AMOVB:	/* macro: move byte with sign extension */
		case AMOVBU:	/* macro: move byte with sign extension & update */
		case AMOVW:
		case AMOVFL:
		case AMULLW:	/* op $s[,r2],r3; op r1[,r2],r3; no cc/v */
		case ASUBC:	/* op r1,$s,r3; op r1[,r2],r3 */
		case ASTSW:
		case ATLBIE:
		case ATW:
		case AWORD:
		case ANOP:
		case ATEXT:
			break;
		}
	}
}

enum{
	ABSD = 0,
	ABSU = 1,
	RELD = 2,
	RELU = 3,
};

int modemap[8] = { 0, 1, -1, 2, 3, 4, 5, 6};

typedef struct Reloc Reloc;

struct Reloc
{
	int n;
	int t;
	uchar *m;
	ulong *a;
};

Reloc rels;

static void
grow(Reloc *r)
{
	int t;
	uchar *m, *nm;
	ulong *a, *na;

	t = r->t;
	r->t += 64;
	m = r->m;
	a = r->a;
	r->m = nm = malloc(r->t*sizeof(uchar));
	r->a = na = malloc(r->t*sizeof(ulong));
	memmove(nm, m, t*sizeof(uchar));
	memmove(na, a, t*sizeof(ulong));
	free(m);
	free(a);
}

void
dynreloc(Sym *s, long v, int abs, int split, int sext)
{
	int i, k, n;
	uchar *m;
	ulong *a;
	Reloc *r;

	if(v&3)
		diag("bad relocation address");
	v >>= 2;
	if(s->type == SUNDEF)
		k = abs ? ABSU : RELU;
	else
		k = abs ? ABSD : RELD;
	if(split)
		k += 4;
	if(sext)
		k += 2;
	/* Bprint(&bso, "R %s a=%ld(%lx) %d\n", s->name, a, a, k); */
	k = modemap[k];
	r = &rels;
	n = r->n;
	if(n >= r->t)
		grow(r);
	m = r->m;
	a = r->a;
	for(i = n; i > 0; i--){
		if(v < a[i-1]){	/* happens occasionally for data */
			m[i] = m[i-1];
			a[i] = a[i-1];
		}
		else
			break;
	}
	m[i] = k;
	a[i] = v;
	r->n++;
}

static int
sput(char *s)
{
	char *p;

	p = s;
	while(*s)
		cput(*s++);
	cput(0);
	return s-p+1;
}

void
asmdyn()
{
	int i, n, t, c;
	Sym *s;
	ulong la, ra, *a;
	vlong off;
	uchar *m;
	Reloc *r;

	cflush();
	off = seek(cout, 0, 1);
	lput(0);
	t = 0;
	lput(imports);
	t += 4;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->type == SUNDEF){
				lput(s->sig);
				t += 4;
				t += sput(s->name);
			}
	
	la = 0;
	r = &rels;
	n = r->n;
	m = r->m;
	a = r->a;
	lput(n);
	t += 4;
	for(i = 0; i < n; i++){
		ra = *a-la;
		if(*a < la)
			diag("bad relocation order");
		if(ra < 256)
			c = 0;
		else if(ra < 65536)
			c = 1;
		else
			c = 2;
		cput((c<<6)|*m++);
		t++;
		if(c == 0){
			cput(ra);
			t++;
		}
		else if(c == 1){
			wput(ra);
			t += 2;
		}
		else{
			lput(ra);
			t += 4;
		}
		la = *a++;
	}

	cflush();
	seek(cout, off, 0);
	lput(t);

	if(debug['v']){
		Bprint(&bso, "import table entries = %d\n", imports);
		Bprint(&bso, "export table entries = %d\n", exports);
	}
}
