/* Copyright (C) 1992, 1993, 1994, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsutil.c,v 1.4 2000/09/19 19:00:33 lpd Exp $ */
/* Utilities for Ghostscript library */
#include "string_.h"
#include "memory_.h"
#include "gstypes.h"
#include "gconfigv.h"		/* for USE_ASM */
#include "gsmemory.h"		/* for init procedure */
#include "gsrect.h"		/* for prototypes */
#include "gsuid.h"
#include "gsutil.h"		/* for prototypes */

/* ------ Unique IDs ------ */

/* Generate a block of unique IDs. */
static ulong gs_next_id;

init_proc(gs_gsutil_init);	/* check prototype */
int
gs_gsutil_init(gs_memory_t *mem)
{
    gs_next_id = 1;
    return 0;
}

ulong
gs_next_ids(uint count)
{
    ulong id = gs_next_id;

    gs_next_id += count;
    return id;
}

/* ------ Memory utilities ------ */

/* Transpose an 8 x 8 block of bits.  line_size is the raster of */
/* the input data.  dist is the distance between output bytes. */
/* This routine may be supplanted by assembly code. */
#if !USE_ASM

void
memflip8x8(const byte * inp, int line_size, byte * outp, int dist)
{
    uint aceg, bdfh;

    {
	const byte *ptr4 = inp + (line_size << 2);
	const int ls2 = line_size << 1;

	aceg = ((uint)*inp) | ((uint)inp[ls2] << 8) |
	    ((uint)*ptr4 << 16) | ((uint)ptr4[ls2] << 24);
	inp += line_size, ptr4 += line_size;
	bdfh = ((uint)*inp) | ((uint)inp[ls2] << 8) |
	    ((uint)*ptr4 << 16) | ((uint)ptr4[ls2] << 24);
    }

    /* Check for all 8 bytes being the same. */
    /* This is especially worth doing for the case where all are zero. */
    if (aceg == bdfh && (aceg >> 8) == (aceg & 0xffffff)) {
	if (aceg == 0)
	    goto store;
	*outp = -((aceg >> 7) & 1);
	outp[dist] = -((aceg >> 6) & 1);
	outp += dist << 1;
	*outp = -((aceg >> 5) & 1);
	outp[dist] = -((aceg >> 4) & 1);
	outp += dist << 1;
	*outp = -((aceg >> 3) & 1);
	outp[dist] = -((aceg >> 2) & 1);
	outp += dist << 1;
	*outp = -((aceg >> 1) & 1);
	outp[dist] = -(aceg & 1);
	return;
    } {
	register uint temp;

/* Transpose a block of bits between registers. */
#define TRANSPOSE(r,s,mask,shift)\
  (r ^= (temp = ((s >> shift) ^ r) & mask),\
   s ^= temp << shift)

/* Transpose blocks of 4 x 4 */
	TRANSPOSE(aceg, aceg, 0x00000f0f, 20);
	TRANSPOSE(bdfh, bdfh, 0x00000f0f, 20);

/* Transpose blocks of 2 x 2 */
	TRANSPOSE(aceg, aceg, 0x00330033, 10);
	TRANSPOSE(bdfh, bdfh, 0x00330033, 10);

/* Transpose blocks of 1 x 1 */
	TRANSPOSE(aceg, bdfh, 0x55555555, 1);

#undef TRANSPOSE
    }

  store:
    *outp = (byte)aceg;
    outp[dist] = (byte)bdfh;
    outp += dist << 1;
    *outp = (byte)(aceg >>= 8);
    outp[dist] = (byte)(bdfh >>= 8);
    outp += dist << 1;
    *outp = (byte)(aceg >>= 8);
    outp[dist] = (byte)(bdfh >>= 8);
    outp += dist << 1;
    *outp = (byte)(aceg >> 8);
    outp[dist] = (byte)(bdfh >> 8);
}

#endif /* !USE_ASM */

/* Get an unsigned, big-endian 32-bit value. */
ulong
get_u32_msb(const byte *p)
{
    return ((uint)p[0] << 24) + ((uint)p[1] << 16) + ((uint)p[2] << 8) + p[3];
}

/* ------ String utilities ------ */

