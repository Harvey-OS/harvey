#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

void
gendata(Node *n)
{
	Type *t;
	Inst *i;
	ulong s;

	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		gendata(n->left);
		gendata(n->right);
		break;

	case OAGDECL:
	case OUNDECL:
	case OADTDECL:
	case OSETDECL:
	case OFUNC:
		break;

	case ONAME:
		switch(n->t->class) {
		case Internal:
		case Global:
			iline = n->srcline;
			t = n->t;
			if(n->init)
				doinit(n, t, n->init, 0);

			s = t->size;
			if(s == 0)
				break;
			if(s & (Align_data-1))
				s += Align_data-(s&(Align_data-1));

			i = ai();
			i->op = AGLOBL;
			mkaddr(con(s), &i->dst, 0);
			mkaddr(n, &i->src1, 0);
			ilink(i);
			break;
		}
		break;
	}
}

/* Load/store instructions by type

		TXXX  TINT   TUINT  TSINT  TSUINT TCHAR  TFLOAT TIND 
		TCHAN  TARY   TAGG   TUNI   TFUNC  TVOID
*/
int ostore[] ={ AGOK, AMOVW, AMOVW, AMOVH, AMOVH, AMOVB, AMOVD, AMOVW, 
		AMOVW, AGOK,  AGOK,  AGOK,  AGOK,  AGOK };

int oload[] = { AGOK, AMOVW, AMOVW, AMOVH, AMOVHU,AMOVBU,AMOVD, AMOVW,
		AMOVW,AGOK,  AGOK,  AGOK,  AGOK,  AGOK };

void
load(Node *from, Node *to)
{
	Node n1;
	int op;

	op = oload[from->t->type];
	reg(&n1, from->t, to);
	instruction(op, from, ZeroN, &n1);
	assign(&n1, to);
	regfree(&n1);
}

void
store(Node *from, Node *to)
{
	Node n1;
	int op, t;

	op = ostore[to->t->type];

	t = from->t->type;

	if(t != TFLOAT)
	if(vconst(from) == 0) {
		instruction(op, from, ZeroN, to);
		return;
	}
		
	if(t == to->t->type)
		reg(&n1, to->t, from);
	else
		reg(&n1, to->t, ZeroN);

	assign(from, &n1);
	instruction(op, &n1, ZeroN, to);
	regfree(&n1);
}

/*
 * Compile code for moves
 */
void
assign(Node *src, Node *dst)
{
	Node n1;
	int op, dt;

	if(opt('a'))
		print("assign: %N = %N\n", dst, src); 

	if(dst == ZeroN)
		return;

	switch(src->type) {
	case ONAME:
	case OINDREG:
	case OIND:
		load(src, dst);
		return;
	}

	switch(dst->type) {
	case ONAME:
	case OINDREG:
	case OIND:
		store(src, dst);
		return;
	}

	/* delete register/register nop moves */
	if(src->type == OREGISTER)
	if(dst->type == OREGISTER)
	if(dst->reg == src->reg)
	if(dst->type == src->type)
		return;

	op = AGOK;
	dt = dst->t->type;
	switch(src->t->type) {
	case TINT:
	case TUINT:
	case TIND:
	case TCHANNEL:
		switch(dt) {
		case TINT:
		case TUINT:
		case TCHAR:
		case TSUINT:
		case TSINT:
		case TIND:
		case TCHANNEL:
			op = AMOVW;
			break;

		case TFLOAT:
			instruction(AMOVW, src, ZeroN, dst);
			instruction(AMOVWD, dst, ZeroN, dst);
			return;
		}
		break;

	case TSUINT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVHU;
			break;

		case TSUINT:
		case TCHAR:
		case TSINT:
			op = AMOVW;
			break;

		case TFLOAT:
			reg(&n1, src->t, ZeroN);
			instruction(AMOVHU, src, ZeroN, &n1);
			instruction(AMOVW, &n1, ZeroN, dst);
			instruction(AMOVWD, dst, ZeroN, dst);
			regfree(&n1);
			return;
		}
		break;

	case TSINT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVH;
			break;

		case TSUINT:
		case TCHAR:
		case TSINT:
			op = AMOVW;
			break;

		case TFLOAT:
			reg(&n1, src->t, ZeroN);
			instruction(AMOVH, src, ZeroN, &n1);
			instruction(AMOVW, &n1, ZeroN, dst);
			instruction(AMOVWD, dst, ZeroN, dst);
			regfree(&n1);
			return;
		}
		break;

	case TFLOAT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			instruction(AMOVDW, src, ZeroN, src);
			instruction(AMOVW, src, ZeroN, dst);
			return;

		case TCHAR:
			reg(&n1, dst->t, ZeroN);
			instruction(AMOVDW, src, ZeroN, src);
			instruction(AMOVW, src, ZeroN, &n1);
			instruction(AMOVBU, &n1, ZeroN, dst);
			regfree(&n1);
			return;

		case TSINT:
			reg(&n1, dst->t, ZeroN);
			instruction(AMOVDW, src, ZeroN, src);
			instruction(AMOVW, src, ZeroN, &n1);
			instruction(AMOVH, &n1, ZeroN, dst);
			regfree(&n1);
			return;

		case TSUINT:
			reg(&n1, dst->t, ZeroN);
			instruction(AMOVDW, src, ZeroN, src);
			instruction(AMOVW, src, ZeroN, &n1);
			instruction(AMOVHU, &n1, ZeroN, dst);
			regfree(&n1);
			return;

		case TFLOAT:
			op = AMOVD;
			break;
		}
		break;
	case TCHAR:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVBU;
			break;

		case TSUINT:
		case TSINT:
		case TCHAR:
			op = AMOVW;
			break;

		case TFLOAT:
			reg(&n1, src->t, ZeroN);
			instruction(AMOVBU, src, ZeroN, &n1);
			instruction(AMOVW, &n1, ZeroN, dst);
			instruction(AMOVWD, dst, ZeroN, dst);
			regfree(&n1);
			return;
		}
		break;
	}
	instruction(op, src, ZeroN, dst);
}

