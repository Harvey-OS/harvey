/***** spin: guided.c *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

#include "spin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "y.tab.h"

extern int	nproc, nstop, Tval, Rvous, Have_claim, m_loss;
extern RunList	*run, *X;
extern Element	*Al_El;
extern Symbol	*Fname, *oFname;
extern int	verbose, lineno, xspin;
extern int	depth;

int	TstOnly = 0;

FILE *fd;

void
whichproc(int p)
{	RunList *oX;

	for (oX = run; oX; oX = oX->nxt)
		if (oX->pid == p)
		{	printf("(%s) ", oX->n->name);
			break;
		}
}

int
newer(char *f1, char *f2)
{	struct stat x, y;

	if (stat(f1, (struct stat *)&x) < 0) return 0;
	if (stat(f2, (struct stat *)&y) < 0) return 1;
	if (x.st_mtime < y.st_mtime) return 0;
	
	return 1;
}

void
match_trail(void)
{	int i, pno, nst;
	Element *dothis;
	char snap[256];

	sprintf(snap, "%s.trail", oFname->name);
	if (!(fd = fopen(snap, "r")))
	{	printf("spin -t: cannot find %s\n", snap);
		exit(1);
	}
			
	if (xspin == 0 && newer(oFname->name, snap))
	printf("Warning, \"%s\" newer than %s\n", oFname->name, snap);

	Tval = m_loss = 1; /* timeouts and losses may be part of trail */

	while (fscanf(fd, "%d:%d:%d\n", &depth, &pno, &nst) == 3)
	{	if (depth == -2)
		{	start_claim(pno);
			continue;
		}
		if (depth == -1)
		{	if (verbose)
			printf("<<<<<START OF CYCLE>>>>>\n");
			continue;
		}
		if (Have_claim)
		{	if (pno == 0)	/* verifier has claim at 0 */
				pno = Have_claim;
			else
			{	if (pno <= Have_claim)
					pno -= 1;
		}	}

		for (dothis = Al_El; dothis; dothis = dothis->Nxt)
		{	if (dothis->Seqno == nst)
				break;
		}
		if (!dothis)
		{	printf("%3d: proc %d, no matching transition %d\n",
				depth, pno, nst);
			lost_trail();
		}
		i = nproc - nstop;
		if (dothis->n->ntyp == '@')
		{	if (pno == i-1)
			{	run = run->nxt;
				nstop++;
				if (verbose&32 || verbose&4)
				printf("%3d: proc %d dies\n", depth, pno);
				continue;
			}
			if (pno <= 1) continue;	/* init dies before never */
			printf("%3d: stop error, proc %d (i=%d) trans %d, %c\n",
				depth, pno, i, nst, dothis->n->ntyp);
			lost_trail();
		}
		for (X = run; X; X = X->nxt)
		{	if (--i == pno)
				break;
		}
		if (!X)
		{	printf("%3d: no process %d ", depth, pno);
			if (Have_claim && pno == Have_claim)
				printf("(state %d)\n", nst);
			else
				printf("(proc .%d state %d)\n", pno, nst);
			lost_trail();
		}
		X->pc  = dothis;
		lineno = dothis->n->ln;
		Fname  = dothis->n->fn;
		if (verbose&32 || verbose&4)
		{	p_talk(X->pc, 1);
			printf("	[");
			comment(stdout, X->pc->n, 0);
			printf("]\n");
		}
		if (dothis->n->ntyp == GOTO)
			X->pc = get_lab(dothis->n,1);
		else
		{	if (dothis->n->ntyp == D_STEP)
			{	Element *g = dothis;
				do {
					g = eval_sub(g);
				} while (g && g != dothis->nxt);
				if (!g)
				{	printf("%3d: d_step failed %d->%d\n",
					depth, dothis->Seqno, dothis->nxt->Seqno);
					wrapup(1);
				}
				X->pc = g;
			} else
			{	(void) eval(dothis->n);
				X->pc = dothis->nxt;
			}
		}
		if (X->pc) X->pc = huntele(X->pc, 0);
		if (verbose&32 || verbose&4)
		{	if (verbose&1) dumpglobals();
			if (verbose&2) dumplocal(X);
			if (xspin) printf("\n");	/* xspin looks for these */
		}
	}
	printf("spin: trail ends after %d steps\n", depth);
	wrapup(0);
}

void
lost_trail(void)
{	int d, p, n, l;

	while (fscanf(fd, "%d:%d:%d:%d\n", &d, &p, &n, &l) == 4)
	{	printf("step %d: proc  %d ", d, p); whichproc(p);
		printf("(state %d) - d %d\n", n, l);
	}
	wrapup(1);	/* no return */
}

int
pc_value(Lextok *n)
{	int i = nproc - nstop;
	int pid = eval(n);
	RunList *Y;

	for (Y = run; Y; Y = Y->nxt)
	{	if (--i == pid)
			return Y->pc->seqno;
	}
	return 0;
}
