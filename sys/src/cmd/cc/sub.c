#include	"cc.h"

Node*
new(int t, Node *l, Node *r)
{
	Node *n;

	ALLOC(n, Node);
	n->op = t;
	n->left = l;
	n->right = r;
	n->lineno = lineno;
	newflag = 1;
	return n;
}

Node*
new1(int o, Node *l, Node *r)
{
	Node *n;

	n = new(o, l, r);
	n->lineno = nearln;
	return n;
}

void
prtree(Node *n, char *s)
{

	print(" == %s ==\n", s);
	prtree1(n, 0, 0);
	print("\n");
}

void
prtree1(Node *n, int d, int f)
{
	int i;

	if(f)
	for(i=0; i<d; i++)
		print("   ");
	if(n == Z) {
		print("Z\n");
		return;
	}
	if(n->op == OLIST) {
		prtree1(n->left, d, 0);
		prtree1(n->right, d, 1);
		return;
	}
	d++;
	print("%O", n->op);
	i = 3;
	switch(n->op)
	{
	case ONAME:
		print(" \"%s\"", n->sym->name);
		print(" %ld", n->offset);
		i = 0;
		break;

	case OINDREG:
		print(" %ld(R%d)", n->offset, n->reg);
		i = 0;
		break;

	case OREGISTER:
		if(n->offset)
			print(" %ld+R%d", n->offset, n->reg);
		else
			print(" R%d", n->reg);
		i = 0;
		break;

	case OSTRING:
		print(" \"%s\"", n->us);
		i = 0;
		break;

	case OLSTRING:
		print(" \"%S\"", n->rs);
		i = 0;
		break;

	case ODOT:
		print(" \"%s\"", n->sym->name);
		break;

	case OCONST:
		if(typefdv[n->type->etype])
			print(" \"%.8e\"", n->ud);
		else
			print(" \"%ld\"", n->offset);
		i = 0;
		break;
	}
	if(n->addable != 0)
		print(" <%d>", n->addable);
	if(n->type != T)
		print(" %T", n->type);
	if(n->complex != 0)
		print(" (%d)", n->complex);
	print("\n");
	if(i & 2)
		prtree1(n->left, d, 1);
	if(i & 1)
		prtree1(n->right, d, 1);
}

Type*
typ(int et, Type *d)
{
	Type *t;

	ALLOC(t, Type);
	t->etype = et;
	t->link = d;
	t->down = T;
	t->sym = S;
	t->width = ewidth[et];
	t->offset = 0;
	t->shift = 0;
	t->nbits = 0;
	return t;
}

int
simplec(long b)
{

	b &= BCLASS;
	switch(b) {
	case 0:
	case BREGISTER:
		return CXXX;
	case BAUTO:
	case BAUTO|BREGISTER:
		return CAUTO;
	case BEXTERN:
		return CEXTERN;
	case BEXTERN|BREGISTER:
		return CEXREG;
	case BSTATIC:
		return CSTATIC;
	case BTYPEDEF:
		return CTYPEDEF;
	}
	diag(Z, "illegal combination of classes %Q", b);
	return CXXX;
}

Type*
simplet(long b)
{

	b &= ~BCLASS;
	switch(b) {
	case BCHAR:
	case BCHAR|BSIGNED:
		return types[TCHAR];

	case BCHAR|BUNSIGNED:
		return types[TUCHAR];

	case BSHORT:
	case BSHORT|BINT:
	case BSHORT|BSIGNED:
	case BSHORT|BINT|BSIGNED:
		return types[TSHORT];

	case BUNSIGNED|BSHORT:
	case BUNSIGNED|BSHORT|BINT:
		return types[TUSHORT];

	case 0:
	case BINT:
	case BINT|BSIGNED:
	case BSIGNED:
		return tint;

	case BUNSIGNED:
	case BUNSIGNED|BINT:
		return tuint;

	case BLONG:
	case BLONG|BINT:
	case BLONG|BSIGNED:
	case BLONG|BINT|BSIGNED:
		return types[TLONG];

	case BUNSIGNED|BLONG:
	case BUNSIGNED|BLONG|BINT:
		return types[TULONG];

	case BVLONG|BLONG:
	case BVLONG|BLONG|BINT:
	case BVLONG|BLONG|BSIGNED:
	case BVLONG|BLONG|BINT|BSIGNED:
		return types[TVLONG];

	case BFLOAT:
		return types[TFLOAT];

	case BDOUBLE:
	case BDOUBLE|BLONG:
	case BFLOAT|BLONG:
		return types[TDOUBLE];

	case BVOID:
		return types[TVOID];
	}

	diag(Z, "illegal combination of types %Q", b);
	return tint;
}

