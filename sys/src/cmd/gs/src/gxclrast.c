/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxclrast.c,v 1.8 2000/09/19 19:00:35 lpd Exp $ */
/* Command list interpreter/rasterizer */
#include "memory_.h"
#include "gx.h"
#include "gp.h"			/* for gp_fmode_rb */
#include "gpcheck.h"
#include "gscdefs.h"		/* for image type table */
#include "gserrors.h"
#include "gsbitops.h"
#include "gsparams.h"
#include "gsstate.h"		/* (should only be imager state) */
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gscoord.h"		/* requires gsmatrix.h */
#include "gsdevice.h"		/* for gs_deviceinitialmatrix */
#include "gsiparm4.h"
#include "gxdevmem.h"		/* must precede gxcldev.h */
#include "gxcldev.h"
#include "gxclpath.h"
#include "gxcmap.h"
#include "gxcolor2.h"
#include "gxcspace.h"		/* for gs_color_space_type */
#include "gxdhtres.h"
#include "gxgetbit.h"
#include "gxpaint.h"		/* for gx_fill/stroke_params */
#include "gxhttile.h"
#include "gxiparam.h"
#include "gdevht.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gzacpath.h"
#include "stream.h"
#include "strimpl.h"

extern_gx_device_halftone_list();
extern_gx_image_type_table();

/* We need color space types for constructing temporary color spaces. */
extern const gs_color_space_type gs_color_space_type_Indexed;

/* Print a bitmap for tracing */
#ifdef DEBUG
private void
cmd_print_bits(const byte * data, int width, int height, int raster)
{
    int i, j;

    dlprintf3("[L]width=%d, height=%d, raster=%d\n",
	      width, height, raster);
    for (i = 0; i < height; i++) {
	const byte *row = data + i * raster;

	dlprintf("[L]");
	for (j = 0; j < raster; j++)
	    dprintf1(" %02x", row[j]);
	dputc('\n');
    }
}
#else
#  define cmd_print_bits(data, width, height, raster) DO_NOTHING
#endif

/* Get a variable-length integer operand. */
#define cmd_getw(var, p)\
  BEGIN\
    if ( *p < 0x80 ) var = *p++;\
    else { const byte *_cbp; var = cmd_get_w(p, &_cbp); p = _cbp; }\
  END
private long
cmd_get_w(const byte * p, const byte ** rp)
{
    long val = *p++ & 0x7f;
    int shift = 7;

    for (; val += (long)(*p & 0x7f) << shift, *p++ > 0x7f; shift += 7);
    *rp = p;
    return val;
}

/*
 * Define the structure for keeping track of the command reading buffer.
 *
 * The ptr member is only used for passing the current pointer to, and
 * receiving an updated pointer from, commands implemented as separate
 * procedures: normally it is kept in a register.
 */
typedef struct command_buf_s {
    byte *data;			/* actual buffer, guaranteed aligned */
    uint size;
    const byte *ptr;		/* next byte to be read (see above) */
    const byte *limit;		/* refill warning point */
    const byte *end;		/* byte just beyond valid data */
    stream *s;			/* for refilling buffer */
    int end_status;
} command_buf_t;

/* Set the end of a command buffer. */
private void
set_cb_end(command_buf_t *pcb, const byte *end)
{
    pcb->end = end;
    pcb->limit = pcb->data + (pcb->size - cmd_largest_size + 1);
    if ( pcb->limit > pcb->end )
	pcb->limit = pcb->end;
}

/* Read more data into a command buffer. */
private const byte *
top_up_cbuf(command_buf_t *pcb, const byte *cbp)
{
    uint nread;
    byte *cb_top = pcb->data + (pcb->end - cbp);

    memmove(pcb->data, cbp, pcb->end - cbp);
    nread = pcb->end - cb_top;
    pcb->end_status = sgets(pcb->s, cb_top, nread, &nread);
    if ( nread == 0 ) {
	/* No data for this band at all. */
	*cb_top = cmd_opv_end_run;
	nread = 1;
    }
    set_cb_end(pcb, cb_top + nread);
    process_interrupts();
    return pcb->data;
}

/* Read data from the command buffer and stream. */
private const byte *
cmd_read_data(command_buf_t *pcb, byte *ptr, uint rsize, const byte *cbp)
{
    if (pcb->end - cbp >= rsize) {
	memcpy(ptr, cbp, rsize);
	return cbp + rsize;
    } else {
	uint cleft = pcb->end - cbp;
	uint rleft = rsize - cleft;

	memcpy(ptr, cbp, cleft);
	sgets(pcb->s, ptr + cleft, rleft, &rleft);
	return pcb->end;
    }
}
#define cmd_read(ptr, rsize, cbp)\
  cbp = cmd_read_data(&cbuf, ptr, rsize, cbp)

/* Read a fixed-size value from the command buffer. */
inline private const byte *
cmd_copy_value(void *pvar, int var_size, const byte *cbp)
{
    memcpy(pvar, cbp, var_size);
    return cbp + var_size;
}
#define cmd_get_value(var, cbp)\
  cbp = cmd_copy_value(&var, sizeof(var), cbp)

/*
 * Render one band to a specified target device.  Note that if
 * action == setup, target may be 0.
 */
private int read_set_tile_size(P2(command_buf_t *pcb, tile_slot *bits));
private int read_set_bits(P8(command_buf_t *pcb, tile_slot *bits,
			     int compress, gx_clist_state *pcls,
			     gx_strip_bitmap *tile, tile_slot **pslot,
			     gx_device_clist_reader *cdev, gs_memory_t *mem));
private int read_set_ht_order(P4(command_buf_t *pcb,
				 gx_device_halftone **ppdht,
				 gx_ht_order **pporder, gs_memory_t *mem));
private int read_set_ht_data(P7(command_buf_t *pcb, uint *pdata_index,
				gx_ht_order *porder,
				gs_halftone_type halftone_type,
				gs_imager_state *pis,
				gx_device_clist_reader *cdev,
				gs_memory_t *mem));
private int read_set_misc2(P3(command_buf_t *pcb, gs_imager_state *pis,
			      segment_notes *pnotes));
private int read_set_color_space(P5(command_buf_t *pcb, gs_imager_state *pis,
				    const gs_color_space **ppcs,
				    gs_color_space *pcolor_space,
				    gs_memory_t *mem));
private int read_begin_image(P3(command_buf_t *pcb, gs_image_common_t *pic,
				const gs_color_space *pcs));
private int read_put_params(P4(command_buf_t *pcb, gs_imager_state *pis,
			       gx_device_clist_reader *cdev,
			       gs_memory_t *mem));

private const byte *cmd_read_rect(P3(int, gx_cmd_rect *, const byte *));
private const byte *cmd_read_matrix(P2(gs_matrix *, const byte *));
private const byte *cmd_read_short_bits(P6(command_buf_t *pcb, byte *data,
					   int width_bytes, int height,
					   uint raster, const byte *cbp));
private int cmd_select_map(P7(cmd_map_index, cmd_map_contents,
			      gs_imager_state *, gx_ht_order *, frac **,
			      uint *, gs_memory_t *));
private int cmd_create_dev_ht(P2(gx_device_halftone **, gs_memory_t *));
private int cmd_resize_halftone(P3(gx_device_halftone **, uint,
				   gs_memory_t *));
private int clist_decode_segment(P7(gx_path *, int, fixed[6],
				    gs_fixed_point *, int, int,
				    segment_notes));
private int clist_do_polyfill(P4(gx_device *, gx_path *,
				 const gx_drawing_color *,
				 gs_logical_operation_t));

