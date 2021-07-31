#include	<alef.h>
#include	"run.h"

/*
 * Called by alloc to allocate and initialise channels
 */

void*
ALEF_chana(int nbuf, int bsize)
{
	Chan *c;
	Msgbuf *b;

	if(nbuf == 0)
		return malloc(sizeof(Chan));

	bsize += sizeof(Msgbuf);
	bsize = (bsize + sizeof(int) - 1) & ~(sizeof(int) - 1);
	c = malloc(sizeof(Chan)+nbuf*bsize);
	if(c == nil)
		return nil;

	b = (Msgbuf*)((byte*)c+sizeof(Chan));
	c->free = b;
	c->async = 1;
	while(nbuf > 1) {
		b->next = (Msgbuf*)((byte*)b+bsize);
		b = b->next;
		nbuf--;
	}
	return c;
}

void
ALEF_chanu(Chan *c)
{
	/* Unalloc when everybody is done */
	c->snd.lock();
	c->rcv.lock();
	free(c);
}

/*
 * Return true if a process can send on this channel without blocking
 */

int
ALEF_csnd(Chan *c)
{
	if(c->rva || c->free)
		return 1;

	if(c->selt != nil && c->seltst == 0)
		return 1;

	return 0;
}

/*
 * Return true if a process can receive on this channel without blocking
 */

int
ALEF_crcv(Chan *c)
{
	if(c->sva || c->qh)
		return 1;

	if(c->selt != nil && c->seltst == 1)
		return 1;

	return 0;
}

/*
 * Append a channel structure to a process private select structure ready
 * to do a selrecv.
 */

intern
void
ALEF_qit(Chan *c)
{
	Task *t;

	t = PROC.tdb->ctask;

	c->lock();
	if(t->slist)
		t->stail->sellink = c;
	else
		t->slist = c;
	t->stail = c;
	c->sellink = nil;
	c->selt = t;
	c->unlock();
}

void
ALEF_selrecv(Chan *c)
{
	if(c->selt != nil)
		return;

	ALEF_qit(c);
	c->seltst = 0;
}

void
ALEF_selsend(Chan *c)
{
	if(c->selt != nil)
		return;

	ALEF_qit(c);
	c->seltst = 1;
}
