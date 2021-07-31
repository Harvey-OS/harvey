#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<libg.h>
#include	"hdr.h"
#include	"../kuten.h"

/*
	map: put kuten for runes from..to into chars
*/


void
map(int from, int to, long *chars)
{
	long *l, *ll;
	int k, k1, n;

	for(n = from; n <= to; n++)
		chars[n-from] = 0;
	for(l = tabkuten, ll = tabkuten+KUTENMAX; l < ll; l++)
		if((*l >= from) && (*l <= to))
			chars[*l-from] = l-tabkuten;
	k = 0;
	k1 = 0;		/* not necessary; just shuts ken up */
	for(n = from; n <= to; n++)
		if(chars[n-from] == 0){
			k++;
			k1 = n;
		}
	if(k){
		fprint(2, "%s: %d chars couldn't be found, e.g. 0x%x(=%d)\n", argv0, k, k1, k1);
		/*exits("map problem");/**/
	}
}
