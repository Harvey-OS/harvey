/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gxhtbit.c,v 1.5 2002/02/21 22:24:53 giles Exp $ */
/* Halftone bit updating for imaging library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsbitops.h"
#include "gscdefs.h"
#include "gxbitmap.h"
#include "gxhttile.h"
#include "gxtmap.h"
#include "gxdht.h"
#include "gxdhtres.h"

extern_gx_device_halftone_list();

/*
 * Construct a standard-representation order from a threshold array.
 */
private int
construct_ht_order_default(gx_ht_order *porder, const byte *thresholds)
{
    gx_ht_bit *bits = (gx_ht_bit *)porder->bit_data;
    uint i;

    for (i = 0; i < porder->num_bits; i++)
	bits[i].mask = max(1, thresholds[i]);
    gx_ht_complete_threshold_order(porder);
    return 0;
}

/*
 * Construct a short-representation order from a threshold array.
 * Uses porder->width, num_levels, num_bits, levels, bit_data;
 * sets porder->levels[], bit_data[].
 */
private int
construct_ht_order_short(gx_ht_order *porder, const byte *thresholds)
{
    uint size = porder->num_bits;
    uint i;
    ushort *bits = (ushort *)porder->bit_data;
    uint *levels = porder->levels;
    uint num_levels = porder->num_levels;

    memset(levels, 0, num_levels * sizeof(*levels));
    /* Count the number of threshold elements with each value. */
    for (i = 0; i < size; i++) {
	uint value = max(1, thresholds[i]);

	if (value + 1 < num_levels)
	    levels[value + 1]++;
    }
    for (i = 2; i < num_levels; ++i)
	levels[i] += levels[i - 1];
    /* Now construct the actual order. */
    {
	uint width = porder->width;
	uint padding = bitmap_raster(width) * 8 - width;

	for (i = 0; i < size; i++) {
	    uint value = max(1, thresholds[i]);

	    /* Adjust the bit index to account for padding. */
	    bits[levels[value]++] = i + (i / width * padding);
	}
    }

    /* Check whether this is a predefined halftone. */
    {
	const gx_dht_proc *phtrp = gx_device_halftone_list;

	for (; *phtrp; ++phtrp) {
	    const gx_device_halftone_resource_t *const *pphtr = (*phtrp)();
	    const gx_device_halftone_resource_t *phtr;

	    while ((phtr = *pphtr++) != 0) {
		if (phtr->Width == porder->width &&
		    phtr->Height == porder->height &&
		    phtr->elt_size == sizeof(ushort) &&
		    !memcmp(phtr->levels, levels, num_levels * sizeof(*levels)) &&
		    !memcmp(phtr->bit_data, porder->bit_data,
			    size * phtr->elt_size)
		    ) {
		    /*
		     * This is a predefined halftone.  Free the levels and
		     * bit_data arrays, replacing them with the built-in ones.
		     */
		    if (porder->data_memory) {
			gs_free_object(porder->data_memory, porder->bit_data,
				       "construct_ht_order_short(bit_data)");
			gs_free_object(porder->data_memory, porder->levels,
				       "construct_ht_order_short(levels)");
		    }
		    porder->data_memory = 0;
		    porder->levels = (uint *)phtr->levels; /* actually const */
		    porder->bit_data = (void *)phtr->bit_data; /* actually const */
		    goto out;
		}
	    }
	}
    }
 out:
    return 0;
}

/* Return the bit coordinate using the standard representation. */
private int
ht_bit_index_default(const gx_ht_order *porder, uint index, gs_int_point *ppt)
{
    const gx_ht_bit *phtb = &((const gx_ht_bit *)porder->bit_data)[index];
    uint offset = phtb->offset;
    int bit = 0;

    while (!(((const byte *)&phtb->mask)[bit >> 3] & (0x80 >> (bit & 7))))
	++bit;
    ppt->x = (offset % porder->raster * 8) + bit;
    ppt->y = offset / porder->raster;
    return 0;
}

/* Return the bit coordinate using the short representation. */
private int
ht_bit_index_short(const gx_ht_order *porder, uint index, gs_int_point *ppt)
{
    uint bit_index = ((const ushort *)porder->bit_data)[index];
    uint bit_raster = porder->raster * 8;

    ppt->x = bit_index % bit_raster;
    ppt->y = bit_index / bit_raster;
    return 0;
}

