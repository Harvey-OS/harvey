#include <u.h>
#include <libc.h>
#include "regexp.h"
#include "regcomp.h"

/*
 *  return	0 if no match
 *		>0 if a match
 *		<0 if we ran out of _relist space
 */
static int
rregexec1(Reprog *progp,	/* program to run */
	Rune *bol,		/* string to run machine on */
	Resub *mp,		/* subexpression elements */
	int ms,			/* number of elements at mp */
	Reljunk *j)
{
	int flag=0;
	Reinst *inst;
	Relist *tlp;
	Rune *s;
	int i, checkstart;
	Rune r, *rp, *ep;
	Relist* tl;		/* This list, next list */
	Relist* nl;
	Relist* tle;		/* ends of this and next list */
	Relist* nle;
	int match;

	match = 0;
	checkstart = j->startchar;
	if(mp)
		for(i=0; i<ms; i++) {
			mp[i].rsp = 0;
			mp[i].rep = 0;
		}
	j->relist[0][0].inst = 0;
	j->relist[1][0].inst = 0;

	/* Execute machine once for each character, including terminal NUL */
	s = j->rstarts;
	do{

		/* fast check for first char */
		if(checkstart) {
			switch(j->starttype) {
			case RUNE:
				while(*s != j->startchar) {
					if(*s == 0 || s == j->reol)
						return match;
					s++;
				}
				break;
			case BOL:
				if(s == bol)
					break;
				while(*s != '\n') {
					if(*s == 0 || s == j->reol)
						return match;
					s++;
				}
				break;
			}
		}

		r = *s;

		/* switch run lists */
		tl = j->relist[flag];
		tle = j->reliste[flag];
		nl = j->relist[flag^=1];
		nle = j->reliste[flag];
		nl->inst = 0;

		/* Add first instruction to current list */
		_rrenewemptythread(tl, progp->startinst, ms, s);

		/* Execute machine until current list is empty */
		for(tlp=tl; tlp->inst; tlp++){
			for(inst=tlp->inst; ; inst = inst->next){
				switch(inst->type){
				case RUNE:	/* regular character */
					if(inst->r == r)
						if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
							return -1;
					break;
				case LBRA:
					tlp->se.m[inst->subid].rsp = s;
					continue;
				case RBRA:
					tlp->se.m[inst->subid].rep = s;
					continue;
				case ANY:
					if(r != '\n')
						if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
							return -1;
					break;
				case ANYNL:
					if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
							return -1;
					break;
				case BOL:
					if(s == bol || *(s-1) == '\n')
						continue;
					break;
				case EOL:
					if(s == j->reol || r == 0 || r == '\n')
						continue;
					break;
				case CCLASS:
					ep = inst->cp->end;
					for(rp = inst->cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1]){
							if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
								return -1;
							break;
						}
					break;
				case NCCLASS:
					ep = inst->cp->end;
					for(rp = inst->cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1])
							break;
					if(rp == ep)
						if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
							return -1;
					break;
				case OR:
					/* evaluate right choice later */
					if(_renewthread(tlp, inst->right, ms, &tlp->se) == tle)
						return -1;
					/* efficiency: advance and re-evaluate */
					continue;
				case END:	/* Match! */
					match = 1;
					tlp->se.m[0].rep = s;
					if(mp != 0)
						_renewmatch(mp, ms, &tlp->se);
					break;
				}
				break;
			}
		}
		if(s == j->reol)
			break;
		checkstart = j->startchar && nl->inst==0;
		s++;
	}while(r);
	return match;
}

static int
rregexec2(Reprog *progp,	/* program to run */
	Rune *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms,		/* number of elements at mp */
	Reljunk *j
)
{
	Relist relist0[5*LISTSIZE], relist1[5*LISTSIZE];

	/* mark space */
	j->relist[0] = relist0;
	j->relist[1] = relist1;
	j->reliste[0] = relist0 + nelem(relist0) - 2;
	j->reliste[1] = relist1 + nelem(relist1) - 2;

	return rregexec1(progp, bol, mp, ms, j);
}

extern int
rregexec(Reprog *progp,	/* program to run */
	Rune *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms)		/* number of elements at mp */
{
	Reljunk j;
	Relist relist0[LISTSIZE], relist1[LISTSIZE];
	int rv;

	/*
 	 *  use user-specified starting/ending location if specified
	 */
	j.rstarts = bol;
	j.reol = 0;
	if(mp && ms>0){
		if(mp->sp)
			j.rstarts = mp->rsp;
		if(mp->ep)
			j.reol = mp->rep;
	}
	j.starttype = 0;
	j.startchar = 0;
	if(progp->startinst->type == RUNE && progp->startinst->r < Runeself) {
		j.starttype = RUNE;
		j.startchar = progp->startinst->r;
	}
	if(progp->startinst->type == BOL)
		j.starttype = BOL;

	/* mark space */
	j.relist[0] = relist0;
	j.relist[1] = relist1;
	j.reliste[0] = relist0 + nelem(relist0) - 2;
	j.reliste[1] = relist1 + nelem(relist1) - 2;

	rv = rregexec1(progp, bol, mp, ms, &j);
	if(rv >= 0)
		return rv;
	rv = rregexec2(progp, bol, mp, ms, &j);
	if(rv >= 0)
		return rv;
	return -1;
}
