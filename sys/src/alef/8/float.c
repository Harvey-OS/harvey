#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

/*
 * Compile floating expressions
 */
void
genfexp(Node *rval, Node *lval)
{
	Node *l, *r, nl;

	l = rval->left;
	r = rval->right;

	switch(rval->type) {
	case OADD:
	case OSUB:
	case ODIV:
	case OMUL:
		if(l->sun >= r->sun) {
			genexp(l, F0);
			if(isaddr(r))
				codfop(rval, r, F0, FNONE);
			else {
				genexp(r, F0);
				codfop(rval, F0, F1, FPOP);
			}
		}
		else {
			genexp(r, F0);
			if(isaddr(l))
				codfop(rval, l, F0, FREV);
			else {
				genexp(l, F0);
				codfop(rval, F0, F1, FPOP|FREV);
			}
		}
		assign(F0, lval);
		break;

	case OADDEQ:
	case OSUBEQ:
	case ODIVEQ:
	case OMULEQ:
		while(l->type == OCONV)
			l = l->left;

		if(l->sun >= r->sun) {
			toaddr(l, &nl);
			genexp(r, F0);
		}
		else {
			genexp(r, F0);
			toaddr(l, &nl);
		}

		if(l->t->type != TFLOAT) {
			assign(&nl, F0);
			codfop(rval, F0, F1, FPOP|FREV);
		}
		else
			codfop(rval, &nl, F0, FREV);

		if(lval != ZeroN)
			instruction(AFMOVD, F0, F0);

		assign(F0, &nl);
		if(lval != ZeroN)
			assign(F0, lval);

		if(!isaddr(l))
			regfree(&nl);
		break;
	}
}
