#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#undef memmove

void	*
kpmemmove(void *s, void *d, long n)
{
	void *p;
	if(up == nil)
		return memmove(s, d, n);

	up->kppc = getcallerpc(&s);
	p = memmove(s, d, n);
	up->kppc = 0;
	return p;
}
