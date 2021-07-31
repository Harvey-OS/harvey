#include "type.h"

typedef	struct	Mag	Mag;

struct	Mag
{
	int	index[3];	/* index of 3 closest variations */
	long	dist[3];	/* distance of 3 closest variations */
	int	count;		/* time to go around list of variations */
	int	ramp;		/* delta variation to apply per step */
	int	i;		/* current index to search variation */
};

static
Mag	mag;

void
mginit(void)
{

	plane.x = 0;
	plane.y = 0;
	plane.z = 5000;
	setxyz('S', 'B', 'J');
}

void
mgupdat(void)
{
	int i, j, k;
	long d, w;

	plane.magvar += mag.ramp;
	i = mag.i;
	mag.i++;
	if(var[i].v == 0) {
		mag.i = 0;
		calcvar();
		return;
	}
	d = fdist(var[i].x, var[i].y);
	w = 0;
	k = -1;
	for(j=0; j<3; j++) {
		if(i == mag.index[j]) {
			mag.dist[j] = d;
			return;
		}
		if(mag.dist[j] > w) {
			w = mag.dist[j];
			k = j;
		}
	}
	if(d < w && k >= 0) {
		mag.index[k] = i;
		mag.dist[k] = d;
	}
}

void
calcvar(void)
{
	long r0, r1, r2, d;
	Fract a0, a1, a2;

	r0 = mag.dist[0] / 100;
	r1 = mag.dist[1] / 100;
	r2 = mag.dist[2] / 100;
	d = r0*r1 + r0*r2 + r1*r2;
	a0 = fdiv(r1*r2, d);
	a1 = fdiv(r0*r2, d);
	a2 = fdiv(r0*r1, d);
	d = fmul((ulong)var[mag.index[0]].v, a0);
	d += fmul((ulong)var[mag.index[1]].v, a1);
	d += fmul((ulong)var[mag.index[2]].v, a2);
	mag.ramp = (d - (int)plane.magvar)/mag.count;

}

void
setxyz(int a, int b, int c)
{
	int i;
	unsigned n;

	if(a == 'X')
	if(b == 'X')
	if(c == 'X')
		exits(0);
	n = a-'A';
	n = n*40 + b-'A';
	n = n*40 + c-'A';
	for(i=0; nav[i].z; i++) {
		if(!(nav[i].flag & VOR))
			continue;
		if(nav[i].obs == n) {
			plane.x = nav[i].x;
			plane.y = nav[i].y;
			plane.z = nav[i].z + 1000;
		}
	}
	for(i=0; i<3; i++) {
		mag.index[i] = i;
		mag.dist[i] = fdist(var[i].x, var[i].y);
	}
	mag.i = 0;
	plane.magvar = 0;
	mag.ramp = 0;
	/*
	 * loop to find size of magupdate cycle
	 */
	for(mag.count=0; plane.magvar == 0; mag.count++)
		mgupdat();
	/*
	 * that many more to ramp the planes magvar
	 */
	for(i=0; i<mag.count; i++)
		mgupdat();
}
