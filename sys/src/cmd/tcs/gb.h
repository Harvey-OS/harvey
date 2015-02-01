/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
	gb ranges from a1a1 to f7fe inclusive
	we use a kuten-like mapping the above range to 101-8794
*/
#define		GBMAX	8795

extern long tabgb[GBMAX];	/* runes indexed by gb ordinal */
