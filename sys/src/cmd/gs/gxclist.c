/* Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxclist.c */
/* Command list writing for Ghostscript. */
#include "memory_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* must precede gxcldev.h */
#include "gxcldev.h"
#include "strimpl.h"
#include "srlx.h"

/* Forward declarations of procedures */
private dev_proc_open_device(clist_open);
private dev_proc_output_page(clist_output_page);
private dev_proc_fill_rectangle(clist_fill_rectangle);
private dev_proc_tile_rectangle(clist_tile_rectangle);
private dev_proc_copy_mono(clist_copy_mono);
private dev_proc_copy_color(clist_copy_color);
extern dev_proc_get_bits(clist_get_bits);	/* in gxclread.c */

/* The device procedures */
gx_device_procs gs_clist_device_procs =
{	clist_open,
	gx_forward_get_initial_matrix,
	gx_default_sync_output,
	clist_output_page,
	gx_default_close_device,
	gx_forward_map_rgb_color,
	gx_forward_map_color_rgb,
	clist_fill_rectangle,
	clist_tile_rectangle,
	clist_copy_mono,
	clist_copy_color,
	gx_default_draw_line,
	clist_get_bits,
	gx_forward_get_params,
	gx_forward_put_params,
	gx_forward_map_cmyk_color,
	gx_forward_get_xfont_procs,
	gx_forward_get_xfont_device,
	gx_forward_map_rgb_alpha_color,
	gx_forward_get_page_device,
	gx_default_get_alpha_bits,		/* no alpha capability */
	gx_default_copy_alpha
};

/* ------ Define the command set and syntax ------ */

#ifdef DEBUG
const char *cmd_op_names[16] = { cmd_op_name_strings };
const char *cmd_misc_op_names[16] = { cmd_misc_op_name_strings };
private ulong far_data cmd_op_counts[256];
private ulong cmd_tile_count, cmd_copy_count, cmd_copy_rle_count,
  cmd_delta_tile_count;
private ulong cmd_tile_reset, cmd_tile_found, cmd_tile_added;
private int
count_op(int op)
{	++cmd_op_counts[op];
	if_debug2('L', ", %s %d\n", cmd_op_names[op >> 4], op & 0xf);
	fflush(dstderr);
	return op;
}
#  define count_add(v, n) (v += (n))
#else
#  define count_op(store_op) store_op
#  define count_add(v, n) DO_NOTHING
#endif
#define count_add1(v) count_add(v, 1)

/* Initialize the device state */
private void clist_init_tiles(P1(gx_device_clist *));
private int
clist_open(gx_device *dev)
{	/*
	 * The buffer area (data, data_size) holds a tile cache and a
	 * set of block range bit masks when both writing and reading.
	 * The rest of the space is used for
	 * the command buffer and band state bookkeeping when writing,
	 * and for the rendering buffer (image device) when reading.
	 * For the moment, we divide the space up arbitrarily.
	 *
	 * This routine requires only data, data_size, target, and mdev
	 * to have been set in the device structure, and is idempotent,
	 * so it can be used to check whether a given-size buffer
	 * is large enough.
	 */
	byte *data = cdev->data;
	uint size = cdev->data_size;
#define alloc_data(n) data += (n), size -= (n)
	gx_device *target = cdev->target;
	uint raster, nbands, band;
	gx_clist_state *states;
	ulong state_size;
	cdev->ymin = cdev->ymax = -1;	/* render_init not done yet */
	cdev->tile_data = data;
	cdev->tile_data_size = (size / 5) & -4;	/* arbitrary! */
	alloc_data(cdev->tile_data_size);
	raster = gx_device_raster(target, 1) + sizeof(byte *);
	cdev->band_height = size / raster;
	if ( cdev->band_height == 0 )	/* can't even fit one scan line */
		return_error(gs_error_limitcheck);
	nbands = target->height / cdev->band_height + 1;
	cdev->nbands = nbands;
	if_debug4('l', "[l]width=%d, raster=%d, band_height=%d, nbands=%d\n",
	         target->width, raster, cdev->band_height, cdev->nbands);
	state_size = nbands * (ulong)sizeof(gx_clist_state);
	if ( state_size + sizeof(cmd_prefix) + cmd_largest_size + raster + 4 > size )		/* not enough room */
		return_error(gs_error_limitcheck);
	cdev->mdev.base = data;
	cdev->states = states = (gx_clist_state *)data;
	alloc_data((uint)state_size);
	cdev->cbuf = data;
	cdev->cnext = data;
	cdev->cend = data + size;
	cdev->ccls = 0;
	for ( band = 0; band < nbands; band++, states++ )
	  *states = cls_initial;
#undef alloc_data
	/* Round up the size of the band mask so that */
	/* the bits, which follow it, stay aligned. */
	cdev->tile_band_mask_size =
	  ((nbands + (align_bitmap_mod * 8 - 1)) >> 3) &
	    ~(align_bitmap_mod - 1);
	cdev->tile_max_size = cdev->tile_data_size -
		(sizeof(tile_hash) * 2 + sizeof(tile_slot) +
		 cdev->tile_band_mask_size);
	cdev->tile_depth = 1;
	clist_init_tiles(cdev);
	return 0;
}

