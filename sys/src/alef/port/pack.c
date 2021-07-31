#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"

	/* function names must be less than 128 chars */
char packfn[]	= "ALEFpack";
char unpackfn[]	= "ALEFupack";
char chpack[] 	= "ALEFchpack";
char chunpack[]	= "ALEFchunpack";

typedef struct Pack Pack;
struct Pack
{
	ulong	sig;
	int	state;
	Pack*	hash;
	Type*	t;
};
Pack*	packhash[NPHASH];

void
packmk(ulong sig, Type *t, int s)
{
	Pack *p, *f, **l;

	l = &packhash[sig%NPHASH];
	for(f = *l; f; f = f->hash)
		if(f->sig == sig)
			return;

	p = malloc(sizeof(Pack));
	p->state = s;
	p->sig = sig;
	p->t = t;
	p->hash = *l;
	*l = p;	
}

Node*
unpacker(Node *c, Node *dp, Node *tp, Type *t)
{
	Type *v;
	Node *n, *y, *x, *tmp;

	switch(t->type) {
	default:
		fatal("unpacker %T", t);
	case TPOLY:
	case TFLOAT:
		fatal("unpacker: not yet implemented %T\n", t);
	case TCHANNEL:
		v = at(TFUNC, at(TIND, builtype[TCHAR]));
		v->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		x = rtnode(chunpack, nil, v);
		n = an(OLIST, dupn(tp), dupn(dp));
		n = an(OCALL, x, n);
		n = an(OASGN, dupn(dp), n);
		c = an(OLIST, c, n);
		break;
	case TINT:
	case TUINT:
		n = an(OIND, dupn(dp), nil);
		n = an(OLSH, n, con(24));
		x = an(OADD, dupn(dp), con(1));
		x = an(OIND, x, nil);
		x = an(OLSH, x, con(16));
		n = an(OLOR, n, x);
		x = an(OADD, dupn(dp), con(2));
		x = an(OIND, x, nil);
		x = an(OLSH, x, con(8));
		n = an(OLOR, n, x);
		x = an(OADD, dupn(dp), con(3));
		x = an(OIND, x, nil);
		n = an(OLOR, n, x);
		x = an(OIND, dupn(tp), nil);
		n = an(OASGN, x, n);
		c = an(OLIST, c, n);
		n = an(OADDEQ, dupn(dp), con(t->size));
		c = an(OLIST, c, n);
		break;		
	case TSINT:
	case TSUINT:
		n = an(OIND, dupn(dp), nil);
		n = an(OLSH, n, con(8));
		x = an(OADD, dupn(dp), con(1));
		x = an(OIND, x, nil);
		n = an(OLOR, n, x);
		x = an(OIND, dupn(tp), nil);
		x = an(OASGN, x, n);
		c = an(OLIST, c, x);
		n = an(OADDEQ, dupn(dp), con(t->size));
		c = an(OLIST, c, n);
		break;		
	case TCHAR:
		n = an(OPINC, dupn(dp), nil);
		n = an(OIND, n, nil);
		x = an(OIND, dupn(tp), nil);
		n = an(OASGN, x, n);
		c = an(OLIST, c, n);
		break;
	case TARRAY:
		if(t->next->type == TCHAR) {
			n = an(OLIST, dupn(dp), con(t->size));
			n = an(OLIST, dupn(tp), n);
			n = an(OCALL, dupn(movenode), n);
			c = an(OLIST, c, n);
			n = an(OADDEQ, dupn(dp), con(t->size));
			c = an(OLIST, c, n);
			break;
		}
		tmp = stknode(builtype[TINT]);
		n = an(OLT, dupn(tmp), con(t->size/t->next->size));
		n = an(OLIST, n, an(OPINC, dupn(tmp), nil));
		n = an(OLIST, an(OASGN, dupn(tmp), con(0)), n);
		tp = dupn(tp);
		tp->t = at(TIND, t->next);
		tp = an(OADD, tp, tmp);
		c = an(OLIST, c, an(OFOR, n, unpacker(nil, dp, tp, t->next)));
		break;
	case TUNION:
		c = nil;
		break;
	case TIND:
		tmp = stknode(builtype[TINT]);
		n = an(OCONV, dupn(dp), nil);
		n->t = at(TIND, builtype[TINT]);
		n = an(OIND, n, nil);
		n = an(OASGN, tmp, n);
		n = an(OEQ, n, con(0));
		x = an(OIND, dupn(tp), nil);
		x = an(OASGN, x, con(0));
		x->right->t = at(TIND, builtype[TVOID]);
		x = an(OLIST, x, an(OADDEQ, dupn(dp), con(builtype[TINT]->size)));
		n = an(OIF, n, an(OELSE, x, nil));
		v = at(TFUNC, at(TIND, builtype[TCHAR]));
		v->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		x = rtnode(unpackfn, t->next, v);
		x = an(OCALL, x, an(OLIST, dupn(dp), an(OIND, dupn(tp), nil)));
		x = an(OASGN, dupn(dp), x);
		y = an(OCALL, dupn(allocnode), con(t->next->size));
		y = an(OASGN, an(OIND, dupn(tp), nil), y);
		n->right->right = an(OLIST, y, x);
		c = an(OLIST, c, n);
		break;
	case TADT:
	case TAGGREGATE:
		t = t->next;
		while(t) {
			n = an(OCONV, tp, nil);
			n->t = at(TIND, builtype[TCHAR]);
			n = an(OADD, n, con(t->offset));
			n = an(OCONV, n, nil);
			n->t = at(TIND, t);
			c = unpacker(c, dp, n, t);
			t = t->member;
		}
		break;
	}
	return c;
}

