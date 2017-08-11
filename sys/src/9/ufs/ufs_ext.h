/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// Functions to be exposed outside the UFS code (e.g. to devufs)

typedef struct Chan Chan;
typedef struct MountPoint MountPoint;
typedef struct ufsmount ufsmount;
typedef struct vnode vnode;


/*
 * filesystem statistics
 */
typedef struct statfs {
	uint64_t f_iosize;		/* optimal transfer block size */
} StatFs;


/* Wrapper for a UFS mount.  Should support reading from both kernel and user
 * space (eventually)
 */
typedef struct MountPoint {
	ufsmount	*mnt_data;
	Chan		*chan;
	int		id;
	StatFs		mnt_stat;		/* cache of filesystem stats */
	int		mnt_maxsymlinklen;	/* max size of short symlink */

	uint64_t	mnt_flag;	/* (i) flags shared with user */
	QLock		mnt_lock;	/* (mnt_mtx) structure lock */

	QLock		vnodes_lock;	/* lock on vnodes in use & freelist */
	vnode		*vnodes;	/* vnode cache */
	vnode		*free_vnodes;	/* vnode freelist */
} MountPoint;



MountPoint *newufsmount(Chan *c, int id);
void releaseufsmount(MountPoint *mp);
int countvnodes(vnode *vn);

int ffs_mount(MountPoint *mp);
int ffs_unmount(MountPoint *mp, int mntflags);

// TODO HARVEY Are these here temporarily?
int ufs_root(MountPoint *mp, int flags, vnode **vpp);
int ufs_lookup(MountPoint *mp);
