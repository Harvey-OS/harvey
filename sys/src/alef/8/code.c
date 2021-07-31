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

		TXXX  	TINT	TUINT	TSINT	TSUINT	TCHAR  TFLOAT
		TIND	TCHAN	TARY	TAGG	TUNI	TFUNC	TVOID
*/
int ostore[] ={ AGOK, 	AMOVL, 	AMOVL, 	AMOVW, 	AMOVW, 	AMOVB, 	AFMOVDP,
		AMOVL,  AMOVL,  AGOK,  	AGOK,  	AGOK,  	AGOK,  	AGOK };

int oload[] = { AGOK, 	AMOVL, 	AMOVL, 	AMOVWLSX,AMOVWLZX,AMOVBLZX,AFMOVD,
		AMOVL,	AMOVL,	AGOK,  	AGOK,  	AGOK,  	AGOK,  	AGOK };

void
load(Node *from, Node *to)
{
	Node n1;
	int op;

	op = from->t->type;
	switch(op) {
	case TFLOAT:
		instruction(oload[op], from, to);
		break;
	default:
		reg(&n1, from->t, to);
		instruction(oload[op], from, &n1);
		assign(&n1, to);
		regfree(&n1);
	}
}

void
store(Node *from, Node *to)
{
	Node n1, *tmp;
	int op, t;

	op = ostore[to->t->type];

	t = from->t->type;
	switch(t) {
	case TFLOAT:
		if(to->t->type == TFLOAT) {
			instruction(op, from, to);
			break;
		}
		tmp = stknode(builtype[TINT]);
		instruction(AFMOVLP, from, tmp);
		assign(tmp, to);
		break;
	default:
		if(vconst(from) == 0) {
			instruction(op, from, to);
			return;
		}
		
		if(t == to->t->type)
			reg(&n1, to->t, from);
		else
			reg(&n1, to->t, ZeroN);

		assign(from, &n1);
		instruction(op, &n1, to);
		regfree(&n1);
	}
}

/*
 * Compile code for moves
 */
void
assign(Node *src, Node *dst)
{
	Node *tmp;
	int op, dt;
	Inst *nofix;

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
			op = AMOVL;
			break;

		case TFLOAT:
			tmp = stknode(builtype[TINT]);
			assign(src, tmp);
			instruction(AFMOVL, tmp, F0);
			if(src->t->type != TUINT)
				return;
			instruction(ACMPL, src, con(0));
			instruction(AJGE, ZeroN, ZeroN);
			nofix = ipc;
			instruction(AFADDD, conf(4294967296.), F0);
			label(nofix, ipc->pc+1);
			return;
		}
		break;

	case TSUINT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVWLZX;
			break;

		case TSUINT:
		case TCHAR:
		case TSINT:
			op = AMOVL;
			break;

		case TFLOAT:
			tmp = stknode(builtype[TINT]);
			assign(src, tmp);
			instruction(AFMOVL, tmp, F0);
			return;
		}
		break;

	case TSINT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVW;
			break;

		case TSUINT:
		case TCHAR:
		case TSINT:
			op = AMOVL;
			break;

		case TFLOAT:
			tmp = stknode(builtype[TINT]);
			assign(src, tmp);
			instruction(AFMOVL, tmp, F0);
			return;
		}
		break;

	case TFLOAT:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
		case TCHAR:
		case TSINT:
		case TSUINT:
			tmp = stknode(builtype[TINT]);
			instruction(AFMOVLP, src, tmp);
			assign(tmp, dst);
			return;

		case TFLOAT:
			op = AFMOVD;
			break;
		}
		break;
	case TCHAR:
		switch(dt) {
		case TINT:
		case TUINT:
		case TIND:
			op = AMOVBLZX;
			break;

		case TSUINT:
		case TSINT:
		case TCHAR:
			op = AMOVL;
			break;

		case TFLOAT:
			tmp = stknode(builtype[TINT]);
			assign(src, tmp);
			instruction(AFMOVL, tmp, F0);
			return;
		}
		break;
	}
	instruction(op, src, dst);
}

/*
 * This routine makes anything addressable
 */
