/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<libg.h>
#include	"hdr.h"
#include	"../gb.h"

/*
	map: put gb for runes from..to into chars
*/

void
gmap(int from, int to, long *chars)
{
	long *l, *ll;
	int k, k1, n;

	for(n = from; n <= to; n++)
		chars[n-from] = 0;
	for(l = tabgb, ll = tabgb+GBMAX; l < ll; l++)
		if((*l >= from) && (*l <= to))
			chars[*l-from] = l-tabgb;
	k = 0;
	k1 = 0;		/* not necessary; just shuts ken up */
	for(n = from; n <= to; n++)
		if(chars[n-from] == 0){
			k++;
			k1 = n;
		}
	if(k){
		fprint(2, "%s: %d/%d chars found (missing include 0x%x=%d)\n", argv0, (to-from+1-k), to-from+1, k1, k1);
		/*exits("map problem");/**/
	}
}