int
clist_playback_band(clist_playback_action playback_action,
		    gx_device_clist_reader *cdev, stream *s,
		    gx_device *target, int x0, int y0, gs_memory_t * mem)
{
    /* cbuf must be maximally aligned, but still be a byte *. */
    typedef union { void *p; double d; long l; } aligner_t;
    aligner_t cbuf_storage[cbuf_size / sizeof(aligner_t) + 1];
    command_buf_t cbuf;
    /* data_bits is for short copy_* bits and copy_* compressed, */
    /* must be aligned */
#define data_bits_size cbuf_size
    byte *data_bits = 0;
    register const byte *cbp;
    int dev_depth = cdev->color_info.depth;
    int dev_depth_bytes = (dev_depth + 7) >> 3;
    gx_device *tdev;
    gx_clist_state state;
    gx_color_index *set_colors;
    tile_slot *state_slot;
    gx_strip_bitmap state_tile;	/* parameters for reading tiles */
    tile_slot tile_bits;	/* parameters of current tile */
    gs_int_point tile_phase, color_phase;
    gx_path path;
    bool in_path;
    gs_fixed_point ppos;
    gx_clip_path clip_path;
    bool use_clip;
    gx_clip_path *pcpath;
    gx_device_cpath_accum clip_accum;
    gs_fixed_rect target_box;
    struct _cas {
	bool lop_enabled;
	gs_fixed_point fill_adjust;
    } clip_save;
    gs_imager_state imager_state;
    gx_device_color dev_color;
    float dash_pattern[cmd_max_dash];
    gx_fill_params fill_params;
    gx_stroke_params stroke_params;
    gs_halftone_type halftone_type;
    gx_ht_order *porder;
    uint ht_data_index;
    union im_ {
	gs_image_common_t c;
	gs_data_image_t d;
	gs_image1_t i1;
	gs_image4_t i4;
    } image;
    gs_int_rect image_rect;
    gs_color_space color_space;	/* only used for indexed spaces */
    const gs_color_space *pcs;
    gx_image_enum_common_t *image_info;
    gx_image_plane_t planes[32];
    uint data_height;
    uint data_size;
    byte *data_on_heap;
    fixed vs[6];
    segment_notes notes;
    int data_x;
    int code = 0;

    cbuf.data = (byte *)cbuf_storage;
    cbuf.size = cbuf_size;
    cbuf.s = s;
    cbuf.end_status = 0;
    set_cb_end(&cbuf, cbuf.data + cbuf.size);
    cbp = cbuf.end;
in:				/* Initialize for a new page. */
    tdev = target;
    set_colors = state.colors;
    use_clip = false;
    pcpath = NULL;
    notes = sn_none;
    data_x = 0;
    {
	static const gx_clist_state cls_initial = { cls_initial_values };

	state = cls_initial;
    }
    state_tile.id = gx_no_bitmap_id;
    state_tile.shift = state_tile.rep_shift = 0;
    tile_phase.x = color_phase.x = x0;
    tile_phase.y = color_phase.y = y0;
    gx_path_init_local(&path, mem);
    in_path = false;
    /*
     * Initialize the clipping region to the full page.
     * (Since we also initialize use_clip to false, this is arbitrary.)
     */
    {
	gs_fixed_rect cbox;

	gx_cpath_init_local(&clip_path, mem);
	cbox.p.x = 0;
	cbox.p.y = 0;
	cbox.q.x = cdev->width;
	cbox.q.y = cdev->height;
	gx_cpath_from_rectangle(&clip_path, &cbox);
    }
    if (target != 0)
	(*dev_proc(target, get_clipping_box))(target, &target_box);
    imager_state = clist_imager_state_initial;
    code = gs_imager_state_initialize(&imager_state, mem);
    if (code < 0)
	goto out;
    imager_state.line_params.dash.pattern = dash_pattern;
    if (tdev != 0)
	gx_set_cmap_procs(&imager_state, tdev);
    gx_imager_setscreenphase(&imager_state, -x0, -y0, gs_color_select_all);
    halftone_type = ht_type_none;
    fill_params.fill_zero_width = false;
    pcs = gs_cspace_DeviceGray(&imager_state);
    color_space.params.indexed.use_proc = 0;
    color_space.params.indexed.lookup.table.size = 0;
    data_bits = gs_alloc_bytes(mem, data_bits_size,
			       "clist_playback_band(data_bits)");
    if (data_bits == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto out;
    }
    while (code >= 0) {
	int op;
	int compress, depth, raster;
	byte *source;
	gx_color_index colors[2];
	gx_color_index *pcolor;
	gs_logical_operation_t log_op;
	tile_slot bits;		/* parameters for reading bits */

	/* Make sure the buffer contains a full command. */
	if (cbp >= cbuf.limit) {
	    if (cbuf.end_status < 0) {	/* End of file or error. */
		if (cbp == cbuf.end) {
		    code = (cbuf.end_status == EOFC ? 0 :
			    gs_note_error(gs_error_ioerror));
		    break;
		}
	    } else {
		cbp = top_up_cbuf(&cbuf, cbp);
	    }
	}
	op = *cbp++;
#ifdef DEBUG
	if (gs_debug_c('L')) {
	    const char *const *sub = cmd_sub_op_names[op >> 4];

	    if (sub)
		dlprintf1("[L]%s:", sub[op & 0xf]);
	    else
		dlprintf2("[L]%s %d:", cmd_op_names[op >> 4], op & 0xf);
	}
#endif
	switch (op >> 4) {
	    case cmd_op_misc >> 4:
		switch (op) {
		    case cmd_opv_end_run:
			if_debug0('L', "\n");
			continue;
		    case cmd_opv_set_tile_size:
			cbuf.ptr = cbp;
			code = read_set_tile_size(&cbuf, &tile_bits);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			continue;
		    case cmd_opv_set_tile_phase:
			cmd_getw(state.tile_phase.x, cbp);
			cmd_getw(state.tile_phase.y, cbp);
			if_debug2('L', " (%d,%d)\n",
				  state.tile_phase.x,
				  state.tile_phase.y);
			goto set_phase;
		    case cmd_opv_set_tile_bits:
			bits = tile_bits;
			compress = 0;
		      stb:
			cbuf.ptr = cbp;
			code = read_set_bits(&cbuf, &bits, compress,
					     &state, &state_tile, &state_slot,
					     cdev, mem);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			goto stp;
		    case cmd_opv_set_bits:
			compress = *cbp & 3;
			bits.cb_depth = *cbp++ >> 2;
			cmd_getw(bits.width, cbp);
			cmd_getw(bits.height, cbp);
			if_debug4('L', " compress=%d depth=%d size=(%d,%d)",
				  compress, bits.cb_depth,
				  bits.width, bits.height);
			bits.cb_raster =
			    bitmap_raster(bits.width * bits.cb_depth);
			bits.x_reps = bits.y_reps = 1;
			bits.shift = bits.rep_shift = 0;
			goto stb;
		    case cmd_opv_set_tile_color:
			set_colors = state.tile_colors;
			if_debug0('L', "\n");
			continue;
		    case cmd_opv_set_misc:
			{
			    uint cb = *cbp++;

			    switch (cb >> 6) {
				case cmd_set_misc_lop >> 6:
				    cmd_getw(state.lop, cbp);
				    state.lop = (state.lop << 6) + (cb & 0x3f);
				    if_debug1('L', " lop=0x%x\n", state.lop);
				    if (state.lop_enabled)
					imager_state.log_op = state.lop;
				    break;
				case cmd_set_misc_data_x >> 6:
				    if (cb & 0x20)
					cmd_getw(data_x, cbp);
				    else
					data_x = 0;
				    data_x = (data_x << 5) + (cb & 0x1f);
				    if_debug1('L', " data_x=%d\n", data_x);
				    break;
				case cmd_set_misc_map >> 6:
				    {
					frac *mdata;
					uint count;
					cmd_map_contents cont =
					    (cmd_map_contents)(cb & 0x30) >> 4;

					code = cmd_select_map(cb & 0xf, cont,
							      &imager_state,
							      porder, &mdata,
							      &count, mem);

					if (code < 0)
					    goto out;
					if (cont == cmd_map_other) {
					    cmd_read((byte *)mdata, count, cbp);
#ifdef DEBUG
					    if (gs_debug_c('L')) {
						uint i;

						for (i = 0; i < count / sizeof(*mdata); ++i)
						    dprintf1(" 0x%04x", mdata[i]);
						dputc('\n');
					    }
					} else {
					    if_debug0('L', " none\n");
#endif
					}
				    }
				    /* Recompute the effective transfer, */
				    /* in case this was a transfer map. */
				    gx_imager_set_effective_xfer(
							     &imager_state);
				    break;
				case cmd_set_misc_halftone >> 6: {
				    uint num_comp;

				    halftone_type = cb & 0x3f;
				    cmd_getw(num_comp, cbp);
				    if_debug2('L', " halftone type=%d num_comp=%u\n",
					      halftone_type, num_comp);
				    code = cmd_resize_halftone(
							&imager_state.dev_ht,
							num_comp, mem);
				    if (code < 0)
					goto out;
				    break;
				}
				default:
				    goto bad_op;
			    }
			}
			continue;
		    case cmd_opv_enable_lop:
			state.lop_enabled = true;
			imager_state.log_op = state.lop;
			if_debug0('L', "\n");
			continue;
		    case cmd_opv_disable_lop:
			state.lop_enabled = false;
			imager_state.log_op = lop_default;
			if_debug0('L', "\n");
			continue;
		    case cmd_opv_set_ht_order:
			cbuf.ptr = cbp;
			code = read_set_ht_order(&cbuf, &imager_state.dev_ht,
						 &porder, mem);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			ht_data_index = 0;
			/*
			 * Free the relevant cache, because its sizes
			 * are probably not correct any more.
			 */
			{
			    gx_ht_cache *pcache = porder->cache;

			    if (pcache) {
				if (pcache != imager_state.ht_cache)
				    gx_ht_free_cache(mem, pcache);
				porder->cache = 0;
			    }
			}
			continue;
		    case cmd_opv_set_ht_data:
			cbuf.ptr = cbp;
			code = read_set_ht_data(&cbuf, &ht_data_index, porder,
						halftone_type,
						&imager_state, cdev, mem);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			continue;
		    case cmd_opv_end_page:
			if_debug0('L', "\n");
			/*
			 * Do end-of-page cleanup, then reinitialize if
			 * there are more pages to come.
			 */
			goto out;
		    case cmd_opv_delta2_color0:
			pcolor = &set_colors[0];
			goto delta2_c;
		    case cmd_opv_delta2_color1:
			pcolor = &set_colors[1];
		      delta2_c:set_colors = state.colors;
			{
			    gx_color_index b = ((uint) * cbp << 8) + cbp[1];

			    cbp += 2;
			    if (dev_depth > 24)
				*pcolor +=
				    ((b & 0xf000) << 12) + ((b & 0x0f00) << 8) +
				    ((b & 0x00f0) << 4) + (b & 0x000f) -
				    cmd_delta2_32_bias;
			    else
				*pcolor +=
				    ((b & 0xf800) << 5) + ((b & 0x07e0) << 3) +
				    (b & 0x001f) - cmd_delta2_24_bias;
			}
			if_debug1('L', " 0x%lx\n", *pcolor);
			continue;
		    case cmd_opv_set_copy_color:
			state.color_is_alpha = 0;
			if_debug0('L', "\n");
			continue;
		    case cmd_opv_set_copy_alpha:
			state.color_is_alpha = 1;
			if_debug0('L', "\n");
			continue;
		    default:
			goto bad_op;
		}
		/*NOTREACHED */
	    case cmd_op_set_color0 >> 4:
		pcolor = &set_colors[0];
		goto set_color;
	    case cmd_op_set_color1 >> 4:
		pcolor = &set_colors[1];
	      set_color:set_colors = state.colors;
		switch (op & 0xf) {
		    case 0:
			break;
		    case 15:	/* special handling because this may */
			/* require more bits than depth */
			*pcolor = gx_no_color_index;
			goto setc;
		    default:
			switch (dev_depth_bytes) {
			    case 4:
				{
				    gx_color_index b =
				    ((gx_color_index) (op & 0xf) << 8) + *cbp++;

				    *pcolor +=
					((b & 07000) << 15) + ((b & 0700) << 10) +
					((b & 070) << 5) + (b & 7) -
					cmd_delta1_32_bias;
				    goto setc;
				}
			    case 3:
				{
				    gx_color_index b = *cbp++;

				    *pcolor +=
					((gx_color_index) (op & 0xf) << 16) +
					((b & 0xf0) << 4) + (b & 0x0f) -
					cmd_delta1_24_bias;
				    goto setc;
				}
			    case 2:
				break;
			    case 1:
				*pcolor += (gx_color_index) (op & 0xf) - 8;
				goto setc;
			}
		}
		{
		    gx_color_index color = 0;

		    switch (dev_depth_bytes) {
			case 4:
			    color |= (gx_color_index) * cbp++ << 24;
			case 3:
			    color |= (gx_color_index) * cbp++ << 16;
			case 2:
			    color |= (gx_color_index) * cbp++ << 8;
			case 1:
			    color |= (gx_color_index) * cbp++;
		    }
		    *pcolor = color;
		}
	      setc:if_debug1('L', " 0x%lx\n", *pcolor);
		continue;
	    case cmd_op_fill_rect >> 4:
	    case cmd_op_tile_rect >> 4:
		cbp = cmd_read_rect(op, &state.rect, cbp);
		break;
	    case cmd_op_fill_rect_short >> 4:
	    case cmd_op_tile_rect_short >> 4:
		state.rect.x += *cbp + cmd_min_short;
		state.rect.width += cbp[1] + cmd_min_short;
		if (op & 0xf) {
		    state.rect.height += (op & 0xf) + cmd_min_dxy_tiny;
		    cbp += 2;
		} else {
		    state.rect.y += cbp[2] + cmd_min_short;
		    state.rect.height += cbp[3] + cmd_min_short;
		    cbp += 4;
		}
		break;
	    case cmd_op_fill_rect_tiny >> 4:
	    case cmd_op_tile_rect_tiny >> 4:
		if (op & 8)
		    state.rect.x += state.rect.width;
		else {
		    int txy = *cbp++;

		    state.rect.x += (txy >> 4) + cmd_min_dxy_tiny;
		    state.rect.y += (txy & 0xf) + cmd_min_dxy_tiny;
		}
		state.rect.width += (op & 7) + cmd_min_dw_tiny;
		break;
	    case cmd_op_copy_mono >> 4:
		depth = 1;
		goto copy;
	    case cmd_op_copy_color_alpha >> 4:
		if (state.color_is_alpha) {
		    if (!(op & 8))
			depth = *cbp++;
		} else
		    depth = dev_depth;
	      copy:cmd_getw(state.rect.x, cbp);
		cmd_getw(state.rect.y, cbp);
		if (op & 8) {	/* Use the current "tile". */
#ifdef DEBUG
		    if (state_slot->index != state.tile_index) {
			lprintf2("state_slot->index = %d, state.tile_index = %d!\n",
				 state_slot->index,
				 state.tile_index);
			code = gs_note_error(gs_error_ioerror);
			goto out;
		    }
#endif
		    depth = state_slot->cb_depth;
		    state.rect.width = state_slot->width;
		    state.rect.height = state_slot->height;
		    raster = state_slot->cb_raster;
		    source = (byte *) (state_slot + 1);
		} else {	/* Read width, height, bits. */
		    /* depth was set already. */
		    uint width_bits, width_bytes;
		    uint bytes;

		    cmd_getw(state.rect.width, cbp);
		    cmd_getw(state.rect.height, cbp);
		    width_bits = state.rect.width * depth;
		    bytes =
			clist_bitmap_bytes(width_bits,
					   state.rect.height,
					   op & 3, &width_bytes,
					   (uint *)&raster);
		    /* copy_mono and copy_color/alpha */
		    /* ensure that the bits will fit in a single buffer, */
		    /* even after decompression if compressed. */
#ifdef DEBUG
		    if (bytes > cbuf_size) {
			lprintf6("bitmap size exceeds buffer!  width=%d raster=%d height=%d\n    file pos %ld buf pos %d/%d\n",
				 state.rect.width, raster,
				 state.rect.height,
				 stell(s), (int)(cbp - cbuf.data),
				 (int)(cbuf.end - cbuf.data));
			code = gs_note_error(gs_error_ioerror);
			goto out;
		    }
#endif
		    if (op & 3) {	/* Decompress the image data. */
			stream_cursor_read r;
			stream_cursor_write w;

			/* We don't know the data length a priori, */
			/* so to be conservative, we read */
			/* the uncompressed size. */
			uint cleft = cbuf.end - cbp;

			if (cleft < bytes) {
			    uint nread = cbuf_size - cleft;

			    memmove(cbuf.data, cbp, cleft);
			    cbuf.end_status = sgets(s, cbuf.data + cleft, nread, &nread);
			    set_cb_end(&cbuf, cbuf.data + cleft + nread);
			    cbp = cbuf.data;
			}
			r.ptr = cbp - 1;
			r.limit = cbuf.end - 1;
			w.ptr = data_bits - 1;
			w.limit = w.ptr + data_bits_size;
			switch (op & 3) {
			    case cmd_compress_rle:
				{
				    stream_RLD_state sstate;

				    clist_rld_init(&sstate);
				    /* The process procedure can't fail. */
				    (*s_RLD_template.process)
					((stream_state *)&sstate, &r, &w, true);
				}
				break;
			    case cmd_compress_cfe:
				{
				    stream_CFD_state sstate;

				    clist_cfd_init(&sstate,
				    width_bytes << 3 /*state.rect.width */ ,
						   state.rect.height, mem);
				    /* The process procedure can't fail. */
				    (*s_CFD_template.process)
					((stream_state *)&sstate, &r, &w, true);
				    (*s_CFD_template.release)
					((stream_state *)&sstate);
				}
				break;
			    default:
				goto bad_op;
			}
			cbp = r.ptr + 1;
			source = data_bits;
		    } else if (state.rect.height > 1 &&
			       width_bytes != raster
			) {
			source = data_bits;
			cbp = cmd_read_short_bits(&cbuf, source, width_bytes,
						  state.rect.height,
						  raster, cbp);
		    } else {
			cmd_read(cbuf.data, bytes, cbp);
			source = cbuf.data;
		    }
#ifdef DEBUG
		    if (gs_debug_c('L')) {
			dprintf2(" depth=%d, data_x=%d\n",
				 depth, data_x);
			cmd_print_bits(source, state.rect.width,
				       state.rect.height, raster);
		    }
#endif
		}
		break;
	    case cmd_op_delta_tile_index >> 4:
		state.tile_index += (int)(op & 0xf) - 8;
		goto sti;
	    case cmd_op_set_tile_index >> 4:
		state.tile_index =
		    ((op & 0xf) << 8) + *cbp++;
	      sti:state_slot =
		    (tile_slot *) (cdev->chunk.data +
				 cdev->tile_table[state.tile_index].offset);
		if_debug2('L', " index=%u offset=%lu\n",
			  state.tile_index,
			  cdev->tile_table[state.tile_index].offset);
		state_tile.data = (byte *) (state_slot + 1);
	      stp:state_tile.size.x = state_slot->width;
		state_tile.size.y = state_slot->height;
		state_tile.raster = state_slot->cb_raster;
		state_tile.rep_width = state_tile.size.x /
		    state_slot->x_reps;
		state_tile.rep_height = state_tile.size.y /
		    state_slot->y_reps;
		state_tile.rep_shift = state_slot->rep_shift;
		state_tile.shift = state_slot->shift;
set_phase:	/*
		 * state.tile_phase is overloaded according to the command
		 * to which it will apply:
		 *	For fill_path, stroke_path, fill_triangle/p'gram,
		 * fill_mask, and (mask or CombineWithColor) images,
		 * it is pdcolor->phase.  For these operations, we
		 * precompute the color_phase values.
		 *	For strip_tile_rectangle and strip_copy_rop,
		 * it is the phase arguments of the call, used with
		 * state_tile.  For these operations, we precompute the
		 * tile_phase values.
		 *
		 * Note that control may get here before one or both of
		 * state_tile or dev_ht has been set.
		 */
		if (state_tile.size.x)
		    tile_phase.x =
			(state.tile_phase.x + x0) % state_tile.size.x;
		if (imager_state.dev_ht && imager_state.dev_ht->lcm_width)
		    color_phase.x =
			(state.tile_phase.x + x0) %
			imager_state.dev_ht->lcm_width;
		/*
		 * The true tile height for shifted tiles is not
		 * size.y: see gxbitmap.h for the computation.
		 */
		if (state_tile.size.y) {
		    int full_height;

		    if (state_tile.shift == 0)
			full_height = state_tile.size.y;
		    else
			full_height = state_tile.rep_height *
			    (state_tile.rep_width /
			     igcd(state_tile.rep_shift,
				  state_tile.rep_width));
		    tile_phase.y =
			(state.tile_phase.y + y0) % full_height;
		}
		if (imager_state.dev_ht && imager_state.dev_ht->lcm_height)
		    color_phase.y =
			(state.tile_phase.y + y0) %
			imager_state.dev_ht->lcm_height;
		gx_imager_setscreenphase(&imager_state,
					 -(state.tile_phase.x + x0),
					 -(state.tile_phase.y + y0),
					 gs_color_select_all);
		continue;
	    case cmd_op_misc2 >> 4:
		switch (op) {
		    case cmd_opv_set_fill_adjust:
			cmd_get_value(imager_state.fill_adjust.x, cbp);
			cmd_get_value(imager_state.fill_adjust.y, cbp);
			if_debug2('L', " (%g,%g)\n",
				  fixed2float(imager_state.fill_adjust.x),
				  fixed2float(imager_state.fill_adjust.y));
			continue;
		    case cmd_opv_set_ctm:
			{
			    gs_matrix mat;

			    cbp = cmd_read_matrix(&mat, cbp);
			    mat.tx -= x0;
			    mat.ty -= y0;
			    gs_imager_setmatrix(&imager_state, &mat);
			    if_debug6('L', " [%g %g %g %g %g %g]\n",
				      mat.xx, mat.xy, mat.yx, mat.yy,
				      mat.tx, mat.ty);
			}
			continue;
		    case cmd_opv_set_misc2:
			cbuf.ptr = cbp;
			code = read_set_misc2(&cbuf, &imager_state, &notes);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			continue;
		    case cmd_opv_set_dash:
			{
			    int nb = *cbp++;
			    int n = nb & 0x3f;
			    float dot_length, offset;

			    cmd_get_value(dot_length, cbp);
			    cmd_get_value(offset, cbp);
			    memcpy(dash_pattern, cbp, n * sizeof(float));

			    gx_set_dash(&imager_state.line_params.dash,
					dash_pattern, n, offset,
					NULL);
			    gx_set_dash_adapt(&imager_state.line_params.dash,
					      (nb & 0x80) != 0);
			    gx_set_dot_length(&imager_state.line_params,
					      dot_length,
					      (nb & 0x40) != 0);
#ifdef DEBUG
			    if (gs_debug_c('L')) {
				int i;

				dprintf4(" dot=%g(mode %d) adapt=%d offset=%g [",
					 dot_length,
					 (nb & 0x40) != 0,
					 (nb & 0x80) != 0, offset);
				for (i = 0; i < n; ++i)
				    dprintf1("%g ", dash_pattern[i]);
				dputs("]\n");
			    }
#endif
			    cbp += n * sizeof(float);
			}
			break;
		    case cmd_opv_enable_clip:
			pcpath = (use_clip ? &clip_path : NULL);
			if_debug0('L', "\n");
			break;
		    case cmd_opv_disable_clip:
			pcpath = NULL;
			if_debug0('L', "\n");
			break;
		    case cmd_opv_begin_clip:
			pcpath = NULL;
			if_debug0('L', "\n");
			code = gx_cpath_reset(&clip_path);
			if (code < 0)
			    goto out;
			gx_cpath_accum_begin(&clip_accum, mem);
			gx_cpath_accum_set_cbox(&clip_accum,
						&target_box);
			tdev = (gx_device *)&clip_accum;
			clip_save.lop_enabled = state.lop_enabled;
			clip_save.fill_adjust =
			    imager_state.fill_adjust;
			state.lop_enabled = false;
			imager_state.log_op = lop_default;
			imager_state.fill_adjust.x =
			    imager_state.fill_adjust.y = fixed_half;
			break;
		    case cmd_opv_end_clip:
			if_debug0('L', "\n");
			gx_cpath_accum_end(&clip_accum, &clip_path);
			tdev = target;
			/*
			 * If the entire band falls within the clip
			 * path, no clipping is needed.
			 */
			{
			    gs_fixed_rect cbox;

			    gx_cpath_inner_box(&clip_path, &cbox);
			    use_clip =
				!(cbox.p.x <= target_box.p.x &&
				  cbox.q.x >= target_box.q.x &&
				  cbox.p.y <= target_box.p.y &&
				  cbox.q.y >= target_box.q.y);
			}
			pcpath = (use_clip ? &clip_path : NULL);
			state.lop_enabled = clip_save.lop_enabled;
			imager_state.log_op =
			    (state.lop_enabled ? state.lop :
			     lop_default);
			imager_state.fill_adjust =
			    clip_save.fill_adjust;
			break;
		    case cmd_opv_set_color_space:
			cbuf.ptr = cbp;
			code = read_set_color_space(&cbuf, &imager_state,
						    &pcs, &color_space, mem);
			cbp = cbuf.ptr;
			if (code < 0) {
			    if (code == gs_error_rangecheck)
				goto bad_op;
			    goto out;
			}
			break;
		    case cmd_opv_begin_image_rect:
			cbuf.ptr = cbp;
			code = read_begin_image(&cbuf, &image.c, pcs);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			{
			    uint diff;

			    cmd_getw(image_rect.p.x, cbp);
			    cmd_getw(image_rect.p.y, cbp);
			    cmd_getw(diff, cbp);
			    image_rect.q.x = image.d.Width - diff;
			    cmd_getw(diff, cbp);
			    image_rect.q.y = image.d.Height - diff;
			    if_debug4('L', " rect=(%d,%d),(%d,%d)",
				      image_rect.p.x, image_rect.p.y,
				      image_rect.q.x, image_rect.q.y);
			}
			goto ibegin;
		    case cmd_opv_begin_image:
			cbuf.ptr = cbp;
			code = read_begin_image(&cbuf, &image.c, pcs);
			cbp = cbuf.ptr;
			if (code < 0)
			    goto out;
			image_rect.p.x = 0;
			image_rect.p.y = 0;
			image_rect.q.x = image.d.Width;
			image_rect.q.y = image.d.Height;
			if_debug2('L', " size=(%d,%d)",
				  image.d.Width, image.d.Height);
ibegin:			if_debug0('L', "\n");
			{
			    gx_drawing_color devc;

			    color_set_pure(&devc, state.colors[1]);
			    code = (*dev_proc(tdev, begin_typed_image))
				(tdev, &imager_state, NULL,
				 (const gs_image_common_t *)&image,
				 &image_rect, &devc, pcpath, mem,
				 &image_info);
			}
			if (code < 0)
			    goto out;
			break;
		    case cmd_opv_image_plane_data:
			cmd_getw(data_height, cbp);
			if (data_height == 0) {
			    if_debug0('L', " done image\n");
			    code = gx_image_end(image_info, true);
			    if (code < 0)
				goto out;
			    continue;
			}
			{
			    uint flags;
			    int plane;
			    uint raster;

			    cmd_getw(flags, cbp);
			    for (plane = 0;
				 plane < image_info->num_planes;
				 ++plane, flags >>= 1
				 ) {
				if (flags & 1) {
				    if (cbuf.end - cbp <
					2 * cmd_max_intsize(sizeof(uint)))
					cbp = top_up_cbuf(&cbuf, cbp);
				    cmd_getw(planes[plane].raster, cbp);
				    if ((raster = planes[plane].raster) != 0)
					cmd_getw(data_x, cbp);
				} else {
				    planes[plane].raster = raster;
				}
				planes[plane].data_x = data_x;
			    }
			}
			goto idata;
		    case cmd_opv_image_data:
			cmd_getw(data_height, cbp);
			if (data_height == 0) {
			    if_debug0('L', " done image\n");
			    code = gx_image_end(image_info, true);
			    if (code < 0)
				goto out;
			    continue;
			}
			{
			    uint bytes_per_plane;
			    int plane;

			    cmd_getw(bytes_per_plane, cbp);
			    if_debug2('L', " height=%u raster=%u\n",
				      data_height, bytes_per_plane);
			    for (plane = 0;
				 plane < image_info->num_planes;
				 ++plane
				 ) {
				planes[plane].data_x = data_x;
				planes[plane].raster = bytes_per_plane;
			    }
			}
idata:			data_size = 0;
			{
			    int plane;

			    for (plane = 0; plane < image_info->num_planes;
				 ++plane)
				data_size += planes[plane].raster;
			}
			data_size *= data_height;
			data_on_heap = 0;
			if (cbuf.end - cbp < data_size)
			    cbp = top_up_cbuf(&cbuf, cbp);
			if (cbuf.end - cbp >= data_size) {
			    planes[0].data = cbp;
			    cbp += data_size;
			} else {
			    uint cleft = cbuf.end - cbp;
			    uint rleft = data_size - cleft;
			    byte *rdata;

			    if (data_size > cbuf.end - cbuf.data) {
				/* Allocate a separate buffer. */
				rdata = data_on_heap =
				    gs_alloc_bytes(mem, data_size,
						   "clist image_data");
				if (rdata == 0) {
				    code = gs_note_error(gs_error_VMerror);
				    goto out;
				}
			    } else
				rdata = cbuf.data;
			    memmove(rdata, cbp, cleft);
			    sgets(s, rdata + cleft, rleft,
				  &rleft);
			    planes[0].data = rdata;
			    cbp = cbuf.end;	/* force refill */
			}
			{
			    int plane;
			    const byte *data = planes[0].data;

			    for (plane = 0;
				 plane < image_info->num_planes;
				 ++plane
				 ) {
				if (planes[plane].raster == 0)
				    planes[plane].data = 0;
				else {
				    planes[plane].data = data;
				    data += planes[plane].raster *
					data_height;
				}
			    }
			}
#ifdef DEBUG
			if (gs_debug_c('L')) {
			    int plane;

			    for (plane = 0; plane < image_info->num_planes;
				 ++plane)
				if (planes[plane].data != 0)
				    cmd_print_bits(planes[plane].data,
						   image_rect.q.x -
						   image_rect.p.x,
						   data_height,
						   planes[plane].raster);
			}
#endif
			code = gx_image_plane_data(image_info, planes,
						   data_height);
			if (data_on_heap)
			    gs_free_object(mem, data_on_heap,
					   "clist image_data");
			data_x = 0;
			if (code < 0)
			    goto out;
			continue;
		    case cmd_opv_set_color:
			op = *cbp++;
#define dcb dev_color.colors.colored.c_base
#define dcl dev_color.colors.colored.c_level
			dcb[0] = ((op & 0xf) << 1) + (*cbp >> 7);
			dcb[1] = (*cbp >> 2) & 0x1f;
			dcb[2] = ((*cbp & 3) << 3) + (cbp[1] >> 5);
			dcb[3] = cbp[1] & 0x1f;
			cbp += 2;
			goto setcc;
		    case cmd_opv_set_color_short:
			op = *cbp++;
			dcb[0] = (op >> 3) & 1;
			dcb[1] = (op >> 2) & 1;
			dcb[2] = (op >> 1) & 1;
			dcb[3] = op & 1;
setcc:			{
			    int i;

			    for (i = 0; i < imager_state.dev_ht->num_comp; ++i)
				if (op & (0x80 >> i))
				    cmd_getw(dcl[i], cbp);
				else
				    dcl[i] = 0;
			    if_debug9('L', " num_comp=%d base=(%u,%u,%u,%u) level=(%u,%u,%u,%u)\n",
				       imager_state.dev_ht->num_comp,
				       dcb[0], dcb[1], dcb[2], dcb[3],
				       dcl[0], dcl[1], dcl[2], dcl[3]);
			    color_finish_set_cmyk_halftone(&dev_color,
						       imager_state.dev_ht);
#undef dcb
#undef dcl
			}
			goto set_phase;
		    case cmd_opv_put_params:
			cbuf.ptr = cbp;
			code = read_put_params(&cbuf, &imager_state, cdev,
					       mem);
			cbp = cbuf.ptr;
			if (code > 0)
			    break; /* empty list */
			if (code < 0)
			    goto out;
			if (playback_action == playback_action_setup)
			    goto out;
			break;
		    default:
			goto bad_op;
		}
		continue;
	    case cmd_op_segment >> 4:
		{
		    int i, code;
		    static const byte op_num_operands[] = {
			cmd_segment_op_num_operands_values
		    };

		    if (!in_path) {
			ppos.x = int2fixed(state.rect.x);
			ppos.y = int2fixed(state.rect.y);
			if_debug2('L', " (%d,%d)", state.rect.x,
				  state.rect.y);
			notes = sn_none;
			in_path = true;
		    }
		    for (i = 0; i < op_num_operands[op & 0xf]; ++i) {
			fixed v;
			int b = *cbp;

			switch (b >> 5) {
			    case 0:
			    case 1:
				vs[i++] =
				    ((fixed) ((b ^ 0x20) - 0x20) << 13) +
				    ((int)cbp[1] << 5) + (cbp[2] >> 3);
				if_debug1('L', " %g", fixed2float(vs[i - 1]));
				cbp += 2;
				v = (int)((*cbp & 7) ^ 4) - 4;
				break;
			    case 2:
			    case 3:
				v = (b ^ 0x60) - 0x20;
				break;
			    case 4:
			    case 5:
				/*
				 * Without the following cast, C's
				 * brain-damaged coercion rules cause the
				 * result to be considered unsigned, and not
				 * sign-extended on machines where
				 * sizeof(long) > sizeof(int).
				 */
				v = (((b ^ 0xa0) - 0x20) << 8) + (int)*++cbp;
				break;
			    case 6:
				v = (b ^ 0xd0) - 0x10;
				vs[i] =
				    ((v << 8) + cbp[1]) << (_fixed_shift - 2);
				if_debug1('L', " %g", fixed2float(vs[i]));
				cbp += 2;
				continue;
			    default /*case 7 */ :
				v = (int)(*++cbp ^ 0x80) - 0x80;
				for (b = 0; b < sizeof(fixed) - 3; ++b)
				    v = (v << 8) + *++cbp;
				break;
			}
			cbp += 3;
			/* Absent the cast in the next statement, */
			/* the Borland C++ 4.5 compiler incorrectly */
			/* sign-extends the result of the shift. */
			vs[i] = (v << 16) + (uint) (cbp[-2] << 8) + cbp[-1];
			if_debug1('L', " %g", fixed2float(vs[i]));
		    }
		    if_debug0('L', "\n");
		    code = clist_decode_segment(&path, op, vs, &ppos,
						x0, y0, notes);
		    if (code < 0)
			goto out;
		}
		continue;
	    case cmd_op_path >> 4:
		{
		    gx_device_color devc;
		    gx_device_color *pdevc;
		    gx_ht_tile ht_tile;

		    if_debug0('L', "\n");
		    switch (op) {
			case cmd_opv_fill:
			    fill_params.rule = gx_rule_winding_number;
			    goto fill_pure;
			case cmd_opv_eofill:
			    fill_params.rule = gx_rule_even_odd;
			  fill_pure:color_set_pure(&devc, state.colors[1]);
			    pdevc = &devc;
			    goto fill;
			case cmd_opv_htfill:
			    fill_params.rule = gx_rule_winding_number;
			    goto fill_ht;
			case cmd_opv_hteofill:
			    fill_params.rule = gx_rule_even_odd;
			  fill_ht:ht_tile.tiles = state_tile;
			    color_set_binary_tile(&devc,
						  state.tile_colors[0],
						  state.tile_colors[1],
						  &ht_tile);
			    pdevc = &devc;
			    pdevc->phase = tile_phase;
			    goto fill;
			case cmd_opv_colorfill:
			    fill_params.rule = gx_rule_winding_number;
			    goto fill_color;
			case cmd_opv_coloreofill:
			    fill_params.rule = gx_rule_even_odd;
			  fill_color:pdevc = &dev_color;
			    pdevc->phase = color_phase;
			    code =
				gx_color_load(pdevc, &imager_state, tdev);
			    if (code < 0)
				break;
			  fill:fill_params.adjust = imager_state.fill_adjust;
			    fill_params.flatness = imager_state.flatness;
			    code = gx_fill_path_only(&path, tdev,
						     &imager_state,
						     &fill_params,
						     pdevc, pcpath);
			    break;
			case cmd_opv_stroke:
			    color_set_pure(&devc, state.colors[1]);
			    pdevc = &devc;
			    goto stroke;
			case cmd_opv_htstroke:
			    ht_tile.tiles = state_tile;
			    color_set_binary_tile(&devc,
						  state.tile_colors[0],
						  state.tile_colors[1],
						  &ht_tile);
			    pdevc = &devc;
			    pdevc->phase = tile_phase;
			    goto stroke;
			case cmd_opv_colorstroke:
			    pdevc = &dev_color;
			    pdevc->phase = color_phase;
			    code =
				gx_color_load(pdevc, &imager_state, tdev);
			    if (code < 0)
				break;
			  stroke:stroke_params.flatness = imager_state.flatness;
			    code = gx_stroke_path_only(&path,
						       (gx_path *) 0, tdev,
					      &imager_state, &stroke_params,
						       pdevc, pcpath);
			    break;
			case cmd_opv_polyfill:
			    color_set_pure(&devc, state.colors[1]);
			    pdevc = &devc;
			    goto poly;
			case cmd_opv_htpolyfill:
			    ht_tile.tiles = state_tile;
			    color_set_binary_tile(&devc,
						  state.tile_colors[0],
						  state.tile_colors[1],
						  &ht_tile);
			    pdevc = &devc;
			    pdevc->phase = tile_phase;
			    goto poly;
			case cmd_opv_colorpolyfill:
			    pdevc = &dev_color;
			    pdevc->phase = color_phase;
			    code = gx_color_load(pdevc, &imager_state, tdev);
			    if (code < 0)
				break;
poly:			    code = clist_do_polyfill(tdev, &path, pdevc,
						     imager_state.log_op);
			    break;
			default:
			    goto bad_op;
		    }
		}
		if (in_path) {	/* path might be empty! */
		    state.rect.x = fixed2int_var(ppos.x);
		    state.rect.y = fixed2int_var(ppos.y);
		    in_path = false;
		}
		gx_path_free(&path, "clist_render_band");
		gx_path_init_local(&path, mem);
		if (code < 0)
		    goto out;
		continue;
	    default:
	      bad_op:lprintf5("Bad op %02x band y0 = %d file pos %ld buf pos %d/%d\n",
		 op, y0, stell(s), (int)(cbp - cbuf.data), (int)(cbuf.end - cbuf.data));
		{
		    const byte *pp;

		    for (pp = cbuf.data; pp < cbuf.end; pp += 10) {
			dlprintf1("%4d:", (int)(pp - cbuf.data));
			dprintf10(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				  pp[0], pp[1], pp[2], pp[3], pp[4],
				  pp[5], pp[6], pp[7], pp[8], pp[9]);
		    }
		}
		code = gs_note_error(gs_error_Fatal);
		goto out;
	}
	if_debug4('L', " x=%d y=%d w=%d h=%d\n",
		  state.rect.x, state.rect.y, state.rect.width,
		  state.rect.height);
	switch (op >> 4) {
	    case cmd_op_fill_rect >> 4:
	    case cmd_op_fill_rect_short >> 4:
	    case cmd_op_fill_rect_tiny >> 4:
		if (!state.lop_enabled) {
		    code = (*dev_proc(tdev, fill_rectangle))
			(tdev, state.rect.x - x0, state.rect.y - y0,
			 state.rect.width, state.rect.height,
			 state.colors[1]);
		    break;
		}
		source = NULL;
		data_x = 0;
		raster = 0;
		colors[0] = colors[1] = state.colors[1];
		log_op = state.lop;
		pcolor = colors;
	      do_rop:code = (*dev_proc(tdev, strip_copy_rop))
		    (tdev, source, data_x, raster, gx_no_bitmap_id,
		     pcolor, &state_tile,
		     (state.tile_colors[0] == gx_no_color_index &&
		      state.tile_colors[1] == gx_no_color_index ?
		      NULL : state.tile_colors),
		     state.rect.x - x0, state.rect.y - y0,
		     state.rect.width - data_x, state.rect.height,
		     tile_phase.x, tile_phase.y, log_op);
		data_x = 0;
		break;
	    case cmd_op_tile_rect >> 4:
	    case cmd_op_tile_rect_short >> 4:
	    case cmd_op_tile_rect_tiny >> 4:
		/* Currently we don't use lop with tile_rectangle. */
		code = (*dev_proc(tdev, strip_tile_rectangle))
		    (tdev, &state_tile,
		     state.rect.x - x0, state.rect.y - y0,
		     state.rect.width, state.rect.height,
		     state.tile_colors[0], state.tile_colors[1],
		     tile_phase.x, tile_phase.y);
		break;
	    case cmd_op_copy_mono >> 4:
		if (state.lop_enabled) {
		    pcolor = state.colors;
		    log_op = state.lop;
		    goto do_rop;
		}
		if ((op & cmd_copy_use_tile) || pcpath != NULL) {	/*
									 * This call of copy_mono originated as a call
									 * of fill_mask.
									 */
		    gx_drawing_color dcolor;
		    gx_ht_tile ht_tile;

		    if (op & cmd_copy_ht_color) {	/* Screwy C assignment rules don't allow: */
			/* dcolor.colors = state.tile_colors; */
			ht_tile.tiles = state_tile;
			color_set_binary_tile(&dcolor,
					      state.tile_colors[0],
					    state.tile_colors[1], &ht_tile);
			dcolor.phase = tile_phase;
		    } else {
			color_set_pure(&dcolor, state.colors[1]);
		    }
		    code = (*dev_proc(tdev, fill_mask))
			(tdev, source, data_x, raster, gx_no_bitmap_id,
			 state.rect.x - x0, state.rect.y - y0,
			 state.rect.width - data_x, state.rect.height,
			 &dcolor, 1, imager_state.log_op, pcpath);
		} else
		    code = (*dev_proc(tdev, copy_mono))
			(tdev, source, data_x, raster, gx_no_bitmap_id,
			 state.rect.x - x0, state.rect.y - y0,
			 state.rect.width - data_x, state.rect.height,
			 state.colors[0], state.colors[1]);
		data_x = 0;
		break;
	    case cmd_op_copy_color_alpha >> 4:
		if (state.color_is_alpha) {
/****** CAN'T DO ROP WITH ALPHA ******/
		    code = (*dev_proc(tdev, copy_alpha))
			(tdev, source, data_x, raster, gx_no_bitmap_id,
			 state.rect.x - x0, state.rect.y - y0,
			 state.rect.width - data_x, state.rect.height,
			 state.colors[1], depth);
		} else {
		    if (state.lop_enabled) {
			pcolor = NULL;
			log_op = state.lop;
			goto do_rop;
		    }
		    code = (*dev_proc(tdev, copy_color))
			(tdev, source, data_x, raster, gx_no_bitmap_id,
			 state.rect.x - x0, state.rect.y - y0,
			 state.rect.width - data_x, state.rect.height);
		}
		data_x = 0;
		break;
	    default:		/* can't happen */
		goto bad_op;
	}
    }
    /* Clean up before we exit. */
  out:gx_cpath_free(&clip_path, "clist_render_band exit");
    gx_path_free(&path, "clist_render_band exit");
    gs_imager_state_release(&imager_state);
    if (imager_state.ht_cache)
	gx_ht_free_cache(mem, imager_state.ht_cache);
    gs_free_object(mem, data_bits, "clist_playback_band(data_bits)");
    if (code < 0)
	return_error(code);
    /* Check whether we have more pages to process. */
    if (playback_action != playback_action_setup && 
	(cbp < cbuf.end || !seofp(s))
	)
	goto in;
    return code;
}

