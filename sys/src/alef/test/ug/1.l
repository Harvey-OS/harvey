#include	<alef.h>

void
receive(chan(byte*) c)
{
	byte *s;

	s = <-c;
	print("%s\n", s);
	terminate(nil);
}

void
main(void)
{
	chan(byte*) c;

	alloc c;
	proc receive(c);

	c <-= "hello world";
	print("done\n");
	terminate(nil);
}
