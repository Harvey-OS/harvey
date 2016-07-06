/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"mk.h"

int runerrs;

void
mk(char *target)
{
	Node *node;
	int did = 0;

	nproc();		/* it can be updated dynamically */
	nrep();			/* it can be updated dynamically */
	runerrs = 0;
	node = graph(target);
	if(DEBUG(D_GRAPH)){
		dumpn("new target\n", node);
		Bflush(&bout);
	}
	clrmade(node);
	while(node->flags&NOTMADE){
		if(work(node, (Node *)0, (Arc *)0))
			did = 1;	/* found something to do */
		else {
			if(waitup(1, (int *)0) > 0){
				if(node->flags&(NOTMADE|BEINGMADE)){
					assert(/*must be run errors*/ runerrs);
					break;	/* nothing more waiting */
				}
			}
		}
	}
	if(node->flags&BEINGMADE)
		waitup(-1, (int *)0);
	while(jobs)
		waitup(-2, (int *)0);
	assert(/*target didnt get done*/ runerrs || (node->flags&MADE));
	if(did == 0)
		Bprint(&bout, "mk: '%s' is up to date\n", node->name);
}

void
clrmade(Node *n)
{
	Arc *a;

	n->flags &= ~(CANPRETEND|PRETENDING);
	if(strchr(n->name, '(') ==0 || n->time)
		n->flags |= CANPRETEND;
	MADESET(n, NOTMADE);
	for(a = n->prereqs; a; a = a->next)
		if(a->n)
			clrmade(a->n);
}

static void
unpretend(Node *n)
{
	MADESET(n, NOTMADE);
	n->flags &= ~(CANPRETEND|PRETENDING);
	n->time = 0;
}

int
work(Node *node, Node *p, Arc *parc)
{
	Arc *a, *ra;
	int weoutofdate;
	int ready;
	int did = 0;
	char cwd[256];

	/*print("work(%s) flags=0x%x time=%lu\n", node->name, node->flags, node->time);/**/
	if(node->flags&BEINGMADE)
		return(did);
	if((node->flags&MADE) && (node->flags&PRETENDING) && p && outofdate(p, parc, 0)){
		if(explain)
			fprint(1, "unpretending %s(%lu) because %s is out of date(%lu)\n",
				node->name, node->time, p->name, p->time);
		unpretend(node);
	}
	/*
		have a look if we are pretending in case
		someone has been unpretended out from underneath us
	*/
	if(node->flags&MADE){
		if(node->flags&PRETENDING){
			node->time = 0;
		}else
			return(did);
	}
	/* consider no prerequisite case */
	if(node->prereqs == 0){
		if(node->time == 0){
			if(getwd(cwd, sizeof cwd))
				fprint(2, "mk: don't know how to make '%s' in directory %s\n", node->name, cwd);
			else
				fprint(2, "mk: don't know how to make '%s'\n", node->name);
			if(kflag){
				node->flags |= BEINGMADE;
				runerrs++;
			} else
				Exit();
		} else
			MADESET(node, MADE);
		return(did);
	}
	/*
		now see if we are out of date or what
	*/
	ready = 1;
	weoutofdate = aflag;
	ra = 0;
	for(a = node->prereqs; a; a = a->next)
		if(a->n){
			did = work(a->n, node, a) || did;
			if(a->n->flags&(NOTMADE|BEINGMADE))
				ready = 0;
			if(outofdate(node, a, 0)){
				weoutofdate = 1;
				if((ra == 0) || (ra->n == 0)
						|| (ra->n->time < a->n->time))
					ra = a;
			}
		} else {
			if(node->time == 0){
				if(ra == 0)
					ra = a;
				weoutofdate = 1;
			}
		}
	if(ready == 0)	/* can't do anything now */
		return(did);
	if(weoutofdate == 0){
		MADESET(node, MADE);
		return(did);
	}
	/*
		can we pretend to be made?
	*/
	if((iflag == 0) && (node->time == 0) && (node->flags&(PRETENDING|CANPRETEND))
			&& p && ra->n && !outofdate(p, ra, 0)){
		node->flags &= ~CANPRETEND;
		MADESET(node, MADE);
		if(explain && ((node->flags&PRETENDING) == 0))
			fprint(1, "pretending %s has time %lu\n", node->name, node->time);
		node->flags |= PRETENDING;
		return(did);
	}
	/*
		node is out of date and we REALLY do have to do something.
		quickly rescan for pretenders
	*/
	for(a = node->prereqs; a; a = a->next)
		if(a->n && (a->n->flags&PRETENDING)){
			if(explain)
				Bprint(&bout, "unpretending %s because of %s because of %s\n",
				a->n->name, node->name, ra->n? ra->n->name : "rule with no prerequisites");

			unpretend(a->n);
			did = work(a->n, node, a) || did;
			ready = 0;
		}
	if(ready == 0)	/* try later unless nothing has happened for -k's sake */
		return(did || work(node, p, parc));
	did = dorecipe(node) || did;
	return(did);
}

void
update(int fake, Node *node)
{
	Arc *a;

	MADESET(node, fake? BEINGMADE : MADE);
	if(((node->flags&VIRTUAL) == 0) && (access(node->name, 0) == 0)){
		node->time = timeof(node->name, 1);
		node->flags &= ~(CANPRETEND|PRETENDING);
		for(a = node->prereqs; a; a = a->next)
			if(a->prog)
				outofdate(node, a, 1);
	} else {
		node->time = 1;
		for(a = node->prereqs; a; a = a->next)
			if(a->n && outofdate(node, a, 1))
				node->time = a->n->time;
	}
/*	print("----node %s time=%lu flags=0x%x\n", node->name, node->time, node->flags);/**/
}

static
pcmp(char *prog, char *p, char *q)
{
	char buf[3*NAMEBLOCK];
	int pid;

	Bflush(&bout);
	snprint(buf, sizeof buf, "%s '%s' '%s'\n", prog, p, q);
	pid = pipecmd(buf, 0, 0);
	while(waitup(-3, &pid) >= 0)
		;
	return(pid? 2:1);
}

int
outofdate(Node *node, Arc *arc, int eval)
{
	char buf[3*NAMEBLOCK], *str;
	Symtab *sym;
	int ret;

	str = 0;
	if(arc->prog){
		snprint(buf, sizeof buf, "%s%c%s", node->name, 0377,
			arc->n->name);
		sym = symlook(buf, S_OUTOFDATE, 0);
		if(sym == 0 || eval){
			if(sym == 0)
				str = strdup(buf);
			ret = pcmp(arc->prog, node->name, arc->n->name);
			if(sym)
				sym->u.value = ret;
			else
				symlook(str, S_OUTOFDATE, (void *)ret);
		} else
			ret = sym->u.value;
		return(ret-1);
	} else if(strchr(arc->n->name, '(') && arc->n->time == 0)  /* missing archive member */
		return 1;
	else
		/*
		 * Treat equal times as out-of-date.
		 * It's a race, and the safer option is to do
		 * extra building rather than not enough.
	 	 */
		return node->time <= arc->n->time;
}
