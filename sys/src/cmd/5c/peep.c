#include "gc.h"

int xtramodes(Reg*, Adr*);

void
peep(void)
{
	Reg *r, *r1, *r2;
	Prog *p, *p1;
	int t;
/*
 * complete R structure
 */
	t = 0;
	for(r=firstr; r!=R; r=r1) {
		r1 = r->link;
		if(r1 == R)
			break;
		p = r->prog->link;
		while(p != r1->prog)
		switch(p->as) {
		default:
			r2 = rega();
			r->link = r2;
			r2->link = r1;

			r2->prog = p;
			r2->p1 = r;
			r->s1 = r2;
			r2->s1 = r1;
			r1->p1 = r2;

			r = r2;
			t++;

		case ADATA:
		case AGLOBL:
		case ANAME:
		case ASIGNAME:
			p = p->link;
		}
	}

loop1:
	t = 0;
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		if(p->as == ASLL || p->as == ASRL || p->as == ASRA) {
			/*
			 * elide shift into D_SHIFT operand of subsequent instruction
			 */
			if(shiftprop(r)) {
				excise(r);
				t++;
			}
		}
		if(p->as == AMOVW || p->as == AMOVF || p->as == AMOVD)
		if(regtyp(&p->to)) {
			if(p->from.type == D_CONST)
				constprop(&p->from, &p->to, r->s1);
			else if(regtyp(&p->from))
			if(p->from.type == p->to.type) {
				if(copyprop(r)) {
					excise(r);
					t++;
				} else
				if(subprop(r) && copyprop(r)) {
					excise(r);
					t++;
				}
			}
		}
	}
	if(t)
		goto loop1;
	/*
	 * look for MOVB x,R; MOVB R,R
	 */
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		switch(p->as) {
		default:
			continue;
		case AEOR:
			/*
			 * EOR -1,x,y => MVN x,y
			 */
			if(p->from.type == D_CONST && p->from.offset == -1) {
				p->as = AMVN;
				p->from.type = D_REG;
				if(p->reg != NREG)
					p->from.reg = p->reg;
				else
					p->from.reg = p->to.reg;
				p->reg = NREG;
			}
			continue;
		case AMOVH:
		case AMOVHU:
		case AMOVB:
		case AMOVBU:
			if(p->to.type != D_REG)
				continue;
			break;
		}
		r1 = r->link;
		if(r1 == R)
			continue;
		p1 = r1->prog;
		if(p1->as != p->as)
			continue;
		if(p1->from.type != D_REG || p1->from.reg != p->to.reg)
			continue;
		if(p1->to.type != D_REG || p1->to.reg != p->to.reg)
			continue;
		excise(r1);
	}

	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		switch(p->as) {
		case AMOVW:
		case AMOVB:
		case AMOVBU:
			if(p->from.type == D_OREG && p->from.offset == 0)
				xtramodes(r, &p->from);
			else if(p->to.type == D_OREG && p->to.offset == 0)
				xtramodes(r, &p->to);
			else
				continue;
			break;
		case ACMP:
			/*
			 * elide CMP $0,x if calculation of x can set condition codes
			 */
			if(p->from.type != D_CONST || p->from.offset != 0)
				continue;
			r2 = r->s1;
			if(r2 == R)
				continue;
			t = r2->prog->as;
			switch(t) {
			default:
				continue;
			case ABEQ:
			case ABNE:
			case ABMI:
			case ABPL:
				break;
			case ABGE:
				t = ABPL;
				break;
			case ABLT:
				t = ABMI;
				break;
			case ABHI:
				t = ABNE;
				break;
			case ABLS:
				t = ABEQ;
				break;
			}
			r1 = r;
			do
				r1 = uniqp(r1);
			while (r1 != R && r1->prog->as == ANOP);
			if(r1 == R)
				continue;
			p1 = r1->prog;
			if(p1->to.type != D_REG)
				continue;
			if(p1->to.reg != p->reg)
			if(!(p1->as == AMOVW && p1->from.type == D_REG && p1->from.reg == p->reg))
				continue;
			switch(p1->as) {
			default:
				continue;
			case AMOVW:
				if(p1->from.type != D_REG)
					continue;
			case AAND:
			case AEOR:
			case AORR:
			case ABIC:
			case AMVN:
			case ASUB:
			case ARSB:
			case AADD:
			case AADC:
			case ASBC:
			case ARSC:
				break;
			}
			p1->scond |= C_SBIT;
			r2->prog->as = t;
			excise(r);
			continue;
		}
	}

	predicate();
}

