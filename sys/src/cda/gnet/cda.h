#include <stdio.h>

class Rectangle;

class Point {
public:
	short x,y;
	Point(){}
	Point(int u, int v)	{x=u; y=v;}
	Point operator-()		{return Point(-x,-y);}
	Point operator+(Point p)	{return Point(x+p.x,y+p.y);}
	Point operator-(Point p)	{return Point(x-p.x,y-p.y);}
	Point operator*(int i)	{return Point(x*i,y*i);}
	Point operator/(int i)	{return Point(x/i,y/i);}
	Point operator%(int i)	{return Point(x%i,y%i);}
	Point operator&(int i)	{return Point(x&i,y&i);}
	int operator==(Point p)	{return x==p.x && y==p.y;}
	int operator!=(Point p)	{return x!=p.x || y!=p.y;}
	int operator>=(Point p)	{return x>=p.x && y>=p.y;}
	int operator<=(Point p)	{return x<=p.x && y<=p.y;}
	int operator>(Point p)	{return x>p.x && y>p.y;}
	int operator<(Point p)	{return x<p.x && y<p.y;}
	inline int operator<(Rectangle);
	inline int operator<=(Rectangle);
	inline int operator>(Rectangle);
};

class Rectangle {
public:
	Point o,c;	/* origin, corner */
	Rectangle(){}
	Rectangle(Point p, Point q)	{o=p; c=q;}
	Rectangle(int x, int y, int u, int v)	{o.x=x; o.y=y; c.x=u; c.y=v;}
	Rectangle operator+(Point p)	{return Rectangle(o+p,c+p);}
	Rectangle operator-(Point p)	{return Rectangle(o-p,c-p);}
	inline int operator<(Rectangle);
	inline int operator<=(Rectangle);
	Point center()		{return (o+c)/2;}
	Rectangle mbb(Point);		/* assumes a sorted Rectangle */
	Rectangle mbb(Rectangle);	/* ditto for argument */
};
inline int Point.operator<(Rectangle r) {return *this>r.o && *this<r.c;}
inline int Point.operator<=(Rectangle r) {return *this>=r.o && *this<=r.c;}
inline int Point.operator>(Rectangle r) {return !(*this<=r);}
inline int Rectangle.operator<(Rectangle r)	{return o<r && c<r;}
inline int Rectangle.operator<=(Rectangle r)	{return o<=r && c<=r;}

class List {
	List *next;
	List()	{next = 0;}
	List(List *l)	{next = l;}
};

class Wire:public List {
	Point p,q;
	char pf,qf;
	Wire(Point pp, Point qq)	{p = pp; q = qq; pf = qf = 0;}
};

class Box:public List {
	Rectangle r;
	Box(Rectangle rr)	{r = rr;}
};

class Label:public List {
	Point p;
	char *t1,*t2;
	Label(Point p, char *s1, char *s2);
};
