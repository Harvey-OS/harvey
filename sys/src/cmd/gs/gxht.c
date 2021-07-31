/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxht.c */
/* Halftone rendering routines for Ghostscript imaging library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"			/* for gs_next_ids */
#include "gxfixed.h"
#include "gzstate.h"
#include "gxdevice.h"			/* for gzht.h */
#include "gzht.h"

/*** Big memory machines ***/
#define max_cached_tiles_LARGE 577
#define max_ht_bits_LARGE 100000
/*** Small memory machines ***/
#define max_cached_tiles_SMALL 25
#define max_ht_bits_SMALL 1000

#if arch_ints_are_short
const uint ht_cache_default_max_tiles = max_cached_tiles_SMALL;
const uint ht_cache_default_max_bits = max_ht_bits_SMALL;
#else
const uint ht_cache_default_max_tiles = max_cached_tiles_LARGE;
const uint ht_cache_default_max_bits = max_ht_bits_LARGE;
#endif

private_st_ht_cache();

/* Allocate a halftone cache. */
gx_ht_cache *
gx_ht_alloc_cache(gs_memory_t *mem, uint max_tiles, uint max_bits)
{	gx_ht_cache *pcache = gs_alloc_struct(mem, gx_ht_cache, &st_ht_cache,
					      "alloc_ht_cache(struct)");
	byte *cbits = gs_alloc_bytes(mem, max_bits,
				     "alloc_ht_cache(bits)");
	gx_ht_tile *tiles = (gx_ht_tile *)gs_alloc_byte_array(mem,
				max_tiles, sizeof(gx_ht_tile),
				"alloc_ht_cache(tiles)");

	if ( pcache == 0 || cbits == 0 || tiles == 0 )
	{	gs_free_object(mem, tiles, "alloc_ht_cache(tiles)");
		gs_free_object(mem, cbits, "alloc_ht_cache(bits)");
		gs_free_object(mem, pcache, "alloc_ht_cache(struct)");
		return 0;
	}
	pcache->bits = cbits;
	pcache->bits_size = max_bits;
	pcache->tiles = tiles;
	pcache->num_tiles = max_tiles;
	gx_ht_clear_cache(pcache);
	return pcache;
}

/* Make the cache order current, and return whether */
/* there is room for all possible tiles in the cache. */
bool
gx_check_tile_cache(gs_state *pgs)
{	const gx_ht_order *porder = &pgs->dev_ht->order;
	gx_ht_cache *pcache = pgs->ht_cache;
	if ( pcache->order.bits != porder->bits )
		gx_ht_init_cache(pcache, porder);
	return pcache->levels_per_tile == 1;
}

/* Determine whether a given (width, y, height) might fit into a */
/* single tile. If so, return the byte offset of the appropriate row */
/* from the beginning of the tile; if not, return -1. */
int
gx_check_tile_size(gs_state *pgs, int w, int y, int h)
{	int tsy;
	const gx_tile_bitmap *ptile0 =
		&pgs->ht_cache->tiles[0].tile;	/* a typical tile */
#define tile0 (*ptile0)
	if ( h > tile0.rep_height || w > tile0.rep_width )
	  return -1;
	tsy = (y + pgs->dev_ht->order.phase.y) % tile0.rep_height;
	if ( tsy + h > tile0.size.y )
	  return -1;
	/* Tile fits in Y, might fit in X. */
	return tsy * tile0.raster;
#undef tile0
}

/* Render a given level into a halftone cache. */
private int render_ht(P4(gx_ht_tile *, int, const gx_ht_order *,
			 gx_bitmap_id));
gx_ht_tile *
gx_render_ht(gx_ht_cache *pcache, int b_level)
{	gx_ht_order *porder = &pcache->order;
	int level = porder->levels[b_level];
	gx_ht_tile *bt = &pcache->tiles[level / pcache->levels_per_tile];
	if ( bt->level != level )
	{	int code = render_ht(bt, level, porder, pcache->base_id);
		if ( code < 0 ) return 0;
	}
	return bt;
}

