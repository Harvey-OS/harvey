#include <alef.h>

aggr Com
{
	int a;
	int b;
};

Com
x(Com a, int c)
{
	if(c > 3)
		return a;

	print("%d %d\n", a.a, a.b);
	become x(a, c+1);
}

Com
y(Com a)
{
	become x(a, 0);
}


void
main()
{
	Com t;

	t = y((Com)(1234, 5678));
	print("%d %d\n", t.a, t.b);
}

