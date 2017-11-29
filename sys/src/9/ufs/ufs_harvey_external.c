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
#include "ufs/ufsfns.h"
#include "ufs/fs.h"
#include "ufs/ufsmount.h"
#include "ufs/quota.h"
#include "ufs/dinode.h"
#include "ufs/inode.h"
#include "ufs/dir.h"
#include "ufs_extern.h"
#include "ffs_extern.h"


static int
countvnodes(vnode* vn)
{
	check_vnodes_locked(vn->mount);
	
	int n = 0;
	for (; vn != nil; vn = vn->next, n++)
		;
	return n;
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

	cclose(mp->chan);
	free(mp);
}

int
writestats(char *buf, int buflen, MountPoint *mp)
{
	qlock(&mp->vnodes_lock);
	int numinuse = countvnodes(mp->vnodes);
	int numfree = countvnodes(mp->free_vnodes);
	qunlock(&mp->vnodes_lock);

	int i = 0;
	i += snprint(buf + i, READSTR - i, "Mountpoint:        %d\n", mp->id);
	i += snprint(buf + i, READSTR - i, "Num vnodes in use: %d\n", numinuse);
	i += snprint(buf + i, READSTR - i, "Num vnodes free:   %d\n", numfree);

	return i;
}

int
writesuperblock(char *buf, int buflen, MountPoint *mp)
{
	qlock(&mp->mnt_lock);

	Fs *fs = mp->mnt_data->um_fs;
	int i = 0;

	i += snprint(buf + i, buflen - i, "fs_sblkno\t%d\n", fs->fs_sblkno);
	i += snprint(buf + i, buflen - i, "fs_cblkno\t%d\n", fs->fs_cblkno);
	i += snprint(buf + i, buflen - i, "fs_iblkno\t%d\n", fs->fs_iblkno);
	i += snprint(buf + i, buflen - i, "fs_dblkno\t%d\n", fs->fs_dblkno);
	i += snprint(buf + i, buflen - i, "fs_ncg\t%u\n", fs->fs_ncg);
	i += snprint(buf + i, buflen - i, "fs_bsize\t%x\n", fs->fs_bsize);
	i += snprint(buf + i, buflen - i, "fs_fsize\t%x\n", fs->fs_fsize);
	i += snprint(buf + i, buflen - i, "fs_frag\t%d\n", fs->fs_frag);
	i += snprint(buf + i, buflen - i, "fs_minfree\t%d\n", fs->fs_minfree);
	i += snprint(buf + i, buflen - i, "fs_bmask\t%d\n", fs->fs_bmask);
	i += snprint(buf + i, buflen - i, "fs_fmask\t%d\n", fs->fs_fmask);
	i += snprint(buf + i, buflen - i, "fs_bshift\t%d\n", fs->fs_bshift);
	i += snprint(buf + i, buflen - i, "fs_fshift\t%d\n", fs->fs_fshift);
	i += snprint(buf + i, buflen - i, "fs_maxcontig\t%d\n", fs->fs_maxcontig);
	i += snprint(buf + i, buflen - i, "fs_maxbpg\t%d\n", fs->fs_maxbpg);
	i += snprint(buf + i, buflen - i, "fs_fragshift\t%d\n", fs->fs_fragshift);
	i += snprint(buf + i, buflen - i, "fs_fsbtodb\t%d\n", fs->fs_fsbtodb);
	i += snprint(buf + i, buflen - i, "fs_sbsize\t%d\n", fs->fs_sbsize);
	i += snprint(buf + i, buflen - i, "fs_nindir\t%d\n", fs->fs_nindir);
	i += snprint(buf + i, buflen - i, "fs_inopb\t%u\n", fs->fs_inopb);
	i += snprint(buf + i, buflen - i, "fs_optim\t%d\n", fs->fs_optim);
	i += snprint(buf + i, buflen - i, "fs_id\t[%d, %d]\n", fs->fs_id[0], fs->fs_id[1]);
	i += snprint(buf + i, buflen - i, "fs_cssize\t%d\n", fs->fs_cssize);
	i += snprint(buf + i, buflen - i, "fs_cgsize\t%d\n", fs->fs_cgsize);
	i += snprint(buf + i, buflen - i, "fs_old_cpg\t%d\n", fs->fs_old_cpg);
	i += snprint(buf + i, buflen - i, "fs_ipg\t%u\n", fs->fs_ipg);
	i += snprint(buf + i, buflen - i, "fs_fpg\t%d\n", fs->fs_fpg);
	i += snprint(buf + i, buflen - i, "fs_fmod\t%hhd\n", fs->fs_fmod);
	i += snprint(buf + i, buflen - i, "fs_clean\t%hhd\n", fs->fs_clean);
	i += snprint(buf + i, buflen - i, "fs_ronly\t%hhd\n", fs->fs_ronly);
	i += snprint(buf + i, buflen - i, "fs_old_flags\t%hhd\n", fs->fs_old_flags);
	i += snprint(buf + i, buflen - i, "fs_fsmnt\t%s\n", (char*)fs->fs_fsmnt);
	i += snprint(buf + i, buflen - i, "fs_volname\t%s\n", (char*)fs->fs_volname);
	i += snprint(buf + i, buflen - i, "fs_swuid\t%llu\n", fs->fs_swuid);
	i += snprint(buf + i, buflen - i, "fs_pad\t%d\n", fs->fs_pad);
	i += snprint(buf + i, buflen - i, "fs_cgrotor\t%d\n", fs->fs_cgrotor);
	//i += snprint(buf + i, buflen - i, "fs_contigdirs\t%d\n", fs->fs_contigdirs);
	//i += snprint(buf + i, buflen - i, "fs_csp\t%d\n", fs->fs_csp);
	//i += snprint(buf + i, buflen - i, "fs_maxcluster\t%d\n", fs->fs_maxcluster);
	//i += snprint(buf + i, buflen - i, "fs_active\t%d\n", fs->fs_active);
	i += snprint(buf + i, buflen - i, "fs_maxbsize\t%d\n", fs->fs_maxbsize);
	i += snprint(buf + i, buflen - i, "fs_unrefs\t%lld\n", fs->fs_unrefs);
	i += snprint(buf + i, buflen - i, "fs_metaspace\t%lld\n", fs->fs_metaspace);
	i += snprint(buf + i, buflen - i, "fs_sblockloc\t%lld\n", fs->fs_sblockloc);
	//i += snprint(buf + i, buflen - i, "fs_cstotal\t%d\n", fs->fs_cstotal);
	i += snprint(buf + i, buflen - i, "fs_time\t%lld\n", fs->fs_time);
	i += snprint(buf + i, buflen - i, "fs_size\t%lld\n", fs->fs_size);
	i += snprint(buf + i, buflen - i, "fs_dsize\t%lld\n", fs->fs_dsize);
	i += snprint(buf + i, buflen - i, "fs_csaddr\t%lld\n", fs->fs_csaddr);
	i += snprint(buf + i, buflen - i, "fs_pendingblocks\t%lld\n", fs->fs_pendingblocks);
	i += snprint(buf + i, buflen - i, "fs_pendinginodes\t%u\n", fs->fs_pendinginodes);
	//i += snprint(buf + i, buflen - i, "fs_snapinum\t%d\n", fs->fs_snapinum);
	i += snprint(buf + i, buflen - i, "fs_avgfilesize\t%u\n", fs->fs_avgfilesize);
	i += snprint(buf + i, buflen - i, "fs_avgfpdir\t%u\n", fs->fs_avgfpdir);
	i += snprint(buf + i, buflen - i, "fs_save_cgsize\t%d\n", fs->fs_save_cgsize);
	i += snprint(buf + i, buflen - i, "fs_mtime\t%lld\n", fs->fs_mtime);
	i += snprint(buf + i, buflen - i, "fs_sujfree\t%d\n", fs->fs_sujfree);
	i += snprint(buf + i, buflen - i, "fs_flags\t%d\n", fs->fs_flags);
	i += snprint(buf + i, buflen - i, "fs_contigsumsize\t%d\n", fs->fs_contigsumsize);
	i += snprint(buf + i, buflen - i, "fs_maxsymlinklen\t%d\n", fs->fs_maxsymlinklen);
	i += snprint(buf + i, buflen - i, "fs_maxfilesize\t%llu\n", fs->fs_maxfilesize);
	i += snprint(buf + i, buflen - i, "fs_qbmask\t%lld\n", fs->fs_qbmask);
	i += snprint(buf + i, buflen - i, "fs_qfmask\t%lld\n", fs->fs_qfmask);
	i += snprint(buf + i, buflen - i, "fs_state\t%d\n", fs->fs_state);
	i += snprint(buf + i, buflen - i, "fs_magic\t%d\n", fs->fs_magic);

	qunlock(&mp->mnt_lock);

	return i;
}

