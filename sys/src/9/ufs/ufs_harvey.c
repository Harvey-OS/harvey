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
#include "../../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "freebsd_util.h"
#include "ufs_harvey.h"
#include "ufs_mountpoint.h"

#include "ufs/quota.h"
#include "ufs/inode.h"


int
findexistingvnode(MountPoint *mp, ino_t ino, vnode **vpp)
{
	*vpp = findvnode(mp, ino);
	return 0;
}

/*
 * Return the next vnode from the free list.
 */
int
getnewvnode(MountPoint *mp, vnode **vpp)
{
	vnode *vp = nil;
	//struct lock_object *lo;
	//static int cyclecount;
	//int error = 0;

//alloc:
	vp = getfreevnode(mp);
	if (vp == nil) {
		return 0;
	}

	// TODO HARVEY Revisit locking of vnodes
	// TODO HARVEY Revisit counters
#if 0
	/*
	 * Locks are given the generic name "vnode" when created.
	 * Follow the historic practice of using the filesystem
	 * name when they allocated, e.g., "zfs", "ufs", "nfs, etc.
	 *
	 * Locks live in a witness group keyed on their name. Thus,
	 * when a lock is renamed, it must also move from the witness
	 * group of its old name to the witness group of its new name.
	 *
	 * The change only needs to be made when the vnode moves
	 * from one filesystem type to another. We ensure that each
	 * filesystem use a single static name pointer for its tag so
	 * that we can compare pointers rather than doing a strcmp().
	 */
	lo = &vp->v_vnlock->lock_object;
	if (lo->lo_name != tag) {
		lo->lo_name = tag;
		WITNESS_DESTROY(lo);
		WITNESS_INIT(lo, tag);
	}
	/*
	 * By default, don't allow shared locks unless filesystems opt-in.
	 */
	vp->v_vnlock->lock_object.lo_flags |= LK_NOSHARE;
	/*
	 * Finalize various vnode identity bits.
	 */
	KASSERT(vp->v_object == NULL, ("stale v_object %p", vp));
	KASSERT(vp->v_lockf == NULL, ("stale v_lockf %p", vp));
	KASSERT(vp->v_pollinfo == NULL, ("stale v_pollinfo %p", vp));
#endif // 0
	vp->type = VNON;
	vp->mount = mp;
#if 0
	v_init_counters(vp);
	//vp->v_bufobj.bo_ops = &buf_ops_bio;
#ifdef DIAGNOSTIC
	if (mp == NULL && vops != &dead_vnodeops)
		printf("NULL mp in getnewvnode(9), tag %s\n", tag);
#endif
#ifdef MAC
	mac_vnode_init(vp);
	if (mp != NULL && (mp->mnt_flag & MNT_MULTILABEL) == 0)
		mac_vnode_associate_singlelabel(mp, vp);
#endif
	if (mp != NULL) {
		vp->v_bufobj.bo_bsize = mp->mnt_stat.f_iosize;
		if ((mp->mnt_kern_flag & MNTK_NOKNOTE) != 0)
			vp->v_vflag |= VV_NOKNOTE;
	}
#endif // 0

	/*
	 * For the filesystems which do not use vfs_hash_insert(),
	 * still initialize v_hash to have vfs_hash_index() useful.
	 * E.g., nullfs uses vfs_hash_index() on the lower vnode for
	 * its own hashing.
	 */
	//vp->v_hash = (uintptr_t)vp >> vnsz2log;

	*vpp = vp;
	return (0);
}

void
releaseufsvnode(MountPoint *mp, vnode *vn)
{
	releasevnode(mp, vn);
}
