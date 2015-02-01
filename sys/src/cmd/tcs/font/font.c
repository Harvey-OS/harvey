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
#include	<libg.h>
#include	<bio.h>
#include	"hdr.h"

Subfont *
bf(int n, int size, Bitmap *b, int *done)
{
	Fontchar *fc;
	int i, j;
	Subfont *f;

	fc = (Fontchar *)malloc(sizeof(Fontchar)*(n+1));
	if(fc == 0){
		fprint(2, "%s: fontchar malloc(%d) failure\n", argv0, sizeof(Fontchar)*(n+1));
		exits("fontchar malloc failure");
	}
	j = 0;
	for(i = 0; i <= n; i++){
		fc[i] = (Fontchar){j, 0, size, 0, size};
		if(done[i])
			j += size;
		else
			fc[i].width = 0;
	}
	fc[n] = (Fontchar){j, 0, size, 0, size};
	f = subfalloc(n, size, size*7/8, fc, b, ~0, ~0);
	if(f == 0){
		fprint(2, "%s: falloc failure\n", argv0);
		exits("falloc failure");
	}
	return(f);
}