Node*
packer(Node *c, Node *dp, Node *tp, Type *t)
{
	Type *v;
	Node *n, *x, *tmp;

	/* Pack the signature */
	if(t == nil) {
		tmp = tp;
		t = tp->t;
		goto packsig;
	}

	switch(t->type) {
	default:
		fatal("packer %T", t);
	case TPOLY:
	case TFLOAT:
		fatal("packer: not yet implemented %T\n", t);
	case TCHANNEL:
		v = at(TFUNC, at(TIND, builtype[TCHAR]));
		v->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		x = rtnode(chpack, nil, v);
		n = an(OLIST, an(OIND, dupn(tp), nil), dupn(dp));
		n = an(OCALL, x, n);
		n = an(OASGN, dupn(dp), n);
		c = an(OLIST, c, n);
		for(t = t->next; t; t = t->variant) {
			v = tmcpy(t);
			v->variant = nil;
			packmk(typesig(v), v, Needed);
		}
		break;
	case TINT:
	case TUINT:
		tmp = stknode(t);
		n = an(OASGN, tmp, an(OIND, tp, nil));
		c = an(OLIST, c, n);
	packsig:
		n = an(OIND, dupn(dp), nil);
		n = an(OASGN, n, an(ORSH, dupn(tmp), con(24)));
		c = an(OLIST, c, n);
		n = an(OADD, dupn(dp), con(1)); 
		n = an(OIND, n, nil);
		n = an(OASGN, n, an(ORSH, dupn(tmp), con(16)));
		c = an(OLIST, c, n);
		n = an(OADD, dupn(dp), con(2)); 
		n = an(OIND, n, nil);
		n = an(OASGN, n, an(ORSH, dupn(tmp), con(8)));
		c = an(OLIST, c, n);
		n = an(OADD, dupn(dp), con(3)); 
		n = an(OIND, n, nil);
		n = an(OASGN, n, dupn(tmp));
		c = an(OLIST, c, n);
		c = an(OLIST, c, an(OADDEQ, dp, con(t->size)));
		break;		
	case TSINT:
	case TSUINT:
		tmp = stknode(t);
		n = an(OASGN, tmp, an(OIND, tp, nil));
		c = an(OLIST, c, n);
		n = an(OIND, dupn(dp), nil);
		n = an(OASGN, n, an(ORSH, dupn(tmp), con(8)));
		c = an(OLIST, c, n);
		n = an(OADD, dupn(dp), con(1));
		n = an(OIND, n, nil);
		n = an(OASGN, n, dupn(tmp));
		c = an(OLIST, c, n);
		c = an(OLIST, c, an(OADDEQ, dp, con(t->size)));
		break;		
	case TCHAR:
		n = an(OPINC, dupn(dp), nil);
		n = an(OIND, n, nil);
		n = an(OASGN, n, an(OIND, tp, nil));
		c = an(OLIST, c, n);
		break;
	case TARRAY:
		if(t->next->type == TCHAR) {
			n = an(OLIST, dupn(tp), con(t->size));
			n = an(OLIST, dupn(dp), n);
			n = an(OCALL, dupn(movenode), n);
			c = an(OLIST, c, n);
			n = an(OADDEQ, dupn(dp), con(t->size));
			c = an(OLIST, c, n);
			break;
		}
		tmp = stknode(builtype[TINT]);
		n = an(OLT, dupn(tmp), con(t->size/t->next->size));
		n = an(OLIST, n, an(OPINC, dupn(tmp), nil));
		n = an(OLIST, an(OASGN, dupn(tmp), con(0)), n);
		tp = dupn(tp);
		tp->t = at(TIND, t->next);
		tp = an(OADD, tp, tmp);
		c = an(OLIST, c, an(OFOR, n, packer(nil, dp, tp, t->next)));
		break;
	case TUNION:
		diag(0, "unions cannot be packed/unpacked");
		c = nil;
		break;
	case TIND:
		n = con(0);
		n->t = at(TIND, builtype[TVOID]);
		n = an(OEQ, an(OIND, dupn(tp), nil), n);
		x = an(OCONV, dupn(dp), nil);
		x->t = at(TIND, builtype[TINT]);
		x = an(OPINC, dupn(dp), nil);
		x = an(OIND, x, nil);
		x = an(OASGN, x, con(0));
		n = an(OIF, n, an(OELSE, x, nil));
		v = at(TFUNC, at(TIND, builtype[TCHAR]));
		v->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		x = rtnode(packfn, t->next, v);
		x = an(OCALL, x, an(OLIST, dupn(dp), an(OIND, dupn(tp), nil)));
		x = an(OASGN, dupn(dp), x);
		n->right->right = x;
		c = an(OLIST, c, n);
		t = t->next;
		packmk(typesig(t), t, Needed);
		break;
	case TADT:
	case TAGGREGATE:
		t = t->next;
		while(t) {
			n = an(OCONV, tp, nil);
			n->t = at(TIND, builtype[TCHAR]);
			n = an(OADD, n, con(t->offset));
			n = an(OCONV, n, nil);
			n->t = at(TIND, t);
			c = packer(c, dp, n, t);
			t = t->member;
		}
		break;
	}
	return c;
}

