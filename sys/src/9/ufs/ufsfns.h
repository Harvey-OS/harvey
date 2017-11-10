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


Vtype ifmt_to_vtype(uint16_t imode);

MountPoint *newufsmount(Chan *c, int id);
void releaseufsmount(MountPoint *mp);
int countvnodes(vnode *vn);
int writesuperblock(char *buf, int buflen, MountPoint *mp);
int writeinode(char *buf, int buflen, vnode *vn);
int writeinodedata(char *buf, int buflen, vnode *vn);

int ffs_init();
int ffs_uninit();

int ffs_mount(MountPoint *mp);
int ffs_unmount(MountPoint *mp, int mntflags);

int ufs_root(MountPoint *mp, int flags, vnode **vpp);
int ufs_lookup(MountPoint *mp);

int lookuppath(MountPoint *mp, char *path, vnode **vn);
vnode *ufs_open_ino(MountPoint *mp, ino_t ino);

int findexistingvnode(MountPoint *mp, ino_t ino, vnode **vpp);
int getnewvnode(MountPoint *mp, vnode **vpp);
void releaseufsvnode(vnode *vn);

void assert_vop_locked(vnode *vp, const char *str);
void assert_vop_elocked(vnode *vp, const char *str);

int32_t bread(MountPoint *mp, ufs2_daddr_t blockno, size_t size, void **buf);

vnode* findvnode(MountPoint *mp, ino_t ino);
vnode* getfreevnode(MountPoint *mp);
void releasevnode(vnode *vn);