void
excise(Reg *r)
{
	Prog *p;

	p = r->prog;
	p->as = ANOP;
	p->scond = zprog.scond;
	p->from = zprog.from;
	p->to = zprog.to;
	p->reg = zprog.reg; /**/
}

Reg*
uniqp(Reg *r)
{
	Reg *r1;

	r1 = r->p1;
	if(r1 == R) {
		r1 = r->p2;
		if(r1 == R || r1->p2link != R)
			return R;
	} else
		if(r->p2 != R)
			return R;
	return r1;
}

Reg*
uniqs(Reg *r)
{
	Reg *r1;

	r1 = r->s1;
	if(r1 == R) {
		r1 = r->s2;
		if(r1 == R)
			return R;
	} else
		if(r->s2 != R)
			return R;
	return r1;
}

int
regtyp(Adr *a)
{

	if(a->type == D_REG)
		return 1;
	if(a->type == D_FREG)
		return 1;
	return 0;
}

/*
 * the idea is to substitute
 * one register for another
 * from one MOV to another
 *	MOV	a, R0
 *	ADD	b, R0	/ no use of R1
 *	MOV	R0, R1
 * would be converted to
 *	MOV	a, R1
 *	ADD	b, R1
 *	MOV	R1, R0
 * hopefully, then the former or latter MOV
 * will be eliminated by copy propagation.
 */
int
subprop(Reg *r0)
{
	Prog *p;
	Adr *v1, *v2;
	Reg *r;
	int t;

	p = r0->prog;
	v1 = &p->from;
	if(!regtyp(v1))
		return 0;
	v2 = &p->to;
	if(!regtyp(v2))
		return 0;
	for(r=uniqp(r0); r!=R; r=uniqp(r)) {
		if(uniqs(r) == R)
			break;
		p = r->prog;
		switch(p->as) {
		case ABL:
			return 0;

		case ACMP:
		case ACMN:
		case AADD:
		case ASUB:
		case ARSB:
		case ASLL:
		case ASRL:
		case ASRA:
		case AORR:
		case AAND:
		case AEOR:
		case AMUL:
		case ADIV:
		case ADIVU:

		case ACMPF:
		case ACMPD:
		case AADDD:
		case AADDF:
		case ASUBD:
		case ASUBF:
		case AMULD:
		case AMULF:
		case ADIVD:
		case ADIVF:
			if(p->to.type == v1->type)
			if(p->to.reg == v1->reg) {
				if(p->reg == NREG)
					p->reg = p->to.reg;
				goto gotit;
			}
			break;

		case AMOVF:
		case AMOVD:
		case AMOVW:
			if(p->to.type == v1->type)
			if(p->to.reg == v1->reg)
				goto gotit;
			break;

		case AMOVM:
			t = 1<<v2->reg;
			if((p->from.type == D_CONST && (p->from.offset&t)) ||
			   (p->to.type == D_CONST && (p->to.offset&t)))
				return 0;
			break;
		}
		if(copyau(&p->from, v2) ||
		   copyau1(p, v2) ||
		   copyau(&p->to, v2))
			break;
		if(copysub(&p->from, v1, v2, 0) ||
		   copysub1(p, v1, v2, 0) ||
		   copysub(&p->to, v1, v2, 0))
			break;
	}
	return 0;

gotit:
	copysub(&p->to, v1, v2, 1);
	if(debug['P']) {
		print("gotit: %D->%D\n%P", v1, v2, r->prog);
		if(p->from.type == v2->type)
			print(" excise");
		print("\n");
	}
	for(r=uniqs(r); r!=r0; r=uniqs(r)) {
		p = r->prog;
		copysub(&p->from, v1, v2, 1);
		copysub1(p, v1, v2, 1);
		copysub(&p->to, v1, v2, 1);
		if(debug['P'])
			print("%P\n", r->prog);
	}
	t = v1->reg;
	v1->reg = v2->reg;
	v2->reg = t;
	if(debug['P'])
		print("%P last\n", r->prog);
	return 1;
}

