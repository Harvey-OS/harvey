#include "lib9.h"

int
refinc(Ref *r)
{
	int i;

	lock(&r->l);
	i = r->ref;
	r->ref++;
	unlock(&r->l);
	return i;
}

int	
refdec(Ref *r)
{
	int i;

	lock(&r->l);
	r->ref--;
	i = r->ref;
	unlock(&r->l);

	return i;
}
