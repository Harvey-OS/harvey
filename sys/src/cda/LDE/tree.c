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
		fprintf(stderr,"out of treespace\n");
		exits("error");;
	}
	tp = &nodes[nextnode++];
	tp->code = op;
	tp->t1 = left;
	tp->t2 = right;
	tp->Cone = 0;
	tp->Czero = 0;
	return(tp);
}

void
treeprint(Node *tp)
{
	char *op;
	extern int yylineno;

	if (tp==0) {
		fprintf(stdout, "<>");
		return;
	}
	op = 0;
	switch(tp->code) {
	case 0:
		return;
	case ELIST:
		treeprint(tp->t1);
		fprintf(stdout, "\n");
		treeprint(tp->t2);
		return;
	case ASSIGN:
		hp = (Hshtab *)tp->t1;
		fprintf(stdout, " %s = ", hp->name);
		treeprint(tp->t2);
		return;
	case DONTCARE:
		hp = (Hshtab *)tp->t1;
		fprintf(stdout, " %s' = ", hp->name);
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
		fprintf(stdout, op);
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
		fprintf(stdout, "if (");
		treeprint(tp->t1);
		fprintf(stdout, ")");
		treeprint(tp->t2);
		return;
	case ALT:
		fprintf(stdout, "then ");
		treeprint(tp->t1);
		fprintf(stdout, "else ");
		treeprint(tp->t2);
		return;
	case EQN:
	case INPUT:
	case FIELD:
		hp = (Hshtab *)tp->t1;
		fprintf(stdout, " %s ",hp->name);
		return;
	case NUMBER:
		fprintf(stdout, " %d ",tp->t1);
		return;
	case SWITCH:
		fprintf(stdout, "SW "); treeprint(tp->t1);
		treeprint(tp->t2);
		return;
	case CASE:
		fprintf(stdout, "case"); treeprint(tp->t1);
		treeprint(tp->t2);
		return;
	default:
		fprintf(stderr,"line %d bad op code %d\n",yylineno,tp->code);
		exits("error");;
	}
	fprintf(stdout, "%s(",op);
	treeprint(tp->t1);
	treeprint(tp->t2);
	fprintf(stdout, ")");
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
		fprintf(stderr,"line %d bad op code %d\n",yylineno,tp->code);
		exits("error");;
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
		fprintf(stderr, "eval: case ASSIGN\n");
		return(~0L);
	case DONTCARE:
		fprintf(stderr, "eval: case DONTCARE\n");
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
					fprintf(stderr, "more than one default\n");
					exits("error");;
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
		fprintf(stderr, "decimal selector value %ld\n",val);
		exits("error");;
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
		fprintf(stderr,"unknown eval op %d\n", tp->code);
		exits("error");;
	}
	return ~0;
}

#define ISCONST(x)  (((x)->Cone | (x)->Czero) == ~0)

