#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"
#include "y.tab.h"

char *sclass[] = 
{
	"????",
	"internal",
	"external",
	"parameter",
	"automatic",
	"argument",
	"private"
};

void
enterblock(void)
{
	Scope++;
	if(Scope >= Sdepth)
		fatal("enterblock too deep");

	block[Scope] = 0;
}

void
leaveblock(void)
{
	Tinfo *t;

	for(t = block[Scope]; t; t = t->dcllist) {
		if(opt('s'))
			print("p<%d> %s %T %s offset %d\n",
			Scope, sclass[t->class], t->t, t->s->name, t->offset); 

		if(t->ref == 0)
			warn(0, "%s %T %s declared but not used",
			sclass[t->class], t->t, t->s->name);

		t->s->instance = t->next;
	}

	Scope--;
	if(Scope < 0)
		fatal("leaveblock: Scope < 0");
}

/*
 * Allocate a typeinfo and install an instance of the variable at the current scope
 */
Tinfo*
atinfo(Node *n, Type *t)
{
	Sym *s;
	Tinfo *new, *l, *i;

	s = n->sym;
	new = malloc(sizeof(Tinfo));
	new->block = Scope;
	new->t = t;
	new->s = s;
	new->offset = 0;
	new->class = t->class;
	new->ref = 0;
	n->ti = new;

	if(n->init && Scope != 0)
		diag(n, "automatic initialialisation is illegal");

	if(t->class == Private) {
		if(n->init)
			diag(n, "private initialialisation is illegal");

		new->offset = privreg--;
		if(opt('r'))
			print("%L %T %s is in R%d\n",
				n->srcline, t, s->name, new->offset);
	}

	i = s->instance;

	if(i && i->block == Scope) {
		if(i->class == External || t->class == External)
		if(typecmp(t, i->t, 5)) {
			if(i->class == External)
				i->class = t->class;
			return i;
		}
		if(!(t->type == TFUNC && t->class == i->class))
			diag(n, "redeclaration: '%T %s' to '%T'", i->t, s->name, t);
		return new;
	}

	new->next = s->instance;
	s->instance = new;

	/* Unusually sleazy */
	new->dcllist = 0;
	if(block[Scope] == 0)
		block[Scope] = new;
	else {
		for(l = block[Scope]; l->dcllist; l = l->dcllist)
			;
		l->dcllist = new;
	}

	return new;
}

ulong
align(ulong val, Type *t)
{
	ulong size, slop;
	Type *p;

	/* look for the base type */
	for(p = t; p; p = p->next)
		if(p->align)
			break;

	if(p == 0 || t->size == 0)
		return val;

	size = p->align;
	slop = val%size;
	if(slop == 0)
		return val;

	return val + (size-slop);
}

void
carray(Type *t)
{
	Type *f;

	for(;;) {
		f = t->next;
		if(f == ZeroT)
			break;
		if(f->type != TIND && f->type != TARRAY) {
			if(t->type == TARRAY)
				t->type = TIND;
			break;
		}
		t = t->next;
	}
}

/*
 * Assign scope and supply rounded frame offsets for auto/parameter space
 */
void
scopeis(int type)
{
	Tinfo *t;

	for(t = block[Scope]; t; t = t->dcllist) {
		t->class = type;
		switch(type) {
		case Parameter:
			carray(t->t);

			params = align(params, builtype[TINT]);
			t->offset = params;
			params += t->t->size;

			/* Adjust offset within int for smaller types */
			switch(t->t->type) {
			default:
				break;

			case TSINT:
			case TSUINT:
				t->offset += Shortfoff;
				break;

			case TCHAR:
				t->offset += Charfoff;
				break;
			}
			break;

		case Automatic:
			frame = align(frame, t->t);
			frame += t->t->size;
			t->offset = frame;
			break;
		}
	}
}

/*
 * Push declarations onto the scope chains
 */
void
pushdcl(Node *n)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		pushdcl(n->left);
		pushdcl(n->right);
		break;

	case OFUNC:
	case OAGDECL:
	case OUNDECL:
	case OADTDECL:
	case OSETDECL:
		break;

	case ONAME:
		atinfo(n, n->t);
		if(opt('s'))
			print("d<%d> %s %T %s \n",
			Scope, sclass[n->ti->class], n->t, n->sym->name);
		break;
	}
}

void
chani(Type *t, Node *n)
{
	typechk(n, 0);
	rewrite(n);

	if(n->type != OCONST || n->ival <= 0)
		diag(n, "channel buffer size must be positive constant");

	t->nbuf = n->ival;
}

int
argcmp(Node *proto, Node *call)
{
	if(proto == ZeroN && call == ZeroN)
		return 0;

	if(proto && call == ZeroN)
		return 1;

	if(proto == ZeroN && call)
		return 1;

	switch(proto->type) {
	case OLIST:
		if(call->type != OLIST)
			return 1;
		if(argcmp(proto->left, call->left))
			return 1;
		if(argcmp(proto->right, call->right))
			return 1;
		break;

	case OVARARG:
		if(call->type == OVARARG)
			break;
		return 1;

	case ONAME:
	case OPROTO:
	case OADTARG:
		if(typecmp(proto->t, call->t, 5) == 0)
			return 1;
	}
	return 0;
}

/*
 * Compare function prototypes
 */
int
protocmp(Node *a, Node *b)
{
	if(typecmp(a->t, b->t, 5) == 0)
		return 1;

	if(a->left == ZeroN && b->left == ZeroN)
		return 0;

	return argcmp(a->left, b->left);
}

