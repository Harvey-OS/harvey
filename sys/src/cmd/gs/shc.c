/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* shc.c */
/* Support code for shc.h */
#include "std.h"
#include "scommon.h"
#include "shc.h"

/* ------ Encoding ------ */

/* Empty the 1-word buffer onto the output stream. */
byte *
hc_put_code_proc(register stream_hc_state *ss, byte *q, uint code)
{	int left = ss->bits_left;
	const uint cw = ss->bits + (code >> -left);
#define cb(n) ((byte)(cw >> (n * 8)))
#define W (hc_bits_size >> 3)
	if ( ss->FirstBitLowOrder )
	{
#if hc_bits_size > 16
		q[1] = sbits_reverse_bits[cb(3)];
		q[2] = sbits_reverse_bits[cb(2)];
#endif
		q[W-1] = sbits_reverse_bits[cb(1)];
		q[W] = sbits_reverse_bits[cb(0)];
	}
	else
	{
#if hc_bits_size > 16
		q[1] = cb(3);
		q[2] = cb(2);
#endif
		q[W-1] = cb(1);
		q[W] = cb(0);
	}
#undef cb
	ss->bits = code << (ss->bits_left = left + hc_bits_size);
	return q + W;
#undef W
}

/* Put out any final bytes. */
byte *
hc_put_last_bits(stream_hc_state *ss, byte *q)
{	while ( ss->bits_left < hc_bits_size )
	{	byte c = (byte)(ss->bits >> (hc_bits_size - 8));
		if ( ss->FirstBitLowOrder )
			c = sbits_reverse_bits[c];
		*++q = c;
		ss->bits <<= 8;
		ss->bits_left += 8;
	}
	return q;
}