/* (Re)initialize the tile cache. */
private void
clist_init_tiles(register gx_device_clist *cldev)
{	gx_clist_state *pcls;
	int i, hc;
	cldev->tile_slot_size =
	  sizeof(tile_slot) + cldev->tile_band_mask_size +
	  cldev->tile.raster * cldev->tile.size.y;
	cldev->tile_max_count = cldev->tile_data_size /
	  (sizeof(tile_hash) * 3 /*(worst case)*/ + cldev->tile_slot_size);
	hc = (cldev->tile_max_count - 1) * 2;
	while ( (hc + 1) & hc ) hc |= hc >> 1;	/* make mask */
	if ( hc >= cldev->tile_max_count * 3 ) hc >>= 1;
	if ( hc > 255 )		/* slot index in set_tile is only 1 byte */
	   {	hc = 255;
		if ( cldev->tile_max_count > 200 )
			cldev->tile_max_count = 200;
	   }
	cldev->tile_hash_mask = hc;
	hc++;				/* make actual size */
	if_debug6('l', "[l]tile.size=%dx%dx%d, slot_size=%d, max_count=%d, hc=%d\n",
		 cldev->tile.size.x, cldev->tile.size.y, cldev->tile_depth,
		 cldev->tile_slot_size, cldev->tile_max_count, hc);
	cldev->tile_hash_table =
		(tile_hash *)(cldev->tile_data + cldev->tile_data_size) - hc;
	cldev->tile_count = 0;
	memset(cldev->tile_data, 0, cldev->tile_data_size);
	memset(cldev->tile_hash_table, -1, hc * sizeof(tile_hash));
	for ( i = 0, pcls = cldev->states; i < cldev->nbands; i++, pcls++ )
		pcls->tile = &no_tile;
	count_add1(cmd_tile_reset);
}

/* Clean up after rendering a page. */
private int
clist_output_page(gx_device *dev, int num_copies, int flush)
{	if ( flush )
	   {	rewind(cdev->cfile);
		rewind(cdev->bfile);
		cdev->bfile_end_pos = 0;
	   }
	else
	   {	fseek(cdev->cfile, 0L, SEEK_END);
		fseek(cdev->bfile, 0L, SEEK_END);
	   }
	return clist_open(dev);		/* reinitialize */
}

/* Print statistics. */
#ifdef DEBUG
void
cmd_print_stats(void)
{	int ci, cj;
	dprintf4("[l]counts: tile = %ld, copy = %ld, copy_rle = %ld, delta = %ld\n",
	         cmd_tile_count, cmd_copy_count, cmd_copy_rle_count, cmd_delta_tile_count);
	dprintf3("           reset = %ld, found = %ld, added = %ld\n",
	         cmd_tile_reset, cmd_tile_found, cmd_tile_added);
	for ( ci = 0; ci < 0x100; ci += 0x10 )
	   {	dprintf1("[l]  %s =", cmd_op_names[ci >> 4]);
		for ( cj = ci; cj < ci + 0x10; cj++ )
			dprintf1(" %ld", cmd_op_counts[cj]);
		dputs("\n");
	   }
}
#endif

