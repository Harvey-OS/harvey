#include "gc.h"

int
propcopies(void)
{
	Reg *r;
	Prog *p;
	int t;

	t = 0;
	for(r = firstr; r; r = r->link){
		p = r->prog;
		if(p->as == AMOVW || p->as == AFMOVF){
			if(regzer(&p->from)){
				p->from.type = D_REG;
				p->from.reg = 0;
			}
			if(regtyp(&p->to) && (regtyp(&p->from) || regzer(&p->from))
			&& p->from.type == p->to.type
			&& (copyprop(r) || subprop(r) && copyprop(r))) {
				excise(r);
				t++;
			}
			if(p->as == AFMOVF
			&& p->from.type == D_INDREG
			&& p->to.type == D_FREG
			&& copypropind(r)){
				excise(r);
				t++;
			}
		}
	}
	return t;
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
	if(v1->reg == REGSP)
		return 0;
	v2 = &p->to;
	if(!regtyp(v2))
		return 0;
	for(r=uniqp(r0); r!=R; r=uniqp(r)) {
		if(uniqs(r) == R)
			break;
		p = r->prog;
		switch(p->as) {
		case ACALL:
			return 0;

		case AADD:
		case ASUB:
		case ASUBR:
		case ASLL:
		case ASRL:
		case ASRA:
		case AOR:
		case AAND:
		case AXOR:
		case AMUL:
		case ADIV:
		case ADIVL:
		case AMOD:
		case AMODL:
			if(p->to.type == v1->type)
			if(p->to.reg == v1->reg)
			if(p->from.type == D_REG || p->as == AADD){
				if(p->reg == NREG)
					p->reg = p->to.reg;
				copysub(&p->to, v1, v2, 1);
				goto gotit;
			}
			break;

		case AMOVW:
			if(p->to.type == v1->type && p->to.reg == v1->reg){
				copysub(&p->to, v1, v2, 1);
				goto gotit;
			}
			break;

		case AFMOVF:
		case AFMOVFN:
			if(p->to.type == v1->type && p->to.reg == v1->reg){
				copysub(&p->to, v1, v2, 1);
				goto gotit;
			}
			if(v1->type == D_FREG && memref(&p->to) && p->reg == NREG){
				p->reg = v2->reg;
				goto gotit;
			}

			/* fall through */
		case AFADD:
		case AFDIV:
		case AFMUL:
		case AFSUB:
			if(v1->type == D_FREG && p->reg == v1->reg){
				p->reg = v2->reg;
				goto gotit;
			}
			if(copyau(&p->from, v2)
			|| copyau(&p->from1, v2)
			|| copyau1(p, v2)
			|| copyau(&p->to, v2))
				return 0;
			if(copysubf(&p->from, v1, v2, 0)
			|| copysubf(&p->from1, v1, v2, 0)
			|| copysub1(p, v1, v2, 0)
			|| copysubf(&p->to, v1, v2, 0))
				return 0;
			continue;
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
	if(debug['P']) {
		print("gotit: %D->%D\n%P", v1, v2, r->prog);
		if(p->from.type == v2->type)
			print(" excise");
		print("\n");
	}
	for(r=uniqs(r); r!=r0; r=uniqs(r)) {
		p = r->prog;
		copysub(&p->from, v1, v2, 1);
		if(p->isf)
			copysub(&p->from1, v1, v2, 1);
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
 *	v1->v2	f=0
 *	(use v2	s/v2/v1/)*
 *	set v1	f=1
 *	use v2	return fail
 *	-----------------
 *	v1->v2	f=0
 *	(use v2	s/v2/v1/)*
 *	set v1	f=1
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
	if(debug['P'])
		print("try copy %P\n", r0->prog);
	return copy1(v1, v2, r0->s1, 0, 0);
}

int
copypropind(Reg *r0)
{
	Prog *p;
	Adr v1, *v2;
	Reg *r;

	p = r0->prog;
	v1 = p->from;
	if(v1.type != D_INDREG || v1.offset)
		return 0;
	v1.type = D_REG;
	v2 = &p->to;
	for(r=firstr; r!=R; r=r->link)
		r->active = 0;
	if(debug['P'])
		print("try copy %P\n", r0->prog);
	return copy1(&v1, v2, r0->s1, 0, 1);
}

int
copy1(Adr *v1, Adr *v2, Reg *r, int f, int ind)
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
		t = copyu(p, v2, A, 0);
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
			if(ind && copyuind(p, v2, v1)
			|| !ind && copyu(p, v2, v1, 1)) {
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
			t = copyu(p, v1, A, 0);
			if(t == 2 || t == 3 || t == 4) {
				f = 1;
				if(debug['P'])
					print("; %Dset and !f; f=%d", v1, f);
			}
			if(ind && memref(&p->to)) {
				f = 1;
				if(debug['P'])
					print("; store and !f; f=%d", f);
			}
		}
		if(debug['P'])
			print("\n");
		if(r->s2 && !copy1(v1, v2, r->s2, f, ind))
			return 0;
	}
	return 1;
}

/*
 * if s is set, substitute for uses v and return failure to substitute
 * else return
 *	1 if v only used,
 *	2 if read-alter-rewrite
 *	3 if set
 *	4 if set and used
 *	0 otherwise (not touched)
 */
int
copyu(Prog *p, Adr *v, Adr *s, int sub)
{
	int n, fu, f1u, f2u, tu;

	switch(p->as) {

	default:
		if(debug['P'])
			print(" (???)");
		return 2;


	case ANOP:	/* read, write */
	case AMOVW:
	case AMOVH:
	case AMOVHU:
	case AMOVB:
	case AMOVBU:
		if(s != A) {
			if(copysub(&p->from, v, s, sub))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, sub))
					return 1;
			return 0;
		}
		fu = copyau(&p->from, v);
		if(fu == 2)
			return 2;
		if(copyas(&p->to, v)) {
			if(fu)
				return 4;
			return 3;
		}
		tu = copyau(&p->to, v);
		if(tu == 2)
			return 2;
		if(fu || tu)
			return 1;
		return 0;

	case AADD:	/* read read write */
	case ASUB:
	case ASUBR:
	case ASLL:
	case ASRL:
	case ASRA:
	case AOR:
	case AAND:
	case AXOR:
	case AMUL:
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
		if(s != A) {
			if(copysub(&p->from, v, s, sub))
				return 1;
			if(copysub1(p, v, s, sub))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, sub))
					return 1;
			return 0;
		}
		if(copyas(&p->to, v)) {
			if((p->from.type == D_REG || p->as == AADD)
			&& p->reg == NREG)
				p->reg = p->to.reg;
			/*
			 * can't change the reg if use and set
			 */
			if(p->reg == NREG)
				return 2;
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

	case AFMOVF:
	case AFMOVFN:
	case AFMOVFW:
	case AFMOVWF:
	case AFMOVFH:
	case AFMOVHF:
		if(s != A) {
			if(copysubf(&p->from, v, s, sub))
				return 1;
			if(!copyas(&p->to, v))
				if(copysubf(&p->to, v, s, sub))
					return 1;
			return 0;
		}
		n = 0;
		if(p->reg == NREG
		&& (p->as == AFMOVF || p->as == AFMOVFN)
		&& memref(&p->to)
		&& p->from.type == D_FREG)
			p->reg = p->from.reg;
		if(copyas(&p->to, v) || copyau1(p, v))
			n = 3;
		fu = copyau(&p->from, v);
		tu = copyau(&p->to, v);
		if(fu == 2 || tu == 2)
			return 2;
		if(fu || !copyas(&p->to, v) && tu)
			return n + 1;
		return n;

	case AFMADD:
	case AFMADDN:
	case AFMSUB:
	case AFMSUBN:
	case AFADD:
	case AFDIV:
	case AFMUL:
	case AFSUB:
		if(s != A) {
			if(copysubf(&p->from, v, s, sub))
				return 1;
			if(copysubf(&p->from1, v, s, sub))
				return 1;
			if(copysubf(&p->from2, v, s, sub))
				return 1;
			if(!copyas(&p->to, v))
				if(copysubf(&p->to, v, s, sub))
					return 1;
			return 0;
		}
		n = 0;
		if(copyau1(p, v))
			n = 3;
		fu = copyau(&p->from, v);
		f1u = copyau(&p->from1, v);
		f2u = copyau(&p->from2, v);
		tu = copyau(&p->to, v);
		if(fu == 2 || f1u == 2 || f2u == 2 || tu == 2)
			return 2;
		if(fu || f1u || f2u || tu)
			return n + 1;
		return n;

	case ADOEND:
	case ABRA:	/* no reference */
		return 0;

	case ADO:	/* read */
	case ADOLOCK:	/* read */
	case ACMP:	/* read read */
		if(s != A) {
			if(copysub(&p->from, v, s, sub))
				return 1;
			return copysub(&p->to, v, s, sub);
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		break;

	case ADBRA:	/* funny */
		if(s != A)
			return copysub(&p->from, v, s, sub);
		if(copyau(&p->from, v))
			return 2;
		break;

	case AJMP:	/* funny */
		if(s != A) {
			if(copysub(&p->to, v, s, sub))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ARETURN:	/* funny */
		if(v->type == D_REG && v->reg == REGRET)
			return 2;
		if(v->type == D_FREG && v->reg == FREGRET)
			return 2;

	case ACALL:	/* funny */
		if(v->type == D_REG) {
			if(v->reg <= REGEXT && v->reg > exregoffset)
				return 2;
			if(v->reg == REGARG || v->reg == REGSP)
				return 2;
		}
		if(v->type == D_FREG
		&& v->reg <= FREGEXT
		&& v->reg > exfregoffset)
			return 2;

		if(s != A) {
			if(copysub(&p->to, v, s, sub))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 4;
		return 3;

	case ATEXT:	/* funny */
		if(v->type == D_REG && v->reg == REGARG)
			return 3;
		return 0;
	}
	return 0;
}

/*
 * substitute indreg s for reg v
 * return failure to substitute
 */
int
copyuind(Prog *p, Adr *v, Adr *s)
{
	switch(p->as){
	default:
		if(debug['P'])
			print(" (???)");
		return 1;

	case AADD:
	case ASUB:
	case ASUBR:
	case ASLL:
	case ASRL:
	case ASRA:
	case AOR:
	case AAND:
	case AXOR:
	case AMUL:
	case ADIV:
	case ADIVL:
	case AMOD:
	case AMODL:
	case ACMP:
	case ABRA:
	case AJMP:
	case ARETURN:
	case ACALL:
	case ATEXT:
		return 0;

	case ANOP:	/* read, write */
	case AMOVW:
	case AMOVH:
	case AMOVHU:
	case AMOVB:
	case AMOVBU:
		if(copysubind(&p->from, v, s, 1))
			return 1;
		return 0;

	case AFMOVF:
	case AFMOVFN:
	case AFMOVFW:
	case AFMOVWF:
	case AFMOVFH:
	case AFMOVHF:
		if(memref(&p->to) && p->reg == NREG
		|| copysubind(&p->from, v, s, 1))
			return 1;
		return 0;

	case AFADD:
	case AFMUL:
	case AFSUB:
		if(copysubind(&p->from, v, s, 1))
			return 1;
		if(copysubind(&p->from1, v, s, 1))
			return 1;
		return 0;

	/*
	 * must use registers for now
	 */
	case AFDIV:
		return 1;
	}
}
