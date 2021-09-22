#include	"type.h"

/*
 * continually calculate the 2 nearest airports
 */

static	Nav*	app;
static	long	dap1, dap2;

void
apinit(void)
{
	plane.ap1 = nil;
	plane.ap2 = nil;

	for(app = &nav[0];; app++) {
		if((app->flag & APT) == 0)
			continue;
		if(plane.ap1 == nil) {
			plane.ap1 = app;
			continue;
		}
		if(plane.ap2 == nil) {
			plane.ap2 = app;
			continue;
		}
		break;
	}
	dap1 = fdist(plane.ap1);
	dap2 = fdist(plane.ap2);
}

void
apupdat(void)
{
	Nav *ap;
	long d1;

	ap = app+1;
	if(ap->name == nil)
		ap = &nav[0];
	app = ap;
	if((app->flag & APT) == 0)
		return;
	if(ap == plane.ap1 || ap == plane.ap2)
		return;
	d1 = fdist(ap);
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