/*
 * The idea is to remove redundant copies.
 *	v1->v2	F=0
 *	(use v2	s/v2/v1/)*
 *	set v1	F=1
 *	use v2	return fail
 *	-----------------
 *	v1->v2	F=0
 *	(use v2	s/v2/v1/)*
 *	set v1	F=1
 *	set v2	return success
 */
int
copyprop(Reg *r0)
{
	Prog *p;
	Adr *v1, *v2;
	Reg *r;

	p = r0->prog;
	v1 = &p->from;
	v2 = &p->to;
	if(copyas(v1, v2))
		return 1;
	for(r=firstr; r!=R; r=r->link)
		r->active = 0;
	return copy1(v1, v2, r0->s1, 0);
}

int
copy1(Adr *v1, Adr *v2, Reg *r, int f)
{
	int t;
	Prog *p;

	if(r->active) {
		if(debug['P'])
			print("act set; return 1\n");
		return 1;
	}
	r->active = 1;
	if(debug['P'])
		print("copy %D->%D f=%d\n", v1, v2, f);
	for(; r != R; r = r->s1) {
		p = r->prog;
		if(debug['P'])
			print("%P", p);
		if(!f && uniqp(r) == R) {
			f = 1;
			if(debug['P'])
				print("; merge; f=%d", f);
		}
		t = copyu(p, v2, A);
		switch(t) {
		case 2:	/* rar, cant split */
			if(debug['P'])
				print("; %Drar; return 0\n", v2);
			return 0;

		case 3:	/* set */
			if(debug['P'])
				print("; %Dset; return 1\n", v2);
			return 1;

		case 1:	/* used, substitute */
		case 4:	/* use and set */
			if(f) {
				if(!debug['P'])
					return 0;
				if(t == 4)
					print("; %Dused+set and f=%d; return 0\n", v2, f);
				else
					print("; %Dused and f=%d; return 0\n", v2, f);
				return 0;
			}
			if(copyu(p, v2, v1)) {
				if(debug['P'])
					print("; sub fail; return 0\n");
				return 0;
			}
			if(debug['P'])
				print("; sub%D/%D", v2, v1);
			if(t == 4) {
				if(debug['P'])
					print("; %Dused+set; return 1\n", v2);
				return 1;
			}
			break;
		}
		if(!f) {
			t = copyu(p, v1, A);
			if(!f && (t == 2 || t == 3 || t == 4)) {
				f = 1;
				if(debug['P'])
					print("; %Dset and !f; f=%d", v1, f);
			}
		}
		if(debug['P'])
			print("\n");
		if(r->s2)
			if(!copy1(v1, v2, r->s2, f))
				return 0;
	}
	return 1;
}

/*
 * The idea is to remove redundant constants.
 *	$c1->v1
 *	($c1->v2 s/$c1/v1)*
 *	set v1  return
 * The v1->v2 should be eliminated by copy propagation.
 */
void
constprop(Adr *c1, Adr *v1, Reg *r)
{
	Prog *p;

	if(debug['C'])
		print("constprop %D->%D\n", c1, v1);
	for(; r != R; r = r->s1) {
		p = r->prog;
		if(debug['C'])
			print("%P", p);
		if(uniqp(r) == R) {
			if(debug['C'])
				print("; merge; return\n");
			return;
		}
		if(p->as == AMOVW && copyas(&p->from, c1)) {
				if(debug['C'])
					print("; sub%D/%D", &p->from, v1);
				p->from = *v1;
		} else if(copyu(p, v1, A) > 1) {
			if(debug['C'])
				print("; %Dset; return\n", v1);
			return;
		}
		if(debug['C'])
			print("\n");
		if(r->s2)
			constprop(c1, v1, r->s2);
	}
}

/*
 * ASLL x,y,w
 * .. (not use w, not set x y w)
 * AXXX w,a,b (a != w)
 * .. (not use w)
 * (set w)
 * ----------- changed to
 * ..
 * AXXX (x<<y),a,b
 * ..
 */