/*
 * Generate the address of an expression
 */
void
genaddr(Node *expr, Node *dst)
{
	Node n, *tmp;

	if(opt('g'))
		print("genaddr: %N\n", expr);

	if(expr->type == OIND) {
		genexp(expr->left, dst);
		return;
	}

	n.srcline = expr->srcline;

	if(isaddr(expr) == 0) {
		tmp = stknode(expr->t);
		n.type = OASGN;
		n.t = tmp->t;
		n.left = tmp;
		n.right = expr;
		n.islval = 0;
		genexp(&n, ZeroN);
		expr = tmp;
	}
	n = *expr;
	reg(&n, builtype[TIND], dst);
	n.type = OADDR;
	n.left = expr;
	n.right = ZeroN;
	n.t = builtype[TIND];
	assign(&n, dst);
	regfree(&n);
}

/* 
 * generate an l-value into a register
 */
void
rgenaddr(Node *t, Node *rval, Node *lval)
{
	Node *r;
	ulong v;

	*t = *rval;
	t->t = builtype[TIND];
	reg(t, t->t, lval);
	if(rval->type == OIND) {
		r = rval->left;
		while(r->type == OADD)
			r = r->right;
		if(immed(r)) {
			v = r->ival;
			r->ival = 0;
			genaddr(rval, t);
			t->ival += v;
			r->ival = v;
			t->type = OINDREG;
			t->t = rval->t;
			return;
		}
	}
	genaddr(rval, t);
	t->type = OINDREG;
	t->t = rval->t;
}

/*
 * pre and post increment generation
 */
void
postop(Node *rval, Node *lval)
{
	Node *l, *nconst;
	Node n1, n2, n3, n4;

	l = rval->left;
	nconst = con(1);
	if(l->t->type == TIND)
		nconst->ival = l->t->next->size;

	if(rval->type == OPDEC)
		nconst->ival = -nconst->ival;

	if(l->islval < Sucompute)
		rgenaddr(&n2, l, ZeroN);
	else
		n2 = *l;

	n3.type = OADD;
	n3.t = builtype[TINT];

	/* Easy if we dont need the value after the inc */
	if(lval == ZeroN) {
		reg(&n1, l->t, lval);
		assign(&n2, &n1);

		codmop(&n3, nconst, &n1, &n1);

		assign(&n1, &n2);
		goto out;
	}

	reg(&n4, l->t, lval);
	assign(&n2, &n4);
	reg(&n1, l->t, ZeroN);
	/* Thank RISC for three address ! */
	codmop(&n3, nconst, &n4, &n1);
	assign(&n1, &n2);

	regfree(&n4);
out:
	regfree(&n1);
	if(l->islval < Sucompute)
		regfree(&n2);
}

