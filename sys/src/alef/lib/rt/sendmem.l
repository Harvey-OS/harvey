#include	<alef.h>
#include	"run.h"

void
ALEF_sndmem(Chan *c, void *p, int len, uint signature)
{
	Chan *n;
	Task *t;
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		memmove(c->rva, p, len);
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}

	if(c->async == 0) {
		c->signature = signature;
		c->sva = p;
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
	memmove(b->data, p, len);
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