/* Load the device color into the halftone cache if needed. */
int
gx_dc_ht_binary_load(gx_device_color *pdevc, const gs_state *pgs)
{	const gx_ht_order *porder = &pgs->dev_ht->order;
	gx_ht_cache *pcache = pgs->ht_cache;
	gx_ht_tile *tile;
	if ( pcache->order.bits != porder->bits )
	  gx_ht_init_cache(pcache, porder);
	tile = gx_render_ht(pcache, pdevc->colors.binary.b_level);
	if ( tile == 0 )
	  return_error(gs_error_Fatal);
	pdevc->colors.binary.b_tile = tile;
	return 0;
}

/* Initialize the tile cache for a given screen. */
/* Cache as many different levels as will fit. */
void
gx_ht_init_cache(gx_ht_cache *pcache, const gx_ht_order *porder)
{	uint width = porder->width;
	uint height = porder->height;
	uint size = width * height + 1;
	int width_unit =
	  (width <= ht_mask_bits / 2 ? ht_mask_bits / width * width :
	   width);
	int height_unit = height;
	uint raster = porder->raster;
	uint tile_bytes = raster * height;
	int num_cached;
	int i;
	byte *tbits = pcache->bits;
	/* Make sure num_cached is within bounds */
	num_cached = pcache->bits_size / tile_bytes;
	if ( num_cached > size )
	  num_cached = size;
	if ( num_cached > pcache->num_tiles )
	  num_cached = pcache->num_tiles;
	if ( num_cached == size &&
	     tile_bytes * num_cached <= pcache->bits_size / 2
	   )
	  {	/*
		 * We can afford to replicate every tile in the cache,
		 * which will reduce breakage when tiling.  Since
		 * horizontal breakage is more expensive than vertical,
		 * and since wide shallow fills are more common than
		 * narrow deep fills, we replicate the tile horizontally.
		 */
		uint rep_raster = (pcache->bits_size / num_cached) / height;
		uint rep_count = rep_raster * 8 / width;
		width_unit = width * rep_count;
		raster = rep_raster;
		tile_bytes = rep_raster * height;
	  }
	pcache->base_id = gs_next_ids(size);
	pcache->order = *porder;
	pcache->num_cached = num_cached;
	pcache->levels_per_tile = (size + num_cached - 1) / num_cached;
	memset(tbits, 0, pcache->bits_size);
	for ( i = 0; i < num_cached; i++, tbits += tile_bytes )
	{	register gx_ht_tile *bt = &pcache->tiles[i];
		bt->level = 0;
		bt->index = i;
		bt->tile.data = tbits;
		bt->tile.raster = raster;
		bt->tile.size.x = width_unit;
		bt->tile.size.y = height_unit;
		bt->tile.rep_width = width;
		bt->tile.rep_height = height;
	}
}

/*
 * Compute and save the rendering of a given gray level
 * with the current halftone.  The cache holds multiple tiles,
 * where each tile covers a range of possible levels.
 * We adjust the tile whose range includes the desired level incrementally;
 * this saves a lot of time for the average image, where gray levels
 * don't change abruptly.  Note that the "level" is the number of bits,
 * not the index in the levels vector.
 */
private int
render_ht(gx_ht_tile *pbt, int level /* [1..num_bits-1] */,
  const gx_ht_order *porder, gx_bitmap_id base_id)
{	int old_level = pbt->level;
	register gx_ht_bit *p = &porder->bits[old_level];
	register byte *data = pbt->tile.data;
	if_debug7('H', "[H]Halftone cache slot 0x%lx: old=%d, new=%d, w=%d(%d), h=%d(%d):\n",
		 (ulong)data, old_level, level, pbt->tile.size.x,
		 porder->width, pbt->tile.size.y, porder->height);
#ifdef DEBUG
	if ( level < 0 || level > porder->num_bits )
	{	lprintf3("Error in render_ht: level=%d, old_level=%d, num_bits=%d\n", level, old_level, porder->num_bits);
		return_error(gs_error_Fatal);
	}
#endif
	/* Invert bits between the two pointers. */
	/* Note that we can use the same loop to turn bits either */
	/* on or off, using xor. */
	/* The Borland compiler generates truly dreadful code */
	/* if we don't assign the offset to a temporary. */
#if arch_ints_are_short
#  define invert_data(i)\
     { uint off = p[i].offset; *(ht_mask_t *)&data[off] ^= p[i].mask; }
#else
#  define invert_data(i) *(ht_mask_t *)&data[p[i].offset] ^= p[i].mask
#endif
#ifdef DEBUG
#  define invert(i)\
     { if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
	         (int)(p + i - porder->bits), p[i].offset, p[i].mask);\
       invert_data(i);\
     }
