#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

static Glab *gotos;
static Glab *labels;
static Jmps *contstack;
static Jmps *brkstack;
static Jmps *retstack;
static Jmps *rescues;

/* Compile a function */
void
fungen(Node *code, Node *args)
{
	Type *t;
	Node n1;
	ulong rpc;
	Inst *fcode;

	typechk(code, 0);
	rewrite(code);

	args->left->right = code;

	if(code)
		sucalc(code);

	if(opt('t')) {
		print("\nTree:\n");
		ptree(code, 0);
	}

	/* Clear the label stack */
	labels = 0;
	gotos = 0;
	rescues = 0;
	fcode = ipc;

	if(opt('q') == 0 && nerr == 0) {
		t = curfunc->t->next;
		switch(t->type) {
		case TADT:
		case TUNION:
		case TAGGREGATE:
			t = at(TIND, t);
			regret(&n1, t);
			rnode = stknode(n1.t);
			assign(&n1, rnode);
			regfree(&n1);
		}

		if(code)
			stmnt(code);

		rpc = ipc->pc+1;
		/* Improve optimiser by using non return register */
		switch(t->type) {
		case TVOID:
			instruction(ANOP, ZeroN, ZeroN, regn(Freg));
			instruction(ANOP, ZeroN, ZeroN, regn(Retireg));
			break;
		case TFLOAT:
			instruction(ANOP, ZeroN, ZeroN, regn(Retireg));
			break;
		default:
			instruction(ANOP, ZeroN, ZeroN, regn(Freg));
			break;
		}

		instruction(ARETURN, ZeroN, ZeroN, ZeroN);

		/* Patch in all the returns */
		while(retstack) {
			label(retstack->i, rpc);
			popjmp(&retstack);
		}

		/* Back patch the entry */
		resolvegoto();

		/* Patch in the frame size */
		funentry->dst.ival = maxframe + frame;
	}

	leaveblock();

	regcheck();

	if(opt('N') || opt('R') || opt('P'))
		regopt(fcode);
}

