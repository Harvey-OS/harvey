/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "a.h"

/*
 * 13. Hyphenation.
 */

void
t13init(void)
{
	addreq(L("nh"), r_nop, -1);
	addreq(L("hy"), r_nop, -1);
	addreq(L("hc"), r_nop, -1);
	addreq(L("hw"), r_nop, -1);
	
	addesc('%', e_nop, 0);
}

