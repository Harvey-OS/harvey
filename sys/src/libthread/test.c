#include <u.h>
#include <libc.h>

void
printabcd(int a, int b, int c, int d)
{
	print("%d %d %d %d\n", a, b, c, d);
}

void
main(int argc, char **argv)
{
	printabcd(atoi(argv[0]), atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
}