void
preop(Node *rval, Node *lval)
{
	Node n1, n2, n3;
	Node *l, *nconst;

	l = rval->left;
	nconst = con(1);
	if(l->t->type == TIND)
		nconst->ival = l->t->next->size;

	if(rval->type == OEDEC)
		nconst->ival = -nconst->ival;

	if(l->islval < Sucompute)
		rgenaddr(&n2, l, ZeroN);
	else
		n2 = *l;

	reg(&n1, l->t, lval);
	assign(&n2, &n1);

	n3.type = OADD;
	n3.t = builtype[TINT];
	codmop(&n3, nconst, &n1, &n1);

	assign(&n1, &n2);

	regfree(&n1);
	if(l->islval < Sucompute)
		regfree(&n2);
}

void
docall(Node *n)
{
	Node *rsp;

	if(atv == ZeroN) {
		instruction(AJAL, ZeroN, ZeroN, n);
		return;
	}

	/*
	 * if calling through an activation, link
	 * to the new stack
	 */
	ratv.ival = atv->atvsafe;
	rsp = regn(RegSP);
	instruction(AMOVW, rsp, ZeroN, &ratv);
	ratv.type = OREGISTER;
	assign(&ratv, rsp);
	instruction(AJAL, ZeroN, ZeroN, n);
	rsp->type = OINDREG;
	rsp->ival = atv->atvsafe;
	instruction(AMOVW, rsp, ZeroN, regn(RegSP));
	regfree(&ratv);
}

void
become(Node *rval)
{
	Node nl;
	Node *l, *r, *ns, *tmp;

	tmp = rval->left;
	r = tmp->right;
	l = tmp->left;

	if(opt('b')) {
		print("BECOME:\n");
		ptree(rval, 0);
	}

	tmp = ZeroN;
	if(!isaddr(l)) {
		tmp = stknode(builtype[TIND]);
		genaddr(l, tmp);
	}

	/* Generate the alias saves */
	genelist(rval->right);

	evalarg(r, 0);

	/*
	 * Write out tail recursion
	 */
	if(l->type == ONAME && curfunc->sym == l->sym) {
		tip = block[Scope];
		evalarg(r, 4);
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		label(ipc, becomentry->pc+1);
		return;
	}

	ns = an(ONAME, ZeroN, ZeroN);
	ns->sym = malloc(sizeof(Sym));
	ns->sym->name = ".xframe";
	ns->ti = ati(builtype[TADT], Parameter);
	ns->ti->offset = 0;
	ns->t = builtype[TINT];
	ns = an(OADDR, ns, nil);
	sucalc(ns);

	reg(&ratv, builtype[TIND], ZeroN);
	ratv.ival = Parambase;
	genexp(ns, &ratv);
	evalarg(r, 3);
	regfree(&ratv);

	/* Restore the complex pointer */
	switch(curfunc->t->next->type) {
	case TADT:
	case TUNION:
	case TAGGREGATE:
		assign(rnode, regn(Regspass));
		break;
	}

	if(tmp) {
		reg(&nl, builtype[TIND], ZeroN);
		assign(tmp, &nl);
		nl.type = OINDREG;
		nl.ival = 0;
		instruction(ARET, con(ratv.ival), ZeroN, &nl);
		regfree(&nl);
	}
	else
		instruction(ARET, con(ratv.ival), ZeroN, l);
}

/*
 * Compile code expressions
 */