/* Compile code for a statement */
void
stmnt(Node *n)
{
	Jmps *p;
	Node *l, *r, n1, com;
	Inst *false, *truedone, *loop, *enter;

	if(n == ZeroN)
		return;

	l = n->left;
	r = n->right;

	iline = n->srcline;

	switch(n->type) {
	default:
		genexp(n, ZeroN);
		break;

	case OLIST:
		stmnt(l);
		stmnt(r);
		break;

	case OBLOCK:
		stmnt(l);
		break;

	case OLBLOCK:
		lblock(l);
		break;

	case OPAR:
		parcode(l->left);
		break;

	case OSWITCH:
		switchcode(n);
		break;

	case OSELECT:
		selcode(n);
		break;

	case OGOTO:
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		setgoto(n, ipc);
		break;

	case OLABEL:
		setlabel(n, ipc->pc+1);
		break;

	case OCASE:
	case ODEFAULT:
		n->pc = ipc->pc+1;

		if(l && l->type != OCONST)
			stmnt(l);

		if(r)
			stmnt(r);
		break;

	case OIF:
		gencond(l, ZeroN, True);
		false = ipc;
		if(r->type == OELSE) {
			stmnt(r->left);
			truedone = instruction(AJMP, ZeroN, ZeroN, ZeroN);
			label(false, ipc->pc+1);
			stmnt(r->right);
			label(truedone, ipc->pc+1);
			break;
		}

		if(r)
			stmnt(r);

		label(false, ipc->pc+1);
		break;

	case ORET:
		if(l) {
			switch(l->t->type) {
			default:
				regret(&n1, l->t);
				genexp(l, &n1);
				regfree(&n1);
				break;

			case TADT:
			case TUNION:
			case TAGGREGATE:
				com.type = OASGN;
				com.t = l->t;
				com.left = an(OIND, rnode, ZeroN);
				com.left->t = rnode->t->next;
				com.right = l;
				sucalc(&com);
				genexp(&com, ZeroN);
				break;
			}
		}

		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		pushjmp(&retstack);
		break;

	case ODWHILE:
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		enter = ipc;
		instruction(AJMP, ZeroN, ZeroN, ZeroN);	/* Continue */
		pushjmp(&contstack);
		instruction(AJMP, ZeroN, ZeroN, ZeroN); /* Break */
		pushjmp(&brkstack);

		label(enter, ipc->pc+1);
		loop = ipc;
		if(r)
			stmnt(r);

		label(contstack->i, ipc->pc+1);
		if(l) {
			gencond(l, ZeroN, True);
			label(ipc, brkstack->i->pc);
		}
		instruction(AJMP, ZeroN, ZeroN, ZeroN); /* Loop */
		label(ipc, loop->pc+1);

		label(brkstack->i, ipc->pc+1);
		popjmp(&contstack);
		popjmp(&brkstack);
		break;

	case OWHILE:
		instruction(AJMP, ZeroN, ZeroN, ZeroN);	/* Continue */
		pushjmp(&contstack);
		instruction(AJMP, ZeroN, ZeroN, ZeroN); /* Break */
		pushjmp(&brkstack);

		label(contstack->i, ipc->pc+1);
		loop = ipc;
		if(l) {					/* Cond */
			gencond(l, ZeroN, True);
			label(ipc, brkstack->i->pc);
		}
		
		if(r)
			stmnt(r);

		instruction(AJMP, ZeroN, ZeroN, ZeroN);	/* Loop */
		label(ipc, loop->pc+1);

		label(brkstack->i, ipc->pc+1);
		popjmp(&contstack);
		popjmp(&brkstack);
		break;

	case OFOR:
		if(l->left)				/* Assign */
			genexp(l->left, ZeroN);

		l = l->right;

		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		enter = ipc;
		instruction(AJMP, ZeroN, ZeroN, ZeroN);	/* Continue */
		pushjmp(&contstack);
		instruction(AJMP, ZeroN, ZeroN, ZeroN); /* Break */
		pushjmp(&brkstack);
		loop = ipc;
		label(enter, ipc->pc+1);

		if(l->left) {				/* Cond */
			gencond(l->left, ZeroN, True);
			label(ipc, brkstack->i->pc);
		}
		if(r)
			stmnt(r);
		label(contstack->i, ipc->pc+1);
		if(l->right)				/* Action */
			genexp(l->right, ZeroN);

		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		label(ipc, loop->next->pc);
		label(brkstack->i, ipc->pc+1);
		popjmp(&contstack);
		popjmp(&brkstack);
		break;

	case ORAISE:
		if(l) {
			instruction(AJMP, ZeroN, ZeroN, ZeroN);
			setgoto(l, ipc);
			break;
		}
		if(rescues == 0) {
			diag(n, "raise without rescue");
			break;
		}
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		label(ipc, rescues->i->pc+1);
		if(rescues->par != inpar)
			diag(n, "raise breaks join in par");
		if(rescues->crit != incrit)
			diag(n, "raise breaks critical section");
		break;

	case ORESCUE:
		instruction(AJMP, ZeroN, ZeroN, ZeroN);
		enter = ipc;
		if(l->type == OLABEL) {
			setlabel(l, ipc->pc+1);
			l = l->left;
		}
		pushjmp(&rescues);
		rescues->par = inpar;
		rescues->crit = incrit;
		stmnt(l);
		label(enter, ipc->pc+1);
		break;

	case OBREAK:
		p = brkstack;
		while(p) {
			n->ival--;
			if(n->ival == 0)
				break;
			p = p->next;
		}
		if(p) {
			instruction(AJMP, ZeroN, ZeroN, ZeroN);
			label(ipc, p->i->pc);
			break;
		}
		diag(n, "break not in loop/switch/select");
		break;

	case OCONT:
		p = contstack;
		while(p) {
			n->ival--;
			if(n->ival == 0)
				break;
			p = p->next;
		}
		if(p) {
			instruction(AJMP, ZeroN, ZeroN, ZeroN);
			label(ipc, p->i->pc);
			break;
		}
		diag(n, "continue not in loop");
		break;
	}
}

int
cascmp(Node **a, Node **b)
{
	return (*a)->left->ival - (*b)->left->ival;
}

void
casecount(Node *n, Node **vec)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	case ODEFAULT:
	case OCASE:
		if(vec)
			vec[veccnt] = n;
		veccnt++;	
		break;

	default:
		casecount(n->left, vec);
		casecount(n->right, vec);
		break;
	}
}

