#include	<alef.h>
#include	"run.h"

void
ALEF_sndint(Chan *c, int val, uint signature)
{
	Chan *n;
	Task *t;
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		*(int*)c->rva = val;
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}

	if(c->async == 0) {
		c->sva = &val;
		c->signature = signature;
		goto out;
	}

	for(;;) {
		b = c->free;
		if(b)
			break;
		c->unlock();
		c->br.Sleep(&c->free, 1);
		c->lock();
	}
	c->free = b->next;
	b->i = val;
	b->signature = signature;
	if(c->qh == nil)
		c->qh = b;
	else
		c->qt->next = b;
	c->qt = b;
	b->next = nil;

out:
	c->unlock();
	t = c->selt;
	if(t != nil && c->seltst == 0) {
		n = t->slist;
		if(n != nil)
			n->selp = 1;
		t->Wakeup();
	}
	if(c->async == 0)
		c->sndr.Sleep(&c->sva, 0);
	c->snd.unlock();
}

void
ALEF_sndsint(Chan *c, sint val, uint signature)
{
	ALEF_sndint(c, val, signature);
}

void
ALEF_sndchar(Chan *c, byte val, uint signature)
{
	ALEF_sndint(c, val, signature);
}
