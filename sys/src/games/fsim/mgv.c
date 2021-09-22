#include "type.h"

typedef	struct	Mag	Mag;

struct	Mag
{
	Nav*	apt;		/* nearest airport */
	long	dist;		/* distance to nearest airport */
	Angle	magvar;		/* desired mag var, slowly chase this value */
	int	i;		/* current index to search variation */
};

static	Mag	mag;

void
mginit(void)
{
}

void
mgupdat(void)
{
	int i;
	long d;
	Angle v;
	Nav *a;

	if(plane.magvar != mag.magvar) {
		v = mag.magvar - plane.magvar;
		v &= 2*PI-1;
		if(v >= DEG(180))
			plane.magvar--;
		else
			plane.magvar++;
		plane.magvar &= 2*PI-1;
	}

	i = mag.i;
	a = &nav[i];
	while((a->flag & APT) == 0 || a->obs == 0) {
		if(a->name == nil) {
			mag.i = 0;
			return;
		}
		i++;
		a++;
	}
	mag.i = i+1;

	d = fdist(a);
	if(a == mag.apt) {
		mag.dist = d;
		return;
	}
	if(d < mag.dist) {
		mag.apt = a;
		mag.dist = d;
		mag.magvar = DEG(a->obs/10);
	}
}

Nav*
lookup(char *name, int flag)
{
	Nav *v;
	int c0;

	c0 = name[0];
	for(v=nav; v->name != nil; v++) {
		if(!(v->flag & flag))
			continue;
		if(v->name[0] == c0)
			if(strcmp(name, v->name) == 0)
				return v;
	}
	return 0;
}

int
setxyz(char *apt)
{
	Nav *a;
	long d;

	if(strcmp(apt, "xxx") == 0)
		exits(0);

	a = lookup(apt, APT);
	if(a == nil)
		return 0;

	plane.lat = a->lat;
	plane.lng = a->lng;
	plane.z = a->z + 1000;
	plane.coslat = cos(plane.lat * C1);

	mag.i = 0;
	mag.dist = 0;
	mag.apt = nil;

	for(a=nav; a->name; a++) {
		if((a->flag & APT) && a->obs != 0) {
			d = fdist(a);
			if(d < mag.dist || mag.apt == nil) {
				mag.apt = a;
				mag.dist = d;
			}
		}
	}

	mag.magvar = DEG((mag.apt->obs + 5)/10);
	plane.magvar = mag.magvar;

	return 1;
}
