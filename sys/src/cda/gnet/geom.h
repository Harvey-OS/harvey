#include <stdio.h>
#ifndef GEOM_H
#define GEOM_H

#define min(x,y)	((x < y) ? x : y)
#define max(x,y)	((x > y) ? x : y)

class Rectangle;

class Point {
public:
	int x,y;
	Point(){}
	Point(int u, int v)	{x=u; y=v;}
#define Pt(x,y)		Point(x,y)
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
#define Rpt(p,q)	Rectangle(p,q)
	Rectangle(int x, int y, int u, int v)	{o.x=x; o.y=y; c.x=u; c.y=v;}
#define Rect(x,y,u,v)	Rectangle(x,y,u,v)
	Rectangle operator+(Point p)	{return Rectangle(o+p,c+p);}
	Rectangle operator-(Point p)	{return Rectangle(o-p,c-p);}
	Rectangle translate(Point p)	{return Rectangle(p,c+(p-o));}
	inline int operator<(Rectangle);
	inline int operator<=(Rectangle);
	Point center()		{return (o+c)/2;}
	Rectangle mbb(Point);		/* assumes a sorted Rectangle */
	Rectangle mbb(Rectangle);	/* ditto for argument */
	Rectangle inset(Point p)	{return Rectangle(o+p,c-p);}
};
inline int Point::operator<(Rectangle r)	{return *this>r.o && *this<r.c;}
inline int Point::operator<=(Rectangle r){return *this>=r.o && *this<=r.c;}
inline int Point::operator>(Rectangle r) {return !(*this<=r);}
inline int Rectangle::operator<(Rectangle r)	{return o<r && c<r;}
inline int Rectangle::operator<=(Rectangle r)	{return o<=r && c<=r;}

/* nonsense because unions don't do it */
#define P	R.o
#define Q	R.c
#define X	P.x
#define Y	P.y
#define U	Q.x
#define V	Q.y
#define ul	P
#define ur	Pt(U,Y)
#define ll	Pt(X,V)
#define lr	Q

#endif
