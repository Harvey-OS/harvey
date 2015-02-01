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
#include <bio.h>
#include <libsec.h>
#include <disk.h>
#include "iso9660.h"

void
dirtoxdir(XDir *xd, Dir *d)
{
	xd->name = atom(d->name);
	xd->uid = atom(d->uid);
	xd->gid = atom(d->gid);
	xd->uidno = 0;
	xd->gidno = 0;
	xd->mode = d->mode;
	xd->atime = d->atime;
	xd->mtime = d->mtime;
	xd->ctime = 0;
	xd->length = d->length;
};

void
fdtruncate(int fd, ulong size)
{
	USED(fd, size);
}