int
stcompat(Node *n, Type *t1, Type *t2, long ttab[])
{
	int i, b;

	i = 0;
	if(t2 != T)
		i = t2->etype;
	b = 1L << i;
	i = 0;
	if(t1 != T)
		i = t1->etype;
	if(b & ttab[i]) {
		if(ttab == tasign)
			if(b == BSTRUCT || b == BUNION)
				if(!sametype(t1, t2))
					return 1;
		if(n->op != OCAST)
		 	if(b == BIND && i == TIND)
				if(!sametype(t1, t2))
					return 1;
		return 0;
	}
	return 1;
}

int
tcompat(Node *n, Type *t1, Type *t2, long ttab[])
{

	if(stcompat(n, t1, t2, ttab)) {
		if(t1 == T)
			diag(n, "incompatible type: \"%T\" for op \"%O\"",
				t2, n->op);
		else
			diag(n, "incompatible types: \"%T\" and \"%T\" for op \"%O\"",
				t1, t2, n->op);
		return 1;
	}
	return 0;
}

void
makedot(Node *n, Type *t, long o)
{
	Node *n1, *n2;

	if(t->nbits) {
		n1 = new(OXXX, Z, Z);
		*n1 = *n;
		n->op = OBIT;
		n->left = n1;
		n->right = Z;
		n->type = t;
		n->addable = n1->left->addable;
		n = n1;
	}
	n->addable = n->left->addable;
	if(n->addable == 0) {
		n1 = new1(OCONST, Z, Z);
		n1->offset = o;
		n1->type = types[TLONG];
		n->right = n1;
		n->type = t;
		return;
	}
	n->left->type = t;
	if(o == 0) {
		*n = *n->left;
		return;
	}
	n->type = t;
	n1 = new1(OCONST, Z, Z);
	n1->offset = o;
	t = typ(TIND, t);
	t->width = types[TIND]->width;
	n1->type = t;

	n2 = new1(OADDR, n->left, Z);
	n2->type = t;

	n1 = new1(OADD, n1, n2);
	n1->type = t;

	n->op = OIND;
	n->left = n1;
	n->right = Z;
}

Type*
dotsearch(Sym *s, Type *t, Node *n)
{
	Type *t1, *xt;

	xt = T;

	/*
	 * look it up by name
	 */
	for(t1 = t; t1 != T; t1 = t1->down)
		if(t1->sym == s) {
			if(xt != T)
				goto ambig;
			xt = t1;
		}
	if(xt != T)
		return xt;

	/*
	 * look it up by type
	 */
	for(t1 = t; t1 != T; t1 = t1->down)
		if(t1->sym == S && typesu[t1->etype])
			if(sametype(s->type, t1)) {
				if(xt != T)
					goto ambig;
				xt = t1;
			}
	if(xt != T)
		return xt;

	/*
	 * look it up in unnamed substructures
	 */
	for(t1 = t; t1 != T; t1 = t1->down)
		if(t1->sym == S && typesu[t1->etype])
			if(dotsearch(s, t1->link, n) != T) {
				if(xt != T)
					goto ambig;
				xt = t1;
			}
	return xt;

ambig:
	diag(n, "ambiguous structure element: %s", s->name);
	return xt;
}

long
dotoffset(Type *st, Type *lt, Node *n)
{
	Type *t;
	Sym *g;
	long o, o1;

	o = -1;
	/*
	 * first try matching at the top level
	 * for matching tag names
	 */
	g = st->tag;
	if(g != S)
		for(t=lt->link; t!=T; t=t->down)
			if(t->sym == S)
				if(g == t->tag) {
					if(o >= 0)
						goto ambig;
					o = t->offset;
				}
	if(o >= 0)
		return o;

	/*
	 * second try matching at the top level
	 * for similar types
	 */
	for(t=lt->link; t!=T; t=t->down)
		if(t->sym == S)
			if(sametype(st, t)) {
				if(o >= 0)
					goto ambig;
				o = t->offset;
			}
	if(o >= 0)
		return o;

	/*
	 * last try matching sub-levels
	 */
	for(t=lt->link; t!=T; t=t->down)
		if(t->sym == S)
		if(typesu[t->etype]) {
			o1 = dotoffset(st, t, n);
			if(o1 >= 0) {
				if(o >= 0)
					goto ambig;
				o = o1 + t->offset;
			}
		}
	return o;

ambig:
	diag(n, "ambiguous unnamed structure element");
	return o;
}

