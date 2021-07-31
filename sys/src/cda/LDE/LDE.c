#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "y.tab.h"
#include "op.h"
#define LEQ 3
#define INSWT 4
#define CARRY 5
#define LTN 6
#define NULLARGS (void *) 0, (void *) 0 
#define RETONE(mt) {(mt)->care = 0; (mt)->true = 0; return((mt) + (flip ? 0 : 1) );}
#define RETZERO(mt) {(mt)->care = 0; (mt)->true = 0; return((mt) + (flip ? 1 : 0) );}

Minterm* caseneg(int, unsigned int, Node *, Node *, Minterm *);
Minterm* varls(int, unsigned int, Node *, Node *, Minterm *);
Minterm* varrs(int, unsigned int, Node *, Node *, Minterm *);
Minterm* caseinswt(int, unsigned int, Node * , Node* , Minterm *, int *, Node **);
Minterm* caseswitch(int , unsigned int , Node * , Node * , Minterm *);
Minterm* casesub(int , unsigned int , Node * , Node * , Minterm *);
Minterm* casexor(int , unsigned int , Node * , Node * , Minterm *);
Minterm* caselt(int , unsigned int , Node * , Node * , Minterm *);
Minterm* caseadd(int , unsigned int , Node * , Node * , Minterm *);
Minterm* casecarry(int , unsigned int , Node * , Node * , Minterm *);
Minterm* caseeq( int , Node * , Node * , Minterm *);
Minterm* and(int, Minterm *, Minterm *, Minterm *);
Minterm* logic(int, Node *, Minterm *);
void qsort(void *base, long nel, long width, int (*compar)(void*, void*));
int min_sort(Minterm * dest, Minterm *start, int count);



int
min_min(Minterm *m0, Minterm *m1, Minterm * m2) 
{

/* special case return -1 if have xn & !xn */
	if(m1->care & m2->care
			& (m1->true ^ m2->true)) {
		return (-1);
	}

	m0->care = (m1->care | m2->care);
	m0->true = (m1->true & m1->care) | (m2->true & m2->care);
	return (0);
}

#define TWARGS flip, bit, code, left, right, mt

