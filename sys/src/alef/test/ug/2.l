#include	<alef.h>

aggr Point {
	int x;
	int y;
};

aggr Circle {
	Point;
	int radius;
};

aggr Line {
	Point p1;
	Point p2;
};

aggr Shape {
	int 	type;
	union {
		Circle;
		Line;
	};
};

int
eqpt(Point p1, Point p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

void
main(void)
{
	Point p;
	Circle c;
	Shape s;

	c.x = 1; c.y = 1;
	c.radius = 5;
	p = c;
	print("p = (%d, %d)\n", p.x, p.y);

	print("EQ = %d\n", eqpt(c, p));

	memset(&s.Circle, 0, sizeof(s.Circle));
	print("s.Circle = (%d, %d) at %d\n", s.Circle.x, s.Circle.y, s.radius);
}
