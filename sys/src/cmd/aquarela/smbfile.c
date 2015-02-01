/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

static void
smbfilefree(SmbFile **fp)
{
	SmbFile *f;
	f = *fp;
	if (f) {
		smbsharedfileput(f, f->sf, f->share);
		if (f->fd >= 0)
			close(f->fd);
		free(f->name);
		free(f);
		*fp = nil;
	}
}

void
smbfileclose(SmbSession *s, SmbFile *f)
{
	smblogprintif(smbglobals.log.fids, "smbfileclose: 0x%.4ux/0x%.4ux %s%s\n",
		f->t->id, f->id, f->t->serv->path, f->name);
	smbidmapremove(s->fidmap, f);
	smbfilefree(&f);
}
