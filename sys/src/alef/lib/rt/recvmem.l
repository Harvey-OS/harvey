#include	<alef.h>
#include	"run.h"

void
ALEF_rcvmem(Chan *c, int l)
{
	Chan *n;
	Task *t;
	void *p;
	Msgbuf *b;

	p = ALEF_getrp();

	c->rcv.lock();
	c->lock();
	if(c->qh) {
		b = c->qh;
		memmove(p, b->data, l);
		c->qh = b->next;
		b->next = c->free;
		c->free = b;
		c->unlock();
		if(b->next == nil)
			c->br.Wakeup();
		c->rcv.unlock();
		return;
	}
	if(c->sva) {
		memmove(p, c->sva, l);
		c->sva = nil;
		c->unlock();
		c->rcv.unlock();
		c->sndr.Wakeup();
		return;
	}
	c->rva = p;
	c->unlock();
	t = c->selt;
	if(t != nil && c->seltst != 0) {
		n = t->slist;
		if(n != nil)
			n->selp = 1;
		t->Wakeup();
	}
	c->rcvr.Sleep(&c->rva, 0);
	c->rcv.unlock();
}
