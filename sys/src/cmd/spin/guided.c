/***** spin: guided.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include "spin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "y.tab.h"

extern RunList	*run, *X;
extern Element	*Al_El;
extern Symbol	*Fname, *oFname;
extern int	verbose, lineno, xspin, jumpsteps, depth, merger, cutoff;
extern int	nproc, nstop, Tval, ntrail, columns;
extern short	Have_claim, Skip_claim;
extern void ana_src(int, int);

int	TstOnly = 0, pno;

static int	lastclaim = -1;
static FILE	*fd;
static void	lost_trail(void);

static void
whichproc(int p)
{	RunList *oX;

	for (oX = run; oX; oX = oX->nxt)
		if (oX->pid == p)
		{	printf("(%s) ", oX->n->name);
			break;
		}
}

static int
newer(char *f1, char *f2)
{
#if defined(WIN32) || defined(WIN64)
	struct _stat x, y;
#else
	struct stat x, y;
#endif

	if (stat(f1, (struct stat *)&x) < 0) return 0;
	if (stat(f2, (struct stat *)&y) < 0) return 1;
	if (x.st_mtime < y.st_mtime) return 0;
	
	return 1;
}

void
hookup(void)
{	Element *e;

	for (e = Al_El; e; e = e->Nxt)
		if (e->n
		&& (e->n->ntyp == ATOMIC
		||  e->n->ntyp == NON_ATOMIC
		||  e->n->ntyp == D_STEP))
			(void) huntstart(e);
}

int
not_claim(void)
{
	return (!Have_claim || !X || X->pid != 0);
}

void
match_trail(void)
{	int i, a, nst;
	Element *dothis;
	char snap[512], *q;

	/*
	 * if source model name is leader.pml
	 * look for the trail file under these names:
	 *	leader.pml.trail
	 *	leader.pml.tra
	 *	leader.trail
	 *	leader.tra
	 */

	if (ntrail)
		sprintf(snap, "%s%d.trail", oFname->name, ntrail);
	else
		sprintf(snap, "%s.trail", oFname->name);

	if ((fd = fopen(snap, "r")) == NULL)
	{	snap[strlen(snap)-2] = '\0';	/* .tra */
		if ((fd = fopen(snap, "r")) == NULL)
		{	if ((q = strchr(oFname->name, '.')) != NULL)
			{	*q = '\0';
				if (ntrail)
					sprintf(snap, "%s%d.trail",
						oFname->name, ntrail);
				else
					sprintf(snap, "%s.trail",
						oFname->name);
				*q = '.';

				if ((fd = fopen(snap, "r")) != NULL)
					goto okay;

				snap[strlen(snap)-2] = '\0';	/* last try */
				if ((fd = fopen(snap, "r")) != NULL)
					goto okay;
			}
			printf("spin: cannot find trail file\n");
			alldone(1);
	}	}
