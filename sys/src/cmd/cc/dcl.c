#include "cc.h"

Node*
dodecl(void (*f)(int,Type*,Sym*), int c, Type *t, Node *n)
{
	Sym *s;
	Node *n1;
	long v;

	nearln = lineno;
	lastfield = 0;

loop:
	if(n != Z)
	switch(n->op) {
	default:
		diag(n, "unknown declarator: %O", n->op);
		break;

	case OARRAY:
		t = typ(TARRAY, t);
		t->width = 0;
		n1 = n->right;
		n = n->left;
		if(n1 != Z) {
			complex(n1);
			v = -1;
			if(n1->op == OCONST)
				v = n1->offset;
			if(v <= 0) {
				diag(n, "array size must be a positive constant");
				v = 1;
			}
			t->width = v * t->link->width;
		}
		goto loop;

	case OIND:
		t = typ(TIND, t);
		n = n->left;
		goto loop;

	case OFUNC:
		t = typ(TFUNC, t);
		t->down = fnproto(n);
		n = n->left;
		goto loop;

	case OBIT:
		n1 = n->right;
		complex(n1);
		lastfield = -1;
		if(n1->op == OCONST)
			lastfield = n1->offset;
		if(lastfield < 0) {
			diag(n, "field width must be non-negative constant");
			lastfield = 1;
		}
		if(lastfield == 0) {
			lastbit = 0;
			firstbit = 1;
			if(n->left != Z) {
				diag(n, "zero width named field");
				lastfield = 1;
			}
		}
		if(!typechl[t->etype]) {
			diag(n, "field type must be int-like");
			t = tint;
			lastfield = 1;
		}
		if(lastfield > tfield->width*8) {
			diag(n, "field width larger than field unit");
			lastfield = 1;
		}
		lastbit += lastfield;
		if(lastbit > tfield->width*8) {
			lastbit = lastfield;
			firstbit = 1;
		}
		n = n->left;
		goto loop;

	case ONAME:
		if(f == NODECL)
			break;
		s = n->sym;
		(*f)(c, t, s);
		if(s->class == CLOCAL)
			s = mkstatic(s);
		firstbit = 0;
		n->sym = s;
		n->type = s->type;
		n->offset = s->offset;
		n->class = s->class;
		n->etype = TVOID;
		if(n->type != T)
			n->etype = n->type->etype;
		if(debug['d'])
			dbgdecl(s);
		s->varlineno = lineno;
		if(n->ref)
			if(f == edecl) {
				n->ref->class = CSELEM+CLAST;
				strl->lineno = n->ref->lineno;
			} else {
				n->ref->class = s->class+CLAST;
				if(s->class == CGLOBL && s->varlineno != 0)
					n->ref->dlineno = s->varlineno;
				else
					s->varlineno = n->ref->lineno;
			}
		break;
	}
	lastdcl = t;
	return n;
}

Sym*
mkstatic(Sym *s)
{
	Sym *s1;

	if(s->class != CLOCAL)
		return s;
	sprint(symb, "%s.%d", s->name, s->block);
	s1 = lookup();
	if(s1->class != CSTATIC) {
		s1->type = s->type;
		s1->offset = s->offset;
		s1->block = s->block;
		s1->class = CSTATIC;
	}
	return s1;
}

/*
 * make a copy of a typedef
 * the problem is to split out imcomplete
 * arrays so that it is in the variable
 * rather than the typedef.
 */
Type*
tcopy(Type *t)
{
	Type *tl, *tx;
	int et;

	if(t == T)
		return t;
	et = t->etype;
	if(typesu[et])
		return t;
	tl = tcopy(t->link);
	if(tl != t->link ||
	  (et == TARRAY && t->width == 0)) {
		tx = typ(TXXX, 0);
		*tx = *t;
		tx->link = tl;
		return tx;
	}
	return t;
}