Node*
simpledecl(Type *type, Node *vars)
{
	/* Declaration of type in member list only */
	if(vars == 0) {
		vars = an(OTYPE, ZeroN, ZeroN);
		vars->t = type;
		return vars;
	}

	/* Fold array expressions and tag var decls by type */
	rewrite(vars);
	applytype(type, vars);

	return vars;
}

/*
 * Function declaration
 */
void
fundecl(Type *basic, Node *name, Node *args)
{
	Sym *s;
	Type *t, *instance;
	Node *n, *proto;

	if(Scope != 0 || name->type != ONAME) {
		diag(name, "illegal function declaration");
		return;
	}

	for(n = name->right; n; n = n->left)
		basic = at(TIND, basic);

	name->right = ZeroN;
	s = name->sym;

	proto = an(0, ZeroN, ZeroN);
	*proto = *name;
	proto->left = args;
	proto->right = ZeroN;
	proto->t = at(TFUNC, basic);

	curfunc = proto;		/* For return typechecking */
	
	/* Check for prototypes */
	if(s->instance) {
		instance = s->instance->t;
		if(instance->type != TFUNC)
			diag(name, "%s redeclared as function: %P", s->name, proto);
		else
		if(protocmp(proto, instance->proto))
			diag(name, "function redeclaration: '%P' to '%P'",
							proto, instance->proto);
	}

	if(name->left) {
		diag(name, "illegal function declaration");
		return;
	}

	frame =	Autobase;
	params = Parambase;
	stmp = 0;
	maxframe = 0;

	n = an(OFUNC, args, ZeroN);
	n->sym = name->sym;
	n->t = basic;
	name->left = n;

	t = at(TFUNC, basic);
	t->class = basic->class;
	if(t->class == 0)
		fatal("fundecl %d %P\n", t->class, proto);

	t->proto = proto;

	atinfo(n, t);
	proto->t = t;

	/* push arguments */
	enterblock();
	pushdcl(args);
	scopeis(Parameter);
	preamble(n);
}

Node*
adtfunc(Tysym t, Node *name)
{
	Sym *s;
	Type *p;
	char *n;
	int junk;
	char buf[Strsize];

	if(t.t->type != TADT) {
		diag(name, "member function must be part of an adt %T", t);
		return ZeroN;
	}
	adtbfun = t.t;

	n = name->sym->name;

	p = walktype(t.t, name, &junk);
	if(p == ZeroT) {
		diag(name, "%s not a member of adt %s", n, t.s->name);
		return ZeroN;
	}
	if(p->type != TFUNC)
		diag(name, "%s not a function member %T", n, p);

	sprint(buf, "%s_%s", t.s->name, n);
	s = lookup(buf);
	if(s == ZeroS)
		s = enter(buf, Tid);

	name->sym = s;
	return p->proto;
}

void
adtchk(Node *adtp, Node *fun)
{
	Node *p;

	if(adtp == ZeroN)
		return;

	p = fun->sym->instance->t->proto;
	if(protocmp(adtp, p))
		diag(fun, "prototype mismatch %P and %P", adtp, p);	
}

/*
 * build a pseudo symbol entry for ... in varargs
 */
Node*
vargptr(void)
{
	Node *n;
	Tinfo *t;

	n = an(ONAME, ZeroN, ZeroN);
	t = malloc(sizeof(Tinfo));
	n->sym = malloc(sizeof(Sym));

	n->ti = t;
	n->t = builtype[TVOID];
	n->sym->name = "...";

	t->class = Parameter;
	params = align(params, builtype[TINT]);
	t->offset = params;

	n = an(OADDR, n, ZeroN);

	return n;
}

/*
/* Function prototype
 */
void
funproto(Node *name, Node *args)
{
	Type *t, *ti;

	name->left = args;

	t = at(TFUNC, name->t);
	t->proto = name;
	name->t = t;

	if(buildadt)
		return;

	if(name->sym->instance) {
		ti = name->sym->instance->t;
		if(ti->type != TFUNC)
			diag(name, "redeclared as prototype %P", name);
		else
		if(protocmp(name, t->proto))
			diag(name, "prototype mismatch: '%P' to '%P'", name, t->proto);
	}
	atinfo(name, t);
}

/*
 * Return node for a string constant
 */
Node*
strnode(String *s)
{
	Node *n;
	static strno;
	char buf[Strsize];

	sprint(buf, ".s%d", strno++);

	/* Make a node */
	n = an(ONAME, ZeroN, ZeroN);
	n->t = at(TARRAY, builtype[TCHAR]);
	n->t->size = s->len;
	n->t->class = Internal;

	n->sym = enter(buf, 0);
	n->ti = malloc(sizeof(Tinfo));
	n->ti->offset = 0;
	n->ti->class = Internal;

	s->next = strdat;
	s->n = *n;
	strdat = s;

	return n;
}

void
coverset(Node *n)
{
	if(n->left) {
		typechk(n->left, 0);
		rewrite(n->left);

		if(n->left->type != OCONST)
			diag(n, "value must be constant");
		else
			setbase = n->left->ival;
	}
	n->sym->sval = setbase++;
}

void
newtype(Type *tspec, Node *name)
{
	Sym *sym;

	sym = name->sym;

	if(sym->ltype)
	if(sym->ltype->type != TXXX) {
		diag(name, "%s redclared type specifier", sym->name);
		return;
	}

	if(tspec == ZeroT) {
		tspec = at(TXXX, 0);
		tspec->sym = sym;
	}

	applytype(tspec, name);

	sym->lexval = Ttypename;
	sym->ltype = name->t;
}