/* ------ Writing ------ */

/* Utilities */

#define cmd_set_rect(rect)\
  ((rect).x = x, (rect).y = y,\
   (rect).width = width, (rect).height = height)

/* Write out the buffered commands, and reset the buffer. */
private int
cmd_write_buffer(gx_device_clist *cldev)
{	FILE *cfile = cldev->cfile;
	FILE *bfile = cldev->bfile;
	int nbands = cldev->nbands;
	gx_clist_state *pcls;
	int band;
	for ( band = 0, pcls = cldev->states; band < nbands; band++, pcls++ )
	{	const cmd_prefix *cp = pcls->head;
		if ( cp != 0 )
		{	cmd_block cb;
			cb.band = band;
			cb.pos = ftell(cfile);
			if_debug2('l', "[l]writing for band %d at %ld\n",
				  band, cb.pos);
			clist_write(bfile, (const byte *)&cb, sizeof(cb));
			pcls->tail->next = 0;	/* terminate the list */
			for ( ; cp != 0; cp = cp->next )
			  clist_write(cfile, (const byte *)(cp + 1), cp->size);
			pcls->head = pcls->tail = 0;
			fputc(cmd_opv_end_run, cfile);
		}
	}
	cldev->cnext = cldev->cbuf;
	cldev->ccls = 0;
	if ( ferror(bfile) || ferror(cfile) )
		return_error(gs_error_ioerror);
	return_check_interrupt(0);
}
/* Export under a different name for gxclread.c */
int
clist_flush_buffer(gx_device_clist *cldev)
{	return cmd_write_buffer(cldev);
}

/* Add a command to the appropriate band list, */
/* and allocate space for its data. */
/* Return the pointer to the data area. */
private byte *
cmd_put_op(gx_device_clist *cldev, gx_clist_state *pcls, uint size)
{	byte *dp = cldev->cnext;
	if_debug3('L', "[L]band %d: size=%u, left=%u",
		  (int)(pcls - cldev->states),
		  size, (uint)(cldev->cend - dp));
	if ( size + (sizeof(cmd_prefix) + 4) > cldev->cend - dp )
	  { cmd_write_buffer(cldev);
	    return cmd_put_op(cldev, pcls, size);
	  }
	if ( cldev->ccls == pcls )
	  { /* We're adding another command for the same band. */
	    /* Tack it onto the end of the previous one. */
	    pcls->tail->size += size;
	  }
	else
	  { /* Skip to an appropriate alignment boundary. */
	    /* (We assume the command buffer itself is aligned.) */
	    cmd_prefix *cp =
	      (cmd_prefix *)(dp +
			     ((cldev->cbuf - dp) & (arch_align_ptr_mod - 1)));
	    dp = (byte *)(cp + 1);
	    if ( pcls->tail != 0 ) pcls->tail->next = cp;
	    else pcls->head = cp;
	    pcls->tail = cp;
	    cldev->ccls = pcls;
	    cp->size = size;
	  }
	cldev->cnext = dp + size;
	return dp;
}

/* Shorten the last allocated command. */
private void
cmd_shorten_op(gx_device_clist *cldev, gx_clist_state *pcls, uint delta)
{	pcls->tail->size -= delta;
	cldev->cnext -= delta;
}

/* Write a variable-size positive integer. */
/* (This works for negative integers also; they are written as though */
/* they were unsigned.) */
#define w1byte(w) (!((w) & ~0x7f))
#define w2byte(w) (!((w) & ~0x3fff))
#define cmd_sizew(w)\
  (w1byte(w) ? 1 : w2byte(w) ? 2 : cmd_w_size((uint)(w)))
#define cmd_sizexy(xy)\
  (w1byte((xy).x | (xy).y) ? 2 :\
   cmd_w_size((uint)(xy).x) + cmd_w_size((uint)(xy).y))