Node*
doinit(Sym *s, Type *t, long o, Node *a)
{
	Node *n;

	if(t == T)
		return Z;
	if(s->class == CEXTERN) {
		s->class = CGLOBL;
		if(debug['d'])
			dbgdecl(s);
	}
	if(debug['i']) {
		print("t = %T; o = %ld; n = %s\n", t, o, s->name);
		prtree(a, "doinit value");
	}


	n = initlist;
	if(a->op == OINIT)
		a = a->left;
	initlist = a;

	a = init1(s, t, o, 0);
	if(initlist != Z)
		diag(initlist, "more initializers than structure: %s",
			s->name);
	initlist = n;

	return a;
}

/*
 * get next major operator,
 * dont advance initlist.
 */
Node*
peekinit(void)
{
	Node *a;

	a = initlist;

loop:
	if(a == Z)
		return a;
	if(a->op == OLIST) {
		a = a->left;
		goto loop;
	}
	return a;
}

/*
 * consume and return next element on
 * initlist. expand strings.
 */
Node*
nextinit(void)
{
	Node *a, *b, *n;

	a = initlist;
	n = Z;

loop:
	if(a == Z)
		return a;
	if(a->op == OLIST) {
		n = a->right;
		a = a->left;
	}
	if(a->op == OUSED) {
		a = a->left;
		b = new(OCONST, Z, Z);
		b->type = a->type->link;
		if(a->op == OSTRING) {
			b->offset = castto(*a->us, TCHAR);
			a->us++;
		}
		if(a->op == OLSTRING) {
			b->offset = castto(*a->rs, TUSHORT);
			a->rs++;
		}
		a->type->width -= b->type->width;
		if(a->type->width <= 0)
			initlist = n;
		return b;
	}
	initlist = n;
	return a;
}

int
isstruct(Node *a, Type *t)
{
	Node *n;

	switch(a->op) {
	case ODOTDOT:
		n = a->left;
		if(n && n->type && sametype(n->type, t))
			return 1;
	case OSTRING:
	case OLSTRING:
	case OCONST:
	case OINIT:
		return 0;
	}

	n = new(ODOTDOT, Z, Z);
	*n = *a;

	/*
	 * ODOTDOT is a flag for tcom
	 * a second tcom will not be performed
	 */
	a->op = ODOTDOT;
	a->left = n;
	a->right = Z;

	if(tcom(n))
		return 0;

	if(sametype(n->type, t))
		return 1;
	return 0;
}