void
switchcode(Node *n)
{
	int i, r;
	Node **cases, *defl, *c;
	Node val;
	Inst *enter;
	long defpc;

	instruction(AJMP, ZeroN, ZeroN, ZeroN);		/* Entry */
	enter = ipc;
	instruction(AJMP, ZeroN, ZeroN, ZeroN); 	/* Break */
	pushjmp(&brkstack);

	/* Generate the code */
	stmnt(n->right);

	instruction(AJMP, ZeroN, ZeroN, ZeroN); 	/* Done break */
	label(ipc, brkstack->i->pc);

	/* Count */
	veccnt = 0;
	casecount(n->right, 0);

	/* Save */
	cases = malloc(sizeof(Node*)*veccnt);
	veccnt = 0;
	casecount(n->right, cases);

	defl = 0;
	for(i = 0; i < veccnt; i++) {
		c = cases[i];
		switch(c->type) {
		case OCASE:
			if(c->left->type != OCONST) {
				diag(c, "case must be constant");
				cases[i] = ZeroN;
			}
			break;

		case ODEFAULT:
			if(defl)
				diag(c, "switch already has default");
			defl = c;
			cases[i] = ZeroN;
			break;
		}
	}

	/* Close up the table */
	for(i = 0; i < veccnt; i++) {
		if(cases[i] == ZeroN) {
			veccnt--;
			memmove(cases+i, cases+i+1,  (veccnt-i)*sizeof(Node*));
		}
	}

	qsort(cases, veccnt, sizeof(Node*), cascmp);

	for(i = 0; i < veccnt-1; i++) {
		r = cases[i]->left->ival;
		if(r == cases[i+1]->left->ival)
			diag(cases[i+1], "duplicate case %d in switch", r);
	}

	label(enter, ipc->pc+1);
	reg(&val, builtype[TINT], ZeroN);
	genexp(n->left, &val);

	defpc = brkstack->i->pc;
	if(defl)
		defpc = defl->pc;
 
	gencmps(cases, veccnt, defpc, &val);

	regfree(&val);
	label(brkstack->i, ipc->pc+1);
	popjmp(&brkstack);
}

Node*
gcom(Node *n)
{
	Node *l;

	if(n->type == ORECV)
		return n;
	if(n->left) {
		l = gcom(n->left);
		if(l)
			return l;
	}
	if(n->right) {
		l = gcom(n->right);
		if(l)
			return l;
	}
	return ZeroN;
}

void
selcode(Node *n)
{
	int i;
	Node **cases, *c, *l;
	Node val;
	Inst *enter;
	Type *t;

	instruction(AJMP, ZeroN, ZeroN, ZeroN);		/* Entry */
	enter = ipc;
	instruction(AJMP, ZeroN, ZeroN, ZeroN); 	/* Break */
	pushjmp(&brkstack);

	stmnt(n->left);

	instruction(AJMP, ZeroN, ZeroN, ZeroN); 	/* Done break */
	label(ipc, brkstack->i->pc);

	veccnt = 0;
	casecount(n->left, 0);
	cases = malloc(sizeof(Node*)*veccnt);
	veccnt = 0;
	casecount(n->left, cases);

	for(i = 0; i < veccnt; i++) {
		c = cases[i];
		switch(c->type) {
		case OCASE:
			l = gcom(c->left);
			if(l) {
				c->left = l;
				l->type = OCALL;
				t = abt(0);
				*t = *(l->t);
				t->next = builtype[TVOID];
				l->t = t;
				l->left = selrecv;
				break;
			}
			diag(c, "case expr needs send/receive");
			cases[i] = ZeroN;
			break;

		case ODEFAULT:
			diag(c, "default illegal in select");
			cases[i] = ZeroN;
			break;
		}
	}

	for(i = 0; i < veccnt; i++) {
		if(cases[i] == ZeroN) {
			veccnt--;
			memmove(cases+i, cases+i+1,  (veccnt-i)*sizeof(Node*));
		}
	}
	if(veccnt == 0)
		return;

	label(enter, ipc->pc+1);
	for(i = 0; i < veccnt; i++) {
		c = cases[i];
		genexp(c->left, ZeroN);
		c->left->ival = i;
	}
	instruction(AJMPL, ZeroN, ZeroN, doselect);
	regret(&val, builtype[TINT]);
	gencmps(cases, veccnt, -1, &val);
	regfree(&val);

	label(brkstack->i, ipc->pc+1);
	popjmp(&brkstack);
}

