#include <alef.h>

int
Ref.inc(Ref *r)
{
	int i;

	r->lock();
	i = r->ref;
	r->ref++;
	r->unlock();

	return i;
}

int
Ref.dec(Ref *r)
{
	int i;

	r->lock();
	r->ref--;
	i = r->ref;
	r->unlock();

	return i;
}

int
Ref.val(Ref *r)
{
	return r->ref;
}
