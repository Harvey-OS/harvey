#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

/* These are machine specific convs which generate no code */
ulong
nopconv[Ntype] =
{
	0,			/* TXXX */
	MINT|MUINT|MIND,	/* TINT */
	MINT|MUINT|MIND,	/* TUINT */
	MSINT|MSUINT,		/* TSINT */
	MSINT|MSUINT,		/* TSUINT */
	MCHAR,			/* TCHAR */
	MFLOAT,			/* TFLOAT */
	MINT|MUINT|MIND,	/* TIND */
	0,			/* TCHANNEL */
	0,			/* TARRAY */
	MAGGREGATE,		/* TAGGREGATE */
	MUNION,			/* TUNION */
	0,			/* TFUNC */
	0,			/* TVOID */
	TADT,			/* TADT */
};

Inst*
ai(void)
{
	Inst *i;

	i = malloc(sizeof(Inst));
	i->src1.type = A_NONE;
	i->dst.type = A_NONE;
	i->reg = Nreg;
	i->next = 0;
	i->lineno = iline;
	return i;
}

/* Emit an assembler instruction */
Inst*
instruction(int op, Node *s1, Node *s2, Node *dst)
{
	Inst *i;

	i = ai();
	i->op = op;
	i->pc = pc++;

	if(s1)
		mkaddr(s1, &i->src1, 1);

	if(s2 && s2 != dst) {
		if(s2->type != OREGISTER)
			fatal("instruction %s %N,%N,%N", itab[op], s1, s2, dst);
		if(s2->t->type == TFLOAT)
			i->reg = s2->reg - Freg;
		else
			i->reg = s2->reg;
	}

	if(dst) {
		mkaddr(dst, &i->dst, 1);
		if(i->dst.type == A_REG)
		if(i->reg == i->dst.reg)
			i->reg = Nreg;
	}
	ilink(i);
	return ipc;
}

void
ilink(Inst *i)
{
	if(opt('c'))
		print("(%d)%i\n", i->lineno, i);

	if(proghead)
		ipc->next = i;
	else
		proghead = i;

	ipc = i;
}

/* Back patch a branch */
void
label(Inst *i, ulong pc)
{
	Adres *a;

	switch(i->op) {
	case ABEQ:
	case ABFPF:
	case ABFPT:
	case ABGEZ:
	case ABGEZAL:
	case ABGTZ:
	case ABLEZ:
	case ABLTZ:
	case ABLTZAL:
	case ABNE:
	case AJAL:
	case AJMP:
		break;

	default:
		fatal("label not branch %i", i);
	}
	a = &i->dst;
	a->type = A_BRANCH;
	a->ival = pc;
}

/* Select code for arithmetic operations */
void
codmop(Node *o, Node *l, Node *r, Node *dst)
{
	Node n1;
	int op;

	op = AGOK;
	switch(o->type) {
	case OADD:
	case OADDEQ:
		switch(o->t->type) {
		default:
			op = AADDU;
			break;
		case TFLOAT:
			op = AADDD;
			break;
		}
		break;

	case OSUB:
	case OSUBEQ:
		switch(o->t->type) {
		default:
			op = ASUBU;
			break;
		case TFLOAT:
			op = ASUBD;
			break;
		}
		break;

	case OMUL:
	case OMULEQ:
		switch(o->t->type) {
		default:
			op = AMUL;
		lowreg:
			n1.type = OREGISTER;
			n1.t = builtype[TINT];
			n1.reg = Loreg;
			instruction(op, l, dst, ZeroN);
			instruction(AMOVW, &n1, ZeroN, dst);
			return;
		case TFLOAT:
			op = AMULD;
			break;
		}
		break;

	case ODIV:
	case ODIVEQ:	
		switch(o->t->type) {
		default:
			op = ADIV;
			goto lowreg;
		case TUINT:
		case TSUINT:
		case TCHAR:
			op = ADIVU;
			goto lowreg;
		case TFLOAT:
			op = ADIVD;
			break;
		}
		break;

	case OMOD:
	case OMODEQ:
		switch(o->t->type) {
		default:
			op = ADIV;
			break;
		case TUINT:
		case TSUINT:
		case TCHAR:
			op = ADIVU;
			break;
		}
		n1.type = OREGISTER;
		n1.t = builtype[TINT];
		n1.reg = Hireg;
		instruction(op, l, dst, ZeroN);
		instruction(AMOVW, &n1, ZeroN, dst);
		return;

	case OALSH:
	case OLSH:
	case OLSHEQ:
		op = ASLL;
		break;

	case ORSH:
	case ORSHEQ:
		op = ASRL;
		break;

	case OXOR:
	case OXOREQ:
		op = AXOR;
		break;

	case OLOR:
	case OOREQ:
		op = AOR;
		break;

	case OLAND:
	case OANDEQ:
		op = AAND;
		break;

	case OARSH:
		op = ASRA;
		break;
	}
	instruction(op, l, r, dst);
}

