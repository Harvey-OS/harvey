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

/* gsimage1.c */
/* Image procedures for Ghostscript library */
/* This file is logically part of gsimage.c; we have split it out */
/* to reduce the code working set. */
#include "gx.h"
#include "memory_.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxarith.h"
#include "gxmatrix.h"
#include "gscspace.h"
#include "gsccolor.h"
#include "gspaint.h"
#include "gzstate.h"
#include "gxcmap.h"
#include "gzpath.h"
#include "gxcpath.h"
#include "gxdevmem.h"
#include "gximage.h"
#include "gzht.h"
#include "gxdraw.h"

/* Process the next piece of an image */
int
gs_image_next(register gs_image_enum *penum, const byte *dbytes, uint dsize,
  uint *pused)
{	uint rsize = penum->bytes_per_row;
	uint pos = penum->byte_in_row;
	int width = penum->width;
	int nplanes = penum->num_planes;
	uint dleft = dsize;
	uint dpos = 0;
	gs_state *pgs = penum->pgs;
	gx_device *save_dev = 0;
	fixed xcur_save, ycur_save;
	int y_save;
	int code;
	/* Accumulate separated colors, if needed */
	if ( penum->plane_index != 0 )
	  if ( dsize != penum->planes[0].size )
	    return_error(gs_error_undefinedresult);
	penum->planes[penum->plane_index].data = dbytes;
	penum->planes[penum->plane_index].size = dsize;
	if ( ++(penum->plane_index) != nplanes )
		return 0;
	penum->plane_index = 0;
	/* Save the dynamic state components in case of an error. */
	xcur_save = penum->xcur;
	ycur_save = penum->ycur;
	y_save = penum->y;
	/* We've accumulated an entire set of planes. */
	if ( !penum->never_clip )
	   {	/* Install the clipping device if needed. */
		gx_device_clip *cdev = penum->clip_dev;
		save_dev = gs_currentdevice(pgs);
		cdev->target = save_dev;
		gx_set_device_only(pgs, (gx_device *)cdev);
	   }
	while ( dleft )
	   {	/* Fill up a row, then display it. */
		uint bcount = min(dleft, rsize - pos);
		int px;
		for ( px = 0; px < nplanes; px++ )
		  (*penum->unpack)(penum, &penum->map[px],
				   penum->buffer + (px << penum->log2_xbytes),
				   penum->planes[px].data + dpos, bcount, pos);
		pos += bcount;
		dpos += bcount;
		dleft -= bcount;
		if ( pos == rsize )	/* filled an entire row */
		   {
#ifdef DEBUG
if ( gs_debug_c('B') )
   {			int i, n = width * penum->spp;
			dputs("[B]row:");
			for ( i = 0; i < n; i++ )
				dprintf1(" %02x", penum->buffer[i]);
			dputs("\n");
   }
#endif
			if ( !penum->interpolate )
			  switch ( penum->posture )
			  {
			  case image_portrait:
			    {	/* Precompute integer y and height, */
				/* and check for clipping. */
				fixed yc = penum->ycur, yn;
				fixed dyy = penum->fyy;
				fixed adjust = penum->adjust;
				if ( dyy > 0 )
				  dyy += adjust << 1,
				  yc -= adjust;
				else
				  dyy = (adjust << 1) - dyy,
				  yc -= dyy - adjust;
				if ( yc >= penum->clip_box.q.y ) goto mt;
				yn = yc + dyy;
				if ( yn <= penum->clip_box.p.y ) goto mt;
				penum->yci = fixed2int_var_rounded(yc);
				penum->hci =
				  fixed2int_var_rounded(yn) - penum->yci;
				if ( penum->hci == 0 ) goto mt;
			    }	break;
			  case image_landscape:
			    {	/* Check for no pixel centers in x. */
				fixed xc = penum->xcur, xn;
				fixed dyx = penum->fyx;
				fixed adjust = penum->adjust;
				if ( dyx > 0 )
				  dyx += adjust << 1,
				  xc -= adjust;
				else
				  dyx = (adjust << 1) - dyx,
				  xc -= dyx - adjust;
				if ( xc >= penum->clip_box.q.x ) goto mt;
				xn = xc + dyx;
				if ( xn <= penum->clip_box.p.x ) goto mt;
				penum->xci = fixed2int_var_rounded(xc);
				penum->wci =
				  fixed2int_var_rounded(xn) - penum->xci;
				if ( penum->wci == 0 ) goto mt;
			    }	break;
			  case image_skewed:
			    ;
			  }
			code = (*penum->render)(penum, penum->buffer, width * penum->spp, 1);
			if ( code < 0 ) goto err;
mt:			if ( ++(penum->y) == penum->height ) goto end;
			pos = 0;
			penum->xcur += penum->fyx;
			penum->ycur += penum->fyy;
		   }
	   }
	penum->byte_in_row = pos;
	code = 0;
	goto out;
end:	/* End of data */
	code = 1;
	goto out;
err:	/* Error or interrupt, restore original state. */
	penum->plane_index = penum->spread - 1;
	penum->xcur = xcur_save;
	penum->ycur = ycur_save;
	penum->y = y_save;
	/* Note that caller must call gs_image_cleanup */
	/* for both error and normal termination. */
out:	if ( save_dev != 0 )
		gx_set_device_only(pgs, save_dev);
	if ( code >= 0 )
		*pused = dpos;
	return code;
}

