#include <stdlib.h>
#include <stdio.h>
#include "regexp.h"
#include "regcomp.h"

static Resublist sempty;		/* empty set of matches */

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
	char *starts,
	char *eol,
	wchar_t startchar)
{
	int flag=0;
	Reinst *inst;
	Relist *tlp;
	char *s;
	int i, checkstart;
	wchar_t r, *rp, *ep;
	int n;
	Relist* tl;		/* This list, next list */
	Relist* nl;
	Relist* tle;		/* ends of this and next list */
	Relist* nle;
	int match;

	match = 0;
	checkstart = startchar;
	sempty.m[0].s.sp = 0;
	if(mp!=0)
		for(i=0; i<ms; i++)
			mp[i].s.sp = mp[i].e.ep = 0;
	_relist[0][0].inst = _relist[1][0].inst = 0;

	/* Execute machine once for each character, including terminal NUL */
	s = starts;
	do{
		/* fast check for first char */
		r = *(unsigned char*)s;
		if(checkstart && r != startchar){
			s++;
			continue;
		}

		if(r < Runeself)
			n = 1;
		else {
			n = mbtowc(&r, s, MB_CUR_MAX);
			if (n <= 0)
				n = 1;
		}

		/* switch run lists */
		tl = _relist[flag];
		tle = _reliste[flag];
		nl = _relist[flag^=1];
		nle = _reliste[flag];
		nl->inst = 0;

		/* Add first instruction to current list */
		if(match == 0){
			sempty.m[0].s.sp = s;
			_renewthread(tl, progp->startinst, &sempty);
		}

		/* Execute machine until current list is empty */
		for(tlp=tl; tlp->inst; tlp++){	/* assignment = */
			if(s == eol)
				break;

			for(inst = tlp->inst; ; inst = inst->l.next){
				switch(inst->type){
				case RUNE:	/* regular character */
					if(inst->r.r == r)
						if(_renewthread(nl, inst->l.next, &tlp->se)==nle)
							return -1;
					break;
				case LBRA:
					tlp->se.m[inst->r.subid].s.sp = s;
					continue;
				case RBRA:
					tlp->se.m[inst->r.subid].e.ep = s;
					continue;
				case ANY:
					if(r != '\n')
						if(_renewthread(nl, inst->l.next, &tlp->se)==nle)
							return -1;
					break;
				case ANYNL:
					if(_renewthread(nl, inst->l.next, &tlp->se)==nle)
						return -1;
					break;
				case BOL:
					if(s == bol || *(s-1) == '\n')
						continue;
					break;
				case EOL:
					if(r == 0 || r == '\n')
						continue;
					break;
				case CCLASS:
					ep = inst->r.cp->end;
					for(rp = inst->r.cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1]){
							if(_renewthread(nl, inst->l.next, &tlp->se)==nle)
								return -1;
							break;
						}
					break;
				case NCCLASS:
					ep = inst->r.cp->end;
					for(rp = inst->r.cp->spans; rp < ep; rp += 2)
						if(r >= rp[0] && r <= rp[1])
							break;
					if(rp == ep)
						if(_renewthread(nl, inst->l.next, &tlp->se)==nle)
							return -1;
					break;
				case OR:
					/* evaluate right choice later */
					if(_renewthread(tlp, inst->r.right, &tlp->se) == tle)
						return -1;
					/* efficiency: advance and re-evaluate */
					continue;
				case END:	/* Match! */
					match = 1;
					tlp->se.m[0].e.ep = s;
					if(mp != 0)
						_renewmatch(mp, ms, &tlp->se);
					break;
				}
				break;
			}
		}
		checkstart = startchar && nl->inst==0;
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
	char *starts;	/* where to start match */
	char *eol;	/* where to end match */
	wchar_t startchar;
	int rv;

	/*
 	 *  use user-specified starting/ending location if specified
	 */
	starts = bol;
	eol = 0;
	if(mp && ms>0){
		if(mp->s.sp)
			starts = mp->s.sp;
		if(mp->e.ep)
			eol = mp->e.ep;
	}
	startchar = (progp->startinst->type == RUNE && progp->startinst->r.r < Runeself)
		? progp->startinst->r.r : 0;

	/* keep trying till we have enough list space to terminate */
	for(;;){
		if(_relist[0] == 0){
			_relist[0] = malloc(2*_relistsize*sizeof(Relist));
			_relist[1] = _relist[0] + _relistsize;
			_reliste[0] = _relist[0] + _relistsize - 1;
			_reliste[1] = _relist[1] + _relistsize - 1;
			if(_relist[0] == 0)
				regerror("_relist overflow");
		}
		rv = regexec1(progp, bol, mp, ms, starts, eol, startchar);
		if(rv >= 0)
			return rv;
		free(_relist[0]);
		_relist[0] = 0;
		_relistsize += LISTINCREMENT;
	}
}
