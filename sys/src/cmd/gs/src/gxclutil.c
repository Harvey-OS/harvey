/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclutil.c,v 1.12 2005/03/14 18:08:36 dan Exp $ */
/* Command list writing utilities. */

#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* must precede gxcldev.h */
#include "gxcldev.h"
#include "gxclpath.h"
#include "gsparams.h"

/* ---------------- Statistics ---------------- */

#ifdef DEBUG
const char *const cmd_op_names[16] =
{cmd_op_name_strings};
private const char *const cmd_misc_op_names[16] =
{cmd_misc_op_name_strings};
private const char *const cmd_misc2_op_names[16] =
{cmd_misc2_op_name_strings};
private const char *const cmd_segment_op_names[16] =
{cmd_segment_op_name_strings};
private const char *const cmd_path_op_names[16] =
{cmd_path_op_name_strings};
const char *const *const cmd_sub_op_names[16] =
{cmd_misc_op_names, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0,
 0, cmd_misc2_op_names, cmd_segment_op_names, cmd_path_op_names
};
struct stats_cmd_s {
    ulong op_counts[256];
    ulong op_sizes[256];
    ulong tile_reset, tile_found, tile_added;
    ulong same_band, other_band;
} stats_cmd;
extern ulong stats_cmd_diffs[5];	/* in gxclpath.c */
int
cmd_count_op(int op, uint size)
{
    stats_cmd.op_counts[op]++;
    stats_cmd.op_sizes[op] += size;
    if (gs_debug_c('L')) {
	const char *const *sub = cmd_sub_op_names[op >> 4];

	if (sub)
	    dlprintf2(", %s(%u)\n", sub[op & 0xf], size);
	else
	    dlprintf3(", %s %d(%u)\n", cmd_op_names[op >> 4], op & 0xf,
		      size);
	dflush();
    }
    return op;
}
void
cmd_uncount_op(int op, uint size)
{
    stats_cmd.op_counts[op]--;
    stats_cmd.op_sizes[op] -= size;
}
#endif

/* Print statistics. */
#ifdef DEBUG
void
cmd_print_stats(void)
{
    int ci, cj;

    dlprintf3("[l]counts: reset = %lu, found = %lu, added = %lu\n",
	      stats_cmd.tile_reset, stats_cmd.tile_found,
	      stats_cmd.tile_added);
    dlprintf5("     diff 2.5 = %lu, 3 = %lu, 4 = %lu, 2 = %lu, >4 = %lu\n",
	      stats_cmd_diffs[0], stats_cmd_diffs[1], stats_cmd_diffs[2],
	      stats_cmd_diffs[3], stats_cmd_diffs[4]);
    dlprintf2("     same_band = %lu, other_band = %lu\n",
	      stats_cmd.same_band, stats_cmd.other_band);
    for (ci = 0; ci < 0x100; ci += 0x10) {
	const char *const *sub = cmd_sub_op_names[ci >> 4];

	if (sub != 0) {
	    dlprintf1("[l]  %s =", cmd_op_names[ci >> 4]);
	    for (cj = ci; cj < ci + 0x10; cj += 2)
		dprintf6("\n\t%s = %lu(%lu), %s = %lu(%lu)",
			 sub[cj - ci],
			 stats_cmd.op_counts[cj], stats_cmd.op_sizes[cj],
			 sub[cj - ci + 1],
		   stats_cmd.op_counts[cj + 1], stats_cmd.op_sizes[cj + 1]);
	} else {
	    ulong tcounts = 0, tsizes = 0;

	    for (cj = ci; cj < ci + 0x10; cj++)
		tcounts += stats_cmd.op_counts[cj],
		    tsizes += stats_cmd.op_sizes[cj];
	    dlprintf3("[l]  %s (%lu,%lu) =\n\t",
		      cmd_op_names[ci >> 4], tcounts, tsizes);
	    for (cj = ci; cj < ci + 0x10; cj++)
		if (stats_cmd.op_counts[cj] == 0)
		    dputs(" -");
		else
		    dprintf2(" %lu(%lu)", stats_cmd.op_counts[cj],
			     stats_cmd.op_sizes[cj]);
	}
	dputs("\n");
    }
}
#endif /* DEBUG */

/* ---------------- Writing utilities ---------------- */

