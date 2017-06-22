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

#include "ufs_harvey.h"


MountPoint*
newufsmount(Chan *c)
{
	// TODO HARVEY - Implement caching
	MountPoint *mp = mallocz(sizeof(MountPoint), 1);
	mp->chan = c;
	return mp;
}

vnode*
newufsvnode()
{
	// TODO HARVEY - Implement caching
	vnode *vn = mallocz(sizeof(vnode), 1);
	return vn;
}

void
releaseufsmount(MountPoint *mp)
{
	// TODO HARVEY - This assumes no sharing
	free(mp);
}

void
releaseufsvnode(vnode *vn)
{
	// TODO HARVEY - This assumes no sharing
	free(vn);
}