#define FAIL(msg) { if(debug['H']) print("\t%s; FAILURE\n", msg); return 0; }
int
shiftprop(Reg *r)
{
	Reg *r1;
	Prog *p, *p1, *p2;
	int n, o;
	Adr a;

	p = r->prog;
	if(p->to.type != D_REG)
		FAIL("BOTCH: result not reg");
	n = p->to.reg;
	a = zprog.from;
	if(p->reg != NREG && p->reg != p->to.reg) {
		a.type = D_REG;
		a.reg = p->reg;
	}
	if(debug['H'])
		print("shiftprop\n%P", p);
	r1 = r;
	for(;;) {
		/* find first use of shift result; abort if shift operands or result are changed */
		r1 = uniqs(r1);
		if(r1 == R)
			FAIL("branch");
		if(uniqp(r1) == R)
			FAIL("merge");
		p1 = r1->prog;
		if(debug['H'])
			print("\n%P", p1);
		switch(copyu(p1, &p->to, A)) {
		case 0:	/* not used or set */
			if((p->from.type == D_REG && copyu(p1, &p->from, A) > 1) ||
			   (a.type == D_REG && copyu(p1, &a, A) > 1))
				FAIL("args modified");
			continue;
		case 3:	/* set, not used */
			FAIL("BOTCH: noref");
		}
		break;
	}
	/* check whether substitution can be done */
	switch(p1->as) {
	default:
		FAIL("non-dpi");
	case AAND:
	case AEOR:
	case AADD:
	case AADC:
	case AORR:
	case ASUB:
	case ARSB:
	case ASBC:
	case ARSC:
		if(p1->reg == n || (p1->reg == NREG && p1->to.type == D_REG && p1->to.reg == n)) {
			if(p1->from.type != D_REG)
				FAIL("can't swap");
			p1->reg = p1->from.reg;
			p1->from.reg = n;
			switch(p1->as) {
			case ASUB:
				p1->as = ARSB;
				break;
			case ARSB:
				p1->as = ASUB;
				break;
			case ASBC:
				p1->as = ARSC;
				break;
			case ARSC:
				p1->as = ASBC;
				break;
			}
			if(debug['H'])
				print("\t=>%P", p1);
		}
	case ABIC:
	case ACMP:
	case ACMN:
		if(p1->reg == n)
			FAIL("can't swap");
		if(p1->reg == NREG && p1->to.reg == n)
			FAIL("shift result used twice");
	case AMVN:
		if(p1->from.type == D_SHIFT)
			FAIL("shift result used in shift");
		if(p1->from.type != D_REG || p1->from.reg != n)
			FAIL("BOTCH: where is it used?");
		break;
	}
	/* check whether shift result is used subsequently */
	p2 = p1;
	if(p1->to.reg != n)
	for (;;) {
		r1 = uniqs(r1);
		if(r1 == R)
			FAIL("inconclusive");
		p1 = r1->prog;
		if(debug['H'])
			print("\n%P", p1);
		switch(copyu(p1, &p->to, A)) {
		case 0:	/* not used or set */
			continue;
		case 3: /* set, not used */
			break;
		default:/* used */
			FAIL("reused");
		}
		break;
	}
	/* make the substitution */
	p2->from.type = D_SHIFT;
	p2->from.reg = NREG;
	o = p->reg;
	if(o == NREG)
		o = p->to.reg;
	switch(p->from.type){
	case D_CONST:
		o |= (p->from.offset&0x1f)<<7;
		break;
	case D_REG:
		o |= (1<<4) | (p->from.reg<<8);
		break;
	}
	switch(p->as){
	case ASLL:
		o |= 0<<5;
		break;
	case ASRL:
		o |= 1<<5;
		break;
	case ASRA:
		o |= 2<<5;
		break;
	}
	p2->from.offset = o;
	if(debug['H'])
		print("\t=>%P\tSUCCEED\n", p2);
	return 1;
}

Reg*
findpre(Reg *r, Adr *v)
{
	Reg *r1;

	for(r1=uniqp(r); r1!=R; r=r1,r1=uniqp(r)) {
		if(uniqs(r1) != r)
			return R;
		switch(copyu(r1->prog, v, A)) {
		case 1: /* used */
		case 2: /* read-alter-rewrite */
			return R;
		case 3: /* set */
		case 4: /* set and used */
			return r1;
		}
	}
	return R;
}

Reg*
findinc(Reg *r, Reg *r2, Adr *v)
{
	Reg *r1;
	Prog *p;


	for(r1=uniqs(r); r1!=R && r1!=r2; r=r1,r1=uniqs(r)) {
		if(uniqp(r1) != r)
			return R;
		switch(copyu(r1->prog, v, A)) {
		case 0: /* not touched */
			continue;
		case 4: /* set and used */
			p = r1->prog;
			if(p->as == AADD)
			if(p->from.type == D_CONST)
			if(p->from.offset > -4096 && p->from.offset < 4096)
				return r1;
		default:
			return R;
		}
	}
	return R;
}

