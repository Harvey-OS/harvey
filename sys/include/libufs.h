/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */


struct Chan;
struct mount;
struct vnode;


struct mount* newufsmount(struct Chan* c);
struct vnode* newufsvnode();

void releaseufsmount(struct mount* mp);
void releaseufsvnode(struct vnode* vn);

int ffs_mount(struct mount *mp);
