/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxclread.c */
/* Command list reading for Ghostscript. */
#include "memory_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gsdevice.h"			/* for gs_deviceinitialmatrix */
#include "gxdevmem.h"			/* must precede gxcldev.h */
#include "gxcldev.h"
#include "strimpl.h"
#include "srlx.h"

/* Imported from gxclist.c */
int clist_flush_buffer(P1(gx_device_clist *));

/* Print a bitmap for tracing */
#ifdef DEBUG
private void
cmd_print_bits(const byte *data, int height, int raster)
{	int i, j;
	for ( i = 0; i < height; i++ )
	   {	const byte *row = data + i * raster;
		dprintf("[L]");
		for ( j = 0; j < raster; j++ )
		  dprintf1(" %02x", row[j]);
		dputc('\n');
	   }
}
#else
#  define cmd_print_bits(data, height, raster)
#endif

/* ------ Reading/rendering ------ */

private int clist_render_init(P1(gx_device_clist *));
private int clist_render(P3(gx_device_clist *, gx_device *, int));

/* Copy a scan line to the client.  This is where rendering gets done. */
int
clist_get_bits(gx_device *dev, int y, byte *str, byte **actual_data)
{	gx_device_memory *mdev = &cdev->mdev;
	/* Initialize for rendering if we haven't done so yet. */
	if ( cdev->ymin < 0 )
	{	int code = clist_render_init(cdev);
		if ( code < 0 ) return code;
	}
	/* Render a band if necessary, and copy it incrementally. */
	if ( !(y >= cdev->ymin && y < cdev->ymax) )
	   {	int band = y / mdev->height;
		int code;
		rewind(cdev->bfile);
		(*dev_proc(mdev, open_device))((gx_device *)mdev);	/* reinitialize */
		code = clist_render(cdev, (gx_device *)mdev, band);
		if ( code < 0 ) return code;
		cdev->ymin = band * mdev->height;
		cdev->ymax = cdev->ymin + mdev->height;
	   }
	return (*dev_proc(mdev, get_bits))((gx_device *)mdev,
				y - cdev->ymin, str, actual_data);
}

#undef cdev

/* Initialize for reading. */
private int
clist_render_init(gx_device_clist *cdev)
{	gx_device *target = cdev->target;
	byte *base = cdev->mdev.base;	/* save */
	int depth = target->color_info.depth;
	uint raster = gx_device_raster(target, 1);
	const gx_device_memory *mdev = gdev_mem_device_for_bits(depth);
	int code;
	if ( mdev == 0 )
		return_error(gs_error_rangecheck);
	code = clist_flush_buffer(cdev);		/* flush buffer */
	if ( code < 0 ) return code;
	/* Write the terminating entry in the block file. */
	/* Note that because of copypage, there may be many such entries. */
	   {	cmd_block cb;
		cb.band = -1;
		cb.pos = ftell(cdev->cfile);
		clist_write(cdev->bfile, (byte *)&cb, sizeof(cb));
		cdev->bfile_end_pos = ftell(cdev->bfile);
	   }
	gs_make_mem_device(&cdev->mdev, mdev, 0, 0, target);
	cdev->mdev.base = base;		/* restore */
	/* The matrix in the memory device is irrelevant, */
	/* because all we do with the device is call the device-level */
	/* output procedures, but we may as well set it to */
	/* something halfway reasonable. */
	gs_deviceinitialmatrix(target, &cdev->mdev.initial_matrix);
	cdev->mdev.width = target->width;
	cdev->mdev.height = cdev->band_height;
	cdev->mdev.raster = raster;
	cdev->ymin = cdev->ymax = 0;
#ifdef DEBUG
if ( gs_debug_c('l') )
	{	extern void cmd_print_stats(P0());
		cmd_print_stats();
	}
#endif
	return 0;
}