void
typeext(Type *st, Node *l)
{
	Type *lt;
	Node *n1, *n2;
	long o;

	lt = l->type;
	if(lt == T)
		return;
	if(st->etype == TIND && vconst(l) == 0) {
		l->type = st;
		l->offset = 0;
		return;
	}

	/*
	 * extension of C
	 * if assign of struct containing unnamed sub-struct
	 * to type of sub-struct, insert the DOT.
	 * if assign of *struct containing unnamed substruct
	 * to type of *sub-struct, insert the add-offset
	 */
	if(typesu[st->etype] && typesu[lt->etype]) {
		o = dotoffset(st, lt, l);
		if(o >= 0) {
			n1 = new1(OXXX, Z, Z);
			*n1 = *l;
			l->op = ODOT;
			l->left = n1;
			l->right = Z;
			makedot(l, st, o);
		}
		return;
	}
	if(st->etype == TIND && typesu[st->link->etype])
	if(lt->etype == TIND && typesu[lt->link->etype]) {
		o = dotoffset(st->link, lt->link, l);
		if(o >= 0) {
			l->type = st;
			if(o == 0)
				return;
			n1 = new1(OXXX, Z, Z);
			*n1 = *l;
			n2 = new1(OCONST, Z, Z);
			n2->offset = o;
			n2->type = st;
			l->op = OADD;
			l->left = n1;
			l->right = n2;
		}
		return;
	}
}

/*
 * a cast that generates no code
 * (same size move)
 */
int
nocast(Type *t1, Type *t2)
{
	int i, b;

	if(t1->nbits)
		return 0;
	i = 0;
	if(t2 != T)
		i = t2->etype;
	b = 1<<i;
	i = 0;
	if(t1 != T)
		i = t1->etype;
	if(b & ncast[i])
		return 1;
	return 0;
}

/*
 * a cast that has a noop semantic
 * (small to large, convert)
 */
int
nilcast(Type *t1, Type *t2)
{
	int i, b;

	if(t1->nbits)
		return 0;
	i = 0;
	if(t2 != T)
		i = t2->etype;
	b = 1<<i;
	i = 0;
	if(t1 != T)
		i = t1->etype;
	if(b & lcast[i])
		return 1;
	return 0;
}

/*
 * "the usual arithmetic conversions are performed"
 */
void
arith(Node *n, int f)
{
	Type *t1, *t2;
	int i, j, k;
	Node *n1;
	long w;

	t1 = n->left->type;
	if(n->right == Z)
		t2 = t1;
	else
		t2 = n->right->type;
	i = TXXX;
	if(t1 != T)
		i = t1->etype;
	j = TXXX;
	if(t2 != T)
		j = t2->etype;
	k = tab[i][j];
	if(k == TIND) {
		if(i == TIND)
			n->type = t1;
		else
		if(j == TIND)
			n->type = t2;
	} else {
		/* convert up to at least int */
		if(f == 1)
		while(k < tint->etype)
			k += 2;
		n->type = types[k];
	}
	if(n->op == OSUB)
	if(i == TIND && j == TIND) {
		w = n->right->type->link->width;
		if(w < 1)
			goto bad;
		if(w > 1) {
			n1 = new1(OXXX, Z, Z);
			*n1 = *n;
			n->op = ODIV;
			n->left = n1;
			n1 = new1(OCONST, Z, Z);
			n1->offset = n->right->type->link->width;
			n1->type = n->type;
			n->right = n1;
		}
		n->type = types[TLONG];
		return;
	}
	if(!sametype(n->type, n->left->type)) {
		n->left = new1(OCAST, n->left, Z);
		n->left->type = n->type;
		if(n->type->etype == TIND) {
			w = n->type->link->width;
			if(w < 1)
				goto bad;
			if(w > 1) {
				n1 = new1(OCONST, Z, Z);
				n1->offset = w;
				n1->type = n->type;
				n->left = new1(OMUL, n->left, n1);
				n->left->type = n->type;
			}
		}
	}
	if(n->right != Z)
	if(!sametype(n->type, n->right->type)) {
		n->right = new1(OCAST, n->right, Z);
		n->right->type = n->type;
		if(n->type->etype == TIND) {
			w = n->type->link->width;
			if(w < 1)
				goto bad;
			if(w != 1) {
				n1 = new1(OCONST, Z, Z);
				n1->offset = w;
				n1->type = n->type;
				n->right = new1(OMUL, n->right, n1);
				n->right->type = n->type;
			}
		}
	}
	return;
bad:
	diag(n, "illegal pointer operation");
}

