#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"

typedef struct Call Call;
struct Call
{
	int	veccnt;
	int 	argc;
	int 	ok;
	int	mismatch;
	Node**	argv;
	Type*	t;
};

Call cc;

void
treeflat(Node *n, Node **vec)
{
	if(n == nil)
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

Node*
rtnode(char *name, Type *t, Type *vt)
{
	Node *s;
	char buf[Strsize];

	s = an(ONAME, nil, nil);
	s->sym = malloc(sizeof(Sym));
	if(t != nil) {
		sprint(buf, "%s_%.8luX", name, typesig(t));
		s->sym->name = strdup(buf);
	}
	else
		s->sym->name = strdup(name);

	s->islval = 1;
	s->t = vt;
	s->ti = ati(s->t, Global);
	s->ti->s = s->sym;
	s->sym->instance = s->ti;

	return s;
}

int
polyarg(Type *pt, Node *arg, Type *poly)
{
	Type *f;
	int oline;
	Node *tmp, *n, *n1, *sn;

	if(opt('b'))
		print("polyarg: %T %N %T\n", pt, arg, poly);

	oline = line;
	line = arg->srcline;

	f = boundtype(poly, pt);
	tmp = stknode(at(TIND, f));

	sn = rtnode("ALEF_SI", f, builtype[TINT]);
	mkdynt(sn, f);
	sn = an(OADDR, sn, nil);
	sn = an(OCONV, sn, nil);
	sn->t = builtype[TINT];

	n1 = an(OCALL, dupn(allocnode), con(f->size));
	n1 = an(OASGN, dupn(tmp), n1);
	n = an(OIND, dupn(tmp), nil);
	n = an(OASGN, n, dupn(arg));
	n1 = an(OLIST, n1, n);
	n1 = an(OBLOCK, n1, dupn(tmp));
	n1->t = tmp->t;

	n1 = an(OLIST, n1, sn);
	n1 = an(OILIST, n1, nil);
	n1->t = polyshape;

	line = oline;

	if(typechk(n1, 0))
		return 1;

	if(opt('b'))
		ptree(n1, 0);

	*arg = *n1;
	return 0;
}

int
tyarg(Node *proto, Type *poly)
{
	Type *t, *f;
	Node *new, *arg;

	if(cc.ok || proto == nil)
		return 0;

	if(cc.veccnt == 0) {
		if(proto->t && proto->t->type == TVOID)
			return 0;
		return 1;
	}

	if(proto->type == OVARARG) {
		cc.ok = 1;
		return 0;
	}

	if(cc.argc >= cc.veccnt)
		return 1;

	if(proto->type == OLIST) {
		if(tyarg(proto->left, poly))
			return 1;
		if(tyarg(proto->right, poly))
			return 1;
		return 0;
	}

	arg = cc.argv[cc.argc];
	cc.t = arg->t;
	t = proto->t;

	if(t->type == TPOLY && poly) {
		if(arg->t->type == TPOLY) {
			for(f = poly->param; f; f = f->variant) {
				if(f->sym == arg->t->sym) {
					cc.mismatch = cc.argc;
					cc.t = f;
					return 1;
				}
			}
		}
		else
		if(polyarg(t, arg, poly)) {
			cc.mismatch = cc.argc;
			return 1;
		}
		cc.argc++;
		return 0;	
	}

	if(typeval(typeasgn, t, arg->t)) {
		if(promote(arg, proto)) {
			cc.mismatch = cc.argc;
			return 1;
		}
	}

	if(t->type == TADT && vcmp(t->param, arg->t->param) == 0) {
		cc.mismatch = cc.argc;
		return 1;
	}

	if(typecmp(t, arg->t, 5) == 0) {
		new = dupn(arg);
		arg->t = t;
		arg->type = OCONV;
		arg->left = new;
		arg->right = nil;
	}
	cc.argc++;
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

	switch(template->t->type) {
	case TADT:
	case TIND:
	case TUNION:
	case TAGGREGATE:
		break;
	default:
		return 1;
	}

	switch(base->t->type) {
	case TADT:
	case TUNION:
	case TAGGREGATE:
		n.sym = ltytosym(template->t);
		n.srcline = base->srcline;
		if(n.sym && walktype(base->t, &n, &off)) {
			new = dupn(base);
			base->sym = n.sym;
			base->type = ODOT;
			base->right = nil;
			base->left = new;
			base->t = nil;
			return typechk(base,  0);
		}
		break;

	case TIND:
		if(template->t->type != TIND)
			break;

		t = base->t->next;
		if(((1<<t->type) & MCOMPLEX) == 0)
			break;

		n.sym = ltytosym(template->t->next);
		n.srcline = base->srcline;
		off = 0;
		if(n.sym && walktype(t, &n, &off)) {
			new = dupn(base);
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
	int i;
	Call oc;
	Type *pt;
	Node *f, *l, *proto, *c;

	l = n->left;
	if(l->t->type != TFUNC) {
		diag(n, "call is not to a function %T", l->t);
		return 1;
	}

	proto = l->t->proto;
	if(proto == nil) {
		diag(n, "no prototype for function");
		return 1;
	}

	oc = cc;

	/* Count */
	veccnt = 0;
	treeflat(n->right, 0);
	cc.argv = malloc(sizeof(Node*)*veccnt);

	/* Save */
	veccnt = 0;
	treeflat(n->right, cc.argv);

	cc.veccnt = veccnt;
	cc.argc = 0;
	cc.ok = 0;
	cc.mismatch = -1;

	if(opt('y')) {
		print("prototype: %P\n", proto);
		for(i = 0; i < cc.veccnt; i++) {
			print("ARG%d\n", i);
			ptree(cc.argv[i], 0);
		}
	}

	if(n->poly != nil)
		pt = n->poly->t;
	else
		pt = nil;

	if(tyarg(proto->left, pt)) {
		if(cc.mismatch < 0)
			goto bad;
		diag(n, "prototype mismatch: %P at arg %d of type %T",
			proto, cc.mismatch+1, cc.t);
		cc = oc;
		return 1;
	}

	if(cc.ok == 0 && cc.argc != cc.veccnt)
		goto bad;

	for(i = 0; i < cc.veccnt; i++) {
		c = cc.argv[i];
		if(c->type == OBLOCK)
			c = c->right;

		switch(c->t->type) {
		case TVOID:
			diag(c, "void function passed as '...' arg %d", i);
			break;

		case TPOLY:
			c->t = polyshape;
			break;

		case TSINT:
			f = dupn(c);
			c->left = f;
			c->right = nil;
			c->type = OCONV;
			c->t = builtype[TINT];
			break;

		case TSUINT:
		case TCHAR:
			f = dupn(c);
			c->left = f;
			c->right = nil;
			c->type = OCONV;
			c->t = builtype[TUINT];
			break;
		}
	}
	cc = oc;
	return 0;
bad:
	if(opt('y'))
		print("argc=%d veccnt=%d\n", cc.argc, cc.veccnt);

	diag(n, "bad argument count: %P", proto);
	cc = oc;
	return 1;
}
