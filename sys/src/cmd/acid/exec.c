#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

void
error(char *fmt, ...)
{
	char buf[128];

	ret = 0;
	gotint = 0;
	Bflush(bin);
	Bflush(bout);
	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(filename)
		fprint(2, "%d: (%s) %s\n", line, filename, buf);
	else
		fprint(2, "%d: (error) %s\n", line, buf);

	longjmp(err, 1);
}

void
unwind(void)
{
	int i;
	Lsym *s;
	Value *v;

	for(i = 0; i < Hashsize; i++) {
		for(s = hash[i]; s; s = s->hash) {
			while(s->v->pop) {
				v = s->v->pop;
				free(s->v);
				s->v = v;
			}
		}
	}
}

void
execute(Node *n)
{
	Node res;
	Value *v;
	Lsym *sl;
	Node *l, *r;
	int i, s, e;
	static int stmnt;

	if(gotint)
		error("interrupted");

	if(n == 0)
		return;

	if(stmnt++ > 10000) {
		Bflush(bout);
		stmnt = 0;
	}

	l = n->left;
	r = n->right;

	switch(n->op) {
	default:
		expr(n, &res);
		break;
	case OCOMPLEX:
		decl(n->sym, l->sym);
		break;
	case OLOCAL:
		if(ret == 0)
			error("local not in function");
		sl = n->sym;
		if(sl->v->ret == ret)
			error("%s already declared at this scope", sl->name);
		v = gmalloc(sizeof(Value));
		v->ret = ret;
		v->pop = sl->v;
		sl->v = v;
		v->scope = 0;
		*(ret->tail) = sl;
		ret->tail = &v->scope;
		v->set = 0;
		break;
	case ORET:
		if(ret == 0)
			error("return not in function");
		expr(n->left, ret->val);
		longjmp(ret->rlab, 1);
	case OLIST:
		execute(n->left);
		execute(n->right);
		break;
	case OIF:
		expr(l, &res);
		if(r && r->op == OELSE) {
			if(bool(&res))
				execute(r->left);
			else
				execute(r->right);
		}
		else if(bool(&res))
			execute(r);
		break;
	case OWHILE:
		for(;;) {
			expr(l, &res);
			if(!bool(&res))
				break;
			execute(r);
		}
		break;
	case ODO:
		expr(l->left, &res);
		if(res.type != TINT)
			error("do must have integer start");
		s = res.ival;
		expr(l->right, &res);
		if(res.type != TINT)
			error("do must have integer end");
		e = res.ival;
		for(i = s; i < e; i++)
			execute(r);
		break;
	}
}

int
bool(Node *n)
{
	int true = 0;

	if(n->op != OCONST)
		fatal("bool: not const");

	switch(n->type) {
	case TINT:
		if(n->ival != 0)
			true = 1;
		break;
	case TFLOAT:
		if(n->fval != 0.0)
			true = 1;
		break;
	case TSTRING:
		if(n->string->len)
			true = 1;
		break;
	case TLIST:
		if(n->l)
			true = 1;
		break;
	}
	return true;
}

