#include <alef.h>

chan(tuple (int, byte*)) a;

void
t()
{
	int x;
	byte *p;

	(x, p) = <-a;
	print("x=%d p=%s\n", x, p);
}

void
main()
{
	alloc a;

	task t();
	a <-= (1, "hello");

	task t();
	a <-= (2, "world");
}
