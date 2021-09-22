#include "a.h"

Ysym*
yynewsym(Ygram *g, char *name, char *type, int prec, int assoc)
{
	Ysym *s;
	Ysym **a;

	if(name){
		foreach(sym, s, g){
			if(s->name && strcmp(s->name, name) == 0){
				if((type==nil || s->type==nil) && (type!=nil || s->type!=nil))
					goto bad;
				if(s->prec != 2*prec || s->assoc != assoc)
					goto bad;
				return s;
			}
		}
	}

	s = mallocz(sizeof *s, 1);
	if(s == nil)
		return nil;
	s->n = g->nsym;
	if(name)
		s->name = strdup(name);
	if(type)
		s->type = strdup(type);
	s->prec = 2*prec;
	s->assoc = assoc;
	if(assoc != Ynonterm)
		s->flags |= YsymTerminal;

	if(g->nsym%32 == 0){
		a = realloc(g->sym, (g->nsym+32)*sizeof g->sym[0]);
		g->sym = a;
	}
	g->sym[g->nsym++] = s;
	return s;

bad:
//	werrstr("bad redeclaration of symbol %s", name);
//	if(g->debug)
//XXX		fprint(2, "yynewsym: %r\n");
	return nil;
}

Ysym*
yylooksym(Ygram *g, char *name)
{
	int i;

	for(i=0; i<g->nsym; i++)
		if(g->sym[i]->name && strcmp(g->sym[i]->name, name) == 0)
			return g->sym[i];
	// werrstr("unknown symbol %s", name);
	if(g->debug)
		fprint(2, "%r\n");
	return nil;
}