int
ovflo(long l, int f)
{
	long v;

	v = castto(l, f);
	if(v != l)
		return 1;
	return 0;
}

int
side(Node *n)
{

loop:
	if(n != Z)
	switch(n->op) {
	case OCAST:
	case ONOT:
	case OADDR:
	case OIND:
		n = n->left;
		goto loop;

	case OCOND:
		if(side(n->left))
			break;
		n = n->right;

	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OGT:
	case OLE:
	case OADD:
	case OSUB:
	case OMUL:
	case OLMUL:
	case ODIV:
	case OLDIV:
	case OLSHR:
	case OASHL:
	case OASHR:
	case OAND:
	case OOR:
	case OXOR:
	case OMOD:
	case OLMOD:
	case OANDAND:
	case OOROR:
	case OCOMMA:
	case ODOT:
		if(side(n->left))
			break;
		n = n->right;
		goto loop;

	case OSIZE:
	case OCONST:
	case OSTRING:
	case OLSTRING:
	case ONAME:
		return 0;
	}
	return 1;
}

int
vconst(Node *n)
{
	int i;

	if(n == Z)
		goto no;
	if(n->op != OCONST)
		goto no;
	if(n->type == T)
		goto no;
	switch(n->type->etype)
	{
	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		i = 100;
		if(n->ud > i || n->ud < -i)
			goto no;
		i = n->ud;
		if(i != n->ud)
			goto no;
		return i;

	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
	case TIND:
		i = n->offset;
		if(i != n->offset)
			goto no;
		return i;
	}
no:
	return -159;	/* first uninteresting constant */
}

/*
 * try to cast a constant down
 * rather than cast a variable up
 * example:
 *	if(c == 'a')
 */
void
relcon(Node *l, Node *r)
{
	long v;

	if(l->op != OCONST)
		return;
	if(r->op != OCAST)
		return;
	if(!nilcast(r->left->type, r->type))
		return;
	v = l->offset;
	switch(r->type->etype) {
	default:
		return;
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
		if(ovflo(v, r->type->etype))
			return;
		break;
	}
	l->type = r->left->type;
	*r = *r->left;
}

int
relindex(int o)
{

	switch(o) {
	default:
		diag(Z, "bad in relindex: %O", o);
	case OEQ: return 0;
	case ONE: return 1;
	case OLE: return 2;
	case OLS: return 3;
	case OLT: return 4;
	case OLO: return 5;
	case OGE: return 6;
	case OHS: return 7;
	case OGT: return 8;
	case OHI: return 9;
	}
}

Node*
invert(Node *n)
{
	Node *i;

	if(n == Z || n->op != OLIST)
		return n;
	i = n;
	for(n = n->left; n != Z; n = n->left) {
		if(n->op != OLIST)
			break;
		i->left = n->right;
		n->right = i;
		i = n;
	}
	i->left = n;
	return i;
}

int
bitno(long b)
{
	int i;

	for(i=0; i<32; i++)
		if(b & (1L<<i))
			return i;
	diag(Z, "bad in bitno");
	return 0;
}

void
diag(Node *n, char *a, ...)
{
	char buf[STRINGSZ];

	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);	/* ugly */
	print("%L %s\n", (n==Z)? nearln: n->lineno, buf);

	if(debug['X'])
		abort();
	if(n != Z)
	if(debug['v'])
		prtree(n, "diagnostic");

	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}

void
warn(Node *n, char *a, ...)
{
	char buf[STRINGSZ];

	if(debug['w']) {
		print("warning: ");
		doprint(buf, buf+sizeof(buf), a, &(&a)[1]);	/* ugly */
		print("%L %s\n", (n==Z)? nearln: n->lineno, buf);

		if(n != Z)
		if(debug['v'])
			prtree(n, "warning");
	}
}

void
yyerror(char *a, ...)
{
	char buf[STRINGSZ];

	/*
	 * hack to intercept message from yaccpar
	 */
	if(strcmp(a, "syntax error") == 0) {
		yyerror("syntax error, last name: %s", symb);
		return;
	}
	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);	/* ugly */
	print("%L %s\n", lineno, buf);
	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}

