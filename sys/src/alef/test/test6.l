#include <alef.h>

void
main(void)
{
	int a,b,c;

	a = b/c;

	for(a = 0; a < 100; a++)
		a = 100*b+c;

	print("", a, b, c);
}
