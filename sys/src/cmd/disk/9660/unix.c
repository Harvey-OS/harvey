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
#include <libproto.h>
#include <ctype.h>

#include "iso9660.h"

#include <grp.h>
#include <pwd.h>

typedef struct Xarg Xarg;
struct Xarg {
	void (*enm)(char*,char*,XDir*,void*);
	void (*warn)(char*,void*);
	void *arg;
};

static long numericuid(char *user);
static long numericgid(char *gp);

void
dirtoxdir(XDir *xd, Dir *d)
{
	//	char buf[NAMELEN+1];
	memset(xd, 0, sizeof *xd);

	xd->name = atom(d->name);
	xd->uid = atom(d->uid);
	xd->gid = atom(d->gid);
	xd->uidno = numericuid(d->uid);
	xd->gidno = numericgid(d->gid);
	xd->mode = d->mode;
	xd->atime = d->atime;
	xd->mtime = d->mtime;
	xd->ctime = 0;
	xd->length = d->length;
	if(xd->mode & CHLINK) {
		xd->mode |= 0777;
		xd->symlink = atom(d->symlink);
	}
};

void
fdtruncate(int fd, ulong size)
{
	ftruncate(fd, size);

	return;
}

static long
numericuid(char *user)
{
	struct passwd *pass;
	static int warned = 0;

	if (! (pass = getpwnam(user))) {
		if (!warned)
			fprint(2, "Warning: getpwnam(3) failed for \"%s\"\n", user);
		warned = 1;
		return 0;
	}

	return pass->pw_uid;
}

static long
numericgid(char *gp)
{
	struct group *gr;
	static int warned = 0;

	if (! (gr = getgrnam(gp))) {
		if (!warned)
			fprint(2, "Warning: getgrnam(3) failed for \"%s\"\n", gp);
		warned = 1;
		return 0;
	}

	return gr->gr_gid;
}
