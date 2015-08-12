/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "mk.h"

Job *
newjob(Rule *r, Node *nlist, char *stem, char **match, Word *pre,
       Word *npre, Word *tar, Word *atar)
{
	register Job *j;

	j = (Job *)Malloc(sizeof(Job));
	j->r = r;
	j->n = nlist;
	j->stem = stem;
	j->match = match;
	j->p = pre;
	j->np = npre;
	j->t = tar;
	j->at = atar;
	j->nproc = -1;
	j->next = 0;
	return (j);
}

void
dumpj(char *s, Job *j, int all)
{
	Bprint(&bout, "%s\n", s);
	while(j) {
		Bprint(&bout, "job@%p: r=%p n=%p stem='%s' nproc=%d\n",
		       j, j->r, j->n, j->stem, j->nproc);
		Bprint(&bout, "\ttarget='%s' alltarget='%s' prereq='%s' nprereq='%s'\n",
		       wtos(j->t, ' '), wtos(j->at, ' '), wtos(j->p, ' '), wtos(j->np, ' '));
		j = all ? j->next : 0;
	}
}