void
genexp(Node *rval, Node *lval)
{
	Node nl, nr, nspare;
	Node *l, *r, *nstack;

	l = rval->left;
	r = rval->right;

	if(opt('v'))
		print("%L R %N L %N\n", rval->srcline, rval, lval);

	if(rval->t)
	switch(rval->t->type) {
	case TADT:
	case TUNION:
	case TAGGREGATE:
		gencomplex(rval, lval);
		return;
	}

	/* addressable */
	if(isaddr(rval)) {
		assign(rval, lval);
		return;
	}

	/* Follow function call evaluations into stack temporary */
	if(rval->sun >= Sucall)
	if(l->sun >= Sucall)
	if(r && r->sun >= Sucall)
	switch(rval->type) {
	case OCALL:
	case OLAND:
	case OLOR:
		break;

	default:
		regret(&nspare, r->t);
		genexp(r, &nspare);
		nstack = stknode(r->t);
		assign(&nspare, nstack);
		regfree(&nspare);
		nspare = *rval;
		nspare.right = nstack;
		genexp(&nspare, lval);
		return;
	}

	switch(rval->type) {
	default:
		fatal("genexp: %N", rval);
		break;

	case OBLOCK:
		bstmnt(l);
		genexp(r, lval);
		break;

	case OBECOME:
		become(rval);
		break;

	case ODOT:
		nstack = stknode(l->t);
		genexp(l, nstack);
		if(rval == ZeroN)
			break;

		rgenaddr(&nspare, nstack, ZeroN);
		nspare.ival = r->ival;
		nspare.t = rval->t;
		genexp(&nspare, lval);
		regfree(&nspare);
		break;

	case OCONV:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}

		if(convisnop(l->t, rval->t))
		if(convisnop(rval->t, lval->t)) {
				genexp(l, lval);
				break;
		}
		reg(&nl, l->t, lval);
		genexp(l, &nl);
		reg(&nr, rval->t, &nl);
		assign(&nl, &nr);
		assign(&nr, lval);
		regfree(&nr);
		regfree(&nl);
		break;

	case OASGN:
		oasgn(rval, lval);
		break;

	case OADDR:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}
		genaddr(l, lval);
		break;

	case OIND:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}
		reg(&nr, builtype[TIND], lval);
		if(l->type == OADD && l->right->type == OCONST) {
			nr.ival = l->right->ival;
			genexp(l->left, &nr);
		}
		else {
			genexp(l, &nr);
			nr.ival = 0;
		}
		nr.type = OINDREG;
		nr.t = rval->t;
		assign(&nr, lval);
		regfree(&nr);
		break;

	case OCAND:
	case OCOR:
	case OEQ:
	case OLEQ:
	case ONEQ:
	case OLT:
	case OGEQ:
	case OGT:
	case OHI:
	case OLO:
	case OLOS:
	case OHIS:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}
		gencond(rval, lval, True);
		break;

	case ONOT:
		gencond(l, lval, False);
		break;

	case OADDEQ:
	case OSUBEQ:
	case ORSHEQ:
	case OLSHEQ:
	case OANDEQ:
	case OOREQ:
	case OXOREQ:
		while(l->type == OCONV)
			l = l->left;

		/* Peel off the instructions with immediate operands */
		if(r->t->type != TFLOAT)
		if(r->type == OCONST) {
			if(l->islval < Sucompute)
				rgenaddr(&nspare, l, ZeroN);
			else
				nspare = *l;
			reg(&nl, r->t, lval);
			assign(&nspare, &nl);
			codmop(rval, r, &nl, &nl);
			assign(&nl, &nspare);
	
			regfree(&nl);
			if(l->islval < Sucompute)
				regfree(&nspare);
			break;
		}

		/* Fall through */
	case OMULEQ:
	case ODIVEQ:
	case OMODEQ:
		while(l->type == OCONV)
			l = l->left;

		if(l->sun >= r->sun) {
			if(l->islval < Sucompute)
				rgenaddr(&nspare, l, ZeroN);
			else
				nspare = *l;
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
		} else {
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
			if(l->islval < Sucompute)
				rgenaddr(&nspare, l, ZeroN);
			else
				nspare = *l;
		}

		reg(&nl, rval->t, lval);
		assign(&nspare, &nl);
		codmop(rval, &nr, ZeroN, &nl);
		assign(&nl, &nspare);
		assign(&nl, lval);
		regfree(&nl);
		regfree(&nr);
		if(l->islval < Sucompute)
			regfree(&nspare);
		break;
		
	case OADD:
	case OSUB:
	case OLAND:
	case OLOR:
	case OXOR:
	case OLSH:
	case ORSH:
	case OALSH:
	case OARSH:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}
		/* Peel off the instructions with immediate operands */
		if(r->t->type != TFLOAT)
		if(r->type == OCONST) {
			genexp(l, lval);
			if(r->ival == 0)
			if(rval->type != OLAND)
				break;
			codmop(rval, r, lval, lval);
			break;
		}
		
		/* Fall through */
	case OMUL:
	case ODIV:
	case OMOD:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}

		if(rval->type == OMUL && mulcon(rval, lval))
			break;

		if(l->sun >= r->sun) {
			reg(&nl, l->t, lval);
			genexp(l, &nl);
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
			codmop(rval, &nr, &nl, &nl);
		}
		else {
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
			reg(&nl, l->t, lval);
			genexp(l, &nl);
			codmop(rval, &nr, &nl, &nl);
		}
		regfree(&nr);
		assign(&nl, lval);
		regfree(&nl);
		break;

	case OPINC:
	case OPDEC:
		postop(rval, lval);
		break;

	case OEINC:
	case OEDEC:
		preop(rval, lval);
		break;

	case OSEND:
		genexp(l, lval);
		break;

	case ORECV:
	case OCALL:
		if(l->sun >= Sucall) {
			reg(&nr, builtype[TIND], ZeroN);
			genaddr(l, &nr);
			if(nr.type != OREGISTER)
				fatal("genexp: call %N", &nr);

			nstack = stknode(builtype[TIND]);
			assign(&nr, nstack);
			regfree(&nr);

			genarg(r);

			reg(&nr, builtype[TIND], ZeroN);
			assign(nstack, &nr);
			nr.type = OINDREG;
			nr.ival = 0;
			docall(&nr);
			regfree(&nr);
			break;
		}

		genarg(r);

		if(!isaddr(l)) {
			reg(&nr, builtype[TIND], ZeroN);
			genaddr(l, &nr);
			if(nr.type != OREGISTER)
				fatal("genexp: call %N", &nr);

			nr.type = OINDREG;
			nr.ival = 0;
			docall(&nr);
			regfree(&nr);
		}
		else
			docall(l);

		/* Return value */
		if(lval) {
			regret(&nr, rval->t);
			assign(&nr, lval);
			regfree(&nr);
		}
		break;
	
	}
}

