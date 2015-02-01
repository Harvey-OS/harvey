/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pic.h"
#include "y.tab.h"

#define	SLOP	1.001

typedef struct {
	char	*var;	/* index variable */
	double	to;	/* limit */
	double	by;
	int	op;	/* operator */
	char	*str;	/* string to push back */
} For;

For	forstk[10];	/* stack of for loops */
For	*forp = forstk;	/* pointer to current top */

void	setfval(char *, double);
void	nextfor(void);

void forloop(char *var, double from, double to, int op,
	double by, char *str)	/* set up a for loop */
{
	dprintf("# for %s from %g to %g by %c %g \n",
		var, from, to, op, by);
	if (++forp >= forstk+10)
		ERROR "for loop nested too deep" FATAL;
	forp->var = var;
	forp->to = to;
	forp->op = op;
	forp->by = by;
	forp->str = str;
	setfval(var, from);
	nextfor();
	unput('\n');
}

void nextfor(void)	/* do one iteration of a for loop */
{
	/* BUG:  this should depend on op and direction */
	if (getfval(forp->var) > SLOP * forp->to) {	/* loop is done */
		free(forp->str);
		if (--forp < forstk)
			ERROR "forstk popped too far" FATAL;
	} else {		/* another iteration */
		pushsrc(String, "\nEndfor\n");
		pushsrc(String, forp->str);
	}
}

void endfor(void)	/* end one iteration of for loop */
{
	struct symtab *p = lookup(forp->var);

	switch (forp->op) {
	case '+':
	case ' ':
		p->s_val.f += forp->by;
		break;
	case '-':
		p->s_val.f -= forp->by;
		break;
	case '*':
		p->s_val.f *= forp->by;
		break;
	case '/':
		p->s_val.f /= forp->by;
		break;
	}
	nextfor();
}

char *ifstat(double expr, char *thenpart, char *elsepart)
{
	dprintf("if %g then <%s> else <%s>\n", expr, thenpart, elsepart? elsepart : "");
	if (expr) {
		unput('\n');
		pushsrc(Free, thenpart);
		pushsrc(String, thenpart);
		unput('\n');
  		if (elsepart)
			free(elsepart);
		return thenpart;	/* to be freed later */
	} else {
		free(thenpart);
		if (elsepart) {
			unput('\n');
			pushsrc(Free, elsepart);
			pushsrc(String, elsepart);
			unput('\n');
		}
		return elsepart;
	}
}
