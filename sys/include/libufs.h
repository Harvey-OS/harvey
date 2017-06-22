/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */


typedef struct Chan Chan;
typedef struct ufsmount ufsmount;
typedef struct vnode vnode;
typedef struct thread thread;


/* Wrapper for a UFS mount.  Should support reading from both kernel and user
 * space (eventually)
 */
typedef struct MountPoint {
	ufsmount	*mnt_data;
	Chan		*chan;
	int32_t		(*read)(struct MountPoint*, void*, int32_t, int64_t);
} MountPoint;


MountPoint *newufsmount(
	Chan *c,
	int32_t (*read)(MountPoint*, void*, int32_t, int64_t));

vnode* newufsvnode();

void releaseufsmount(MountPoint *mp);
void releaseufsvnode(vnode *vn);

int ffs_mount(MountPoint *mp);