private int near
cmd_w_size(register uint w)
{	register int size = 1;
	while ( w > 0x7f ) w >>= 7, size++;
	return size;
}
#define cmd_putw(w,dp)\
  (w1byte(w) ? (*dp = w, ++dp) :\
   w2byte(w) ? (*dp = (w) | 0x80, dp[1] = (w) >> 7, dp += 2) :\
   (dp = cmd_w_put((uint)(w), dp)))
#define cmd_putxy(xy,dp)\
  (w1byte((xy).x | (xy).y) ? (dp[0] = (xy).x, dp[1] = (xy).y, dp += 2) :\
   (dp = cmd_w_put((uint)(xy).y, cmd_w_put((uint)(xy).x, dp))))
private byte *near
cmd_w_put(register uint w, register byte *dp)
{	while ( w > 0x7f ) *dp++ = w | 0x80, w >>= 7;
	*dp = w;
	return dp + 1;
}

/* Write a rectangle. */
private int
cmd_size_rect(register const gx_cmd_rect *prect)
{	return cmd_sizew(prect->x) + cmd_sizew(prect->y) +
		cmd_sizew(prect->width) + cmd_sizew(prect->height);
}
private byte *
cmd_put_rect(register const gx_cmd_rect *prect, register byte *dp)
{	cmd_putw(prect->x, dp);
	cmd_putw(prect->y, dp);
	cmd_putw(prect->width, dp);
	cmd_putw(prect->height, dp);
	return dp;
}

/* Write a short bitmap.  1 <= bwidth <= 3. */
private void
cmd_put_short_bits(register byte *dp, register const byte *data,
  int raster, register int bwidth, register int height)
{	while ( --height >= 0 )
	   {	switch ( bwidth )
		   {
		case 3: dp[2] = data[2];
		case 2: dp[1] = data[1];
		case 1: dp[0] = data[0];
		   }
		dp += bwidth, data += raster;
	   }
}

private int
cmd_write_rect_cmd(gx_device *dev, gx_clist_state *pcls,
  int op, int x, int y, int width, int height)
{	int dx = x - pcls->rect.x;
	int dy = y - pcls->rect.y;
	int dwidth = width - pcls->rect.width;
	int dheight = height - pcls->rect.height;
#define check_ranges_1()\
  ((unsigned)(dx - rmin) <= (rmax - rmin) &&\
   (unsigned)(dy - rmin) <= (rmax - rmin) &&\
   (unsigned)(dwidth - rmin) <= (rmax - rmin))
#define check_ranges()\
  (check_ranges_1() &&\
   (unsigned)(dheight - rmin) <= (rmax - rmin))
#define rmin cmd_min_tiny
#define rmax cmd_max_tiny
	cmd_set_rect(pcls->rect);
	if ( dheight == 0 && check_ranges_1() )
	   {	byte *dp = cmd_put_op(cdev, pcls, 2);
		count_op(*dp = op + 0x20 + dwidth - rmin);
		dp[1] = (dx << 4) + dy - (rmin * 0x11);
	   }
#undef rmin
#undef rmax
#define rmin cmd_min_short
#define rmax cmd_max_short
	else if ( check_ranges() )
	   {	int dh = dheight - cmd_min_tiny;
		byte *dp;
		if ( (unsigned)dh <= cmd_max_tiny - cmd_min_tiny && dh != 0 &&
		     dy == 0
		   )
		   {	op += dh;
			dp = cmd_put_op(cdev, pcls, 3);
		   }
		else
		   {	dp = cmd_put_op(cdev, pcls, 5);
			dp[3] = dy - rmin;
			dp[4] = dheight - rmin;
		   }
		count_op(*dp = op + 0x10);
		dp[1] = dx - rmin;
		dp[2] = dwidth - rmin;
	   }
	else
	   {	byte *dp = cmd_put_op(cdev, pcls, 1 + cmd_size_rect(&pcls->rect));
		count_op(*dp = op);
		dp = cmd_put_rect(&pcls->rect, dp + 1);
	   }
	return 0;
}