Minterm*
tree_walk(int flip, unsigned int bit, int code, Node * left, Node * right, int Czero, int Cone, Minterm * mt, void * arg0, void * arg1) 
{
	Hshtab * hp;
	unsigned int lo;
	int i;
	Minterm *mt0 , *mt1, *mt2;
	Node *tp0;
if(yydebug) {
	fprintf(stderr, "tree_walk %d %x %s %x %x %x %x %x\n", flip, bit, getop(code), left, right, Czero, Cone, mt);
	fflush(stderr);
}
	if(Cone & bit) RETONE(mt)
	else if(Czero & bit)  RETZERO(mt)
	switch(code) {

   case ASSIGN:
	while((right->code == ASSIGN) || (right->code == DONTCARE))
		right = right->t2;
	return(tree_walk(flip, bit, right->code, right->t1, right->t2, right->Czero, right->Cone, mt, NULLARGS));

   case OR:

	mt1 = tree_walk(flip, bit, left->code, left->t1, left->t2, left->Czero, left->Cone, mt, arg0, arg1);
	if((flip == 0) && ((mt1 - mt) == 1) && (mt->care == 0)) return(mt1);
	if(flip && (mt1 == mt)) return(mt1);
	mt2 = tree_walk(flip, bit, right->code, right->t1, right->t2, right->Czero, right->Cone, mt1, arg0, arg1);
	return(and(flip?0:1, mt, mt1, mt2));

   case AND:

	mt1 = tree_walk(flip, bit, left->code, left->t1, left->t2, left->Czero, left->Cone, mt, NULLARGS);
	if((flip == 0) && (mt1 == mt)) return(mt1);
	if(flip && ((mt1 - mt) == 1) && (mt->care == 0)) return(mt1);
	mt2 = tree_walk(flip, bit, right->code, right->t1, right->t2, right->Czero, right->Cone, mt1, NULLARGS);
	return(and(flip, mt, mt1, mt2));


   case COM:
	return(tree_walk(flip?0:1, bit, left->code, left->t1, left->t2, left->Czero, left->Cone, mt, NULLARGS));

   case NEG:
	return(caseneg(flip, bit, left, right, mt));

   case FIELD:
	lo = LO((Hshtab *) left);
	while(bit != 1) { bit >>= 1; lo += 1; }
	if(lo >= HI((Hshtab *) left))
		RETZERO(mt);
	mt->care = finddepend(fields[lo]);
	mt->true = flip ? 0 : mt->care;
	return(mt+1);

   case BOTH:
   case INPUT:
	if(bit != 1)  RETZERO(mt);
	hp = (Hshtab *) left;
	mt->care = finddepend(hp);
	mt->true = flip ? 0 : mt->care;
	return(mt+1);

   case NUMBER:
	if((((Value) left) & bit) ? !flip : flip ) {
		mt->care = 0;
		mt->true = 0;
		mt0 = (mt+1);
	}
	else mt0 = mt;
if(yydebug) {
	fprintf(stderr,"number: %d\n", left);
	printmt(mt, mt0);
}
	return(mt0);

   case RS:
	if(!vconst(right))
		return(varrs(flip, bit, left, right, mt));
	bit <<= eval(right);
	if(!bit) RETZERO(mt);
	return(tree_walk(flip, bit, left->code, left->t1, left->t2,  left->Czero, left->Cone,mt, NULLARGS));

   case LS:
	if(!vconst(right))
		return(varls(flip, bit, left, right, mt));
	bit >>= eval(right);
	if(!bit) RETZERO(mt);
	return(tree_walk(flip, bit, left->code, left->t1, left->t2,  left->Czero, left->Cone,mt, NULLARGS));
	
   case NE:
   case EQ:
	if(bit != 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			return(mt+1);
		}
		return(mt);
	}
	if(code == NE) flip = flip ? 0 : 1;
   case LEQ:
	return(caseeq(flip, left, right, mt));


   case EQN:
	tp0 = ((Hshtab *) left)->assign;
	return(tree_walk(flip, bit, tp0->code, tp0->t1, tp0->t2, tp0->Czero, tp0->Cone, mt, NULLARGS));


   case LE:
   case GE:
   case GT:
   case LT:
	if(bit != 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			return(mt+1);
		}
		return(mt);
	}
	if((code == GT) || (code == LE)) {
		tp0 = left;
		left = right;
		right = tp0;
	}
	if((code == GE) || (code == LE))
		flip = flip?0:1;
	for(i = 0x80000000; i != 0; i = ((unsigned int) i) >> 1) {
		if(!(left->Cone & right->Cone & i)
		 && !(left->Czero & right->Czero & i)) break;
	}
	return(caselt(flip, i<<1, left, right, mt));


   case LTN:
	return(caselt(flip, bit, left, right, mt));

   case NOT:
	if(bit != 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			return(mt+1);
		}
		return(mt);
	}
	return(logic(flip?0:1, left, mt));

   case LOR:
   case LAND:
	if(bit != 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			return(mt+1);
		}
		return(mt);
	}
	mt0 = logic(flip, left, mt);
	mt1 = logic(flip, right, mt0);
	if(code == LOR) flip = flip?0:1;
	return(and(flip, mt, mt0, mt1));

   case XOR:
	return(casexor(flip, bit, left, right, mt)); 
	
   case CND:
	mt0 = logic(flip, left, mt);
	tp0 = right->t1;
	mt1 = tree_walk(flip, bit, tp0->code, tp0->t1, tp0->t2, tp0->Czero, tp0->Cone, mt0, NULLARGS);
	mt0 = and(flip, mt, mt0, mt1);
	mt1 = logic(flip?0:1, left, mt0);
	tp0 = right->t2;
	mt2 = tree_walk(flip, bit, tp0->code, tp0->t1, tp0->t2, tp0->Czero,tp0->Cone, mt1, NULLARGS);
	mt1 = and(flip, mt0, mt1, mt2);
	mt1 = and(flip?0:1, mt, mt0, mt1);
	return(mt1);
   case SUB:
	return(casesub(flip, bit, left, right, mt));

   case ADD:
	return(caseadd(flip, bit, left, right, mt));

   case CARRY:
	return(casecarry(flip, bit, left, right, mt));

   case SWITCH:
	return(caseswitch(flip, bit, left, right, mt));

   case INSWT:
	return(caseinswt(flip, bit, left, right, mt, (int *) arg0, (Node **) arg1));

   default:
	fprintf(stderr, "unknown code %d\n", code);
	exits("error");;

	}
	return((Minterm * ) 0 );
}


