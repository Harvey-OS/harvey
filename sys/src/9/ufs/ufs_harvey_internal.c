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
#include "ufs_extern.h"


// TODO HARVEY Pool of buffers?  Aiming for as simple as possible for now.
Buf*
newbuf(size_t size)
{
	Buf *b = smalloc(sizeof(Buf));
	b->vnode = nil;
	b->data = smalloc(size);
	b->offset = 0;
	b->bcount = size;
	return b;
}

void
releasebuf(Buf *b)
{
	if (b->data) {
		free(b->data);
	}
	free(b);
}

Uio*
newuio(int blocksize)
{
	Uio *uio = smalloc(sizeof(Uio));
	return uio;
}

void
releaseuio(Uio *uio)
{
	if (uio->dest) {
		freeblist(uio->dest);
	}
	free(uio);
}

void
packuio(Uio *uio)
{
	if (uio->dest) {
		uio->dest = concatblock(uio->dest);
	}
}

int
uiomove(void *src, int64_t srclen, Uio *uio)
{
	Block *block = mem2bl(src, srclen);

	if (!uio->dest) {
		uio->dest = block;
	} else {
		// Append block - seems like this should be a standard function
		Block *b = uio->dest;
		while (b->next) {
			b = b->next;
		}
		b->next = block;
	}

	uio->resid -= srclen;
	uio->offset += srclen;
	return 0;
}

/*
 * Wrapper to enable Harvey's channel read function to be used like FreeBSD's
 * block read function.
 * Use when reading Fs or anything else not relative to a vnode.
 */
int32_t
breadmp(MountPoint *mp, daddr_t blkno, size_t size, Buf **buf)
{
	Buf *b = newbuf(size);

	Chan *c = mp->chan;
	int64_t offset = dbtob(blkno);
	int32_t bytesRead = c->dev->read(c, b->data, size, offset);
	if (bytesRead != size) {
		releasebuf(b);
		print("bread returned wrong size\n");
		return 1;
	}

	b->resid = size - bytesRead;
	*buf = b;
	return 0;
}

/*
 * Wrapper to enable Harvey's channel read function to be used like FreeBSD's
 * block read function.
 * Use when reading relative to a vnode.
 */
int32_t
bread(vnode *vn, daddr_t lblkno, size_t size, Buf **buf)
{
	daddr_t pblkno;
	int rcode = ufs_bmaparray(vn, lblkno, &pblkno, nil, nil, nil);
	if (rcode) {
		print("bread failed to transform logical block to physical\n");
		return 1;
	}

	Buf *b = newbuf(size);
	b->vnode = vn;

	MountPoint *mp = vn->mount;
	Chan *c = mp->chan;
	int64_t offset = dbtob(pblkno);

	int32_t bytesRead = c->dev->read(c, b->data, size, offset);

	if (bytesRead != size) {
		releasebuf(b);
		print("bread returned wrong size\n");
		return 1;
	}

	b->resid = size - bytesRead;
	*buf = b;
	return 0;
}

Buf *
getblk(vnode *vn, daddr_t lblkno, size_t size, int flags)
{
	Buf *b;
	int32_t rcode = bread(vn, lblkno, size, &b);
	if (rcode) {
		print("getblk - bread failed\n");
		return nil;
	}
	return b;
}
