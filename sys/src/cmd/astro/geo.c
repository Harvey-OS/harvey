/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "astro.h"

void
geo(void)
{

/*
 *	uses alpha, delta, rp
 */

/*
 *	sets ra, decl, lha, decl2, az, el
 */

/*
 *	geo converts geocentric equatorial coordinates
 *	to topocentric equatorial and topocentric horizon
 *	coordinates.
 *	All are (usually) referred to the true equator.
 */

	double sel, saz, caz;
	double f;
	double sa, ca, sd;

/*
 *	convert to local hour angle and declination
 */

	lha = gst - alpha - wlong;
	decl = delta;

/*
 *	compute diurnal parallax (requires geocentric latitude)
 */

	sa = cos(decl)*sin(lha);
	ca = cos(decl)*cos(lha) - erad*cos(glat)*sin(hp);
	sd = sin(decl)           - erad*sin(glat)*sin(hp);

	lha = atan2(sa, ca);
	decl2 = atan2(sd, sqrt(sa*sa+ca*ca));
	f = sqrt(sa*sa+ca*ca+sd*sd);
	semi2 = semi/f;
	ra = gst - lha - wlong;
	ra = pinorm(ra);

/*
 *	convert to horizon coordinates
 */

	sel = sin(nlat)*sin(decl2) + cos(nlat)*cos(decl2)*cos(lha);
	el = atan2(sel, pyth(sel));
	saz = sin(lha)*cos(decl2);
	caz = cos(nlat)*sin(decl2) - sin(nlat)*cos(decl2)*cos(lha);
	az = pi + atan2(saz, -caz);

	az /= radian;
	el /= radian;
}
