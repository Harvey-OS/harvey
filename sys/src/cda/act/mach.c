#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "symbols.h"
#include "dat.h"

#define NSTACK 1000
Tree *stack[NSTACK];
Tree **sp = &stack[NSTACK];
int netno;
char *globname;		/* for awful tristate hack */

void
cout(void)
{
	char *tail;
	Sym *s;
	print("#include <u.h>\n#include <libc.h>\n");
	print("#include <libg.h>\n#include <stdio.h>\n#include <cda/sim.h>\n\n");
	for (s = symtab; s->name; s++) {
		tail = &s->name[strlen(s->name)-1];
		switch(*tail) {
		case '-':
			strcpy(tail, "_0");
			break;
		case '+':
			strcpy(tail, "_1");
			break;
		}
		print("Cell %s;\n",s->name);
	}
	print("\nvoid\ninit(void)\n{\n");
	for (s = symtab; s->name; s++)
		print("\tbinde(\"%s\",&%s);\n",s->name,s->name);
	print("}\n");
	print("\nuchar\nsettle(void)\n{\n\tuchar _t,change=0;\n");
	for (s = symtab; s->name; s++) {
		if (!s->lval)
			continue;
		if (s->clk) {
			print("\tif ((_t = ");
			ctree(s->clk);
			print(") && %s.ce",s->name);
			if (!s->latch)
				print(" && !%s.ck",s->name);
			print(")\n\t\t%s.m %s= ",s->name,s->tog ? "^" : "");
			/*if (s->inv)
				print("!");	/* inv stuff - dangerous */
			ctree(s->exp);
			print(";\n\t%s.ck = _t;\n",s->name);
			if (s->rd) {
				print("\t%s.rd = ",s->name);
				ctree(s->rd);
				print(";\n",s->name);
				print("\tif (%s.rd)\n\t\t%s.m = 0;\n",s->name,s->name);
			}
			if (s->pre) {
				print("\t%s.pre = ",s->name);
				ctree(s->pre);
				print(";\n",s->name);
				print("\tif (%s.pre)\n\t\t%s.m = 1;\n",s->name,s->name);
			}
			if (s->cke) {
				print("\t%s.ce = ",s->name);
				ctree(s->cke);
				print(";\n",s->name);
			}
			else
				print("\t%s.ce = 1;\n",s->name);
		}
		else {
			globname = s->name;
			if (s->ena) {
				print("\tif (!(%s.dis = !", s->name);
				ctree(s->ena);
				print("))\n\t");
			}
			print("\t%s.m = ",s->name);
			/*if (s->inv)
				print("!");	/* inv stuff - dangerous */
			ctree(s->exp);
			print(";\n");
		}
	}
	for (s = symtab; s->name; s++)
		print("\tchange |= %s.s ^ %s.m; %s.s = %s.m;\n",s->name,s->name,s->name,s->name);
	print("\treturn change;\n}\n");
}

void
ctree(Tree *t)
{
	int op;
	if (t == 0)
		return;
	if (t->op == not || t->op == buffer) {
		if (t->op == not)
			print("!");
		/*if (t->left->op != ID)
			print("(");*/
		ctree(t->left);
		/*if (t->left->op != ID)
			print(")");*/
		return;
	}
	if (t->op == ID || t->op == CK) {
		/*if (t->id->inv)
			print("!");	/* inv stuff - dangerous */
		print("%s.s",t->id->name);
		return;
	}
	if (t->op == C0 || t->op == C1) {
		print("%d",t->val);
		return;
	}
	if (t->op == mux) {
		print("(");
		ctree(t->left);
		print(" ? ");
		ctree(t->cke);
		print(" : ");
		ctree(t->right);
		print(")");
		return;
	}
	print("(");
	if (t->op == tribuf || t->op == bibuf) {
		ctree(t->right);
		print(" ? ");
		ctree(t->left);
		print(" : %s)",globname);
		return;
	}
	for (op = t->op; t->op == op; t = t->right) {
		ctree(t->left);
		print("%s",(op == and) ? "&" : (op == or) ? " | " : " ^ ");
	}
	ctree(t);
	print(")");
}

void
merge(void)
{
	Sym *s;
	Tree *t;
	int f;
	for (s = symtab; s->name; s++)
		if (s->internal && !s->clk && (t = s->exp)
			&& (f = s->exp->fanout)  && f <= maxload) {
			if (t->id) {
				s->lval = s->rval = 0;
				s->name = t->id->name;
			}
			else
				t->id = s;
		}
}

Tree *
invtree(Tree *t)
{
	switch (t->op) {
	case C0:
	case C1:
	case inbuf:
		return t;
	case ID:
	case CK:
		if (t->id->inv)		/* bug - have to avoid stacking nots here */
			return notnode(t);
		return t;
	case dff:
		t->pre = invtree(t->pre);
	case lat:
		t->clr = invtree(t->clr);
	case mux:
		t->cke = invtree(t->cke);
	case xor:
	case or:
	case and:
	case tribuf:
	case bibuf:
		t->right = invtree(t->right);
	case buffer:
	case outbuf:
	case not:
		t->left = invtree(t->left);
		return t;
	default:
		yyerror("invtree: unexpected op %d",t->op);
		return 0;
	}
}

void
invsense(void)
{
	Sym *s;
	for (s = symtab; s->name; s++)
		;		/* need to do something, but what? */ 
	for (s = symtab; s->name; s++)
		if (s->lval)
			s->exp = invtree(s->exp);
}

