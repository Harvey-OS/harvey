/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "libufs.h"


enum {
	Qdir = 0,		// #U
	Qufsdir,		// #U/ufs
	Qmount,			// #U/ufs/mount
	QmountIdBase = 1000,	// #U/ufs/0-MaxMounts
	Qmax,
};

static Dirtab ufsdir[] =
{
	{"mount",	{ Qmount },	0,	0666},
};


#define QID(q)	((int)(q).path)

// Just one possible mountpoint for now.  Replace with a collection later
static struct mount* mountpoint = nil;

static int MaxMounts = 1;


static int
ufsgen(Chan* c, char* d, Dirtab* dir, int j, int s, Dir* dp)
{
	Qid q;
	char name[64];

	//print("ufsgen q %#x s %d...\n", QID(c->qid), s);

	if(s == DEVDOTDOT) {
		if (QID(c->qid) <= Qufsdir) {
			mkqid(&q, Qdir, 0, QTDIR);
			devdir(c, q, "#U", 0, eve, 0555, dp);
		} else {
			mkqid(&q, Qufsdir, 0, QTDIR);
			devdir(c, q, "ufs", 0, eve, 0555, dp);
		}
		return 1;
	}

	switch (QID(c->qid)) {
	case Qdir:				// list #U
		if (s == 0) {
			mkqid(&q, Qufsdir, 0, QTDIR);
			devdir(c, q, "ufs", 0, eve, 0555, dp);
			return 1;
		}
		return -1;

	case Qufsdir:				// list #U/ufs
		if (s < nelem(ufsdir)) {
			// Populate with ufsdir table
			dir = &ufsdir[s];
			mkqid(&q, dir->qid.path, 0, QTFILE);
			devdir(c, q, dir->name, dir->length, eve, dir->perm, dp);
			return 1;
		}
		s -= nelem(ufsdir);
		if (s < 0 || s >= MaxMounts) {
			return -1;
		}

		if (mountpoint == nil) {
			return -1;
		}

		// Mount point (more in the future)
		sprint(name, "%d", s);
		mkqid(&q, QmountIdBase+s, 0, QTDIR);
		devdir(c, q, name, 0, eve, 0555, dp);
		return 1;

	default:
		return -1;
	}
}


static Chan*
ufsattach(char* spec)
{
	return devattach('U', spec);
}

Walkqid*
ufswalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, ufsgen);
}

static int32_t
ufsstat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, nil, 0, ufsgen);
}

static Chan*
ufsopen(Chan* c, int omode)
{
	return devopen(c, omode, nil, 0, ufsgen);
}

static void
ufsclose(Chan*unused)
{
}

static int32_t
ufsread(Chan *c, void *a, int32_t n, int64_t offset)
{
	if (c->qid.type == QTDIR) {
		return devdirread(c, a, n, nil, 0, ufsgen);
	}

	error(Eperm);

	return n;
}

static struct mount*
mountufs(Chan* c)
{
	struct mount* mp = newufsmount(c);
	if (mp == nil) {
		print("couldn't prepare UFS mount\n");
		error(Enoattach);
	}

	int rcode = ffs_mount(mp);
	if (rcode != 0) {
		print("couldn't mount as UFS.  Error code: %d\n", rcode);
		error(Enoattach);
	}

	return mp;
}

static void
mount(char* a, int32_t n)
{
	Proc *up = externup();

	// Accept only one mount for now
	if (mountpoint != nil) {
		error(Enoattach);
	}

	Cmdbuf* cb = parsecmd(a, n);
	if (waserror()) {
		print("couldn't parse mount path: %s: %r", a);
		free(cb);
		nexterror();
	}
	poperror();

	// TODO Make namec and related functions accept const char*
	Chan* c = namec(cb->buf, Aopen, ORDWR, 0);
	free(cb);
	cb = nil;
	if (waserror()) {
		print("couldn't open file %s to mount: %r", cb->buf);
		cclose(c);
		nexterror();
	}
	poperror();

	struct mount* mp = mountufs(c);
	if (waserror()) {
		print("couldn't mount %s: %r", cb->buf);
		cclose(c);
		nexterror();
	}
	poperror();

	mountpoint = mp;
}

static int32_t
ufswrite(Chan *c, void *a, int32_t n, int64_t offset)
{
	if(c->qid.type == QTDIR) {
		error(Eisdir);
		return -1;
	}

	switch (QID(c->qid)) {
	case Qmount:
		mount((char*)a, n);
		return n;

	default:
		error(Eperm);
		return -1;
	}
}

Dev ufsdevtab = {
	.dc = 'U',
	.name = "ufs",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = ufsattach,
	.walk = ufswalk,
	.stat = ufsstat,
	.open = ufsopen,
	.create = devcreate,
	.close = ufsclose,
	.read = ufsread,
	.bread = devbread,
	.write = ufswrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