int
vconst(Node *n)
{
	int i;

	if(n == ZeroN || n->type != OCONST || n->t == ZeroT)
		return -159;

	switch(n->t->type) {
	case TFLOAT:
		i = 100;
		if(n->fval > i || n->fval < -i)
			return -159;
		i = n->fval;
		if(i != n->fval)
			return -159;
		return i;

	case TCHAR:
	case TSINT:
	case TSUINT:
	case TINT:
	case TUINT:
	case TIND:
		i = n->ival;
		if(i != n->ival)
			return -159;
		return i;
	}
}

/* Select code for conditional operators */
void
codcond(int comp, Node *src1, Node *src2, Node *dst)
{
	int a;
	Node *r;

	if(src1->type == OCONST)
	if(src1->ival == 0)
	if(src1->t->type != TFLOAT)
		src1 = regn(0);

	if(src2->type == OCONST)
	if(src2->ival == 0)
	if(src2->t->type != TFLOAT)
		src2 = regn(0);

	a = AGOK;
	switch(comp) {
	default:
		fatal("codcond: %s %N, %N, %N", treeop[comp]+1, src1, src2, dst);

	case OEQ:
		switch(src1->t->type) {
		case TFLOAT:
			break;
		default:
			instruction(ABEQ, src1, src2, dst);
			return;
		}

	case ONEQ:
		switch(src1->t->type) {
		case TFLOAT:
			break;
		default:
			instruction(ABNE, src1, src2, dst);
			return;
		}

	case OLT:
	case OGT:
	case OLEQ:
	case OGEQ:
		switch(src1->t->type) {
		case TFLOAT:
			switch(comp) {
			default:
				a = ACMPGTD;
				break;
			case ONEQ:
			case OEQ:
				a = ACMPEQD;
				break;
			case OLT:
			case OGEQ:
				a = ACMPGED;
				break;
			}
			instruction(a, src1, src2, ZeroN);
			if(comp == OEQ || comp == OGEQ || comp == OGT)
				instruction(ABFPT, ZeroN, ZeroN, ZeroN);
			else
				instruction(ABFPF, ZeroN, ZeroN, ZeroN);
			return;
		default:
			if(vconst(src1) == 0 || vconst(src2) == 0) {
				if(vconst(src1) == 0) {
					comp = inverted[comp];
					src1 = src2;
				}
				switch(comp) {
				case OGT:
					a = ABGTZ;
					break;
				case OLT:
					a = ABLTZ;
					break;
				case OLEQ:
					a = ABLEZ;
					break;
				case OGEQ:
					a = ABGEZ;
					break;
				}
				src2 = ZeroN;
				break;
			}
		}
		USED(a);
		/* Fall through */
	case OLO:
	case OHI:
	case OLOS:
	case OHIS:
		r = regtmp();
		switch(comp) {
		default:
			a = ASGT;
			break;
		case OLO:
		case OLOS:
		case OHIS:
		case OHI:
			a = ASGTU;
			break;
		}
		switch(comp) {
		default:
			instruction(a, src2, src1, r);
			break;
		case OLEQ:
		case OGT:
		case OLOS:
		case OHI:
			instruction(a, src1, src2, r);
			break;
		}
		switch(comp) {
		default:
			a = ABEQ;
			break;
		case OLT:
		case OGT:
		case OLO:
		case OHI:
			a = ABNE;
			break;
		}
		instruction(a, r, ZeroN, ZeroN);
		regfree(r);
		return;
	}
}