Minterm*
varrs(int flip, unsigned int bit, Node *left, Node *right, Minterm *mt0)
{

	Node t0, t1, t2;
	Minterm *mt1, *mt2;
	int i;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = 0;
	mt1 = mt0;
	for(i = 0; bit; bit <<=1, ++i) {
		t0.code = NUMBER;
		t0.t1 = (Node *) i;
		t1.code = LEQ;
		t1.t1 = right;
		t1.t2 = &t0;
		t2.code = AND;
		t2.t1 = &t1;
		t2.t2 = left;
		mt2 = tree_walk(flip, bit, AND, &t1, &t2, 0, 0, mt1, NULLARGS);
		mt1 = and(flip?0:1, mt0, mt1, mt2);
if(yydebug){
	fprintf(stderr, "ls: bit %x\n", bit); printmt(mt0, mt1);}
	}
	return(mt1);
}

Minterm*
caseneg(int flip,  unsigned int bit, Node *left, Node *right, Minterm *mt)
{
	Node t0;

	USED(right);
	t0.code  = NUMBER;
	t0.t1 = (Node *) 0;
	t0.Czero = ~1;
	t0.Cone = 1;
	return(tree_walk(flip, bit, SUB, &t0, left, 0, 0, mt, NULLARGS));
}	




Minterm*
varls(int flip, unsigned int bit, Node *left, Node *right, Minterm *mt0)
{

	Node t0, t1, t2;
	Minterm *mt1, *mt2;
	int i;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = 0;
	mt1 = mt0;
	for(i = 0; bit; bit >>=1, ++i) {
		t0.code = NUMBER;
		t0.t1 = (Node *) i;
		t1.code = LEQ;
		t1.t1 = right;
		t1.t2 = &t0;
		t2.code = AND;
		t2.t1 = &t1;
		t2.t2 = left;
		mt2 = tree_walk(flip, bit, AND, &t1, &t2, 0, 0, mt1, NULLARGS);
		mt1 = and(flip?0:1, mt0, mt1, mt2);
if(yydebug) {
	fprintf(stderr, "ls: bit %x\n", bit); printmt(mt0, mt1);}
	}
	return(mt1);
}
/*
Minterm* 
casemul(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt, NULLARGS)
{
	Node t0, t1;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	if(bit < (1 << shift) RETZERO(mt);
	t1.code = LS;
	t1.t2 = shift;
	t1.t1 = left;
	t0.code = AND;
	t0.
	t2.code = IMUL;
	t2.t1 = left;
	t2.t2 = right;
	mt0 = tree_walk(flip, bit, ADD, &t0, &t1, mt, lastcasep, defltp);
if(yydebug) {
	fprintf(stderr, "lt: bit %x\n", bit); printmt(mt, mt0);
}
*/

Minterm*
caseswitch(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt)
{
	int lastcase;
	Minterm* mt0;
	Node * deflt;
	deflt = (Node * ) 0;
	lastcase = -1;
	mt0 = tree_walk(flip, bit, INSWT, left, right, 0, 0, mt, &lastcase, &deflt);
if(yydebug) {
	fprintf(stderr, "caseswitch: bit %x\n", bit); printmt(mt, mt0);
}
	return(mt0);
}

