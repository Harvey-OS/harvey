#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

#define	NSWIT	150
#define	NCOMP	10000
Rpol	revpol[NCOMP];
Rpol	*rp;


swcmp(void *a1, void *a2)
{
	Sw *s1, *s2;

	s1 = a1;
	s2 = a2;
	if(s1->val > s2->val)
		return 1;
	if(s1->val < s2->val)
		return -1;
	/* on equal, preserve order */
	if(s1 > s2)
		return 1;
	if(s1 < s2)
		return -1;
	return 0;
}

int
getmsk(Hshtab *hp, Node *tp)
{
	Hshtab * hp1;
	hp1 = (Hshtab *) tp->t1;
	if(hp1->code != FIELD)
		return(1);
	return(1 << (hp->fref - LO(hp1)));
}

void
compile(int o)
{
	Hshtab *hp;
	Node *tp;
	rp = &revpol[0];
	hp = outputs[o];
	if(tp = hp->dontcare) comp(tp, getmsk(hp, tp));
	if(tp = hp->assign) comp(tp, getmsk(hp, tp));
	(rp++)->code = END;
}

void
compsw(Node *tp, int mask)
{
	Rpol *rp1, *rp2;
	Node *def, *tp1;
	int i, f, n;
	Sw sw[NSWIT];

	tp1 = tp;
	rp2 = 0;
	comp(tp->t1, ALLBITS);
	def = 0;
	n = 0;
	sw[0].val = 0;
	f = 0;
	for(tp = tp->t2; tp; tp = tp->t2) {
		if(tp->code == ALT) {
			if(tp->t1) {
				if(!vconst(tp->t1))
					f++;
				sw[n].val = eval(tp->t1);
				continue;
			}
			tp = tp->t2;
			if(tp == 0)
				break;
			if(tp->code != CASE)
				break;
			if(def != 0) {
				fprint(2, "more than one default\n");
#ifdef PLAN9
				exits("more than one default\n");
#else
				exit(1);
#endif
			}
			def = tp->t1;
			continue;
		}
		if(f)
			continue;
		sw[n].tp = tp->t1;
		n++;
		sw[n].val = sw[n-1].val + 1;
		if(n >= NSWIT-3)
			f++;
	}
	if(f)
		goto slowsw;
	qsort(sw, n, sizeof(sw[0]), swcmp);
	f = sw[n-1].val - sw[0].val + 2;
	if(f >= NSWIT)
		goto slowsw;
	(rp++)->code = DSWITCH;
	(rp++)->val = sw[0].val - 1;	/* base */
	(rp++)->val = f;			/* span */
	rp1 = rp;			/* base of cases */
	rp += f;
	for(i=0; i<f; i++)
		(rp1+i)->rp = rp;	/* default */
	if(def != 0) {
		comp(def, mask);
		(rp++)->code = GOTO;
		rp->rp = rp2;
		rp2 = rp;
		rp++;
	} else
		(rp++)->code = DNOCASE;
	for(i=n-1; i>=0; i--) {
		(rp1+sw[i].val-sw[0].val+1)->rp = rp;
		comp(sw[i].tp, mask);
		if(i > 0) {
			(rp++)->code = GOTO;
			rp->rp = rp2;
			rp2 = rp;
			rp++;
		}
	}
	while(rp2 != 0) {
		rp1 = rp2->rp;
		rp2->rp = rp;
		rp2 = rp1;
	}
	return;

/*
 * direct switch fails,
 * do it compare-at-a-time
 */
slowsw:
	tp = tp1;
	(rp++)->code = SWITCH;
	for(tp = tp->t2; tp; tp = tp->t2) {
		if(tp->code == ALT) {
			if(tp->t1) {
				comp(tp->t1, ALLBITS);
				(rp++)->code = ALT;
				continue;
			}
			tp = tp->t2;
			if(tp == 0)
				break;
			if(tp->code != CASE)
				break;
			continue;
		}
		if(tp->code != CASE)
			break;
		(rp++)->code = CASE;
		rp1 = rp;
		rp++;
		comp(tp->t1, mask);
		(rp++)->code = GOTO;
		rp->rp = rp2;
		rp2 = rp;
		rp++;
		rp1->rp = rp;
	}
	if(def != 0) {
		(rp++)->code = POP2;
		comp(def, mask);
	} else
		(rp++)->code = NOCASE;
	while(rp2 != 0) {
		rp1 = rp2->rp;
		rp2->rp = rp;
		rp2 = rp1;
	}
}
int
islogic(Node *tp)
{
	switch(tp->code) {
	case LT:
	case GT:
	case EQ:
	case NE:
	case GE:
	case LE:
	case LAND:
	case LOR:
		return(1);
	default:
		return(0);
	}
}