/* Clean up by releasing the buffers. */
void
gs_image_cleanup(register gs_image_enum *penum)
{	gs_memory_t *mem = penum->pgs->memory;
	gs_free_object(mem, penum->clip_dev, "image clipper");
	penum->clip_dev = 0;
	if ( penum->interpolate )
	  gs_image_scale_cleanup(&penum->scale_state);
	gs_free_object(mem, penum->line, "image line");
	penum->line = 0;
	gs_free_object(mem, penum->buffer, "image buffer");
	penum->buffer = 0;
}

/* ------ Unpacking procedures ------ */

void
image_unpack_copy(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, const byte *data, uint dsize, uint inpos)
{	register byte *bufp = bptr + inpos;
	if ( data != bufp )
	  memcpy(bufp, data, dsize);
}

void
image_unpack_1(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register bits32 *bufp = (bits32 *)(bptr + (inpos << 3));
	int left = dsize;
	register const bits32 *map = &pmap->table.lookup4x1to32[0];
	register uint b;
	if ( left & 1 )
	   {	b = data[0];
		bufp[0] = map[b >> 4];
		bufp[1] = map[b & 0xf];
		data++, bufp += 2;
	   }
	left >>= 1;
	while ( left-- )
	   {	b = data[0];
		bufp[0] = map[b >> 4];
		bufp[1] = map[b & 0xf];
		b = data[1];
		bufp[2] = map[b >> 4];
		bufp[3] = map[b & 0xf];
		data += 2, bufp += 4;
	   }
}

void
image_unpack_2(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register bits16 *bufp = (bits16 *)(bptr + (inpos << 2));
	int left = dsize;
	register const bits16 *map = &pmap->table.lookup2x2to16[0];
	while ( left-- )
	   {	register unsigned b = *data++;
		*bufp++ = map[b >> 4];
		*bufp++ = map[b & 0xf];
	   }
}

void
image_unpack_4(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register int spread = penum->spread;
	register byte *bufp = bptr + (inpos << 1) * spread;
	int left = dsize;
	register const byte *map = &pmap->table.lookup8[0];
	while ( left-- )
	   {	register unsigned b = *data++;
		*bufp = map[b >> 4]; bufp += spread;
		*bufp = map[b & 0xf]; bufp += spread;
	   }
}

void
image_unpack_8(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, const byte *data, uint dsize, uint inpos)
{	register byte *bufp = bptr + inpos;
	if ( pmap->table.lookup8[0] != 0 || pmap->table.lookup8[255] != 255 )
	  {	register uint left = dsize;
		register const byte *map = &pmap->table.lookup8[0];
		while ( left-- )
		  *bufp++ = map[*data++];
	  }
	else if ( data != bufp )
	  memcpy(bufp, data, dsize);
}

/* ------ Rendering procedures ------ */

/* Rendering procedure for ignoring an image.  We still need to iterate */
/* over the samples, because the procedure might have side effects. */
int
image_render_skip(gs_image_enum *penum, byte *data, uint w, int h)
{	return h;
}

