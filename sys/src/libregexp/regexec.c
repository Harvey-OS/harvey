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
regexec1(Reprog *progp,	/* program to run */
	char *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms,		/* number of elements at mp */
	Reljunk *j)
{
	int flag=0;
	Reinst *inst;
	Relist *tlp;
	char *s;
	int i, checkstart;
	Rune r, *rp, *ep;
	int n;
	Relist* tl;		/* This list, next list */
	Relist* nl;
	Relist* tle;		/* ends of this and next list */
	Relist* nle;
	int match;
	char *p;

	match = 0;
	checkstart = j->starttype;
	if(mp)
		for(i=0; i<ms; i++) {
			mp[i].sp = 0;
			mp[i].ep = 0;
		}
	j->relist[0][0].inst = 0;
	j->relist[1][0].inst = 0;

	/* Execute machine once for each character, including terminal NUL */
	s = j->starts;
	do{
		/* fast check for first char */
		if(checkstart) {
			switch(j->starttype) {
			case RUNE:
				p = utfrune(s, j->startchar);
				if(p == 0 || s == j->eol)
					return match;
				s = p;
				break;
			case BOL:
				if(s == bol)
					break;
				p = utfrune(s, '\n');
				if(p == 0 || s == j->eol)
					return match;
				s = p+1;
				break;
			}
		}
		r = *(uchar*)s;
		if(r < Runeself)
			n = 1;
		else
			n = chartorune(&r, s);

		/* switch run lists */
		tl = j->relist[flag];
		tle = j->reliste[flag];
		nl = j->relist[flag^=1];
		nle = j->reliste[flag];
		nl->inst = 0;

		/* Add first instruction to current list */
		if(match == 0)
			_renewemptythread(tl, progp->startinst, ms, s);

		/* Execute machine until current list is empty */
		for(tlp=tl; tlp->inst; tlp++){
			for(inst = tlp->inst; ; inst = inst->next){
				switch(inst->type){
				case RUNE:	/* regular character */
					if(inst->r == r){
						if(_renewthread(nl, inst->next, ms, &tlp->se)==nle)
							return -1;
					}
					break;
				case LBRA:
					tlp->se.m[inst->subid].sp = s;
					continue;
				case RBRA:
					tlp->se.m[inst->subid].ep = s;
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
					if(s == j->eol || r == 0 || r == '\n')
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
					tlp->se.m[0].ep = s;
					if(mp != 0)
						_renewmatch(mp, ms, &tlp->se);
					break;
				}
				break;
			}
		}
		if(s == j->eol)
			break;
		checkstart = j->starttype && nl->inst==0;
		s += n;
	}while(r);
	return match;
}

extern int
regexec(Reprog *progp,	/* program to run */
	char *bol,	/* string to run machine on */
	Resub *mp,	/* subexpression elements */
	int ms)		/* number of elements at mp */
{
	Reljunk j, jbig;
	int rv;

	/*
 	 *  use user-specified starting/ending location if specified
	 */
	memset(&j, 0, sizeof j);
	j.starts = bol;
	j.eol = 0;
	if(mp && ms>0){
		if(mp->sp)
			j.starts = mp->sp;
		if(mp->ep)
			j.eol = mp->ep;
	}
	j.starttype = 0;
	j.startchar = 0;

	if (progp == nil)
		return -1;
	assert(progp->startinst != nil);
	if(progp->startinst->type == RUNE && progp->startinst->r < Runeself) {
		j.starttype = RUNE;
		j.startchar = progp->startinst->r;
	}
	if(progp->startinst->type == BOL)
		j.starttype = BOL;
	jbig = j;

	if (_regalloclists(&j, LISTSIZE) < 0)
		return -1;
	rv = regexec1(progp, bol, mp, ms, &j);
	_regfreelists(&j, LISTSIZE);
	if (rv >= 0)
		return rv;

	if (_regalloclists(&jbig, BIGLISTSIZE) < 0)
		return -1;
	rv = regexec1(progp, bol, mp, ms, &jbig);
	_regfreelists(&jbig, BIGLISTSIZE);
	return rv;
}