void
export(Type *t)
{
	Type *ft;
	ulong sig;
	Pack *p;
	char buf[128];
	Node *si, *x, *n, *c, *dp, *tp, *arg;

	sig = typesig(t);
	for(p = packhash[sig%NPHASH]; p; p = p->hash)
		if(p->sig == sig)
			break;

	if(p != nil) {
		if(p->state == Made)
			return;
		p->state = Made;
	}
	else
		packmk(sig, t, Made);

	if(opt('S'))
		print("PACK #%.8luX %T\n", sig, t);

	dp = an(ONAME, nil, nil);
	dp->t = at(TIND, builtype[TCHAR]);
	dp->sym = enter("ALEF_dp", Tid);
	tp = an(ONAME, nil, nil);
	tp->t = at(TIND, t);
	tp->sym = enter("ALEF_tp", Tid);

	arg = an(OLIST, dupn(dp), dupn(tp));

	ft = at(TIND, builtype[TCHAR]);
	ft->class = Global;

	n = an(ONAME, nil, nil);
	sprint(buf, "%s_%.8luX", packfn, sig);
	n->sym = enter(buf, Tid);

	fundecl(ft, n, arg);
	dupok();


	derivetype(dp);
	derivetype(tp);

	c = packer(nil, dp, con(sig), nil);

	/* Lay down the type */
	c = packer(c, dp, tp, t);

	c = an(OLIST, c, an(ORET, dupn(dp), nil));

	fungen(c, n);

	/* Make the runtime links */
	si = rtnode("ALEF_SI", t, builtype[TINT]);
	dynt(rtnode("ALEF_PACK", nil, at(TIND, builtype[TFUNC])), si);
	init(rtnode("ALEF_MAP", nil, at(TARRAY, builtype[TINT])), con(sig));

	dp = an(ONAME, nil, nil);
	dp->t = at(TIND, builtype[TCHAR]);
	dp->sym = enter("ALEF_dp", Tid);
	tp = an(ONAME, nil, nil);
	tp->t = at(TIND, t);
	tp->sym = enter("ALEF_tp", Tid);

	arg = an(OLIST, dupn(dp), dupn(tp));

	ft = at(TIND, builtype[TCHAR]);
	ft->class = Global;

	n = an(ONAME, nil, nil);
	sprint(buf, "%s_%.8luX", unpackfn, sig);
	n->sym = enter(buf, Tid);

	fundecl(ft, n, arg);
	dupok();

	derivetype(dp);
	derivetype(tp);

	x = an(OADDEQ, dupn(dp), con(builtype[TINT]->size));
	x = an(OLIST, x, unpacker(nil, dp, tp, t));
	c = an(OLIST, x, an(ORET, dupn(dp), nil));

	fungen(c, n);

	dynt(rtnode("ALEF_UPACK", nil, at(TIND, builtype[TFUNC])), si);
}

void
packdepend(void)
{
	int i;
	Pack *p;

loop:
	for(i = 0; i < NPHASH; i++) {
		for(p = packhash[i]; p; p = p->hash) {
			if(p->state == Needed) {
				export(p->t);
				goto loop;
			}
		}
	}
}