char*	tnames[] =
{
	"TXXX",
	"CHAR","UCHAR","SHORT","USHORT","LONG","ULONG","VLONG","FLOAT","DOUBLE",
	"IND","FUNC","ARRAY","VOID","STRUCT","UNION","ENUM","FILE","OLD","DOT",
};
char*	qnames[] =
{
	"Q0",
	"CHAR","UCHAR","SHORT","USHORT","LONG","ULONG","VLONG","FLOAT","DOUBLE",
	"Q-IND","UNSIGNED","Q-ARRAY","VOID","SIGNED","INTEGER","Q-ENUM",
	"AUTO","EXTERN","STATIC","TYPEDEF","REGISTER",
};
char*	cnames[] =
{
	"CXXX",
	"AUTO","EXTERN","GLOBL","STATIC","LOCAL","TYPEDEF",
	"PARAM","SELEM","LABEL","EXREG",
	"HELP","MACRO","MACARG","LEXICAL","PREPROC","SUETAG",
	"ENUM",
};
char*	onames[] =
{
	"OXXX",
	"ADD","ADDR","AND","ANDAND","ARRAY","AS","ASADD","ASAND",
	"ASASHL","ASASHR","ASDIV","ASHL","ASHR","ASLDIV","ASLMOD","ASLMUL",
	"ASLSHR","ASMOD","ASMUL","ASOR","ASSUB","ASXOR","BIT","BREAK",
	"CASE","CAST","COMMA","COND","CONST","CONTINUE","DIV","DOT",
	"DOTDOT","DWHILE","ENUM","EQ","FOR","FUNC","GE","GOTO",
	"GT","HI","HS","IF","IND","INDREG","INIT","LABEL",
	"LDIV","LE","LIST","LMOD","LMUL","LO","LS","LSHR",
	"LT","MOD","MUL","NAME","NE","NOT","OR","OROR",
	"POSTDEC","POSTINC","PREDEC","PREINC","PROTO","REGISTER","RETURN","SET",
	"SIZE","STRING","LSTRING","STRUCT","SUB","SWITCH","UNION","USED",
	"WHILE","XOR","END"
};

char	comrel[12] =
{
	ONE, OEQ, OGT, OHI, OGE, OHS, OLT, OLO, OLE, OLS,
};
char	invrel[12] =
{
	OEQ, ONE, OGE, OHS, OGT, OHI, OLE, OLS, OLT, OLO,
};
char	logrel[12] =
{
	OEQ, ONE, OLS, OLS, OLO, OLO, OHS, OHS, OHI, OHI,
};

