/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "map.h"
extern void abort(void);

/* these routines duplicate names found in map.c.  they are
called from routines in hex.c, guyou.c, and tetra.c, which
are in turn invoked directly from map.c.  this bad organization
arises from data hiding; only these three files know stuff
that's necessary for the proper handling of the unusual cuts
involved in these projections.

the calling routines are not advertised as part of the library,
and the library duplicates should never get loaded, however they
are included to make the libary self-standing.*/

int
picut(struct place *g, struct place *og, double *cutlon)
{
	g; og; cutlon;
	abort();
	return 0;
}

int
ckcut(struct place *g1, struct place *g2, double lon)
{
	g1; g2; lon;
	abort();
	return 0;
}

double
reduce(double x)
{
	x;
	abort();
	return 0;
}
