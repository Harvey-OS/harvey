#include	"l.h"

static int spaninit;

/*
 * called twice,
 * once before noops, once after
 * assumes noops will not extend the branches at all
 */
void
span(int first)
{
	Prog *p;
	Optab *o;
	long c, memc, ramc, idat, rdat, etext;
	int m, again;

	if(first){
		xdefine("etext", STEXT, textstart);
		xdefine("_ramtext", STEXT, ramstart);
		xdefine("_ramsize", STEXT, 0);
		spaninit = 1;
	}
	etext = 0;

loop:
	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);

	c = textstart;
	memc = textstart;
	ramc = ramstart;
	idat = datstart;
	rdat = ramdstart;
	again = 0;
	curtext = P;
	for(p = firstp; p != P; p = p->link){
		o = opsearch(p);
		m = o->size;
		if(m == 0) {
			if(p->as == ATEXT){
				if(curtext && curtext->from.sym->isram)
					ramc = c;
				else
					memc = c;
				curtext = p;
				c = memc;
				if(p->from.sym->isram)
					c = ramc;
				p->from.sym->value = c;
				if(p->pc != c)
					again = 1;
				p->pc = c;
				continue;
			}
			if(p->as != ANOP && p->as != ADOEND)
				diag("zero-width instruction\n%P\n", p);
		}
		if(p->pc != c){
			again = 1;
			p->pc = c;
		}
		c += m;
		if(first && !p->nosched)
			c += o->nops * 4;
	}
	if(curtext && curtext->from.sym->isram)
		ramc = c;
	else
		memc = c;
	curtext = P;

	c = rnd(memc, 8);
	ramdstart = ramc;
	if(datrnd)
		datstart = rnd(c, datrnd);

	xdefine("etext", STEXT, c);
	if(spaninit || again || idat != datstart || rdat != ramdstart || etext != c){
		spaninit = 0;
		etext = c;
		assignram();
		xdefine("_ramtext", STEXT, ramc);
		xdefine("_ramsize", STEXT, ramend-ramstart);
		goto loop;
	}
	textsize = c - textstart;
	if(debug['v']){
		Bprint(&bso, "tsize = 0x%lux\n", textsize);
		Bprint(&bso, "rtextsize = 0x%lux\n", ramdstart-ramstart);
		Bprint(&bso, "rdatsize = 0x%lux\n", ramend-ramdstart);
		Bprint(&bso, "ramover = 0x%lux\n", ramover);
	}
	Bflush(&bso);
}

/*
 * assign small data to internal ram
 */
void
assignram(void)
{
	Sym *s;
	long c;

	c = ramdstart;
	for(s = ram; s; s = s->ram){
		if(s->type != SDATA && s->type != SSDATA)
			continue;
		if(c+s->size >= maxram)
			break;
		s->type = SSDATA;
		s->value = c;
		c += s->size;
	}
	ramend = c;

	ramover = 0;
	c = 0;
	for(; s; s = s->ram){
		if(s->type == SSDATA){
			s->type = SDATA;
			s->value = c;
			c += s->size;
		}
		if(s->type == SDATA)
			ramover += s->size;
	}
}

void
xdefine(char *p, int t, long v)
{
	Sym *s;

	s = lookup(p, 0);
	s->type = t;
	s->value = v;
}

long
regoff(Prog *p, Adr *a)
{

	offset = 0;
	aclass(p, a);
	return offset;
}

Optab*
oplook(Prog *p)
{
	int op;

	op = p->optab;
	if(op)
		return &optab[op-1];
	diag("oplook with no opcode\n");
	errorexit();
	return 0;
}

Optab*
opsearch(Prog *p)
{
	int a1, a2, a3, a4, a5, r;
	char *c1, *c2, *c3, *c4, *c5;
	Optab *o, *e;

	if(p->onlyop && p->optab)
		return &optab[p->optab-1];

	p->onlyop = 1;
	a1 = aclass(p, &p->from);
	p->from.class = a1+1;

	if(p->isf > 1)
		a2 = aclass(p, &p->from1);
	else
		a2 = C_NONE;
	p->from1.class = a2 + 1;

	if(p->isf > 2)
		a3 = aclass(p, &p->from2);
	else
		a3 = C_NONE;
	p->from2.class = a3 + 1;

	a5 = aclass(p, &p->to);
	p->to.class = a5 + 1;

	a4 = C_NONE;
	if(p->reg != NREG){
		if(p->reg == REGPC || p->reg == REGPCSH)
			a4 = C_REGPC;
		else
			a4 = C_REG;
	}
	r = p->as;
	o = oprange[r].start;
	if(o == 0)
		o = oprange[r].stop; /* just generate an error */
	e = oprange[r].stop;
	c1 = xcmp[a1];
	c2 = xcmp[a2];
	c3 = xcmp[a3];
	c4 = xcmp[a4];
	c5 = xcmp[a5];
	for(; o<e; o++)
		if(c2[o->a2])
		if(c1[o->a1])
		if(c3[o->a3])
		if(c4[o->a4])
		if(c5[o->a5]) {
			p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %R %R %R %R %R\n",
		p->as, a1, a2, a3, a4, a5);
	prasm(p);
	if(o == 0)
		errorexit();
	return o;
}