Minterm*
caseinswt(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt, int *lastcasep, Node **defltp)
{
	Node *tp, t0, t1, t2, t3, t4, t5;
	Minterm* mt0;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = t3.Czero = t3.Cone = 0;
	t4.Czero = t4.Cone = t5.Czero = t5.Cone = 0;
	tp = right;
	if(tp == (Node *) 0) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			return(mt+1);
		}
		return(mt);
	}
	if (tp->code == ALT) {
		if(tp->t1 == (Node *) 0) {
			if((*defltp)) {
				fprintf(stderr, "switch has 2 deflts\n");
				exits("error");;
			}
			tp = tp->t2;
			if(tp->code != CASE) {
				fprintf(stderr, "switch scrogged\n");
				exits("error");;
			}
			*defltp = tp->t1;
			mt0 =
				tree_walk(flip, bit, INSWT,
					left, tp->t2, 0, 0, mt, lastcasep, defltp);
if(yydebug) {
	fprintf(stderr, "lt: bit %x\n", bit); printmt(mt, mt0);
}
			return(mt0);
		}
		if(!vconst(tp->t1)) {	
			fprintf(stderr, "switch case not constant\n");
			exits("error");;
		}
		*lastcasep = eval(tp->t1) - 1;
		tp = tp->t2;
	}
	if(tp->code != CASE) {
		fprintf(stderr, "switch scrogged\n");
		exits("error");;
	}
	t0.code = NUMBER;
	t0.t1 = (Node *) ++(*lastcasep);
	t1.code = LEQ;
	t1.t1 = left;
	t1.t2 = &t0;
	t2.code = AND;
	t2.t1 = &t1;
	t2.t2 = tp->t1;
	t3.code = INSWT;
	t3.t1 = left;
	t3.t2 = tp->t2;
	mt0 = tree_walk(flip, bit, OR, &t2, &t3, 0, 0, mt, lastcasep, defltp);
if(yydebug) {
	fprintf(stderr, "lt: bit %x\n", bit); printmt(mt, mt0);
}
	return(mt0);
}
	
	
	
		
		


	
Minterm*
casesub(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt)
{
	Node t0, t1, t2, t3, t4, t5;
	Minterm* mt0;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = t3.Czero = t3.Cone = 0;
	t4.Czero = t4.Cone = t5.Czero = t5.Cone = 0;
	t0.code = ADD;
	t0.t1 = &t1;
	t0.t2 = &t2;
	t1.code = NUMBER;
	t1.t1 = (Node *) 1;
	t2.code = COM;
	t2.t1 = right;
	mt0 = tree_walk(flip, bit, ADD, left, &t0, 0, 0, mt, NULLARGS);
if(yydebug) {
	fprintf(stderr, "lt: bit %x\n", bit); printmt(mt, mt0);
}
	return(mt0);
}


Minterm*
caselt(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt)
{
	Node t0, t1, t2, t3, t4, t5;
	Minterm* mt0;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = t3.Czero = t3.Cone = 0;
	t4.Czero = t4.Cone = t5.Czero = t5.Cone = 0;
	if(bit == 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			mt0 = mt+1;
		}
		else mt0 = mt;
	}
	else {
		if(bit == 0) bit = 0x80000000;
		else bit >>= 1;
		t0.code = AND;
		t0.t1 = &t1;
		t0.t2 = right;
		t1.code = COM;
		t1.t1 = left;
		t1.t2 = (Node *) 0;
		t2.code = AND;
		t2.t1 = &t3;
		t2.t2 = &t4;
		t3.code = COM;
		t3.t1 = &t5;         /*          LT  RT             */
		t3.t2 = (Node *) 0;  /*            XOR              */
		t4.code = LTN;       /*    LT NUL   5 NUL LT RT     */
		t4.t1 = left;        /*     COM      COM   LTN      */
		t4.t2 = right;       /*      1   RT   3     4       */
		t5.code = XOR;       /*        AND      AND         */
		t5.t1 = left;        /*         0        2          */
		t5.t2 = right;       /*            OR               */
		mt0 = tree_walk(flip, bit, OR, &t0, &t2, 0, 0, mt, NULLARGS);
	}
if(yydebug) {
	fprintf(stderr, "lt: bit %x\n", bit); printmt(mt, mt0); fflush(stderr);
}
	return(mt0);
}


Minterm*
casecarry(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt)
{
	Node t0, t1, t2, t3;
	Minterm* mt0;
	t0.Czero = t0.Cone = t1.Czero = t1.Cone = 0;
	t2.Czero = t2.Cone = t3.Czero = t3.Cone = 0;
	if(bit != 1) {
		mt0 = tree_walk(flip, bit>>1, left->code, left->t1, left->t2, left->Czero, left->Cone, mt, NULLARGS);
		if(mt0 == mt) {
			mt0 = tree_walk(flip, bit>>1, right->code, right->t1, right->t2, right->Czero, right->Cone, mt, NULLARGS);
			if(mt0 == mt) bit = 1;
		}
	} 
	if(bit == 1) {
		if(flip) {
			mt->care = 0;
			mt->true = 0;
			mt0 = mt+1;
		}
		else mt0 = mt;
	}
	else {
		bit >>= 1;
		t0.code = AND;
		t0.t1 = left;
		t0.t2 = right; 
		t1.code = CARRY;
		t1.t1 = left;
		t1.t2 = right;
		t2.code = OR;            /*          L R    L R   */
		t2.t1 = left;            /*    L R    C      |    */
		t2.t2 = right;           /*     *         *       */
		t3.code = AND;           /*           |           */
		t3.t1 = &t1;
		t3.t2 = &t2;
		mt0 = tree_walk(flip, bit, OR, &t0, &t3, 0, 0, mt, NULLARGS);
	}
if(yydebug) {
	fprintf(stderr, "carry: bit %x\n", bit); printmt(mt, mt0);
}
	return(mt0);
}


