extern long a[];
long
f(long l, long r, long x, long y, long z)
{
	return (l*r)*(x/y)*(z/a[y])*a[l];
}