/* Write the commands for one band or band range. */
private int	/* ret 0 all ok, -ve error code, or +1 ok w/low-mem warning */
cmd_write_band(gx_device_clist_writer * cldev, int band_min, int band_max,
	       cmd_list * pcl, byte cmd_end)
{
    const cmd_prefix *cp = pcl->head;
    int code_b = 0;
    int code_c = 0;

    if (cp != 0 || cmd_end != cmd_opv_end_run) {
	clist_file_ptr cfile = cldev->page_cfile;
	clist_file_ptr bfile = cldev->page_bfile;
	cmd_block cb;
	byte end = cmd_count_op(cmd_end, 1);

	if (cfile == 0 || bfile == 0)
 	    return_error(gs_error_ioerror);
	cb.band_min = band_min;
	cb.band_max = band_max;
	cb.pos = clist_ftell(cfile);
	if_debug3('l', "[l]writing for bands (%d,%d) at %ld\n",
		  band_min, band_max, cb.pos);
	clist_fwrite_chars(&cb, sizeof(cb), bfile);
	if (cp != 0) {
	    pcl->tail->next = 0;	/* terminate the list */
	    for (; cp != 0; cp = cp->next) {
#ifdef DEBUG
		if ((const byte *)cp < cldev->cbuf ||
		    (const byte *)cp >= cldev->cend ||
		    cp->size > cldev->cend - (const byte *)cp
		    ) {
		    lprintf1("cmd_write_band error at 0x%lx\n", (ulong) cp);
		    return_error(gs_error_Fatal);
		}
#endif
		clist_fwrite_chars(cp + 1, cp->size, cfile);
	    }
	    pcl->head = pcl->tail = 0;
	}
	clist_fwrite_chars(&end, 1, cfile);
	process_interrupts(cldev->memory);
	code_b = clist_ferror_code(bfile);
	code_c = clist_ferror_code(cfile);
	if (code_b < 0)
	    return_error(code_b);
	if (code_c < 0)
	    return_error(code_c); 
    }
    return code_b | code_c;
}

/* Write out the buffered commands, and reset the buffer. */
int	/* ret 0 all-ok, -ve error code, or +1 ok w/low-mem warning */
cmd_write_buffer(gx_device_clist_writer * cldev, byte cmd_end)
{
    int nbands = cldev->nbands;
    gx_clist_state *pcls;
    int band;
    int code = cmd_write_band(cldev, cldev->band_range_min,
			      cldev->band_range_max,
			      &cldev->band_range_list, cmd_opv_end_run);
    int warning = code;

    for (band = 0, pcls = cldev->states;
	 code >= 0 && band < nbands; band++, pcls++
	 ) {
	code = cmd_write_band(cldev, band, band, &pcls->list, cmd_end);
	warning |= code;
    }
    /* If an error occurred, finish cleaning up the pointers. */
    for (; band < nbands; band++, pcls++)
	pcls->list.head = pcls->list.tail = 0;
    cldev->cnext = cldev->cbuf;
    cldev->ccl = 0;
#ifdef DEBUG
    if (gs_debug_c('l'))
	cmd_print_stats();
#endif
    return_check_interrupt(cldev->memory, code != 0 ? code : warning);
}

/*
 * Add a command to the appropriate band list, and allocate space for its
 * data.  Return the pointer to the data area.  If an error or (low-memory
 * warning) occurs, set cldev->error_code and return 0.
 */
