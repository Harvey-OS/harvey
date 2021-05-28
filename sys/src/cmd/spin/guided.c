/***** spin: guided.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "y.tab.h"

extern RunList	*run_lst, *X_lst;
extern Element	*Al_El;
extern Symbol	*Fname, *oFname;
extern int	verbose, lineno, xspin, jumpsteps, depth, merger, cutoff;
extern int	nproc, nstop, Tval, ntrail, columns;
extern short	Have_claim, Skip_claim, has_code;
extern void ana_src(int, int);
extern char	**trailfilename;

int	TstOnly = 0, prno;

static int	lastclaim = -1;
static FILE	*fd;
static void	lost_trail(void);

static void
whichproc(int p)
{	RunList *oX;

	for (oX = run_lst; oX; oX = oX->nxt)
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
	return (!Have_claim || !X_lst || X_lst->pid != 0);
}

int globmin = INT_MAX;
int globmax = 0;

int
find_min(Sequence *s)
{	SeqList *l;
	Element *e;

	if (s->minel < 0)
	{	s->minel = INT_MAX;
		for (e = s->frst; e; e = e->nxt)
		{	if (e->status & 512)
			{	continue;
			}
			e->status |= 512;

			if (e->n->ntyp == ATOMIC
			||  e->n->ntyp == NON_ATOMIC
			||  e->n->ntyp == D_STEP)
			{	int n = find_min(e->n->sl->this);
				if (n < s->minel)
				{	s->minel = n;
				}
			} else if (e->Seqno < s->minel)
			{	s->minel = e->Seqno;
			}
			for (l = e->sub; l; l = l->nxt)
			{	int n = find_min(l->this);
				if (n < s->minel)
				{	s->minel = n;
		}	}	}
	}
	if (s->minel < globmin)
	{	globmin = s->minel;
	}
	return s->minel;
}

int
find_max(Sequence *s)
{
	if (s->last->Seqno > globmax)
	{	globmax = s->last->Seqno;
	}
	return s->last->Seqno;
}

void
match_trail(void)
{	int i, a, nst;
	Element *dothis;
	char snap[512], *q;

	if (has_code)
	{	printf("spin: important:\n");
		printf("  =======================================warning====\n");
		printf("  this model contains embedded c code statements\n");
		printf("  these statements will not be executed when the trail\n");
		printf("  is replayed in this way -- they are just printed,\n");
		printf("  which will likely lead to inaccurate variable values.\n");
		printf("  for an accurate replay use: ./pan -r\n");
		printf("  =======================================warning====\n\n");
	}

	/*
	 * if source model name is leader.pml
	 * look for the trail file under these names:
	 *	leader.pml.trail
	 *	leader.pml.tra
	 *	leader.trail
	 *	leader.tra
	 */

	if (trailfilename)
	{	if (strlen(*trailfilename) < sizeof(snap))
		{	strcpy(snap, (const char *) *trailfilename);
		} else
		{	fatal("filename %s too long", *trailfilename);
		}
	} else
	{	if (ntrail)
			sprintf(snap, "%s%d.trail", oFname->name, ntrail);
		else
			sprintf(snap, "%s.trail", oFname->name);
	}

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
	{	printf("spin: warning, \"%s\" is newer than %s\n",
			oFname->name, snap);
	}
	Tval = 1;

	/*
	 * sets Tval because timeouts may be part of trail
	 * this used to also set m_loss to 1, but that is
	 * better handled with the runtime -m flag
	 */

	hookup();

	while (fscanf(fd, "%d:%d:%d\n", &depth, &prno, &nst) == 3)
	{	if (depth == -2)
		{	if (verbose)
			{	printf("starting claim %d\n", prno);
			}
			start_claim(prno);
			continue;
		}
		if (depth == -4)
		{	if (verbose&32)
			{	printf("using statement merging\n");
			}
			merger = 1;
			ana_src(0, 1);
			continue;
		}
		if (depth == -1)
		{	if (1 || verbose)
			{	if (columns == 2)
				dotag(stdout, " CYCLE>\n");
				else
				dotag(stdout, "<<<<<START OF CYCLE>>>>>\n");
			}
			continue;
		}
		if (depth <= -5
		&&  depth >= -8)
		{	printf("spin: used search permutation, replay with ./pan -r\n");
			return;	/* permuted: -5, -6, -7, -8 */
		}

		if (cutoff > 0 && depth >= cutoff)
		{	printf("-------------\n");
			printf("depth-limit (-u%d steps) reached\n", cutoff);
			break;
		}

		if (Skip_claim && prno == 0) continue;

		for (dothis = Al_El; dothis; dothis = dothis->Nxt)
		{	if (dothis->Seqno == nst)
				break;
		}
		if (!dothis)
		{	printf("%3d: proc %d, no matching stmnt %d\n",
				depth, prno - Have_claim, nst);
			lost_trail();
		}

		i = nproc - nstop + Skip_claim;

		if (dothis->n->ntyp == '@')
		{	if (prno == i-1)
			{	run_lst = run_lst->nxt;
				nstop++;
				if (verbose&4)
				{	if (columns == 2)
					{	dotag(stdout, "<end>\n");
						continue;
					}
					if (Have_claim && prno == 0)
					printf("%3d: claim terminates\n",
						depth);
					else
					printf("%3d: proc %d terminates\n",
						depth, prno - Have_claim);
				}
				continue;
			}
			if (prno <= 1) continue;	/* init dies before never */
			printf("%3d: stop error, ", depth);
			printf("proc %d (i=%d) trans %d, %c\n",
				prno - Have_claim, i, nst, dothis->n->ntyp);
			lost_trail();
		}

		if (0 && !xspin && (verbose&32))
		{	printf("step %d i=%d pno %d stmnt %d\n", depth, i, prno, nst);
		}

		for (X_lst = run_lst; X_lst; X_lst = X_lst->nxt)
		{	if (--i == prno)
				break;
		}

		if (!X_lst)
		{	if (verbose&32)
			{	printf("%3d: no process %d (stmnt %d)\n", depth, prno - Have_claim, nst);
				printf(" max %d (%d - %d + %d) claim %d ",
					nproc - nstop + Skip_claim,
					nproc, nstop, Skip_claim, Have_claim);
				printf("active processes:\n");
				for (X_lst = run_lst; X_lst; X_lst = X_lst->nxt)
				{	printf("\tpid %d\tproctype %s\n", X_lst->pid, X_lst->n->name);
				}
				printf("\n");
				continue;	
			} else
			{	printf("%3d:\tproc  %d (?) ", depth, prno);
				lost_trail();
			}
		} else
		{	int min_seq = find_min(X_lst->ps);
			int max_seq = find_max(X_lst->ps);

			if (nst < min_seq || nst > max_seq)
			{	printf("%3d: error: invalid statement", depth);
				if (verbose&32)
				{	printf(": pid %d:%d (%s:%d:%d) stmnt %d (valid range %d .. %d)",
					prno, X_lst->pid, X_lst->n->name, X_lst->tn, X_lst->b,
					nst, min_seq, max_seq);
				}
				printf("\n");
				continue;
				/* lost_trail(); */
			}
			X_lst->pc  = dothis;
		}

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
					if (verbose&2) dumplocal(X_lst, 0);
					if (xspin) printf("\n");
				}
				og = g;
			} while (g && g != dothis->nxt);
			if (X_lst != NULL)
			{	X_lst->pc = g?huntele(g, 0, -1):g;
			}
		} else
		{
keepgoing:		if (dothis->merge_start)
				a = dothis->merge_start;
			else
				a = dothis->merge;

			if (X_lst != NULL)
			{	X_lst->pc = eval_sub(dothis);
				if (X_lst->pc) X_lst->pc = huntele(X_lst->pc, 0, a);
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
						(X_lst && X_lst->pc)?X_lst->pc->seqno:-1);
					printf("\n");
				}
				if (verbose&1) dumpglobals();
				if (verbose&2) dumplocal(X_lst, 0);
				if (xspin) printf("\n");

				if (X_lst && !X_lst->pc)
				{	X_lst->pc = dothis;
					printf("\ttransition failed\n");
					a = 0;	/* avoid inf loop */
				}
			}
			if (a && X_lst && X_lst->pc && X_lst->pc->seqno != a)
			{	dothis = X_lst->pc;
				goto keepgoing;
		}	}

		if (Have_claim && X_lst && X_lst->pid == 0
		&&  dothis->n
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

	for (Y = run_lst; Y; Y = Y->nxt)
	{	if (--i == pid)
			return Y->pc->seqno;
	}
	return 0;
}