int
writeinode(char *buf, int buflen, vnode *vn)
{
	inode *ip = vn->data;

	int i = 0;
	i += snprint(buf + i, buflen - i, "i_number\t%llu\n", (uint64_t)ip->i_number);
	i += snprint(buf + i, buflen - i, "i_flag\t%u\n", ip->i_flag);
	i += snprint(buf + i, buflen - i, "i_size\t%llu\n", ip->i_size);
	i += snprint(buf + i, buflen - i, "i_gen\t%llu\n", ip->i_gen);
	i += snprint(buf + i, buflen - i, "i_flags\t%u\n", ip->i_flags);
	i += snprint(buf + i, buflen - i, "i_uid\t%u\n", ip->i_uid);
	i += snprint(buf + i, buflen - i, "i_gid\t%u\n", ip->i_gid);
	i += snprint(buf + i, buflen - i, "i_mode\t%hhu\n", ip->i_mode);
	i += snprint(buf + i, buflen - i, "i_nlink\t%hhd\n", ip->i_nlink);

	ufs2_dinode *din = ip->din2;
	i += snprint(buf + i, buflen - i, "di_mode\t%hhu\n", din->di_mode);
	i += snprint(buf + i, buflen - i, "di_nlink\t%hhd\n", din->di_nlink);
	i += snprint(buf + i, buflen - i, "di_uid\t%u\n", din->di_uid);
	i += snprint(buf + i, buflen - i, "di_gid\t%u\n", din->di_gid);
	i += snprint(buf + i, buflen - i, "di_blksize\t%u\n", din->di_blksize);
	i += snprint(buf + i, buflen - i, "di_size\t%llu\n", din->di_size);
	i += snprint(buf + i, buflen - i, "di_blocks\t%llu\n", din->di_blocks);
	i += snprint(buf + i, buflen - i, "di_atime\t%lld\n", din->di_atime);
	i += snprint(buf + i, buflen - i, "di_mtime\t%lld\n", din->di_mtime);
	i += snprint(buf + i, buflen - i, "di_ctime\t%lld\n", din->di_ctime);
	i += snprint(buf + i, buflen - i, "di_birthtime\t%lld\n", din->di_birthtime);
	i += snprint(buf + i, buflen - i, "di_mtimensec\t%d\n", din->di_mtimensec);
	i += snprint(buf + i, buflen - i, "di_atimensec\t%d\n", din->di_atimensec);
	i += snprint(buf + i, buflen - i, "di_ctimensec\t%d\n", din->di_ctimensec);
	i += snprint(buf + i, buflen - i, "di_birthnsec\t%d\n", din->di_birthnsec);
	i += snprint(buf + i, buflen - i, "di_gen\t%u\n", din->di_gen);
	i += snprint(buf + i, buflen - i, "di_kernflags\t%u\n", din->di_kernflags);
	i += snprint(buf + i, buflen - i, "di_flags\t%u\n", din->di_flags);
	i += snprint(buf + i, buflen - i, "di_extsize\t%u\n", din->di_extsize);
	for (int j = 0; j < nelem(din->di_extb); j++) {
		i += snprint(buf + i, buflen - i, "di_extb[%d]\t%u\n", j, din->di_extb[j]);
	}
	for (int j = 0; j < nelem(din->di_db); j++) {
		i += snprint(buf + i, buflen - i, "di_db[%d]\t%u\n", j, din->di_db[j]);
	}
	for (int j = 0; j < nelem(din->di_ib); j++) {
		i += snprint(buf + i, buflen - i, "di_ib[%d]\t%u\n", j, din->di_ib[j]);
	}
	i += snprint(buf + i, buflen - i, "di_modrev\t%llu\n", din->di_modrev);
	i += snprint(buf + i, buflen - i, "di_freelink\t%u\n", din->di_freelink);

	return i;
}