void
gencmps(Node **c, int cnt, long defpc, Node *val)
{
	Node n, con, **r;
	int i;
	Inst *patch;

	con.type = OCONST;
	con.t = builtype[TINT];

	if(cnt < 4) {
		for(i = 0; i < cnt; i++) {
			con.ival = (*c)->left->ival;
			reg(&n, builtype[TINT], ZeroN);
			assign(&con, &n);
			codcond(OEQ, &n, val);
			label(ipc, (*c)->pc);
			regfree(&n);
			c++;
		}
		if(defpc != -1) {
			instruction(AJMP, ZeroN, ZeroN, ZeroN);
			label(ipc, defpc);
		}
		return;
	}
	i = cnt/2;
	r = c+i;

	con.ival = (*r)->left->ival;
	reg(&n, builtype[TINT], ZeroN);
	assign(&con, &n);
	codcond(OLT, &n, val);
	patch = ipc;
	codcond(OEQ, &n, val);
	label(ipc, (*r)->pc);
	regfree(&n);
	gencmps(c, i, defpc, val);

	label(patch, ipc->pc+1);
	gencmps(r+1, cnt-i-1, defpc, val);
}

ulong
framefind(Node *n)
{
	ulong l, r;

	if(n == ZeroN)
		return 0;

	switch(n->type) {
	default:
		l = framefind(n->left);
		r = framefind(n->right);
		if(r > l)
			l = r;
		break;
	case OCALL:
		frsize = 0;
		framesize(n->right);
		l = frsize;
	}
	return l;
}

void
parcode(Node *n)
{
	Type *t;
	ulong frs;
	int i, cnt;
	Inst *loop;
	Node *barrier, **slist, com, retr;
	Node *stv, *stvp, *oatv, *p;

	veccnt = 0;
	listcount(n, 0);
	slist = malloc(sizeof(Node*)*veccnt);
	veccnt = 0;
	listcount(n, slist);

	if(opt('O')) {
		for(i = 0; i < veccnt; i++) {
			ptree(slist[i], 0);
			print("*\n");
		}
	}

	if(veccnt < 2) {
		warn(n, "only one statement in par");
		stmnt(slist[0]);
		return;
	}

	inpar++;
	oatv = atv;
	cnt = veccnt;

	barrier = stknode(builtype[TINT]);
	com.type = OASGN;
	com.t = barrier->t;
	com.left = barrier;
	com.right = con(veccnt-1);
	sucalc(&com);
	genexp(&com, ZeroN);

	/*
	 * Build activation vector
	 */
	t = at(TIND, builtype[TIND]);
	t->size = t->next->size * cnt;	
	stv = stknode(t);

	/*
	 * craft: pid = pfork(cnt, stv)
	 */
	com.type = OCALL;
	com.t = builtype[TINT];
	com.left = pforknode;
	stvp = an(OADDR, stv, ZeroN);
	stvp->t = builtype[TIND];
	com.right = an(OLIST, con(veccnt-1), stvp);

	sucalc(&com);
	genexp(&com, ZeroN);

	for(i = 0; i < cnt-1; i++) {
		regret(&retr, builtype[TINT]);
		instruction(ACMP, con(i), ZeroN, &retr);
		instruction(ABNE, ZeroN, ZeroN, ZeroN);
		loop = ipc;
		regfree(&retr);

		/*
		 * find the largest frame in this activation
		 */
		frs = framefind(slist[i]);
		/* ensure enough space for pointer to barrier passed to ALEF_pexit */
		if(frs < builtype[TIND]->size) {
			frs = builtype[TIND]->size;
			frs = align(frs, builtype[TIND]);
		}
		if(opt('O'))
			print("%d: frame %d\n", i, frs);

		/*
		 * Compute my activation from stack vector: atv = stv[pid]
		 */
		t = builtype[TIND];
		atv = stknode(t);
		p = an(OADD, stvp, con(i*builtype[TIND]->size));
		p->t = t;
		p = an(OIND, p, ZeroN);
		p->t = t;
		/*
		 * 2*sizeof(*) is enough for save SP at activation top plus hole
		 * for the saved activation pc
		 */
		p = an(OSUB, p, con(frs+2*builtype[TIND]->size));
		p->t = t;
		/*
		 * word used for activation link
		 */
		atv->atvsafe = frs+builtype[TIND]->size;
		p = an(OASGN, atv, p);
		p->t = t;

		sucalc(p);
		genexp(p, ZeroN);

		stmnt(slist[i]);

		com.type = OCALL;
		com.t = builtype[TVOID];
		com.left = pexitnode;
		com.right = an(OADDR, barrier, ZeroN);
		com.right->t = at(TIND, barrier->t);

		sucalc(&com);
		stmnt(&com);

		label(loop, ipc->pc+1);
	}
	atv = oatv;

	stmnt(slist[i]);

	/*
	 * craft terminator: ALEF_pdone(&barrier, stv);
	 */
	com.type = OCALL;
	com.t = builtype[TINT];
	com.left = pdonenode;
	stvp = an(OADDR, stv, ZeroN);
	stvp->t = builtype[TIND];
	p = an(OADDR, barrier, 0);
	p->t = builtype[TIND];
	com.right = an(OLIST, p, stvp);

	sucalc(&com);
	stmnt(&com);

	inpar--;
}