char	typefdv[XTYPE] =	{ 0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0 };
char	typesu[XTYPE] =		{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0 };
char	typechlp[XTYPE] =	{ 0,1,1,1,1,1,1,0,0,0,1,0,0,0,0,0,0 };
char	typscalar[XTYPE] =	{ 0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0 };
char	typechl[XTYPE] =	{ 0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0 };
char	typel[XTYPE] =		{ 0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0 };
char	typelp[XTYPE] =		{ 0,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0 };
char	typeu[XTYPE] =		{ 0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0 };
char	typeaf[XTYPE] =		{ 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0 };
char	typec[XTYPE] =		{ 0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char	typeh[XTYPE] =		{ 0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0 };
/*
char	typeX[XTYPE] =		{ X,C,C,H,H,L,L,L,F,D,P,N,A,V,S,U,E };
 */

long	tasign[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER,
	/* TUCHAR */	BNUMBER,
	/* TSHORT */	BNUMBER,
	/* TUSHORT */	BNUMBER,
	/* TLONG */	BNUMBER,
	/* TULONG */	BNUMBER,
	/* TVLONG */	BNUMBER,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	BSTRUCT,
	/* TUNION */	BUNION,
	/* TENUM */	0,
};
long	tasadd[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER,
	/* TUCHAR */	BNUMBER,
	/* TSHORT */	BNUMBER,
	/* TUSHORT */	BNUMBER,
	/* TLONG */	BNUMBER,
	/* TULONG */	BNUMBER,
	/* TVLONG */	BNUMBER,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	BINTEGER,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tcast[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER|BIND|BVOID,
	/* TUCHAR */	BNUMBER|BIND|BVOID,
	/* TSHORT */	BNUMBER|BIND|BVOID,
	/* TUSHORT */	BNUMBER|BIND|BVOID,
	/* TLONG */	BNUMBER|BIND|BVOID,
	/* TULONG */	BNUMBER|BIND|BVOID,
	/* TVLONG */	BNUMBER|BIND|BVOID,
	/* TFLOAT */	BNUMBER|BVOID,
	/* TDOUBLE */	BNUMBER|BVOID,
	/* TIND */	BINTEGER|BIND|BVOID,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	BVOID,
	/* TSTRUCT */	BSTRUCT|BVOID,
	/* TUNION */	BUNION|BVOID,
	/* TENUM */	0,
};
long	lcast[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BCHAR|BSHORT|BLONG|BUSHORT|BULONG|BVLONG|BIND,
	/* TUCHAR */	BUCHAR|BUSHORT|BULONG|BSHORT|BLONG|BVLONG|BIND,
	/* TSHORT */	BSHORT|BLONG|BULONG|BVLONG|BIND,
	/* TUSHORT */	BUSHORT|BULONG|BLONG|BVLONG|BIND,
	/* TLONG */	BLONG|BULONG|BVLONG|BIND,
	/* TULONG */	BULONG|BLONG|BVLONG|BIND,
	/* TVLONG */	BULONG|BLONG|BVLONG,
	/* TFLOAT */	BFLOAT|BDOUBLE,
	/* TDOUBLE */	BDOUBLE,
	/* TIND */	BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tadd[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER|BIND,
	/* TUCHAR */	BNUMBER|BIND,
	/* TSHORT */	BNUMBER|BIND,
	/* TUSHORT */	BNUMBER|BIND,
	/* TLONG */	BNUMBER|BIND,
	/* TULONG */	BNUMBER|BIND,
	/* TVLONG */	BNUMBER|BIND,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	BINTEGER,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tsub[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER,
	/* TUCHAR */	BNUMBER,
	/* TSHORT */	BNUMBER,
	/* TUSHORT */	BNUMBER,
	/* TLONG */	BNUMBER,
	/* TULONG */	BNUMBER,
	/* TVLONG */	BNUMBER,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	BINTEGER|BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tmul[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER,
	/* TUCHAR */	BNUMBER,
	/* TSHORT */	BNUMBER,
	/* TUSHORT */	BNUMBER,
	/* TLONG */	BNUMBER,
	/* TULONG */	BNUMBER,
	/* TVLONG */	BNUMBER,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	0,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tand[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BINTEGER,
	/* TUCHAR */	BINTEGER,
	/* TSHORT */	BINTEGER,
	/* TUSHORT */	BINTEGER,
	/* TLONG */	BINTEGER,
	/* TULONG */	BINTEGER,
	/* TVLONG */	BINTEGER,
	/* TFLOAT */	0,
	/* TDOUBLE */	0,
	/* TIND */	0,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	trel[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BNUMBER,
	/* TUCHAR */	BNUMBER,
	/* TSHORT */	BNUMBER,
	/* TUSHORT */	BNUMBER,
	/* TLONG */	BNUMBER,
	/* TULONG */	BNUMBER,
	/* TVLONG */	BNUMBER,
	/* TFLOAT */	BNUMBER,
	/* TDOUBLE */	BNUMBER,
	/* TIND */	BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	0,
	/* TUNION */	0,
	/* TENUM */	0,
};
long	tfunct[1] =
{
	/* TXXX */	BFUNC,
};
long	tindir[1] =
{
	/* TXXX */	BIND,
};
long	tdot[1] =
{
	/* TXXX */	BSTRUCT|BUNION,
};
long	tnot[1] =
{
	/* TXXX */	BNUMBER|BIND,
};
long	targ[1] =
{
	/* TXXX */	BNUMBER|BIND|BSTRUCT|BUNION,
};

char	tab[NTYPE][NTYPE] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
	0, 2, 2, 4, 4, 6, 6, 7, 8, 9,10,
	0, 3, 4, 3, 4, 5, 6, 7, 8, 9,10,
	0, 4, 4, 4, 4, 6, 6, 7, 8, 9,10,
	0, 5, 6, 5, 6, 5, 6, 7, 8, 9,10,
	0, 6, 6, 6, 6, 6, 6, 7, 8, 9,10,
	0, 7, 7, 7, 7, 7, 7, 7, 8, 9,10,
	0, 8, 8, 8, 8, 8, 8, 8, 8, 9,10,
	0, 9, 9, 9, 9, 9, 9, 9, 9, 9,10,
	0,10,10,10,10,10,10,10,10,10,10,
};