/*
 * Compile assignment operator
 */
void
oasgn(Node *rval, Node *lval)
{
	Node n1, n2;
	Node *l, *r;

	r = rval->right;
	l = rval->left;

	if(opt('q')) {
		print("OASGN r");
		ptree(rval, 0);
		print("OASGN l");
		ptree(lval,0);
		print("*\n");
	}

	if(l->islval >= Sucompute)
	if(l->sun < Sucall) {
		if(lval != ZeroN || r->islval < Sucompute) {
			reg(&n1, r->t, lval);
			genexp(r, &n1);
			assign(&n1, l);
			regfree(&n1);
		} else
			assign(r, l);
		return;
	}

	if(l->sun >= r->sun) {
		rgenaddr(&n2, l, ZeroN);
		if(r->islval >= Sucompute) {
			assign(r, &n2);
			assign(r, lval);
			regfree(&n2);
			return;
		}
		reg(&n1, r->t, lval);
		genexp(r, &n1);
	}
	else {
		reg(&n1, r->t, lval);
		genexp(r, &n1);
		rgenaddr(&n2, l, ZeroN);
	}
	assign(&n1, &n2);
	regfree(&n1);
	regfree(&n2);
}

void
evalarg(Node *n, int pass)
{
	Node *tmp, n1;

	if(n == ZeroN)
		return;

	n1.srcline = n->srcline;

	switch(n->type) {
	case OLIST:
		evalarg(n->left, pass);
		evalarg(n->right, pass);
		break;

	default:
		switch(pass) {
		case 0:
			if(n->type == OBLOCK) {
				stmnt(n->left);
				*n = *n->right;
				break;
			}
			if(n->sun < Sucall)
				break;

			tmp = stknode(n->t);
			n1.type = OASGN;
			n1.left = tmp;
			n1.right = n;
			n1.t = n->t;
			sucalc(&n1);
			genexp(&n1, ZeroN);
			*n = *tmp;
			break;

		case 1:
			if(atv)
				tmp = atvnode(n->t);
			else
				tmp = argnode(n->t);
		compute:
			switch(n->t->type) {
			default:
				fatal("evalarg %T",  n->t);
			case TADT:
			case TUNION:
			case TAGGREGATE:
				gencomplex(n, tmp);
				break;

			case TSINT:
			case TSUINT:
			case TCHAR:
			case TINT:
			case TUINT:
			case TIND:
			case TFLOAT:
			case TCHANNEL:
				n1.type = OASGN;
				n1.left = tmp;
				n1.right = n;
				n1.t = n->t;
				sucalc(&n1);
				genexp(&n1, ZeroN);
				break;
			}
			break;
		case 3:
			/*
			 * Second pass becoming somebody else
			 */
			tmp = atvnode(n->t);
			if(n->type == ONAME)
			if(n->ti->class == Parameter)
			if(n->ti->offset == tmp->left->right->ival)
				break;
			goto compute;
		case 4:
			/*
			 * Second pass becoming myself
			 */
			tmp = paramnode(n->t);
			if(tmp->type == ONAME)
			if(tmp->sym == n->sym)
			if(tmp->ti->class == n->ti->class)
				break;
			goto compute;
		}
	}	
}

