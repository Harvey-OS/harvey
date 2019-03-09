/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

static int fsize[] =
{
	['A'] = 4,
	['B'] = 4,
	['C'] = 1,
	['D'] = 4,
	['F'] = 8,
	['G'] = 8,
	['O'] = 4,
	['Q'] = 4,
	['R'] = 4,
	['S'] = 4,
	['U'] = 4,
	['V'] = 8,
	['W'] = 8,
	['X'] = 4,
	['Y'] = 8,
	['Z'] = 8,
	['a'] = 4,
	['b'] = 1,
	['c'] = 1,
	['d'] = 2,
	['f'] = 4,
	['g'] = 4,
	['o'] = 2,
	['q'] = 2,
	['r'] = 2,
	['s'] = 4,
	['u'] = 2,
	['x'] = 2,
	['3'] = 10,
	['8'] = 10,
};

int
fmtsize(Value *v)
{
	int ret;

	switch(v->store.fmt) {
	default:
		return  fsize[v->store.fmt];
	case 'i':
	case 'I':
		if(v->type != TINT || machdata == 0)
			error("no size for i fmt pointer ++/--");
		ret = (*machdata->instsize)(cormap, v->store.ival);
		if(ret < 0) {
			ret = (*machdata->instsize)(symmap, v->store.ival);
			if(ret < 0)
				error("%r");
		}
		return ret;
	}
}

void
chklval(Node *lp)
{
	if(lp->op != ONAME)
		error("need l-value");
}

void
olist(Node *n, Node *res)
{
	expr(n->left, res);
	expr(n->right, res);
}

void
oeval(Node *n, Node *res)
{
	expr(n->left, res);
	if(res->type != TCODE)
		error("bad type for eval");
	expr(res->store.cc, res);
}

void
ocast(Node *n, Node *res)
{
	if(n->sym->lt == 0)
		error("%s is not a complex type", n->sym->name);

	expr(n->left, res);
	res->store.comt = n->sym->lt;
	res->store.fmt = 'a';
}

void
oindm(Node *n, Node *res)
{
	Map *m;
	Node l;

	m = cormap;
	if(m == 0)
		m = symmap;
	expr(n->left, &l);
	if(l.type != TINT)
		error("bad type for *");
	if(m == 0)
		error("no map for *");
	indir(m, l.store.ival, l.store.fmt, res);
	res->store.comt = l.store.comt;
}

void
oindc(Node *n, Node *res)
{
	Map *m;
	Node l;

	m = symmap;
	if(m == 0)
		m = cormap;
	expr(n->left, &l);
	if(l.type != TINT)
		error("bad type for @");
	if(m == 0)
		error("no map for @");
	indir(m, l.store.ival, l.store.fmt, res);
	res->store.comt = l.store.comt;
}

void
oframe(Node *n, Node *res)
{
	char *p;
	Node *lp;
	uint64_t ival;
	Frtype *f;

	p = n->sym->name;
	while(*p && *p == '$')
		p++;
	lp = n->left;
	if(localaddr(cormap, p, lp->sym->name, &ival, rget) < 0)
		error("colon: %r");

	res->store.ival = ival;
	res->op = OCONST;
	res->store.fmt = 'X';
	res->type = TINT;

	/* Try and set comt */
	for(f = n->sym->local; f; f = f->next) {
		if(f->var == lp->sym) {
			res->store.comt = f->type;
			res->store.fmt = 'a';
			break;
		}
	}
}

void
oindex(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);

	if(r.type != TINT)
		error("bad type for []");

	switch(l.type) {
	default:
		error("lhs[] has bad type");
	case TINT:
		indir(cormap, l.store.ival+(r.store.ival*fsize[l.store.fmt]), l.store.fmt, res);
		res->store.comt = l.store.comt;
		res->store.fmt = l.store.fmt;
		break;
	case TLIST:
		nthelem(l.store.l, r.store.ival, res);
		break;
	case TSTRING:
		res->store.ival = 0;
		if(r.store.ival >= 0 && r.store.ival < l.store.string->len) {
			int xx8;	/* to get around bug in vc */
			xx8 = r.store.ival;
			res->store.ival = l.store.string->string[xx8];
		}
		res->op = OCONST;
		res->type = TINT;
		res->store.fmt = 'c';
		break;
	}
}