/* Compare two strings, returning -1 if the first is less, */
/* 0 if they are equal, and 1 if first is greater. */
/* We can't use memcmp, because we always use unsigned characters. */
int
bytes_compare(const byte * s1, uint len1, const byte * s2, uint len2)
{
    register uint len = len1;

    if (len2 < len)
	len = len2;
    {
	register const byte *p1 = s1;
	register const byte *p2 = s2;

	while (len--)
	    if (*p1++ != *p2++)
		return (p1[-1] < p2[-1] ? -1 : 1);
    }
    /* Now check for differing lengths */
    return (len1 == len2 ? 0 : len1 < len2 ? -1 : 1);
}

/* Test whether a string matches a pattern with wildcards. */
/* '*' = any substring, '?' = any character, '\' quotes next character. */
const string_match_params string_match_params_default = {
    '*', '?', '\\', false
};

bool
string_match(const byte * str, uint len, const byte * pstr, uint plen,
	     register const string_match_params * psmp)
{
    const byte *pback = 0;
    const byte *spback = 0;	/* initialized only to pacify gcc */
    const byte *p = pstr, *pend = pstr + plen;
    const byte *sp = str, *spend = str + len;

    if (psmp == 0)
	psmp = &string_match_params_default;
  again:while (p < pend) {
	register byte ch = *p;

	if (ch == psmp->any_substring) {
	    pback = ++p, spback = sp;
	    continue;
	} else if (ch == psmp->any_char) {
	    if (sp == spend)
		return false;	/* str too short */
	    p++, sp++;
	    continue;
	} else if (ch == psmp->quote_next) {
	    if (++p == pend)
		return true;	/* bad pattern */
	    ch = *p;
	}
	if (sp == spend)
	    return false;	/* str too short */
	if (*sp == ch ||
	    (psmp->ignore_case && (*sp ^ ch) == 0x20 &&
	     (ch &= ~0x20) >= 0x41 && ch <= 0x5a)
	    )
	    p++, sp++;
	else if (pback == 0)
	    return false;	/* no * to back up to */
	else {
	    sp = ++spback;
	    p = pback;
	}
    }
    if (sp < spend) {		/* We got a match, but there are chars left over. */
	/* If we can back up, back up to the only place that */
	/* could produce a complete match, otherwise fail. */
	if (pback == 0)
	    return false;
	p = pback;
	pback = 0;
	sp = spend - (pend - p);
	goto again;
    }
    return true;
}

/* ------ UID utilities ------ */

/* Compare two UIDs for equality. */
/* We know that at least one of them is valid. */
bool
uid_equal(register const gs_uid * puid1, register const gs_uid * puid2)
{
    if (puid1->id != puid2->id)
	return false;
    if (puid1->id >= 0)
	return true;		/* UniqueID */
    return
	!memcmp((const char *)puid1->xvalues,
		(const char *)puid2->xvalues,
		(uint) - (puid1->id) * sizeof(long));
}

/* ------ Rectangle utilities ------ */

/*
 * Calculate the difference of two rectangles, a list of up to 4 rectangles.
 * Return the number of rectangles in the list, and set the first rectangle
 * to the intersection.
 */
int
int_rect_difference(gs_int_rect * outer, const gs_int_rect * inner,
		    gs_int_rect * diffs /*[4] */ )
{
    int x0 = outer->p.x, y0 = outer->p.y;
    int x1 = outer->q.x, y1 = outer->q.y;
    int count = 0;

    if (y0 < inner->p.y) {
	diffs[0].p.x = x0, diffs[0].p.y = y0;
	diffs[0].q.x = x1, diffs[0].q.y = min(y1, inner->p.y);
	outer->p.y = y0 = diffs[0].q.y;
	++count;
    }
    if (y1 > inner->q.y) {
	diffs[count].p.x = x0, diffs[count].p.y = max(y0, inner->q.y);
	diffs[count].q.x = x1, diffs[count].q.y = y1;
	outer->q.y = y1 = diffs[count].p.y;
	++count;
    }
    if (x0 < inner->p.x) {
	diffs[0].p.x = x0, diffs[0].p.y = y0;
	diffs[0].q.x = min(x1, inner->p.x), diffs[0].q.y = y1;
	outer->p.x = x0 = diffs[count].q.x;
	++count;
    }
    if (x1 > inner->q.x) {
	diffs[count].p.x = max(x0, inner->q.x), diffs[count].p.y = y0;
	diffs[count].q.x = x1, diffs[count].q.y = y1;
	outer->q.x = x1 = diffs[count].p.x;
	++count;
    }
    return count;
}
