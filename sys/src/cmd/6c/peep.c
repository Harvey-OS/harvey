#include "gc.h"

void
peep(void)
{
	Reg *r, *r1, *r2;
	Prog *p;
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
			p = p->link;
		}
	}

loop1:
	t = 0;
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		if(p->as == AMOV)
		if(regtyp(&p->to))
		if(regtyp(&p->from)) {
			if(copyprop(r)) {
				excise(r);
				t++;
			}
			if(subprop(r) && copyprop(r)) {
				excise(r);
				t++;
			}
		}
	}
	if(t)
		goto loop1;
}

void
excise(Reg *r)
{
	Prog *p;

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

int
regtyp(Adr *a)
{
	int t;

	t = a->type;
	if(t >= D_R0 && t <= D_R0+31)
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
		case ABAL:
			return 0;

		case AADDI:	/* 3-op */
		case AADDO:
		case AAND:
		case ASUBI:
		case ASUBO:
		case AOR:
		case AXOR:
		case ASHLI:
		case ASHRI:
		case ASHLO:
		case ASHRO:
		case AMULI:
		case AMULO:
		case ADIVI:
		case ADIVO:
		case AREMI:
		case AREMO:
			if(p->type != D_NONE)
				return 0;
			break;	/* botch */
			if(p->to.type == v1->type) {
				if(p->type == D_NONE)
					p->type = p->to.type;
				goto gotit;
			}
			break;

		case AMOV:
			if(p->to.type == v1->type)
				goto gotit;
			break;
		}
		if(copyau(&p->from, v2) ||
		   copyau(&p->to, v2))
			break;
		if(copysub(&p->from, v1, v2, 0) ||
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
		copysub(&p->to, v1, v2, 1);
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
				print("; %D rar; return 0\n", v2);
			return 0;

		case 3:	/* set */
			if(debug['P'])
				print("; %D set; return 1\n", v2);
			return 1;

		case 1:	/* used, substitute */
		case 4:	/* use and set */
			if(f) {
				if(!debug['P'])
					return 0;
				if(t == 4)
					print("; %D used+set and f=%d; return 0\n", v2, f);
				else
					print("; %D used and f=%d; return 0\n", v2, f);
				return 0;
			}
			if(copyu(p, v2, v1)) {
				if(debug['P'])
					print("; sub fail; return 0\n");
				return 0;
			}
			if(debug['P'])
				print("; sub %D/%D", v2, v1);
			if(t == 4) {
				if(debug['P'])
					print("; %D used+set; return 1\n", v2);
				return 1;
			}
			break;
		}
		if(!f) {
			t = copyu(p, v1, A);
			if(!f && (t == 2 || t == 3 || t == 4)) {
				f = 1;
				if(debug['P'])
					print("; %D set and !f; f=%d", v1, f);
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
copyu(Prog *p, Adr *v, Adr *s)
{

	switch(p->as) {

	default:
print("default -- rar %P\n", p);
	case ASHLI:
	case ASHRI:
	case ASHLO:
	case ASHRO:
	case AMULI:
	case AMULO:
	case ADIVI:
	case ADIVO:
	case AREMI:
	case AREMO:
		if(debug['P'])
			print("unknown op %A\n", p->as);
		return 2;

	case AMOVA:	/* lhs addr, rhs store */
		if(copyas(&p->from, v))
			return 2;


	case ANOP:	/* rhs store */
	case AMOV:
	case AMOVOB:
	case AMOVIB:
	case AMOVOS:
	case AMOVIS:
		if(copyas(&p->to, v)) {
			if(s != A)
				return copysub(&p->from, v, s, 1);
			if(copyau(&p->from, v))
				return 4;
			return 3;
		}
		goto caseread;


	case AADDI:	/* rhs rar */
	case AADDO:
	case AAND:
	case ASUBI:
	case ASUBO:
	case AOR:
	case AXOR:
		if(copyas(&p->to, v))
			return 2;
		goto caseread;

	case ACMPI:	/* read only */
	case ACMPO:

	caseread:
		if(p->type != D_NONE)
			return 2;		/* botch */
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			return copysub(&p->to, v, s, 1);
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		break;

	case AB:	/* no reference */
	case ABNE:
	case ABE:
	case ABL:
	case ABLE:
	case ABG:
	case ABGE:
		break;

	case ARTS:	/* funny */
	case ABAL:	/* funny */
		if(v->type == REGRET)
			return 2;

		if(s != A) {
			if(s->type == REGRET)
				return 2;
			if(copysub(&p->to, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 4;
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
	if(regtyp(v))
		return 1;
	if(v->type == D_AUTO || v->type == D_PARAM)
		if(v->offset == a->offset)
			return 1;
	return 0;
}

int
copyas1(Prog *p, Adr *v)
{
	if(p->type != v->type)
		return 0;
	if(regtyp(v))
		return 1;
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
	if(regtyp(v)) {
		if(a->type-D_INDIR == v->type)
			return 1;
		if(a->index == v->type)
			return 1;
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
	int t;

	if(copyas(a, v)) {
		t = s->type;
		if(t >= D_R0 && t <= D_R0+31) {
			if(f)
				a->type = t;
		}
		return 0;
	}
	if(regtyp(v)) {
		t = v->type;
		if(a->type == t+D_INDIR) {
			if(f)
				a->type = s->type+D_INDIR;
			return 0;
		}
		if(a->index == t) {
			if(f)
				a->index = s->type;
			return 0;
		}
		return 0;
	}
	return 0;
}

/*
 * substitute s for v in p
 * return failure to substitute
 */
int
copysub1(Prog *p, Adr *v, Adr *s, int f)
{
	int t;

	if(copyas1(p, v)) {
		t = s->type;
		if(t >= D_R0 && t <= D_R0+31) {
			if(f)
				p->type = t;
		}
		return 0;
	}
	return 0;
}
