#include	<alef.h>

void
main(void)
{
	float a;
	byte *b;
	tuple(int, int, byte*) t;
	byte i;

	i = 1;
	t = (0, (int) i, "abc");
	t = (tuple (int, int, byte*)) (0, i, "abc");

	t = (100, 200, "xyz");
	(nil, a, b) = t;
	print("a = %f b = %s\n", a, b);
}