void
symout(void)
{
	Sym *s;
	void dofanout(Tree *, int);
	char nsym[40];
	merge();
	if (cflag) {
		cout();
		return;
	}
	for (s = symtab; s->name; s++) {
		if (!s->lval) {
			if (s->rval) {		/* switcheroo */
				sprint(nsym,"%s_",s->name);
				if (s->clock)
					s->exp = clknode(s,idnode(lookup(nsym),0));
				else
					s->exp = innode(s,idnode(lookup(nsym),0));
			}
			else
				continue;
		}
		if (s->clk) {
			if (s->latch)
				s->exp = latnode(s->internal ? s : 0,
					s->exp,
					s->clk,
					s->cke ? notnode(notnode(s->cke)) : ONE,
					s->rd ? notnode(notnode(s->rd)) : ZERO);
			else
				s->exp = dffnode(s->internal ? s : 0,
					s->tog ? xornode(s->exp,idnode(s,0)) : s->exp,
					s->clk,
					s->cke ? notnode(notnode(s->cke)) : ONE,
					s->rd ? notnode(notnode(s->rd)) : ZERO,
					s->pre ? notnode(notnode(s->pre)) : ZERO);
			if (s->tog)
				s->rval = 1;
		}
		if (!s->internal && s->lval && s->rval)
			s->exp = binode(s,s->exp,s->ena ? s->ena : ONE);
		else if (!s->internal && s->lval)
			s->exp = s->ena ? trinode(s,s->exp,s->ena) : outnode(s,s->exp);
		if (!s->clk && s->internal && s->exp->fanout > maxload)
			s->exp = bufnode(s->exp);
		if (uflag)
			dofanout(s->exp, 1);
	}
	/*invsense();*/
	for (s = symtab; s->name; s++) {
		if (!s->lval && !s->rval)
			continue;
		if (!aflag)
			fprintf(stdout,"%s = ",s->name);
		if (dflag)
			prtree(s->exp);
		else {
			if (Root = s->exp) {
				if (s->clk == 0 && !s->exp->id)
					s->exp->id = s;
				/* buffer named identical nets */
				if (s->exp->op == ID || !s->clk && s->exp->use && s->exp->op > C1) {
					Root = bufnode(s->exp);
					Root->id = s;
				}
				_match();
			}
			if (!aflag)
				evalprint(stdout);
		}
		if (!aflag)
			fprintf(stdout,"\n");
		if (bflag)
			fflush(stdout);
	}
	if (aflag)
		fprintf(stdout,"net VCC POWER VCC\nnet GND POWER GND\n");
}

void
push(Tree *t)
{
	Sym *s;
	if (sp <= stack)
		exits("push: blown output stack");
	if (t->op == ID && (s = t->id) && !s->clk && (s->exp == ZERO || s->exp == ONE))
		t = s->exp;
	*(--sp) = t;
	if (t->op == CK)
		return;
	if (t->fanout > maxload && uflag) {
		if (t->buf == 0 || t->buf->use >= maxload)	/* t->buf->use? */
			t->buf = bufnode(t);
		func(t->buf,"BUF",1,"Y","A");
	}
}

void
actout(Tree *t, char *s, int n, char *args[])
{
	int i;
	Tree *u;
	char *id,name[20];
	if (t->id == 0) {
		sprintf(name,"U%d",++netno);
		t->id = lookup(name);
	}
	id = t->id->name;
	if (t->use == 0) {
		t->use++;
		if (t->level > levcrit)
			fprintf(critfile, "NET %s;; CRT:M.\n", id);
		fprintf(stdout,"use %s %s\n",s,id);
		fprintf(stdout,"net %s %s %s\n",id,id,args[0]);
		for (i = 0; i < n; i++) {
			u = sp[i];
			u->use++;
			/*if (strcmp("CLK", args[i+1]) == 0)
				u->use++;	/* the hard way */
			fprintf(stdout,"net %s %s %s\n",u->id->name,id,args[i+1]);
		}
	}
	sp += n;
	push(t);
}

void
namepin(char *s, int extra)
{
	if (vflag)
		sp[0]->pname = s;
	if (extra)
		sp[0]->use++;
}

void
func(Tree *t, char *s, int n, ...)
{
	if (aflag) {
		actout(t,s,n,((char **) &n)+1);
		return;
	}
/*fprintf(stdout, "%s@%u:%d ", s, sp, n);*/
	if (t->fname == 0)
		t->fname = s;
	if (sp > &stack[1]) {
		*((int *)(--sp)) = n;
		*(--sp) = t;
	}
	else
		yyerror("blown output stack");
	if (t->op != buffer && t->fanout > maxload && uflag) {
		if (t->buf == 0 || t->buf->use >= maxload)
			t->buf = bufnode(t);
t->buf->fanout = 0;
		namepin("A", 0);
		func(t->buf,"BUF",1,"Y","A");
	}	/* in or out?? */
}

void
evalprint(FILE *ouf)
{
	int i,n;
	Tree *t;
	t = *sp++;
/*fprintf(stdout, "<sp %u:%d>", sp, *sp); fflush(stdout);	/**/
	if (t->pname)
		fprintf(ouf, "%s:", t->pname);
/*fflush(stdout);/**/
	if (t->op == ID || t->op == CK)
		fprintf(ouf, "%s", t->id->name);
	else if (t->op == C0 || t->op == C1)
		fprintf(ouf, "%d", t->val);
	else {				/* must be a function */
		n = (ulong) *sp++;
		if (uflag || nflag) {
			if (nflag || t->use++ == 0)
				t->net = ++netno;
			fprintf(ouf, "N%d", t->net);
			if (t->use > 1) {
				fprintf(ouf, ":%d", t->use);
				ouf = devnull;	/* clever, eh what? */
			}
			fprintf(ouf,"=");
		}
		fprintf(ouf,"%s(", t->fname?t->fname:"?");
		for (i = 0; i < n; i++) {
			if (i > 0)
				fprintf(ouf,",");
			evalprint(ouf);
		}
		fprintf(ouf,")");
	}
}