Node*
init1(Sym *s, Type *t, long o, int exflag)
{
	Node *a, *l, *r;
	long e, w, so, mw;

	a = peekinit();
	if(a == Z)
		return Z;

	if(debug['i']) {
		print("t = %T; o = %ld; n = %s\n", t, o, s->name);
		prtree(a, "init1 value");
	}

	if(exflag && a->op == OINIT)
		return doinit(s, t, o, nextinit());

	switch(t->etype) {
	default:
		diag(Z, "unknown type in initialization: %T to: %s", t, s->name);
		return Z;

	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
	case TVLONG:
	case TFLOAT:
	case TDOUBLE:
	case TIND:
	single:
		if(a->op == OARRAY)
			return Z;

		a = nextinit();
		if(a == Z)
			return Z;

		if(t->nbits)
			diag(Z, "cannot initialize bitfields");
		if(s->class == CAUTO) {
			l = new(ONAME, Z, Z);
			l->sym = s;
			l->type = t;
			l->etype = TVOID;
			if(s->type)
				l->etype = s->type->etype;
			l->offset = s->offset + o;
			l->class = s->class;

			l = new(OAS, l, a);
			return l;
		}

		complex(a);
		if(a->type == T)
			return Z;

		if(a->op == OCONST) {
			if(!sametype(a->type, t)) {
				e = a->lineno;
				a = new(OCAST, a, Z);
				a->lineno = e;
				a->type = t;
				complex(a);
			}
			if(a->op != OCONST) {
				diag(a, "initializer is not a constant: %s",
					s->name);
				return Z;
			}
			if(vconst(a) == 0)
				return Z;
			goto gext;
		}
		if(t->etype == TIND) {
			while(a->op == OCAST) {
				warn(a, "CAST in initialization ignored");
				a = a->left;
			}
			if(!sametype(t, a->type)) {
				diag(a, "initialization of incompatible pointers: %s",
					s->name);
				print("%T and %T\n", t, a->type);
			}
			if(a->op == OADDR)
				a = a->left;
			goto gext;
		}
		if(typelp[t->etype]) {
			while(a->op == OCAST)
				a = a->left;
			if(a->op == OADDR) {
				warn(a, "initialize pointer to an integer", s->name);
				a = a->left;
				goto gext;
			}
		}
		diag(a, "initializer is not a constant: %s", s->name);
		return Z;

	gext:
		gextern(s, a, o, t->width);

		return Z;

	case TARRAY:
		w = t->link->width;
		if(a->op == OSTRING || a->op == OLSTRING)
		if(typechl[t->link->etype]) {
			/*
			 * get rid of null if sizes match exactly
			 */
			a = nextinit();
			mw = t->width/w;
			so = a->type->width/a->type->link->width;
			if(mw && so > mw) {
				if(so != mw+1)
					diag(a, "string initialization larger than array");
				a->type->width -= a->type->link->width;
			}

			/*
			 * arrange strings to be expanded
			 * inside OINIT braces.
			 */
			a = new(OUSED, a, Z);
			return doinit(s, t, o, a);
		}

		mw = -w;
		l = Z;
		for(e=0;;) {
			/*
			 * peek ahead for element initializer
			 */
			a = peekinit();
			if(a == Z)
				break;
			if(a->op == OARRAY) {
				a = nextinit();
				r = a->left;
				complex(r);
				if(r->op != OCONST) {
					diag(r, "initializer subscript must be constant");
					return Z;
				}
				e = r->offset;
				if(t->width != 0)
					if(e < 0 || e*w >= t->width) {
						diag(a, "initilization index out of range: %ld", e);
						e = 0;
					}
				continue;
			}

			so = e*w;
			if(so > mw)
				mw = so;
			if(t->width != 0)
				if(mw >= t->width)
					break;
			r = init1(s, t->link, o+so, 1);
			l = newlist(l, r);
			e++;
		}
		if(t->width == 0)
			t->width = mw+w;
		return l;

	case TUNION:
	case TSTRUCT:
		/*
		 * peek ahead to find type of rhs.
		 * if its a structure, then treat
		 * this element as a variable
		 * rather than an aggregate.
		 */
		if(isstruct(a, t))
			goto single;

		if(t->width <= 0) {
			diag(Z, "incomplete structure: %s", s->name);
			return Z;
		}
		l = Z;
		for(t = t->link; t != T; t = t->down) {
			a = peekinit();
			if(a == Z)
				break;
			if(a->op == OARRAY)
				break;
			r = init1(s, t, o+t->offset, 1);
			l = newlist(l, r);
		}
		return l;
	}
}

Node*
newlist(Node *l, Node *r)
{
	if(r == Z)
		return l;
	if(l == Z)
		return r;
	return new(OLIST, l, r);
}

void
suallign(Type *t)
{
	Type *l;
	long o, w;

	o = 0;
	switch(t->etype) {

	case TSTRUCT:
		t->offset = 0;
		w = 0;
		for(l = t->link; l != T; l = l->down) {
			if(l->nbits) {
				if(l->shift <= 0) {
					l->shift = -l->shift;
					w += round(w, tfield->width);
					o = w;
					w += tfield->width;
				}
				l->offset = o;
			} else {
				if(l->width <= 0)
					if(l->sym)
						diag(Z, "incomplete structure element: %s",
							l->sym->name);
					else
						diag(Z, "incomplete structure element");
				w += round(w, allign(l));
				l->offset = w;
				w += l->width;
			}
		}
		w += round(w, supad);
		t->width = w;
		return;

	case TUNION:
		t->offset = 0;
		w = 0;
		for(l = t->link; l != T; l = l->down) {
			if(l->width <= 0)
				if(l->sym)
					diag(Z, "incomplete union element: %s",
						l->sym->name);
				else
					diag(Z, "incomplete union element");
			l->offset = 0;
			l->shift = 0;
			if(l->width > w)
				w = l->width;
		}
		w += round(w, supad);
		t->width = w;
		return;

	default:
		diag(Z, "unknown type in suallign: %T", t);
		break;
	}
}

