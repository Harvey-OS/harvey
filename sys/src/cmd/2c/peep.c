#include "gc.h"

void
peep(void)
{
	Reg *r, *r1, *r2;
	Prog *p, *p1;
	int t, s;
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
	/*
	 * propigate move's by renaming
	 */
	t = 0;
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		if(p->as == AMOVL || p->as == AFMOVEF || p->as == AFMOVED)
		if(regtyp(p->from.type))
		if(anyvar(&p->to)) {
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
	if(t)
		goto loop1;
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		/*
		 * convert (A) ... A++ into (A)++
		 * and     A-- ... (A) into --(A)
		 */
		t = aregind(&p->from);
		if(t == D_NONE)
			goto out1;
		s = asize(p->as);
		if(s == 0)
			goto out1;
		r1 = findop(r, t, AADDL, s);
		if(r1 != R) {
			if(usedin(t, &p->to))
				goto out1;
			p->from.type += I_INDINC - I_INDIR;
			excise(r1);
			goto out1;
		}
		r1 = findop(r, t, ASUBL, s);
		if(r1 != R) {
			p->from.type += I_INDDEC - I_INDIR;
			excise(r1);
		}
	out1:
		t = aregind(&p->to);
		if(t == D_NONE)
			goto out2;
		s = asize(p->as);
		if(s == 0)
			goto out2;
		r1 = findop(r, t, AADDL, s);
		if(r1 != R) {
			p->to.type += I_INDINC - I_INDIR;
			excise(r1);
			goto out2;
		}
		r1 = findop(r, t, ASUBL, s);
		if(r1 != R) {
			if(usedin(t, &p->from))
				goto out2;
			p->to.type += I_INDDEC - I_INDIR;
			excise(r1);
		}
	out2:
		/*
		 * get rid of unneeded save/restore CCR
		 */
		if(p->from.type == D_CCR) {
			r1 = findccr(r);
			if(r1 != R) {
				excise(r);
				excise(r1);
			}
		}
		switch(p->as) {
		case ATSTB:
		case ATSTW:
		case ATSTL:
			if(findtst(r, r->prog, 0))
				excise(r);
		}
		/*
		 * turn TSTB (A); BLT; ORB $128,(A) into TAS (A); BLT; NOP
		 */
		if(p->as == ATSTB && (r1 = r->s1)) {
			if((r1->prog->as == ABLT && (r2 = r1->s1)) ||
			   (r1->prog->as == ABGE && (r2 = r1->s2))) {
				p1 = r2->prog;
				if(p1->as == AORB)
				if(p1->from.type == D_CONST)
				if(p1->from.offset == 128)
				if(r1 == uniqp(r2))
				if(tasas(&p->to, &p1->to)) {
					p->as = ATAS;
					excise(r2);
				}
			}
		}
	}
}