void
genaddr(Node *expr, Node *dst)
{
	Node n, *tmp;

	if(opt('g')) {
		print("genaddr:\n");
		ptree(expr, 0);
	}

	switch(expr->type) {
	case OIND:
		genexp(expr->left, dst);
		return;
	default:
		if(!isaddr(expr)) {
			tmp = stknode(expr->t);
			n.type = OASGN;
			n.t = tmp->t;
			n.left = tmp;
			n.right = expr;
			n.islval = 0;
			genexp(&n, ZeroN);
			expr = tmp;
		}
		switch(dst->type) {
		case OINDREG:
		case OREGISTER:
			instruction(ALEAL, expr, dst);
			break;
		default:
			reg(&n, builtype[TIND], ZeroN);
			instruction(ALEAL, expr, &n);
			assign(&n, dst);
			regfree(&n);
			break;
		}
	}
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

void
toaddr(Node *l, Node *dst)
{
	if(isaddr(l)) {
		*dst = *l;
		return;
	}

	rgenaddr(dst, l, ZeroN);
}

/*
 * pre and post increment generation
 */
void
postop(Node *rval, Node *lval)
{
	Node *l, *nconst, n1, nop;

	l = rval->left;
	nconst = con(1);
	if(l->t->type == TIND)
		nconst->ival = l->t->next->size;

	if(rval->type == OPDEC)
		nconst->ival = -nconst->ival;

	toaddr(l, &n1);
	nop.type = OADD;
	nop.t = builtype[TINT];
	assign(&n1, lval);
	codmop(&nop, nconst, &n1, &n1);
	if(!isaddr(l))
		regfree(&n1);
}

void
preop(Node *rval, Node *lval)
{
	Node *l, *nconst, n1, nop;

	l = rval->left;
	nconst = con(1);
	if(l->t->type == TIND)
		nconst->ival = l->t->next->size;

	if(rval->type == OEDEC)
		nconst->ival = -nconst->ival;

	toaddr(l, &n1);
	nop.type = OADD;
	nop.t = builtype[TINT];
	codmop(&nop, nconst, &n1, &n1);
	assign(&n1, lval);
	if(!isaddr(l))
		regfree(&n1);
}

void
docall(Node *n)
{
	Node *rsp;

	if(atv == ZeroN) {
		instruction(ACALL, ZeroN, n);
		return;
	}

	/*
	 * if calling through an activation, link
	 * to the new stack
	 */
	ratv.ival = atv->atvsafe;
	rsp = regn(RegSP);
	instruction(AMOVL, rsp, &ratv);
	ratv.type = OREGISTER;
	assign(&ratv, rsp);
	instruction(ACALL, ZeroN, n);
	rsp->type = OINDREG;
	rsp->ival = atv->atvsafe;
	instruction(AMOVL, rsp, regn(RegSP));
	regfree(&ratv);
}

/*
 * make reg free so we can use it as an implied register for things
 * like IDIV and SALL
 */
int
need(int reg, Node *rnod, Node *rval, Node *lval)
{
	int ref;
	Node *safe, *r, *l, n;

	l = rval->left;
	r = rval->right;

	if(needreg(reg, rnod, lval) == 0)
		return 0;

	safe = stknode(builtype[TINT]);
	assign(rnod, safe);
	ref = regsave(reg);
	/*
	 * If the destination is the safe register generate an
	 * assign into the safe stack location and let the optimiser
	 * figure the redundant moves
	 */
	if(l->type == OREGISTER && l->reg == reg) {
		n = *rval;
		n.left = safe;
		genexp(&n, lval);
	}
	else
	if(r->type == OREGISTER && r->reg == reg) {
		n = *rval;
		n.right = safe;
		genexp(&n, lval);
	}
	else
		genexp(rval, lval);
	assign(safe, regn(reg));
	regrest(reg, ref);
	return 1;
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
		instruction(AJMP, ZeroN, ZeroN);
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
		instruction(ARET, con(ratv.ival), &nl);
		regfree(&nl);
	}
	else
		instruction(ARET, con(ratv.ival), l);
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
		print("%L (R) %N (L) %N\n", rval->srcline, rval, lval);

	if(rval->t) {
		switch(rval->t->type) {
		case TADT:
		case TUNION:
		case TAGGREGATE:
			gencomplex(rval, lval);
			return;
		}
	}

	/*
	 * addressable
	 */
	if(isaddr(rval)) {
		assign(rval, lval);
		return;
	}

	/*
	 * Follow function call evaluations into stack temporary
	 */
	if(l->sun >= Sucall && r && r->sun >= Sucall) {
		switch(rval->type) {
		case OFUNC:
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

		if(convisnop(l->t, rval->t) && convisnop(rval->t, lval->t)) {
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
		if(lval == ZeroN)
			break;

		gencond(rval, lval, True);
		break;

	case ONOT:
		gencond(l, lval, False);
		break;

	case ORSHEQ:
	case OLSHEQ:
		/*
		 * Peel off the instructions with immediate operands
		 */
		while(l->type == OCONV)
			l = l->left;

		if(r->type == OCONST) {
			toaddr(l, &nspare);
			reg(&nl, r->t, lval);
			assign(&nspare, &nl);
			codmop(rval, r, ZeroN, &nl);
			assign(&nl, &nspare);
	
			regfree(&nl);
			if(!isaddr(l))
				regfree(&nspare);
			break;
		}
	
		/*
		 * Clear out CX
		 * set to nspare
		 */
		if(need(D_CX, &nspare, rval, lval))
			break;

		regrest(D_CX, Rinc);
		if(l->sun >= r->sun) {
			toaddr(l, &nl);
			genexp(r, &nspare);
		}
		else {
			genexp(r, &nspare);
			toaddr(l, &nl);
		}
		codmop(rval, &nspare, ZeroN, &nl);
		regfree(&nspare);
		assign(&nl, lval);
		if(!isaddr(l))
			regfree(&nl);
		break;

	case OADDEQ:
	case OSUBEQ:
	case OANDEQ:
	case OOREQ:
	case OXOREQ:
		if(rval->t->type == TFLOAT) {
			genfexp(rval, lval);
			return;
		}

		while(l->type == OCONV)
			l = l->left;

		/* Peel off the instructions with immediate operands */
		if(r->type == OCONST) {
			toaddr(l, &nspare);
			reg(&nl, r->t, lval);
			assign(&nspare, &nl);
			codmop(rval, r, &nl, &nl);
			assign(&nl, &nspare);
	
			regfree(&nl);
			if(!isaddr(l))
				regfree(&nspare);
			break;
		}

		if(l->sun >= r->sun) {
			toaddr(l, &nspare);
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
		}
		else {
			reg(&nr, r->t, ZeroN);
			genexp(r, &nr);
			toaddr(l, &nspare);
		}

		reg(&nl, rval->t, lval);
		assign(&nspare, &nl);
		codmop(rval, &nr, ZeroN, &nl);
		regfree(&nr);
		assign(&nl, &nspare);
		if(!isaddr(l))
			regfree(&nspare);
		assign(&nl, lval);
		regfree(&nl);
		break;
		
	case OMULEQ:
	case ODIVEQ:
	case OMODEQ:
		if(rval->t->type == TFLOAT) {
			genfexp(rval, lval);
			return;
		}

		while(l->type == OCONV)
			l = l->left;

		/* Ops are EDX:EDA = EDX:EDA OP mod/rm
		 * Clear out AX
		 * set to nl
		 */
		if(need(D_AX, &nl, rval, lval))
			break;
		/*
		 * Clear out DX
		 * set to nr
		 */
		if(need(D_DX, &nr, rval, lval))
			break;

		regrest(D_AX, Rinc);
		regrest(D_DX, Rinc);

		if(l->sun >= r->sun) {
			toaddr(l, &nspare);
			genexp(&nspare, &nl);
			if(r->type == OCONST || !isaddr(r)) {
				nstack = stknode(builtype[TINT]);
				genexp(r, nstack);
				r = nstack;
			}
			codmop(rval, r, ZeroN, ZeroN);
		}
		else {
			nstack = stknode(builtype[TINT]);
			genexp(r, nstack);
			toaddr(l, &nspare);
			genexp(&nspare, &nl);
			codmop(rval, nstack, ZeroN, ZeroN);
		}
		r = &nl;			/* Res is D_AX */
		if(rval->type == OMODEQ) 	/* Res is D_DX */
			r = &nr;
		assign(r, &nspare);
		assign(&nspare, lval);
		if(!isaddr(l))
			regfree(&nspare);
		regfree(&nl);
		regfree(&nr);
		break;

	case OMUL:
	case ODIV:
	case OMOD:
		if(lval == ZeroN)
			break;

		if(rval->t->type == TFLOAT) {
			genfexp(rval, lval);
			return;
		}

		/* Ops are EDX:EDA = EDX:EDA OP mod/rm
		 * Clear out AX
		 * set to nl
		 */
		if(need(D_AX, &nl, rval, lval))
			break;
		/*
		 * Clear out DX
		 * set to nr
		 */
		if(need(D_DX, &nr, rval, lval))
			break;

		regrest(D_AX, Rinc);
		if(l->sun >= r->sun) {
			genexp(l, &nl);
			regrest(D_DX, Rinc);	/* Hold DX in use */
			if(r->type == OCONST || !isaddr(r)) {
				nstack = stknode(builtype[TINT]);
				genexp(r, nstack);
				r = nstack;
			}
			codmop(rval, r, ZeroN, ZeroN);
		}
		else {
			nstack = stknode(builtype[TINT]);
			genexp(r, nstack);
			genexp(l, &nl);
			regrest(D_DX, Rinc);	/* Hold DX in use */
			codmop(rval, nstack, ZeroN, ZeroN);
		}
		if(rval->type == OMOD) 		/* Res is D_DX */
			assign(&nr, lval);
		else 				/* Res is D_AX */
			assign(&nl, lval);
		regfree(&nl);
		regfree(&nr);
		break;

	case OLSH:
	case ORSH:
	case OALSH:
	case OARSH:
		/*
		 * Shift is either $imm, mod/rm or CX, mod/rm
		 */
		if(lval == ZeroN)
			break;

		/*
		 * Peel off the instructions with immediate operands
		 */
		if(r->type == OCONST) {
			genexp(l, lval);
			if(r->ival == 0 && rval->type != OLAND)
				break;
			codmop(rval, r, lval, lval);
			break;
		}
		
		/*
		 * Clear out CX
		 * set to nspare
		 */
		if(need(D_CX, &nspare, rval, lval))
			break;

		regrest(D_CX, Rinc);

		/* Dont tread on the allocated register if it was
		 * passed down
		 */
		if(lval->type == OREGISTER && lval->reg == D_CX)
			reg(&nl, l->t, ZeroN);
		else
			reg(&nl, l->t, lval);

		if(l->sun >= r->sun) {
			genexp(l, &nl);
			genexp(r, &nspare);
		}
		else {
			genexp(r, &nspare);
			genexp(l, &nl);
		}
		codmop(rval, &nspare, ZeroN, &nl);
		regfree(&nspare);
		assign(&nl, lval);
		regfree(&nl);
		break;

	case OADD:
	case OSUB:
	case OLAND:
	case OLOR:
	case OXOR:
		if(lval == ZeroN)
			break;

		if(rval->t->type == TFLOAT) {
			genfexp(rval, lval);
			return;
		}

		/* Peel off the instructions with immediate operands */
		if(r->type == OCONST) {
			genexp(l, lval);
			if(r->ival == 0 && rval->type != OLAND)
				break;
			codmop(rval, r, lval, lval);
			break;
		}
		
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
			nr.ival = 0;
			docall(&nr);
			regfree(&nr);
		}
		else
			docall(l);

		if(lval == 0) {
			/*
			 * Balance the floating stack by poping
			 * the return value
			 */
			if(rval->t->type == TFLOAT)
				instruction(AFMOVDP, F0, F0);
			break;
		}

		/* Return value */
		regret(&nr, rval->t);
		assign(&nr, lval);
		regfree(&nr);
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

	if(opt('m')) {
		print("OASGN r\n");
		ptree(rval, 0);
		print("OASGN l\n");
		ptree(lval,0);
		print("*\n");
	}

	if(rval->t->type == TFLOAT) {
		genexp(r, F0);
		if(lval != ZeroN)
			instruction(AFMOVD, F0, F0);
		if(isaddr(l))
			assign(F0, l);
		else {
			rgenaddr(&n1, l, ZeroN);
			assign(F0, &n1);
			regfree(&n1);
		}
		if(lval != ZeroN)
			assign(F0, lval);
		return;
	}

	if(l->islval >= Sucompute && l->sun < Sucall) {
		if(lval != ZeroN || r->islval < Sucompute) {
			reg(&n1, r->t, lval);
			genexp(r, &n1);
			assign(&n1, l);
			regfree(&n1);
			return;
		}
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
	Node rp;

	evalarg(n, 0);

	args = Argbase;
	if(atv) {
		if(needreg(D_DI, &rp, ZeroN))
			fatal("genarg: can't free ratv D_DI");

		reg(&ratv, builtype[TIND], &rp);
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
	instruction(AJMP, ZeroN, ZeroN);	
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
		instruction(AJMP, ZeroN, ZeroN);
	else
		gencond(l, ZeroN, bool);
	
	false = ipc;

	if(r->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN);
	else
		gencond(r, ZeroN, !bool);

	true = ipc;

	label(false, ipc->pc+1);
	instruction(AJMP, ZeroN, ZeroN);
	label(true, ipc->pc+1);

}

void
oror(Node *l, Node *r, int bool)
{
	Inst *false1, *false2;

	if(l->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN);
	else
		gencond(l, ZeroN, !bool);
	
	false1 = ipc;

	if(r->t == ZeroT)
		instruction(AJMP, ZeroN, ZeroN);
	else
		gencond(r, ZeroN, !bool);

	false2 = ipc;

	instruction(AJMP, ZeroN, ZeroN);
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
		codcond(op, &n1, con(0));
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

		if(l->t->type == TFLOAT) {
			if(l->sun >= r->sun) {
				genexp(l, F0);
				if(isaddr(r))
					condfop(op, r, F0, FPOP);
				else {
					genexp(r, F0);
					op = inverted[op];
					condfop(op, ZeroN, ZeroN, FPOP|FREV);
				}
			}
			else {
				op = inverted[op];
				genexp(r, F0);
				if(isaddr(l))
					condfop(op, l, F0, FPOP);
				else {
					genexp(l, F0);
					op = inverted[op];
					condfop(op, ZeroN, ZeroN, FPOP|FREV);
				}
			}
			break;
		}
		if(immed(r)) {
			if(isaddr(l))
				codcond(op, l, r);
			else {
				reg(&n1, l->t, lval);
				genexp(l, &n1);
				codcond(op, &n1, r);
				regfree(&n1);
			}
			setcond(lval);
			return;
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
		codcond(op, &n1, &n2);
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
	int size, o, rs1, rs2, rs3;
	Node *rs1n, *rs2n, *rs3n;
	Node *tmp, *l, *r, n1, n2, treg, sav1, sav2, sav3, *ptr;

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
			n2.type = OADDR;
			n2.ival = 0;
			n2.islval = 0;
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
		size /= builtype[TINT]->size;
		if(size < 6 && notunion(rval->t) && bitmove(rval, lval))
			break;
		
		rs3 = 0;
		rs1n = ZeroN;
		rs2n = ZeroN;
		rs3n = ZeroN;
		if(rval->sun > lval->sun) {
			if(needreg(D_SI, &sav1, ZeroN))
				rs1n = rpush(D_SI);
			rs1 = regsave(D_SI);

			t = rval->t;
			rval->t = builtype[TINT];
			genaddr(rval, &sav1);
			rval->t = t;
			regrest(D_SI, Rinc);

			if(needreg(D_DI, &sav2, ZeroN))
				rs2n = rpush(D_DI);
			rs2 = regsave(D_DI);

			t = lval->t;
			lval->t = builtype[TINT];
			genaddr(lval, &sav2);
			lval->t = t;
			regrest(D_DI, Rinc);
		}
		else {
			if(needreg(D_DI, &sav1, ZeroN))
				rs1n = rpush(D_DI);
			rs1 = regsave(D_DI);
			t = lval->t;
			lval->t = builtype[TINT];
			genaddr(lval, &sav1);
			lval->t = t;
	
			rs2 = regsave(D_SI);
			if(needreg(D_SI, &sav2, ZeroN))
				rs2n = rpush(D_SI);

			t = rval->t;
			rval->t = builtype[TINT];
			genaddr(rval, &sav2);
			rval->t = t;
		}
	
		if(needreg(D_CX, &sav3, ZeroN)) {
			rs3 = regsave(D_CX);
			rs3n = rpush(D_CX);
		}

		instruction(AMOVL, con(size), regn(D_CX));
		instruction(ACLD, ZeroN, ZeroN);
		instruction(AREP, ZeroN, ZeroN);
		instruction(AMOVSL, ZeroN, ZeroN);

		if(rs1)
			rpop(&sav1, rs1n);
		regrest(sav1.reg, rs1);

		if(rs2)
			rpop(&sav2, rs2n);
		regrest(sav2.reg, rs2);

		if(rs3) {
			regrest(sav3.reg, rs3);
			rpop(&sav3, rs3n);
		}
	}
}
