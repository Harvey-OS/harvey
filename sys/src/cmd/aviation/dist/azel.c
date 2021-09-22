#include <u.h>
#include <libc.h>
#include "obj.h"
#include "sphere.h"

#define	LAT	(40.0+41.0/60.0)
#define	LNG	(74.0+24.0/60.0)
#define	RADS	(20600.+RADE)

main(int argc, char *argv[])
{
	double lat1, lat2, lng1, lng2;
	double a, d, x, b, c;

	if(argc < 2) {
		print("azel lat\n");
		exit(1);
	}
	lat1 = LAT * RADIAN;
	lng1 = LNG * RADIAN;
	lat2 = 0.0 * RADIAN;
	lng2 = atof(argv[1]) * RADIAN;

	/* a is az */
	a = sphere(MYPI/2.-lat2, MYPI/2.-lat1, MISSING,
		UNKNOWN, MISSING, fabs(lng1-lng2));
	if(lng1 < lng2)
		a = 2.0*MYPI - a;
	print("a = %.2f\n", a/RADIAN);

	/* d is sep ang from center of earth */
	d = sin(lat1) * sin(lat2) +
		cos(lat1) * cos(lat2) *
		cos(lng1 - lng2);
	d = atan2(sqrt(1- d*d), d);
	if(d < 0.0)
		d = -d;
	print("d = %.2f\n", d*RADE);


	/* x is range to sat */
	x = sqrt(RADE*RADE + RADS*RADS - 2*RADE*RADS*cos(d));
	print("x = %.2f\n", x);

	/* b is sin of angle between center and sat */
	b = RADS*sin(d)/x;
	print("b = %.2f\n", asin(b)/RADIAN);

	/* c is desired */
	c = 90. - asin(b)/RADIAN;
	print("c = %.2f\n", c);
	exit(0);
}
