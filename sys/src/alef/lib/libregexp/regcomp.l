#include <alef.h>
#include <regexp.h>
#include "regcomp.h"

#define	TRUE	1
#define	FALSE	0

/*
 * Parser Information
 */
aggr Node
{
	Reinst*	first;
	Reinst*	last;
};

#define	NSTACK	20
intern	Node	andstack[NSTACK];
intern	Node	*andp;
intern	int	atorstack[NSTACK];
intern	int*	atorp;
intern	int	cursubid;		/* id of current subexpression */
intern	int	subidstack[NSTACK];	/* parallel to atorstack */
intern	int*	subidp;
intern	int	lastwasand;	/* Last token was operand */
intern	int	nbra;
intern	byte*	exprp;		/* pointer to next character in source expression */
intern	int	lexdone;
intern	int	nclass;
intern	Reclass*classp;
intern	Reinst*	freep;
intern	Rune	yyrune;		/* last lex'd rune */
intern	Reclass*yyclassp;	/* last lex'd class */
intern	chan(Reprog*) regchan;
intern	Reprog	*regpp;
intern	QLock	regexp;

/* predeclared crap */
intern	void	operator(int);
intern	void	pushand(Reinst*, Reinst*);
intern	void	pushator(int);
intern	void	evaluntil(int);
intern	int	bldcclass(void);

intern	void
rcerror(byte *s)
{
	free(regpp);
	regerror(s);
	regchan <-= nil;
	terminate(nil);
}

intern	Reinst*
newinst(int t)
{
	freep->type = t;
	freep->left = nil;
	freep->right = nil;
	return freep++;
}

intern	void
operand(int t)
{
	Reinst *i;

	if(lastwasand)
		operator(CAT);	/* catenate is implicit */
	i = newinst(t);

	if(t == CCLASS || t == NCCLASS)
		i->cp = yyclassp;
	if(t == RUNE)
		i->r = yyrune;

	pushand(i, i);
	lastwasand = TRUE;
}

intern	void
operator(int t)
{
	if(t==RBRA && --nbra<0)
		rcerror("unmatched right paren");
	if(t==LBRA){
		if(++cursubid >= NSUBEXP)
			rcerror ("too many subexpressions");
		nbra++;
		if(lastwasand)
			operator(CAT);
	} else
		evaluntil(t);
	if(t != RBRA)
		pushator(t);
	lastwasand = FALSE;
	if(t==STAR || t==QUEST || t==PLUS || t==RBRA)
		lastwasand = TRUE;	/* these look like operands */
}

intern	void
regerr2(byte *s, int c)
{
	byte buf[100];
	byte *cp;
	cp = buf;
	while(*s)
		*cp++ = *s++;
	*cp++ = c;
	*cp = '\0'; 
	rcerror(buf);
}

intern	void
cant(byte *s)
{
	byte buf[100];
	strcpy(buf, "can't happen: ");
	strcat(buf, s);
	rcerror(buf);
}

intern	void
pushand(Reinst *f, Reinst *l)
{
	if(andp >= &andstack[NSTACK])
		cant("operand stack overflow");
	andp->first = f;
	andp->last = l;
	andp++;
}

intern	void
pushator(int t)
{
	if(atorp >= &atorstack[NSTACK])
		cant("operator stack overflow");
	*atorp++ = t;
	*subidp++ = cursubid;
}

intern	Node*
popand(int op)
{
	Reinst *inst;

	if(andp <= &andstack[0]){
		regerr2("missing operand for ", op);
		inst = newinst(NOP);
		pushand(inst,inst);
	}
	return --andp;
}

intern	int
popator(void)
{
	if(atorp <= &atorstack[0])
		cant("operator stack underflow");
	--subidp;
	return *--atorp;
}

