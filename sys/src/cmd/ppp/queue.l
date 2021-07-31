#include <alef.h>
#include "queue.h"

aggr Qelem {
	void	*p;
	Qelem	*next;
};

void
Queue.putq(Queue *q, void *p)
{
	Qelem *qelem;

	alloc qelem;
	q->lock();
	qelem->p = p;
	qelem->next = q->elems;
	q->elems = qelem;
	q->unlock();
}

void *
Queue.getq(Queue *q)
{
	Qelem	*next;
	void	*p;

	p = nil;
	q->lock();
	if(q->elems != nil) {
		p = q->elems->p;
		next = q->elems->next;
		unalloc q->elems;
		q->elems = next;
	}
	q->unlock();
	return p;
}

int
Queue.isempty(Queue *q)
{
	int ret;

	q->lock();
	if(q->elems == nil)
		ret = 1;
	else
		ret = 0;
	q->unlock();
	return ret;
}