int
nochange(Reg *r, Reg *r2, Prog *p)
{
	Adr a[3];
	int i, n;

	if(r == r2)
		return 1;
	n = 0;
	if(p->reg != NREG && p->reg != p->to.reg) {
		a[n].type = D_REG;
		a[n++].reg = p->reg;
	}
	switch(p->from.type) {
	case D_SHIFT:
		a[n].type = D_REG;
		a[n++].reg = p->from.offset&0xf;
	case D_REG:
		a[n].type = D_REG;
		a[n++].reg = p->from.reg;
	}
	if(n == 0)
		return 1;
	for(; r!=R && r!=r2; r=uniqs(r)) {
		p = r->prog;
		for(i=0; i<n; i++)
			if(copyu(p, &a[i], A) > 1)
				return 0;
	}
	return 1;
}

int
findu1(Reg *r, Adr *v)
{
	for(; r != R; r = r->s1) {
		if(r->active)
			return 0;
		r->active = 1;
		switch(copyu(r->prog, v, A)) {
		case 1: /* used */
		case 2: /* read-alter-rewrite */
		case 4: /* set and used */
			return 1;
		case 3: /* set */
			return 0;
		}
		if(r->s2)
			if (findu1(r->s2, v))
				return 1;
	}
	return 0;
}

int
finduse(Reg *r, Adr *v)
{
	Reg *r1;

	for(r1=firstr; r1!=R; r1=r1->link)
		r1->active = 0;
	return findu1(r, v);
}

int
xtramodes(Reg *r, Adr *a)
{
	Reg *r1, *r2, *r3;
	Prog *p, *p1;
	Adr v;

	p = r->prog;
	if(debug['h'] && p->as == AMOVB && p->from.type == D_OREG)	/* byte load */
		return 0;
	v = *a;
	v.type = D_REG;
	r1 = findpre(r, &v);
	if(r1 != R) {
		p1 = r1->prog;
		if(p1->to.type == D_REG && p1->to.reg == v.reg)
		switch(p1->as) {
		case AADD:
			if(p1->from.type == D_REG ||
			   (p1->from.type == D_SHIFT && (p1->from.offset&(1<<4)) == 0 &&
			    (p->as != AMOVB || (a == &p->from && (p1->from.offset&~0xf) == 0))) ||
			   (p1->from.type == D_CONST && 
			    p1->from.offset > -4096 && p1->from.offset < 4096))
			if(nochange(uniqs(r1), r, p1)) {
				if(a != &p->from || v.reg != p->to.reg)
				if (finduse(r->s1, &v)) {
					if(p1->reg == NREG || p1->reg == v.reg)
						/* pre-indexing */
						p->scond |= C_WBIT;
					else return 0;	
				}
				switch (p1->from.type) {
				case D_REG:
					/* register offset */
					a->type = D_SHIFT;
					a->offset = p1->from.reg;
					break;
				case D_SHIFT:
					/* scaled register offset */
					a->type = D_SHIFT;
				case D_CONST:
					/* immediate offset */
					a->offset = p1->from.offset;
					break;
				}
				if(p1->reg != NREG)
					a->reg = p1->reg;
				excise(r1);
				return 1;
			}
			break;
		case AMOVW:
			if(p1->from.type == D_REG)
			if((r2 = findinc(r1, r, &p1->from)) != R) {
			for(r3=uniqs(r2); r3->prog->as==ANOP; r3=uniqs(r3))
				;
			if(r3 == r) {
				/* post-indexing */
				p1 = r2->prog;
				a->reg = p1->to.reg;
				a->offset = p1->from.offset;
				p->scond |= C_PBIT;
				if(!finduse(r, &r1->prog->to))
					excise(r1);
				excise(r2);
				return 1;
			}
			}
			break;
		}
	}
	if(a != &p->from || a->reg != p->to.reg)
	if((r1 = findinc(r, R, &v)) != R) {
		/* post-indexing */
		p1 = r1->prog;
		a->offset = p1->from.offset;
		p->scond |= C_PBIT;
		excise(r1);
		return 1;
	}
	return 0;
}

