#include <u.h>
#include <libc.h>

extern int _nsec(vlong*);

vlong
nsec(void)
{
	vlong l;

	if(_nsec(&l) < 0)
		l = -1LL;
	return l;
}