int
allign(Type *t)
{
	int w;

	while(t->etype == TARRAY)
		t = t->link;
	w = ewidth[t->etype];
	if(w <= 0 || w > suround)
		w = suround;
	return w;
}

int
round(long v, long w)
{
	int r;

	if(w <= 0) {
		diag(Z, "rounding by %d", w);
		w = 1;
	}
	if(w > types[TLONG]->width)
		w = types[TLONG]->width;
	r = v%w;
	if(r)
		r = w-r;
	return r;
}

Type*
ofnproto(Node *n)
{
	Type *tl, *tr, *t;

loop:
	if(n == Z)
		return T;
	switch(n->op) {
	case OLIST:
		tl = ofnproto(n->left);
		tr = ofnproto(n->right);
		if(tl == T)
			return tr;
		tl->down = tr;
		return tl;

	case ONAME:
		t = typ(TXXX, T);
		*t = *n->sym->type;
		t->down = T;
		return t;
	}
	return T;
}

#define	ANSIPROTO	1
#define	OLDPROTO	2

void
argmark(Node *n, int pass)
{
	Type *t;

	autoffset = 0;
	if(typesu[thisfn->link->etype]) {
		autoffset += types[TIND]->width;
		autoffset += round(autoffset, tint->width);
	}
	stkoff = 0;
	for(; n->left != Z; n = n->left) {
		if(n->op != OFUNC || n->left->op != ONAME)
			continue;
		walkparam(n->right, pass);
		if(pass != 0 && anyproto(n->right) == OLDPROTO) {
			t = typ(TFUNC, n->left->sym->type->link);
			t->down = typ(TOLD, T);
			t->down->down = ofnproto(n->right);
			tmerge(t, n->left->sym);
			n->left->sym->type = t;
		}
		break;
	}
	autoffset = 0;
	stkoff = 0;
}

void
walkparam(Node *n, int pass)
{
	Sym *s;

	if(n != Z && n->op == OPROTO && n->left == Z && n->type == types[TVOID])
		return;

loop:
	if(n == Z)
		return;
	switch(n->op) {
	default:
		diag(n, "argument not a name/prototype: %O", n->op);
		break;

	case OLIST:
		walkparam(n->left, pass);
		n = n->right;
		goto loop;

	case OPROTO:
		if(pass == 0) {
			for(; n != Z; n=n->left)
				if(n->op == ONAME) {
					s = n->sym;
					push1(s);
					s->offset = -1;
					break;
				}
			if(n == Z)
				diag(Z, "no name in argument declaration");
			break;
		}
		dodecl(pdecl, CPARAM, n->type, n->left);
		break;

	case ODOTDOT:
		break;
	
	case ONAME:
		s = n->sym;
		if(pass == 0) {
			push1(s);
			s->offset = -1;
			break;
		}
		if(s->offset != -1) {
			if(autoffset == 0) {
				firstarg = s;
				firstargtype = s->type;
			}
			s->offset = autoffset;
			autoffset += s->type->width;
			autoffset += round(autoffset, tint->width);
		} else
			dodecl(pdecl, CXXX, tint, n);
		break;
	}
}

void
markdcl(void)
{
	Decl *d;

	blockno++;
	d = push();
	d->val = DMARK;
	d->offset = autoffset;
	d->block = autobn;
	autobn = blockno;
}