Minterm*
caseadd(int flip, unsigned int bit, Node * left, Node* right, Minterm *mt)
{
	Node t1, t2;
	Minterm *mt0;
	t2.Czero = t2.Cone = t1.Czero = t1.Cone = 0;
	t1.code = CARRY;
	t1.t1 = left;
	t1.t2 = right;
	t2.code = XOR;
	t2.t1 = left;
	t2.t2 = right;
	mt0 = tree_walk(flip, bit, XOR, &t1, &t2, 0, 0, mt, NULLARGS);
if(yydebug) {	fprintf(stderr, "add: bit %x\n", bit); printmt(mt, mt0);}
	return(mt0);
}

Minterm*
logic(int flip, Node *tp, Minterm *mt)
{
	unsigned int i;
	Minterm *mt0, *mt1;
	mt0 = tree_walk(flip, 1, tp->code,  tp->t1, tp->t2, 0, 0, mt, NULLARGS); 
	for(i = 2 ; i; i <<= 1) {
		mt1 = tree_walk(flip, i, tp->code,  tp->t1, tp->t2, 0, 0, mt0, NULLARGS); 
		mt0 = and(flip?0:1, mt, mt0, mt1);
	}
	return(mt0);
}

Minterm*
caseeq(int flip, Node * left, Node * right, Minterm *mt)
{
	Minterm *mt0, *mt1;
	unsigned int i;
	if(flip == 0) {
		mt->care = 0;
		mt->true = 0;
		mt0 = mt + 1;
	}
	else 
		mt0 = mt;
	for(i = 1; i; i <<= 1) {
		mt1 = casexor(flip?0:1, i, left, right, mt0);
		mt0 = and(flip, mt, mt0, mt1);
	}
	return(mt0);
}
		
Minterm*
casexor(int flip, unsigned int bit, Node * left, Node * right, Minterm *mt)
{
	Node t1, t2, t3, t4;     
	Minterm *mt0;
	t1.Czero = t1.Cone = t2.Czero = t2.Cone = 0;
	t3.Czero = t3.Cone = t4.Czero = t4.Cone = 0;
	t1.code = t2.code = AND;
	t1.t1 = left;
	t1.t2 = &t3;
	t3.code = COM;
	t3.t1 = right;
	t2.t1 = &t4;
	t2.t2 = right;
	t4.code = COM;
	t4.t1 = left;
	mt0 = tree_walk(flip, bit, OR, &t1, &t2, 0, 0, mt, NULLARGS);
if(yydebug) {	fprintf(stderr, "xor: bit %x\n", bit); printmt(mt, mt0);}
	return(mt0);
}

int get_mask(Minterm *, Minterm *);
Minterm * reduce(Minterm*, Minterm*, int, Minterm*);

Minterm*
and(int flip, Minterm * mt0, Minterm * mt1, Minterm * mt2) 
{
	int n, mask;
	Minterm *m0, *m1, *m2;
	if(flip == 0) {
		for(m0 = mt0, m2 = mt2; m0 < mt1; ++m0)
			for(m1 = mt1; m1 < mt2; ++m1)
				if(min_min(m2, m0, m1) >= 0) ++m2;
		n = min_sort(mt0, mt2, m2-mt2);
	}
	else
		n = min_sort(mt0, mt0, mt2-mt0);
	if(n > 1000) {
		mask = get_mask(mt0, mt0+n);
		mt1 = reduce(mt0, mt0+n, mask, &minterms[NCOMP]);
		n = mt1-mt0;
	}
if(yydebug) {fprintf(stderr, "and: "); printmt(mt0, mt0+n);}
	return(mt0+n);
}