/* ---------------- Individual commands ---------------- */

/*
 * These single-use procedures implement a few large individual commands,
 * primarily for readability but also to avoid overflowing compilers'
 * optimization limits.  They all take the command buffer as their first
 * parameter (pcb), assume that the current buffer pointer is in pcb->ptr,
 * and update it there.
 */

private int
read_set_tile_size(command_buf_t *pcb, tile_slot *bits)
{
    const byte *cbp = pcb->ptr;
    uint rep_width, rep_height;
    byte bd = *cbp++;

    bits->cb_depth = (bd & 31) + 1;
    cmd_getw(rep_width, cbp);
    cmd_getw(rep_height, cbp);
    if (bd & 0x20) {
	cmd_getw(bits->x_reps, cbp);
	bits->width = rep_width * bits->x_reps;
    } else {
	bits->x_reps = 1;
	bits->width = rep_width;
    }
    if (bd & 0x40) {
	cmd_getw(bits->y_reps, cbp);
	bits->height = rep_height * bits->y_reps;
    } else {
	bits->y_reps = 1;
	bits->height = rep_height;
    }
    if (bd & 0x80)
	cmd_getw(bits->rep_shift, cbp);
    else
	bits->rep_shift = 0;
    if_debug6('L', " depth=%d size=(%d,%d), rep_size=(%d,%d), rep_shift=%d\n",
	      bits->cb_depth, bits->width,
	      bits->height, rep_width,
	      rep_height, bits->rep_shift);
    bits->shift =
	(bits->rep_shift == 0 ? 0 :
	 (bits->rep_shift * (bits->height / rep_height)) % rep_width);
    bits->cb_raster = bitmap_raster(bits->width * bits->cb_depth);
    pcb->ptr = cbp;
    return 0;
}