int
writeinodedata(char *buf, int32_t n, int64_t offset, vnode *vn)
{
	Proc *up = externup();

	if (vn->type == VREG) {
		uint64_t size = vn->data->i_size;
		if (offset >= size) {
			return 0;
		}
		if (offset + n > size) {
			n = size - offset;
		}

		Uio* uio = newuio(n);
		if (waserror()) {
			releaseuio(uio);
			nexterror();
		}

		uio->resid = vn->data->i_size;
		uio->offset = offset;

		int rcode = ffs_read(vn, uio);
		if (rcode) {
			error("error reading file");
		}

		packuio(uio);


		uio->dest = bl2mem((uint8_t*)buf, uio->dest, n);

		poperror();
		releaseuio(uio);
		return n;
	
	} else if (vn->type == VDIR) {
		Uio* uio = newuio(n);
		if (waserror()) {
			releaseuio(uio);
			nexterror();
		}

		// We don't set uio->offset here - that's accounted for when
		// writing the directory strings, since that's where the offset
		// is really relative to.
		uio->resid = vn->data->i_size;

		int rcode = ufs_readdir(vn, uio);
		if (rcode) {
			error("error reading directory");
		}

		packuio(uio);

		// Iterate over all files
		int i = 0, offseti = 0;
		int64_t diroffset = 0;
		int64_t endoffset = uio->offset - uio->resid;
		while (uio->offset >= 0 && diroffset < endoffset) {
			Dirent* dir = (Dirent*)(uio->dest->rp + diroffset);

			int len = snprint(buf + i, n - i, "%s\t%llu\n", dir->name, (uint64_t)dir->fileno);

			// Only progress if we've passed the offset.  This will
			// continually overwrite until that point.
			if (offseti >= offset) {
				i += len;
			}
			offseti += len;
			diroffset += dir->reclen;
		}

		poperror();
		releaseuio(uio);
		return i;

	} else {
		return snprint(buf, n, "unsupported inode type (%d)\n", vn->type);
	}
}

vnode *
ufs_open_ino(MountPoint *mp, ino_t ino) {
	vnode *vn;
	int rcode = ffs_vget(mp, ino, LK_SHARED, &vn);
	if (rcode) {
		error("cannot get file to open");
	}
	return vn;
}

int
lookuppath(MountPoint *mp, char *path, vnode **vn)
{
	// Get the root
	vnode *root = nil;
	int rcode = ufs_root(mp, LK_EXCLUSIVE, &root);
	if (rcode != 0) {
		print("couldn't get root: %d", rcode);
		return -1;
	}

	// TODO UFS caches lookups.  We could do that in devufs.
	ComponentName cname = { .cn_pnbuf = path, .cn_nameiop = LOOKUP };
	rcode = ufs_lookup_ino(root, vn, &cname, nil);
	if (rcode != 0) {
		print("couldn't lookup path: %s: %d", path, rcode);
		return -1;
	}

	return 0;
}
