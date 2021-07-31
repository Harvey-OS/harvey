#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern
#include "globl.h"

static int 	argc;
static int 	ok;
static int	mismatch;
static Node	**argv;

void
treeflat(Node *n, Node **vec)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		if(vec)
			vec[veccnt] = n;
		veccnt++;	
		break;

	case OBREAK:
		break;

	case OLIST:
		treeflat(n->left, vec);
		treeflat(n->right, vec);
		break;
	}
}

int
tyarg(Node *proto)
{
	Node *new, *arg;

	if(ok || proto == ZeroN)
		return 0;

	if(veccnt == 0) {
		if(proto->t->type == TVOID)
			return 0;
		return 1;
	}

	if(proto->type == OVARARG) {
		ok = 1;
		return 0;
	}

	if(argc >= veccnt)
		return 1;

	switch(proto->type) {
	case OLIST:
		if(tyarg(proto->left))
			return 1;
		if(tyarg(proto->right))
			return 1;
		break;

	default:
		arg = argv[argc];
		if(typeval(typeasgn, proto->t, arg->t)) {
			if(promote(arg, proto)) {
				mismatch = argc;
				return 1;
			}
		}

		if(typecmp(proto->t, arg->t, 5) == 0) {
			new = an(0, ZeroN, ZeroN);
			*new = *arg;

			arg->t = proto->t;
			arg->type = OCONV;
			arg->left = new;
			arg->right = ZeroN;
		}
		argc++;
		break;
	}
	return 0;
}

/*
 * Try to promote the base complex type into the template type.
 */

int
promote(Node *base, Node *template)
{
	int off;
	Type *t;
	Node *new, n;

	switch(base->t->type) {
	case TADT:
	case TUNION:
	case TAGGREGATE:
		n.sym = ltytosym(template->t);
		n.srcline = base->srcline;
		if(n.sym && walktype(base->t, &n, &off)) {
			new = an(0, ZeroN, ZeroN);
			*new = *base;
			base->sym = n.sym;
			base->type = ODOT;
			base->right = ZeroN;
			base->left = new;
			base->t = ZeroT;
			return typechk(base,  0);
		}
		break;

	case TIND:
		if(template->t->type != TIND)
			break;

		t = base->t->next;
		if(t->type != TAGGREGATE && t->type != TADT && t->type != TUNION)
			break;

		n.sym = ltytosym(template->t->next);
		n.srcline = base->srcline;
		off = 0;
		if(n.sym && walktype(t, &n, &off)) {
			new = an(0, ZeroN, ZeroN);
			*new = *base;
			base->type = OADD;
			base->right = con(off);
			base->left = new;
			return typechk(base,  0);
		}
		break;
	}
	return 1;
}

int
tycall(Node *n)
{
	Node *f, *l, *proto, *c;
	int i;

	l = n->left;

	if(l->t->type != TFUNC) {
		diag(n, "call is not to a function %T", l->t);
		return 1;
	}

	proto = l->t->proto;
	if(proto == ZeroN) {
		diag(n, "no prototype for function");
		return 1;
	}

	/* Count */
	veccnt = 0;
	treeflat(n->right, 0);
	argv = malloc(sizeof(Node*)*veccnt);

	/* Save */
	veccnt = 0;
	treeflat(n->right, argv);

	argc = 0;
	ok = 0;
	mismatch = -1;

	if(opt('y'))
		print("PROTO %P\n", proto);

	if(tyarg(proto->left)) {
		if(mismatch >= 0) {
			diag(n, "prototype mismatch: %P at arg %d of type %T",
					proto, mismatch+1, argv[mismatch]->t);
			return 1;
		}
		goto bad;
	}

	if(ok == 0)
	if(argc != veccnt)
		goto bad;

	for(i = 0; i < veccnt; i++) {
		c = argv[i];
		switch(c->t->type) {
		case TSINT:
			f = an(0, ZeroN, ZeroN);
			*f = *c;
			c->left = f;
			c->right = ZeroN;
			c->type = OCONV;
			c->t = builtype[TINT];
			break;

		case TSUINT:
		case TCHAR:
			f = an(0, ZeroN, ZeroN);
			*f = *c;
			c->left = f;
			c->right = ZeroN;
			c->type = OCONV;
			c->t = builtype[TUINT];
			break;
		}
	}
	return 0;
bad:
	diag(n, "bad argument count: %P", proto);
	return 1;
}
