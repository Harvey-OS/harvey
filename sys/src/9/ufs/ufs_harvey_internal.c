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


// TODO HARVEY Pool of buffers?  Aiming for as simple as possible for now.
Buf*
newbuf(size_t size)
{
	Buf *b = smalloc(sizeof(Buf));
	b->data = smalloc(size);
	return b;
}

void
releasebuf(Buf *b)
{
	if (b->data) {
		free(b->data);
		b->data = nil;
	}
	free(b);
}

/*
 * Wrapper to enable Harvey's channel read function to be used like FreeBSD's
 * block read function.
 */
int32_t
bread(MountPoint *mp, daddr_t blockno, size_t size, Buf **buf)
{
	Buf *b = newbuf(size);

	Chan *c = mp->chan;
	int64_t offset = dbtob(blockno);
	int32_t bytesRead = c->dev->read(c, b->data, size, offset);
	if (bytesRead != size) {
		releasebuf(b);
		print("bread returned wrong size\n");
		return 1;
	}

	*buf = b;
	return 0;
}

Buf *
getblk(vnode *vp, daddr_t blkno, size_t size, int flags)
{
	Buf *b;
	MountPoint *mp = vp->mount;
	int32_t rcode = bread(mp, blkno, size, &b);
	if (rcode) {
		print("getblk - bread failed\n");
		return nil;
	}
	return b;
}
