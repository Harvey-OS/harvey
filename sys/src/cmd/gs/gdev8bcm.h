/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdev8bcm.h */
/* Dynamic color mapping for 8-bit displays */
/* Requires gxdevice.h (for gx_color_value) */

/*
 * The MS-DOS, MS Windows, and X Windows drivers all use (at least on
 * some platforms) an 8-bit color map in which some fraction is reserved
 * for a pre-allocated cube and some or all of the remainder is
 * allocated dynamically.  Since looking up colors in this map can be
 * a major performance bottleneck, we provide an efficient implementation
 * that can be shared among drivers.
 *
 * As a performance compromise, we only look up the top 5 bits of the
 * RGB value in the color map.  This compromises color quality very little,
 * and allows substantial optimizations.
 */

#define gx_8bit_map_size 323
#define gx_8bit_map_spreader 123	/* approx. 323 - (1.618 * 323) */
typedef struct gx_8bit_map_entry_s {
	ushort rgb;			/* key = 0rrrrrgggggbbbbb */
#define gx_8bit_no_rgb ((ushort)0xffff)
#define gx_8bit_rgb_key(r, g, b)\
  (((r >> (gx_color_value_bits - 5)) << 10) +\
   ((g >> (gx_color_value_bits - 5)) << 5) +\
   (b >> (gx_color_value_bits - 5)))
	short index;			/* value */
} gx_8bit_map_entry;
typedef struct gx_8bit_color_map_s {
	int count;			/* # of occupied entries */
	int max_count;			/* max # of occupied entries */
	gx_8bit_map_entry map[gx_8bit_map_size + 1];
} gx_8bit_color_map;

/* Initialize an 8-bit color map. */
void gx_8bit_map_init(P2(gx_8bit_color_map *, int));

/* Look up a color in an 8-bit color map. */
/* Return -1 if not found. */
int gx_8bit_map_rgb_color(P4(const gx_8bit_color_map *, gx_color_value,
			     gx_color_value, gx_color_value));

/* Test whether an 8-bit color map has room for more entries. */
#define gx_8bit_map_is_full(pcm)\
  ((pcm)->count == (pcm)->max_count)

/* Add a color to an 8-bit color map. */
/* Return -1 if the map is full. */
int gx_8bit_add_rgb_color(P4(gx_8bit_color_map *, gx_color_value,
			     gx_color_value, gx_color_value));
