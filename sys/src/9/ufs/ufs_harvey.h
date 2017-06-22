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
typedef struct inode inode;


/* Wrapper for a UFS mount.  Should support reading from both kernel and user
 * space (eventually)
 */
typedef struct MountPoint {
	ufsmount	*mnt_data;
	Chan		*chan;

	uint64_t	mnt_flag;		/* (i) flags shared with user */
} MountPoint;


/* Harvey equivalent to FreeBSD vnode, but not exactly the same.  Acts as a
 * wrapper for the inode and any associated data.  This is not intended to be
 * support multiple filesystems and should probably be renamed after it works.
 */
typedef struct vnode {
	inode	*v_data;
	//MountPoint	*v_mount;
} vnode;


// Not sure we even need this - if not we can remove it later.
typedef struct thread {
} thread;


/*
 * MAXBSIZE -	Filesystems are made out of blocks of at most MAXBSIZE bytes
 *		per block.  MAXBSIZE may be made larger without effecting
 *		any existing filesystems as long as it does not exceed MAXPHYS,
 *		and may be made smaller at the risk of not being able to use
 *		filesystems which require a block size exceeding MAXBSIZE.
 */
#define MAXBSIZE	65536	/* must be power of 2 */


MountPoint *newufsmount(Chan *c);

vnode* newufsvnode();

void releaseufsmount(MountPoint *mp);
void releaseufsvnode(vnode *vn);

int ffs_mount(MountPoint *mp);