intern	void
evaluntil(int pri)
{
	Node *op1, *op2;
	Reinst *inst1, *inst2;

	while(pri==RBRA || atorp[-1]>=pri){
		switch(popator()){
		default:
			rcerror("unknown operator in evaluntil");
			break;
		case LBRA:		/* must have been RBRA */
			op1 = popand('(');
			inst2 = newinst(RBRA);
			inst2->subid = *subidp;
			op1->last->next = inst2;
			inst1 = newinst(LBRA);
			inst1->subid = *subidp;
			inst1->next = op1->first;
			pushand(inst1, inst2);
			return;
		case OR:
			op2 = popand('|');
			op1 = popand('|');
			inst2 = newinst(NOP);
			op2->last->next = inst2;
			op1->last->next = inst2;
			inst1 = newinst(OR);
			inst1->right = op1->first;
			inst1->left = op2->first;
			pushand(inst1, inst2);
			break;
		case CAT:
			op2 = popand(0);
			op1 = popand(0);
			op1->last->next = op2->first;
			pushand(op1->first, op2->last);
			break;
		case STAR:
			op2 = popand('*');
			inst1 = newinst(OR);
			op2->last->next = inst1;
			inst1->right = op2->first;
			pushand(inst1, inst1);
			break;
		case PLUS:
			op2 = popand('+');
			inst1 = newinst(OR);
			op2->last->next = inst1;
			inst1->right = op2->first;
			pushand(op2->first, inst1);
			break;
		case QUEST:
			op2 = popand('?');
			inst1 = newinst(OR);
			inst2 = newinst(NOP);
			inst1->left = inst2;
			inst1->right = op2->first;
			op2->last->next = inst2;
			pushand(inst1, inst2);
			break;
		}
	}
}

intern	Reprog*
optimize(Reprog *pp)
{
	Reinst *inst, *target;
	int size;
	Reprog *npp;
	int diff;
	Reclass *cl;

	/*
	 *  get rid of NOOP chains
	 */
	for(inst=pp->firstinst; inst->type!=END; inst++){
		target = inst->next;
		while(target->type == NOP)
			target = target->next;
		inst->next = target;
	}

	/*
	 *  The original allocation is for an area larger than
	 *  necessary.  Reallocate to the actual space used
	 *  and then relocate the code.
	 */
	size = sizeof(Reprog) + (freep - pp->firstinst)*sizeof(Reinst);
	npp = realloc(pp, size);
	if(npp==nil || npp==pp)
		return pp;
	diff = (byte *)npp - (byte *)pp;
	freep = (Reinst *)((byte *)freep + diff);
	for(inst=npp->firstinst; inst<freep; inst++){
		switch(inst->type){
		case OR:
		case STAR:
		case PLUS:
		case QUEST:
			*(byte **)&inst->right += diff;
			break;
		case CCLASS:
		case NCCLASS:
			*(byte **)&inst->right += diff;
			cl = inst->cp;
			*(byte **)&cl->end += diff;
			break;
		}
		*(byte **)&inst->left += diff;
	}
	*(byte **)&npp->startinst += diff;
	return npp;
}

#ifdef	DEBUG
intern	void
dumpstack(void){
	Node *stk;
	int *ip;

	print("operators\n");
	for(ip=atorstack; ip<atorp; ip++)
		print("0%o\n", *ip);
	print("operands\n");
	for(stk=andstack; stk<andp; stk++)
		print("0%o\t0%o\n", stk->first->type, stk->last->type);
}

intern	void
dump(Reprog *pp)
{
	Reinst *l;
	Rune *p;

	l = pp->firstinst;
	do{
		print("%d:\t0%o\t%d\t%d", l-pp->firstinst, l->type,
			l->left-pp->firstinst, l->right-pp->firstinst);
		if(l->type == RUNE)
			print("\t%C\n", l->r);
		else if(l->type == CCLASS || l->type == NCCLASS){
			print("\t[");
			if(l->type == NCCLASS)
				print("^");
			for(p = l->cp->spans; p < l->cp->end; p += 2)
				if(p[0] == p[1])
					print("%C", p[0]);
				else
					print("%C-%C", p[0], p[1]);
			print("]\n");
		} else
			print("\n");
	}while(l++->type);
}
#endif

intern	Reclass*
newclass(void)
{
	if(nclass >= NCLASS)
		regerr2("too many character classes; limit", NCLASS+'0');
	return &(classp[nclass++]);
}

intern	int
nextc(Rune *rp)
{
	if(lexdone){
		*rp = 0;
		return 1;
	}
	exprp += chartorune(rp, exprp);
	if(*rp == '\\'){
		exprp += chartorune(rp, exprp);
		return 1;
	}
	if(*rp == 0)
		lexdone = 1;
	return 0;
}