private int
read_set_bits(command_buf_t *pcb, tile_slot *bits, int compress,
	      gx_clist_state *pcls, gx_strip_bitmap *tile, tile_slot **pslot,
	      gx_device_clist_reader *cdev, gs_memory_t *mem)
{
    const byte *cbp = pcb->ptr;
    uint rep_width = bits->width / bits->x_reps;
    uint rep_height = bits->height / bits->y_reps;
    uint index;
    ulong offset;
    uint width_bits = rep_width * bits->cb_depth;
    uint width_bytes;
    uint raster;
    uint bytes =
	clist_bitmap_bytes(width_bits, rep_height,
			   compress |
			   (rep_width < bits->width ?
			    decompress_spread : 0) |
			   decompress_elsewhere,
			   &width_bytes,
			   (uint *)&raster);
    byte *data;
    tile_slot *slot;

    cmd_getw(index, cbp);
    cmd_getw(offset, cbp);
    if_debug2('L', " index=%d offset=%lu\n", pcls->tile_index, offset);
    pcls->tile_index = index;
    cdev->tile_table[pcls->tile_index].offset = offset;
    slot = (tile_slot *)(cdev->chunk.data + offset);
    *pslot = slot;
    *slot = *bits;
    tile->data = data = (byte *)(slot + 1);
#ifdef DEBUG
    slot->index = pcls->tile_index;
#endif
    if (compress) {
	/*
	 * Decompress the image data.  We'd like to share this code with the
	 * similar code in copy_*, but right now we don't see how.
	 */
	stream_cursor_read r;
	stream_cursor_write w;
	/*
	 * We don't know the data length a priori, so to be conservative, we
	 * read the uncompressed size.
	 */
	uint cleft = pcb->end - cbp;

	if (cleft < bytes) {
	    uint nread = cbuf_size - cleft;

	    memmove(pcb->data, cbp, cleft);
	    pcb->end_status = sgets(pcb->s, pcb->data + cleft, nread, &nread);
	    set_cb_end(pcb, pcb->data + cleft + nread);
	    cbp = pcb->data;
	}
	r.ptr = cbp - 1;
	r.limit = pcb->end - 1;
	w.ptr = data - 1;
	w.limit = w.ptr + bytes;
	switch (compress) {
	case cmd_compress_rle:
	    {
		stream_RLD_state sstate;

		clist_rld_init(&sstate);
		(*s_RLD_template.process)
		    ((stream_state *)&sstate, &r, &w, true);
	    }
	    break;
	case cmd_compress_cfe:
	    {
		stream_CFD_state sstate;

		clist_cfd_init(&sstate,
			       width_bytes << 3 /*width_bits */ ,
			       rep_height, mem);
		(*s_CFD_template.process)
		    ((stream_state *)&sstate, &r, &w, true);
		(*s_CFD_template.release)
		    ((stream_state *)&sstate);
	    }
	    break;
	default:
	    return_error(gs_error_unregistered);
	}
	cbp = r.ptr + 1;
    } else if (rep_height > 1 && width_bytes != bits->cb_raster) {
	cbp = cmd_read_short_bits(pcb, data,
				  width_bytes, rep_height,
				  bits->cb_raster, cbp);
    } else {
	cbp = cmd_read_data(pcb, data, bytes, cbp);
    }
    if (bits->width > rep_width)
	bits_replicate_horizontally(data,
				    rep_width * bits->cb_depth, rep_height,
				    bits->cb_raster,
				    bits->width * bits->cb_depth,
				    bits->cb_raster);
    if (bits->height > rep_height)
	bits_replicate_vertically(data,
				  rep_height, bits->cb_raster,
				  bits->height);
#ifdef DEBUG
    if (gs_debug_c('L'))
	cmd_print_bits(data, bits->width, bits->height, bits->cb_raster);
#endif
    pcb->ptr = cbp;
    return 0;
}