#else
#  define invert(i) invert_data(i)
#endif
sw:	switch ( level - old_level )
	{
	default:
		if ( level > old_level )
		{	invert(0); invert(1); invert(2); invert(3);
			p += 4; old_level += 4;
		}
		else
		{	invert(-1); invert(-2); invert(-3); invert(-4);
			p -= 4; old_level -= 4;
		}
		goto sw;
	case 7: invert(6);
	case 6: invert(5);
	case 5: invert(4);
	case 4: invert(3);
	case 3: invert(2);
	case 2: invert(1);
	case 1: invert(0);
	case 0: break;			/* Shouldn't happen! */
	case -7: invert(-7);
	case -6: invert(-6);
	case -5: invert(-5);
	case -4: invert(-4);
	case -3: invert(-3);
	case -2: invert(-2);
	case -1: invert(-1);
	}
#undef invert
	pbt->level = level;
	pbt->tile.id = base_id + level;
	/*
	 * Check whether we want to replicate the tile in the cache.
	 * Since we only do this when all the renderings will fit
	 * in the cache, we only do it once per level, and it doesn't
	 * have to be very efficient.
	 */
	/****** TEST IS WRONG if width > rep_width but tile.raster ==
	 ****** order raster.
	 ******/
	if ( pbt->tile.raster > porder->raster )
	  {	/* Replicate the tile horizontally. */
		/* The current algorithm is extremely inefficient. */
		uint orig_raster = porder->raster;
		uint tile_raster = pbt->tile.raster;
		int sx, dx, y;
		for ( y = pbt->tile.rep_height; --y >= 0; )
		  {	byte *orig_row = data + y * orig_raster;
			byte *tile_row = data + y * tile_raster;
			for ( sx = pbt->tile.rep_width; --sx >= 0; )
			  {	byte sm =
				  orig_row[sx >> 3] & (0x80 >> (sx & 7));
				for ( dx = sx + pbt->tile.size.x;
				      (dx -= pbt->tile.rep_width) >= 0;
				    )
				  {	byte *dp =
					  tile_row + (dx >> 3);
					byte dm = 0x80 >> (dx & 7);
					if ( sm ) *dp |= dm;
					else *dp &= ~dm;
				  }
			  }
		  }
	  }
	if ( pbt->tile.size.y > pbt->tile.rep_height )
	  {	/* Replicate the tile vertically. */
		uint rh = pbt->tile.rep_height;
		uint h = pbt->tile.size.y;
		uint tsize = pbt->tile.raster * rh;
		while ( (h -= rh) != 0 )
		  { memcpy(data + tsize, data, tsize);
		    data += tsize;
		  }
	  }
#ifdef DEBUG
if ( gs_debug_c('H') )
	{	byte *p = pbt->tile.data;
		int wb = pbt->tile.raster;
		byte *ptr = p + wb * pbt->tile.size.y;
		while ( p < ptr )
		{	dprintf8(" %d%d%d%d%d%d%d%d",
				 *p >> 7, (*p >> 6) & 1, (*p >> 5) & 1,
				 (*p >> 4) & 1, (*p >> 3) & 1, (*p >> 2) & 1,
				 (*p >> 1) & 1, *p & 1);
			if ( (++p - data) % wb == 0 ) dputc('\n');
		}
	}
#endif
	return 0;
}