void
Ceval(Node *tp)
{
	unsigned int i1, i2;
	Node * tp0;
	if(tp->Cone ||  tp->Czero) return;
	switch(tp->code) {
	case 0:
		return;
	case ELIST:
		if(tp->t1 != 0)
			Ceval(tp->t1);
		if(tp->t2 != 0)
			Ceval(tp->t2);
		tp->Czero = tp->Cone = 0;
		return;
	case ASSIGN:
		Ceval(tp->t2);
		tp->Czero = tp->t2->Czero;
		tp->Cone = tp->t2->Cone;
		return;
	case DONTCARE:
		Ceval(tp->t2);
		tp->Czero = tp->t2->Czero;
		tp->Cone = tp->t2->Cone;
		return;
	case AND:
		Ceval(tp->t2);
		Ceval(tp->t1);
		tp->Czero = tp->t2->Czero | tp->t1->Czero;
		tp->Cone = tp->t2->Cone & tp->t1->Cone;
		return;
	case OR:
		Ceval(tp->t2);
		Ceval(tp->t1);
		tp->Czero = tp->t2->Czero & tp->t1->Czero;
		tp->Cone = tp->t2->Cone | tp->t1->Cone;
		return;
	case LAND:
		Ceval(tp->t2);
		Ceval(tp->t1);
		tp->Czero = (~0 << 1) | ((tp->t2->Czero == ~0) || (tp->t1->Czero == ~0));
		tp->Cone = (tp->t1->Cone) && (tp->t2->Cone);
		return;
	case LOR:
		Ceval(tp->t2);
		Ceval(tp->t1);
		tp->Czero = (~0 << 1) | ((tp->t2->Czero == ~0) && (tp->t1->Czero == ~0));
		tp->Cone = (tp->t1->Cone) ||(tp->t2->Cone);
		return;
	case NEG:
		Ceval(tp->t1);
		tp->Cone = tp->Czero = 0;
		if(ISCONST(tp->t1))
			tp->Czero = ~(tp->Cone = -tp->t1->Cone);
		return;
	case XOR:
		Ceval(tp->t2);
		Ceval(tp->t1);
		tp->Czero = (tp->t2->Czero & tp->t1->Czero)
			| (tp->t2->Cone & tp->t1->Cone);
		tp->Cone = (tp->t2->Czero & tp->t1->Cone)
			| (tp->t2->Cone & tp->t1->Czero);
		return;
	case FLONE:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done find left one for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1);
		tp->Czero = ~tp->Cone;
		return;
	case FRONE:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done find right one for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1);
		tp->Czero = ~tp->Cone;
		return;
	case GREY:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done grey code for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1);
		tp->Czero = ~tp->Cone;
		return;
	case NOT:
		Ceval(tp->t1);
		tp->Cone = (tp->t1->Czero == ~0);
		tp->Czero = (tp->t1->Cone) ? ~0 : (~0 << 1);
		return;
	case COM:
		Ceval(tp->t1);
		tp->Cone = (tp->t1->Czero);
		tp->Czero = (tp->t1->Cone);
		return;
	case ADD:
		Ceval(tp->t2);
		Ceval(tp->t1);
		i1 = tp->t1->Cone | tp->t1->Czero;
		i1 = i1 & ~(i1 + 1);
		i2 = tp->t2->Cone | tp->t2->Czero;
		i2 = i2 & ~(i2 + 1);
		tp->Cone = i1 & i2 & (tp->t1->Cone + tp->t2->Cone);
		tp->Czero = i1 & i2 & ~tp->Cone;
		for(i1 = 0; ; i1 = i1<<1 | 1)
			if(((tp->t1->Czero | i1) == ~0)
				&& ((tp->t2->Czero | i1) == ~0)) break;
		tp->Czero |= ~(i1<<1 | 1);
		return;
	case SUB:
		Ceval(tp->t2);
		Ceval(tp->t1);
		i1 = tp->t1->Cone | tp->t1->Czero;
		i1 = i1 & ~(i1 + 1);
		i2 = tp->t2->Cone | tp->t2->Czero;
		i2 = i2 & ~(i2 + 1);
		tp->Cone = i1 & i2 & (tp->t1->Cone + tp->t2->Cone);
		tp->Czero = i1 & i2 & ~tp->Cone;
		for(i1 = 0; ; i1 = i1<<1 | 1)
			if(((tp->t1->Czero | i1) == ~0)
				&& ((tp->t2->Czero | i1) == ~0)) break;
		i2 = i1 ^ i1>>1;
		if((tp->t1->Cone & i2) && (tp->t2->Czero & i2)) 
			tp->Czero |= ~(i1<<1 | 1);
		else if((tp->t1->Czero & i2) && (tp->t2->Cone & i2)) 
			tp->Cone |= ~(i1<<1 | 1);
		return;
	case MUL:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done mult for variable\n");
			exits("error");;
		}
		if(!vconst(tp->t2)) {
			fprintf(stderr, "Restrick hasn't done mult for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1) * eval(tp->t2);
		tp->Czero = ~tp->Cone;
		return;
	case DIV:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done div for variable\n");
			exits("error");;
		}
		if(!vconst(tp->t2)) {
			fprintf(stderr, "Restrick hasn't done div for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1) / eval(tp->t2);
		tp->Czero = ~tp->Cone;
		return;
	case MOD:
		if(!vconst(tp->t1)) {
			fprintf(stderr, "Restrick hasn't done mod for variable\n");
			exits("error");;
		}
		if(!vconst(tp->t2)) {
			fprintf(stderr, "Restrick hasn't done mod for variable\n");
			exits("error");;
		}
		tp->Cone = eval(tp->t1) % eval(tp->t2);
		tp->Czero = ~tp->Cone;
		return;
	case GT:
	case LT:
	case GE:
	case LE:
		Ceval(tp->t2);
		Ceval(tp->t1);
		for(i1 = 0; ; i1 = i1<<1 | 1)
			if(((tp->t1->Czero | tp->t1->Cone | i1) == ~0)
				&& ((tp->t2->Czero | tp->t2->Cone | i1) == ~0)) break;
		i1 = ~i1;
		switch(tp->code) {
		case LT:
		case LE:
			tp->Cone = (tp->t1->Cone & i1) < (tp->t2->Cone & i1);
			tp->Czero = (tp->t1->Cone & i1) > (tp->t2->Cone & i1);
			break;
		case GT:
		case GE:
			tp->Cone = (tp->t1->Cone & i1) > (tp->t2->Cone & i1);
			tp->Czero = (tp->t1->Cone & i1) < (tp->t2->Cone & i1);
		}
		return;
	case EQ:
		Ceval(tp->t2);
		Ceval(tp->t1);
		if(ISCONST(tp->t1) && ISCONST(tp->t2)) {
			tp->Cone = (tp->t1->Cone == tp->t2->Cone) ? 1 : 0;
			tp->Czero = ~tp->Cone;
		}
		else {
			i1 = (tp->t1->Cone | tp->t1->Czero)
				& (tp->t2->Cone | tp->t2->Czero);
			tp->Czero = (i1 & (tp->t1->Cone ^ tp->t2->Cone)) ?
				~0 : ~1;
			tp->Cone = 0;
		}
		return;
	case NE:
		Ceval(tp->t2);
		Ceval(tp->t1);
		if(ISCONST(tp->t1) && ISCONST(tp->t2)) {
			tp->Cone = (tp->t1->Cone != tp->t2->Cone) ? 1 : 0;
			tp->Czero = ~tp->Cone;
		}
		else {
			tp->Czero = ~1;
			i1 = (tp->t1->Cone | tp->t1->Czero)
				& (tp->t2->Cone | tp->t2->Czero);
			tp->Cone = (i1 & (tp->t1->Cone ^ tp->t2->Cone)) ?
				1 : 0;
		}
		return;
	case LS:
		Ceval(tp->t2);
		Ceval(tp->t1);
		if(ISCONST(tp->t2)) {
			tp->Cone = tp->t1->Cone << tp->t2->Cone;
			tp->Czero = tp->t1->Czero << tp->t2->Cone;
			i1 = ~0 << tp->t2->Cone;
			tp->Czero |= ~i1;
			return;
		} 
		tp->Cone = tp->Czero = 0;
		return;
	case RS:
		Ceval(tp->t2);
		Ceval(tp->t1);
		if(ISCONST(tp->t2)) {
			tp->Cone = tp->t1->Cone >> tp->t2->Cone;
			tp->Czero = tp->t1->Czero >> tp->t2->Cone;
			i1 = ~0 >> tp->t2->Cone;
			tp->Czero |= ~i1;
			return;
		} 
		tp->Cone = tp->Czero = 0;
		return;
	case CND:
		Ceval(tp->t1);
		Ceval(tp->t2->t1);
		Ceval(tp->t2->t2);
		if(ISCONST(tp->t1)) {
			if (tp->t1->Cone) {
				tp->Cone = tp->t2->t1->Cone;
				tp->Czero = tp->t2->t1->Czero;
				return;
			}
			tp->Cone = tp->t2->t2->Cone;
			tp->Czero = tp->t2->t2->Czero;
			return;
		}
		tp->Cone = tp->t2->t1->Cone & tp->t2->t2->Cone;
		tp->Czero = tp->t2->t1->Czero & tp->t2->t2->Czero;
		return;
	case SWITCH:
	{
		int o, z;
		if(vconst(tp)) {
			tp->Cone = eval(tp);
			tp->Czero = ~tp->Cone;
			return;
		}
		Ceval(tp->t1);
		tp0 = tp;
		o = z = ~0;
		for(tp = tp->t2; tp; tp = tp->t2) {
			if(tp->code == ALT) {
				if(tp->t1) {
					Ceval(tp->t1);
					continue;
				}
				tp = tp->t2;
				if(tp == 0)
					break;
				if(tp->code != CASE)
					break;
				Ceval(tp->t1);
				o &= tp->t1->Cone;
				z &= tp->t1->Czero;
				continue;
			}
			if(tp->code != CASE)
				break;
			Ceval(tp->t1);
			o &= tp->t1->Cone;
			z &= tp->t1->Czero;
		}
		tp0->Cone = o;
		tp0->Czero = z;
		return;	
	}
	case EQN:
		tp0 = ((Hshtab *) (tp->t1))->assign;
		Ceval(tp0);
		tp->Cone = tp0->Cone;
		tp->Czero = tp0->Czero;
		return;
	case BOTH:
	case INPUT:
		tp->Czero = ~1;
		tp->Cone = 0;
		return;
	case FIELD:
		i1 =  HI((Hshtab *) tp->t1) - LO((Hshtab *) tp->t1);
		tp->Cone = 0;
		tp->Czero = (i1 > 31) ? 0 : ( ~0 << i1);
		return;
	case NUMBER:
		tp->Cone = (int) tp->t1;
		tp->Czero = ~tp->Cone;
		return;
	default:
		fprintf(stderr,"unknown Ceval op %d\n", tp->code);
		exits("error");;
	}
}