#define cmd_headroom (sizeof(cmd_prefix) + arch_align_ptr_mod)
byte *
cmd_put_list_op(gx_device_clist_writer * cldev, cmd_list * pcl, uint size)
{
    byte *dp = cldev->cnext;

    if (size + cmd_headroom > cldev->cend - dp) {
	if ((cldev->error_code =
	       cmd_write_buffer(cldev, cmd_opv_end_run)) != 0) {
	    if (cldev->error_code < 0)
		cldev->error_is_retryable = 0;	/* hard error */
	    else {
		/* upgrade lo-mem warning into an error */
		if (!cldev->ignore_lo_mem_warnings)
		    cldev->error_code = gs_note_error(gs_error_VMerror);
		cldev->error_is_retryable = 1;
	    }
	    return 0;
	}
	else
	    return cmd_put_list_op(cldev, pcl, size);
    }
    if (cldev->ccl == pcl) {	/* We're adding another command for the same band. */
	/* Tack it onto the end of the previous one. */
	cmd_count_add1(stats_cmd.same_band);
#ifdef DEBUG
	if (pcl->tail->size > dp - (byte *) (pcl->tail + 1)) {
	    lprintf1("cmd_put_list_op error at 0x%lx\n", (ulong) pcl->tail);
	}
#endif
	pcl->tail->size += size;
    } else {
	/* Skip to an appropriate alignment boundary. */
	/* (We assume the command buffer itself is aligned.) */
	cmd_prefix *cp = (cmd_prefix *)
	    (dp + ((cldev->cbuf - dp) & (arch_align_ptr_mod - 1)));

	cmd_count_add1(stats_cmd.other_band);
	dp = (byte *) (cp + 1);
	if (pcl->tail != 0) {
#ifdef DEBUG
	    if (pcl->tail < pcl->head ||
		pcl->tail->size > dp - (byte *) (pcl->tail + 1)
		) {
		lprintf1("cmd_put_list_op error at 0x%lx\n",
			 (ulong) pcl->tail);
	    }
#endif
	    pcl->tail->next = cp;
	} else
	    pcl->head = cp;
	pcl->tail = cp;
	cldev->ccl = pcl;
	cp->size = size;
    }
    cldev->cnext = dp + size;
    return dp;
}
#ifdef DEBUG
byte *
cmd_put_op(gx_device_clist_writer * cldev, gx_clist_state * pcls, uint size)
{
    if_debug3('L', "[L]band %d: size=%u, left=%u",
	      (int)(pcls - cldev->states),
	      size, 0);
    return cmd_put_list_op(cldev, &pcls->list, size);
}
#endif

/* Add a command for a range of bands. */
byte *
cmd_put_range_op(gx_device_clist_writer * cldev, int band_min, int band_max,
		 uint size)
{
    if_debug4('L', "[L]band range(%d,%d): size=%u, left=%u",
	      band_min, band_max, size, 0);
    if (cldev->ccl != 0 && 
	(cldev->ccl != &cldev->band_range_list ||
	 band_min != cldev->band_range_min ||
	 band_max != cldev->band_range_max)
	) {
	if ((cldev->error_code = cmd_write_buffer(cldev, cmd_opv_end_run)) != 0) {
	    if (cldev->error_code < 0)
		cldev->error_is_retryable = 0;	/* hard error */
	    else {
		/* upgrade lo-mem warning into an error */
		cldev->error_code = gs_error_VMerror;
		cldev->error_is_retryable = 1;
	    }
	    return 0;
	}
	cldev->band_range_min = band_min;
	cldev->band_range_max = band_max;
    }
    return cmd_put_list_op(cldev, &cldev->band_range_list, size);
}

/* Write a variable-size positive integer. */
int
cmd_size_w(register uint w)
{
    register int size = 1;

    while (w > 0x7f)
	w >>= 7, size++;
    return size;
}
byte *
cmd_put_w(register uint w, register byte * dp)
{
    while (w > 0x7f)
	*dp++ = w | 0x80, w >>= 7;
    *dp = w;
    return dp + 1;
}


/*
 * This next two arrays are used for the 'delta' mode of placing a color
 * in the clist.  These arrays are indexed by the number of bytes in the
 * color value (the depth).
 *
 * Delta values are calculated by subtracting the old value for the color
 * from the desired new value.  Then each byte of the differenece is
 * examined.  For most bytes, if the difference fits into 4 bits (signed)
 * then those bits are packed into the clist along with an opcode.  If
 * the size of the color (the depth) is an odd number of bytes then instead
 * of four bits per byte, extra bits are used for the upper three bytes
 * of the color.  In this case, five bits are used for the first byte,
 * six bits for the second byte, and five bits for third byte.  This
 * maximizes the chance that the 'delta' mode can be used for placing
 * colors in the clist.
 */
/*
 * Depending upon the compiler and user choices, the size of a gx_color_index
 * may be 4 to 8 bytes.  We will define table entries for up to 8 bytes.
 * This macro is being used to prevent compiler warnings if gx_color_index is
 * only 4 bytes.
 */