void
revertdcl(void)
{
	Decl *d;
	Sym *s;

	for(;;) {
		d = dclstack;
		if(d == D) {
			diag(Z, "pop off dcl stack");
			break;
		}
		dclstack = d->link;
		s = d->sym;
		switch(d->val) {
		case DMARK:
			autoffset = d->offset;
			autobn = d->block;
			return;

		case DAUTO:
			if(debug['d'])
				print("revert1 \"%s\"\n", s->name);
			if(s->aused == 0) {
				nearln = s->varlineno;
				if(s->class == CAUTO)
					warn(Z, "auto declared and not used: %s", s->name);
				if(s->class == CPARAM)
					warn(Z, "param declared and not used: %s", s->name);
			}
			s->type = d->type;
			s->class = d->class;
			s->offset = d->offset;
			s->block = d->block;
			s->varlineno = d->varlineno;
			s->aused = d->aused;
			break;

		case DSUE:
			if(debug['d'])
				print("revert2 \"%s\"\n", s->name);
			s->suetag = d->type;
			s->sueblock = d->block;
			break;

		case DLABEL:
			if(debug['d'])
				print("revert3 \"%s\"\n", s->name);
			s->label = Z;
			break;
		}
	}
}

Type*
fnproto(Node *n)
{
	int r;

	r = anyproto(n->right);
	if(r == 0 || (r & OLDPROTO)) {
		if(r & ANSIPROTO)
			diag(n, "mixed ansi/old function declaration: %F", n->left);
		return T;
	}
	return fnproto1(n->right);
}

int
anyproto(Node *n)
{
	int r;

	r = 0;

loop:
	if(n == Z)
		return r;
	switch(n->op) {
	case OLIST:
		r |= anyproto(n->left);
		n = n->right;
		goto loop;

	case ODOTDOT:
	case OPROTO:
		return r | ANSIPROTO;
	}
	return r | OLDPROTO;
}

Type*
fnproto1(Node *n)
{
	Type *t;

	if(n == Z)
		return T;
	switch(n->op) {
	case OLIST:
		t = fnproto1(n->left);
		if(t != T)
			t->down = fnproto1(n->right);
		return t;

	case OPROTO:
		lastdcl = T;
		dodecl(NODECL, CXXX, n->type, n->left);
		t = typ(TXXX, T);
		if(lastdcl != T)
			*t = *paramconv(lastdcl, 1);
		return t;

	case ONAME:
		diag(n, "incomplete argument prototype");
		return typ(tint->etype, T);

	case ODOTDOT:
		return typ(TDOT, T);
	}
	diag(n, "unknown op in fnproto");
	return T;
}

void
dbgdecl(Sym *s)
{

	print("decl \"%s\": %s ", s->name, cnames[s->class]);
	if(s->class == CAUTO)
		print("[%d:%ld] ", s->block, s->offset);
	print("%T\n", s->type);
}

Decl*
push(void)
{
	Decl *d;

	ALLOC(d, Decl);
	d->link = dclstack;
	dclstack = d;
	return d;
}

Decl*
push1(Sym *s)
{
	Decl *d;

	d = push();
	d->sym = s;
	d->val = DAUTO;
	d->type = s->type;
	d->class = s->class;
	d->offset = s->offset;
	d->block = s->block;
	d->varlineno = s->varlineno;
	d->aused = s->aused;
	return d;
}

int
sametype(Type *t1, Type *t2)
{

	if(t1 == t2)
		return 1;
	return rsametype(t1, t2, 5);
}

int
rsametype(Type *t1, Type *t2, int n)
{
	int et;

	n--;
	for(;;) {
		if(t1 == t2)
			return 1;
		if(t1 == T || t2 == T)
			return 0;
		if(n <= 0)
			return 1;
		et = t1->etype;
		if(et != t2->etype)
			return 0;
		if(et == TFUNC) {
			if(!rsametype(t1->link, t2->link, n))
				return 0;
			t1 = t1->down;
			t2 = t2->down;
			while(t1 != T && t2 != T) {
				if(t1->etype == TOLD) {
					t1 = t1->down;
					continue;
				}
				if(t2->etype == TOLD) {
					t2 = t2->down;
					continue;
				}
				while(t1 != T || t2 != T) {
					if(!rsametype(t1, t2, n))
						return 0;
					t1 = t1->down;
					t2 = t2->down;
				}
				break;
			}
			return 1;
		}
		t1 = t1->link;
		t2 = t2->link;
		if(typesu[et])
			for(;;) {
				if(t1 == t2)
					return 1;
				if(!rsametype(t1, t2, n))
					return 0;
				t1 = t1->down;
				t2 = t2->down;
			}
		if(et == TIND)
			if(t1->etype == TVOID || t2->etype == TVOID)
				return 1;
	}
}