int
aclass(Prog *p, Adr *a)
{
	Sym *s;
	int t;

	switch(a->type) {
	case D_NONE:
		return C_NONE;

	case D_REG:
		if(a->reg == REGPC || a->reg == REGPCSH)
			return C_REGPC;
		return C_REG;

	case D_FREG:
		return C_FREG;

	case D_CREG:
		return C_CREG;

	case D_INDREG:
		if(a->reg <= REGPTR)
			return C_INDF;
		return C_IND;

	case D_INC:
	case D_DEC:
		if(a->reg <= REGPTR)
			return C_INDFM;
		return C_INDM;

	case D_INCREG:
		if(a->reg <= REGPTR && a->increg > REGPTR && a->increg <= 19)
			return C_INDFR;
		return C_INDR;

	case D_OREG:
		offset = a->offset;
		if(offset >= -BIG && offset < BIG)
			return C_SOREG;
		if(offset >= 0 && offset <= 0xffffff)
			return C_MOREG;
		return C_LOREG;

	case D_NAME:
		p->onlyop = 0;
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			t = SSDATA;
			offset = a->offset;
			if(s){
				t = s->type;
				if(t == 0 || t == SXREF) {
					diag("undefined external: %s in %s\n",
						a->sym->name, TNAME);
					a->sym->type = SDATA;
				}
				offset += s->value;
			}else
				offset += ramoffset;
			if(spaninit)
				return C_LEXT;
			if(t == SSDATA){
				offset -= ramoffset;
				if(offset >= 0 && offset <= 0xffff)
					return C_SEXT;
				diag("ram offset for %s out of range: 0x%lux\n",
					a->sym->name, offset);
			}else if(t == SDATA || t == SBSS){
				offset += datstart;
				if(!s->isram)
					offset += ramover;
			}
			if(offset >= 0 && offset <= 0xffffff)
				return C_MEXT;
			if(!datrnd && (offset & 0xffff) == 0)
				return C_MEXT;
			return C_LEXT;
		}
		return C_GOK;

	case D_CONST:
		switch(a->name) {
		case D_NONE:
			offset = a->offset;
			return consize(offset, 1);

		case D_EXTERN:
		case D_STATIC:
			p->onlyop = 0;
			s = a->sym;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s\n",
					s->name, TNAME);
				s->type = SDATA;
			}
			if(spaninit)
				return C_LCON;
			offset = s->value + a->offset;
			if(t == SDATA || t == SBSS){
				offset += datstart;
				if(!s->isram)
					offset += ramover;
			}
			return consize(offset, !datrnd);
		case D_AUTO:
		case D_PARAM:
			offset = a->offset;
			if(offset >= -BIG && offset < BIG)
				return C_SACON;
			return C_LACON;
		}
		diag("bad name %N (%d) in const\n", a, a->name);
		return C_GOK;

	case D_BRANCH:
		p->onlyop = 0;
		if(spaninit)
			return C_LBRA;
		/*
		 * only small jumps are pc-relative
		 * cond == 0 ==> jump to self
		 */
		offset = -8;
		if(p->cond)
			offset = p->cond->pc - (p->pc + 8);
		if(offset >= -BIG && offset < BIG)
			return C_SBRA;
		offset = p->cond->pc;
		if(offset >= 0 && offset <= 0xffffff)
			return C_MBRA;
		return C_LBRA;
	}
	return C_GOK;
}

int
consize(long offset, int hcon)
{
	if(offset == 0)
		return C_ZCON;
	if(offset >= -BIG && offset < BIG)
		return C_SCON;
	if(hcon && (offset & 0xffff) == 0)
		return C_HCON;
	if(offset >= 0 && offset <= 0xffffff)
		return C_MCON;
	return C_LCON;
}

