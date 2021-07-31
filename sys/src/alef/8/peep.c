#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

void
peep(void)
{
	Reg *r, *r1, *r2;
	Inst *p;
	int t;
/*
 * complete R structure
 */
	t = 0;
	for(r=firstr; r!=R; r=r1) {
		r1 = r->next;
		if(r1 == R)
			break;
		p = r->prog->next;
		while(p != r1->prog)
		switch(p->op) {
		default:
			r2 = rega();
			r->next = r2;
			r2->next = r1;

			r2->prog = p;
			r2->p1 = r;
			r->s1 = r2;
			r2->s1 = r1;
			r1->p1 = r2;

			r = r2;
			t++;

		case ADATA:
		case AINIT:
		case ADYNT:
		case AGLOBL:
		case ANAME:
			p = p->next;
		}
	}

loop1:
	t = 0;
	for(r=firstr; r!=R; r=r->next) {
		p = r->prog;
		if(p->op == AMOVL)
		if(regtyp(&p->dst))
		if(regtyp(&p->src1)) {
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
	Inst *p;

	p = r->prog;
	p->op = ANOP;
	p->src1 = zprog.src1;
	p->dst = zprog.dst;
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
regtyp(Adres *a)
{
	int t;

	t = a->type;
	if(t >= D_AX && t <= D_DI)
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
	Inst *p;
	Adres *v1, *v2;
	Reg *r;
	int t;

	p = r0->prog;
	v1 = &p->src1;
	if(!regtyp(v1))
		return 0;
	v2 = &p->dst;
	if(!regtyp(v2))
		return 0;
	for(r=uniqp(r0); r!=R; r=uniqp(r)) {
		if(uniqs(r) == R)
			break;
		p = r->prog;
		switch(p->op) {
		case ACALL:
			return 0;

		case ADIVB:
		case ADIVL:
		case ADIVW:
		case AIDIVB:
		case AIDIVL:
		case AIDIVW:
		case AIMULB:
		case AIMULL:
		case AIMULW:
		case AMULB:
		case AMULL:
		case AMULW:

		case AROLB:
		case AROLL:
		case AROLW:
		case ARORB:
		case ARORL:
		case ARORW:
		case ASALB:
		case ASALL:
		case ASALW:
		case ASARB:
		case ASARL:
		case ASARW:
		case ASHLB:
		case ASHLL:
		case ASHLW:
		case ASHRB:
		case ASHRL:
		case ASHRW:

		case AREP:
		case AREPN:

		case ACWD:
		case ACDQ:

		case AMOVSL:
		case AFSTSW:
			return 0;

		case AMOVL:
			if(p->dst.type == v1->type)
				goto gotit;
			break;
		}
		if(copyau(&p->src1, v2) ||
		   copyau(&p->dst, v2))
			break;
		if(copysub(&p->src1, v1, v2, 0) ||
		   copysub(&p->dst, v1, v2, 0))
			break;
	}
	return 0;

gotit:
	copysub(&p->dst, v1, v2, 1);
	if(opt('P')) {
		print("gotit: %a->%a\n%i", v1, v2, r->prog);
		if(p->src1.type == v2->type)
			print(" excise");
		print("\n");
	}
	for(r=uniqs(r); r!=r0; r=uniqs(r)) {
		p = r->prog;
		copysub(&p->src1, v1, v2, 1);
		copysub(&p->dst, v1, v2, 1);
		if(opt('P'))
			print("%i\n", r->prog);
	}
	t = v1->type;
	v1->type = v2->type;
	v2->type = t;
	if(opt('P'))
		print("%i last\n", r->prog);
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
	Inst *p;
	Adres *v1, *v2;
	Reg *r;

	p = r0->prog;
	v1 = &p->src1;
	v2 = &p->dst;
	if(copyas(v1, v2))
		return 1;
	for(r=firstr; r!=R; r=r->next)
		r->active = 0;
	return copy1(v1, v2, r0->s1, 0);
}

int
copy1(Adres *v1, Adres *v2, Reg *r, int f)
{
	int t;
	Inst *p;

	if(r->active) {
		if(opt('P'))
			print("act set; return 1\n");
		return 1;
	}
	r->active = 1;
	if(opt('P'))
		print("copy %a->%a f=%d\n", v1, v2, f);
	for(; r != R; r = r->s1) {
		p = r->prog;
		if(opt('P'))
			print("%i", p);
		if(!f && uniqp(r) == R) {
			f = 1;
			if(opt('P'))
				print("; merge; f=%d", f);
		}
		t = copyu(p, v2, A);
		switch(t) {
		case 2:	/* rar, cant split */
			if(opt('P'))
				print("; %a rar; return 0\n", v2);
			return 0;

		case 3:	/* set */
			if(opt('P'))
				print("; %a set; return 1\n", v2);
			return 1;

		case 1:	/* used, substitute */
		case 4:	/* use and set */
			if(f) {
				if(!opt('P'))
					return 0;
				if(t == 4)
					print("; %a used+set and f=%d; return 0\n", v2, f);
				else
					print("; %a used and f=%d; return 0\n", v2, f);
				return 0;
			}
			if(copyu(p, v2, v1)) {
				if(opt('P'))
					print("; sub fail; return 0\n");
				return 0;
			}
			if(opt('P'))
				print("; sub %a/%a", v2, v1);
			if(t == 4) {
				if(opt('P'))
					print("; %a used+set; return 1\n", v2);
				return 1;
			}
			break;
		}
		if(!f) {
			t = copyu(p, v1, A);
			if(!f && (t == 2 || t == 3 || t == 4)) {
				f = 1;
				if(opt('P'))
					print("; %a set and !f; f=%d", v1, f);
			}
		}
		if(opt('P'))
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
copyu(Inst *p, Adres *v, Adres *s)
{

	switch(p->op) {

	default:
		if(opt('P'))
			print("unknown op %A\n", p->op);
		return 2;

	case ALEAL:	/* lhs addr, rhs store */
		if(copyas(&p->src1, v))
			return 2;


	case ANOP:	/* rhs store */
	case AMOVL:
	case AMOVBLSX:
	case AMOVBLZX:
	case AMOVWLSX:
	case AMOVWLZX:
		if(copyas(&p->dst, v)) {
			if(s != A)
				return copysub(&p->src1, v, s, 1);
			if(copyau(&p->src1, v))
				return 4;
			return 3;
		}
		goto caseread;

	case AROLB:
	case AROLL:
	case AROLW:
	case ARORB:
	case ARORL:
	case ARORW:
	case ASALB:
	case ASALL:
	case ASALW:
	case ASARB:
	case ASARL:
	case ASARW:
	case ASHLB:
	case ASHLL:
	case ASHLW:
	case ASHRB:
	case ASHRL:
	case ASHRW:
		if(copyas(&p->dst, v))
			return 2;
		if(copyas(&p->src1, v))
			if(p->src1.type == D_CX)
				return 2;

		goto caseread;

	case AADDB:	/* rhs rar */
	case AADDL:
	case AADDW:
	case AANDB:
	case AANDL:
	case AANDW:
	case ASUBB:
	case ASUBL:
	case ASUBW:
	case AORB:
	case AORL:
	case AORW:
	case AXORB:
	case AXORL:
	case AXORW:
	case AMOVB:
	case AMOVW:

	case AFMOVB:
	case AFMOVBP:
	case AFMOVD:
	case AFMOVDP:
	case AFMOVF:
	case AFMOVFP:
	case AFMOVL:
	case AFMOVLP:
	case AFMOVV:
	case AFMOVVP:
	case AFMOVW:
	case AFMOVWP:
	case AFMOVX:
	case AFMOVXP:
	case AFADDDP:
	case AFADDW:
	case AFADDL:
	case AFADDF:
	case AFADDD:
	case AFMULDP:
	case AFMULW:
	case AFMULL:
	case AFMULF:
	case AFMULD:
	case AFSUBDP:
	case AFSUBW:
	case AFSUBL:
	case AFSUBF:
	case AFSUBD:
	case AFSUBRDP:
	case AFSUBRW:
	case AFSUBRL:
	case AFSUBRF:
	case AFSUBRD:
	case AFDIVDP:
	case AFDIVW:
	case AFDIVL:
	case AFDIVF:
	case AFDIVD:
	case AFDIVRDP:
	case AFDIVRW:
	case AFDIVRL:
	case AFDIVRF:
	case AFDIVRD:
		if(copyas(&p->dst, v))
			return 2;
		goto caseread;

	case ACMPL:	/* read only */
	case ACMPW:
	case ACMPB:

	case AFCOMB:
	case AFCOMBP:
	case AFCOMD:
	case AFCOMDP:
	case AFCOMDPP:
	case AFCOMF:
	case AFCOMFP:
	case AFCOML:
	case AFCOMLP:
	case AFCOMW:
	case AFCOMWP:
	case AFUCOM:
	case AFUCOMP:
	case AFUCOMPP:
	caseread:
		if(s != A) {
			if(copysub(&p->src1, v, s, 1))
				return 1;
			return copysub(&p->dst, v, s, 1);
		}
		if(copyau(&p->src1, v))
			return 1;
		if(copyau(&p->dst, v))
			return 1;
		break;

	case AJGE:	/* no reference */
	case AJNE:
	case AJLE:
	case AJEQ:
	case AJHI:
	case AJLS:
	case AJMI:
	case AJPL:
	case AJGT:
	case AJLT:
	case AJCC:
	case AJCS:

	case AADJSP:
	case AFLDZ:
	case AWAIT:
		break;

	case ADIVB:
	case ADIVL:
	case ADIVW:
	case AIDIVB:
	case AIDIVL:
	case AIDIVW:
	case AIMULB:
	case AIMULL:
	case AIMULW:
	case AMULB:
	case AMULL:
	case AMULW:

	case ACWD:
	case ACDQ:
		if(v->type == D_AX || v->type == D_DX)
			return 2;
		goto caseread;

	case AMOVSL:
	case AREP:
	case AREPN:
		if(v->type == D_CX || v->type == D_DI || v->type == D_SI)
			return 2;
		goto caseread;

	case AFSTSW:
		if(v->type == D_AX)
			return 2;
		goto caseread;

	case AJMP:	/* funny */
		if(s != A) {
			if(copysub(&p->dst, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->dst, v))
			return 1;
		return 0;

	case ARET:	/* funny */
		if(v->type == Retireg)
			return 2;

	case ACALL:	/* funny */
		if(v->type == Retireg)
			return 2;
		if(v->type == D_SP)
			return D_SP;
		if(s != A) {
			if(copysub(&p->dst, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->dst, v))
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
copyas(Adres *a, Adres *v)
{
	if(a->type != v->type)
		return 0;
	if(regtyp(v))
		return 1;
	if(v->type == D_AUTO || v->type == D_PARAM)
		if(v->ival == a->ival)
			return 1;
	return 0;
}

/*
 * either direct or indirect
 */
int
copyau(Adres *a, Adres *v)
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
copysub(Adres *a, Adres *v, Adres *s, int f)
{
	int t;
	if(copyas(a, v)) {
		t = s->type;
		if(t >= D_AX && t <= D_DI) {
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
