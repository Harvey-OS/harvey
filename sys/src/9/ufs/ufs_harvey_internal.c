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

/*
 * Wrapper to enable Harvey's channel read function to be used like FreeBSD's
 * block read function.
 */
int32_t
bread(MountPoint *mp, ufs2_daddr_t blockno, size_t size, void **buf)
{
	*buf = smalloc(size);

	Chan *c = mp->chan;
	int64_t offset = dbtob(blockno);
	int32_t bytesRead = c->dev->read(c, *buf, size, offset);
	if (bytesRead != size) {
		error("bread returned wrong size");
	}
	return 0;
}