private int
read_set_ht_order(command_buf_t *pcb, gx_device_halftone **ppdht,
		  gx_ht_order **pporder, gs_memory_t *mem)
{
    const byte *cbp = pcb->ptr;
    gx_ht_order *porder;
    uint old_num_levels;
    uint *levels;
    uint old_num_bits;
    void *bit_data;
    int index;
    gx_ht_order new_order;
    int code = cmd_create_dev_ht(ppdht, mem);
    gx_device_halftone *pdht = *ppdht;

    if (code < 0)
	return code;
    cmd_getw(index, cbp);
    if (index == 0)
	porder = &pdht->order;
    else {
	gx_ht_order_component *pcomp = &pdht->components[index - 1];

	cmd_getw(pcomp->cname, cbp);
	if_debug1('L', " cname=%lu", (ulong) pcomp->cname);
	porder = &pcomp->corder;
    }
    *pporder = porder;
    new_order = *porder;
    cmd_getw(new_order.width, cbp);
    cmd_getw(new_order.height, cbp);
    cmd_getw(new_order.raster, cbp);
    cmd_getw(new_order.shift, cbp);
    cmd_getw(new_order.num_levels, cbp);
    cmd_getw(new_order.num_bits, cbp);
    new_order.procs = &ht_order_procs_table[*cbp++];
    pcb->ptr = cbp;
    if_debug8('L', " index=%d size=(%d,%d) raster=%d shift=%d num_levels=%d num_bits=%d procs=%d\n",
	      index, new_order.width, new_order.height,
	      new_order.raster, new_order.shift,
	      new_order.num_levels, new_order.num_bits, cbp[-1]);
    if (new_order.data_memory == 0) {
	/* levels and bit_data point into ROM. */
	old_num_levels = 0;
	levels = 0;
	old_num_bits = 0;
	bit_data = 0;
    } else {
	old_num_levels = porder->num_levels;
	levels = porder->levels;
	old_num_bits = porder->num_bits;
	bit_data = porder->bit_data;
    }
    new_order.data_memory = mem;
    /*
     * Note that for resizing a byte array, the element size is 1 byte,
     * not the element size given to alloc_byte_array!
     */
    if (new_order.num_levels > old_num_levels) {
	if (levels == 0)
	    levels = (uint *) gs_alloc_byte_array(mem, new_order.num_levels,
						  sizeof(*levels),
						  "ht order(levels)");
	else
	    levels = gs_resize_object(mem, levels,
				      new_order.num_levels * sizeof(*levels),
				      "ht order(levels)");
	if (levels == 0)
	    return_error(gs_error_VMerror);
	/* Update porder in case we bail out. */
	porder->levels = levels;
	porder->num_levels = new_order.num_levels;
    }
    if (new_order.num_bits != old_num_bits ||
	new_order.procs != porder->procs
	) {
	if (bit_data == 0)
	    bit_data = gs_alloc_byte_array(mem, new_order.num_bits,
					   new_order.procs->bit_data_elt_size,
					   "ht order(bit_data)");
	else
	    bit_data = gs_resize_object(mem, bit_data,
					new_order.num_bits *
					  new_order.procs->bit_data_elt_size,
					"ht order(bit_data)");
	if (bit_data == 0)
	    return_error(gs_error_VMerror);
    }
    *porder = new_order;
    porder->levels = levels;
    porder->bit_data = bit_data;
    porder->full_height = ht_order_full_height(porder);
    return 0;
}

