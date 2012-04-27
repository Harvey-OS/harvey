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
	int i;
	char buf[2048];
	va_list arg;

	/* Unstack io channels */
	if(iop != 0) {
		for(i = 1; i < iop; i++)
			Bterm(io[i]);
		bout = io[0];
		iop = 0;
	}

	ret = 0;
	gotint = 0;
	Bflush(bout);
	if(silent)
		silent = 0;
	else {
		va_start(arg, fmt);
		vseprint(buf, buf+sizeof(buf), fmt, arg);
		va_end(arg);
		fprint(2, "%L: (error) %s\n", buf);
	}
	while(popio())
		;
	interactive = 1;
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
	Value *v;
	Lsym *sl;
	Node *l, *r;
	vlong i, s, e;
	Node res, xx;
	static int stmnt;

	gc();
	if(gotint)
		error("interrupted");

	if(n == 0)
		return;

	if(stmnt++ > 5000) {
		Bflush(bout);
		stmnt = 0;
	}

	l = n->left;
	r = n->right;

	switch(n->op) {
	default:
		expr(n, &res);
		if(ret || (res.type == TLIST && res.l == 0 && n->op != OADD))
			break;
		prnt->right = &res;
		expr(prnt, &xx);
		break;
	case OASGN:
	case OCALL:
		expr(n, &res);
		break;
	case OCOMPLEX:
		decl(n);
		break;
	case OLOCAL:
		for(n = n->left; n; n = n->left) {
			if(ret == 0)
				error("local not in function");
			sl = n->sym;
			if(sl->v->ret == ret)
				error("%s declared twice", sl->name);
			v = gmalloc(sizeof(Value));
			v->ret = ret;
			v->pop = sl->v;
			sl->v = v;
			v->scope = 0;
			*(ret->tail) = sl;
			ret->tail = &v->scope;
			v->set = 0;
		}
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
			error("loop must have integer start");
		s = res.ival;
		expr(l->right, &res);
		if(res.type != TINT)
			error("loop must have integer end");
		e = res.ival;
		for(i = s; i <= e; i++)
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
convflt(Node *r, char *flt)
{
	char c;

	c = flt[0];
	if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
		r->type = TSTRING;
		r->fmt = 's';
		r->string = strnode(flt);
	}
	else {
		r->type = TFLOAT;
		r->fval = atof(flt);
	}
}

