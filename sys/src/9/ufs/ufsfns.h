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
typedef struct MountPoint MountPoint;
typedef struct vnode vnode;


// Vnode
vnode*		findvnode(MountPoint *mp, ino_t ino);
vnode*		getfreevnode(MountPoint *mp);
int		getnewvnode(MountPoint *mp, vnode **vpp);
void		releasevnode(vnode *vn);
vnode*		ufs_open_ino(MountPoint *mp, ino_t ino);

// External
MountPoint*	newufsmount(Chan *c, int id);
void		releaseufsmount(MountPoint *mp);
int		lookuppath(MountPoint *mp, char *path, vnode **vn);
int		writestats(char *buf, int buflen, MountPoint *mp);
int		writesuperblock(char *buf, int buflen, MountPoint *mp);
int		writeinode(char *buf, int buflen, vnode *vn);
int		writeinodedata(char *buf, int buflen, vnode *vn);

// Misc
int		ffs_init();
int		ffs_uninit();
int		ffs_mount(MountPoint *mp);
int		ffs_unmount(MountPoint *mp, int mntflags);

// Internal functions - maybe they should be somewhere else?
void		assert_vop_locked(vnode *vp, const char *str);
void		assert_vop_elocked(vnode *vp, const char *str);
int32_t		bread(MountPoint *mp, ufs2_daddr_t blockno, size_t size, void **buf);
void		check_vnodes_locked(MountPoint *mp);
Vtype		ifmt_to_vtype(uint16_t imode);
