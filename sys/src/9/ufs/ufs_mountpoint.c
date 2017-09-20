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

#include <ufs/ufsdat.h>
#include <ufs/freebsd_util.h>
#include "ufs_harvey.h"
#include "ufs_ext.h"

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

MountPoint*
newufsmount(Chan *c, int id)
{
	MountPoint *mp = mallocz(sizeof(MountPoint), 1);
	mp->chan = c;
	mp->free_vnodes = nil;
	mp->id = id;
	return mp;
}

void
releaseufsmount(MountPoint *mp)
{
	// No need to unlock later, since we're freeing mp
	qlock(&mp->vnodes_lock);

	// TODO HARVEY What if there are referenced vnodes?
	// Ron's suggestion: you'd probably have a ref in the mountpoint and
	// sleep on it until it went to zero maybe? Not sure.

	vnode *vntofree = nil;
	vnode *vn = mp->vnodes;
	while (vn) {
		vntofree = vn;
		vn = vn->next;
		free(vntofree);
	}

	vn = mp->free_vnodes;
	while (vn) {
		vntofree = vn;
		vn = vn->next;
		free(vntofree);
	}

	free(mp);
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

void
releasevnode(MountPoint *mp, vnode *vn)
{
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
