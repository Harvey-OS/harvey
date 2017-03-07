/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

mpint*
mpfield(mpint *N){
	Mfield *f;

	if(N == nil || N->flags & (MPfield|MPstatic))
		return N;
	if((f = cnfield(N)) != nil)
		goto Exchange;
	if((f = gmfield(N)) != nil)
		goto Exchange;
	return N;
Exchange:
	setmalloctag(f, getcallerpc());
	mpfree(N);
	return &(f->mpi);
}
