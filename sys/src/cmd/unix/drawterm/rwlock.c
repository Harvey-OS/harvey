#include "lib9.h"
#include "sys.h"

void
rlock(RWlock *l)
{
	qlock(&l->x);		/* wait here for writers and exclusion */
	lock(&l->l);
	l->readers++;
	canqlock(&l->k);	/* block writers if we are the first reader */
	unlock(&l->l);
	qunlock(&l->x);
}

void
runlock(RWlock *l)
{
	lock(&l->l);
	if(--l->readers == 0)	/* last reader out allows writers */
		qunlock(&l->k);
	unlock(&l->l);
}

void
wlock(RWlock *l)
{
	qlock(&l->x);		/* wait here for writers and exclusion */
	qlock(&l->k);		/* wait here for last reader */
}

void
wunlock(RWlock *l)
{
	qunlock(&l->k);
	qunlock(&l->x);
}