/* Rendering procedure for a monobit image with no */
/* skew or rotation and pure colors. */
int
image_render_simple(gs_image_enum *penum, byte *buffer, uint w, int h)
{	byte *line = penum->line;
	uint line_size, line_width;
	gx_device *dev = gs_currentdevice(penum->pgs);
	dev_proc_copy_mono((*copy_mono)) = dev_proc(dev, copy_mono);
	int ix = fixed2int_rounded(penum->xcur);
	const int iy = penum->yci, ih = penum->hci;
	gx_color_index
		zero = penum->icolor0.colors.pure,
		one = penum->icolor1.colors.pure;
	int dy;

	if ( penum->map[0].table.lookup4x1to32[0] != 0 )
		zero = penum->icolor1.colors.pure,
		one = penum->icolor0.colors.pure;

	if ( line == 0 )
	{	/* A direct BitBlt is possible. */
		line = buffer;
		line_size = (w + 7) >> 3;
		line_width = w;
	}
	else
	{	fixed xl = penum->xcur + fixed_half - int2fixed(ix);
		const fixed dxx = penum->fxx;
		const fixed dxx_4 = dxx << 2;
		const fixed dxx_8 = dxx_4 << 1;
		register const byte *psrc = buffer;
		byte sbit = 0x80;
		byte *endp = buffer + (w >> 3);
		const byte endbit = 1 << (~w & 7);
		byte data;

		line_size = penum->line_size;
		line_width = penum->line_width;
		if ( dxx < 0 )
			ix -= line_width,
			xl += int2fixed(line_width);

		/* Invert the bit following the last valid data bit. */
		if ( endbit == 0x80 ) *endp = ~endp[-1] << 7;
		else if ( *endp & (endbit << 1) ) *endp &= ~endbit;
		else *endp |= endbit;
		data = *psrc;

		memset(line, 0, line_size);
		/*
		 * Loop invariants:
		 *	data = *psrc;
		 *	sbit = 1 << n, 0<=n<=7.
		 */
		do
		{	int x0, n, bit;
			byte *bp;
			static const byte lmasks[9] =
			 { 0xff, 0x7f, 0x3f, 0x1f, 0xf, 7, 3, 1, 0 };
			static const byte rmasks[8] =
			 { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

			/* Scan a run of zeros. */
			while ( ~data & sbit )
			{	xl += dxx;
				sbit >>= 1;
			}
			if ( !sbit )
			{	while ( (data = *++psrc) == 0 )
					xl += dxx_8;
				if ( data > 0xf )
					sbit = 0x80;
				else
					xl += dxx_4,
					sbit = 0x08;
				while ( ~data & sbit )
				{	xl += dxx;
					sbit >>= 1;
				}
			}
			if ( !(psrc < endp || sbit > endbit) )
				break;
			x0 = fixed2int_var(xl);

			/* Scan a run of ones. */
			while ( data & sbit )
			{	xl += dxx;
				sbit >>= 1;
			}
			if ( !sbit )
			{	while ( (data = *++psrc) == 0xff )
					xl += dxx_8;
				if ( data < 0xf0 )
					sbit = 0x80;
				else
					xl += dxx_4,
					sbit = 0x08;
				while ( data & sbit )
				{	xl += dxx;
					sbit >>= 1;
				}
			}

			/* Fill the run in the scan line. */
			n = fixed2int_var(xl) - x0;
			if ( n < 0 ) x0 += n, n = -n;
			bp = line + (x0 >> 3);
			bit = x0 & 7;
			if ( (n += bit) <= 8 )
				*bp |= lmasks[bit] - lmasks[n];
			else
			{	*bp++ |= lmasks[bit];
				if ( n > 64 )
				{	int nb = (n >> 3) - 1;
					memset(bp, 0xff, nb);
					bp += nb;
					n &= 7;
				}
				else
				{	n -= 8;
					while ( (n -= 8) >= 0 )
						*bp++ = 0xff;
				}
				*bp |= rmasks[n & 7];
			}

		} while ( psrc < endp || sbit > endbit );
	}

	/* Finally, transfer the scan line to the device. */
	for ( dy = 0; dy < ih; dy++ )
		(*copy_mono)(dev, line, 0, line_size, gx_no_bitmap_id,
			ix, iy + dy, line_width, 1, zero, one);

	return_check_interrupt(1);
}

/* Rendering procedure for the general case of displaying a */
/* monochrome image, dealing with multiple bit-per-sample images, */
/* general transformations, and arbitrary single-component */
/* color spaces (DeviceGray, CIEBasedA, Separation, Indexed). */
/* This procedure handles a single scan line. */
int
image_render_mono(gs_image_enum *penum, byte *buffer, uint w, int h)
{	gs_state *pgs = penum->pgs;
	const int masked = penum->masked;
	const fixed dxx = penum->fxx;
	const fixed dxx3 = (dxx << 1) + dxx;
	fixed xt = penum->xcur;
	gs_color_space *pcs = pgs->color_space;
	cs_proc_remap_color((*remap_color)) = pcs->type->remap_color;
	gs_client_color cc;
	cmap_proc_gray((*map_gray)) = pgs->cmap_procs->map_gray;
	gx_device_color *pdevc = pgs->dev_color;
	/* Make sure the cache setup matches the graphics state. */
	/* Also determine whether all tiles fit in the cache. */
	int tiles_fit = gx_check_tile_cache(pgs);
#define image_set_gray(sample_value)\
   { pdevc = &penum->dev_colors[sample_value];\
     if ( pdevc->type == &gx_dc_none )\
      {  if ( penum->device_color )\
	    (*map_gray)(byte2frac(sample_value), pdevc, pgs);\
	 else\
	  { decode_sample(sample_value, cc, 0);\
	    (*remap_color)(&cc, pcs, pdevc, pgs);\
	  }\
      }\
     else if ( pdevc->type != &gx_dc_pure )\
      { if ( !tiles_fit )\
         { code = gx_color_load(pdevc, pgs);\
	   if ( code < 0 ) return code;\
	 }\
      }\
   }
	fixed xl = xt;
	register const byte *psrc = buffer;
	byte *endp = buffer + w;
	fixed xrun = xt;		/* x at start of run */
	register byte run = *psrc;	/* run value */
	int htrun =			/* halftone run value */
	  (masked ? 255 : -2);
	int code;

	*endp = ~endp[-1];	/* force end of run */
	if ( penum->slow_loop )
	  { /* Skewed, or imagemask with a halftone. */
	    const fixed
	      dxy = penum->fxy, dyx = penum->fyx,
	      dyy = penum->fyy;
	    const fixed dxy3 = (dxy << 1) + dxy;
	    fixed ytf = penum->ycur;
	    fixed yrun = ytf;
	    for ( ; ; )
	      {	/* Skip large constant regions quickly, */
		/* but don't slow down transitions too much. */
sks:		if ( psrc[0] == run )
		  { if ( psrc[1] == run )
		      { if ( psrc[2] == run )
			  { if ( psrc[3] == run )
			      { psrc += 4, xl += dxx << 2, ytf += dxy << 2;
				goto sks;
			      }
			    else
			      psrc += 4, xl += dxx3, ytf += dxy3;
			  }
		        else
			  psrc += 3, xl += dxx << 1, ytf += dxy << 1;
		      }
		    else
		      psrc += 2, xl += dxx, ytf += dxy;
		  }
		else
		  psrc++;
		/* Now fill the region between xrun and xl. */
		if ( run != htrun )
		  { if ( run == 0 )
		      { if ( masked ) goto trans;
		      }
		    htrun = run;
		    image_set_gray(run);
		  }
		code = gx_fill_pgram_fixed(xrun, yrun, xl - xrun,
					   ytf - yrun, dyx, dyy,
					   pdevc, pgs);
		if ( code < 0 ) return code;
trans:		if ( psrc > endp ) break;
		yrun = ytf;
		xrun = xl;
		run = psrc[-1];
		xl += dxx;
		ytf += dxy;		/* harmless if no skew */
	      }
	  }
	else			/* fast loop */
	  { /* No skew, and not imagemask with a halftone. */
	    const fixed adjust = penum->adjust;
	    fixed xa = (dxx >= 0 ? adjust : -adjust);
	    const int yt = penum->yci, iht = penum->hci;
	    gx_device *dev = gs_currentdevice(pgs);
	    dev_proc_fill_rectangle((*fill_proc)) = dev_proc(dev, fill_rectangle);
	    dev_proc_tile_rectangle((*tile_proc)) = dev_proc(dev, tile_rectangle);
	    dev_proc_copy_mono((*copy_mono_proc)) = dev_proc(dev, copy_mono);
	    /* If each pixel is likely to fit in a single halftone tile, */
	    /* determine that now (tile_offset = offset of row within tile). */
	    int tile_offset =
	      gx_check_tile_size(pgs,
				 fixed2int_rounded(any_abs(dxx) + (xa << 1)),
				 yt, iht);
	    gx_ht_order *porder = &pgs->dev_ht->order;
	    /* Fold the adjustment into xrun and xl, */
	    /* including the +0.5 for rounding. */
	    xrun = xrun - xa + fixed_half;
	    xl = xl + xa + fixed_half;
	    xa <<= 1;
	    for ( ; ; )
	      {	/* Skip large constant regions quickly, */
		/* but don't slow down transitions too much. */
skf:		if ( psrc[0] == run )
		  { if ( psrc[1] == run )
		      { if ( psrc[2] == run )
			  { if ( psrc[3] == run )
			      { psrc += 4, xl += dxx << 2;
				goto skf;
			      }
			    else
			      psrc += 4, xl += dxx3;
			  }
		        else
			  psrc += 3, xl += dxx << 1;
		      }
		    else
		      psrc += 2, xl += dxx;
		  }
		else
		  psrc++;
		{ /* Now fill the region between xrun and xl. */
		  int xi = fixed2int_var(xrun);
		  int wi = fixed2int_var(xl) - xi;
		  int tsx;
		  const gx_tile_bitmap *tile;
		  if ( wi <= 0 )
		    { if ( wi == 0 ) goto mt;
		      xi += wi, wi = -wi;
		    }
		  switch ( run )
		    {
		    case 0:
		      if ( masked ) goto mt;
		      if ( !color_is_pure(&penum->icolor0) ) goto ht;
		      code = (*fill_proc)(dev, xi, yt, wi, iht,
					  penum->icolor0.colors.pure);
		      break;
		    case 255:		/* just for speed */
		      if ( !color_is_pure(&penum->icolor1) ) goto ht;
		      code = (*fill_proc)(dev, xi, yt, wi, iht,
					  penum->icolor1.colors.pure);
		      break;
		    default:
ht:		      /* Use halftone if needed */
		      if ( run != htrun )
			{ image_set_gray(run);
			  htrun = run;
			}
		      /* We open-code gx_fill_rectangle, */
		      /* because we've done some of the work for */
		      /* halftone tiles in advance. */
		      if ( color_is_pure(pdevc) )
			{ code = (*fill_proc)(dev, xi, yt, wi, iht,
					      pdevc->colors.pure);
			}
		      else if ( pdevc->type != &gx_dc_ht_binary )
			{ code = gx_fill_rectangle(xi, yt, wi, iht,
						   pdevc, pgs);
			}
		      else if ( tile_offset >= 0 &&
			        (tile = &pdevc->colors.binary.b_tile->tile,
				 (tsx = (xi + porder->phase.x) % tile->rep_width) + wi <= tile->size.x)
			      )
			{ /* The pixel(s) fit(s) in a single (binary) tile. */
			  byte *row = tile->data + tile_offset;
			  code = (*copy_mono_proc)
			    (dev, row, tsx, tile->raster, gx_no_bitmap_id,
			     xi, yt, wi, iht,
			     pdevc->colors.binary.color[0],
			     pdevc->colors.binary.color[1]);
			}
		      else
			{ code = (*tile_proc)(dev,
					      &pdevc->colors.binary.b_tile->tile,
					      xi, yt, wi, iht,
					      pdevc->colors.binary.color[0],
					      pdevc->colors.binary.color[1],
					      porder->phase.x, porder->phase.y);
			}
		    }
		    if ( code < 0 ) return code;
mt:		    if ( psrc > endp ) break;
		    xrun = xl - xa;	/* original xa << 1 */
		    run = psrc[-1];
		}
		xl += dxx;
	      }
	  }
	return 1;
}