void
comp(Node *tp, int mask)
{
	Rpol *rp1, *rp2;
	Hshtab *hp;
	Node *ttp;
	if(vconst(tp)) {
		(rp++)->code = NUMBER;
		(rp++)->val = eval(tp);
		return;
	}
	switch(tp->code) {
	case ELIST:
		if(tp->t1 != 0)
			comp(tp->t1, mask);
		if(tp->t2 != 0)
			comp(tp->t2, mask);
		break;
	case DONTCARE:
		ttp = tp->t2;
		while((ttp->code == ASSIGN) || (ttp->code == DONTCARE)) ttp = ttp->t2;
		comp(ttp, mask);
		(rp++)->code = DCOUTPUT;
		(rp++)->val = mask;
		break;
	case ASSIGN:
		hp = (Hshtab*)tp->t1;
		ttp = tp->t2;
		while((ttp->code == ASSIGN) || (ttp->code == DONTCARE)) ttp = ttp->t2;
		comp(ttp, mask);
		switch(hp->code) {
		default:
			goto bad;
		case EQN:
			(rp++)->code = AEQN;
			(rp++)->hp = hp;
			break;
		case BOTH:
		case OUTPUT:
		case FIELD:
			(rp++)->code = AOUTPUT;
			(rp++)->val = mask;
			break;
		}
		break;
	bad:
		fprint(2,"unknown code in assgn %s\n", hp->name);
#ifdef PLAN9
		exits("unknown code in assign");
#else
		exit(1);
#endif
	case LT:
	case GT:
	case EQ:
	case NE:
	case GE:
	case LE:
	case LAND:
	case LOR:
		if((mask & 1) == 0) {
			(rp++)->code = NUMBER;
			(rp++)->val = 0;
			break;
		}
		comp(tp->t1, ALLBITS);
		comp(tp->t2, ALLBITS);
		(rp++)->code = tp->code;
		break;
	case MUL:
		if(islogic(tp->t1)) {
			comp(tp->t1, 1);
			comp(tp->t2, mask);
			(rp++)->code = tp->code;
			break;
		}
		if(islogic(tp->t2)) {
			comp(tp->t2, 1);
			comp(tp->t1, mask);
			(rp++)->code = tp->code;
			break;
		}
	case ADD:
	case SUB:
		while(mask != (mask | (mask>>1))) 
			mask = (mask | (mask>>1));
		comp(tp->t1, mask);
		comp(tp->t2, mask);
		(rp++)->code = tp->code;
		break;
	case DIV:
	case MOD:
		comp(tp->t1, ALLBITS);
		comp(tp->t2, ALLBITS);
		(rp++)->code = tp->code;
		break;
	case AND:
		if(vconst(tp->t2)) {
			comp(tp->t1, mask & eval(tp->t2));
			comp(tp->t2, mask);
			(rp++)->code = tp->code;
			break;
		}
	case OR:
	case XOR:
		comp(tp->t1, mask);
		comp(tp->t2, mask);
		(rp++)->code = tp->code;
		break;
	case RS:
		if(vconst(tp->t2)) {
			comp(tp->t1, mask << eval(tp->t2));
			comp(tp->t2, mask);
			(rp++)->code = tp->code;
			break;
		}
		comp(tp->t1, ALLBITS);
		comp(tp->t2, ALLBITS);
		(rp++)->code = tp->code;
		break;
	case LS:
		if(vconst(tp->t2)) {
			comp(tp->t1, mask >> eval(tp->t2));
			comp(tp->t2, mask);
			(rp++)->code = tp->code;
			break;
		}
		comp(tp->t1, ALLBITS);
		comp(tp->t2, ALLBITS);
		(rp++)->code = tp->code;
		break;
	case FLONE:
	case FRONE:
	case GREY:
	case NEG:
	case NOT:
		comp(tp->t1, ALLBITS);
		(rp++)->code = tp->code;
		break;
	case COM:
		comp(tp->t1, mask);
		(rp++)->code = tp->code;
		break;
	case CND:
		comp(tp->t1, ALLBITS);
		(rp++)->code = tp->code;
		rp1 = rp;
		rp++;

		tp = tp->t2;
		comp(tp->t1, mask);
		(rp++)->code = GOTO;
		rp2 = rp;
		rp++;

		rp1->rp = rp;
		comp(tp->t2, mask);
		rp2->rp = rp;
		break;
	case SWITCH:
		compsw(tp, mask);
		break;
	case EQN:
		hp = (Hshtab *) tp->t1;
		(rp++)->code = CNDEQN;	/* check if evaluated */
		(rp++)->hp = hp;
		rp1 = rp;		/* yes, get it */
		rp++;
		comp(hp->assign, mask);	/* evaluate it */
		(rp++)->code = GOTO;
		rp2 = rp;		/* jump to next */
		rp++;
		rp1->rp = rp;
		(rp++)->code = EQN;	/* get value */
		(rp++)->hp = hp;
		rp2->rp = rp;		/* next */
		break;
	case BOTH:
	case INPUT:
	case FIELD:
		hp = (Hshtab *) tp->t1;
		setiused(hp, mask);
		(rp++)->code = tp->code;
		(rp++)->hp = hp;
		break;
	case NUMBER:
		(rp++)->code = tp->code;
		(rp++)->tp = tp->t1;
		break;
	default:
		fprint(2,"unknown comp op %d\n", tp->code);
#ifdef PLAN9
		exits("unknown comp op");
#else
		exit(1);
#endif
	}
}

