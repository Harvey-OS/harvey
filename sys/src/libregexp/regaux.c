#include <u.h>
#include <libc.h>
#include "regexp.h"
#include "regcomp.h"


/*
 *  save a new match in mp
 */
extern void
_renewmatch(Resub *mp, int ms, Resublist *sp)
{
	int i;

	if(mp==0 || ms<=0)
		return;
	if(mp[0].sp==0 || sp->m[0].sp<mp[0].sp ||
	   (sp->m[0].sp==mp[0].sp && sp->m[0].ep>mp[0].ep)){
		for(i=0; i<ms && i<NSUBEXP; i++)
			mp[i] = sp->m[i];
		for(; i<ms; i++)
			mp[i].sp = mp[i].ep = 0;
	}
}

/*
 * Note optimization in _renewthread:
 * 	*lp must be pending when _renewthread called; if *l has been looked
 *		at already, the optimization is a bug.
 */
extern Relist*
_renewthread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	Resublist *sep)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if((sep)->m[0].sp < p->se.m[0].sp)
				p->se = *sep;
			return 0;
		}
	}
	p->inst = ip;
	p->se = *sep;
	(++p)->inst = 0;
	return p;
}

/*
 * same as renewthread, but called with
 * initial empty start pointer.
 */
extern Relist*
_renewemptythread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	char *sp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(sp < p->se.m[0].sp) {
				memset(&p->se, 0, sizeof(p->se));
				p->se.m[0].sp = sp;
			}
			return 0;
		}
	}
	p->inst = ip;
	memset(&p->se, 0, sizeof(p->se));
	p->se.m[0].sp = sp;
	(++p)->inst = 0;
	return p;
}

extern Relist*
_rrenewemptythread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	Rune *rsp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(rsp < p->se.m[0].rsp) {
				memset(&p->se, 0, sizeof(p->se));
				p->se.m[0].rsp = rsp;
			}
			return 0;
		}
	}
	p->inst = ip;
	memset(&p->se, 0, sizeof(p->se));
	p->se.m[0].rsp = rsp;
	(++p)->inst = 0;
	return p;
}
