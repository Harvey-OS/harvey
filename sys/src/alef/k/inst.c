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
		switch(s2->type) {
		case OREGISTER:
			break;
		case OCONST:
			if(s2->ival == 0) {
				s2->reg = 0;
				break;
			}
			/* Fall */
		default:
			fatal("inst %s %N,%N,%N", itab[op], s1, s2, dst);
		}
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
	case ABA:
	case ABCC:
	case ABCS:
	case ABE:
	case ABG:
	case ABGE:
	case ABGU:
	case ABL:
	case ABLE:
	case ABLEU:
	case ABN:
	case ABNE:
	case ABNEG:
	case ABPOS:
	case ABVC:
	case ABVS:
	case AJMPL:
	case AJMP:
	case AFBA:
	case AFBE:
	case AFBG:
	case AFBGE:
	case AFBL:
	case AFBLE:
	case AFBLG:
	case AFBN:
	case AFBNE:
	case AFBO:
	case AFBU:
	case AFBUE:
	case AFBUG:
	case AFBUGE:
	case AFBUL:
	case AFBULE:
		break;

	default:
		fatal("label not branch");
	}
	a = &i->dst;
	a->type = A_BRANCH;
	a->ival = pc;
}

/* Select code for arithmetic operations */
void
codmop(Node *o, Node *l, Node *r, Node *dst)
{
	int op;

	op = AGOK;
	switch(o->type) {
	case OADD:
	case OADDEQ:
		switch(o->t->type) {
		default:
			op = AADD;
			break;
		case TFLOAT:
			op = AFADDD;
			break;
		}
		break;

	case OSUB:
	case OSUBEQ:
		switch(o->t->type) {
		default:
			op = ASUB;
			break;
		case TFLOAT:
			op = AFSUBD;
			break;
		}
		break;

	case OMUL:
	case OMULEQ:
		switch(o->t->type) {
		default:
			op = AMUL;
			break;
		case TFLOAT:
			op = AFMULD;
			break;
		}
		break;

	case ODIV:
	case ODIVEQ:	
		switch(o->t->type) {
		default:
			op = ADIV;
			break;
		case TUINT:
		case TSUINT:
			op = ADIVL;
			break;
		case TFLOAT:
			op = AFDIVD;
			break;
		}
		break;

	case OMOD:
	case OMODEQ:
		switch(o->t->type) {
		default:
			op = AMOD;
			break;
		case TUINT:
		case TSUINT:
			op = AMODL;
			break;
		}
		break;

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
mulcon(Node *n, Node *nn)
{
	int o;
	long v;
	Multab *m;
	Node *l, *r, nod1, nod2, mop;
	char code[sizeof(m->code)+2], *p;

	if(n->t->type == TFLOAT)
		return 0;

	mop.t = n->t;
	l = n->left;
	r = n->right;
	if(l->type == OCONST) {
		l = r;
		r = n->left;
	}
	if(r->type != OCONST)
		return 0;

	v = r->ival;
	m = mulcon0(v);
	if(!m) {
		if(opt('M'))
			print("%L multiply table: %lld\n", r->ival);
		return 0;
	}
	if(opt('M'))
		print("%L multiply: %ld\n", v);

	memmove(code, m->code, sizeof(m->code));
	code[sizeof(m->code)] = 0;

	p = code;
	if(p[1] == 'i')
		p += 2;
	reg(&nod1, n->t, nn);
	genexp(l, &nod1);
	if(v < 0) {
		mop.type = OSUB;
		codmop(&mop, &nod1, con(0), &nod1);
	}
	reg(&nod2, n->t, ZeroN);

	for(;;) {
		switch(*p) {
		case '\0':
			regfree(&nod2);
			assign(&nod1, nn);
			regfree(&nod1);
			return 1;
		case '+':
			o = OADD;
			goto addsub;
		case '-':
			o = OSUB;
		addsub:	/* number is r,n,l */
			v = p[1] - '0';
			r = &nod1;
			if(v&4)
				r = &nod2;
			n = &nod1;
			if(v&2)
				n = &nod2;
			l = &nod1;
			if(v&1)
				l = &nod2;
			mop.type = o;
			codmop(&mop, l, n, r);
			break;
		default: /* op is shiftcount, number is r,l */
			v = p[1] - '0';
			r = &nod1;
			if(v&2)
				r = &nod2;
			l = &nod1;
			if(v&1)
				l = &nod2;
			v = *p - 'a';
			if(v < 0 || v >= 32) {
				diag(n, "mulcon unknown op: %c%c", p[0], p[1]);
				break;
			}
			mop.type = OALSH;
			codmop(&mop, con(v), l, r);
			break;
		}
		p += 2;
	}
	return 0;
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
codcond(int comp, Node *src1, Node *src2)
{
	int a, t;

	if(src1->type == OCONST)
	if(src1->ival == 0)
	if(src1->t->type != TFLOAT)
		src1 = regn(0);

	if(src2->type == OCONST)
	if(src2->ival == 0)
	if(src2->t->type != TFLOAT)
		src2 = regn(0);

	t = src1->t->type;
	switch(comp) {
	default:
		fatal("codcond: %N %s %N", src1, treeop[comp]+1, src2);

	case OEQ:
		a = ABE;
		if(t == TFLOAT)
			a = AFBE;
		break;

	case ONEQ:
		a = ABNE;
		if(t == TFLOAT)
			a = AFBNE;
		break;

	case OLT:
		a = ABL;
		if(t == TFLOAT)
			a = AFBL;
		break;
		
	case OGT:
		a = ABG;
		if(t == TFLOAT)
			a = AFBG;
		break;

	case OLEQ:
		a = ABLE;
		if(t == TFLOAT)
			a = AFBLE;
		break;

	case OGEQ:
		a = ABGE;
		if(t == TFLOAT)
			a = AFBGE;
		break;

	case OLO:
		a = ABCS;
		break;

	case OHI:
		a = ABGU;
		break;

	case OLOS:
		a = ABLEU;
		break;

	case OHIS:
		a = ABCC;
		break;
	}

	if(t == TFLOAT)
		instruction(AFCMPD,src1, ZeroN, src2);
	else
		instruction(ACMP, src1, ZeroN, src2);

	instruction(a, ZeroN, ZeroN, ZeroN);
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
	long o;
	Tinfo *ti;

	a->sym = ZeroS;
	a->reg = Nreg;

	switch(n->type) {
	default:
		fatal("mkaddr: %N\n", n);

	case ONAME:
		ti = n->ti;

		a->type = A_INDREG;
		a->etype = ti->t->type;
		a->ival = ti->offset;
		a->sym = n->sym;
		if(ti->class == Argument) {
			a->reg = RegSP;
			a->class = A_NONE;
			break;
		}
		a->class = ti->class;
		break;

	case OCONST:
		switch(n->t->type) {
		default:
			if(ur0 && n->ival == 0) {
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

	case OADD:
		if(n->left->type == OCONST) {
			mkaddr(n->left, a, 0);
			o = a->ival;
			mkaddr(n->right, a, 1);
		}
		else {
			mkaddr(n->right, a, 0);
			o = a->ival;
			mkaddr(n->left, a, 1);
		}
		a->ival += o;
		break;

	case OINDREG:
		a->type = A_INDREG;
		a->reg = n->reg;
		a->ival = n->ival;
		break;

	case OREGISTER:
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
	i->lineno = name->srcline;

	n = an(0, ZeroN, ZeroN);
	*n = *name;
	n->type = ONAME;
	mkaddr(n, &i->src1, 0);
	mkaddr(con(0), &i->dst, 0);
	ilink(i);
	funentry = i;		/* To back patch arg space */
	becomentry = i;
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