void
indir(Map *m, uvlong addr, char fmt, Node *r)
{
	int i;
	ulong lval;
	uvlong uvval;
	int ret;
	uchar cval;
	ushort sval;
	char buf[512], reg[12];

	r->op = OCONST;
	r->fmt = fmt;
	switch(fmt) {
	default:
		error("bad pointer format '%c' for *", fmt);
	case 'c':
	case 'C':
	case 'b':
		r->type = TINT;
		ret = get1(m, addr, &cval, 1);
		if (ret < 0)
			error("indir: %r");
		r->ival = cval;
		break;
	case 'x':
	case 'd':
	case 'u':
	case 'o':
	case 'q':
	case 'r':
		r->type = TINT;
		ret = get2(m, addr, &sval);
		if (ret < 0)
			error("indir: %r");
		r->ival = sval;
		break;
	case 'a':
	case 'A':
	case 'W':
		r->type = TINT;
		ret = geta(m, addr, &uvval);
		if (ret < 0)
			error("indir: %r");
		r->ival = uvval;
		break;
	case 'B':
	case 'X':
	case 'D':
	case 'U':
	case 'O':
	case 'Q':
		r->type = TINT;
		ret = get4(m, addr, &lval);
		if (ret < 0)
			error("indir: %r");
		r->ival = lval;
		break;
	case 'V':
	case 'Y':
	case 'Z':
		r->type = TINT;
		ret = get8(m, addr, &uvval);
		if (ret < 0)
			error("indir: %r");
		r->ival = uvval;
		break;
	case 's':
		r->type = TSTRING;
		for(i = 0; i < sizeof(buf)-1; i++) {
			ret = get1(m, addr, (uchar*)&buf[i], 1);
			if (ret < 0)
				error("indir: %r");
			addr++;
			if(buf[i] == '\0')
				break;
		}
		buf[i] = 0;
		if(i == 0)
			strcpy(buf, "(null)");
		r->string = strnode(buf);
		break;
	case 'R':
		r->type = TSTRING;
		for(i = 0; i < sizeof(buf)-2; i += 2) {
			ret = get1(m, addr, (uchar*)&buf[i], 2);
			if (ret < 0)
				error("indir: %r");
			addr += 2;
			if(buf[i] == 0 && buf[i+1] == 0)
				break;
		}
		buf[i++] = 0;
		buf[i] = 0;
		r->string = runenode((Rune*)buf);
		break;
	case 'i':
	case 'I':
		if ((*machdata->das)(m, addr, fmt, buf, sizeof(buf)) < 0)
			error("indir: %r");
		r->type = TSTRING;
		r->fmt = 's';
		r->string = strnode(buf);
		break;
	case 'f':
		ret = get1(m, addr, (uchar*)buf, mach->szfloat);
		if (ret < 0)
			error("indir: %r");
		machdata->sftos(buf, sizeof(buf), (void*) buf);
		convflt(r, buf);
		break;
	case 'g':
		ret = get1(m, addr, (uchar*)buf, mach->szfloat);
		if (ret < 0)
			error("indir: %r");
		machdata->sftos(buf, sizeof(buf), (void*) buf);
		r->type = TSTRING;
		r->string = strnode(buf);
		break;
	case 'F':
		ret = get1(m, addr, (uchar*)buf, mach->szdouble);
		if (ret < 0)
			error("indir: %r");
		machdata->dftos(buf, sizeof(buf), (void*) buf);
		convflt(r, buf);
		break;
	case '3':	/* little endian ieee 80 with hole in bytes 8&9 */
		ret = get1(m, addr, (uchar*)reg, 10);
		if (ret < 0)
			error("indir: %r");
		memmove(reg+10, reg+8, 2);	/* open hole */
		memset(reg+8, 0, 2);		/* fill it */
		leieee80ftos(buf, sizeof(buf), reg);
		convflt(r, buf);
		break;
	case '8':	/* big-endian ieee 80 */
		ret = get1(m, addr, (uchar*)reg, 10);
		if (ret < 0)
			error("indir: %r");
		beieee80ftos(buf, sizeof(buf), reg);
		convflt(r, buf);
		break;
	case 'G':
		ret = get1(m, addr, (uchar*)buf, mach->szdouble);
		if (ret < 0)
			error("indir: %r");
		machdata->dftos(buf, sizeof(buf), (void*) buf);
		r->type = TSTRING;
		r->string = strnode(buf);
		break;
	}
}

void
windir(Map *m, Node *addr, Node *rval, Node *r)
{
	uchar cval;
	ushort sval;
	long lval;
	Node res, aes;
	int ret;

	if(m == 0)
		error("no map for */@=");

	expr(rval, &res);
	expr(addr, &aes);

	if(aes.type != TINT)
		error("bad type lhs of @/*");

	if(m != cormap && wtflag == 0)
		error("not in write mode");

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
		ret = put1(m, aes.ival, &cval, 1);
		break;
	case 'r':
	case 'x':
	case 'd':
	case 'u':
	case 'o':
		sval = res.ival;
		ret = put2(m, aes.ival, sval);
		r->ival = sval;
		break;
	case 'a':
	case 'A':
	case 'W':
		ret = puta(m, aes.ival, res.ival);
		break;
	case 'B':
	case 'X':
	case 'D':
	case 'U':
	case 'O':
		lval = res.ival;
		ret = put4(m, aes.ival, lval);
		break;
	case 'V':
	case 'Y':
	case 'Z':
		ret = put8(m, aes.ival, res.ival);
		break;
	case 's':
	case 'R':
		ret = put1(m, aes.ival, (uchar*)res.string->string, res.string->len);
		break;
	}
	if (ret < 0)
		error("windir: %r");
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

	rlab.local = 0;

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

	rlab.tail = &rlab.local;

	ret = &rlab;
	for(i = 0; i < np; i++) {
		n = ava[i];
		switch(n->op) {
		default:
			error("%s: %d formal not a name", fn, i);
		case ONAME:
			expr(avp[i], &res);
			s = n->sym;
			break;
		case OINDM:
			res.cc = avp[i];
			res.type = TCODE;
			res.comt = 0;
			if(n->left->op != ONAME)
				error("%s: %d formal not a name", fn, i);
			s = n->left->sym;
			break;
		}
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
