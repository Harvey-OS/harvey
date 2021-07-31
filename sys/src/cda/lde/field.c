#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

/*
 * check fields diagnostic
 * prints current values of all fields
 */
void
checkfields(void)
{
	int i,j;
	Hshtab *tp;
	int lo, hi;

	fprint(1, "checkfields\n");
	for(i=0; i<HSHSIZ; i++) {
		tp = &hshtab[i];
		if(tp->name[0] == 0 || tp->code != FIELD)
			continue;
		lo = LO(tp);
		hi = HI(tp);
		fprint(1, "field %s %d %d:\n", tp->name, lo, hi);
		for(j=lo;j<hi;j++) {
			tp = fields[j];
			fprint(1, "\t%s %ld %ld\n",tp->name,tp->val,tp->iref);
		}
	}
}

void
checkio(Hshtab *tp, int type)
{
	int i, lo, hi;

	if(tp->code == FIELD){
		lo = LO(tp);
		hi = HI(tp);
		for(i=lo; i<hi; i++)
			checkio(fields[i], type);
		return;
	}
	if(tp->code == BOTH)
		return;
	if(tp->code != type)
		yyerror("%s %s used as %s",
			tp->code==INPUT ? "input" : "output", tp->name,
			type==INPUT ? "input"  : "output");
}

/*
 * load a field
 */
Value
loadf(Hshtab *tp)
{
	int i, lo, hi;
	Value x;

	lo = LO(tp);
	hi = HI(tp);
	x = 0;
	for(i=lo; i<hi;i++) {
		tp = fields[i];
		x |= (tp->val) << (i-lo);
	}
	return(x);
}

/*
 * store a field
 */
void
storef(Hshtab *tp, Value x, int dc)
{
	int i, lo, hi;

	lo = LO(tp);
	hi = HI(tp);
	if(dc)
		goto dcsf;
	for(i=lo; i<hi; i++) {
		tp = fields[i];
		rom[tp->oref] = (x >> (i-lo)) & 1;
	}
	return;

dcsf:
	for(i=lo; i<hi; i++) {
		tp = fields[i];
		dcare[tp->oref] = (x >> (i-lo)) & 1;
	}
}

/*
 * codes:
 *	EQN	user defined temp name
 *	CEQN	same as EQN, but known to be constant
 *	FIELD	set of other codes
 *	INPUT	input pin
 *	OUTPUT	output pin
 *	BOTH	input and output pin
 */

void
setctl(Hshtab *h, char type)
{
	int lo, hi, i;

	if(h->code == FIELD){
		lo = LO(h);
		hi = HI(h);
		for(i=lo; i<hi; i++)
			setctl(fields[i], type);
		return;
	}
	h->type |= type;
}

void
setiused(Hshtab *h, int mask)
{
	int lo, hi, i, index;
	if(h->code == FIELD) {
		lo = LO(h);
		hi = HI(h);
		for(i=lo, index=1 ; i<hi; i++,index <<= 1)
			if(mask & index)
				setiused(fields[i], 1);
		return;
	}
	if((mask & 1) == 0)
		return;
	if(h->iused)
		return;
	h->iused = 1;
	switch(h->code) {
	case INPUT:
	case BOTH:
		break;

	case EQN:
		if(!h->assign)
			fprint(2, "used and not set: %s\n", h->name);
		break;
	}
}

void
setoused(Hshtab *h, Node *tp)
{
	int lo, hi, i;

	if(h->code == FIELD) {
		lo = LO(h);
		hi = HI(h);
		for(i=lo; i<hi; i++) {
			fields[i]->fref = i;
			setoused(fields[i], tp);
		}
		return;
	}
	if(h->assign) {
		fprint(2, "set more then once: %s\n", h->name);
		return;
	}
	h->assign = tp;
}

void
setdcused(Hshtab *h, Node *tp)
{
	int lo, hi, i;

	if(h->code == FIELD) {
		lo = LO(h);
		hi = HI(h);
		for(i=lo; i<hi; i++)
			setdcused(fields[i], tp);
		return;
	}
	if(h->dontcare) {
		fprint(2, "dc: set more then once: %s\n", h->name);
		return;
	}
	h->dontcare = tp;
}

void
checkdefns(void)
{
	Hshtab *h;

	for(h = &hshtab[0]; h < &hshtab[HSHSIZ]; h++) {
		if(h->name[0] == 0)
			continue;
		switch(h->code) {
		case OUTPUT:
		case INPUT:
		case BOTH:
			if(h->code != OUTPUT && !h->iused)
				fprint(2, "input not used: %s\n", h->name);
			if(h->code != INPUT && !h->assign)
				fprint(2, "output not set: %s\n", h->name);
			break;

		case CEQN:
			if(h->assign && !h->iused)
				fprint(2, "temp set and not used: %s\n", h->name);
			break;
		}
	}
}

/*
 * add a control output or field (clock, set, reset, enable)
 */
Hshtab *
addctl(Hshtab *hp, char type)
{
	Hshtab *hp1, *hp2;
	int i, lo, hi;
	char buf[NCPS];

	sprint(buf, "%s@%c", hp->name, type);
	hp1 = lookup(buf, 1);
	hp1->pin = 0;
	hp1->iused = 0;
	hp1->assign = 0;
	if(hp->code == FIELD){
		hp1->code = FIELD;
		hp1->type = 0;
		SETLO(hp1, nextfield);
		lo = LO(hp);
		hi = HI(hp);
		for(i = lo; i < hi; ++i){
			hp = fields[i];
			hp2 = addctl(hp, type);
			fields[nextfield++] = hp2;
			if(nextfield >= NFIELDS)
				yyerror("too many fields\n");
		}
		SETHI(hp1, nextfield);
		return hp1;
	}
	hp1->code = OUTPUT;
	hp1->oref = ++noutputs;
	if(noutputs >= NLEAVES) {
		fprint(2, "too many outputs\n");
#ifdef PLAN9
		exits("too many outputs");
#else
		exit(1);
#endif
	}
	outputs[noutputs] = hp1;
	switch(type) {
	case 's':
		hp1->type |= CTLSET;
		break;
	case 'c':
		hp1->type |= CTLCLR;
		break;
	case 'e':
		hp1->type |= CTLENB;
		break;
	case 'd':
		hp1->type |= CTLCLK;
		break;
	case 't':
		hp1->type |= CTLTOG;
		break;
	case 'g':
		hp1->type |= CTLCKE;
		break;
	}
	return hp1;
}
