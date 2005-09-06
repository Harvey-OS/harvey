#include <u.h>
#include <libc.h>
#include <draw.h>

#include "sokoban.h"

void
initanimation(Animation *a)
{
	if (a == nil)
		return;

	memset(a, 0, sizeof(Animation));
}

void
setupanimation(Animation *a, Route *r)
{
	if (a == nil || r == nil || r->step == nil)
		return;

	a->route = r;
	a->step = r->step;
	if (a->step < a->route->step + a->route->nstep)
		a->count = a->step->count;
	else
		stopanimation(a);
}

int
onestep(Animation *a)
{
	if (a == nil)
		return 0;

	if (a->count > 0 && a->step != nil && a->route != nil) {
		move(a->step->dir);
		a->count--;
		if (a->count == 0) {
			a->step++;
			if (a->step < a->route->step + a->route->nstep)
				a->count = a->step->count;
			else
				stopanimation(a);
		}
	} else if (a->count > 0 && (a->step == nil || a->route == nil))
		stopanimation(a);
	return (a->count > 0);
}

void
stopanimation(Animation *a)
{
	if (a == nil)
		return;

	if (a->route != nil)
		freeroute(a->route);
	memset(a, 0, sizeof(Animation));
}
