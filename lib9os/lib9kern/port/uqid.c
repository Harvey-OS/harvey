#include	"dat.h"
#include	"fns.h"
#include	"error.h"

/*
 * unique Qid path generation,
 * for exportfs.c and various devfs-*
 */

#define	QIDMASK	(((vlong)1<<48)-1)

void
uqidinit(Uqidtab *tab)
{
	memset(tab, 0, sizeof(*tab));
}

static int
uqidhash(vlong path)
{
	ulong p;
	p = (ulong)path;
	return ((p>>16) ^ (p>>8) ^ p) & (Nqidhash-1);
}

static Uqid **
uqidlook(Uqid **tab, Chan *c, vlong path)
{
	Uqid **hp, *q;

	for(hp = &tab[uqidhash(path)]; (q = *hp) != nil; hp = &q->next)
		if(q->type == c->type && q->dev == c->dev && q->oldpath == path)
			break;
	return hp;
}

static int
uqidexists(Uqid **tab, vlong path)
{
	int i;
	Uqid *q;

	for(i=0; i<Nqidhash; i++)
		for(q = tab[i]; q != nil; q = q->next)
			if(q->newpath == path)
				return 1;
	return 0;
}

Uqid *
uqidalloc(Uqidtab *tab, Chan *c)
{
	Uqid **hp, *q;

	qlock(&tab->l);
	hp = uqidlook(tab->qids, c, c->qid.path);
	if((q = *hp) != nil){
		incref(&q->r);
		qunlock(&tab->l);
		return q;
	}
	q = mallocz(sizeof(*q), 1);
	if(q == nil){
		qunlock(&tab->l);
		p9error(Enomem);
	}
	q->r.ref = 1;
	q->type = c->type;
	q->dev = c->dev;
	q->oldpath = c->qid.path;
	q->newpath = c->qid.path;
	while(uqidexists(tab->qids, q->newpath)){
		if(++tab->pathgen >= (1<<16))
			tab->pathgen = 1;
		q->newpath = ((vlong)tab->pathgen<<48) | (q->newpath & QIDMASK);
	}
	q->next = nil;
	*hp = q;
	qunlock(&tab->l);
	return q;
}

void
freeuqid(Uqidtab *tab, Uqid *q)
{
	Uqid **hp;

	if(q == nil)
		return;
	qlock(&tab->l);
	if(decref(&q->r) == 0){
		hp = &tab->qids[uqidhash(q->oldpath)];
		for(; *hp != nil; hp = &(*hp)->next)
			if(*hp == q){
				*hp = q->next;
				free(q);
				break;
			}
	}
	qunlock(&tab->l);
}

Qid
mkuqid(Chan *c, Uqid *qid)
{
	Qid q;

	if(qid == nil)
		return c->qid;
	q.path = qid->newpath;
	q.vers = c->qid.vers;
	q.type = c->qid.type;
	return q;
}