private void
cmd_put_color(gx_device *dev, gx_clist_state *pcls,
  int op, gx_color_index color)
{	if ( (long)color >= -1 && (long)color <= 13 )
		count_op(*cmd_put_op(cdev, pcls, 1) = op + (int)color + 2);
	else
	   {	byte *dp = cmd_put_op(cdev, pcls, 1 + sizeof(color));
		count_op(*dp = op);
		memcpy(dp + 1, &color, sizeof(color));
	   }
}
private void
cmd_set_colors(gx_device *dev, gx_clist_state *pcls,
  gx_color_index color0, gx_color_index color1)
{	if ( color0 != pcls->color0 )
	   {	cmd_put_color(dev, pcls, cmd_op_set_color0, color0);
		pcls->color0 = color0;
	   }
	if ( color1 != pcls->color1 )
	   {	cmd_put_color(dev, pcls, cmd_op_set_color1, color1);
		pcls->color1 = color1;
	   }
}

/* Driver interface */

/* Macros for dividing up a single call into bands */
#define BEGIN_RECT\
   {	int yend = y + height;\
	int band_height = cdev->band_height;\
	do\
	   {	int band = y / band_height;\
		gx_clist_state *pcls = cdev->states + band;\
		height = band_height - y % band_height;\
		if ( yend - y < height ) height = yend - y;\
		   {
#define END_RECT\
		   }\
		y += height;\
	   }\
	while ( y < yend );\
   }

private int
clist_fill_rectangle(gx_device *dev, int x, int y, int width, int height,
  gx_color_index color)
{	fit_fill(dev, x, y, width, height);
	BEGIN_RECT
	if ( color != pcls->color1 )
		cmd_set_colors(dev, pcls, pcls->color0, color);
	cmd_write_rect_cmd(dev, pcls, cmd_op_fill_rect, x, y, width, height);
	END_RECT
	return 0;
}

/* Compare unequal tiles.  Return -1 if unrelated, */
/* or 2<=N<=50 for the size of the delta encoding. */
private int
tile_diff(const byte *old_data, const byte *new_data, uint tsize,
  byte _ss *delta)
{	register const bits16 *old2, *new2;
	register bits16 diff;
	int count;
	register int i;
	byte _ss *pd;
	if ( tsize > 128 ) return -1;
	old2 = (const bits16 *)old_data;
	new2 = (const bits16 *)new_data;
	count = 0;
	pd = delta + 2;			/* skip slot index */
	for ( i = 0; i < tsize; i += 2, old2++, new2++ )
	  if ( (diff = *new2 ^ *old2) != 0 )
#if arch_is_big_endian
#  define i_hi 0
#  define b_0(w) ((w) >> 8)
#  define b_1(w) ((byte)(w))
#else
#  define i_hi 1
#  define b_0(w) ((byte)(w))
#  define b_1(w) ((w) >> 8)
#endif
	   {	if ( count == 16 ) return -1;
		if ( diff & 0xff00 )
		   {	if ( diff & 0xff )
				*pd++ = 0x80 + i,
				*pd++ = b_0(diff),
				*pd++ = b_1(diff);
			else
				*pd++ = i + i_hi, *pd++ = diff >> 8;
		   }
		else			/* know diff != 0 */
			*pd++ = i + (1 - i_hi), *pd++ = (byte)diff;
		count++;
	   }
#undef b_0
#undef b_1
#undef i_hi
	if ( count == 0 )
	{	/* Tiles are identical.  This is highly unusual, */
		/* but not impossible. */
		pd[0] = pd[1] = 0;
		pd += 2;
		count = 1;
	}
	delta[0] = (byte)cmd_op_delta_tile_bits + count - 1;
	return pd - delta;
}