void
mkdata(Node *n, int off, int size, Inst *i)
{
	Adres *a;

	if(n->type != ONAME)
		fatal("mkdata %N\n", n);

	a = &i->src1;

	a->reg = Nreg;
	a->ival = off;
	a->sym = n->sym;
	a->type = A_INDREG;
	a->class = n->t->class;
	a->etype = n->t->type;

	i->reg = size;
}

void
mkaddr(Node *n, Adres *a, int ur0)
{
	char class;

	a->sym = ZeroS;
	a->reg = Nreg;

	switch(n->type) {
	default:
		fatal("mkaddr: %N", n);

	case ONAME:
		a->type = A_INDREG;
		a->etype = n->t->type;
		a->ival = n->ti->offset;
		a->sym = n->sym;
		class = n->ti->class;
		if(class == Argument) {
			a->reg = RegSP;
			a->class = A_NONE;
			break;
		}
		a->class = class;
		break;

	case OCONST:
		switch(n->t->type) {
		default:
			if(ur0)
			if(n->ival == 0) {
				a->type = A_REG;
				a->reg = 0;
				break;
			}
			a->type = A_CONST;
			a->ival = n->ival;
			break;
		case TFLOAT:
			a->type = A_FCONST;
			a->fval = n->fval;
			break;
		}
		break;

	case OINDREG:
		a->type = A_INDREG;
		a->reg = n->reg;
		a->ival = n->ival;
		break;

	case OREGISTER:
		if(n->reg == Hireg)
			a->type = A_HI;
		else
		if(n->reg == Loreg)
			a->type = A_LO;
		else
			a->type = A_REG;

		switch(n->t->type) {
		default:
			a->reg = n->reg;
			break;
	
		case TFLOAT:
			a->type = A_FREG;
			if(n->reg == Retfreg)
				a->reg = Retfregno;
			else
				a->reg = n->reg - Freg;
			break;
		}
		break;

	case OIND:
		mkaddr(n->left, a, 1);
		if(a->type == A_REG || a->type == A_CONST)
			a->type = A_INDREG;
		break;

	case OADDR:
		mkaddr(n->left, a, 1);
		a->type = A_CONST;
		a->ival += n->ival;
		break;
	}
}

/* Generate function preamble and assign parameter offsets */
void
preamble(Node *name)
{
	Inst *i;
	Node *n;

	i = ai();
	i->op = ATEXT;
	i->pc = pc++;

	n = an(0, ZeroN, ZeroN);
	*n = *name;
	n->type = ONAME;
	mkaddr(n, &i->src1, 0);
	mkaddr(con(0), &i->dst, 0);
	ilink(i);
	funentry = i;		/* To back patch arg space */
}

/* Output accumulated string constants */
void
strop(void)
{
	String *s;
	int c, l;
	Inst *i;
	char *p;

	for(s = strdat; s; s = s->next) {
		i = ai();
		i->op = AGLOBL;
		mkaddr(&s->n, &i->src1, 0);
		mkaddr(con(s->len), &i->dst, 0);
		ilink(i);

		c = 0;
		p = s->string;
		while(s->len) {
			l = s->len;
			if(l > 8)
				l = 8;

			iline = s->n.srcline;
			i = ai();
			i->op = ADATA;
			i->dst.type = A_STRING;
			memset(i->dst.str, 0, sizeof(i->dst.str));
			memmove(i->dst.str, p, l);
			s->len -= l;
			mkdata(&s->n, c, l, i);
			p += l;
			c += l;
			ilink(i);
		}
	}
}