#define tab_entry(x) ((x) & (~((gx_color_index) 0)))

const gx_color_index cmd_delta_offsets[] = {
	tab_entry(0),
	tab_entry(0),
	tab_entry(0x0808),
	tab_entry(0x102010),
	tab_entry(0x08080808),
	tab_entry(0x1020100808),
	tab_entry(0x080808080808),
	tab_entry(0x10201008080808),
	tab_entry(0x0808080808080808),
	};

private const gx_color_index cmd_delta_masks[] = {
	tab_entry(0),
	tab_entry(0),
	tab_entry(0x0f0f),
	tab_entry(0x1f3f1f),
	tab_entry(0x0f0f0f0f),
	tab_entry(0x1f3f1f0f0f),
	tab_entry(0x0f0f0f0f0f0f),
	tab_entry(0x1f3f1f0f0f0f0f),
	tab_entry(0x0f0f0f0f0f0f0f0f),
	};

#undef tab_entry

/*
 * There are currently only four different color "types" that can be placed
 * into the clist.  These are called "color0", "color1", and "tile_color0",
 * and "tile_color1".  There are separate command codes for color0 versus
 * color1, both for the full value and delta commands - see cmd_put_color.
 * Tile colors are preceded by a cmd_opv_set_tile_color command.
 */
const clist_select_color_t
    clist_select_color0 = {cmd_op_set_color0, cmd_opv_delta_color0, 0},
    clist_select_color1 = {cmd_op_set_color1, cmd_opv_delta_color1, 0},
    clist_select_tile_color0 = {cmd_op_set_color0, cmd_opv_delta_color0, 1},
    clist_select_tile_color1 = {cmd_op_set_color1, cmd_opv_delta_color1, 1};

/*
 * This routine is used to place a color into the clist.  Colors, in the
 * clist, can be specified either as by a full value or by a "delta" value.
 *
 * See the comments before cmd_delta_offsets[] for a description of the
 * 'delta' mode.  The delta mode may allow for a smaller command in the clist.
 *
 * For the full value mode, values are sent as a cmd code plus n bytes of
 * data.  To minimize the number of bytes, a count is made of any low order
 * bytes which are zero.  This count is packed into the low order 4 bits
 * of the cmd code.  The data for these bytes are not sent.
 *
 * The gx_no_color_index value is treated as a special case.  This is done
 * because it is both a commonly sent value and because it may require
 * more bytes then the other color values.
 *
 * Parameters:
 *   cldev - Pointer to clist device
 *   pcls - Pointer to clist state
 *   select - Descriptor record for type of color being sent.  See comments
 *       by clist_select_color_t.
 *   color - The new color value.
 *   pcolor - Pointer to previous color value.  (If the color value is the
 *       same as the previous value then nothing is placed into the clist.)
 *
 * Returns:
 *   Error code
 *   clist and pcls and cldev may be updated.
 */
