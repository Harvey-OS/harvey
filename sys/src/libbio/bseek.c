/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>

i64
Bseek(Biobufhdr *bp, i64 offset, int base)
{
	i64 n, d;

	switch(bp->state) {
	default:
		fprint(2, "Bseek: unknown state %d\n", bp->state);
		return Beof;

	case Bracteof:
		bp->state = Bractive;
		bp->icount = 0;
		bp->gbuf = bp->ebuf;

	case Bractive:
		n = offset;
		if(base == 1) {
			n += Boffset(bp);
			base = 0;
		}

		/*
		 * try to seek within buffer
		 */
		if(base == 0) {
			/*
			 * if d is too large for an int, icount may wrap,
			 * so we need to ensure that icount hasn't wrapped
			 * and points within the buffer's valid data.
			 */
			d = n - Boffset(bp);
			bp->icount += d;
			if(d <= bp->bsize && bp->icount <= 0 &&
			    bp->ebuf - bp->gbuf >= -bp->icount)
				return n;
		}

		/*
		 * reset the buffer
		 */
		n = seek(bp->fid, n, base);
		bp->icount = 0;
		bp->gbuf = bp->ebuf;
		break;

	case Bwactive:
		Bflush(bp);
		n = seek(bp->fid, offset, base);
		break;
	}
	bp->offset = n;
	return n;
}
