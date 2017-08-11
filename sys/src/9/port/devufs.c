/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// TODO HARVEY
// General todos...  I guess we need a general lock on reading and writing
// operations.  Would this compensate for the lack of sigdeferstop, which is
// present in FreeBSD's VFS_GET?.  I think so...

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "../ufs/ufs_ext.h"


enum {
	Qdir = 0,			// #U
	Qufsdir,			// #U/ufs
	Qmount,				// #U/ufs/mount
	Qmountdir,			// #U/ufs/x (x embedded in qid)
	Qmountctl,			// #U/ufs/x/ctl
	Qmountstats,			// #U/ufs/x/stats
};

enum {
	Qidshift = 6,
	MaxMounts = 1,			// Maximum possible mounts
};

static Dirtab ufsdir[] =
{
	{"mount",	{Qmount},	0,	0666},
};

static Dirtab ufsmntdir[] =
{
	{"ctl",		{Qmountctl},	0,	0666},
	{"stats",	{Qmountstats},	0,	0666},
};

enum
{
	CMunmount,
	CMtestmount,
};

static
Cmdtab mountcmds[] = {
	{CMunmount,	"unmount",	1},
	{CMtestmount,	"test",		1},
};

// Rather vague errors until we interpret the UFS error codes
static char Eufsmount[] = "could not mount";
static char Eufsunmount[] = "could not unmount";
static char Eufsnomp[] = "no empty mountpoints";
static char Eufsinvalidmp[] = "not a valid mountpoint";

// Just one possible mountpoint for now.  Replace with a collection later
static MountPoint *mountpoint = nil;


