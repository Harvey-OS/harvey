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
#include <draw.h>

int
rectclip(Rectangle *rp, Rectangle b)		/* first by reference, second by value */
{
	Rectangle *bp = &b;
	/*
	 * Expand rectXrect() in line for speed
	 */
	if((rp->min.x<bp->max.x && bp->min.x<rp->max.x &&
	    rp->min.y<bp->max.y && bp->min.y<rp->max.y)==0)
		return 0;
	/* They must overlap */
	if(rp->min.x < bp->min.x)
		rp->min.x = bp->min.x;
	if(rp->min.y < bp->min.y)
		rp->min.y = bp->min.y;
	if(rp->max.x > bp->max.x)
		rp->max.x = bp->max.x;
	if(rp->max.y > bp->max.y)
		rp->max.y = bp->max.y;
	return 1;
}
