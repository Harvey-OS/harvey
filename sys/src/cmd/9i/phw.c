#include <u.h>
#include <libc.h>

int	value(int);

void
main(void)
{
	print("hello world %d\n", value(12345));
	exits(0);
}

int
value(int i)
{
	return i+1;
}