int
cmp(int a, int b)
{

	if(a == b)
		return 1;
	switch(a) {
	case C_SCON:
		return b == C_ZCON;
	case C_HCON:
		return b == C_ZCON;
	case C_MCON:
		return b == C_ZCON || b == C_SCON || b == C_HCON;
	case C_LCON:
		return b == C_ZCON || b == C_SCON || b == C_HCON || b == C_MCON;
	case C_LACON:
		return b == C_SACON;
	case C_MBRA:
		return b == C_SBRA;
	case C_LBRA:
		return b == C_SBRA || b == C_MBRA;
	case C_MEXT:
		return b == C_SEXT;
	case C_LEXT:
		return b == C_SEXT || b == C_MEXT;
	case C_REG:
		return b == C_ZCON;
	case C_REGPC:
		return b == C_ZCON || b == C_REG;
	case C_SOREG:
		return b == C_IND || b == C_INDF;
	case C_MOREG:
		return b == C_IND || b == C_INDF || b == C_SOREG;
	case C_LOREG:
		return b == C_IND || b == C_INDF || b == C_SOREG || b == C_MOREG;
	case C_IND:
		return b == C_INDF || b == C_INDFM || b == C_INDFR
			|| b == C_INDM || b == C_INDR;
	case C_INDF:
		return b == C_INDFM || b == C_INDFR;
	case C_FOP:
		return b == C_INDFM || b == C_INDFR || b == C_INDF || b == C_FREG;
	case C_FST:
		return b == C_INDFM || b == C_INDFR || b == C_INDF || b == C_NONE;
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
	n = p1->a5 - p2->a5;
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
			diag("unknown op in build: %A\n", r);
			errorexit();
		case AADD:
			oprange[AADDH] = oprange[r];
			break;
		case AXOR:
			oprange[AXORH] = oprange[r];
			oprange[AADDCR] = oprange[r];
			oprange[AADDCRH] = oprange[r];
			oprange[AAND] = oprange[r];
			oprange[AANDH] = oprange[r];
			oprange[AAND] = oprange[r];
			oprange[AANDH] = oprange[r];
			oprange[AANDN] = oprange[r];
			oprange[AANDNH] = oprange[r];
			oprange[AORH] = oprange[r];
			oprange[ASUB] = oprange[r];
			oprange[ASUBH] = oprange[r];
			oprange[ASUBR] = oprange[r];
			oprange[ASUBRH] = oprange[r];

			oprange[ASLL] = oprange[r];
			oprange[ASLLH] = oprange[r];
			oprange[ASRA] = oprange[r];
			oprange[ASRAH] = oprange[r];
			oprange[ASRL] = oprange[r];
			oprange[ASRLH] = oprange[r];
			break;
		case ACMP:
			oprange[ACMPH] = oprange[r];
			oprange[ABIT] = oprange[r];
			oprange[ABITH] = oprange[r];
			break;
		case AROR:
			oprange[ARORH] = oprange[r];
			oprange[AROL] = oprange[r];
			oprange[AROLH] = oprange[r];
			break;
		case AFRND:
			oprange[AFIFEQ] = oprange[r];
			oprange[AFIFGT] = oprange[r];
			oprange[AFIFLT] = oprange[r];
			oprange[AFIEEE] = oprange[r];
			oprange[AFSEED] = oprange[r];
			oprange[AFMOVFB] = oprange[r];
			oprange[AFMOVFW] = oprange[r];
			oprange[AFMOVFH] = oprange[r];
			oprange[AFMOVBF] = oprange[r];
			oprange[AFMOVWF] = oprange[r];
			oprange[AFMOVHF] = oprange[r];
			break;
		case AFADD:
			oprange[AFSUB] = oprange[r];
			oprange[AFADDN] = oprange[r];
			oprange[AFSUBN] = oprange[r];
			break;
		case AFADDT:
			oprange[AFADDTN] = oprange[r];
			oprange[AFSUBT] = oprange[r];
			oprange[AFSUBTN] = oprange[r];
			break;
		case AFMUL:
			oprange[AFMULN] = oprange[r];
			break;
		case AFMULT:
			oprange[AFMULTN] = oprange[r];
			break;
		case AFMADD:
			oprange[AFMADDN] = oprange[r];
			oprange[AFMSUB] = oprange[r];
			oprange[AFMSUBN] = oprange[r];
			break;
		case AFMADDT:
			oprange[AFMADDTN] = oprange[r];
			oprange[AFMSUBT] = oprange[r];
			oprange[AFMSUBTN] = oprange[r];
			break;
		case AFMOVF:
			oprange[AFMOVFN] = oprange[r];
			break;

		case ADO:
			oprange[ADOLOCK] = oprange[r];
			break;

		case AOR:	/* special case for shift-or */
		case AFDSP:
		case AMOVW:
		case AMOVB:
		case AMOVH:
		case AMOVHU:
		case AMOVBU:
		case AMOVHB:
		case AWORD:
		case AIRET:
		case AWAITI:
		case ASFTRST:
		case ACALL:
		case AJMP:
		case ABRA:
		case ADBRA:
		case ABMOVW:
		case ANOP:
		case ADOEND:
		case ATEXT:
			break;
		}
	}
}
