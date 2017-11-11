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
#include "port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "ufsdat.h"
#include "ufs/libufsdat.h"
#include "ufs/quota.h"
#include "ufs/inode.h"


const static int VnodeFreelistBatchSize = 1000;

static vnode*
alloc_freelist()
{
	vnode *head = nil;
	vnode *curr = nil;

	for (int i = 0; i < VnodeFreelistBatchSize; i++) {
		vnode *vn = mallocz(sizeof(vnode), 1);
		if (vn == nil) {
		       break;
		}

		if (head == nil) {
			head = vn;
		}

		vn->prev = curr;
		if (curr != nil) {
			curr->next = vn;
		}

		curr = vn;
	}
	return head;
}

vnode*
findvnode(MountPoint *mp, ino_t ino)
{
	vnode *vn = nil;

	qlock(&mp->vnodes_lock);

	// Check for existing vnode
	for (vn = mp->vnodes; vn != nil; vn = vn->next) {
		if (vn->data->i_number == ino) {
			break;
		}
	}

	if (vn != nil) {
		incref(&vn->ref);
	}

	qunlock(&mp->vnodes_lock);

	return vn;
}

vnode*
getfreevnode(MountPoint *mp)
{
	qlock(&mp->vnodes_lock);

	if (mp->free_vnodes == nil) {
		mp->free_vnodes = alloc_freelist();
		if (mp->free_vnodes == nil) {
			qunlock(&mp->vnodes_lock);
			return nil;
		}
	}

	vnode *vn = mp->free_vnodes;

	// Move from freelist to vnodes
	mp->free_vnodes = vn->next;
	mp->free_vnodes->prev = nil;

	// Clear out
	memset(vn, 0, sizeof(vnode));
	vn->mount = mp;

	if (mp->vnodes != nil) {
		mp->vnodes->prev = vn;
	}
	vn->next = mp->vnodes;
	vn->prev = nil;

	incref(&vn->ref);

	mp->vnodes = vn;

	qunlock(&mp->vnodes_lock);

	return vn;
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
releasevnode(vnode *vn)
{
	MountPoint *mp = vn->mount;
	qlock(&mp->vnodes_lock);

	if (decref(&vn->ref) == 0) {
		// Remove vnode from vnodes list
		if (vn->prev != nil) {
		       vn->prev->next = vn->next;
		}
		if (vn->next != nil) {
			vn->next->prev = vn->prev;
		}
		if (mp->vnodes == vn) {
			mp->vnodes = vn->next;
		}

		// Return to free list
		vn->next = mp->free_vnodes;
		mp->free_vnodes->prev = vn;
		mp->free_vnodes = vn;
		vn->prev = nil;
	}

	qunlock(&mp->vnodes_lock);
}

int
countvnodes(vnode* vn)
{
	int n = 0;
	for (; vn != nil; vn = vn->next, n++)
		;
	return n;
}

static void
vfs_badlock(const char *msg, const char *str, vnode *vp)
{
	print("*** %s: %p %s\n", str, (void *)vp, msg);
}

void
assert_vop_locked(vnode *vp, const char *str)
{
	if (vp == nil) {
		print("assert_vop_locked: vnode is nil (checking %s)\n", str);
	} else if (vp->vnlock.readers == 0 && vp->vnlock.writer == 0) {
		vfs_badlock("is not locked but should be", str, vp);
	}
}

void
assert_vop_elocked(vnode *vp, const char *str)
{
	if (vp == nil) {
		print("assert_vop_elocked: vnode is nil (checking %s)\n", str);
	} else if (vp->vnlock.writer == 0) {
		vfs_badlock("is not exclusive locked but should be", str, vp);
	}
}
