#include <stdlib.h>
#include <stdio.h>
#include "regexp.h"
#include "regcomp.h"

/*
 *	Machine state
 */
Relist*	_relist[2];
Relist*	_reliste[2];
int	_relistsize = LISTINCREMENT;

/*
 *  save a new match in mp
 */
extern void
_renewmatch(Resub *mp, int ms, Resublist *sp)
{
	int i;

	if(mp==0 || ms<=0)
		return;
	if(mp[0].s.sp==0 || sp->m[0].s.sp<mp[0].s.sp ||
	   (sp->m[0].s.sp==mp[0].s.sp && sp->m[0].e.ep>mp[0].e.ep)){
		for(i=0; i<ms && i<NSUBEXP; i++)
			mp[i] = sp->m[i];
		for(; i<ms; i++)
			mp[i].s.sp = mp[i].e.ep = 0;
	}
}

/*
 * Note optimization in _renewthread:
 * 	*lp must be pending when _renewthread called; if *l has been looked
 *		at already, the optimization is a bug.
 */
extern Relist*
_renewthread(Relist *lp,	/* _relist to add to */
	Reinst *ip,	/* instruction to add */
	Resublist *sep)	/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if((sep)->m[0].s.sp < p->se.m[0].s.sp)
				p->se = *sep;
			return 0;
		}
	}
	p->inst = ip;
	p->se = *sep;
	(++p)->inst = 0;
	return p;
}

