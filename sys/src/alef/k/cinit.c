#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

void
cominit(Node *v, Type *t, Node *i, int off)
{
	Type *comt;
	int ind, l, cnt, dim, sz;
	Node n, **ilist, *in, *treg, *tdat;

	if(opt('i')) {
		print("COMINIT %N T %T\n", v, t);
		ptree(i, 0);
	}

	switch(t->type) {
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
	case TIND:
		if(i->sun >= Sucall) {
			treg = stknode(builtype[TIND]);
			v->type = OREGISTER;
			assign(v, treg);
			tdat = stknode(i->t);
			genexp(i, tdat);
			assign(treg, v);
			v->type = OINDREG;
			v->ival = off;
			v->t = t;
			genexp(tdat, v);
			break;
		}
		v->ival = off;
		v->t = t;
		n.type = OASGN;
		n.t = t;
		n.left = v;
		n.right = i;
		n.islval = 0;
		genexp(&n, ZeroN);
		break;
	
	case TARRAY:
		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);
		cnt = veccnt;

		sz = t->next->size;
		ind = 0;
		dim = 0;
		for(l = 0; l < cnt; l++) {
			if(ind > dim)
				dim = ind;
			in = ilist[l];
			if(in->type != OINDEX) {
				cominit(v, t->next, in, off+ind);
				ind += sz;
				continue;
			}
			ind = in->left->ival*sz;	
			cominit(v, t->next, in->right, off+ind);
			ind += sz;
		}	
		break;

	case TADT:
	case TUNION:
	case TAGGREGATE:
		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);

		cnt = veccnt;
		comt = t->next;
		for(l = 0; l < cnt; l++) {
			cominit(v, comt, ilist[l], off+comt->offset);
			for(;;) {
				comt = comt->member;
				if(comt == ZeroT)
					break;
				/* Skip adt prototypes */
				if(comt->type != TFUNC)
					break;
			}
		}
		break;

	case TFUNC:
	case TVOID:
	case TCHANNEL:
		fatal("cominit");
	}
}
