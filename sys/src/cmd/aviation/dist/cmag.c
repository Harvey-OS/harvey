#define	DLAT	2.0
#define	DLNG	3.0
#define	RADIAN	(3.14159265359/180.)
double	magvar();
double	dist();
struct	obj
{
	char	name[4];
	float	lat;
	float	lng;
} ob;

main()
{
	double lat, lng;
	double m;

	print("struct mag { float lat; float lng; float var; } mag[] = {\n");
	for(lat = 23.0; lat < 50.001; lat = lat + DLAT)
	{
		ob.lat = lat;
		for(lng = 65.0; lng < 126.001; lng = lng + DLNG)
		{
			ob.lng = lng;
			m = magvar(&ob);
			if(m > 99.0)
				continue;
			print("%.7f, %.7f, %.2f,", lat*RADIAN, lng*RADIAN, m);
			print("	/* %.0f %.0f */\n", lat, lng);
		}
	}
	print("0., 0., 1000.\n};");
	return(0);
}
struct	mag
{
	float	lat;
	float	lng;
	float	var;
} amag[], lmag[100];

double
magvar(o)
struct obj *o;
{
	register i, j;
	double d, t, w;


	j = 0;
	for(i=0; amag[i].var < 500.; i++) {
		if(amag[i].lat > o->lat+DLAT)
			continue;
		if(amag[i].lat < o->lat-DLAT)
			continue;
		if(amag[i].lng > o->lng+DLNG)
			continue;
		if(amag[i].lng < o->lng-DLNG)
			continue;
		lmag[j] = amag[i];
		j++;
		if(j >= 100)
			j = near(o, lmag, j);
	}
	j = near(o, lmag, j);
/*
	print("%.0f %.0f\n", o->lat, o->lng);
	for(i=0; i<j; i++)
		print("	%.7f %.7f %.2f\n",
			lmag[i].lat, lmag[i].lng, lmag[i].var);
*/
	if(j == 0)
		return(100.);
	t = 0.;
	w = 0.;
	for(i=0; i<j; i++) {
		d = dist(o, lmag+i);
		if(d < 1e-6)
			d = 1e-6;
		d = 1.0/d;
		t += lmag[i].var * d;
		w += d;
	}
	return(t/w);
}

double
dist(m1, m2)
struct obj *m1;
struct mag *m2;
{
	double d1, d2;
	double cos();

	d1 = m1->lat - m2->lat;
	d1 = d1*d1;
	d2 = m1->lng - m2->lng;
	d2 = d2 * cos(m1->lat*RADIAN);
	d2 = d2*d2;
	return(d1+d2);
}

#define	N 10
near(o, a, n)
struct obj *o;
struct mag *a;
{
	register i, j;
	int xj;
	double d[N], x, x1;

	if(n <= N)
		return(n);
	for(i=0; i<N; i++)
		d[i] = dist(o, a+i);
	for(i=N; i<n; i++) {
		x = dist(o, a+i);
		x1 = x;
		xj = N;
		for(j=0; j<N; j++)
			if(x < d[j]) {
				xj = j;
				x = d[j];
			}
		if(xj < N) {
			d[xj] = x1;
			a[xj] = a[i];
		}
	}
	return(N);
}
