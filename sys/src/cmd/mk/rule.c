#include	"mk.h"

static Rule *lr, *lmr;
static rcmp(Rule *r, char *target, Word *tail);
static int nrules = 0;

void
addrule(char *head, Word *tail, char *body, Word *ahead, int attr, int hline, int override, char *prog)
{
	Rule *r;
	Rule *rr;
	Symtab *sym;
	int reuse;

	if(sym = symlook(head, S_TARGET, (char *)0)){
		for(r = (Rule *)sym->value; r; r = r->chain)
			if(rcmp(r, head, tail) == 0) break;
		if(r && !override)
			return;
	} else
		r = 0;
	reuse = r != 0;
	if(r == 0)
		r = (Rule *)Malloc(sizeof(Rule));
	r->target = head;
	r->tail = tail;
	r->recipe = body;
	r->line = hline;
	r->file = infile;
	r->attr = attr;
	r->alltargets = ahead;
	r->prog = prog;
	r->rule = nrules++;
	if(!reuse){
		rr = (Rule *)symlook(head, S_TARGET, (char *)r)->value;
		if(rr != r){
			r->chain = rr->chain;
			rr->chain = r;
		} else
			r->chain = 0;
	}
	if(utfrune(head, '%') || utfrune(head, '&') || (attr&REGEXP))
		goto meta;
	if(reuse)
		return;
	r->next = 0;
	r->pat = 0;
	if(rules == 0)
		rules = lr = r;
	else {
		lr->next = r;
		lr = r;
	}
	return;
meta:
	r->attr |= META;
	if(reuse)
		return;
	r->next = 0;
	if(r->attr&REGEXP){
		patrule = r;
		r->pat = regcomp(head);
	}
	if(metarules == 0)
		metarules = lmr = r;
	else {
		lmr->next = r;
		lmr = r;
	}
}

void
dumpr(char *s, Rule *r)
{
	Bprint(&stdout, "%s: start=%ld\n", s, r);
	for(; r; r = r->next){
		Bprint(&stdout, "\tRule %ld: %s[%d] attr=%x next=%ld chain=%ld alltarget='%s'",
			r, r->file, r->line, r->attr, r->next, r->chain, wtos(r->alltargets));
		if(r->prog)
			Bprint(&stdout, " prog='%s'", r->prog);
		Bprint(&stdout, "\n\ttarget=%s: %s\n", r->target, wtos(r->tail));
		Bprint(&stdout, "\trecipe@%ld='%s'\n", r->recipe, r->recipe);
	}
}

void
frule(Word *w)
{
	extern Word *target1;
	Word *ww;
	char *s;

#define	ADD(s)	{if(target1==0)target1=ww=newword(s);else ww=ww->next=newword(s);}

	for(ww = w; ww; ww = ww->next)
		if(utfrune(w->s, '%') || utfrune(w->s, '&'))
			return;	/* no metarule targets */
	while(w && w->s){
		if(s = utfrune(w->s, '+')){
			*s++ = 0;
			if(*w->s)
				ADD(w->s);
			if(*s)
				ADD(s);
			s[-1] = '+';
		} else
			ADD(w->s);
		w = w->next;
	}
}

static int
rcmp(Rule *r, char *target, Word *tail)
{
	Word *w;

	if(strcmp(r->target, target))
		return(1);
	for(w = r->tail; w && tail; w = w->next, tail = tail->next)
		if(strcmp(w->s, tail->s))
			return(1);
	return(w || tail);
}

char *
rulecnt(void)
{
	char *s;

	s = Malloc(nrules);
	memset(s, 0, nrules);
	return(s);
}

int
ismeta(char *s)
{
	Rune r;

	while(*s) {
		s += chartorune(&r, s);
		switch(r)
		{
		case '\'':
			while (*s) {
				s += chartorune(&r, s);
				if (r == '\'')
					break;
			}
			break;
		case '[':
		case '*':
		case '?':
			return(1);
		default:
			break;
		}
	}
	return(0);
}
