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
#include <ffs/fs.h>
#include "ufs_ext.h"
#include <ufs/ufsmount.h>

#include "ufs_harvey.h"

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

int
writesuperblock(MountPoint *mp, char *buf, int buflen)
{
	qlock(&mp->mnt_lock);

	Fs *fs = mp->mnt_data->um_fs;
	int i = 0;

	i += snprint(buf + i, buflen - i, "fs_sblkno: %d\n", fs->fs_sblkno);
	i += snprint(buf + i, buflen - i, "fs_cblkno: %d\n", fs->fs_cblkno);
	i += snprint(buf + i, buflen - i, "fs_iblkno: %d\n", fs->fs_iblkno);
	i += snprint(buf + i, buflen - i, "fs_dblkno: %d\n", fs->fs_dblkno);
	i += snprint(buf + i, buflen - i, "fs_ncg: %u\n", fs->fs_ncg);
	i += snprint(buf + i, buflen - i, "fs_bsize: %x\n", fs->fs_bsize);
	i += snprint(buf + i, buflen - i, "fs_fsize: %x\n", fs->fs_fsize);
	i += snprint(buf + i, buflen - i, "fs_frag: %d\n", fs->fs_frag);
	i += snprint(buf + i, buflen - i, "fs_minfree: %d\n", fs->fs_minfree);
	i += snprint(buf + i, buflen - i, "fs_bmask: %d\n", fs->fs_bmask);
	i += snprint(buf + i, buflen - i, "fs_fmask: %d\n", fs->fs_fmask);
	i += snprint(buf + i, buflen - i, "fs_bshift: %d\n", fs->fs_bshift);
	i += snprint(buf + i, buflen - i, "fs_fshift: %d\n", fs->fs_fshift);
	i += snprint(buf + i, buflen - i, "fs_maxcontig: %d\n", fs->fs_maxcontig);
	i += snprint(buf + i, buflen - i, "fs_maxbpg: %d\n", fs->fs_maxbpg);
	i += snprint(buf + i, buflen - i, "fs_fragshift: %d\n", fs->fs_fragshift);
	i += snprint(buf + i, buflen - i, "fs_fsbtodb: %d\n", fs->fs_fsbtodb);
	i += snprint(buf + i, buflen - i, "fs_sbsize: %d\n", fs->fs_sbsize);
	i += snprint(buf + i, buflen - i, "fs_nindir: %d\n", fs->fs_nindir);
	i += snprint(buf + i, buflen - i, "fs_inopb: %u\n", fs->fs_inopb);
	i += snprint(buf + i, buflen - i, "fs_optim: %d\n", fs->fs_optim);
	i += snprint(buf + i, buflen - i, "fs_id: [%d, %d]\n", fs->fs_id[0], fs->fs_id[1]);
	i += snprint(buf + i, buflen - i, "fs_cssize: %d\n", fs->fs_cssize);
	i += snprint(buf + i, buflen - i, "fs_cgsize: %d\n", fs->fs_cgsize);
	i += snprint(buf + i, buflen - i, "fs_old_cpg: %d\n", fs->fs_old_cpg);
	i += snprint(buf + i, buflen - i, "fs_ipg: %u\n", fs->fs_ipg);
	i += snprint(buf + i, buflen - i, "fs_fpg: %d\n", fs->fs_fpg);
	i += snprint(buf + i, buflen - i, "fs_fmod: %hhd\n", fs->fs_fmod);
	i += snprint(buf + i, buflen - i, "fs_clean: %hhd\n", fs->fs_clean);
	i += snprint(buf + i, buflen - i, "fs_ronly: %hhd\n", fs->fs_ronly);
	i += snprint(buf + i, buflen - i, "fs_old_flags: %hhd\n", fs->fs_old_flags);
	i += snprint(buf + i, buflen - i, "fs_fsmnt: %s\n", (char*)fs->fs_fsmnt);
	i += snprint(buf + i, buflen - i, "fs_volname: %s\n", (char*)fs->fs_volname);
	i += snprint(buf + i, buflen - i, "fs_swuid: %llu\n", fs->fs_swuid);
	i += snprint(buf + i, buflen - i, "fs_pad: %d\n", fs->fs_pad);
	i += snprint(buf + i, buflen - i, "fs_cgrotor: %d\n", fs->fs_cgrotor);
	//i += snprint(buf + i, buflen - i, "fs_contigdirs: %d\n", fs->fs_contigdirs);
	//i += snprint(buf + i, buflen - i, "fs_csp: %d\n", fs->fs_csp);
	//i += snprint(buf + i, buflen - i, "fs_maxcluster: %d\n", fs->fs_maxcluster);
	//i += snprint(buf + i, buflen - i, "fs_active: %d\n", fs->fs_active);
	i += snprint(buf + i, buflen - i, "fs_maxbsize: %d\n", fs->fs_maxbsize);
	i += snprint(buf + i, buflen - i, "fs_unrefs: %lld\n", fs->fs_unrefs);
	i += snprint(buf + i, buflen - i, "fs_metaspace: %lld\n", fs->fs_metaspace);
	i += snprint(buf + i, buflen - i, "fs_sblockloc: %lld\n", fs->fs_sblockloc);
	//i += snprint(buf + i, buflen - i, "fs_cstotal: %d\n", fs->fs_cstotal);
	i += snprint(buf + i, buflen - i, "fs_time: %lld\n", fs->fs_time);
	i += snprint(buf + i, buflen - i, "fs_size: %lld\n", fs->fs_size);
	i += snprint(buf + i, buflen - i, "fs_dsize: %lld\n", fs->fs_dsize);
	i += snprint(buf + i, buflen - i, "fs_csaddr: %lld\n", fs->fs_csaddr);
	i += snprint(buf + i, buflen - i, "fs_pendingblocks: %lld\n", fs->fs_pendingblocks);
	i += snprint(buf + i, buflen - i, "fs_pendinginodes: %u\n", fs->fs_pendinginodes);
	//i += snprint(buf + i, buflen - i, "fs_snapinum: %d\n", fs->fs_snapinum);
	i += snprint(buf + i, buflen - i, "fs_avgfilesize: %u\n", fs->fs_avgfilesize);
	i += snprint(buf + i, buflen - i, "fs_avgfpdir: %u\n", fs->fs_avgfpdir);
	i += snprint(buf + i, buflen - i, "fs_save_cgsize: %d\n", fs->fs_save_cgsize);
	i += snprint(buf + i, buflen - i, "fs_mtime: %lld\n", fs->fs_mtime);
	i += snprint(buf + i, buflen - i, "fs_sujfree: %d\n", fs->fs_sujfree);
	i += snprint(buf + i, buflen - i, "fs_flags: %d\n", fs->fs_flags);
	i += snprint(buf + i, buflen - i, "fs_contigsumsize: %d\n", fs->fs_contigsumsize);
	i += snprint(buf + i, buflen - i, "fs_maxsymlinklen: %d\n", fs->fs_maxsymlinklen);
	i += snprint(buf + i, buflen - i, "fs_maxfilesize: %llu\n", fs->fs_maxfilesize);
	i += snprint(buf + i, buflen - i, "fs_qbmask: %lld\n", fs->fs_qbmask);
	i += snprint(buf + i, buflen - i, "fs_qfmask: %lld\n", fs->fs_qfmask);
	i += snprint(buf + i, buflen - i, "fs_state: %d\n", fs->fs_state);
	i += snprint(buf + i, buflen - i, "fs_magic: %d\n", fs->fs_magic);


	qunlock(&mp->mnt_lock);

	return i;
}