void
oappend(Node *n, Node *res)
{
	Value *v;
	Node r, l;
	int  empty;

	expr(n->left, &l);
	expr(n->right, &r);
	if(l.type != TLIST)
		error("must append to list");
	empty = (l.store.l == nil && (n->left->op == ONAME));
	append(res, &l, &r);
	if(empty) {
		v = n->left->sym->v;
		v->type = res->type;
		v->store = res->store;
		v->store.comt = res->store.comt;
	}
}

void
odelete(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	if(l.type != TLIST)
		error("must delete from list");
	if(r.type != TINT)
		error("delete index must be integer");

	delete(l.store.l, r.store.ival, res);
}

void
ohead(Node *n, Node *res)
{
	Node l;

	expr(n->left, &l);
	if(l.type != TLIST)
		error("head needs list");
	res->op = OCONST;
	if(l.store.l) {
		res->type = l.store.l->type;
		res->store = l.store.l->store;
	}
	else {
		res->type = TLIST;
		res->store.l = 0;
	}
}

void
otail(Node *n, Node *res)
{
	Node l;

	expr(n->left, &l);
	if(l.type != TLIST)
		error("tail needs list");
	res->op = OCONST;
	res->type = TLIST;
	if(l.store.l)
		res->store.l = l.store.l->next;
	else
		res->store.l = 0;
}

void
oconst(Node *n, Node *res)
{
	res->op = OCONST;
	res->type = n->type;
	res->store = n->store;
	res->store.comt = n->store.comt;
}

void
oname(Node *n, Node *res)
{
	Value *v;

	v = n->sym->v;
	if(v->set == 0)
		error("%s used but not set", n->sym->name);
	res->op = OCONST;
	res->type = v->type;
	res->store = v->store;
	res->store.comt = v->store.comt;
}

void
octruct(Node *n, Node *res)
{
	res->op = OCONST;
	res->type = TLIST;
	res->store.l = construct(n->left);
}

void
oasgn(Node *n, Node *res)
{
	Node *lp, r;
	Value *v;

	lp = n->left;
	switch(lp->op) {
	case OINDM:
		windir(cormap, lp->left, n->right, res);
		break;
	case OINDC:
		windir(symmap, lp->left, n->right, res);
		break;
	default:
		chklval(lp);
		v = lp->sym->v;
		expr(n->right, &r);
		v->set = 1;
		v->type = r.type;
		v->store = r.store;
		res->op = OCONST;
		res->type = v->type;
		res->store = v->store;
		res->store.comt = v->store.comt;
	}
}

void
oadd(Node *n, Node *res)
{
	Node l, r;

	if(n->right == nil){		/* unary + */
		expr(n->left, res);
		return;
	}
	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TFLOAT;
	switch(l.type) {
	default:
		error("bad lhs type +");
	case TINT:
		switch(r.type) {
		case TINT:
			res->type = TINT;
			res->store.ival = l.store.ival+r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.ival+r.store.fval;
			break;
		default:
			error("bad rhs type +");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.fval = l.store.fval+r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.fval+r.store.fval;
			break;
		default:
			error("bad rhs type +");
		}
		break;
	case TSTRING:
		if(r.type == TSTRING) {
			res->type = TSTRING;
			res->store.fmt = 's';
			res->store.string = stradd(l.store.string, r.store.string);
			break;
		}
		if(r.type == TINT) {
			res->type = TSTRING;
			res->store.fmt = 's';
			res->store.string = straddrune(l.store.string, r.store.ival);
			break;
		}
		error("bad rhs for +");
	case TLIST:
		res->type = TLIST;
		switch(r.type) {
		case TLIST:
			res->store.l = addlist(l.store.l, r.store.l);
			break;
		default:
			r.left = 0;
			r.right = 0;
			res->store.l = addlist(l.store.l, construct(&r));
			break;
		}
	}
}

void
osub(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TFLOAT;
	switch(l.type) {
	default:
		error("bad lhs type -");
	case TINT:
		switch(r.type) {
		case TINT:
			res->type = TINT;
			res->store.ival = l.store.ival-r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.ival-r.store.fval;
			break;
		default:
			error("bad rhs type -");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.fval = l.store.fval-r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.fval-r.store.fval;
			break;
		default:
			error("bad rhs type -");
		}
		break;
	}
}

void
omul(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TFLOAT;
	switch(l.type) {
	default:
		error("bad lhs type *");
	case TINT:
		switch(r.type) {
		case TINT:
			res->type = TINT;
			res->store.ival = l.store.ival*r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.ival*r.store.fval;
			break;
		default:
			error("bad rhs type *");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.fval = l.store.fval*r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.fval*r.store.fval;
			break;
		default:
			error("bad rhs type *");
		}
		break;
	}
}