/* Handle changing tiles for clist_tile_rectangle. */
/* We put this in a separate routine, even though it is called only once, */
/* to avoid cluttering up the main-line case of tile_rectangle. */
private int
clist_change_tile(gx_device_clist *cldev, gx_clist_state *pcls,
  const gx_tile_bitmap *tile, int depth)
{	uint tile_size = tile->raster * tile->size.y;
	tile_slot *old_tile, *new_tile;
	int slot_index;
	/* Look up the tile in the cache. */
top:	   {	gx_bitmap_id id = tile->id;
		uint probe = (uint)(id >> 16) + (uint)(id);
		old_tile = pcls->tile;
		for ( ; ; probe += 25 /* semi-random odd # */ )
		   {	tile_hash *hptr = cldev->tile_hash_table +
			  (probe & cldev->tile_hash_mask);
			if ( (slot_index = hptr->slot_index) < 0 ) /* empty entry */
			   {	/* Must change tiles.  Check whether the */
				/* tile size or depth has changed. */
				if ( tile->size.x != cldev->tile.size.x ||
				     tile->size.y != cldev->tile.size.y ||
				     depth != cldev->tile_depth
				   )
				   {	if ( tile->raster !=
					     bitmap_raster(tile->size.x * depth) ||
					     tile_size > cldev->tile_max_size
					   )
						return -1;
					cldev->tile = *tile;	/* reset size, raster */
					cldev->tile_depth = depth;
					clist_init_tiles(cldev);
					goto top;
				   }
				if ( cldev->tile_count == cldev->tile_max_count )
				   {	/* Punt. */
					clist_init_tiles(cldev);
					goto top;
				   }
				hptr->slot_index = slot_index =
				  cldev->tile_count++;
				new_tile = tile_slot_ptr(cldev, slot_index);
				new_tile->id = id;
				memcpy(ts_bits(cldev, new_tile), tile->data, tile_size);
				count_add1(cmd_tile_added);
				if_debug3('L', "[L]adding tile %d, hash=%d, id=0x%lx\n",
					 slot_index,
					 (int)(hptr - cldev->tile_hash_table),
					 id);
				break;
			   }
			new_tile = tile_slot_ptr(cldev, slot_index);
			if ( new_tile->id == id )
			   {	count_add1(cmd_tile_found);
				if_debug1('L', "[L]found tile %d\n",
					  slot_index);
				break;
			   }
		   }
	   }
	/* Check whether this band knows about this tile yet. */
	   {	int band_index = pcls - cldev->states;
		byte pmask = 1 << (band_index & 7);
		byte *ppresent = ts_mask(new_tile) + (band_index >> 3);
		if ( *ppresent & pmask )
		   {	/* Tile is known, just put out the index. */
			byte *dp = cmd_put_op(cldev, pcls, 2);
			count_op(*dp = cmd_opv_set_tile_index);
			dp[1] = slot_index;
		   }
		else
		   {	/* Tile is not known, put out the bits.  Use a */
			/* delta encoding or a short encoding if possible. */
			byte *new_data = ts_bits(cldev, new_tile);
			byte *dp;
			byte delta[2+16*3];
			int diff;
			*ppresent |= pmask;
			if ( old_tile != &no_tile &&
			     (diff = tile_diff(ts_bits(cldev, old_tile), new_data, tile_size, delta)) >= 0
			   )
			   {	/* Use delta representation */
				dp = cmd_put_op(cldev, pcls, diff);
				count_op(delta[0]);
				delta[1] = slot_index;
				memcpy(dp, delta, diff);
				count_add(cmd_delta_tile_count, diff - 2);
			   }
			else
			   {	if ( old_tile == &no_tile )
				   {	byte *dp = cmd_put_op(cldev, pcls,
					    1 + cmd_sizexy(cldev->tile.size));
					byte op =
					  (depth == 1 ?
					   (byte)cmd_opv_set_tile_size :
					   (byte)cmd_opv_set_tile_size_colored);
					count_op(*dp++ = op);
					cmd_putxy(cldev->tile.size, dp);
				   }
				if ( tile->size.x * depth <= 16 )
				   {	dp = cmd_put_op(cldev, pcls, 2 + (tile_size >> 1));
					cmd_put_short_bits(dp + 2, new_data, tile->raster, 2, tile->size.y);
					count_add(cmd_tile_count, tile_size >> 1);
				   }
				else
				   {	dp = cmd_put_op(cldev, pcls, 2 + tile_size);
					memcpy(dp + 2, new_data, tile_size);
					count_add(cmd_tile_count, tile_size);
				   }
				count_op(*dp = (byte)cmd_opv_set_tile_bits);
				dp[1] = slot_index;
			   }
		   }
	   }
	pcls->tile = new_tile;
	return 0;
}
private int
clist_tile_rectangle(gx_device *dev, const gx_tile_bitmap *tile, int x, int y,
  int width, int height, gx_color_index color0, gx_color_index color1,
  int px, int py)
{	int depth =
	  (color1 == gx_no_color_index && color0 == gx_no_color_index ?
	   dev->color_info.depth : 1);
	fit_fill(dev, x, y, width, height);
	BEGIN_RECT
	if ( tile->id != pcls->tile->id )
	   {	if ( tile->id == gx_no_bitmap_id ||
		     clist_change_tile(cdev, pcls, tile, depth) < 0
		   )
		  return gx_default_tile_rectangle(dev, tile,
						   x, y, width, height,
						   color0, color1, px, py);
	   }
	if ( color0 != pcls->color0 || color1 != pcls->color1 )
		cmd_set_colors(dev, pcls, color0, color1);
	if ( px != pcls->tile_phase.x || py != pcls->tile_phase.y )
	   {	byte *dp = cmd_put_op(cdev, pcls, 1 + cmd_sizexy(pcls->tile_phase));
		count_op(*dp++ = (byte)cmd_opv_set_tile_phase);
		pcls->tile_phase.x = px;
		pcls->tile_phase.y = py;
		cmd_putxy(pcls->tile_phase, dp);
	   }
	cmd_write_rect_cmd(dev, pcls, cmd_op_tile_rect, x, y, width, height);
	END_RECT
	return 0;
}

