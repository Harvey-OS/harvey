double
dist(o1, o2)
struct obj *o1, *o2;
{
	double a;

	a = sin(o1->lat) * sin(o2->lat) +
		cos(o1->lat) * cos(o2->lat) *
		cos(o1->lng - o2->lng);
	a = atan2(sqrt(1.0 - a*a), a);
	if(a < 0.0)
		a = -a;
	return(a * 3444.054);
}
