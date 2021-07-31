#include	"l.h"

void
span(void)
{
	Prog *p, *q;
	long v, c, idat;
	int m, n, mf, mt;
	Optab *o;

	xdefine("etext", STEXT, 0L);
	idat = INITDAT;
	m = 0; /* set */

start:
	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		o = &optab[p->as];
		p->pc = c;

		switch(o->type) {
		default:
			diag("zero-width instruction\n%P\n", p);
			break;

		case TTEXT:
			m = 0;
			autosize = p->to.offset;
			p->back = 0;
			break;

		case TWORD:
			m = 2;
			p->back = 0;
			break;

		case TLONG:
			m = 4;
			p->back = 0;
			break;

		case TPCTO:
			m = 2;
			q = p->cond;
			if(p->to.type != D_BRANCH) {
				diag("not a branch\n%P\n", p);
				q = p;
				p->cond = q;
			}
			n = 0;
			if(q != P)
				if(q->back != 2)
					n = 1;
			p->back = n;
			break;

		case TNNAD:
			m = 2;
			p->back = 0;
			break;

		case TFLOT:
		case TDYAD:
			m = 2;
			mf = andsize(p, &p->from);
			mt = andsize(p, &p->to);
			m += anddec(mf, mt, o->decod);
			p->back = 0;
			break;

		case TDMON:
			if(p->from.type != D_NONE || p->to.type == D_NONE)
				diag("unary op required in dst\n%P\n", p);
			m = and10size(p);
			p->back = 0;
			break;
		}
		c += m;
		p->mark = m;
	}
	n = 0;

loop:
	n++;
	if(debug['v'])
		Bprint(&bso, "%5.2f pass %d\n", cputime(), n);
	if(n >= 40) {
		diag("too many passes in span\n");
		errorexit();
	}

	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->pc != c)
			break;
		if((q = p->cond) != P) {
			if((m = p->mark) == 0)
				continue;
			v = q->pc;
			p->to.offset = v;
			v -= c;
			c += m;
			if(v < JMPLO || v > 1022)
				c += 4;
			if(p->as == AJMP)
				c = rnd(c, AMOD);
			continue;
		}
		c += p->mark;
		if(p->as == ARETURN)
			c = rnd(c, AMOD);
	}
	if(p == P)
		goto done;
	for(; p != P; p = p->link) {
		if((q = p->cond) != P) {
			if((m = p->mark) == 0) {
				p->pc = c;
				continue;
			}
			v = q->pc;
			if(p->back)
				v -= c;
			else
				v -= p->pc;
			p->pc = c;
			c += m;
			if(v < JMPLO || v > 1022)
				c += 4;
			if(p->as == AJMP)
				c = rnd(c, AMOD);
			continue;
		}
		p->pc = c;
		c += p->mark;
		if(p->as == ARETURN)
			c = rnd(c, AMOD);
	}
	goto loop;

done:
	c = rnd(c, 4);
	if(c != textsize) {
		textsize = c;
		goto loop;
	}

	/*
	 * Mask3 bug
	 * if an ENTER is near a page boundary, pad it out
	 * with NOPs to move it over the boundary
	 */
	for(p = firstp, q = P; p != P; q = p, p = p->link) {
		if(p->as == AENTER && ((p->pc & 0xFFF) > 0xFF0)){
			for(n = 16-(p->pc & 0x0F); n; n -= 2, p->pc += 2){
				Prog *p1 = prg();
				p1->as = ANOP;
				p1->link = q->link;
				q->link = p1;
			}
			goto start;
		}
	}

	/*
	 * for old boot ROMs, make sure the text is
	 * rounded up to a 4-byte boundary.
	 */
	c = rnd(c, 4);
	if(INITRND)
		INITDAT = rnd(c, INITRND);
	if(INITDAT != idat) {
		idat = INITDAT;
		goto start;
	}
	xdefine("etext", STEXT, c);
	if(debug['v'])
		Bprint(&bso, "etext = %lux\n", c);
	for(p = textp; p != P; p = p->cond)
		p->from.sym->value = p->pc;
	textsize = c - INITTEXT;
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
	if(s->type == STEXT && s->value == 0)
		s->value = v;
}