/*
 * return
 * 1 if v only used (and substitute),
 * 2 if read-alter-rewrite
 * 3 if set
 * 4 if set and used
 * 0 otherwise (not touched)
 */
int
copyu(Prog *p, Adr *v, Adr *s)
{

	switch(p->as) {

	default:
		if(debug['P'])
			print(" (???)");
		return 2;

	case AMOVM:
		if(v->type != D_REG)
			return 0;
		if(p->from.type == D_CONST) {	/* read reglist, read/rar */
			if(s != A) {
				if(p->from.offset&(1<<v->reg))
					return 1;
				if(copysub(&p->to, v, s, 1))
					return 1;
				return 0;
			}
			if(copyau(&p->to, v)) {
				if(p->scond&C_WBIT)
					return 2;
				return 1;
			}
			if(p->from.offset&(1<<v->reg))
				return 1;
		} else {			/* read/rar, write reglist */
			if(s != A) {
				if(p->to.offset&(1<<v->reg))
					return 1;
				if(copysub(&p->from, v, s, 1))
					return 1;
				return 0;
			}
			if(copyau(&p->from, v)) {
				if(p->scond&C_WBIT)
					return 2;
				if(p->to.offset&(1<<v->reg))
					return 4;
				return 1;
			}
			if(p->to.offset&(1<<v->reg))
				return 3;
		}
		return 0;
		
	case ANOP:	/* read, write */
	case AMOVW:
	case AMOVF:
	case AMOVD:
	case AMOVH:
	case AMOVHU:
	case AMOVB:
	case AMOVBU:
	case AMOVDW:
	case AMOVWD:
	case AMOVFD:
	case AMOVDF:
		if(p->scond&(C_WBIT|C_PBIT))
		if(v->type == D_REG) {
			if(p->from.type == D_OREG || p->from.type == D_SHIFT) {
				if(p->from.reg == v->reg)
					return 2;
			} else {
		  		if(p->to.reg == v->reg)
				return 2;
			}
		}
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, 1))
					return 1;
			return 0;
		}
		if(copyas(&p->to, v)) {
			if(copyau(&p->from, v))
				return 4;
			return 3;
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		return 0;


	case AADD:	/* read, read, write */
	case ASUB:
	case ARSB:
	case ASLL:
	case ASRL:
	case ASRA:
	case AORR:
	case AAND:
	case AEOR:
	case AMUL:
	case ADIV:
	case ADIVU:
	case AADDF:
	case AADDD:
	case ASUBF:
	case ASUBD:
	case AMULF:
	case AMULD:
	case ADIVF:
	case ADIVD:

	case ACMPF:
	case ACMPD:
	case ACMP:
	case ACMN:
	case ACASE:
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			if(copysub1(p, v, s, 1))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, 1))
					return 1;
			return 0;
		}
		if(copyas(&p->to, v)) {
			if(p->reg == NREG)
				p->reg = p->to.reg;
			if(copyau(&p->from, v))
				return 4;
			if(copyau1(p, v))
				return 4;
			return 3;
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau1(p, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ABEQ:	/* read, read */
	case ABNE:
	case ABCS:
	case ABHS:
	case ABCC:
	case ABLO:
	case ABMI:
	case ABPL:
	case ABVS:
	case ABVC:
	case ABHI:
	case ABLS:
	case ABGE:
	case ABLT:
	case ABGT:
	case ABLE:
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			return copysub1(p, v, s, 1);
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau1(p, v))
			return 1;
		return 0;

	case AB:	/* funny */
		if(s != A) {
			if(copysub(&p->to, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ARET:	/* funny */
		if(v->type == D_REG)
		if(v->reg == REGRET)
			return 2;
		if(v->type == D_FREG)
		if(v->reg == FREGRET)
			return 2;

	case ABL:	/* funny */
		if(v->type == D_REG) {
			if(v->reg <= REGEXT && v->reg > exregoffset)
				return 2;
			if(v->reg == (uchar)REGARG)
				return 2;
		}
		if(v->type == D_FREG)
			if(v->reg <= FREGEXT && v->reg > exfregoffset)
				return 2;

		if(s != A) {
			if(copysub(&p->to, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 4;
		return 3;

	case ATEXT:	/* funny */
		if(v->type == D_REG)
			if(v->reg == (uchar)REGARG)
				return 3;
		return 0;
	}
}

int
a2type(Prog *p)
{

	switch(p->as) {

	case ACMP:
	case ACMN:

	case AADD:
	case ASUB:
	case ARSB:
	case ASLL:
	case ASRL:
	case ASRA:
	case AORR:
	case AAND:
	case AEOR:
	case AMUL:
	case ADIV:
	case ADIVU:
		return D_REG;

	case ACMPF:
	case ACMPD:

	case AADDF:
	case AADDD:
	case ASUBF:
	case ASUBD:
	case AMULF:
	case AMULD:
	case ADIVF:
	case ADIVD:
		return D_FREG;
	}
	return D_NONE;
}

/*
 * direct reference,
 * could be set/use depending on
 * semantics
 */
int
copyas(Adr *a, Adr *v)
{

	if(regtyp(v)) {
		if(a->type == v->type)
		if(a->reg == v->reg)
			return 1;
	} else if(v->type == D_CONST) {		/* for constprop */
		if(a->type == v->type)
		if(a->name == v->name)
		if(a->sym == v->sym)
		if(a->reg == v->reg)
		if(a->offset == v->offset)
			return 1;
	}
	return 0;
}

/*
 * either direct or indirect
 */
int
copyau(Adr *a, Adr *v)
{

	if(copyas(a, v))
		return 1;
	if(v->type == D_REG) {
		if(a->type == D_OREG) {
			if(v->reg == a->reg)
				return 1;
		} else if(a->type == D_SHIFT) {
			if((a->offset&0xf) == v->reg)
				return 1;
			if((a->offset&(1<<4)) && (a->offset>>8) == v->reg)
				return 1;
		}
	}
	return 0;
}

int
copyau1(Prog *p, Adr *v)
{

	if(regtyp(v)) {
		if(a2type(p) == v->type)
		if(p->reg == v->reg) {
			if(a2type(p) != v->type)
				print("botch a2type %P\n", p);
			return 1;
		}
	}
	return 0;
}

/*
 * substitute s for v in a
 * return failure to substitute
 */
int
copysub(Adr *a, Adr *v, Adr *s, int f)
{

	if(f)
	if(copyau(a, v)) {
		if(a->type == D_SHIFT) {
			if((a->offset&0xf) == v->reg)
				a->offset = (a->offset&~0xf)|s->reg;
			if((a->offset&(1<<4)) && (a->offset>>8) == v->reg)
				a->offset = (a->offset&~(0xf<<8))|(s->reg<<8);
		} else
			a->reg = s->reg;
	}
	return 0;
}

int
copysub1(Prog *p1, Adr *v, Adr *s, int f)
{

	if(f)
	if(copyau1(p1, v))
		p1->reg = s->reg;
	return 0;
}

struct {
	int opcode;
	int notopcode;
	int scond; 
	int notscond; 
} predinfo[]  = { 
	{ ABEQ,	ABNE,	0x0,	0x1, }, 
	{ ABNE,	ABEQ,	0x1,	0x0, }, 
	{ ABCS,	ABCC,	0x2,	0x3, }, 
	{ ABHS,	ABLO,	0x2,	0x3, }, 
	{ ABCC,	ABCS,	0x3,	0x2, }, 
	{ ABLO,	ABHS,	0x3,	0x2, }, 
	{ ABMI,	ABPL,	0x4,	0x5, }, 
	{ ABPL,	ABMI,	0x5,	0x4, }, 
	{ ABVS,	ABVC,	0x6,	0x7, }, 
	{ ABVC,	ABVS,	0x7,	0x6, }, 
	{ ABHI,	ABLS,	0x8,	0x9, }, 
	{ ABLS,	ABHI,	0x9,	0x8, }, 
	{ ABGE,	ABLT,	0xA,	0xB, }, 
	{ ABLT,	ABGE,	0xB,	0xA, }, 
	{ ABGT,	ABLE,	0xC,	0xD, }, 
	{ ABLE,	ABGT,	0xD,	0xC, }, 
}; 

typedef struct {
	Reg *start;
	Reg *last;
	Reg *end;
	int len;
} Joininfo;

enum {
	Join,
	Split,
	End,
	Branch,
	Setcond,
	Toolong
};
	
enum {
	Falsecond,
	Truecond,
	Delbranch,
	Keepbranch
};

int 
isbranch(Prog *p)
{
	return (ABEQ <= p->as) && (p->as <= ABLE); 
}

int
predicable(Prog *p)
{
	if (isbranch(p)
		|| p->as == ANOP
		|| p->as == AXXX
		|| p->as == ADATA
		|| p->as == AGLOBL
		|| p->as == AGOK
		|| p->as == AHISTORY
		|| p->as == ANAME
		|| p->as == ASIGNAME
		|| p->as == ATEXT
		|| p->as == AWORD
		|| p->as == ADYNT
		|| p->as == AINIT
		|| p->as == ABCASE
		|| p->as == ACASE)
		return 0; 
	return 1; 
}

/* 
 * Depends on an analysis of the encodings performed by 5l. 
 * These seem to be all of the opcodes that lead to the "S" bit
 * being set in the instruction encodings. 
 * 
 * C_SBIT may also have been set explicitly in p->scond.
 */ 
int
modifiescpsr(Prog *p)
{
	return (p->scond&C_SBIT)
		|| p->as == ATST 
		|| p->as == ATEQ 
		|| p->as == ACMN
		|| p->as == ACMP
		|| p->as == AMULU
		|| p->as == ADIVU
		|| p->as == AMUL
		|| p->as == ADIV
		|| p->as == AMOD
		|| p->as == AMODU
		|| p->as == ABL;
} 

/*
 * Find the maximal chain of instructions starting with r which could
 * be executed conditionally
 */
int
joinsplit(Reg *r, Joininfo *j)
{
	j->start = r;
	j->last = r;
	j->len = 0;
	do {
		if (r->p2 && (r->p1 || r->p2->p2link)) {
			j->end = r;
			return Join;
		}
		if (r->s1 && r->s2) {
			j->end = r;
			return Split;
		}
		j->last = r;
		if (r->prog->as != ANOP)
			j->len++;
		if (!r->s1 && !r->s2) {
			j->end = r->link;
			return End;
		}
		if (r->s2) {
			j->end = r->s2;
			return Branch;
		}
		if (modifiescpsr(r->prog)) {
			j->end = r->s1;
			return Setcond;
		}
		r = r->s1;
	} while (j->len < 4);
	j->end = r;
	return Toolong;
}

Reg *
successor(Reg *r)
{
	if (r->s1)
		return r->s1; 
	else
		return r->s2; 
}

void
applypred(Reg *rstart, Joininfo *j, int cond, int branch)
{
	int pred; 
	Reg *r; 

	if(j->len == 0)
		return;
	if (cond == Truecond)
		pred = predinfo[rstart->prog->as - ABEQ].scond;
	else
		pred = predinfo[rstart->prog->as - ABEQ].notscond; 
	
	for (r = j->start; ; r = successor(r)) {
		if (r->prog->as == AB) {
			if (r != j->last || branch == Delbranch)
				excise(r);
			else {
			  if (cond == Truecond)
				r->prog->as = predinfo[rstart->prog->as - ABEQ].opcode;
			  else
				r->prog->as = predinfo[rstart->prog->as - ABEQ].notopcode;
			}
		}
		else if (predicable(r->prog)) 
			r->prog->scond = (r->prog->scond&~C_SCOND)|pred;
		if (r->s1 != r->link) {
			r->s1 = r->link;
			r->link->p1 = r;
		}
		if (r == j->last)
			break;
	}
}

void
predicate(void)
{	
	Reg *r;
	int t1, t2;
	Joininfo j1, j2;

	for(r=firstr; r!=R; r=r->link) {
		if (isbranch(r->prog)) {
			t1 = joinsplit(r->s1, &j1);
			t2 = joinsplit(r->s2, &j2);
			if(j1.last->link != j2.start)
				continue;
			if(j1.end == j2.end)
			if((t1 == Branch && (t2 == Join || t2 == Setcond)) ||
			   (t2 == Join && (t1 == Join || t1 == Setcond))) {
				applypred(r, &j1, Falsecond, Delbranch);
				applypred(r, &j2, Truecond, Delbranch);
				excise(r);
				continue;
			}
			if(t1 == End || t1 == Branch) {
				applypred(r, &j1, Falsecond, Keepbranch);
				excise(r);
				continue;
			}
		} 
	} 
}