/*
 * genarg - Push function arguments onto the stack
 */
void
genarg(Node *n)
{
	evalarg(n, 0);

	args = Argbase;
	if(atv) {
		reg(&ratv, builtype[TIND], 0);
		assign(atv, &ratv);
		ratv.type = OINDREG;
		ratv.ival = Argbase;
		evalarg(n, 1);
		return;	
	}

	evalarg(n, 1);
	if(args > maxframe)
		maxframe = args;
}

/*
 * Compute the frame size of function arguments
 */
void
framesize(Node *n)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	case OLIST:
		framesize(n->left);
		framesize(n->right);
		break;

	default:
		frsize = align(frsize, builtype[TINT]);
		frsize += n->t->size;
	}
}

void
setcond(Node *lval)
{
	Inst *true, *false;

	if(lval == ZeroN)
		return;

	true = ipc;
	assign(con(1), lval);
	instruction(AJMP, ZeroN, ZeroN, ZeroN);	
	false = ipc;
	label(true, ipc->pc+1);
	assign(con(0), lval);
	label(false, ipc->pc+1);
}

void
andand(Node *l, Node *r, int bool)
{
	Inst *true, *false;

	if(l->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
	else
		gencond(l, ZeroN, bool);
	
	false = ipc;

	if(r->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
	else
		gencond(r, ZeroN, !bool);

	true = ipc;

	label(false, ipc->pc+1);
	instruction(AJMP, ZeroN, ZeroN, ZeroN);
	label(true, ipc->pc+1);

}

void
oror(Node *l, Node *r, int bool)
{
	Inst *false1, *false2;

	if(l->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
	else
		gencond(l, ZeroN, !bool);
	
	false1 = ipc;

	if(r->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
	else
		gencond(r, ZeroN, !bool);

	false2 = ipc;

	instruction(AJMP, ZeroN, ZeroN, ZeroN);
	label(false1, ipc->pc+1);
	label(false2, ipc->pc+1);
}

/*
 * Generate for boolean conditionals
 */
void
gencond(Node *rval, Node *lval, int bool)
{
	int op;
	Node n1, n2;
	Node *l, *r, *nstack;

	l = rval->left;
	r = rval->right;

	op = rval->type;
	switch(op) {
	default:
		reg(&n1, rval->t, lval);
		genexp(rval, &n1);
		if(bool)
			op = OEQ;
		else
			op = ONEQ;
		codcond(op, &n1, con(0), ZeroN);
		setcond(lval);
		regfree(&n1);
		return;

	case OCAND:
		if(bool == False)
			oror(l, r, bool);
		else
			andand(l, r, bool);
		break;

	case OCOR:
		if(bool == False)
			andand(l, r, bool);
		else
			oror(l, r, bool);
		break;

	case ONOT:
		gencond(rval->left, lval, !bool);
		break;

	case OHI:
	case OLO:
	case OLOS:
	case OHIS:
	case OEQ:
	case OLEQ:
	case ONEQ:
	case OLT:
	case OGEQ:
	case OGT:
		if(r->sun >= Sucall && l->sun >= Sucall) {
			nstack = stknode(l->t);
			genexp(l, nstack);
			rval->left = nstack;
			gencond(rval, lval, bool);
			return;				
		}

		if(bool)
			op = not[op];

		if(immed(l)) {
			switch(op) {
			default:
				if(l->ival != 0)
					break;

			case OGT:
			case OHI:
			case OLEQ:
			case OLOS:
				reg(&n1, r->t, lval);
				genexp(r, &n1);
				codcond(op, l, &n1, ZeroN);
				regfree(&n1);
				setcond(lval);
				return;
			}
		}
		if(immed(r)) {
			switch(op) {
			default:
				if(r->ival != 0)
					break;

			case OGEQ:
			case OHIS:
			case OLT:
			case OLO:
				reg(&n1, l->t, lval);
				genexp(l, &n1);
				codcond(op, &n1, r, ZeroN);
				regfree(&n1);
				setcond(lval);
				return;
			}
		}
		if(l->sun >= r->sun) {
			reg(&n1, l->t, lval);
			genexp(l, &n1);
			reg(&n2, r->t, ZeroN);
			genexp(r, &n2);
		}
		else {
			reg(&n2, r->t, lval);
			genexp(r, &n2);
			reg(&n1, l->t, ZeroN);
			genexp(l, &n1);
		}
		codcond(op, &n1, &n2, ZeroN);
		regfree(&n1);
		regfree(&n2);
	}
	setcond(lval);
}

void
genelist(Node *n)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	case OLIST:
		genelist(n->left);
		genelist(n->right);
		break;
	default:
		genexp(n, ZeroN);
		break;
	}
}

void
gensubreg(Node *n, Node *reg)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		gensubreg(n->left, reg);
		gensubreg(n->right, reg);
		break;
	case OREGISTER:
		*n = *reg;
		break;
	}
}