void
dotag(Sym *s, int et, int bn)
{
	Decl *d;

	if(bn != 0 && bn != s->sueblock) {
		d = push();
		d->sym = s;
		d->val = DSUE;
		d->type = s->suetag;
		d->block = s->sueblock;
		s->suetag = T;
	}
	if(s->suetag == T) {
		s->suetag = typ(et, T);
		s->sueblock = autobn;
	}
	if(s->suetag->etype != et)
		diag(Z, "tag used for more than one type: %s",
			s->name);
	if(s->suetag->tag == S)
		s->suetag->tag = s;
}

Node*
dcllabel(Sym *s, int f)
{
	Decl *d, d1;
	Node *n;

	n = s->label;
	if(n != Z) {
		if(f) {
			if(n->complex)
				diag(Z, "label reused: %s", s->name);
			n->complex = 1;
		}
		return n;
	}

	d = push();
	d->sym = s;
	d->val = DLABEL;
	dclstack = d->link;

	d1 = *firstdcl;
	*firstdcl = *d;
	*d = d1;

	firstdcl->link = d;
	firstdcl = d;

	n = new(OXXX, Z, Z);
	n->sym = s;
	n->complex = f;
	s->label = n;

	if(debug['d'])
		dbgdecl(s);
	return n;
}

Type*
paramconv(Type *t, int f)
{

	switch(t->etype) {
	case TARRAY:
		t = typ(TIND, t->link);
		t->width = types[TIND]->width;
		break;

	case TFUNC:
		t = typ(TIND, t);
		t->width = types[TIND]->width;
		break;

	case TFLOAT:
		if(!f)
			t = types[TDOUBLE];
		break;

	case TCHAR:
	case TSHORT:
		if(!f)
			t = tint;
		break;

	case TUCHAR:
	case TUSHORT:
		if(!f)
			t = tuint;
		break;
	}
	return t;
}

void
adecl(int c, Type *t, Sym *s)
{

	if(c == CSTATIC)
		c = CLOCAL;
	if(t->etype == TFUNC) {
		if(c == CXXX)
			c = CEXTERN;
		if(c == CLOCAL)
			c = CSTATIC;
		if(c == CAUTO || c == CEXREG)
			diag(Z, "function cannot be %s %s", cnames[c], s->name);
	}
	if(c == CXXX)
		c = CAUTO;
	if(s->class == CAUTO || s->class == CPARAM || s->class == CLOCAL)
	if(s->block == autobn)
		diag(Z, "auto redeclaration of: %s", s->name);
	if(c != CPARAM)
		push1(s);
	s->block = autobn;
	s->offset = 0;
	s->type = t;
	s->class = c;
	s->aused = 0;

	if(c != CAUTO && c != CPARAM)
		return;
	if(c == CPARAM && autoffset == 0) {
		firstarg = s;
		firstargtype = t;
	}
	if(t->width < tint->width)
		autoffset += endian(t->width);
	s->offset = autoffset;
	autoffset += t->width;
	autoffset += round(autoffset, tint->width);
	if(c == CAUTO)
		s->offset = -autoffset;
	if(autoffset > stkoff) {
		stkoff = autoffset;
		stkoff += round(stkoff, types[TLONG]->width);
	}
}

void
pdecl(int c, Type *t, Sym *s)
{
	if(s->offset != -1) {
		diag(Z, "not a parameter: %s", s->name);
		return;
	}
	t = paramconv(t, c==CPARAM);
	if(c == CXXX)
		c = CPARAM;
	if(c != CPARAM) {
		diag(Z, "parameter cannot have class: %s", s->name);
		c = CPARAM;
	}
	adecl(c, t, s);
}