static int
ufsgen(Chan* c, char* d, Dirtab* dir, int j, int s, Dir* dp)
{
	Qid q;
	char name[64];

	// The qid has a mount id embedded in it, so we need to mask out the
	// real qid value.
	int qid = c->qid.path & ((1 << Qidshift)-1);
	int qidmntid = c->qid.path >> Qidshift;

	//print("ufsgen q %#x s %d qidmntid %d...\n", qid, s, qidmntid);

	if(s == DEVDOTDOT) {
		if (qid <= Qufsdir) {
			mkqid(&q, Qdir, 0, QTDIR);
			devdir(c, q, "#U", 0, eve, 0555, dp);
		} else {
			mkqid(&q, Qufsdir, 0, QTDIR);
			devdir(c, q, "ufs", 0, eve, 0555, dp);
		}
		return 1;
	}

	switch (qid) {
	case Qdir:
		// list #U
		if (s == 0) {
			mkqid(&q, Qufsdir, 0, QTDIR);
			devdir(c, q, "ufs", 0, eve, 0555, dp);
			return 1;
		}
		return -1;

	case Qufsdir:
		// list #U/ufs
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
		mkqid(&q, Qmountdir | (s << Qidshift), 0, QTDIR);
		devdir(c, q, name, 0, eve, 0555, dp);
		return 1;

	case Qmountdir:
		// Generate #U/ufs/x/...
		if (s >= nelem(ufsmntdir)) {
			return -1;
		}

		// #U/ufs/x/...
		if (!mountpoint) {
			return -1;
		}

		dir = &ufsmntdir[s];
		mkqid(&q, dir->qid.path | (qidmntid << Qidshift), 0, QTFILE);
		devdir(c, q, dir->name, dir->length, eve, dir->perm, dp);
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
ufsclose(Chan* unused)
{
}

static int
dumpstats(MountPoint *mp, void *a, int32_t n, int64_t offset)
{
	int i = 0;
	char *buf = malloc(READSTR);

	qlock(&mp->vnodes_lock);
	int numinuse = countvnodes(mp->vnodes);
	int numfree = countvnodes(mp->free_vnodes);
	qunlock(&mp->vnodes_lock);

	i += snprint(buf + i, READSTR - i, "Mountpoint:        %d\n", mp->id);
	i += snprint(buf + i, READSTR - i, "Num vnodes in use: %d\n", numinuse);
	i += snprint(buf + i, READSTR - i, "Num vnodes free:   %d\n", numfree);

	n = readstr(offset, a, n, buf);
	free(buf);

	return n;
}

static int32_t
ufsread(Chan *c, void *a, int32_t n, int64_t offset)
{
	if (c->qid.type == QTDIR) {
		return devdirread(c, a, n, nil, 0, ufsgen);
	}

	switch ((int)c->qid.path) {
	case Qmountstats:
		n = dumpstats(mountpoint, a, n, offset);
		break;
	default:
		error(Eperm);
	}

	return n;
}

static MountPoint*
mountufs(Chan* c)
{
	// TODO Use real IDs eventually
	int id = 0;
	MountPoint *mp = newufsmount(c, id);
	if (mp == nil) {
		print("couldn't prepare UFS mount\n");
		error(Eufsmount);
	}

	int rcode = ffs_mount(mp);
	if (rcode != 0) {
		print("couldn't mount as UFS.  Error code: %d\n", rcode);
		error(Eufsmount);
	}

	return mp;
}

static void
unmountufs()
{
	// Unnmount, then release the mountpoint even if there's been an error
	int rcode = ffs_unmount(mountpoint, 0);
	releaseufsmount(mountpoint);
	mountpoint = nil;

	if (rcode != 0) {
		print("couldn't unmount UFS.  Error code: %d\n", rcode);
		error(Eufsunmount);
	}
}

static void
testmount()
{
	if (mountpoint == nil) {
		error(Eufsinvalidmp);
	}

	// Get the root
	vnode *root = nil;
	int rcode = ufs_root(mountpoint, 0, &root);
	if (rcode != 0) {
		print("couldn't get root: %d", rcode);
	}

	// TODO Probably need to pass in:
	//   vdp Root vnode or pwd vnode?  Will an empty struct work?
	//   vpp nil?  Seems to be the desination.
	// vpp will be populated with the found vnode.
	// TODO UFS caches lookups.  We could do that in devufs.
	/*char path[] = "/";
	ComponentName cname;
	cname.cn_pnbuf = path;
	int rcode = ufs_lookup(mountpoint, &cname);
	if (rcode != 0) {
		print("couldn't lookup path: %s: %d", path, rcode);
	}*/
}

static void
mount(char* a, int32_t n)
{
	Proc *up = externup();

	// Accept only one mount for now
	if (mountpoint != nil) {
		error(Eufsnomp);
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

	MountPoint *mp = mountufs(c);
	if (waserror()) {
		print("couldn't mount %s: %r", cb->buf);
		cclose(c);
		nexterror();
	}
	poperror();

	mountpoint = mp;
}

static void
ctlreq(int mntid, void *a, int32_t n)
{
	Proc *up = externup();

	if (mountpoint == nil) {
		error(Eufsinvalidmp);
	}

	Cmdbuf* cb = parsecmd(a, n);
	if (waserror()) {
		print("couldn't parse command: %s: %r", a);
		free(cb);
		nexterror();
	}
	poperror();

	Cmdtab *ct = lookupcmd(cb, mountcmds, nelem(mountcmds));

	// TODO HARVEY handle errors here
	switch (ct->index) {
	case CMunmount:
		unmountufs();
		break;
	case CMtestmount:
		testmount();
		break;
	}
	free(cb);
}

static int32_t
ufswrite(Chan *c, void *a, int32_t n, int64_t offset)
{
	int qid = c->qid.path & ((1 << Qidshift)-1);
	int mntid = c->qid.path >> Qidshift;

	if(c->qid.type == QTDIR) {
		error(Eisdir);
	}

	switch (qid) {
	case Qmount:
		mount((char*)a, n);
		break;

	case Qmountctl:
		ctlreq(mntid, (char*)a, n);
		break;

	default:
		error(Eperm);
	}

	return n;
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
