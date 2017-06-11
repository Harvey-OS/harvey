/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */


typedef	struct Chan	Chan;


struct mount {
	struct ufsmount*	mnt_data;
	Chan*			chan;
};

/* Harvey equivalent to FreeBSD vnode, but not exactly the same.  Acts as a
 * wrapper for the inode and any associated data.  This is not intended to be
 * support multiple filesystems and should probably be renamed after it works.
 */
struct vnode {
	struct inode* v_data;
	//struct mount* v_mount;
};