void
indir(Map *m, ulong addr, char fmt, Node *r)
{
	int i;
	int ival;
	uchar cval;
	ushort sval;
	char buf[512];

	r->op = OCONST;
	r->fmt = fmt;
	switch(fmt) {
	default:
		error("bad pointer format '%c' for *", fmt);
	case 'c':
	case 'C':
	case 'b':
		r->type = TINT;
		get1(m, addr, SEGANY, &cval, 1);
		r->ival = cval;
		break;
	case 'x':
	case 'd':
	case 'u':
	case 'o':
	case 'q':
		r->type = TINT;
		get2(m, addr, SEGANY, &sval);
		r->ival = sval;
		break;
	case 'X':
	case 'D':
	case 'U':
	case 'O':
	case 'Q':
	case 'Y':
		r->type = TINT;
		get4(m, addr, SEGANY, &ival);
		r->ival = ival;
		break;
	case 's':
		r->type = TSTRING;
		for(i = 0; i < sizeof(buf)-1; i++) {
			get1(m, addr, SEGANY, (uchar*)&buf[i], 1);
			addr++;
			if(buf[i] == '\0')
				break;
		}
		buf[i] = 0;
		r->string = strnode(buf);
		break;
	case 'R':
		r->type = TSTRING;
		for(i = 0; i < sizeof(buf)-1; i += 2) {
			get1(m, addr, SEGANY, (uchar*)&buf[i], 2);
			addr += 2;
			if(buf[i] == 0 && buf[i+1] == 0)
				break;
		}
		buf[i] = 0;
		r->string = strnode(buf);
		break;
	case 'i':
	case 'I':
		xprint = 1;
		asmbuf[0] = '\0';
		dot = addr;
		(*machdata->printins)(m, fmt, SEGANY);
		xprint = 0;
		r->type = TSTRING;
		r->fmt = 's';
		r->string = strnode(asmbuf);
		break;
	case 'f':
		r->type = TFLOAT;
		get1(m, addr, SEGANY, (uchar*)buf, mach->szfloat);
		break;
	case 'F':
		r->type = TFLOAT;
		get1(m, addr, SEGANY, (uchar*)buf, mach->szdouble);
		break;
	}
}

void
windir(Map *m, Node *addr, Node *rval, Node *r)
{
	uchar cval;
	ushort sval;
	Node res, aes;

	if(m == 0)
		error("no map for */@=");

	expr(rval, &res);
	expr(addr, &aes);

	if(aes.type != TINT)
		error("bad type lhs of @/*");

	r->type = res.type;
	r->fmt = res.fmt;
	r->Store = res.Store;

	switch(res.fmt) {
	default:
		error("bad pointer format '%c' for */@=", res.fmt);
	case 'c':
	case 'C':
	case 'b':
		cval = res.ival;
		put1(m, aes.ival, SEGANY, &cval, 1);
		break;
	case 'r':
	case 'x':
	case 'd':
	case 'u':
	case 'o':
		sval = res.ival;
		put2(m, aes.ival, SEGANY, sval);
		r->ival = sval;
		break;
	case 'X':
	case 'D':
	case 'U':
	case 'O':
	case 'Y':
		put4(m, aes.ival, SEGANY, res.ival);
		break;
	case 's':
	case 'R':
		put1(m, aes.ival, SEGANY, (uchar*)res.string->string, res.string->len);
		break;
	}
}

void
call(char *fn, Node *parameters, Node *local, Node *body, Node *retexp)
{
	int np, i;
	Rplace rlab;
	Node *n, res;
	Value *v, *f;
	Lsym *s, *next;
	Node *avp[Maxarg], *ava[Maxarg];

	na = 0;
	flatten(avp, parameters);
	np = na;
	na = 0;
	flatten(ava, local);
	if(np != na) {
		if(np < na)
			error("%s: too few arguments", fn);

		error("%s: too many arguments", fn);
	}

	rlab.local = 0;
	rlab.tail = &rlab.local;

	ret = &rlab;
	for(i = 0; i < np; i++) {
		n = ava[i];
		if(n->op != ONAME)
			error("%s: %d formal not a name", fn, i);

		expr(avp[i], &res);

		s = n->sym;
		if(s->v->ret == ret)
			error("%s already declared at this scope", s->name);

		v = gmalloc(sizeof(Value));
		v->ret = ret;
		v->pop = s->v;
		s->v = v;
		v->scope = 0;
		*(rlab.tail) = s;
		rlab.tail = &v->scope;

		v->Store = res.Store;
		v->type = res.type;
		v->set = 1;
	}

	ret->val = retexp;
	if(setjmp(rlab.rlab) == 0)
		execute(body);

	for(s = rlab.local; s; s = next) {
		f = s->v;
		next = f->scope;
		s->v = f->pop;
		free(f);
	}
}
