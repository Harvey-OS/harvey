#include <u.h>
#include <libc.h>

vlong
b(vlong a1, int i1, void *p1)
{
	return a1 + atoll((char *)p1) + i1;
}

vlong
a(vlong a1, int i1, void *p1)
{
	return b(a1, i1, p1);
}

void
main(void)
{
	print("stuff %ld\n", a(34, 23, 0));
	exits(0);
}