void
xdecl(int c, Type *t, Sym *s)
{
	long o;

	o = 0;
	if(c == CEXREG) {
		o = exreg(t);
		if(o == 0)
			c = CEXTERN;
	}
	if(c == CXXX) {
		c = CGLOBL;
		if(s->class == CEXTERN)
			s->class = c;
	}
	if(c == CEXTERN)
		if(s->class == CGLOBL)
			c = CGLOBL;
	if(c == CAUTO) {
		diag(Z, "external declaration cannot be auto: %s", s->name);
		c = CEXTERN;
	}
	if(s->class == CSTATIC)
		if(c == CEXTERN || c == CGLOBL)
			c = CSTATIC;
	if(s->type != T)
		if(s->class != c || !sametype(t, s->type) || t->etype == TENUM) {
			diag(Z, "external redeclaration of: %s", s->name);
			print("	%T; %T\n", t, s->type);
		}
	tmerge(t, s);
	s->type = t;
	s->class = c;
	s->block = 0;
	s->offset = o;
}

void
tmerge(Type *t1, Sym *s)
{
	Type *ta, *tb, *t2;

	t2 = s->type;
/*print("merge	%T; %T\n", t1, t2);/**/
	for(;;) {
		if(t1 == T || t2 == T || t1 == t2)
			break;
		if(t1->etype != t2->etype)
			break;
		switch(t1->etype) {
		case TFUNC:
			ta = t1->down;
			tb = t2->down;
			if(ta == T) {
				t1->down = tb;
				break;
			}
			if(tb == T)
				break;
			while(ta != T && tb != T) {
				if(ta == tb)
					break;
				/* ignore old-style flag */
				if(ta->etype == TOLD) {
					ta = ta->down;
					continue;
				}
				if(tb->etype == TOLD) {
					tb = tb->down;
					continue;
				}
				/* checking terminated by ... */
				if(ta->etype == TDOT && tb->etype == TDOT) {
					ta = T;
					tb = T;
					break;
				}
				if(!sametype(ta, tb))
					break;
				ta = ta->down;
				tb = tb->down;
			}
			if(ta != tb)
				diag(Z, "function inconsistently declared: %s", s->name);

			/* take new-style over old-style */
			ta = t1->down;
			tb = t2->down;
			if(ta != T && ta->etype == TOLD)
				if(tb != T && tb->etype != TOLD)
					t1->down = tb;
			break;

		case TARRAY:
			/* should we check array size change? */
			if(t2->width > t1->width)
				t1->width = t2->width;
			break;

		case TUNION:
		case TSTRUCT:
			return;
		}
		t1 = t1->link;
		t2 = t2->link;
	}
}

void
edecl(int c, Type *t, Sym *s)
{
	Type *t1;
	int n;

	if(s == S) {
		if(!typesu[t->etype])
			diag(Z, "unnamed structure element must be struct/union");
		if(c != CXXX)
			diag(Z, "unnamed structure element cannot have class");
	} else
		if(c != CXXX)
			diag(Z, "structure element cannot have class: %s", s->name);
	t1 = t;
	t = typ(TXXX, T);
	*t = *t1;
	t->sym = s;
	t->down = T;
	if(lastfield) {
		t->shift = lastbit - lastfield;
		t->nbits = lastfield;
		if(firstbit)
			t->shift = -t->shift;
		/* real bitfields or not */
		if(0) {
			n = t->nbits;
			lastfield = 0;
			firstbit = 0;
			lastbit = 0;
			t->nbits = 0;
			t->shift = 0;
			if(typeu[t->etype]) {
				/* BUG */
				if(n <= 8)
					t->etype = TUCHAR;
				else
				if(n <= 16)
					t->etype = TUSHORT;
				else
					t->etype = TULONG;
			} else {
				if(n <= 8)
					t->etype = TCHAR;
				else
				if(n <= 16)
					t->etype = TSHORT;
				else
					t->etype = TLONG;
			}
		}
	}
	if(strf == T)
		strf = t;
	else
		strl->down = t;
	strl = t;
}