private int
read_set_ht_data(command_buf_t *pcb, uint *pdata_index, gx_ht_order *porder,
		 gs_halftone_type halftone_type, gs_imager_state *pis,
		 gx_device_clist_reader *cdev, gs_memory_t *mem)
{
    gx_device_halftone *pdht = pis->dev_ht;
    uint elt_size = porder->procs->bit_data_elt_size;
    const byte *cbp = pcb->ptr;
    int n = *cbp++;

    if (*pdata_index < porder->num_levels) {	/* Setting levels */
	byte *lptr = (byte *)(porder->levels + *pdata_index);

	cbp = cmd_read_data(pcb, lptr, n * sizeof(*porder->levels), cbp);
#ifdef DEBUG
	if (gs_debug_c('L')) {
	    int i;

	    dprintf1(" levels[%u]", *pdata_index);
	    for (i = 0; i < n; ++i)
		dprintf1(" %u",
			 porder->levels[*pdata_index + i]);
	    dputc('\n');
	}
#endif
    } else {	/* Setting bits */
	byte *bptr =
	    (byte *)porder->bit_data +
	      (*pdata_index - porder->num_levels) * elt_size;

	cbp = cmd_read_data(pcb, bptr, n * elt_size, cbp);
#ifdef DEBUG
	if (gs_debug_c('L')) {
	    int i;

	    dprintf1(" bits[%u]", *pdata_index - porder->num_levels);
	    for (i = 0; i < n; ++i) {
		const byte *pb = (byte *)porder->bit_data +
		    (*pdata_index - porder->num_levels + i) * elt_size;

		if (porder->procs == &ht_order_procs_default) {
		    const gx_ht_bit *pbit = (const gx_ht_bit *)pb;

		    dprintf2(" (%u,0x%lx)", pbit->offset, (ulong)pbit->mask);
		} else {
		    debug_dump_bytes(pb, pb + elt_size, NULL);
		}
	    }
	    dputc('\n');
	}
#endif
    }
    *pdata_index += n;
    /*
     * If this is the end of a component, check whether a copy of its data
     * exists in ROM.
     */
    if (*pdata_index == porder->num_levels + porder->num_bits) {
	const gx_dht_proc *phtrp = gx_device_halftone_list;

	for (; *phtrp; ++phtrp) {
	    const gx_device_halftone_resource_t *const *pphtr = (*phtrp)();
	    const gx_device_halftone_resource_t *phtr;

	    while ((phtr = *pphtr++) != 0) {
		if (phtr->Width == porder->width &&
		    phtr->Height == porder->height &&
		    phtr->num_levels == porder->num_levels &&
		    !memcmp(phtr->levels, porder->levels,
			    phtr->num_levels * sizeof(*phtr->levels)) &&
		    !memcmp(phtr->bit_data, porder->bit_data,
			    phtr->Width * phtr->Height * elt_size)
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
    out:
	/*
	 * If this is the end of the data, install the (device) halftone.
	 */
	if (porder ==
	    (pdht->components != 0 ?
	     &pdht->components[0].corder :
	     &pdht->order)) {
	    /* Make sure we have a halftone cache. */
	    uint i;

	    if (pis->ht_cache == 0) {
		gx_ht_cache *pcache =
		    gx_ht_alloc_cache(mem,
				      porder->num_levels + 2,
				      gx_ht_cache_default_bits());

		if (pcache == 0)
		    return_error(gs_error_VMerror);
		pis->ht_cache = pcache;
	    }
	    for (i = 1; i < pdht->num_comp; ++i) {
		gx_ht_order *pco = &pdht->components[i].corder;

		if (!pco->cache) {
		    gx_ht_cache *pcache =
			gx_ht_alloc_cache(mem, 4,
					  pco->raster *
					  (pco->num_bits / pco->width) * 4);

		    if (pcache == 0)
			return_error(gs_error_VMerror);
		    pco->cache = pcache;
		    gx_ht_init_cache(pco->cache, pco);
		}
	    }
	    if (pdht->num_comp) {
		pdht->components[0].corder.cache = pis->ht_cache;
		pdht->order = pdht->components[0].corder;
	    }
	    /*
	     * Normally, the following procedure releases the existing
	     * device halftone and assumes ownership of everything in the
	     * new one except the top-level structure.  However, there is a
	     * special check for the case where pdht == pis->dev_ht, solely
	     * so that we can avoid the sleazy things we would otherwise
	     * have to do here.
	     */
	    gx_imager_dev_ht_install(pis, pdht, halftone_type,
				     (const gx_device *)cdev);
	}
    }
    pcb->ptr = cbp;
    return 0;
}

private int
read_set_misc2(command_buf_t *pcb, gs_imager_state *pis, segment_notes *pnotes)
{
    const byte *cbp = pcb->ptr;
    uint mask, cb;

    cmd_getw(mask, cbp);
    if (mask & cap_join_known) {
	cb = *cbp++;
	pis->line_params.cap = (gs_line_cap)((cb >> 3) & 7);
	pis->line_params.join = (gs_line_join)(cb & 7);
	if_debug2('L', " cap=%d join=%d\n",
		  pis->line_params.cap, pis->line_params.join);
    }
    if (mask & cj_ac_sa_known) {
	cb = *cbp++;
	pis->line_params.curve_join = ((cb >> 2) & 7) - 1;
	pis->accurate_curves = (cb & 2) != 0;
	pis->stroke_adjust = cb & 1;
	if_debug3('L', " CJ=%d AC=%d SA=%d\n",
		  pis->line_params.curve_join, pis->accurate_curves,
		  pis->stroke_adjust);
    }
    if (mask & flatness_known) {
	cmd_get_value(pis->flatness, cbp);
	if_debug1('L', " flatness=%g\n", pis->flatness);
    }
    if (mask & line_width_known) {
	float width;

	cmd_get_value(width, cbp);
	if_debug1('L', " line_width=%g\n", width);
	gx_set_line_width(&pis->line_params, width);
    }
    if (mask & miter_limit_known) {
	float limit;

	cmd_get_value(limit, cbp);
	if_debug1('L', " miter_limit=%g\n", limit);
	gx_set_miter_limit(&pis->line_params, limit);
    }
    if (mask & op_bm_tk_known) {
	cb = *cbp++;
	pis->blend_mode = cb >> 3;
	pis->text_knockout = (cb & 4) != 0;
	pis->overprint_mode = (cb >> 1) & 1;
	pis->overprint = cb & 1;
	if_debug4('L', " BM=%d TK=%d OPM=%d OP=%d\n",
		  pis->blend_mode, pis->text_knockout, pis->overprint_mode,
		  pis->overprint);
    }
    if (mask & segment_notes_known) {
	cb = *cbp++;
	*pnotes = (segment_notes)(cb & 0x3f);
	if_debug1('L', " notes=%d\n", *pnotes);
    }
    if (mask & opacity_alpha_known) {
	cmd_get_value(pis->opacity.alpha, cbp);
	if_debug1('L', " opacity.alpha=%g\n", pis->opacity.alpha);
    }
    if (mask & shape_alpha_known) {
	cmd_get_value(pis->shape.alpha, cbp);
	if_debug1('L', " shape.alpha=%g\n", pis->shape.alpha);
    }
    if (mask & alpha_known) {
	cmd_get_value(pis->alpha, cbp);
	if_debug1('L', " alpha=%u\n", pis->alpha);
    }
    pcb->ptr = cbp;
    return 0;
}

private int
read_set_color_space(command_buf_t *pcb, gs_imager_state *pis,
		     const gs_color_space **ppcs, gs_color_space *pcolor_space,
		     gs_memory_t *mem)
{
    const byte *cbp = pcb->ptr;
    byte b = *cbp++;
    int index = b >> 4;
    const gs_color_space *pcs;
    int code = 0;

    if_debug3('L', " %d%s%s\n", index,
	      (b & 8 ? " (indexed)" : ""),
	      (b & 4 ? "(proc)" : ""));
    switch (index) {
    case gs_color_space_index_DeviceGray:
	pcs = gs_cspace_DeviceGray(pis);
	break;
    case gs_color_space_index_DeviceRGB:
	pcs = gs_cspace_DeviceRGB(pis);
	break;
    case gs_color_space_index_DeviceCMYK:
	pcs = gs_cspace_DeviceCMYK(pis);
	break;
    default:
	code = gs_note_error(gs_error_rangecheck);	/* others are NYI */
	goto out;
    }
    /* Free any old indexed color space data. */
    if (pcolor_space->params.indexed.use_proc) {
	if (pcolor_space->params.indexed.lookup.map)
	    gs_free_object(mem,
			   pcolor_space->params.indexed.lookup.map->values,
			   "old indexed map values");
	    gs_free_object(mem, pcolor_space->params.indexed.lookup.map,
			   "old indexed map");
	pcolor_space->params.indexed.lookup.map = 0;
    } else {
	if (pcolor_space->params.indexed.lookup.table.size)
	    gs_free_const_string(mem,
			pcolor_space->params.indexed.lookup.table.data,
			pcolor_space->params.indexed.lookup.table.size,
				 "old indexed table");
	pcolor_space->params.indexed.lookup.table.size = 0;
    }
    if (b & 8) {
	bool use_proc = (b & 4) != 0;
	int hival;
	int num_values;
	byte *data;
	uint data_size;

	cmd_getw(hival, cbp);
	num_values = (hival + 1) * gs_color_space_num_components(pcs);
	if (use_proc) {
	    gs_indexed_map *map;

	    code = alloc_indexed_map(&map, num_values, mem, "indexed map");
	    if (code < 0)
		goto out;
	    map->proc.lookup_index = lookup_indexed_map;
	    pcolor_space->params.indexed.lookup.map = map;
	    data = (byte *)map->values;
	    data_size = num_values * sizeof(map->values[0]);
	} else {
	    byte *table = gs_alloc_string(mem, num_values, "indexed table");

	    if (table == 0) {
		code = gs_note_error(gs_error_VMerror);
		goto out;
	    }
	    pcolor_space->params.indexed.lookup.table.data = table;
	    pcolor_space->params.indexed.lookup.table.size = num_values;
	    data = table;
	    data_size = num_values;
	}
	cbp = cmd_read_data(pcb, data, data_size, cbp);
	pcolor_space->type =
	    &gs_color_space_type_Indexed;
	memmove(&pcolor_space->params.indexed.base_space, pcs,
		sizeof(pcolor_space->params.indexed.base_space));
	pcolor_space->params.indexed.hival = hival;
	pcolor_space->params.indexed.use_proc = use_proc;
	pcs = pcolor_space;
    }
    *ppcs = pcs;
out:
    pcb->ptr = cbp;
    return code;
}

private int
read_begin_image(command_buf_t *pcb, gs_image_common_t *pic,
		 const gs_color_space *pcs)
{
    uint index = *(pcb->ptr)++;
    const gx_image_type_t *image_type = gx_image_type_table[index];
    stream s;
    int code;

    /* This is sloppy, but we don't have enough information to do better. */
    pcb->ptr = top_up_cbuf(pcb, pcb->ptr);
    sread_string(&s, pcb->ptr, pcb->end - pcb->ptr);
    code = image_type->sget(pic, &s, pcs);
    pcb->ptr = sbufptr(&s);
    return code;
}

private int
read_put_params(command_buf_t *pcb, gs_imager_state *pis,
		gx_device_clist_reader *cdev, gs_memory_t *mem)
{
    const byte *cbp = pcb->ptr;
    gs_c_param_list param_list;
    uint cleft;
    uint rleft;
    bool alloc_data_on_heap = false;
    byte *param_buf;
    uint param_length;
    int code = 0;

    cmd_get_value(param_length, cbp);
    if_debug1('L', " length=%d\n", param_length);
    if (param_length == 0) {
	code = 1;		/* empty list */
	goto out;
    }

    /* Make sure entire serialized param list is in cbuf */
    /* + force void* alignment */
    cbp = top_up_cbuf(pcb, cbp);
    if (pcb->end - cbp >= param_length) {
	param_buf = (byte *)cbp;
	cbp += param_length;
    } else {
	/* NOTE: param_buf must be maximally aligned */
	param_buf = gs_alloc_bytes(mem, param_length,
				   "clist put_params");
	if (param_buf == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto out;
	}
	alloc_data_on_heap = true;
	cleft = pcb->end - cbp;
	rleft = param_length - cleft;
	memmove(param_buf, cbp, cleft);
	pcb->end_status = sgets(pcb->s, param_buf + cleft, rleft, &rleft);
	cbp = pcb->end;  /* force refill */
    }

    /*
     * Create a gs_c_param_list & expand into it.
     * NB that gs_c_param_list doesn't copy objects into
     * it, but rather keeps *pointers* to what's passed.
     * That's OK because the serialized format keeps enough
     * space to hold expanded versions of the structures,
     * but this means we cannot deallocate source buffer
     * until the gs_c_param_list is deleted.
     */
    gs_c_param_list_write(&param_list, mem);
    code = gs_param_list_unserialize
	( (gs_param_list *)&param_list, param_buf );
    if (code >= 0 && code != param_length)
	code = gs_error_unknownerror;  /* must match */
    if (code >= 0) {
	gs_c_param_list_read(&param_list);
	code = gs_imager_putdeviceparams(pis, (gx_device *)cdev,
					 (gs_param_list *)&param_list);
    }
    gs_c_param_list_release(&param_list);
    if (alloc_data_on_heap)
	gs_free_object(mem, param_buf, "clist put_params");

out:
    pcb->ptr = cbp;
    return code;
}

/* ---------------- Utilities ---------------- */

/* Read and unpack a short bitmap */
private const byte *
cmd_read_short_bits(command_buf_t *pcb, byte *data, int width_bytes,
		    int height, uint raster, const byte *cbp)
{
    uint bytes = width_bytes * height;
    const byte *pdata = data /*src*/ + bytes;
    byte *udata = data /*dest*/ + height * raster;

    cbp = cmd_read_data(pcb, data, width_bytes * height, cbp);
    while (--height >= 0) {
	udata -= raster, pdata -= width_bytes;
	switch (width_bytes) {
	    default:
		memmove(udata, pdata, width_bytes);
		break;
	    case 6:
		udata[5] = pdata[5];
	    case 5:
		udata[4] = pdata[4];
	    case 4:
		udata[3] = pdata[3];
	    case 3:
		udata[2] = pdata[2];
	    case 2:
		udata[1] = pdata[1];
	    case 1:
		udata[0] = pdata[0];
	    case 0:;		/* shouldn't happen */
	}
    }
    return cbp;
}

/* Read a rectangle. */
private const byte *
cmd_read_rect(int op, gx_cmd_rect * prect, const byte * cbp)
{
    cmd_getw(prect->x, cbp);
    if (op & 0xf)
	prect->y += ((op >> 2) & 3) - 2;
    else {
	cmd_getw(prect->y, cbp);
    }
    cmd_getw(prect->width, cbp);
    if (op & 0xf)
	prect->height += (op & 3) - 2;
    else {
	cmd_getw(prect->height, cbp);
    }
    return cbp;
}

/* Read a transformation matrix. */
private const byte *
cmd_read_matrix(gs_matrix * pmat, const byte * cbp)
{
    stream s;

    sread_string(&s, cbp, 1 + sizeof(*pmat));
    sget_matrix(&s, pmat);
    return cbp + stell(&s);
}

/* Select a map for loading with data. */
private int
cmd_select_map(cmd_map_index map_index, cmd_map_contents cont,
	       gs_imager_state * pis, gx_ht_order * porder, frac ** pmdata,
	       uint * pcount, gs_memory_t * mem)
{
    gx_transfer_map *map;
    gx_transfer_map **pmap;
    const char *cname;

    switch (map_index) {
	case cmd_map_transfer:
	    if_debug0('L', " transfer");
	    map = pis->set_transfer.colored.gray;
	    pis->effective_transfer.indexed[0] =
		pis->effective_transfer.indexed[1] =
		pis->effective_transfer.indexed[2] =
		pis->effective_transfer.indexed[3] =
		map;
	    goto transfer;
	case cmd_map_transfer_0:
	case cmd_map_transfer_1:
	case cmd_map_transfer_2:
	case cmd_map_transfer_3:
	    {
		int i = map_index - cmd_map_transfer_0;

		if_debug1('L', " transfer[%d]", i);
		rc_unshare_struct(pis->set_transfer.indexed[i], gx_transfer_map,
				  &st_transfer_map, mem,
				  return_error(gs_error_VMerror),
				  "cmd_select_map(transfer)");
		map = pis->set_transfer.indexed[i];
		pis->effective_transfer.indexed[i] = map;
	    }
transfer:   if (cont != cmd_map_other) {
		gx_set_identity_transfer(map);
		*pmdata = 0;
		*pcount = 0;
		return 0;
	    }
	    break;
	case cmd_map_ht_transfer:
	    if_debug0('L', " ht transfer");
	    /* Halftone transfer maps are never shared, but */
	    /* rc_unshare_struct is a good way to get one allocated */
	    /* if it hasn't been yet. */
	    pmap = &porder->transfer;
	    cname = "cmd_select_map(ht transfer)";
	    goto alloc;
	case cmd_map_black_generation:
	    if_debug0('L', " black generation");
	    pmap = &pis->black_generation;
	    cname = "cmd_select_map(black generation)";
	    goto alloc;
	case cmd_map_undercolor_removal:
	    if_debug0('L', " undercolor removal");
	    pmap = &pis->undercolor_removal;
	    cname = "cmd_select_map(undercolor removal)";
alloc:	    if (cont == cmd_map_none) {
		rc_decrement(*pmap, cname);
		*pmap = 0;
		*pmdata = 0;
		*pcount = 0;
		return 0;
	    }
	    rc_unshare_struct(*pmap, gx_transfer_map, &st_transfer_map,
			      mem, return_error(gs_error_VMerror), cname);
	    map = *pmap;
	    if (cont == cmd_map_identity) {
		gx_set_identity_transfer(map);
		*pmdata = 0;
		*pcount = 0;
		return 0;
	    }
	    break;
	default:
	    *pmdata = 0;
	    return 0;
    }
    map->proc = gs_mapped_transfer;
    *pmdata = map->values;
    *pcount = sizeof(map->values);
    return 0;
}

/* Create a device halftone for the imager if necessary. */
private int
cmd_create_dev_ht(gx_device_halftone **ppdht, gs_memory_t *mem)
{
    gx_device_halftone *pdht = *ppdht;

    if (pdht == 0) {
	rc_header rc;

	rc_alloc_struct_1(pdht, gx_device_halftone, &st_device_halftone, mem,
			  return_error(gs_error_VMerror),
			  "cmd_create_dev_ht");
	rc = pdht->rc;
	memset(pdht, 0, sizeof(*pdht));
	pdht->rc = rc;
	*ppdht = pdht;
    }
    return 0;
}

/* Resize the halftone components array if necessary. */
private int
cmd_resize_halftone(gx_device_halftone **ppdht, uint num_comp,
		    gs_memory_t * mem)
{
    int code = cmd_create_dev_ht(ppdht, mem);
    gx_device_halftone *pdht = *ppdht;

    if (code < 0)
	return code;
    if (num_comp != pdht->num_comp) {
	gx_ht_order_component *pcomp;

	/*
	 * We must be careful not to shrink or free the components array
	 * before releasing any relevant elements.
	 */
	if (num_comp < pdht->num_comp) {
	    uint i;

	    /* Don't release the default order. */
	    for (i = pdht->num_comp; i-- > num_comp;)
		if (pdht->components[i].corder.bit_data != pdht->order.bit_data)
		    gx_ht_order_release(&pdht->components[i].corder, mem, true);
	    if (num_comp == 0) {
		gs_free_object(mem, pdht->components, "cmd_resize_halftone");
		pcomp = 0;
	    } else {
		pcomp = gs_resize_object(mem, pdht->components, num_comp,
					 "cmd_resize_halftone");
		if (pcomp == 0) {
		    pdht->num_comp = num_comp;	/* attempt consistency */
		    return_error(gs_error_VMerror);
		}
	    }
	} else {
	    /* num_comp > pdht->num_comp */
	    if (pdht->num_comp == 0)
		pcomp = gs_alloc_struct_array(mem, num_comp,
					      gx_ht_order_component,
					      &st_ht_order_component_element,
					      "cmd_resize_halftone");
	    else
		pcomp = gs_resize_object(mem, pdht->components, num_comp,
					 "cmd_resize_halftone");
	    if (pcomp == 0)
		return_error(gs_error_VMerror);
	    memset(&pcomp[pdht->num_comp], 0,
		   sizeof(*pcomp) * (num_comp - pdht->num_comp));
	}
	pdht->num_comp = num_comp;
	pdht->components = pcomp;
    }
    return 0;
}

/* ------ Path operations ------ */

/* Decode a path segment. */
private int
clist_decode_segment(gx_path * ppath, int op, fixed vs[6],
		 gs_fixed_point * ppos, int x0, int y0, segment_notes notes)
{
    fixed px = ppos->x - int2fixed(x0);
    fixed py = ppos->y - int2fixed(y0);
    int code;

#define A vs[0]
#define B vs[1]
#define C vs[2]
#define D vs[3]
#define E vs[4]
#define F vs[5]

    switch (op) {
	case cmd_opv_rmoveto:
	    code = gx_path_add_point(ppath, px += A, py += B);
	    break;
	case cmd_opv_rlineto:
	    code = gx_path_add_line_notes(ppath, px += A, py += B, notes);
	    break;
	case cmd_opv_hlineto:
	    code = gx_path_add_line_notes(ppath, px += A, py, notes);
	    break;
	case cmd_opv_vlineto:
	    code = gx_path_add_line_notes(ppath, px, py += A, notes);
	    break;
	case cmd_opv_rmlineto:
	    if ((code = gx_path_add_point(ppath, px += A, py += B)) < 0)
		break;
	    code = gx_path_add_line_notes(ppath, px += C, py += D, notes);
	    break;
	case cmd_opv_rm2lineto:
	    if ((code = gx_path_add_point(ppath, px += A, py += B)) < 0 ||
		(code = gx_path_add_line_notes(ppath, px += C, py += D,
					       notes)) < 0
		)
		break;
	    code = gx_path_add_line_notes(ppath, px += E, py += F, notes);
	    break;
	case cmd_opv_rm3lineto:
	    if ((code = gx_path_add_point(ppath, px += A, py += B)) < 0 ||
		(code = gx_path_add_line_notes(ppath, px += C, py += D,
					       notes)) < 0 ||
		(code = gx_path_add_line_notes(ppath, px += E, py += F,
					       notes)) < 0
		)
		break;
	    code = gx_path_add_line_notes(ppath, px -= C, py -= D, notes);
	    break;
	case cmd_opv_rrcurveto:	/* a b c d e f => a b a+c b+d a+c+e b+d+f */
rrc:	    E += (C += A);
	    F += (D += B);
curve:	    code = gx_path_add_curve_notes(ppath, px + A, py + B,
					   px + C, py + D,
					   px + E, py + F, notes);
	    px += E, py += F;
	    break;
	case cmd_opv_hvcurveto:	/* a b c d => a 0 a+b c a+b c+d */
hvc:	    F = C + D, D = C, E = C = A + B, B = 0;
	    goto curve;
	case cmd_opv_vhcurveto:	/* a b c d => 0 a b a+c b+d a+c */
vhc:	    E = B + D, F = D = A + C, C = B, B = A, A = 0;
	    goto curve;
	case cmd_opv_nrcurveto:	/* a b c d => 0 0 a b a+c b+d */
	    F = B + D, E = A + C, D = B, C = A, B = A = 0;
	    goto curve;
	case cmd_opv_rncurveto:	/* a b c d => a b a+c b+d a+c b+d */
	    F = D += B, E = C += A;
	    goto curve;
	case cmd_opv_vqcurveto:	/* a b => VH a b TS(a,b) TS(b,a) */
	    if ((A ^ B) < 0)
		C = -B, D = -A;
	    else
		C = B, D = A;
	    goto vhc;
	case cmd_opv_hqcurveto:	/* a b => HV a TS(a,b) b TS(b,a) */
	    if ((A ^ B) < 0)
		D = -A, C = B, B = -B;
	    else
		D = A, C = B;
	    goto hvc;
	case cmd_opv_scurveto: /* (a b c d e f) => */
	    {
		fixed a = A, b = B;

		/* See gxclpath.h for details on the following. */
		if (A == 0) {
		    /* Previous curve was vh or vv */
		    A = E - C, B = D - F, C = C - a, D = b - D, E = a, F = -b;
		} else {
		    /* Previous curve was hv or hh */
		    A = C - E, B = F - D, C = a - C, D = D - b, E = -a, F = b;
		}
	    }
	    goto rrc;
	case cmd_opv_closepath:
	    code = gx_path_close_subpath(ppath);
	    gx_path_current_point(ppath, (gs_fixed_point *) vs);
	    px = A, py = B;
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }
#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
    ppos->x = px + int2fixed(x0);
    ppos->y = py + int2fixed(y0);
    return code;
}

/*
 * Execute a polyfill -- either a fill_parallelogram or a fill_triangle.
 * If we ever implement fill_trapezoid in the band list, that will be
 * detected here too.
 *
 * Note that degenerate parallelograms or triangles may collapse into
 * a single line or point.  We must check for this so we don't try to
 * access non-existent segments.
 */
private int
clist_do_polyfill(gx_device *dev, gx_path *ppath,
		  const gx_drawing_color *pdcolor,
		  gs_logical_operation_t lop)
{
    const subpath *psub = ppath->first_subpath;
    const segment *pseg1;
    const segment *pseg2;
    int code;

    if (psub && (pseg1 = psub->next) != 0 && (pseg2 = pseg1->next) != 0) {
	fixed px = psub->pt.x, py = psub->pt.y;
	fixed ax = pseg1->pt.x - px, ay = pseg1->pt.y - py;
	fixed bx, by;
	/*
	 * We take advantage of the fact that the parameter lists for
	 * fill_parallelogram and fill_triangle are identical.
	 */
	dev_proc_fill_parallelogram((*fill));

	if (pseg2->next) {
	    /* Parallelogram */
	    fill = dev_proc(dev, fill_parallelogram);
	    bx = pseg2->pt.x - pseg1->pt.x;
	    by = pseg2->pt.y - pseg1->pt.y;
	} else {
	    /* Triangle */
	    fill = dev_proc(dev, fill_triangle);
	    bx = pseg2->pt.x - px;
	    by = pseg2->pt.y - py;
	}
	code = fill(dev, px, py, ax, ay, bx, by, pdcolor, lop);
    } else
	code = 0;
    gx_path_new(ppath);
    return code;
}