intern	int
lex(int literal, int dot_type)
{
	int quoted;

	quoted = nextc(&yyrune);
	if(literal || quoted){
		if(yyrune == 0)
			return END;
		return RUNE;
	}

	switch(yyrune){
	case 0:
		return END;
	case '*':
		return STAR;
	case '?':
		return QUEST;
	case '+':
		return PLUS;
	case '|':
		return OR;
	case '.':
		return dot_type;
	case '(':
		return LBRA;
	case ')':
		return RBRA;
	case '^':
		return BOL;
	case '$':
		return EOL;
	case '[':
		return bldcclass();
	}
	return RUNE;
}

intern int
bldcclass(void)
{
	int type;
	Rune r[NCCRUNE];
	Rune *p, *ep, *np;
	Rune rune;
	int quoted;

	/* we have already seen the '[' */
	type = CCLASS;
	yyclassp = newclass();

	/* look ahead for negation */
	/* SPECIAL CASE!!! negated classes don't match \n */
	ep = r;
	quoted = nextc(&rune);
	if(!quoted && rune == '^'){
		type = NCCLASS;
		quoted = nextc(&rune);
		*ep++ = '\n';
		*ep++ = '\n';
	}

	/* parse class into a set of spans */
	for(; ep<&r[NCCRUNE];){
		if(rune == 0){
			rcerror("malformed '[]'");
			return 0;
		}
		if(!quoted && rune == ']')
			break;
		if(!quoted && rune == '-'){
			if(ep == r){
				rcerror("malformed '[]'");
				return 0;
			}
			quoted = nextc(&rune);
			if((!quoted && rune == ']') || rune == 0){
				rcerror("malformed '[]'");
				return 0;
			}
			*(ep-1) = rune;
		} else {
			*ep++ = rune;
			*ep++ = rune;
		}
		quoted = nextc(&rune);
	}

	/* sort on span start */
	for(p = r; p < ep; p += 2){
		for(np = p; np < ep; np += 2)
			if(*np < *p){
				rune = np[0];
				np[0] = p[0];
				p[0] = rune;
				rune = np[1];
				np[1] = p[1];
				p[1] = rune;
			}
	}

	/* merge spans */
	np = yyclassp->spans;
	p = r;
	if(r == ep)
		yyclassp->end = np;
	else {
		np[0] = *p++;
		np[1] = *p++;
		for(; p < ep; p += 2)
			if(p[0] <= np[1]){
				if(p[1] > np[1])
					np[1] = p[1];
			} else {
				np += 2;
				np[0] = p[0];
				np[1] = p[1];
			}
		yyclassp->end = np+2;
	}

	return type;
}

intern	void
regcomp1(byte *s, int literal, int dot_type)
{
	int token;
	Reprog *pp;

	/* get memory for the program */
	pp = malloc(sizeof(Reprog) + 6*sizeof(Reinst)*strlen(s));
	if(pp == nil)
		rcerror("out of memory");
	regpp = pp;
	freep = pp->firstinst;
	classp = pp->class;

	/* go compile the sucker */
	lexdone = 0;
	exprp = s;
	nclass = 0;
	nbra = 0;
	atorp = atorstack;
	andp = andstack;
	subidp = subidstack;
	lastwasand = FALSE;
	cursubid = 0;

	/* Start with a low priority operator to prime parser */
	pushator(START-1);
	while((token = lex(literal, dot_type)) != END){
		if((token&0300) == OPERATOR)
			operator(token);
		else
			operand(token);
	}

	/* Close with a low priority operator */
	evaluntil(START);

	/* Force END */
	operand(END);
	evaluntil(START);
#ifdef DEBUG
	dumpstack();
#endif
	if(nbra)
		rcerror("unmatched left paren");
	--andp;	/* points to first and only operand */
	pp->startinst = andp->first;
#ifdef DEBUG
	dump(pp);
#endif
	pp = optimize(pp);
#ifdef DEBUG
	print("start: %d\n", andp->first-pp->firstinst);
	dump(pp);
#endif
out:
	regchan <-= pp;
}

Reprog*
regcomp2(byte *s, int a, int b)
{
	Reprog *r;

	regexp.lock();
	alloc regchan;
	task regcomp1(s, a, b);
	r = <-regchan;
	unalloc regchan;
	regexp.unlock();
	return r;
}

Reprog*
regcomp(byte *s)
{
	return regcomp2(s, 0, ANY);
}

Reprog*
regcomplit(byte *s)
{
	return regcomp2(s, 1, ANY);
}

Reprog*
regcompnl(byte *s)
{
	return regcomp2(s, 0, ANYNL);
}