/* Render one band to a specified target device. */
typedef byte _ss *cb_ptr;
#define cmd_getw(var, p)\
{ int _shift = 0; for ( var = 0; var += (int)(*p & 0x7f) << _shift,\
    *p++ > 0x7f; _shift += 7 ) ;\
}
private cb_ptr cmd_read_rect(P2(gx_cmd_rect *, cb_ptr));
private void clist_read(P3(FILE *, byte *, uint));
private void clist_unpack_short_bits(P3(byte *, int, int));
private int
clist_render(gx_device_clist *cdev, gx_device *tdev, int band)
{	byte cbuf[cbuf_size];
		/* 'bits' is for short copy_mono bits and copy_mono_rle, */
		/* must be aligned */
	long bits[max(255, cbuf_size / sizeof(long) + 1)];
	register cb_ptr cbp;
	cb_ptr cb_limit;
	cb_ptr cb_end;
	FILE *file = cdev->cfile;
	FILE *bfile = cdev->bfile;
	int y0 = band * cdev->band_height;
	gx_clist_state state;
	gx_tile_bitmap state_tile;
	int state_tile_depth;
	uint tile_bits_size;		/* size of bits of each tile */
	gs_int_point tile_phase;
	cmd_block b_this;
	long pos;
	uint left;
#define cmd_read(ptr, rsize, cbp)\
  if ( cb_end - cbp >= (rsize) )\
    memcpy(ptr, cbp, rsize), cbp += rsize;\
  else\
   { uint cleft = cb_end - cbp, rleft = (rsize) - cleft;\
     memcpy(ptr, cbp, cleft);\
     clist_read(file, ptr + cleft, rleft); left -= rleft;\
     cbp = cb_end;\
   }
#define cmd_read_short_bits(ptr, bw, ht, cbp)\
  cmd_read(ptr, (bw) * (ht), cbp);\
  clist_unpack_short_bits(ptr, bw, ht)
	state = cls_initial;
	state_tile.id = gx_no_bitmap_id;
	tile_phase.x = tile_phase.y = 0;
trd:	clist_read(bfile, (byte *)&b_this, sizeof(b_this));
top:	/* Find the next run of commands for this band. */
	if ( b_this.band < 0 && ftell(bfile) == cdev->bfile_end_pos )
		return 0;	/* end of bfile */
	if ( b_this.band != band ) goto trd;
	pos = b_this.pos;
	clist_read(bfile, (byte *)&b_this, sizeof(b_this));
	fseek(file, pos, SEEK_SET);
	left = (uint)(b_this.pos - pos);
	cb_limit = cbuf + (cbuf_size - cmd_largest_size);
	cb_end = cbuf + cbuf_size;
	cbp = cb_end;
	for ( ; ; )
	   {	int op;
		uint bytes;
		int data_x, raster;
		int code;
		cb_ptr source;
		gx_color_index _ss *pcolor;
		/* Make sure the buffer contains a full command. */
		if ( cbp > cb_limit )
		{	uint nread;
			memcpy(cbuf, cbp, cb_end - cbp);
			cbp = cbuf + (cb_end - cbp);
			nread = cb_end - cbp;
			if ( nread > left ) nread = left;
			clist_read(file, cbp, nread);
			cb_end = cbp + nread;
			cbp = cbuf;
			left -= nread;
			if ( cb_limit > cb_end ) cb_limit = cb_end;
			process_interrupts();
		}
		op = *cbp++;
		if_debug2('L', "[L]%s %d:\n",
			  cmd_op_names[op >> 4], op & 0xf);
		switch ( op >> 4 )
		   {
		case cmd_op_misc >> 4:
			switch ( op )
			   {
			case cmd_opv_end_run:
				goto top;
			case cmd_opv_set_tile_size_colored:
				state_tile_depth = cdev->color_info.depth;
				goto sts;
			case cmd_opv_set_tile_size:
				state_tile_depth = 1;
sts:				cmd_getw(state_tile.size.x, cbp);
				cmd_getw(state_tile.size.y, cbp);
				state_tile.raster =
				  bitmap_raster(state_tile.size.x * state_tile_depth);
				/* We can't actually know the rep_size, */
				/* so we play it safe. */
				state_tile.rep_width = state_tile.size.x;
				state_tile.rep_height = state_tile.size.y;
				cdev->tile_slot_size = tile_bits_size =
					state_tile.raster * state_tile.size.y;
				break;
			case cmd_opv_set_tile_phase:
				cmd_getw(state.tile_phase.x, cbp);
				cmd_getw(state.tile_phase.y, cbp);
				break;
			case cmd_opv_set_tile_index:
				state_tile.data =
				  (byte *)tile_slot_ptr(cdev, *cbp);
				cbp++;
				continue;
			case cmd_opv_set_tile_bits:
				state_tile.data =
				  (byte *)tile_slot_ptr(cdev, *cbp);
				cbp++;
				if ( state_tile.size.x * state_tile_depth <= 16 )
				  {	cmd_read_short_bits(state_tile.data, 2,
						state_tile.size.y, cbp);
				  }
				else
				  {	bytes = tile_bits_size;
					cmd_read(state_tile.data, bytes, cbp);
				  }
#ifdef DEBUG
if ( gs_debug_c('L') )
				cmd_print_bits(state_tile.data,
					       state_tile.size.y,
					       state_tile.raster);
#endif
				continue;
			default:
				goto bad_op;
			   }
			tile_phase.x = state.tile_phase.x % state_tile.size.x;
			tile_phase.y = (state.tile_phase.y + y0) % state_tile.size.y;
			continue;
		case cmd_op_set_color0 >> 4:
			pcolor = &state.color0;
			goto set_color;
		case cmd_op_set_color1 >> 4:
			pcolor = &state.color1;
set_color:		if ( op & 0xf )
				*pcolor = (gx_color_index)(long)((op & 0xf) - 2);
			else
			{	memcpy(pcolor, cbp, sizeof(gx_color_index));
				cbp += sizeof(gx_color_index);
			}
			continue;
		case cmd_op_copy_mono >> 4:
			if ( op & 0xf )
			   {	cmd_getw(state.rect.x, cbp);
				cmd_getw(state.rect.y, cbp);
				state.rect.width = *cbp++;
				state.rect.height = *cbp++;
				break;
			   }
			/* falls through */
		case cmd_op_fill_rect >> 4:
		case cmd_op_tile_rect >> 4:
		case cmd_op_copy_mono_rle >> 4:
		case cmd_op_copy_color >> 4:
			cbp = cmd_read_rect(&state.rect, cbp);
			break;
		case cmd_op_fill_rect_short >> 4:
		case cmd_op_tile_rect_short >> 4:
			state.rect.x += *cbp + cmd_min_short;
			state.rect.width += cbp[1] + cmd_min_short;
			if ( op & 0xf )
			   {	state.rect.height += (op & 0xf) + cmd_min_tiny;
				cbp += 2;
			   }
			else
			   {	state.rect.y += cbp[2] + cmd_min_short;
				state.rect.height += cbp[3] + cmd_min_short;
				cbp += 4;
			   }
			break;
		case cmd_op_fill_rect_tiny >> 4:
		case cmd_op_tile_rect_tiny >> 4:
		   {	int txy = *cbp++;
			state.rect.x += (txy >> 4) + cmd_min_tiny;
			state.rect.y += (txy & 0xf) + cmd_min_tiny;
			state.rect.width += (op & 0xf) + cmd_min_tiny;
		   }	break;
		case cmd_op_delta_tile_bits >> 4:
		   {	byte *new_data = (byte *)tile_slot_ptr(cdev, *cbp);
			cbp++;
			memcpy(new_data, state_tile.data, tile_bits_size);
			state_tile.data = new_data;
			do
			   {	uint offset = *cbp;
				if ( offset < 0x80 )
					new_data[offset] ^= cbp[1],
					cbp += 2;
				else
					offset -= 0x80,
					new_data[offset] ^= cbp[1],
					new_data[offset + 1] ^= cbp[2],
					cbp += 3;
			   }
			while ( op-- & 0xf );
		   }	continue;
		default:
bad_op:			lprintf5("Bad op %02x band %d file pos %ld buf pos %d/%d\n",
				 op, band, ftell(file), (int)(cbp - cbuf), (int)(cb_end - cbuf));
			   {	cb_ptr pp;
				for ( pp = cbuf; pp < cb_end; pp += 10 )
				  lprintf10(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					   pp[0], pp[1], pp[2], pp[3], pp[4],
					   pp[5], pp[6], pp[7], pp[8], pp[9]);
			   }
			return_error(gs_error_Fatal);
		   }
		if_debug4('L', "[L]  x=%d y=%d w=%d h=%d\n",
			  state.rect.x, state.rect.y, state.rect.width,
			  state.rect.height);
		switch ( op >> 4 )
		   {
		case cmd_op_fill_rect >> 4:
		case cmd_op_fill_rect_short >> 4:
		case cmd_op_fill_rect_tiny >> 4:
			code = (*dev_proc(tdev, fill_rectangle))
			  (tdev, state.rect.x, state.rect.y - y0,
			   state.rect.width, state.rect.height, state.color1);
			break;
		case cmd_op_tile_rect >> 4:
		case cmd_op_tile_rect_short >> 4:
		case cmd_op_tile_rect_tiny >> 4:
			code = (*dev_proc(tdev, tile_rectangle))
			  (tdev, &state_tile,
			   state.rect.x, state.rect.y - y0,
			   state.rect.width, state.rect.height,
			   state.color0, state.color1,
			   tile_phase.x, tile_phase.y);
			break;
		case cmd_op_copy_mono >> 4:
			if ( op & 0xf )
			   {	data_x = (op & 0xf) - 1;
				raster = 4;
				cmd_read_short_bits((byte *)bits, (data_x + state.rect.width + 7) >> 3, state.rect.height, cbp);
				source = (byte *)bits;
				goto copy;
			   }
			/* falls through */
		case cmd_op_copy_mono_rle >> 4:
		case cmd_op_copy_color >> 4:
			cmd_getw(data_x, cbp);
			cmd_getw(raster, cbp);
			bytes = state.rect.height * raster;
			/* copy_mono[_rle] and copy_color have ensured that */
			/* the bits will fit in a single buffer. */
#ifdef DEBUG
			if ( bytes > cbuf_size )
			{	lprintf2("bitmap size exceeds buffer!  raster=%d height=%d\n",
					raster, state.rect.height);
				return_error(gs_error_ioerror);
			}
#endif
			if ( op >> 4 == (byte)cmd_op_copy_mono_rle >> 4 )
			  {	/* Decompress the image data. */
				stream_RLD_state sstate;
				stream_cursor_read r;
				stream_cursor_write w;
				/* We don't know the data length a priori, */
				/* so to be conservative, we read */
				/* the lesser of the uncompressed size and */
				/* the amount left in this command run. */
				uint cleft = cb_end - cbp;
				if ( cleft < bytes && left != 0 )
				  {	uint nread = cbuf_size - cleft;
					memcpy(cbuf, cbp, cleft);
					if ( nread > left ) nread = left;
					clist_read(file, cbuf + cleft, nread);
					left -= nread;
					cb_end = cbuf + cleft + nread;
					cbp = cbuf;
				  }
				s_RLD_init_inline(&sstate);
				r.ptr = cbp - 1;
				r.limit = cb_end - 1;
				w.ptr = (byte *)bits - 1;
				w.limit = w.ptr + sizeof(bits);
				/* The process procedure can't fail. */
				(*s_RLD_template.process)
				  ((stream_state *)&sstate, &r, &w, true);
				cbp = (cb_ptr)r.ptr + 1;
				source = (byte *)bits;
			  }
			else
			  {	cmd_read(cbuf, bytes, cbp);
				source = cbuf;
			  }
copy:
#ifdef DEBUG
if ( gs_debug_c('L') )
   {			dprintf2("[L]  data_x=%d raster=%d\n",
				 data_x, raster);
			cmd_print_bits(source, state.rect.height, raster);
   }
#endif
			code = (op >> 4 == (byte)cmd_op_copy_color >> 4 ?
			  (*dev_proc(tdev, copy_color))
			    (tdev, source, data_x, raster, gx_no_bitmap_id,
			     state.rect.x, state.rect.y - y0,
			     state.rect.width, state.rect.height) :
			  (*dev_proc(tdev, copy_mono))
			    (tdev, source, data_x, raster, gx_no_bitmap_id,
			     state.rect.x, state.rect.y - y0,
			     state.rect.width, state.rect.height,
			     state.color0, state.color1));
			break;
		default:		/* can't happen */
			return_error(gs_error_ioerror);
		   }
		if ( code < 0 )
		  return_error(code);
	   }
}

