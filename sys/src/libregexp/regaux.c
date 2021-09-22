#include <u.h>
#include <libc.h>
#include "regexp.h"
#include "regcomp.h"

enum {
	Minalloc = 10,
};

static void
resizesublist(Resublist *sl, int alloc)
{
	int oalloc;
	static int subid;

	if (alloc < Minalloc)
		alloc = Minalloc;		/* sanity; must be > 0 */
	oalloc = sl->alloced;
	if (sl->m && oalloc >= alloc)
		return;
	if (oalloc == 0 && sl->id == 0)
		sl->id = ++subid;

	sl->m = realloc(sl->m, alloc * sizeof(Resub));
	if (sl->m == nil)
		regerror("out of memory");
	if (alloc > oalloc)
		memset(sl->m + oalloc, 0, (alloc - oalloc)*sizeof(Resub));
	sl->alloced = alloc;
}

static void
zerosublist(Relist *p, int ms)
{
	Resublist *sl;

	sl = &p->se;
	resizesublist(sl, ms);
	assert(sl->alloced >= 0);
	memset(sl->m, 0, sl->alloced * sizeof(Resub));
}

/* copy ms Resubs from sep->m (if sep has that many allocated) to p->se.m */
static void
copysublist(Relist *p, int ms, Resublist *sep)
{
	int copy;
	Resublist *sl;

	zerosublist(p, ms);
	copy = ms;
	if (sep->alloced < copy)
		copy = sep->alloced;	/* only copy as many as sep has */
	sl = &p->se;
	if (copy > 1)
		memmove(sl->m, sep->m, copy * sizeof(Resub));
	else
		sl->m[0] = sep->m[0];		/* optimisation */
}

int
_regalloclists(Reljunk *j, int elems)
{
	Relist *relist0, *relist1;

	/* mark space */
	relist0 = mallocz(elems*sizeof(Relist), 1);
	relist1 = mallocz(elems*sizeof(Relist), 1);
	if(relist0 == nil || relist1 == nil){
		free(relist1);
		free(relist0);
		return -1;
	}
	j->relist[0] = relist0;
	j->relist[1] = relist1;
	j->reliste[0] = relist0 + elems - 2;
	j->reliste[1] = relist1 + elems - 2;
	return 0;
}

/*
 * free dynamically-alloced storage attached to an Relist,
 * but not the Relist itself.
 */
static void
_regfreelist(Relist *p)
{
	if (p)
		free(p->se.m);		/* frees the entire array */
	free(p);
}

void
_regfreelists(Reljunk *j, int elems)
{
	USED(elems);
	if (j == nil)
		return;
	_regfreelist(j->relist[0]);
	_regfreelist(j->relist[1]);
	j->relist[0] = j->relist[1] = nil;
	j->reliste[0] = j->reliste[1] = nil;
}

/*
 *  save a new match in mp, which is ms elements long
 */
extern void
_renewmatch(Resub *mp, int ms, Resublist *sp)
{
	int i, copy;

	if (mp == 0 || ms <= 0)
		return;
	copy = sp->alloced;		/* in case < ms */
	if (copy > ms)
		copy = ms;
	if (mp[0].sp == 0 || sp->m[0].sp < mp[0].sp ||
	    (sp->m[0].sp == mp[0].sp && sp->m[0].ep > mp[0].ep)) {
		for (i = 0; i < copy; i++)
			mp[i] = sp->m[i];
		for (; i < ms; i++)
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
	int ms,
	Resublist *sep)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++)
		if(p->inst == ip){
			if(sep->m[0].sp < p->se.m[0].sp)
				copysublist(p, ms, sep);
			return 0;
		}
	p->inst = ip;
	copysublist(p, ms, sep);
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
	int ms,
	char *sp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++)
		if(p->inst == ip){
			if(sp < p->se.m[0].sp) {
				zerosublist(p, ms);
				p->se.m[0].sp = sp;
			}
			return 0;
		}
	p->inst = ip;
	zerosublist(p, ms);
	p->se.m[0].sp = sp;
	(++p)->inst = 0;
	return p;
}

extern Relist*
_rrenewemptythread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	int ms,
	Rune *rsp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++)
		if(p->inst == ip){
			if(rsp < p->se.m[0].rsp) {
				zerosublist(p, ms);
				p->se.m[0].rsp = rsp;
			}
			return 0;
		}
	p->inst = ip;
	zerosublist(p, ms);
	p->se.m[0].rsp = rsp;
	(++p)->inst = 0;
	return p;
}