void
odiv(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TFLOAT;
	switch(l.type) {
	default:
		error("bad lhs type /");
	case TINT:
		switch(r.type) {
		case TINT:
			res->type = TINT;
			if(r.store.ival == 0)
				error("zero divide");
			res->store.ival = l.store.ival/r.store.ival;
			break;
		case TFLOAT:
			if(r.store.fval == 0)
				error("zero divide");
			res->store.fval = l.store.ival/r.store.fval;
			break;
		default:
			error("bad rhs type /");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.fval = l.store.fval/r.store.ival;
			break;
		case TFLOAT:
			res->store.fval = l.store.fval/r.store.fval;
			break;
		default:
			error("bad rhs type /");
		}
		break;
	}
}

void
omod(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type %");
	res->store.ival = l.store.ival%r.store.ival;
}

void
olsh(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type <<");
	res->store.ival = l.store.ival<<r.store.ival;
}

void
orsh(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type >>");
	res->store.ival = (uint64_t)l.store.ival>>r.store.ival;
}

void
olt(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);

	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	switch(l.type) {
	default:
		error("bad lhs type <");
	case TINT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.ival < r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.ival < r.store.fval;
			break;
		default:
			error("bad rhs type <");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.fval < r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.fval < r.store.fval;
			break;
		default:
			error("bad rhs type <");
		}
		break;
	}
}

void
ogt(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = 'D';
	res->op = OCONST;
	res->type = TINT;
	switch(l.type) {
	default:
		error("bad lhs type >");
	case TINT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.ival > r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.ival > r.store.fval;
			break;
		default:
			error("bad rhs type >");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.fval > r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.fval > r.store.fval;
			break;
		default:
			error("bad rhs type >");
		}
		break;
	}
}

void
oleq(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = 'D';
	res->op = OCONST;
	res->type = TINT;
	switch(l.type) {
	default:
		error("bad expr type <=");
	case TINT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.ival <= r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.ival <= r.store.fval;
			break;
		default:
			error("bad expr type <=");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.fval <= r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.fval <= r.store.fval;
			break;
		default:
			error("bad expr type <=");
		}
		break;
	}
}

void
ogeq(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = 'D';
	res->op = OCONST;
	res->type = TINT;
	switch(l.type) {
	default:
		error("bad lhs type >=");
	case TINT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.ival >= r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.ival >= r.store.fval;
			break;
		default:
			error("bad rhs type >=");
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.fval >= r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.fval >= r.store.fval;
			break;
		default:
			error("bad rhs type >=");
		}
		break;
	}
}

void
oeq(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = 'D';
	res->op = OCONST;
	res->type = TINT;
	res->store.ival = 0;
	switch(l.type) {
	default:
		break;
	case TINT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.ival == r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.ival == r.store.fval;
			break;
		default:
			break;
		}
		break;
	case TFLOAT:
		switch(r.type) {
		case TINT:
			res->store.ival = l.store.fval == r.store.ival;
			break;
		case TFLOAT:
			res->store.ival = l.store.fval == r.store.fval;
			break;
		default:
			break;
		}
		break;
	case TSTRING:
		if(r.type == TSTRING) {
			res->store.ival = scmp(r.store.string, l.store.string);
			break;
		}
		break;
	case TLIST:
		if(r.type == TLIST) {
			res->store.ival = listcmp(l.store.l, r.store.l);
			break;
		}
		break;
	}
	if(n->op == ONEQ)
		res->store.ival = !res->store.ival;
}


void
oland(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type &");
	res->store.ival = l.store.ival&r.store.ival;
}

void
oxor(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type ^");
	res->store.ival = l.store.ival^r.store.ival;
}

void
olor(Node *n, Node *res)
{
	Node l, r;

	expr(n->left, &l);
	expr(n->right, &r);
	res->store.fmt = l.store.fmt;
	res->op = OCONST;
	res->type = TINT;
	if(l.type != TINT || r.type != TINT)
		error("bad expr type |");
	res->store.ival = l.store.ival|r.store.ival;
}

void
ocand(Node *n, Node *res)
{
	Node l, r;

	res->op = OCONST;
	res->type = TINT;
	res->store.ival = 0;
	res->store.fmt = 'D';
	expr(n->left, &l);
	if(bool(&l) == 0)
		return;
	expr(n->right, &r);
	if(bool(&r) == 0)
		return;
	res->store.ival = 1;
}