vconst(Node *tp)
{

	switch(tp->code) {
	default:
		return 0;
	case LAND:
	case LOR:
	case AND:
	case OR:
	case XOR:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case LT:
	case GT:
	case EQ:
	case NE:
	case GE:
	case LE:
	case LS:
	case RS:
		if(!vconst(tp->t1))
			return 0;
		if(!vconst(tp->t2))
			return 0;
		return 1;
	case FLONE:
	case FRONE:
	case GREY:
	case NEG:
	case NOT:
	case COM:
		if(!vconst(tp->t1))
			return 0;
		return 1;
	case CND:
		if(!vconst(tp->t1))
			return 0;
		if(eval(tp->t1)) {
			if(!vconst(tp->t2->t1))
				return 0;
			return 1;
		}
		if(!vconst(tp->t2->t2))
			return 0;
		return 1;
	case NUMBER:
		return 1;
	}
}

int
eval1(void)
{
	register Rpol *rp;
	register Value *sp, v;
	register Hshtab *hp;
	int i, ret;
	Value stack[100];

	rp = &revpol[0];
	sp = &stack[100];
	ret = 0;
	for(i = 1; i <= neqns; ++i) eqns[i]->iused = 0;

loop:
	switch((rp++)->code) {
	case ELIST:
		goto loop;
	case POP2:
		sp += 2;
		goto loop;
	case DCOUTPUT:
		v = (rp++)->val;
		ret |= (v & *sp++) ? 2 : 0;
		goto loop;
	case DCFIELD:
		hp = (rp++)->hp;
		storef(hp, *sp++, 1);
		goto loop;
	case AOUTPUT:
		v = (rp++)->val;
		ret |= (v & *sp++) ? 1 : 0;
		goto loop;
	case FIELDS:
		hp = (rp++)->hp;
		storef(hp, *sp++, 0);
		goto loop;
	case AEQN:
		hp = (rp++)->hp;
		hp->val = *sp;
		goto loop;
	case LAND:
		v = *sp++;
		*sp = *sp && v;
		goto loop;
	case LOR:
		v = *sp++;
		*sp = *sp || v;
		goto loop;
	case AND:
		v = *sp++;
		*sp &= v;
		goto loop;
	case OR:
		v = *sp++;
		*sp |= v;
		goto loop;
	case XOR:
		v = *sp++;
		*sp ^= v;
		goto loop;
	case FLONE:
		*sp = flone(*sp);
		goto loop;
	case FRONE:
		*sp = frone(*sp);
		goto loop;
	case GREY:
		*sp = btog(*sp);
		goto loop;
	case NEG:
		*sp = -*sp;
		goto loop;
	case NOT:
		*sp = !*sp;
		goto loop;
	case COM:
		*sp = ~*sp;
		goto loop;
	case ADD:
		v = *sp++;
		*sp += v;
		goto loop;
	case SUB:
		v = *sp++;
		*sp -= v;
		goto loop;
	case MUL:
		v = *sp++;
		*sp *= v;
		goto loop;
	case DIV:
		v = *sp++;
		*sp /= v;
		goto loop;
	case MOD:
		v = *sp++;
		*sp %= v;
		goto loop;
	case LT:
		v = *sp++;
		*sp = *sp < v;
		goto loop;
	case GT:
		v = *sp++;
		*sp = *sp > v;
		goto loop;
	case EQ:
		v = *sp++;
		*sp = *sp == v;
		goto loop;
	case NE:
		v = *sp++;
		*sp = *sp != v;
		goto loop;
	case GE:
		v = *sp++;
		*sp = *sp >= v;
		goto loop;
	case LE:
		v = *sp++;
		*sp = *sp <= v;
		goto loop;
	case LS:
		v = *sp++;
		*sp <<= v;
		goto loop;
	case RS:
		v = *sp++;
		*sp >>= v;
		goto loop;
	case EQN:
	case BOTH:
	case INPUT:
		hp = (rp++)->hp;
		*--sp = hp->val;
		goto loop;
	case FIELD:
		hp = (rp++)->hp;
		*--sp = loadf(hp);
		goto loop;
	case NUMBER:
		v = (rp++)->val;
		*--sp = v;
		goto loop;
	case CNDEQN:
		hp = (rp++)->hp;
		if(!(hp->iused++)) {
			rp++;
			goto loop;
		}
		rp = rp->rp;
		goto loop;
	case CND:
		if(*sp++) {
			rp++;
			goto loop;
		}
	case GOTO:
		rp = rp->rp;
		goto loop;
	case DSWITCH:
		v = *sp++ - (rp++)->val;
		if(v >= (rp++)->val || v < 0)
			v = 0;
		rp = (rp+v)->rp;
		goto loop;
	case SWITCH:
		*--sp = 0;		/* current case */
		goto loop;
	case ALT:
		v = *sp++;
		*sp = v;		/* replace current case */
		goto loop;
	case CASE:
		if(sp[1] == *sp) {
			rp++;
			sp += 2;
			goto loop;
		}
		(*sp)++;		/* increment current case */
		rp = rp->rp;		/* goto next case */
		goto loop;
	case DNOCASE:
		fprint(2, "selector value %ld and no default\n", sp[-1]);
#ifdef PLAN9
		exits("selector value and no default");
#else
		exit(1);
#endif
	case NOCASE:
		fprint(2, "selector value %ld and no default\n", sp[1]);
#ifdef PLAN9
		exits("selector value and no default\n");
#else
		exit(1);
#endif
	case END:
		return(ret);
	default:
		fprint(2,"unknown eval1 op %d\n", (rp-1)->code);
#ifdef PLAN9
		exits("unknown eval1 op");
#else
		exit(1);
#endif
		goto loop;
	}
}