private int
clist_copy_mono(gx_device *dev,
    const byte *data, int data_x, int raster, gx_bitmap_id id,
    int x, int y, int width, int height,
    gx_color_index color0, gx_color_index color1)
{	int y0;
	fit_copy(dev, data, data_x, raster, id, x, y, width, height);
	y0 = y;
	BEGIN_RECT
	gx_cmd_rect rect;
	int dx, row_bytes;
	uint dsize;
	int rsize;
	int bwidth;
	const byte *row = data + (y - y0) * raster;
	byte *dp;
	if ( color0 != pcls->color0 || color1 != pcls->color1 )
		cmd_set_colors(dev, pcls, color0, color1);
	cmd_set_rect(rect);
	rsize = cmd_size_rect(&rect);
	if ( width >= 2 && (bwidth = (width + (data_x & 7) + 7) >> 3) <= 3 &&
	     height <= 255 &&
	     height <= (cbuf_size - cmd_largest_size) / align_bitmap_mod
	   )
	   {	dsize = height * bwidth;
		dp = cmd_put_op(cdev, pcls, 1 + rsize + dsize);
		count_op(*dp++ = (byte)cmd_op_copy_mono + (data_x & 7) + 1);
		dp = cmd_put_rect(&rect, dp);
		row += data_x >> 3;
		cmd_put_short_bits(dp, row, raster, bwidth, height);
		pcls->rect = rect;
		count_add(cmd_copy_count, dsize);
	   }
	else
	   {	if ( height == 1 )
		  {	/* We don't need to write the entire row, */
			/* only the part that is being copied. */
			dx = data_x & 7;
			row_bytes = (uint)((data_x & 7) + width + 7) >> 3;
			dsize = row_bytes;
			row += data_x >> 3;
		  }
		else
		  {	dx = data_x;
			row_bytes = raster;
			dsize = height * raster;
		  }
		if ( dsize > cbuf_size )
		   {	/* We have to split it into pieces. */
			if ( height > 1 )
			   {	int h2 = height >> 1;
				clist_copy_mono(dev, row, data_x, raster,
					gx_no_bitmap_id, x, y, width, h2,
					color0, color1);
				clist_copy_mono(dev, row + h2 * raster,
					data_x, raster, gx_no_bitmap_id,
					x, y + h2, width, height - h2,
					color0, color1);
			   }
			else
			/* Split a single (very long) row. */
			   {	int w2 = width >> 1;
				clist_copy_mono(dev, row, dx, row_bytes,
					gx_no_bitmap_id, x, y, w2, 1,
					color0, color1);
				clist_copy_mono(dev, row, dx + w2,
					row_bytes, gx_no_bitmap_id, x + w2, y,
					width - w2, 1, color0, color1);
			   }
		   }
		else
		{	uint csize =
			  1 + rsize + cmd_sizew(dx) + cmd_sizew(row_bytes);
			dp = cmd_put_op(cdev, pcls, csize + dsize);
			/* See if compressing with RLE is worthwhile. */
			if ( dsize >= 50 )
			  {	stream_RLE_state sstate;
				stream_cursor_read r;
				byte *wbase = dp + (csize - 1);
				stream_cursor_write w;
				int status;
				uint wcount;
				sstate.record_size = 0;
				s_RLE_init_inline(&sstate);
				r.ptr = row - 1;
				r.limit = r.ptr + dsize;
				w.ptr = wbase;
				w.limit = w.ptr + dsize;
				status =
				  (*s_RLE_template.process)
				    ((stream_state *)&sstate, &r, &w, true);
				if ( status == 0 && (wcount = w.ptr - wbase) <= dsize >> 1 )
				  {	/* Use compressed representation. */
					cmd_shorten_op(cdev, pcls, dsize - wcount);
					count_op(*dp++ = (byte)cmd_op_copy_mono_rle);
					count_add(cmd_copy_rle_count, wcount);
					goto out;
				  }
			  }
			memcpy(dp + csize, row, dsize);
			count_op(*dp++ = (byte)cmd_op_copy_mono);
			count_add(cmd_copy_count, dsize);
out:			dp = cmd_put_rect(&rect, dp);
			cmd_putw(dx, dp);
			cmd_putw(row_bytes, dp);
			pcls->rect = rect;
		}
	   }
	END_RECT
	return 0;
}

