#include	"type.h"

static	Apt	*app;
static	long	dap1, dap2;

void
apinit(void)
{
	app = &apt[0];

	plane.ap1 = &apt[0];
	plane.ap2 = &apt[1];
	dap1 = fdist(plane.ap1->x, plane.ap2->y);
	dap2 = fdist(plane.ap2->x, plane.ap2->y);
}

void
apupdat(void)
{
	Apt *ap;
	long d1;

	ap = app+1;
	if(!ap->z)
		ap = &apt[0];
	app = ap;
	if(ap == plane.ap1 || ap == plane.ap2)
		return;
	d1 = fdist(ap->x, ap->y);
	if(dap1 < d1) {
		plane.ap1 = ap;
		dap1 = d1;
		return;
	}
	if(dap2 < d1) {
		plane.ap2 = ap;
		dap2 = d1;
	}
}
