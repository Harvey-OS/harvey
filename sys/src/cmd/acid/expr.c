#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

char fsize[] =
{
	['f'] 4,
	['F'] 8,
	['a'] 4,
	['c'] 1,
	['C'] 1,
	['b'] 1,
	['x'] 2,
	['d'] 2,
	['u'] 2,
	['o'] 2,
	['X'] 4,
	['D'] 4,
	['U'] 4,
	['Y'] 4,
	['O'] 4,
	['s'] 1,
	['R'] 1,
	['q'] 2,
	['Q'] 4,
	['r'] 2,
};

int
fmtsize(Value *v)
{
	switch(v->fmt) {
	default:
		return  fsize[v->fmt];
	case 'i':
	case 'I':
		if(v->type != TINT)
			error("no size for i fmt pointer ++/--");
		xprint = 1;
		asmbuf[0] = '\0';
		dot = v->ival;
		(*machdata->printins)(symmap, v->fmt, SEGANY);
		xprint = 0;
		return dotinc;
	}
}

void
chklval(Node *lp)
{
	if(lp->op != ONAME)
		error("need l-value");
}

void
expr(Node *n, Node *res)
{
	Map *m;
	char c;
	Lsym *s;
	Value *v;
	ulong addr;
	Rplace *rsav;
	Node l, r, *lp, *rp;

	lp = n->left;
	rp = n->right;

	switch(n->op) {
	case ODOT:
		dodot(n, res);
		return;
	case OINDM:
		c = '*';
		m = cormap;
		goto common;
	case OINDC:
		c = '@';
		m = symmap;
	common:
		expr(lp, &l);
		if(l.type != TINT)
			error("bad type for %c", c);
		if(m == 0)
			error("no map for %c", c);
		indir(m, l.ival, l.fmt, res);
		res->comt = l.comt;
		break;
	case OFRAME:
		localaddr(n->sym, n->left->sym, res);
		break;	
	case OINDEX:
		expr(lp, &l);
		expr(rp, &r);
		if(r.type != TINT)
			error("bad type for []");
		switch(l.type) {
		default:
			error("lhs[] has bad type");
		case TINT:
			addr = l.ival+(r.ival*fsize[l.fmt]);
			indir(cormap, addr, l.fmt, res);
			break;
		case TLIST:
			nthelem(l.l, r.ival, res);
			break;
		case TSTRING:
			res->ival = 0;
			if(r.ival >= 0 && r.ival < l.string->len)
				res->ival = l.string->string[r.ival];
			res->op = OCONST;
			res->type = TINT;
			res->fmt = 'c';
			break;
		}
		break;
	case OAPPEND:
		expr(lp, &l);
		expr(rp, &r);
		if(l.type != TLIST)
			error("must append to list");
		append(res, &l, &r);
		break;
	case ODELETE:
		expr(lp, &l);
		expr(rp, &r);
		if(l.type != TLIST)
			error("must delete from list");
		if(r.type != TINT)
			error("delete index must be integer");

		delete(l.l, r.ival, res);
		break;
	case OHEAD:
		expr(lp, &l);
		if(l.type != TLIST)
			error("head needs list");
		res->op = OCONST;
		if(l.l) {
			res->type = l.l->type;
			res->Store = l.l->Store;
		}
		else {
			res->type = TLIST;
			res->l = 0;
		}
		break;
	case OTAIL:
		expr(lp, &l);
		if(l.type != TLIST)
			error("tail needs list");
		res->op = OCONST;
		res->type = TLIST;
		if(l.l)
			res->Store.l = l.l->next;
		else
			res->Store.l = 0;
		break;
	case OCONST:
		*res = *n;
		break;
	case ONAME:
		v = n->sym->v;
		if(v->set == 0)
			error("%s used but not set", n->sym->name);
		res->op = OCONST;
		res->type = v->type;
		res->Store = v->Store;
		break;
	case OCTRUCT:
		res->op = OCONST;
		res->type = TLIST;
		res->l = construct(lp);
		break;
	case OASGN:
		switch(lp->op) {
		case OINDM:
			windir(cormap, lp->left, rp, res);
			break;
		case OINDC:
			windir(symmap, lp->left, rp, res);
			break;
		default:
			chklval(lp);
			v = lp->sym->v;
			expr(rp, &r);
			v->set = 1;
			v->type = r.type;
			v->Store = r.Store;
			*res = r;
		}
		break;
	case OADD:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TFLOAT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->type = TINT;
				res->ival = l.ival+r.ival;
				break;
			case TFLOAT:
				res->fval = l.ival+r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type +");
			}
			break;
		case TFLOAT:
			switch(r.type) {
			case TINT:
				res->fval = l.fval+r.ival;
				break;
			case TFLOAT:
				res->fval = l.fval+r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type +");
			}
			break;
		case TSTRING:
			if(r.type == TSTRING) {
				res->type = TSTRING;
				res->fmt = 's';
				res->string = stradd(l.string, r.string); 
				break;
			}
			error("bad rhs for +");

		case TLIST:
			res->type = TLIST;
			switch(r.type) {
			case TLIST:
				res->l = addlist(l.l, r.l);
				break;
			default:
				r.left = 0;
				r.right = 0;
				res->l = addlist(l.l, construct(&r));
				break;
			}
		}
		break;
	case OSUB:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TFLOAT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->type = TINT;
				res->ival = l.ival-r.ival;
				break;
			case TFLOAT:
				res->fval = l.ival-r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type -");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->fval = l.fval-r.ival;
				break;
			case TFLOAT:
				res->fval = l.fval-r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type -");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type -");
		}
		break;
	case OMUL:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TFLOAT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->type = TINT;
				res->ival = l.ival*r.ival;
				break;
			case TFLOAT:
				res->fval = l.ival*r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type *");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->fval = l.fval*r.ival;
				break;
			case TFLOAT:
				res->fval = l.fval*r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type *");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type *");
		}
		break;
	case ODIV:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TFLOAT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->type = TINT;
				res->ival = l.ival/r.ival;
				break;
			case TFLOAT:
				res->fval = l.ival/r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type /");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->fval = l.fval/r.ival;
				break;
			case TFLOAT:
				res->fval = l.fval/r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type /");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type /");
		}
		break;
	case OMOD:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type %");
		res->ival = l.ival%r.ival;
		break;
	case OLSH:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type <<");
		res->ival = l.ival<<r.ival;
		break;
	case ORSH:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type >>");
		res->ival = l.ival>>r.ival;
		break;
	case OLT:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival < r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival < r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type <");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval < r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval < r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type <");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type <");
		}
		break;
	case OGT:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = 'D';
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival > r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival > r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type >");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval > r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval > r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type >");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type >");
		}
		break;
	case OLEQ:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = 'D';
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival <= r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival <= r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type <=");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval <= r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval <= r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type <=");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type <=");
		}
		break;
	case OGEQ:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = 'D';
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival >= r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival >= r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type >=");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval >= r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval >= r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type >=");
			}
			break;
		case TSTRING:
		case TLIST:
			error("bad expr type >=");
		}
		break;
	case OEQ:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = 'D';
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival == r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival == r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type ==");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval == r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval == r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type ==");
			}
			break;
		case TSTRING:
			if(r.type == TSTRING) {
				res->ival = scmp(r.string, l.string);
				break;
			}
		case TLIST:
			if(r.type == TLIST) {
				res->ival = listcmp(l.l, r.l);
				break;
			}
			error("bad expr type ==");
		}
		break;
	case ONEQ:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = 'D';
		res->op = OCONST;
		res->type = TINT;
		switch(l.type) {
		case TINT:
			switch(r.type) {
			case TINT:
				res->ival = l.ival != r.ival;
				break;
			case TFLOAT:
				res->ival = l.ival != r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type !=");
			}
			break;
		case TFLOAT:
			switch(l.type) {
			case TINT:
				res->ival = l.fval != r.ival;
				break;
			case TFLOAT:
				res->ival = l.fval != r.fval;
				break;
			case TSTRING:
			case TLIST:
				error("bad expr type !=");
			}
			break;
		case TSTRING:
			if(r.type == TSTRING) {
				res->ival = !scmp(r.string, l.string);
				break;	
			}
		case TLIST:
			if(r.type == TLIST) {
				res->ival = !listcmp(l.l, r.l);
				break;
			}
			error("bad expr type !=");
		}
		break;
	case OLAND:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type &");
		res->ival = l.ival&r.ival;
		break;
	case OXOR:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type ^");
		res->ival = l.ival^r.ival;
		break;
	case OLOR:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		if(l.type != TINT || r.type != TINT)
			error("bad expr type |");
		res->ival = l.ival|r.ival;
		break;
	case OCAND:
		expr(lp, &l);
		expr(rp, &r);
		res->fmt = l.fmt;
		res->op = OCONST;
		res->type = TINT;
		res->ival = bool(&l) && bool(&r);
		break;
	case OCOR:
		expr(lp, &l);
		expr(rp, &r);
		res->op = OCONST;
		res->type = TINT;
		res->ival = bool(&l) || bool(&r);
		break;
	case OEDEC:
	case OEINC:
		chklval(lp);
		v = lp->sym->v;
		res->op = OCONST;
		res->type = v->type;
		switch(v->type) {
		case TINT:
			if(n->op == OEDEC)
				v->ival -= fmtsize(v);
			else
				v->ival += fmtsize(v);
			break;			
		case TFLOAT:
			if(n->op == OEDEC)
				v->fval--;
			else
				v->fval++;
			break;
		default:
			error("bad type for pre --/++");
		}
		res->Store = v->Store;
		break;	
	case OPDEC:
	case OPINC:
		chklval(lp);
		v = lp->sym->v;
		res->op = OCONST;
		res->type = v->type;
		res->Store = v->Store;
		switch(v->type) {
		case TINT:
			if(n->op == OPDEC)
				v->ival -= fmtsize(v);
			else
				v->ival += fmtsize(v);
			break;			
		case TFLOAT:
			if(n->op == OPDEC)
				v->fval--;
			else
				v->fval++;
			break;
		default:
			error("bad type for post --/++");
		}
		break;
	case OCALL:
		res->op = OCONST;		/* Default return value */
		res->type = TLIST;
		res->l = 0;

		chklval(lp);
		s = lp->sym;

		if(s->builtin) {
			(*s->builtin)(res, rp);
			break;
		}
		if(s->proc == 0)
			error("no function %s", s->name);

		rsav = ret;
		call(s->name, rp, s->proc->left, s->proc->right, res);
		ret = rsav;
		break;
	}
}
