#include <alef.h>

adt Point
{
	int	x;
	int	y;

	Point	add(.Point, Point);
	Point	sub(.Point, Point);
	void	print(*Point);
};

Point
Point.add(Point a, Point b)
{
	return (Point)(a.x+b.x, a.y+b.y);
}

Point
Point.sub(Point a, Point b)
{
	return (Point)(a.x-b.x, a.y-b.y);
}

void
Point.print(Point *a)
{
	print("%d %d\n", a->x, a->y);
}

void
main()
{
	Point a, b, c, d;

	a = (Point)(10, 10);
	b = (Point)(5, 5);
	c = (Point)(2, 2);

	d = a.add(b).sub(c);
	d.print();
}
