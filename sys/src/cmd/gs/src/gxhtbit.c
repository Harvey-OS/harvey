/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxhtbit.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
    { sizeof(gx_ht_bit), construct_ht_order_default, render_ht_default },
    { sizeof(ushort), construct_ht_order_short, render_ht_short }
};
