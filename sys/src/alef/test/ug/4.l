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

Point
midpoint(Point p1, Point p2)
{
	return ((p1.x+p2.x)/2,(p1.y+p2.y)/2);
}

void
main(void)
{
	Circle c;
	Point p;
	int rad;

	c.x = 1; c.y = 2;
	c.radius = 5;
	(p, rad) = c;

	print("p = (%d, %d) rad = %d\n", p.x, p.y, rad);

	c = (Circle)((1.0,1.0), 3);
	c = ((Point)(1.0,1.0), 3);

	p = midpoint((1,1), (3,1));
	print("p = (%d, %d)\n", p.x, p.y);
}
