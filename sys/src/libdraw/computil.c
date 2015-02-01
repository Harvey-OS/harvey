/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>

/*
 * compressed data are seuences of byte codes.  
 * if the first byte b has the 0x80 bit set, the next (b^0x80)+1 bytes
 * are data.  otherwise, it's two bytes specifying a previous string to repeat.
 */
void
_twiddlecompressed(uchar *buf, int n)
{
	uchar *ebuf;
	int j, k, c;

	ebuf = buf+n;
	while(buf < ebuf){
		c = *buf++;
		if(c >= 128){
			k = c-128+1;
			for(j=0; j<k; j++, buf++)
				*buf ^= 0xFF;
		}else
			buf++;
	}
}

int
_compblocksize(Rectangle r, int depth)
{
	int bpl;

	bpl = bytesperline(r, depth);
	bpl = 2*bpl;	/* add plenty extra for blocking, etc. */
	if(bpl < NCBLOCK)
		return NCBLOCK;
	return bpl;
}
