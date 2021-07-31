#include	<stdio.h>
#include	<ctype.h>
#include	"pic.h"
#include	"y.tab.h"

YYSTYPE getvar(s)	/* return value of variable s (usually pointer) */
	char *s;
{
	struct symtab *p;
	static YYSTYPE bug;

	p = lookup(s);
	if (p == NULL) {
		if (islower(s[0]))
			ERROR "no such variable as %s", s WARNING;
		else
			ERROR "no such place as %s", s WARNING;
		return(bug);
	}
	return(p->s_val);
}

double getfval(s)	/* return float value of variable s */
	char *s;
{
	YYSTYPE y;

	y = getvar(s);
	return y.f;
}

setfval(s, f)	/* set variable s to f */
	char *s;
	double f;
{
	struct symtab *p;

	if ((p = lookup(s)) != NULL)
		p->s_val.f = f;
}

struct symtab *makevar(s, t, v)	/* make variable named s in table */
	char *s;		/* assumes s is static or from tostring */
	int t;
	YYSTYPE v;
{
	struct symtab *p;

	for (p = stack[nstack].p_symtab; p != NULL; p = p->s_next)
		if (strcmp(s, p->s_name) == 0)
			break;
	if (p == NULL) {	/* it's a new one */
		p = (struct symtab *) malloc(sizeof(struct symtab));
		if (p == NULL)
			ERROR "out of symtab space with %s", s FATAL;
		p->s_next = stack[nstack].p_symtab;
		stack[nstack].p_symtab = p;	/* stick it at front */
	}
	p->s_name = s;
	p->s_type = t;
	p->s_val = v;
	return(p);
}

struct symtab *lookup(s)	/* find s in symtab */
	char *s;
{
	int i;
	struct symtab *p;

	for (i = nstack; i >= 0; i--)	/* look in each active symtab */
		for (p = stack[i].p_symtab; p != NULL; p = p->s_next)
			if (strcmp(s, p->s_name) == 0)
				return(p);
	return(NULL);
}

freesymtab(p)	/* free space used by symtab at p */
	struct symtab *p;
{
	struct symtab *q;

	for ( ; p != NULL; p = q) {
		q = p->s_next;
		free(p->s_name);	/* assumes done with tostring */
		free((char *)p);
	}
}

freedef(s)	/* free definition for string s */
	char *s;
{
	struct symtab *p, *q, *op;

	for (p = op = q = stack[nstack].p_symtab; p != NULL; p = p->s_next) {
		if (strcmp(s, p->s_name) == 0) { 	/* got it */
			if (p->s_type != DEFNAME)
				break;
			if (p == op)	/* 1st elem */
				stack[nstack].p_symtab = p->s_next;
			else
				q->s_next = p->s_next;
			free(p->s_name);
			free(p->s_val.p);
			free((char *)p);
			return;
		}
		q = p;
	}
	/* ERROR "%s is not defined at this point", s WARNING; */
}