Type*
maxtype(long v)
{
	char c;
	short s;

/*
 * BUG this tests the compile, not the target
 */
	c = v;
	if(c == v)
		return types[TCHAR];
	s = v;
	if(s == v)
		return types[TSHORT];
	return types[TLONG];
}

void
doenum(Sym *s, Node *n)
{

	if(n) {
		complex(n);
		if(n->op != OCONST || !typechl[n->type->etype]) {
			diag(n, "constant integer expected in enum: %s", s->name);
			return;
		}
		if(tint != types[TLONG] && typel[n->type->etype])
			diag(n, "enum constant cannot be long");
		lastenum = n->offset;
	}
	if(dclstack)
		push1(s);
	xdecl(CXXX, types[TENUM], s);
	if(ovflo(lastenum, tint->etype))
		diag(n, "enum constant exceeds an integer: %s", s->name);
	s->offset = lastenum;
	if(debug['d'])
		dbgdecl(s);
	if(lastenum > maxenum)
		maxenum = lastenum;
	if(-lastenum > maxenum)
		maxenum = -lastenum;
	lastenum++;
}

void
dbgprint(Sym *s, Type *t)
{
	Type *l;

	if(suedebug == 0)
		return;
	if(strcmp(s->name, suedebug) != 0)
		return;
	print("%s %ld %T\n", s->name, t->width, t);
	for(l = t->link; l != T; l = l->down) {
		if(l->sym)
			print("%s %ld %T\n", l->sym->name, l->offset, l);
		else
			print("<> %ld %T\n", l->offset, l);
	}
}

void
symadjust(Sym *s, Node *n, long del)
{

	switch(n->op) {
	default:
		if(n->left)
			symadjust(s, n->left, del);
		if(n->right)
			symadjust(s, n->right, del);
		return;

	case ONAME:
		if(n->sym == s)
			n->offset -= del;
		return;

	case OCONST:
	case OSTRING:
	case OLSTRING:
	case OINDREG:
	case OREGISTER:
		return;
	}
}

Node*
contig(Sym *s, Node *n, long v)
{
	Node *p, *r, *q, *m;
	long w;

	if(n == Z)
		goto no;
	w = s->type->width;

	/*
	 * nightmare: an automatic array whose size
	 * increases when it is initialized
	 */
	if(v != w) {
		if(v != 0)
			diag(n, "automatic adjustable array: %s", s->name);
		v = s->offset;
		autoffset += w;
		autoffset += round(autoffset, tint->width);
		s->offset = -autoffset;
		if(autoffset > stkoff) {
			stkoff = autoffset;
			stkoff += round(stkoff, types[TLONG]->width);
		}
		symadjust(s, n, v - s->offset);
	}
	if(w <= 4)
		goto no;
	if(n->op == OAS)
		if(n->left->type)
		if(n->left->type->width == w)
			goto no;
	while(w & 3)
		w++;	/* is this a bug?? */
/*
 * insert the following code
 *
	*(long**)&X = (long*)((char*)X + sizeof(X));
	do {
		*(long**)&X -= 1;
		**(long**)&X = 0;
	} while(*(long**)&X);
 */

	for(q=n; q->op != ONAME; q=q->left)
		;

	p = new(ONAME, Z, Z);
	*p = *q;
	p->type = typ(TIND, types[TLONG]);
	p->offset = s->offset;

	r = new(ONAME, Z, Z);
	*r = *p;
	r = new(OPOSTDEC, r, Z);

	q = new(ONAME, Z, Z);
	*q = *p;
	q = new(OIND, q, Z);

	m = new(OCONST, Z, Z);
	m->offset = 0;
	m->type = types[TLONG];

	q = new(OAS, q, m);

	r = new(OLIST, r, q);

	q = new(ONAME, Z, Z);
	*q = *p;
	r = new(ODWHILE, q, r);

	q = new(ONAME, Z, Z);
	*q = *p;
	q->type = q->type->link;
	q->offset += w;
	q = new(OADDR, q, 0);

	q = new(OAS, p, q);
	r = new(OLIST, q, r);

	n = new(OLIST, r, n);

no:
	return n;
}
