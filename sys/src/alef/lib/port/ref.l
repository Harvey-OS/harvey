#include <alef.h>

int
Ref.inc(Ref *r)
{
	int i;

	r->l.lock();
	i = r->cnt;
	r->cnt++;
	r->l.unlock();

	return i;
}

int
Ref.dec(Ref *r)
{
	int i;

	r->l.lock();
	r->cnt--;
	i = r->cnt;
	r->l.unlock();

	return i;
}

int
Ref.ref(Ref *r)
{
	return r->cnt;
}