void
excise(Reg *r)
{

	p = r->prog;
	p->as = ANOP;
	p->from = zprog.from;
	p->to = zprog.to;
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

/*
 * chase backward all cc setting.
 * returns 1 if all set same.
 */
int
findtst(Reg *r0, Prog *rp, int n)
{
	Reg *r;
	int c;

loop:
	n++;
	if(n >= 10)
		return 0;
	for(r=r0->p2; r!=R; r=r->p2link) {
		c = setcc(r->prog, rp);
		if(c > 0)
			continue;
		if(c == 0)
			return 0;
		if(findtst(r, rp, n) == 0)
			return 0;
	}
	r = r0->p1;
	if(r == R)
		return 1;
	c = setcc(r->prog, rp);
	if(c > 0)
		return 1;
	if(c == 0)
		return 0;
	r0 = r;
	goto loop;
}

/*
 * tests cc
 * returns -1 if no change
 * returns 1 if set the same
 * returns 0 if set different
 */
int
setcc(Prog *p, Prog *rp)
{
	int s;

	s = asize(rp->as);
	switch(p->as) {
	default:
		if(debug['P'])
			print("unknown setcc %A\n", p->as);
		break;

	case ACMPB:
	case ACMPW:
	case ACMPL:
	case ABSR:
		return 0;

	case ABRA:
	case ABGE:
	case ABNE:
	case ABLE:
	case ABEQ:
	case ABHI:
	case ABLS:
	case ABMI:
	case ABPL:
	case ABGT:
	case ABLT:
	case ABCC:
	case ABCS:
	case APEA:
	case ALEA:
	case ANOP:

	case AFADDD:
	case AFMULD:
	case AFDIVD:
	case AFSUBD:
	case AFADDF:
	case AFMULF:
	case AFDIVF:
	case AFSUBF:
	case AADJSP:
		return -1;

	case AADDW:
	case AADDL:
	case ASUBW:
	case ASUBL:
	case ACLRL:
	case ACLRW:
		if(p->to.type >= D_A0 && p->to.type < D_A0+8)
			goto areg;

	case AADDB:
	case ASUBB:
	case AANDB:
	case AANDW:
	case AANDL:
	case AORB:
	case AORW:
	case AORL:
	case AEORB:
	case AEORW:
	case AEORL:
	case ALSLB:
	case ALSLW:
	case ALSLL:
	case ALSRB:
	case ALSRW:
	case ALSRL:
	case AASLB:
	case AASLW:
	case AASLL:
	case AASRB:
	case AASRW:
	case AASRL:
	case ATSTB:
	case ATSTW:
	case ATSTL:
	case ANEGB:
	case ANEGW:
	case ANEGL:
	case ACLRB:
		if(asize(p->as) != s)
			break;
		if(compat(&rp->to, &p->to))
			return 1;
		break;

	case AMOVW:
	case AMOVL:
		if(p->to.type >= D_A0 && p->to.type < D_A0+8)
			goto areg;
	case AMOVB:
		if(asize(p->as) != s)
			break;
		if(compat(&rp->to, &p->to))
			return 1;
		if(compat(&rp->to, &p->from))
			return 1;
	}
	return 0;

areg:
	if((rp->to.type&D_MASK) == p->to.type)
		return 0;
	return -1;
}

int
compat(Adr *a, Adr *b)
{
	int o;

	if(a->index != D_NONE)
		return 0;
	if(b->index != D_NONE)
		return 0;
	o = a->type;
	if((o >= D_R0 && o < D_R0+NREG) ||
	   (o >= D_A0 && o < D_A0+NREG))
		return o == b->type;
	o &= D_MASK;
	if(o >= D_A0 && o < D_A0+NREG) {
		if(o != (b->type&D_MASK))
			return 0;
		if(a->offset != b->offset)
			return 0;
		o = a->type & I_MASK;
		if(o == I_INDIR) {
			o = b->type & I_MASK;
			if(o == I_INDIR || o == I_INDDEC)
				return 1;
			return 0;
		}
		if(o == I_INDINC) {
			o = b->type & I_MASK;
			if(o == I_INDIR) {
				b->type += I_INDINC-I_INDIR;
				return 1;
			}
			if(o == I_INDDEC) {
				b->type += I_INDIR-I_INDDEC;
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

int
aregind(Adr *a)
{
	int t;

	t = a->type;
	if(t >= (D_A0|I_INDIR) && t < ((D_A0+NREG)|I_INDIR))
	while(a->offset == 0 && a->index == D_NONE)
		return t & D_MASK;
	return D_NONE;
}

int
asize(int a)
{

	switch(a) {
	case AFTSTD:
	case AFMOVED:
	case AFADDD:
	case AFSUBD:
	case AFMULD:
	case AFDIVD:
	case AFCMPD:
	case AFNEGD:
		return 8;

	case AFTSTF:
	case AFMOVEF:
	case AFADDF:
	case AFSUBF:
	case AFMULF:
	case AFDIVF:
	case AFCMPF:
	case AFNEGF:

	case ACLRL:
	case ATSTL:
	case AMOVL:
	case AADDL:
	case ASUBL:
	case ACMPL:
	case AANDL:
	case AORL:
	case AEORL:
	case ALSLL:
	case ALSRL:
	case AASLL:
	case AASRL:
	case ANEGL:
		return 4;

	case ACLRW:
	case ATSTW:
	case AMOVW:
	case AADDW:
	case ASUBW:
	case ACMPW:
	case AANDW:
	case AORW:
	case AEORW:
	case ALSLW:
	case ALSRW:
	case AASLW:
	case AASRW:
	case ANEGW:
		return 2;

	case ACLRB:
	case ATSTB:
	case AMOVB:
	case AADDB:
	case ASUBB:
	case ACMPB:
	case AANDB:
	case AORB:
	case AEORB:
	case ALSLB:
	case ALSRB:
	case AASLB:
	case AASRB:
	case ANEGB:
		return 1;
	}
	if(debug['P'])
		print("unknown asize %A\n", p->as);
	return 0;
}

int
usedin(int t, Adr *a)
{

	if((a->type&D_MASK) == t)
		return 1;
	if((a->index&D_MASK) == t)
		return 1;
	return 0;
}

Reg*
findccr(Reg *r)
{
	Prog *p;

	for(;;) {
		r = uniqs(r);
		if(r == R)
			break;
		p = r->prog;
		if(p->to.type == D_CCR)
			return r;
		if(setccr(p))
			break;
	}
	return R;
}

int
setccr(Prog *p)
{

	switch(p->as) {
	case ANOP:
		return 0;

	case AADDL:
	case AMOVL:
	case ACLRL:
		if(p->to.type >= D_A0 && p->to.type < D_A0+8)
			return 0;
	}
	return 1;
}

Reg*
findop(Reg *r, int t, int o, int s)
{
	Prog *p;
	Reg *r1;

	for(;;) {
		if(o == AADDL) {
			r1 = uniqs(r);
			if(r1 == R)
				break;
			if(uniqp(r1) != r)
				break;
		} else {
			r1 = uniqp(r);
			if(r1 == R)
				break;
			if(uniqs(r1) != r)
				break;
		}
		r = r1;
		p = r->prog;
		if(usedin(t, &p->from))
			break;
		if(usedin(t, &p->to)) {
			if(p->as == o)
			if(p->to.type == t)
			if(p->to.index == D_NONE)
			if(p->from.type == D_CONST)
			if(p->from.offset == s)
				return r;
			break;
		}
	}
	return R;
}

int
regtyp(int t)
{

	if(t >= D_R0 && t < D_R0+8)
		return 1;
	if(t >= D_A0 && t < D_A0+8)
		return 1;
	if(t >= D_F0 && t < D_F0+8)
		return 1;
	return 0;
}

int
anyvar(Adr *a)
{

	if(regtyp(a->type))
		return 1;
	return 0;
	if(a->type == D_AUTO || a->type == D_PARAM)
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
 * hopefully, then the former or latter MOVL
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
	if(!regtyp(v1->type))
		return 0;
	v2 = &p->to;
	if(!regtyp(v2->type))
		return 0;
	for(r=uniqp(r0); r!=R; r=uniqp(r)) {
		if(uniqs(r) == R)
			break;
		p = r->prog;
		switch(p->as) {
		case ADIVUW:	/* these set Rn and Rn+1 */
		case ADIVUL:
		case ADIVSW:
		case ADIVSL:
		case ABSR:
			return 0;

		case AFMOVED:
		case AFMOVEF:
		case AMOVL:
			if(p->to.type == v1->type)
				goto gotit;
		}
		if(copyau(&p->from, v2) || copyau(&p->to, v2))
			break;
		if(copysub(&p->from, v1, v2, p, 0) || copysub(&p->to, v1, v2, p, 0))
			break;
	}
	return 0;

gotit:
	copysub(&p->to, v1, v2, p, 1);
	if(debug['P']) {
		print("gotit: %D->%D\n%P", v1, v2, r->prog);
		if(p->from.type == v2->type)
			print(" excise");
		print("\n");
	}
	if(p->from.type == v2->type)
		excise(r);
	for(r=uniqs(r); r!=r0; r=uniqs(r)) {
		p = r->prog;
		copysub(&p->from, v1, v2, p, 1);
		copysub(&p->to, v1, v2, p, 1);
		if(debug['P'])
			print("%P\n", r->prog);
	}
	t = v1->type;
	v1->type = v2->type;
	v2->type = t;
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

	if(r->active) {
		if(debug['P'])
			print("copyret 1\n");
		return 1;
	}
	r->active = 1;
	if(debug['P'])
		print("copy %D->%D\n", v1, v2);
	for(; r != R; r = r->s1) {
		if(debug['P'])
			print("%P", r->prog);
		if(!f && uniqp(r) == R) {
			f = 1;
			if(debug['P'])
				print("; merge; f=%d", f);
		}
		t = copyu(r->prog, v2, A);
		switch(t) {
		case 2:	/* rar, cant split */
			if(debug['P'])
				print("; rar return 0\n");
			return 0;
		case 3:	/* set */
			if(debug['P'])
				print("; set; return 1\n");
			return 1;
		case 1:	/* used, substitute */
		case 4:	/* use and set */
			if(f) {
				if(debug['P'])
					print("; used and f; return 0\n");
				return 0;
			}
			if(copyu(r->prog, v2, v1)) {
				if(debug['P'])
					print("; sub fail; return 0\n");
				return 0;
			}
			if(debug['P'])
				print("; substitute");
			if(t == 4) {
				if(debug['P'])
					print("; used and set; return 1\n");
				return 1;
			}
			break;
		}
		if(!f) {
			t = copyu(r->prog, v1, A);
			if(!f && (t == 2 || t == 3 || t == 4)) {
				if(debug['P'])
					print("; f set used");
				f = 1;
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
	int t;

	switch(p->as) {

	default:
		if(debug['P'])
			print("unknown op %A\n", p->as);
		return 2;

	case APEA:	/* rhs addr */
		if(copyas(&p->to, v))
			return 2;
		goto caseread;

	case ALEA:	/* lhs addr, rhs store */
		if(copyas(&p->from, v))
			return 2;

	case AMOVL:	/* rhs store */
	case ACLRL:
	case AFMOVEF:
	case AFMOVED:
	case AFMOVEB:
	case AFMOVEW:
	case AFMOVEL:
	case ANOP:
		if(copyas(&p->to, v)) {
			if(s != A)
				return copysub(&p->from, v, s, p, 1);
			if(copyau(&p->from, v))
				return 4;
			return 3;
		}
		goto caseread;
			
	case AADDL:	/* rhs rar */
	case AADDW:
	case AADDB:
	case ASUBL:
	case ASUBW:
	case ASUBB:
	case AANDL:
	case AANDW:
	case AANDB:
	case AORL:
	case AORW:
	case AORB:
	case AEORL:
	case AEORW:
	case AEORB:
	case AASRL:
	case AASRW:
	case AASRB:
	case AASLL:
	case AASLW:
	case AASLB:
	case ALSRL:
	case ALSRW:
	case ALSRB:
	case ANOTL:
	case ANOTW:
	case ANOTB:
	case ANEGL:
	case ANEGW:
	case ANEGB:
	case AEXTBL:
	case AEXTWL:
	case AEXTBW:

	case AMULSL:
	case AMULUL:

	case AMOVW:	/* only sets part of register */
	case AMOVB:
	case ACLRW:
	case ACLRB:

	case AFADDD:
	case AFMULD:
	case AFDIVD:
	case AFSUBD:
	case AFNEGD:
	case AFADDF:
	case AFMULF:
	case AFDIVF:
	case AFSUBF:
	case AFNEGF:
		if(copyas(&p->to, v))
			return 2;
		goto caseread;

	case ADBF:	/* lhs rar */
		if(copyas(&p->from, v))
			return 2;
		goto caseread;

	case ACMPL:	/* read only */
	case ACMPW:
	case ACMPB:
	case AFCMPF:
	case AFCMPD:
	case ATSTL:
	case ATSTW:
	case ATSTB:
	case AFTSTF:
	case AFTSTD:
	caseread:
		if(s != A) {
			if(copysub(&p->from, v, s, p, 1))
				return 1;
			return copysub(&p->to, v, s, p, 1);
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		break;

	case ABRA:	/* no reference */
	case ABGE:
	case ABNE:
	case ABLE:
	case ABEQ:
	case ABHI:
	case ABLS:
	case ABMI:
	case ABPL:
	case ABGT:
	case ABLT:
	case ABCC:
	case ABCS:

	case AFBEQ:
	case AFBNE:
	case AFBGT:
	case AFBGE:
	case AFBLE:
	case AFBLT:

	case AADJSP:
	case ACASEW:
		break;

	case ADIVUW:	/* these set Rn and Rn+1 */
	case ADIVUL:
	case ADIVSW:
	case ADIVSL:
		t = v->type;
		if(t == p->to.type || t == p->to.type+1)
			return 2;
		goto caseread;

	case ARTS:	/* funny */
		t = v->type;
		if(t == D_R0 || t == D_F0)
			return 2;
		if(t >= D_R0 && t < D_R0+NREG)
		if(t-D_R0 > exregoffset)
			return 2;
		if(t >= D_A0 && t < D_A0+NREG)
		if(t-D_A0 > exaregoffset)
			return 2;
		if(t >= D_F0 && t < D_F0+NREG)
		if(t-D_F0 > exfregoffset)
			return 2;
		return 3;

	case ABSR:	/* funny */
		t = v->type;
		if(t >= D_R0 && t < D_R0+NREG)
		if(t-D_R0 > exregoffset)
			return 2;
		if(t >= D_A0 && t < D_A0+NREG)
		if(t-D_A0 > exaregoffset)
			return 2;
		if(t >= D_F0 && t < D_F0+NREG)
		if(t-D_F0 > exfregoffset)
			return 2;
		return 3;
	}
	return 0;
}

/*
 * direct reference,
 * could be set/use depending on
 * semantics
 */
int
copyas(Adr *a, Adr *v)
{

	if(a->type != v->type)
		return 0;
	if(regtyp(v->type))
		return 1;
	if(v->type == D_AUTO || v->type == D_PARAM) {
		if(v->offset == a->offset)
			return 1;
		return 0;
	}
	return 0;
}

/*
 * indirect
 */
int
tasas(Adr *a, Adr *v)
{
	int t;

	if(a->index != D_NONE)
		return 0;
	if(v->index != D_NONE)
		return 0;
	t = a->type;
	if(t < I_INDIR+D_A0 && t >= I_INDIR+D_A0+8)
		return 0;
	if(v->type != t)
		return 0;
	if(a->displace != v->displace)
		return 0;
	return 1;
}

/*
 * either direct or indirect
 */
int
copyau(Adr *a, Adr *v)
{
	int t;

	if(copyas(a, v))
		return 1;
	t = v->type;
	if(regtyp(t)) {
		if((a->type & D_MASK) == t)
			return 1;
		if((a->index & D_MASK) == t)
			return 1;
	}
	return 0;
}

/*
 * substitute s for v in a
 * return failure to substitute
 */
int
copysub(Adr *a, Adr *v, Adr *s, Prog *p, int f)
{
	int t;

	if(copyas(a, v)) {
		t = s->type;
		if(t >= D_F0 && t < D_F0+8) {
			if(f)
				a->type = t;
			return 0;
		}
		if(t >= D_R0 && t < D_R0+8) {
			if(f)
				a->type = t;
			return 0;
		}
		if(!(t >= D_A0 && t < D_A0+8))
			return 1;
		switch(p->as) {
		default:
			return 1;

		case AMOVL:
		case AMOVW:
		case ACMPL:
		case ACMPW:
			break;

		case AADDL:
		case AADDW:
		case ASUBL:
		case ASUBW:
			if(a == &p->from && !regtyp(p->to.type))
				return 1;
			break;
		}
		if(f)
			a->type = t;
		return 0;
	}
	t = v->type;
	if(regtyp(t)) {
		if((a->type & D_MASK) == t) {
			if((s->type ^ t) & ~(NREG-1))
				return 1;
			if(f)
				a->type = (a->type & ~D_MASK) | s->type;
			return 0;
		}
		if((a->index & D_MASK) == t) {
			if(f)
				a->index = (a->index & ~D_MASK) | s->type;
			return 0;
		}
		return 0;
	}
	return 0;
}