/* The typical implementations of fread and fseek */
/* are extremely inefficient for small counts, */
/* so we use loops instead. */
private void
clist_read(FILE *f, byte *str, uint len)
{	switch ( len )
	   {
	default: fread(str, 1, len, f); break;
	case 8: *str++ = (byte)getc(f);
	case 7: *str++ = (byte)getc(f);
	case 6: *str++ = (byte)getc(f);
	case 5: *str++ = (byte)getc(f);
	case 4: *str++ = (byte)getc(f);
	case 3: *str++ = (byte)getc(f);
	case 2: *str++ = (byte)getc(f);
	case 1: *str = (byte)getc(f);
	   }
}

/* Unpack a short bitmap */
private void
clist_unpack_short_bits(byte *data, register int bwidth, int height)
{	uint bytes = bwidth * height;
	byte *pdata = data + bytes;
	byte *udata = data + (height << log2_align_bitmap_mod);
	while ( --height > 0 )		/* first row is in place already */
	   {	udata -= align_bitmap_mod, pdata -= bwidth;
		switch ( bwidth )
		   {
		case 3: udata[2] = pdata[2];
		case 2: udata[1] = pdata[1];
		case 1: udata[0] = pdata[0];
		   }
	   }
}

/* Read a rectangle. */
private cb_ptr
cmd_read_rect(register gx_cmd_rect *prect, register cb_ptr cbp)
{	cmd_getw(prect->x, cbp);
	cmd_getw(prect->y, cbp);
	cmd_getw(prect->width, cbp);
	cmd_getw(prect->height, cbp);
	return cbp;
}
