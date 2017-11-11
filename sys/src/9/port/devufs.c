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

#include "ufs/ufsdat.h"
#include "ufs/libufsdat.h"
#include "ufs/ufsfns.h"

enum {
	Qdir = 0,		// #U
	Qufsdir,		// #U/ufs
	Qmount,			// #U/ufs/mount
	Qmountdir,		// #U/ufs/x (x embedded in qid)
	Qmountctl,		// #U/ufs/x/ctl
	Qmountstats,		// #U/ufs/x/stats
	Qsuperblock,		// #U/ufs/x/superblock
	Qinodesdir,		// #U/ufs/x/inodes (directory of all inodes)
	Qinodedir,		// #U/ufs/x/inodes/y (directory of single inode)
	Qinode,			// #U/ufs/x/inodes/y/inode (inode metadata)
	Qinodedata,		// #U/ufs/x/inodes/y/data (inode data)
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
	{"ctl",		{Qmountctl},		0,	0666},
	{"stats",	{Qmountstats},		0,	0444},
	{"superblock",	{Qsuperblock},		0,	0444},
	{"inodes",	{Qinodesdir, 0, QTDIR},	0,	DMDIR|0111},
};

static Dirtab inodedir[] =
{
	{"inode",	{Qinode},		0,	0444},
	{"data",	{Qinodedata},		0,	0444},
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

	case Qmount:
	case Qufsdir:
		// list #U/ufs
		if (s < nelem(ufsdir)) {
			// Populate with ufsdir table
			dir = &ufsdir[s];
			mkqid(&q, dir->qid.path, 0, dir->qid.type);
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

	case Qmountctl:
	case Qmountstats:
	case Qsuperblock:
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
		mkqid(&q, dir->qid.path | (qidmntid << Qidshift), 0, dir->qid.type);
		devdir(c, q, dir->name, dir->length, eve, dir->perm, dp);
		return 1;

	case Qinodesdir:
	{
		// Generate #U/ufs/x/inodes/...
		// We don't allow listing of the inodes folder because the
		// number of files would be enormous and of limited use.
		// Instead, we'll look at the requested path, and parse
		// it to ensure it's a valid number.  If it is, then we'll
		// assume that the inode number exists.
		// TODO Check if the inode number is valid
		// TODO Consider just listing all allocated inode numbers here

		// Validate
		char *filename = d;
		if (strtoll(filename, nil, 10) < 2) {
			error("invalid inode");
		}

		// Ensure we pass through the mount ID
		mkqid(&q, Qinodedir | (qidmntid << Qidshift), 0, QTDIR);
		devdir(c, q, filename, 0, eve, 0444, dp);
		return 1;
	}

	case Qinode:
	case Qinodedata:
	case Qinodedir:
		if (s >= nelem(inodedir)) {
			return -1;
		}
		dir = &inodedir[s];
		mkqid(&q, dir->qid.path | (qidmntid << Qidshift), 0, dir->qid.type);
		devdir(c, q, dir->name, dir->length, eve, dir->perm, dp);
		return 1;

	default:
		return -1;
	}
}

static void
ufsinit()
{
	ffs_init();
}

static void
ufsshutdown()
{
	ffs_uninit();
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
	int qid = c->qid.path & ((1 << Qidshift)-1);

	c = devopen(c, omode, nil, 0, ufsgen);

	switch (qid) {
	case Qinode:
	case Qinodedata:
	{
		// We need to get the inode from the last element of the path
		// We couldn't encode it anywhere, because it'll eventually
		// be a 64-bit value.
		char *str = malloc(strlen(c->path->s) + 1);
		strcpy(str, c->path->s);

		char *inoend = strrchr(str, '/');
		if (!inoend) {
			free(str);
			error("invalid inode");
		}
		*inoend = '\0';

		char *inostart = strrchr(str, '/');
		if (!inostart) {
			free(str);
			error("invalid inode");
		}
		inostart++;

		ino_t ino = strtoll(inostart, nil, 10);
		free(str);
		if (ino == 0) {
			error("invalid inode");
		}

		vnode *vn = ufs_open_ino(mountpoint, ino);
		c->aux = vn;
		break;
	}
	default:
		break;
	}

	return c;
}

static void
ufsclose(Chan* c)
{
	int qid = c->qid.path & ((1 << Qidshift)-1);
	switch (qid) {
	case Qinode:
	case Qinodedata:
	{
		vnode *vn = (vnode*)c->aux;
		if (!vn) {
			error("no vnode to close");
		}
		releasevnode(vn);		
		break;
	}
	default:
		break;
	}
}

static int
dumpstats(void *a, int32_t n, int64_t offset, MountPoint *mp)
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

static int
dumpsuperblock(void *a, int32_t n, int64_t offset, MountPoint *mp)
{
	char *buf = malloc(READSTR);

	writesuperblock(buf, READSTR, mp);
	n = readstr(offset, a, n, buf);

	free(buf);

	return n;
}

static int
dumpinode(void *a, int32_t n, int64_t offset, vnode *vn)
{
	char *buf = malloc(READSTR);

	writeinode(buf, READSTR, vn);
	n = readstr(offset, a, n, buf);

	free(buf);

	return n;
}

static int
dumpinodedata(void *a, int32_t n, int64_t offset, vnode *vn)
{
	char *buf = malloc(READSTR);

	writeinodedata(buf, READSTR, vn);
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

	int qid = (int)c->qid.path;

	switch (qid) {
	case Qmountstats:
		n = dumpstats(a, n, offset, mountpoint);
		break;
	case Qsuperblock:
		n = dumpsuperblock(a, n, offset, mountpoint);
		break;
	case Qinode:
	{
		vnode *vn = (vnode*)c->aux;
		n = dumpinode(a, n, offset, vn);
		break;
	}

	case Qinodedata:
	{
		vnode *vn = (vnode*)c->aux;
		n = dumpinodedata(a, n, offset, vn);
		break;
	}

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

	// TODO UFS caches lookups.  We could do that in devufs.
	vnode *vn;
	char *path = "/";
	int rcode = lookuppath(mountpoint, path, &vn);
	if (rcode != 0) {
		print("couldn't lookup path: %s: %d", path, rcode);
	}
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

	if (c->qid.type == QTDIR) {
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
	.init = ufsinit,
	.shutdown = ufsshutdown,
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
