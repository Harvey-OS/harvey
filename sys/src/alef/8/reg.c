#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

int regmap[D_CS];

void
reginit(void)
{
	privreg = Pregs;
	regmap[D_SP]++;

	F0 = regn(D_F0);
	F1 = regn(D_F0+1);
}

void
regcheck(void)
{
	int r, e;

	e = 0;
	for(r = Ireg; r < Maxireg; r++)
		if(regmap[r] && r != D_SP) {
			print("%R still used\n", r);
			e++;
		}

	for(r = Freg; r < Maxfreg; r++)
		if(regmap[r] && (r != Retfreg || regmap[Retfreg] != 1)) {
			print("%R still used\n", r);
			e++;
		}
	if(e)
		fatal("regcheck %P", curfunc);
}

void
reg(Node *n, Type *t, Node *use)
{
	int r;

	switch(t->type) {
	default:
		fatal("reg: bad type %T", t);

	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TIND:
	case TCHANNEL:
		if(use && use->type == OREGISTER && use->reg < Freg) {
			r = use->reg;
			break;
		}

		for(r = Ireg; r <= Maxireg; r++) {
			if(regmap[r] == 0)
				break;
		}
		if(r > Maxireg)
			fatal("No int registers");
		break;

	case TFLOAT:
		r = D_F0;
		break;
	}
	regmap[r]++;
	n->reg = r;
	n->type = OREGISTER;
	n->islval = 11;
	n->sun = 0;
	n->left = 0;
	n->right = 0;
	n->t = t;
}

int
needreg(int nr, Node *r, Node *use)
{
	r->reg = nr;
	r->type = OREGISTER;
	r->islval = 11;
	r->left = 0;
	r->right = 0;
	r->sun = 0;
	r->t = builtype[TINT];
	/* Ok if we were given it */
	if(use && use->type == OREGISTER && use->reg == nr)
		return 0;

	if(regmap[nr])
		return 1;

	return 0;
}

Node*
regtmp(void)
{
	Node *n;
	int r;

	n = an(OREGISTER, ZeroN, ZeroN);
	n->t = builtype[TINT];
	n->islval = 11;
	n->sun = 0;

	for(r = Ireg; r < Maxireg; r++) {
		if(regmap[r] == 0) {
			n->reg = r;
			regmap[r]++;
			return n;
		}
	}
	fatal("No int registers");
	return ZeroN;	
}

void
regrest(int nr, int ref)
{
	if(ref == Rinc)
		regmap[nr]++;
	else
		regmap[nr] = ref;
}

int
regsave(int nr)
{
	int ref;

	ref = regmap[nr];
	regmap[nr] = 0;
	
	return ref;
}

Node*
regn(int nr)
{
	Node *n;

	n = an(OREGISTER, ZeroN, ZeroN);
	if(nr >= Freg)
		n->t = builtype[TFLOAT];
	else
		n->t = builtype[TINT];
	n->islval = 11;
	n->sun = 0;
	n->reg = nr;
	return n;
}

void
regret(Node *n, Type*t)
{
	int r;

	switch(t->type) {
	default:
		fatal("regret: bad type %T", t);

	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TIND:
	case TCHANNEL:
		r = Retireg;
		break;

	case TFLOAT:
		r = Retfreg;
		break;
	}
	regmap[r]++;
	n->reg = r;
	n->type = OREGISTER;
	n->t = t;
}

void
regfree(Node *n)
{
	if(regmap[n->reg] <= 0)
		fatal("regfree");

	regmap[n->reg]--;
}

/*
 * look for a constant which fits in a 386 immediate
 */
int
immed(Node *n)
{
	long v;

	if(n->type == OCONST && n->t->type != TFLOAT) {
		v = n->ival;
		if(v >= -32766L && v < 32766L)
			return 1;
	}
	return 0;
}

/*
 * Return the name of an activation argument
 */
Node*
atvnode(Type *t)
{
	int o;
	Node *n;

	n = an(OREGISTER, ZeroN, ZeroN);
	n->reg = ratv.reg;
	n->ival = 0;
	n->t = at(TIND, t);

	o = align(ratv.ival, builtype[TINT]);
	ratv.ival = o+t->size;
	if(ratv.ival > maxframe)
		maxframe = ratv.ival;

	/* Adjust offset within int for smaller types */
	switch(t->type) {
	case TSINT:
	case TSUINT:
		o += Shortfoff;
		break;

	case TCHAR:
		o += Charfoff;
		break;
	}

	n = an(OADD, n, con(o));
	n->t = n->left->t;
	n = an(OIND, n, ZeroN);
	n->t = t;

	sucalc(n);
	return n;
}

/*
 * Make a node to alias an argument
 */
Node*
argnode(Type *t)
{
	Node *n;
	int o;

	if(atv)
		return atvnode(t);

	n = an(ONAME, ZeroN, ZeroN);
	n->t = t;
	n->ti = ati(t, Argument);
	args = align(args, builtype[TINT]);
	o = args;

	/* Adjust offset within int for smaller types */
	switch(t->type) {
	case TSINT:
	case TSUINT:
		o += Shortfoff;
		break;

	case TCHAR:
		o += Charfoff;
		break;
	}

	n->ti->offset = o;
	args += t->size;

	sucalc(n);
	return n;
}

Node*
paramnode(Type *t)
{
	Node *n;

	USED(t);

	while(tip->class != Parameter)
		tip = tip->dcllist;

	n = an(ONAME, ZeroN, ZeroN);
	n->sym = tip->s;
	n->ti = tip;
	n->t = tip->t;

	/* for next time */
	tip = tip->dcllist;

	sucalc(n);
	return n;
}

/*
 * Make a stack temporary node and allocate space in the frame
 */
Node*
stknode(Type *o)
{
	Node *n;
	char buf[10];

	n = an(ONAME, ZeroN, ZeroN);
	n->sym = malloc(sizeof(Sym));
	n->ti = ati(o, Automatic);
	n->t = o;

	sprint(buf, ".t%d", stmp++);
	n->sym->name = strdup(buf);

	/* Allocate the space */
	frame = align(frame, o);
	frame += o->size;

	n->ti->offset = -frame;
	sucalc(n);
	return n;
}

Node*
internnode(Type *o)
{
	Node *n;
	char buf[10];

	n = an(ONAME, ZeroN, ZeroN);
	n->sym = malloc(sizeof(Sym));
	n->t = at(o->type, 0);
	n->t->class = Internal;
	n->ti = ati(n->t, Internal);

	sprint(buf, ".i%d", stmp++);
	n->sym->name = strdup(buf);

	n->ti->offset = 0;
	sucalc(n);

	n->init = ZeroN;
	gendata(n);

	return n;
}

Node*
con(int i)
{
	Node *c;

	c = an(OCONST, ZeroN, ZeroN);
	c->t = builtype[TINT];
	c->ival = i;

	return c;
}

Node*
conf(double d)
{
	Node *c;

	c = an(OCONST, ZeroN, ZeroN);
	c->t = builtype[TFLOAT];
	c->fval = d;

	return c;
}

Node*
rpush(int rn)
{
	Node *n;

	if(atv) {
		n = stknode(builtype[TINT]);
		assign(regn(rn), n);
		return n;	
	}
	instruction(APUSHL, regn(rn), ZeroN);
	return ZeroN;
}

void
rpop(Node *r, Node *n)
{
	if(n)
		assign(n, r);
	else
		instruction(APOPL, ZeroN, r);
}