okay:		
	if (xspin == 0 && newer(oFname->name, snap))
	printf("spin: warning, \"%s\" is newer than %s\n",
		oFname->name, snap);

	Tval = 1;

	/*
	 * sets Tval because timeouts may be part of trail
	 * this used to also set m_loss to 1, but that is
	 * better handled with the runtime -m flag
	 */

	hookup();

	while (fscanf(fd, "%d:%d:%d\n", &depth, &pno, &nst) == 3)
	{	if (depth == -2) { start_claim(pno); continue; }
		if (depth == -4) { merger = 1; ana_src(0, 1); continue; }
		if (depth == -1)
		{	if (verbose)
			{	if (columns == 2)
				dotag(stdout, " CYCLE>\n");
				else
				dotag(stdout, "<<<<<START OF CYCLE>>>>>\n");
			}
			continue;
		}

		if (cutoff > 0 && depth >= cutoff)
		{	printf("-------------\n");
			printf("depth-limit (-u%d steps) reached\n", cutoff);
			break;
		}

		if (Skip_claim && pno == 0) continue;

		for (dothis = Al_El; dothis; dothis = dothis->Nxt)
		{	if (dothis->Seqno == nst)
				break;
		}
		if (!dothis)
		{	printf("%3d: proc %d, no matching stmnt %d\n",
				depth, pno - Have_claim, nst);
			lost_trail();
		}

		i = nproc - nstop + Skip_claim;

		if (dothis->n->ntyp == '@')
		{	if (pno == i-1)
			{	run = run->nxt;
				nstop++;
				if (verbose&4)
				{	if (columns == 2)
					{	dotag(stdout, "<end>\n");
						continue;
					}
					if (Have_claim && pno == 0)
					printf("%3d: claim terminates\n",
						depth);
					else
					printf("%3d: proc %d terminates\n",
						depth, pno - Have_claim);
				}
				continue;
			}
			if (pno <= 1) continue;	/* init dies before never */
			printf("%3d: stop error, ", depth);
			printf("proc %d (i=%d) trans %d, %c\n",
				pno - Have_claim, i, nst, dothis->n->ntyp);
			lost_trail();
		}
		for (X = run; X; X = X->nxt)
		{	if (--i == pno)
				break;
		}
		if (!X)
		{	printf("%3d: no process %d ", depth, pno - Have_claim);
			printf("(state %d)\n", nst);
			lost_trail();
		}
		X->pc  = dothis;
		lineno = dothis->n->ln;
		Fname  = dothis->n->fn;

		if (dothis->n->ntyp == D_STEP)
		{	Element *g, *og = dothis;
			do {
				g = eval_sub(og);
				if (g && depth >= jumpsteps
				&& ((verbose&32) || ((verbose&4) && not_claim())))
				{	if (columns != 2)
					{	p_talk(og, 1);
		
						if (og->n->ntyp == D_STEP)
						og = og->n->sl->this->frst;
		
						printf("\t[");
						comment(stdout, og->n, 0);
						printf("]\n");
					}
					if (verbose&1) dumpglobals();
					if (verbose&2) dumplocal(X);
					if (xspin) printf("\n");
				}
				og = g;
			} while (g && g != dothis->nxt);
			if (X != NULL)
			{	X->pc = g?huntele(g, 0, -1):g;
			}
		} else
		{
keepgoing:		if (dothis->merge_start)
				a = dothis->merge_start;
			else
				a = dothis->merge;

			if (X != NULL)
			{	X->pc = eval_sub(dothis);
				if (X->pc) X->pc = huntele(X->pc, 0, a);
			}

			if (depth >= jumpsteps
			&& ((verbose&32) || ((verbose&4) && not_claim())))	/* -v or -p */
			{	if (columns != 2)
				{	p_talk(dothis, 1);
	
					if (dothis->n->ntyp == D_STEP)
					dothis = dothis->n->sl->this->frst;
		
					printf("\t[");
					comment(stdout, dothis->n, 0);
					printf("]");
					if (a && (verbose&32))
					printf("\t<merge %d now @%d>",
						dothis->merge,
						(X && X->pc)?X->pc->seqno:-1);
					printf("\n");
				}
				if (verbose&1) dumpglobals();
				if (verbose&2) dumplocal(X);
				if (xspin) printf("\n");

				if (X && !X->pc)
				{	X->pc = dothis;
					printf("\ttransition failed\n");
					a = 0;	/* avoid inf loop */
				}
			}
			if (a && X && X->pc && X->pc->seqno != a)
			{	dothis = X->pc;
				goto keepgoing;
		}	}

		if (Have_claim && X && X->pid == 0
		&&  dothis && dothis->n
		&&  lastclaim != dothis->n->ln)
		{	lastclaim = dothis->n->ln;
			if (columns == 2)
			{	char t[128];
				sprintf(t, "#%d", lastclaim);
				pstext(0, t);
			} else
			{
				printf("Never claim moves to line %d\t[", lastclaim);
				comment(stdout, dothis->n, 0);
				printf("]\n");
	}	}	}
	printf("spin: trail ends after %d steps\n", depth);
	wrapup(0);
}

static void
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