int
cmd_put_color(gx_device_clist_writer * cldev, gx_clist_state * pcls,
	      const clist_select_color_t * select,
	      gx_color_index color, gx_color_index * pcolor)
{
    byte * dp;		/* This is manipulated by the set_cmd_put_op macro */
    gx_color_index diff = color - *pcolor;
    byte op, op_delta;
    int code;

    if (diff == 0)
	return 0;

    /* If this is a tile color then send tile color type */
    if (select->tile_color) {
	code = set_cmd_put_op(dp, cldev, pcls, cmd_opv_set_tile_color, 1);
	if (code < 0)
	    return code;
    }
    op = select->set_op;
    op_delta = select->delta_op;
    if (color == gx_no_color_index) {
	/*
	 * We must handle this specially, because it may take more
	 * bytes than the color depth.
	 */
	code = set_cmd_put_op(dp, cldev, pcls, op + cmd_no_color_index, 1);
	if (code < 0)
	    return code;
    } else {
	/* Check if the "delta" mode command can be used. */
	int num_bytes = (cldev->color_info.depth + 7) >> 3;
	int delta_bytes = (num_bytes + 1) / 2;
	gx_color_index delta_offset = cmd_delta_offsets[num_bytes];
	gx_color_index delta_mask = cmd_delta_masks[num_bytes];
	gx_color_index delta = (diff + delta_offset) & delta_mask;
	bool use_delta = (color == (*pcolor + delta - delta_offset));
	int bytes_dropped = 0;
	gx_color_index data = color;
	
	/*
	 * If we use the full value mode, we do not send low order bytes
	 * which are zero. Determine how many low order bytes are zero.
	 */
	if (color == 0) {
	    bytes_dropped = num_bytes;
	}
	else  {
	    while ((data & 0xff) == 0) {
	        bytes_dropped++;
		data >>= 8; 
	    }
	}
	
	/* Now send one of the two command forms */
	if (use_delta && delta_bytes < num_bytes - bytes_dropped) {
	    code = set_cmd_put_op(dp, cldev, pcls,
	    				op_delta, delta_bytes + 1);
	    if (code < 0)
	        return code;
	    /*
	     * If we have an odd number of bytes then use extra bits for
	     * the high order three bytes of the color.
	     */
	    if ((num_bytes >= 3) && (num_bytes & 1)) {
		data = delta >> ((num_bytes - 3) * 8);
	        dp[delta_bytes--] = (byte)(((data >> 13) & 0xf8) + ((data >> 11) & 0x07));
	        dp[delta_bytes--] = (byte)(((data >> 3) & 0xe0) + (data & 0x1f));
	    }
	    for(; delta_bytes>0; delta_bytes--) {
	        dp[delta_bytes] = (byte)((delta >> 4) + delta);
		delta >>= 16;
	    }
	}
	else {
	    num_bytes -= bytes_dropped;
	    code = set_cmd_put_op(dp, cldev, pcls,
	    			(byte)(op + bytes_dropped), num_bytes + 1);
	    if (code < 0)
	        return code;
	    for(; num_bytes>0; num_bytes--) {
	        dp[num_bytes] = (byte)data;
		data >>= 8;
	    }
	}
    }
    *pcolor = color;
    return 0;
}


/* Put out a command to set the tile colors. */
int
cmd_set_tile_colors(gx_device_clist_writer * cldev, gx_clist_state * pcls,
		    gx_color_index color0, gx_color_index color1)
{
    int code = 0;

    if (color0 != pcls->tile_colors[0]) {
	code = cmd_put_color(cldev, pcls,
			     &clist_select_tile_color0,
			     color0, &pcls->tile_colors[0]);
	if (code != 0)
	    return code;
    }
    if (color1 != pcls->tile_colors[1])
	code = cmd_put_color(cldev, pcls,
			     &clist_select_tile_color1,
			     color1, &pcls->tile_colors[1]);
    return code;
}

/* Put out a command to set the tile phase. */
int
cmd_set_tile_phase(gx_device_clist_writer * cldev, gx_clist_state * pcls,
		   int px, int py)
{
    int pcsize;
    byte *dp;
    int code;

    pcsize = 1 + cmd_size2w(px, py);
    code =
	set_cmd_put_op(dp, cldev, pcls, (byte)cmd_opv_set_tile_phase, pcsize);
    if (code < 0)
	return code;
    ++dp;
    pcls->tile_phase.x = px;
    pcls->tile_phase.y = py;
    cmd_putxy(pcls->tile_phase, dp);
    return 0;
}

/* Write a command to enable or disable the logical operation. */
int
cmd_put_enable_lop(gx_device_clist_writer * cldev, gx_clist_state * pcls,
		   int enable)
{
    byte *dp;
    int code = set_cmd_put_op(dp, cldev, pcls,
			      (byte)(enable ? cmd_opv_enable_lop :
				     cmd_opv_disable_lop),
			      1);

    if (code < 0)
	return code;
    pcls->lop_enabled = enable;
    return 0;
}

/* Write a command to enable or disable clipping. */
/* This routine is only called if the path extensions are included. */
int
cmd_put_enable_clip(gx_device_clist_writer * cldev, gx_clist_state * pcls,
		    int enable)
{
    byte *dp;
    int code = set_cmd_put_op(dp, cldev, pcls,
			      (byte)(enable ? cmd_opv_enable_clip :
				     cmd_opv_disable_clip),
			      1);

    if (code < 0)
	return code;
    pcls->clip_enabled = enable;
    return 0;
}