void
genmove(Node *rval, Type *rt, Node *lval, int o)
{
	Type *t;
	Node *l, *r;

	for(t = rt->next; t; t = t->member) {
		switch(t->type) {
		case TFUNC:
			break;
		case TADT:
		case TAGGREGATE:
			genmove(rval, t, lval, t->offset);
			break;
		default:
			r = an(OADDR, rval, ZeroN);
			r->t = at(TIND, t);
			r = an(OADD, r, con(o+t->offset));
			r->t = r->left->t;
			r = an(OIND, r, ZeroN);
			r->t = t;
			l = an(OADDR, lval, ZeroN);
			l->t = at(TIND, t);
			l = an(OADD, l, con(o+t->offset));
			l->t = l->left->t;
			l = an(OIND, l, ZeroN);
			l->t = t;
			sucalc(r);
			sucalc(l);
			genexp(r, l);
		}
	}
}

int
bitmove(Node *rval, Node *lval)
{
	if(!isaddr(rval))
		return 0;

	if(!isaddr(lval))
		return 0;

	genmove(rval, rval->t, lval, 0);
	return 1;
}

/*
 * generate moves for complex types
 */
void
gencomplex(Node *rval, Node *lval)
{
	Type *t;
	Inst *back;
	Node *l, *r, *tmp, *ptr;
	Node n1, n2, treg, loop;
	int size, w, i, o;

	if(opt('x')) {
		print("GENCOM r\n");
		ptree(rval, 0);
		print("*\nGENCOM l\n");
		ptree(lval, 0);
	}

	r = rval->right;
	l = rval->left;

	switch(rval->type) {
	case OBLOCK:
		bstmnt(l);
		gencomplex(r, lval);
		break;

	case ODOT:
		tmp = stknode(l->t);
		genexp(l, tmp);
		if(rval == ZeroN)
			break;

		tmp->ival = r->ival;
		tmp->t = rval->t;
		genexp(tmp, lval);
		break;

	case OASGN:
		if(l->type == OILIST) {
			if(!isaddr(r) || lval != 0) {
				tmp = stknode(r->t);
				gencomplex(r, tmp);
			}
			else
				tmp = r;

			reg(&treg, builtype[TIND], ZeroN);
			genaddr(tmp, &treg);
			ptr = stknode(builtype[TIND]);
			assign(&treg, ptr);
			regfree(&treg);

			gensubreg(l->left, ptr);
			genelist(l->left);
			if(lval)
				gencomplex(tmp, lval);
			break;
		}
		if(lval == ZeroN) {
			if(rval->islval < Sucompute)
				gencomplex(r, l);
			break;
		}
		tmp = stknode(rval->t);
		gencomplex(r, tmp);
		gencomplex(tmp, l);
		gencomplex(tmp, lval);
		break;

	case ORECV:
	case OCALL:
		if(lval == ZeroN) {
			tmp = stknode(rval->t);
			gencomplex(rval, tmp);
			break;
		}
		if(l->sun >= Sucall) {
			reg(&treg, builtype[TIND], ZeroN);
			genaddr(l, &treg);
			if(treg.type != OREGISTER)
				fatal("gencomplex: call %N", &treg);

			l = stknode(builtype[TIND]);
			l = an(OIND, l, ZeroN);
			l->t = builtype[TIND];
			assign(&treg, l);
			regfree(&treg);
		}
		genarg(r);

		t = at(TIND, lval->t);
		regret(&n1, t);
		if(isaddr(lval)) {
			lval = an(OADDR, lval, ZeroN);
			lval->t = t;
			genexp(lval, &n1);
			tmp = ZeroN;
		}
		else {
			tmp = stknode(lval->t);
			n2.srcline = lval->srcline;
			n2.type = OADDR;
			n2.ival = 0;
			n2.t = t;
			n2.left = tmp;
			n2.right = ZeroN;
			genexp(&n2, &n1);
		}

		if(!isaddr(l)) {
			reg(&treg, builtype[TIND], ZeroN);
			genaddr(l, &treg);
			if(treg.type != OREGISTER)
				fatal("gencomplex: call %N", &treg);

			treg.type = OINDREG;
			treg.ival = 0;
			docall(&treg);
			regfree(&treg);
		}
		else
			docall(l);

		regfree(&n1);

		if(tmp)
			gencomplex(tmp, lval);

		break;

	case OILIST:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}

		t = lval->t;
		lval->t = builtype[TINT];
		rgenaddr(&n2, lval, ZeroN);
		if(t->type == TPOLY)
			lval->t = polyshape;
		else
			lval->t = t;
		o = n2.ival;
		n2.ival = 0;
		cominit(&n2, lval->t, rval, o);
		regfree(&n2);
		break;

	default:
		if(lval == ZeroN) {
			warn(rval, "result ignored");
			break;
		}
	
		size = rval->t->size;
		w = builtype[TINT]->size;
		size /= w;
		if(size < 6 && notunion(rval->t) && bitmove(rval, lval))
			break;

		if(rval->sun > lval->sun) {
			t = rval->t;
			rval->t = builtype[TINT];
			rgenaddr(&n1, rval, ZeroN);
			rval->t = t;
	
			t = lval->t;
			lval->t = builtype[TINT];
			rgenaddr(&n2, lval, ZeroN);
			lval->t = t;
		}
		else {
			t = lval->t;
			lval->t = builtype[TINT];
			rgenaddr(&n2, lval, ZeroN);
			lval->t = t;
	
			t = rval->t;
			rval->t = builtype[TINT];
			rgenaddr(&n1, rval, ZeroN);
			rval->t = t;
		}

		reg(&treg, builtype[TINT], ZeroN);
		if(size < 6) {
			for(i = 0; i < size; i++) {
				instruction(AMOVW, &n1, ZeroN, &treg);
				instruction(AMOVW, &treg, ZeroN, &n2);
				n1.ival += w;
				n2.ival += w;
			}
		}
		else {
			reg(&loop, builtype[TINT], ZeroN);
			instruction(AMOVW, con(size), ZeroN, &loop);
			instruction(AMOVW, &n1, ZeroN, &treg);
			back = ipc;
			instruction(AMOVW, &treg, ZeroN, &n2);
			n1.type = OREGISTER;
			n2.type = OREGISTER;
			instruction(AADDU, con(w), ZeroN, &n1);
			instruction(AADDU, con(w), ZeroN, &n2);
			instruction(ASUBU, con(1), ZeroN, &loop);
			instruction(ABNE, regn(0),  &loop, ZeroN);
			label(ipc, back->pc);
			regfree(&loop);
		}
		regfree(&treg);
		regfree(&n1);
		regfree(&n2);
	}
}