private int
clist_copy_color(gx_device *dev,
    const byte *data, int data_x, int raster, gx_bitmap_id id,
    int x, int y, int width, int height)
{	int y0;
	fit_copy(dev, data, data_x, raster, id, x, y, width, height);
	y0 = y;
	BEGIN_RECT
	int dx, row_bytes;
	uint dsize;
	const byte *row = data + (y - y0) * raster;
	if ( height == 1 )
	  {	/* We don't need to write the entire row, */
		/* only the part that is being copied. */
		int depth = dev->color_info.depth;
		uint dbit = data_x * depth;
		dx = (dbit & 7) / depth;
		row_bytes = (uint)((dbit & 7) + (width * depth) + 7) >> 3;
		dsize = row_bytes;
		row += dbit >> 3;
	  }
	else
	  {	dx = data_x;
		row_bytes = raster;
		dsize = height * raster;
	  }
	if ( dsize > cbuf_size )
	   {	/* We have to split it into pieces. */
		if ( height > 1 )
		   {	int h2 = height >> 1;
			clist_copy_color(dev, row, data_x, raster,
					 gx_no_bitmap_id, x, y, width, h2);
			clist_copy_color(dev, row + h2 * raster,
					 data_x, raster,
					 gx_no_bitmap_id, x, y + h2,
					 width, height - h2);
		   }
		else
		   {	/* Split a single (very long) row. */
			int w2 = width >> 1;
			clist_copy_color(dev, row, dx, row_bytes,
					 gx_no_bitmap_id, x, y, w2, 1);
			clist_copy_color(dev, row, dx + w2, row_bytes,
					 gx_no_bitmap_id, x + w2, y,
					 width - w2, 1);
		   }
	   }
	else
	  {	gx_cmd_rect rect;
		byte *dp;
		cmd_set_rect(rect);
		dp = cmd_put_op(cdev, pcls, 1 + cmd_size_rect(&rect) + cmd_sizew(dx) + cmd_sizew(row_bytes) + dsize);
		count_op(*dp++ = (byte)cmd_op_copy_color);
		dp = cmd_put_rect(&rect, dp);
		pcls->rect = rect;
		cmd_putw(dx, dp);
		cmd_putw(row_bytes, dp);
		memcpy(dp, row, dsize);
		count_add(cmd_copy_count, dsize);
	  }
	END_RECT
	return 0;
}
