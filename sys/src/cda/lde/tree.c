#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

Node *
build(int op, Node *left, Node *right)
{
	Node *tp;


	if (nextnode == TNODES) {
		fprint(2,"out of treespace\n");
#ifdef PLAN9
		exits("out of treespace");
#else
		exit(1);
#endif
	}
	tp = &nodes[nextnode++];
	tp->code = op;
	tp->t1 = left;
	tp->t2 = right;
	return(tp);
}

void
treeprint(Node *tp)
{
	char *op;
	extern int yylineno;

	op = 0;
	if (tp==0) {
		fprint(1, "<>");
		return;
	}
	switch(tp->code) {
	case 0:
		return;
	case ELIST:
		treeprint(tp->t1);
		fprint(1, "\n");
		treeprint(tp->t2);
		return;
	case ASSIGN:
		hp = (Hshtab *)tp->t1;
		fprint(1, " %s = ", hp->name);
		treeprint(tp->t2);
		return;
	case DONTCARE:
		hp = (Hshtab *)tp->t1;
		fprint(1, " %s' = ", hp->name);
		treeprint(tp->t2);
		return;
	case AND:
		op = "&"; break;
	case OR:
		op = "|"; break;
	case LAND:
		op = "&&"; break;
	case LOR:
		op = "||"; break;
	case XOR:
		op = "^"; break;
	case FLONE:
		op = " <";
		goto unary;
	case FRONE:
		op = " >";
		goto unary;
	case GREY:
		op = " %";
		goto unary;
	case NEG:
		op = " -";
		goto unary;
	case NOT:
		op = " !";
		goto unary;
	case COM:
		op = " ~";
	unary:
		fprint(1, op);
		treeprint(tp->t1);
		return;
	case ADD:
		op = "+"; break;
	case SUB:
		op = "-"; break;
	case MUL:
		op = "*"; break;
	case DIV:
		op = "/"; break;
	case MOD:
		op = "%"; break;
	case LS:
		op = "<<"; break;
	case RS:
		op = ">>"; break;
	case LT:
		op = "<"; break;
	case GT:
		op = ">"; break;
	case LE:
		op = "<="; break;
	case GE:
		op = ">="; break;
	case EQ:
		op = "=="; break;
	case NE:
		op = "!="; break;
	case CND:
		fprint(1, "if (");
		treeprint(tp->t1);
		fprint(1, ")");
		treeprint(tp->t2);
		return;
	case ALT:
		fprint(1, "then ");
		treeprint(tp->t1);
		fprint(1, "else ");
		treeprint(tp->t2);
		return;
	case EQN:
	case INPUT:
	case FIELD:
		hp = (Hshtab *)tp->t1;
		fprint(1, " %s ",hp->name);
		return;
	case NUMBER:
		fprint(1, " %d ",tp->t1);
		return;
	case SWITCH:
		fprint(1, "SW "); treeprint(tp->t1);
		treeprint(tp->t2);
		return;
	case CASE:
		fprint(1, "case"); treeprint(tp->t1);
		treeprint(tp->t2);
		return;
	default:
		fprint(2,"line %d bad op code %d\n",yylineno,tp->code);
#ifdef PLAN9
		exits("bad op code");
#else
		exit(1);
#endif
	}
	fprint(1, "%s(",op);
	treeprint(tp->t1);
	treeprint(tp->t2);
	fprint(1, ")");
}

void
treedep(Node *tp)
{
	int lo, hi;

	if(tp==0)
		return;
	switch(tp->code) {
	case 0:
		return;
	case ELIST:
		treedep(tp->t1);
		treedep(tp->t2);
		return;
	case ASSIGN:
		depend = 0;
		treedep(tp->t2);
		hp = (Hshtab*)tp->t1;
		hp->depend |= depend;
		if(hp->code != FIELD)
			return;
		lo = LO(hp);
		hi = HI(hp);
		for(;lo < hi; lo++) {
			hp = fields[lo];
			hp->depend |= depend;;
		}
		return;
	case DONTCARE:
		return;
	case AND:
	case LAND:
	case LOR:
	case OR:
	case XOR:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case LS:
	case RS:
	case LT:
	case GT:
	case LE:
	case GE:
	case EQ:
	case NE:
	case CND:
	case ALT:
	case SWITCH:
	case CASE:
		treedep(tp->t1);
		treedep(tp->t2);
		return;
	case FLONE:
	case FRONE:
	case GREY:
	case NEG:
	case NOT:
	case COM:
		treedep(tp->t1);
		return;
	case EQN:
		hp = (Hshtab*)tp->t1;
		depend |= hp->depend;
		return;
	case INPUT:
		hp = (Hshtab*)tp->t1;
		depend |= 1L << (hp->iref-1);
		return;
	case FIELD:
		hp = (Hshtab*)tp->t1;
		lo = LO(hp);
		hi = HI(hp);
		for(;lo < hi; lo++) {
			hp = fields[lo];
			depend |= 1L << (hp->iref-1);
		}
		return;
	case NUMBER:
		return;
	default:
		fprint(2,"line %d bad op code %d\n",yylineno,tp->code);
#ifdef PLAN9
		exits("bad op code");
#else
		exit(1);
#endif
	}
}

