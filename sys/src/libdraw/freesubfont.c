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

void
freesubfont(Subfont *f)
{
	if(f == 0)
		return;
	f->ref--;
	if(f->ref > 0)
		return;
	uninstallsubfont(f);
	free(f->info);	/* note: f->info must have been malloc'ed! */
	freeimage(f->bits);
	free(f);
}