/* Write a command to set the logical operation. */
int
cmd_set_lop(gx_device_clist_writer * cldev, gx_clist_state * pcls,
	    gs_logical_operation_t lop)
{
    byte *dp;
    uint lop_msb = lop >> 6;
    int code = set_cmd_put_op(dp, cldev, pcls,
			      cmd_opv_set_misc, 2 + cmd_size_w(lop_msb));

    if (code < 0)
	return code;
    dp[1] = cmd_set_misc_lop + (lop & 0x3f);
    cmd_put_w(lop_msb, dp + 2);
    pcls->lop = lop;
    return 0;
}

/* Disable (if default) or enable the logical operation, setting it if */
/* needed. */
int
cmd_update_lop(gx_device_clist_writer *cldev, gx_clist_state *pcls,
	       gs_logical_operation_t lop)
{
    int code;

    if (lop == lop_default)
	return cmd_disable_lop(cldev, pcls);
    code = cmd_set_lop(cldev, pcls, lop);
    if (code < 0)
	return code;
    return cmd_enable_lop(cldev, pcls);
}

/* Write a parameter list */
int	/* ret 0 all ok, -ve error */
cmd_put_params(gx_device_clist_writer *cldev,
	       gs_param_list *param_list) /* NB open for READ */
{
    byte *dp;
    int code;
    byte local_buf[512];	/* arbitrary */
    int param_length;

    /* Get serialized list's length + try to get it into local var if it fits. */
    param_length = code =
	gs_param_list_serialize(param_list, local_buf, sizeof(local_buf));
    if (param_length > 0) {
	/* Get cmd buffer space for serialized */
	code = set_cmd_put_all_op(dp, cldev, cmd_opv_extend,
				  2 + sizeof(unsigned) + param_length);
	if (code < 0)
	    return code;

	/* write param list to cmd list: needs to all fit in cmd buffer */
	if_debug1('l', "[l]put_params, length=%d\n", param_length);
	dp[1] = cmd_opv_ext_put_params;
	dp += 2;
	memcpy(dp, &param_length, sizeof(unsigned));
	dp += sizeof(unsigned);
	if (param_length > sizeof(local_buf)) {
	    int old_param_length = param_length;

	    param_length = code =
		gs_param_list_serialize(param_list, dp, old_param_length);
	    if (param_length >= 0)
		code = (old_param_length != param_length ?
			gs_note_error(gs_error_unknownerror) : 0);
	    if (code < 0) {
		/* error serializing: back out by writing a 0-length parm list */
		memset(dp - sizeof(unsigned), 0, sizeof(unsigned));
		cmd_shorten_list_op(cldev, &cldev->band_range_list,
				    old_param_length);
	    }
	} else
	    memcpy(dp, local_buf, param_length);	    /* did this when computing length */
    }
    return code;
}

/* Initialize CCITTFax filters. */
private void
clist_cf_init(stream_CF_state *ss, int width)
{
    ss->K = -1;
    ss->Columns = width;
    ss->EndOfBlock = false;
    ss->BlackIs1 = true;
    ss->DecodedByteAlign = align_bitmap_mod;
}
void
clist_cfe_init(stream_CFE_state *ss, int width, gs_memory_t *mem)
{
    s_init_state((stream_state *)ss, &s_CFE_template, mem);
    s_CFE_set_defaults_inline(ss);
    clist_cf_init((stream_CF_state *)ss, width);
    s_CFE_template.init((stream_state *)(ss));
}
void
clist_cfd_init(stream_CFD_state *ss, int width, int height, gs_memory_t *mem)
{
    s_init_state((stream_state *)ss, &s_CFD_template, mem);
    s_CFD_template.set_defaults((stream_state *)ss);
    clist_cf_init((stream_CF_state *)ss, width);
    ss->Rows = height;
    s_CFD_template.init((stream_state *)(ss));
}

/* Initialize RunLength filters. */
void
clist_rle_init(stream_RLE_state *ss)
{
    s_init_state((stream_state *)ss, &s_RLE_template, (gs_memory_t *)0);
    s_RLE_set_defaults_inline(ss);
    s_RLE_init_inline(ss);
}
void
clist_rld_init(stream_RLD_state *ss)
{
    s_init_state((stream_state *)ss, &s_RLD_template, (gs_memory_t *)0);
    s_RLD_set_defaults_inline(ss);
    s_RLD_init_inline(ss);
}