Value
eval(Node *tp)
{

	switch(tp->code) {
	case 0:
		return ~0L;
	case ELIST:
		if(tp->t1 != 0)
			eval(tp->t1);
		if(tp->t2 != 0)
			eval(tp->t2);
		return(~0L);
	case ASSIGN:
		assgn((Hshtab *)tp->t1, eval(tp->t2), 0);
		return(~0L);
	case DONTCARE:
		assgn((Hshtab *)tp->t1, eval(tp->t2), 1);
		return(~0L);
	case LAND:
		return(eval(tp->t1) && eval(tp->t2));
	case LOR:
		return(eval(tp->t1) || eval(tp->t2));
	case AND:
		return(eval(tp->t1) & eval(tp->t2));
	case OR:
		return(eval(tp->t1) | eval(tp->t2));
	case XOR:
		return(eval(tp->t1) ^ eval(tp->t2));
	case FLONE:
		return(flone(eval(tp->t1)));
	case FRONE:
		return(frone(eval(tp->t1)));
	case GREY:
		return(btog(eval(tp->t1)));
	case NEG:
		return(-eval(tp->t1));
	case NOT:
		return(!eval(tp->t1));
	case COM:
		return(~eval(tp->t1));
	case ADD:
		return(eval(tp->t1) + eval(tp->t2));
	case SUB:
		return(eval(tp->t1) - eval(tp->t2));
	case MUL:
		return(eval(tp->t1) * eval(tp->t2));
	case DIV:
		return(eval(tp->t1) / eval(tp->t2));
	case MOD:
		return(eval(tp->t1) % eval(tp->t2));
	case LT:
		return(eval(tp->t1) < eval(tp->t2));
	case GT:
		return(eval(tp->t1) > eval(tp->t2));
	case EQ:
		return(eval(tp->t1) == eval(tp->t2));
	case NE:
		return(eval(tp->t1) != eval(tp->t2));
	case GE:
		return(eval(tp->t1) >= eval(tp->t2));
	case LE:
		return(eval(tp->t1) <= eval(tp->t2));
	case LS:
		return(eval(tp->t1) << eval(tp->t2));
	case RS:
		return(eval(tp->t1) >> eval(tp->t2));
	case CND:
		if (eval(tp->t1)) {
			tp = tp->t2;
			return(eval(tp->t1));
		}
		tp = tp->t2;
		return(eval(tp->t2));
	case SWITCH:
	{
		Value val, i;
		Node *def;

		def = 0;
		val = eval(tp->t1);
		i = 0;
		for(tp = tp->t2; tp; tp = tp->t2) {
			if(tp->code == ALT) {
				if(tp->t1) {
					i = eval(tp->t1);
					continue;
				}
				if(def != 0) {
					fprint(2, "more than one default\n");
#ifdef PLAN9
					exits("more than one default");
#else
					exit(1);
#endif
				}
				tp = tp->t2;
				if(tp == 0)
					break;
				if(tp->code != CASE)
					break;
				def = tp->t1;
				continue;
			}
			if(tp->code != CASE)
				break;
			if(i == val)
				return eval(tp->t1);
			i++;
		}
		if(def != 0)
			return eval(def);
		fprint(2, "decimal selector value %ld\n",val);
#ifdef PLAN9
		exits("decimal selector value ");
#else
		exit(1);
#endif
	}
	case EQN:
	case INPUT:
		hp = (Hshtab*)tp->t1;
		return(hp->val);
	case FIELD:
		hp = (Hshtab*)tp->t1;
		return(loadf(hp));
	case NUMBER:
		return((Value)tp->t1);
	default:
		fprint(2,"unknown eval op %d\n", tp->code);
#ifdef PLAN9
		exits("unknown eval op ");
#else
		exit(1);
#endif
	}
	return ~0;
}

void
assgn(Hshtab *hp, Value x, int dc)
{
	switch(hp->code) {

	case EQN:
		if(dc)
			break;
		hp->val = x;
		return;

	case BOTH:
	case OUTPUT:
		if(dc) {
			dcare[hp->oref-1] = x;
			return;
		}
		rom[hp->oref-1] = x;
		return;

	case FIELD:
		storef(hp, x, dc);
		return;
	}
	fprint(2,"unknown code in assgn %s\n", hp->name);
#ifdef PLAN9
	exits("unknown code in assgn");
#else
	exit(1);
#endif
}