int
andsize(Prog *p, Adr *a)
{
	int n, t;
	long v;

	if(a->type & D_INDIR) {
		a->type &= ~D_INDIR;
		switch(andsize(p, a)) {
		default:
			diag("unknown indirection\n%P\n", p);

		case S_NEG5:
		case S_IMM5:
		case S_UNS5:
		case S_XOR5:
		case S_WAI5:
		case S_IMM16:
		case S_IMM32:
			n = S_ABS32;
			if((operand ^ INITTEXT) & SEGBITS)
				break;
			v = operand & ~SEGBITS;
			if(v >= 0 && v < 65536L)
				n = S_ABS16;
			break;
		case S_STK5:
			n = S_ISTK5;
			break;
		case S_STK16:
			n = S_ISTK16;
			break;
		case S_STK32:
			n = S_ISTK32;
			break;
		}
		a->type |= D_INDIR;
		return n;
	}
	n = a->type;
	switch(n) {

	default:
		diag("unknown type in andsize\n%P\n", p);
		break;

	case D_EXTERN:
	case D_STATIC:
		t = a->sym->type;
		if(t == 0 || t == SXREF) {
			diag("undefined external: %s\n%P\n", a->sym->name, p);
			t = SDATA;
			a->sym->type = t;
		}
		operand = a->sym->value + a->offset;
		if(t == STEXT)
			return S_ABS32;	/* botch */
		operand += INITDAT;
		if((operand ^ INITTEXT) & SEGBITS)
			return S_ABS32;
		v = operand & ~SEGBITS;
		if(v >= 0 && v < 65536L)
			return S_ABS16;
		return S_ABS32;

	case D_FCONST:
		operand = ieeedtof(&a->ieee);
		return S_IMM32;

	case D_CONST:
		operand = a->offset;
		if(operand >= -16 && operand < 0)
			return S_NEG5;
		if(operand >= 0 && operand < 16)
			return S_IMM5;
		if((operand&3) == 0) {
			if(operand >= 16 && operand < 32)
				return S_XOR5;
			if(operand >= 32 && operand < 128)
				return S_WAI5;
		} else
			if(operand >= 16 && operand < 32)
				return S_UNS5;
		if(operand >= -32768L && operand < 32768L)
			return S_IMM16;
		return S_IMM32;

	case D_ADDR:
		t = a->sym->type;
		if(t == 0 || t == SXREF) {
			diag("undefined external: %s\n%P\n", a->sym->name, p);
			t = SDATA;
			a->sym->type = t;
		}
		operand = a->sym->value + a->offset;
		if(t == STEXT)
			return S_IMM32;	/* botch */
		operand += INITDAT;
		if(operand >= -32768L && operand < 32768L)
			return S_IMM16;
		return S_IMM32;

	case D_AUTO:
		operand = autosize + 0 + a->offset;
		goto stk;

	case D_PARAM:
		operand = autosize + 4 + a->offset;
		goto stk;

	case D_REG:
		operand = a->offset;

	stk:
		if((operand & 3) == 0)
		if(operand >= 0 && operand <= 124)
		if(a->width == W_W || a->width == W_NONE)
			return S_STK5;
		if(operand >= -32768L && operand < 32878L)
			return S_STK16;
		return S_STK32;

	case D_CPU:
		operand = a->offset;
		if(operand < 1 || operand >= 14 || a->width != W_UH) {
			diag("bad range or width for cpu register\n%P\n", p);
			a->offset = 0;
			a->width = W_UH;
			operand = 0;
		}
		return S_STK16;
	}
	return S_STK32;
}

int
and10size(Prog *p)
{
	int n;
	
	n = andsize(p, &p->to);
	switch(n) {

	case S_STK5:
	case S_STK16:
	case S_STK32:
		n = p->as;
		if(n == ACATCH || n == APOPN || n == ARETURN) {
			if(operand >= 0 && operand < 4096 && (operand&15) == 0)
				return 2;
		} else
		if(n == AENTER) {
			if(operand >= -4096 && operand < 0 && (operand&15) == 0)
				return 2;
		}
		break;

	case S_NEG5:
	case S_IMM5:
	case S_UNS5:
	case S_XOR5:
	case S_WAI5:
	case S_IMM16:
	case S_IMM32:
		n = p->as;
		if(n == AKCALL) {
			if(operand >= 0 && operand < 4096 && (operand&3) == 0)
				return 2;
		}
		break;
	}
	return 6;
}

int
anddec(int sf, int st, char *c)
{
	int cf, ct;

	cf = codc1[sf];
	if(cf == '2')
		return 8;
	ct = codc1[st];
	if(ct == '2')
		return 8;
	if(!c || cf == '1' || ct == '1')
		return 4;
	for(; *c; c += 6)
		if(c[0] == cf && c[1] == ct)
			return 0;
	return 4;
}

void
called(void)
{
	Sym *s;
	Prog *p;

	if(debug['v'])
		Bprint(&bso, "%5.2f called\n", cputime());
	for(p = textp; p != P; p = p->cond) {
		s = p->from.sym;
		if(s == S)
			continue;
		addto(s, 0);
	}
	if(debug['v'])
	for(p = textp; p != P; p = p->cond) {
		s = p->from.sym;
		if(s == S)
			continue;
		Bprint(&bso, "	%s: %d + %d\n", s->name, s->lstack, s->rstack);
	}
}

void
addto(Sym *s, int d)
{
	Prog *p;
	Sym *t;
	int ls;

	if(s->rstack >= SBSIZE)
		return;
	d += s->rstack;
	if(d > SBSIZE)
		d = SBSIZE;
	s->rstack = d;
	ls = s->lstack;
	s->lstack = SBSIZE;
	d += ls;
	if(d > SBSIZE)
		d = SBSIZE;
	for(p = s->calledby; p != P; p = p->link) {
		t = p->to.sym;
		if(d > t->rstack)
			addto(t, d - t->rstack);
	}
	s->lstack = ls;
}

void
insertenter(Prog *text, Prog *last)
{
	Prog *p;

	p = prg();
	p->link = text->link;
	text->link = p;
	p->as = AENTER;
	p->line = text->line;
	p->to.type = D_REG;
	text->to.offset = 16;
	p->to.offset = -16;
	p->to.width = W_NONE;
	p->mark = text->mark;
	for(p = text->link; p != last; p = p->link) {
		if(p->as == ARETURN)
			p->to.offset += 16;
	}
}

void
bugs(void)
{
	Prog *p, *text;
	int state;

	if(debug['v'])
		Bprint(&bso, "%5.2f bugs\n", cputime());
	state = 0;
	text = 0;
	/*
	 * look for functions with no stack-frame but
	 * containing floating-point instructions,
	 * and give them a minimal stack-frame so the
	 * unimplemented-instruction sequence doesn't
	 * clobber the return pc.
	 * also insert minimal stack if profiling.
	 */
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(state == 2)
				insertenter(text, p);
			state = 0;
			if(p->to.offset == 0) {
				state = 1;
				text = p;
				if(debug['p'] && text->from.width != W_B)
					state = 2;
			}
		}
		if(state == 1 && optab[p->as].type == TFLOT)
			state = 2;
	}
	if(state == 2)
		insertenter(text, p);
}
