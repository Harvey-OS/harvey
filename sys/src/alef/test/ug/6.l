#include	<alef.h>

tuple(int, byte*, byte*)
func()
{
	return (10, "string1", "string2");
}

void
main(void)
{
	int a;
	byte *b, *c;

	(a, b, c) = func();
	print("tuple = (%d %s %s)\n", a, b, c);
}
