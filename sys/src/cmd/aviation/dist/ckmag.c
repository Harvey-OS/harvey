#include <u.h>
#include <libc.h>
#include	"obj.h"
extern	Mag	 amag[];
Obj	o;

main(int argc, char *argv[])
{
	double d, e;
	int i;

	for(i=0; amag[i].var < 500; i++) {
		o.lat = amag[i].lat * RADIAN;
		o.lng = amag[i].lng * RADIAN;
		d = magvar(&o);
		e = d - amag[i].var;
		if(e < 0)
			e = -e;
		if(e > 1)
			print("%.5f, %.5f %.2f\n",
				amag[i].lat, amag[i].lng, e);
	}
	print("%d entries\n", i);
	exit(0);
}