#ifdef PLAN9
int
min_cmp(Minterm * m1, Minterm *m2)
#endif
#ifndef PLAN9
int
min_cmp(void * mm1, void *mm2)
#endif
{
#ifndef PLAN9
	Minterm *m1, *m2;
	m1 = (Minterm *) mm1;
	m2 = (Minterm *) mm2;
#endif
	if(m1->care > m2->care) return(1);
	if(m1->care < m2->care) return(-1);
	if(m1->true > m2->true) return(1);
	if(m1->true < m2->true) return(-1);
	return(0);
}

int
min_cov(Minterm * dest, Minterm *start, int count)
{
	Minterm *mt0, *mt1, *mt2;
	int n, care0, care1, true0, true1;
	mt2 = start + count;
	care1 = 0;
	for(mt0 = start; mt0 < mt2; ++mt0) {
		care0 = mt0->care;
		true0 = mt0->true & care0;
		if((care0 == 0) && (true0 == (~0 & care1))) continue;
		for(mt1 = start; mt1 < mt2; ++mt1) {
			if(mt1 == mt0) continue;
			care1 = mt1->care;
			true1 = mt1->true & care1;
			if((care1 == 0) && (true1 == (~0 & care1))) continue;
			if((care1 == care0) && (true0 == true1)) {
				mt1->care = 0;
				mt1->true = ~0;
				continue;
			}
			n = care1 & care0;
			if((n & (true0 ^ true1)) != 0 ) continue;
			if(care1 & ~care0) continue;
			mt0->care = 0;
			mt0->true = ~0;
		}
	}
	for(mt0 = start, mt1 = dest; mt0 < mt2; ++mt0) {
		if(mt0->care == 0) {
			if(mt0->true == ~0) continue;
			if(mt0->true == 0) {
				dest->care = 0;
				dest->true = 0;
				return(1);
			}
		}
		*mt1++ = *mt0;
	}
	return(mt1-dest);
}

int
min_join(Minterm *start, int count)
{
	Minterm *mt0, *mt1, *mt2, *mt3;
	unsigned int n, care0, care1, true0, true1;
	if(count < 2) return(count);
	if((start->care == 0) && (start->true == 0))  return(1);
	mt3 = mt2 = start + count;
	for(mt0 = start; mt0 < mt2; ++mt0) {
		care0 = mt0->care;
		true0 = mt0->true & care0;
		for(mt1 = mt0+1; mt1 < mt2; ++mt1) {
			care1 = mt1->care;
			true1 = mt1->true & care1;
			if(care1 != care0) continue;
			if((care1 == 0) && true1 == 0) {
				start->care = 0;
				start->true = 0;
				return(1);
			}
			n = (true1 ^ true0);
			n = ~n ^ -((int) n);
			n = n ^ n >> 1;
			if(n != (true1 ^ true0)) continue;
			mt3->care = care0 & ~n;
			mt3->true = true0 & ~n;
			if(mt3 == &minterms[NCOMP-1]) {
fprintf(stderr, "%x/%x %x/%x %x %x %x %x\n", care0, true0, care1, true1, (true1 ^ true0),
~(true1 ^ true0), -((int) (true1 ^ true0)), -1 >> 1, n);
printmt(start, mt3);
				fprintf(stderr, "out of minterms");
				exits("error");;
			}
			++mt3;
		}
		mt2 = mt3;
	}
	return(mt3-start);
}
extern int REDUCE;
			
int
min_sort(Minterm * dest, Minterm *start, int count)
{
	Minterm * mt,*m1;
	int n;
	if(count == 0) return(0);
	if(REDUCE == 0) {
		qsort(start, count, sizeof(Minterm), min_cmp);
		m1= start + count;
		if((start->care == 0) && (start->true == 0)) {
			dest->care = 0;
			dest->true = 0;
			return(1);
		}
		for(mt = start,n = 0; mt < m1; ++mt) {
			*dest++ = *mt;
			++n;
			while(min_cmp(mt, (mt+1)) == 0)
				if(++mt >= m1-1) break;
			
		} 
	}
	else {
		n = min_join(start, count);
		n = min_cov(dest, start, n);
	}
	return(n);
}
	
int
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