void
onot(Node *n, Node *res)
{
	Node l;

	res->op = OCONST;
	res->type = TINT;
	res->store.ival = 0;
	expr(n->left, &l);
	res->store.fmt = l.store.fmt;
	if(bool(&l) == 0)
		res->store.ival = 1;
}

void
ocor(Node *n, Node *res)
{
	Node l, r;

	res->op = OCONST;
	res->type = TINT;
	res->store.ival = 0;
	res->store.fmt = 'D';
	expr(n->left, &l);
	if(bool(&l)) {
		res->store.ival = 1;
		return;
	}
	expr(n->right, &r);
	if(bool(&r)) {
		res->store.ival = 1;
		return;
	}
}

void
oeinc(Node *n, Node *res)
{
	Value *v;

	chklval(n->left);
	v = n->left->sym->v;
	res->op = OCONST;
	res->type = v->type;
	switch(v->type) {
	case TINT:
		if(n->op == OEDEC)
			v->store.ival -= fmtsize(v);
		else
			v->store.ival += fmtsize(v);
		break;
	case TFLOAT:
		if(n->op == OEDEC)
			v->store.fval--;
		else
			v->store.fval++;
		break;
	default:
		error("bad type for pre --/++");
	}
	res->store = v->store;
}

void
opinc(Node *n, Node *res)
{
	Value *v;

	chklval(n->left);
	v = n->left->sym->v;
	res->op = OCONST;
	res->type = v->type;
	res->store = v->store;
	switch(v->type) {
	case TINT:
		if(n->op == OPDEC)
			v->store.ival -= fmtsize(v);
		else
			v->store.ival += fmtsize(v);
		break;
	case TFLOAT:
		if(n->op == OPDEC)
			v->store.fval--;
		else
			v->store.fval++;
		break;
	default:
		error("bad type for post --/++");
	}
}

void
ocall(Node *n, Node *res)
{
	Lsym *s;
	Rplace *rsav;

	res->op = OCONST;		/* Default return value */
	res->type = TLIST;
	res->store.l = 0;

	chklval(n->left);
	s = n->left->sym;

	if(n->builtin && !s->builtin){
		error("no builtin %s", s->name);
		return;
	}
	if(s->builtin && (n->builtin || s->proc == 0)) {
		(*s->builtin)(res, n->right);
		return;
	}
	if(s->proc == 0)
		error("no function %s", s->name);

	rsav = ret;
	call(s->name, n->right, s->proc->left, s->proc->right, res);
	ret = rsav;
}

void
ofmt(Node *n, Node *res)
{
	expr(n->left, res);
	res->store.fmt = n->right->store.ival;
}

void
owhat(Node *n, Node *res)
{
	res->op = OCONST;		/* Default return value */
	res->type = TLIST;
	res->store.l = 0;
	whatis(n->sym);
}

void (*expop[])(Node*, Node*) =
{
	[ONAME] =	oname,
	[OCONST] =	oconst,
	[OMUL] =	omul,
	[ODIV] =	odiv,
	[OMOD] =	omod,
	[OADD] =	oadd,
	[OSUB] =	osub,
	[ORSH] =	orsh,
	[OLSH] =	olsh,
	[OLT] =		olt,
	[OGT] =		ogt,
	[OLEQ] =	oleq,
	[OGEQ] =	ogeq,
	[OEQ] =		oeq,
	[ONEQ] =	oeq,
	[OLAND] =	oland,
	[OXOR] =	oxor,
	[OLOR] =	olor,
	[OCAND] =	ocand,
	[OCOR] =	ocor,
	[OASGN] =	oasgn,
	[OINDM] =	oindm,
	[OEDEC] =	oeinc,
	[OEINC] =	oeinc,
	[OPINC] =	opinc,
	[OPDEC]	=	opinc,
	[ONOT] =	onot,
	[OIF] =		0,
	[ODO] =		0,
	[OLIST] =	olist,
	[OCALL] =	ocall,
	[OCTRUCT] =	octruct,
	[OWHILE] =	0,
	[OELSE] =	0,
	[OHEAD] =	ohead,
	[OTAIL] =	otail,
	[OAPPEND] =	oappend,
	[ORET] =	0,
	[OINDEX] =	oindex,
	[OINDC] =	oindc,
	[ODOT] =	odot,
	[OLOCAL] =	0,
	[OFRAME] =	oframe,
	[OCOMPLEX] =	0,
	[ODELETE] =	odelete,
	[OCAST] =	ocast,
	[OFMT] =	ofmt,
	[OEVAL] =	oeval,
	[OWHAT] =	owhat,
};
