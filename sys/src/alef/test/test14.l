#include <alef.h>


aggr X
{
	int a;
	int b;
};

aggr Y
{
	(int, int);
	(int, int);
};

X
foo()
{
	return (1, 2);
}

void
main()
{
	Y y;
	int a, b, c, d;

	((a, b), (c, d)) = (foo(), foo());

	print("%d %d %d %d\n", a, b, c, d);

	y = (foo(), foo());
	((a, b), (c, d)) = y;

	print("%d %d %d %d\n", y);
}