/* Update a halftone tile using the default order representation. */
private int
render_ht_default(gx_ht_tile *pbt, int level, const gx_ht_order *porder)
{
    int old_level = pbt->level;
    register const gx_ht_bit *p =
	(const gx_ht_bit *)porder->bit_data + old_level;
    register byte *data = pbt->tiles.data;

    /*
     * Invert bits between the two levels.  Note that we can use the same
     * loop to turn bits either on or off, using xor.  The Borland compiler
     * generates truly dreadful code if we don't use a temporary, and it
     * doesn't hurt better compilers, so we always use one.
     */
#define INVERT_DATA(i)\
     BEGIN\
       ht_mask_t *dp = (ht_mask_t *)&data[p[i].offset];\
       *dp ^= p[i].mask;\
     END
#ifdef DEBUG
#  define INVERT(i)\
     BEGIN\
       if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
	         (int)(p + i - (const gx_ht_bit *)porder->bit_data),\
		 p[i].offset, p[i].mask);\
       INVERT_DATA(i);\
     END
#else
#  define INVERT(i) INVERT_DATA(i)
#endif
  sw:switch (level - old_level) {
	default:
	    if (level > old_level) {
		INVERT(0); INVERT(1); INVERT(2); INVERT(3);
		p += 4; old_level += 4;
	    } else {
		INVERT(-1); INVERT(-2); INVERT(-3); INVERT(-4);
		p -= 4; old_level -= 4;
	    }
	    goto sw;
	case 7: INVERT(6);
	case 6: INVERT(5);
	case 5: INVERT(4);
	case 4: INVERT(3);
	case 3: INVERT(2);
	case 2: INVERT(1);
	case 1: INVERT(0);
	case 0: break;		/* Shouldn't happen! */
	case -7: INVERT(-7);
	case -6: INVERT(-6);
	case -5: INVERT(-5);
	case -4: INVERT(-4);
	case -3: INVERT(-3);
	case -2: INVERT(-2);
	case -1: INVERT(-1);
    }
#undef INVERT_DATA
#undef INVERT
    return 0;
}

/* Update a halftone tile using the short representation. */
private int
render_ht_short(gx_ht_tile *pbt, int level, const gx_ht_order *porder)
{
    int old_level = pbt->level;
    register const ushort *p = (const ushort *)porder->bit_data + old_level;
    register byte *data = pbt->tiles.data;

    /* Invert bits between the two levels. */
#define INVERT_DATA(i)\
     BEGIN\
       uint bit_index = p[i];\
       byte *dp = &data[bit_index >> 3];\
       *dp ^= 0x80 >> (bit_index & 7);\
     END
#ifdef DEBUG
#  define INVERT(i)\
     BEGIN\
       if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
	         (int)(p + i - (const ushort *)porder->bit_data),\
		 p[i] >> 3, 0x80 >> (p[i] & 7));\
       INVERT_DATA(i);\
     END
#else
#  define INVERT(i) INVERT_DATA(i)
#endif
  sw:switch (level - old_level) {
	default:
	    if (level > old_level) {
		INVERT(0); INVERT(1); INVERT(2); INVERT(3);
		p += 4; old_level += 4;
	    } else {
		INVERT(-1); INVERT(-2); INVERT(-3); INVERT(-4);
		p -= 4; old_level -= 4;
	    }
	    goto sw;
	case 7: INVERT(6);
	case 6: INVERT(5);
	case 5: INVERT(4);
	case 4: INVERT(3);
	case 3: INVERT(2);
	case 2: INVERT(1);
	case 1: INVERT(0);
	case 0: break;		/* Shouldn't happen! */
	case -7: INVERT(-7);
	case -6: INVERT(-6);
	case -5: INVERT(-5);
	case -4: INVERT(-4);
	case -3: INVERT(-3);
	case -2: INVERT(-2);
	case -1: INVERT(-1);
    }
#undef INVERT_DATA
#undef INVERT
    return 0;
}

/* Define the procedure vectors for the order data implementations. */
const gx_ht_order_procs_t ht_order_procs_table[2] = {
    { sizeof(gx_ht_bit), construct_ht_order_default, ht_bit_index_default,
      render_ht_default },
    { sizeof(ushort), construct_ht_order_short, ht_bit_index_short,
      render_ht_short }
};