void
lblock(Node *n)
{
	Node com, *i;

	i = internnode(builtype[TIND]);
	i = an(OADDR, i, ZeroN);
	i->t = builtype[TIND];

	com.type = OCALL;
	com.t = builtype[TVOID];
	com.left = ginode;
	com.right = i;
	sucalc(&com);
	stmnt(&com);

	incrit++;
	stmnt(n);
	incrit++;

	com.type = OCALL;
	com.t = builtype[TVOID];
	com.left = gonode;
	com.right = i;
	sucalc(&com);
	stmnt(&com);
}

/* determine addressablility and number of registers */
void
sucalc(Node *n)
{
	Node *l, *r;

	l = n->left;
	r = n->right;
	n->sun = 0;
	n->islval = 0;

	/* Addressability */
	switch(n->type) {
	case OCONST:
		n->islval = 20;
		return;

	case OREGISTER:
		n->islval = 11;
		return;

	case OINDREG:
		n->islval = 12;
		return;

	case ONAME:
		n->islval = 10;
		return;

	case OADDR:
		sucalc(l);
		if(l->islval == 10)
			n->islval = 2;
		if(l->islval == 12)
			n->islval = 3;
		break;

	case OIND:
		sucalc(l);
		if(l->islval == 11)
			n->islval = 12;
		if(l->islval == 3)
			n->islval = 12;
		if(l->islval == 2)
			n->islval = 10;
		break;

	case OADD:
		sucalc(l);
		sucalc(r);
		if(l->ival == 20) {
			if(r->ival == 2)
				n->ival = 2;
			if(r->ival == 3)
				n->ival = 3;
		}
		if(r->ival == 20) {
			if(l->ival == 2)
				n->ival = 2;
			if(l->ival == 3)
				n->ival = 3;
		}
		break;

	default:
		if(l)
			sucalc(l);
		if(r)
			sucalc(r);
		break;
	}

	/* Number of registers */
	switch(n->type) {
	default:
		if(l != ZeroN)
			n->sun = l->sun;
		if(r != ZeroN) {
			if(n->sun == r->sun)
				n->sun = n->sun + 1;
			else if(r->sun > n->sun)
				n->sun = r->sun;
		}
		if(n->sun == 0)
			n->sun = 1;
		break;

	case OCALL:
	case OSEND:
	case ORECV:
		n->sun = Sucall;
		break;
	}
}

void
setlabel(Node *n, ulong pc)
{
	Glab *i;

	for(i = labels; i; i = i->next) {
		if(i->n->sym == n->sym) {
			diag(n, "label %s used twice", n->sym->name);
			return;
		}
	}
	i = malloc(sizeof(Glab));
	i->n = n;
	i->par = inpar;
	i->crit = incrit;
	i->pc = pc;
	i->next = labels;
	labels = i;	
}

void
setgoto(Node *n, Inst *i)
{
	Glab *g;

	g = malloc(sizeof(Glab));
	g->n = n;
	g->i = i;
	g->par = inpar;
	g->crit = incrit;
	g->next = gotos;
	gotos = g;
}

Glab*
findlab(Node *n)
{
	Glab *i;

	for(i = labels; i; i = i->next)
		if(i->n->sym == n->sym)
			return i;

	return 0;
}

void
resolvegoto(void)
{
	Glab *g, *l;

	for(g = gotos; g; g = g->next) {
		l = findlab(g->n);
		if(l == 0) {
			diag(g->n, "no label called %s", g->n->sym->name);
			continue;
		}

		if(g->par != l->par)
			diag(g->n, "raise/goto breaks join from par");
		if(g->crit != l->crit)
			diag(g->n, "raise/goto breaks critical section");

		label(g->i, l->pc);
	}
}
