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

/* gsimage3.c */
/* 12-bit and interpolated image procedures */
#include "gx.h"
#include "memory_.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxfrac.h"
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
#include "gxdraw.h"

/* ---------------- Unpacking procedures ---------------- */

void
image_unpack_12(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register int spread = penum->spread;
	register frac *bufp = (frac *)(bptr + inpos * 2 / 3 * spread);
#define inc_bufp(bp, n) bp = (frac *)((byte *)(bp) + (n))
	register uint sample;
	register int left = dsize;
	static const frac bits2frac_4[16] = {
#define frac15(n) ((frac_1 / 15) * (n))
		frac15(0), frac15(1), frac15(2), frac15(3),
		frac15(4), frac15(5), frac15(6), frac15(7),
		frac15(8), frac15(9), frac15(10), frac15(11),
		frac15(12), frac15(13), frac15(14), frac15(15)
#undef frac15
	};
	/* We have to deal with the 3 cases of inpos % 3 individually. */
	/* Let N = inpos / 3. */
	switch ( inpos % 3 )
	{
	case 1:
		/* bufp points to frac N, which was already filled */
		/* with the leftover byte from the previous call. */
		sample = (frac2byte(*bufp) << 4) + (*data >> 4);
		*bufp = bits2frac(sample, 12);
		inc_bufp(bufp, spread);
		*bufp = bits2frac_4[*data++ & 0xf];
		if ( !--left ) return;
	case 2:
		/* bufp points to frac N+1, which was half-filled */
		/* with the second leftover byte from the previous call. */
		sample = (frac2bits(*bufp, 4) << 8) + *data++;
		*bufp = bits2frac(sample, 12);
		inc_bufp(bufp, spread);
		--left;
	case 0:
		/* Nothing special to do. */
		;
	}
	while ( left >= 3 )
	{	sample = ((uint)*data << 4) + (data[1] >> 4);
		*bufp = bits2frac(sample, 12);
		inc_bufp(bufp, spread);
		sample = ((uint)(data[1] & 0xf) << 8) + data[2];
		*bufp = bits2frac(sample, 12);
		inc_bufp(bufp, spread);
		data += 3;
		left -= 3;
	}
	/* Handle trailing bytes. */
	switch ( left )
	{
	case 2:				/* dddddddd ddddxxxx */
		sample = ((uint)*data << 4) + (data[1] >> 4);
		*bufp = bits2frac(sample, 12);
		inc_bufp(bufp, spread);
		*bufp = bits2frac_4[data[1] & 0xf];
		break;
	case 1:				/* dddddddd */
		sample = (uint)*data << 4;
		*bufp = bits2frac(sample, 12);
		break;
	case 0:				/* Nothing more to do. */
		;
	}
}

/* ---------------- Rendering procedures ---------------- */

/* ------ Rendering for 12-bit samples ------ */

#define decode_frac(frac_value, cc, i)\
  cc.paint.values[i] =\
    penum->map[i].decode_base + (frac_value) * penum->map[i].decode_factor

/* Render an image with more than 8 bits per sample. */
/* The samples have been expanded into fracs. */
#define longs_per_4_fracs (arch_sizeof_frac * 4 / arch_sizeof_long)
typedef union {
	frac v[4];
	long all[longs_per_4_fracs];		/* for fast comparison */
} color_fracs;
#if longs_per_4_fracs == 1
#  define color_frac_eq(f1, f2)\
     ((f1).all[0] == (f2).all[0])
#else
#if longs_per_4_fracs == 2
#  define color_frac_eq(f1, f2)\
     ((f1).all[0] == (f2).all[0] && (f1).all[1] == (f2).all[1])
#endif
#endif
int
image_render_frac(gs_image_enum *penum, byte *buffer, uint w, int h)
{	gs_state *pgs = penum->pgs;
	fixed	dxx = penum->fxx, dxy = penum->fxy,
		dyx = penum->fyx, dyy = penum->fyy;
	image_posture posture = penum->posture;
	fixed xt = penum->xcur;
	fixed ytf = penum->ycur;
	int yt = penum->yci, iht = penum->hci;
	gs_color_space *pcs = pgs->color_space;
	cs_proc_remap_color((*remap_color)) = pcs->type->remap_color;
	gs_client_color cc;
	int device_color = penum->device_color;
	cmap_proc_rgb((*map_rgb)) = pgs->cmap_procs->map_rgb;
	cmap_proc_cmyk((*map_cmyk)) = pgs->cmap_procs->map_cmyk;
	gx_device_color devc1, devc2;
	gx_device_color _ss *spdevc = &devc1;
	gx_device_color _ss *spdevc_next = &devc2;
#define pdevc ((gx_device_color *)spdevc)
#define pdevc_next ((gx_device_color *)spdevc_next)
	int spp = penum->spp;
	fixed xl = xt;
	const frac *psrc = (frac *)buffer;
	fixed xrun = xt;		/* x at start of run */
	int irun = fixed2int_var_rounded(xrun);	/* int xrun */
	fixed yrun = ytf;		/* y ditto */
	color_fracs run;		/* run value */
	color_fracs next;		/* next sample value */
	frac *bufend = (frac *)buffer + w;
	bufend[0] = ~bufend[-spp];	/* force end of run */
	if_debug5('b', "[b]y=%d w=%d xt=%f yt=%f yb=%f\n",
		  penum->y, w,
		  fixed2float(xt), fixed2float(ytf), fixed2float(ytf + dyy));
	run.v[0] = run.v[1] = run.v[2] = run.v[3] = 0;
	next.v[0] = next.v[1] = next.v[2] = next.v[3] = 0;
	cc.paint.values[0] = cc.paint.values[1] =
	  cc.paint.values[2] = cc.paint.values[3] = 0;
	cc.pattern = 0;
	(*remap_color)(&cc, pcs, pdevc, pgs);
	run.v[0] = ~psrc[0];		/* force remap */

	while ( psrc <= bufend )	/* 1 extra iteration */
				/* to handle final run */
	{	next.v[0] = psrc[0];
		switch ( spp )
		{
		case 4:			/* cmyk */
			next.v[1] = psrc[1];
			next.v[2] = psrc[2];
			next.v[3] = psrc[3];
			psrc += 4;
			if ( color_frac_eq(next, run) ) goto inc;
			if ( device_color )
			{	(*map_cmyk)(next.v[0], next.v[1],
					    next.v[2], next.v[3],
					    pdevc_next, pgs);
				goto f;
			}
			decode_frac(next.v[0], cc, 0);
			decode_frac(next.v[1], cc, 1);
			decode_frac(next.v[2], cc, 2);
			decode_frac(next.v[3], cc, 3);
			if_debug4('B', "[B]cc[0..3]=%g,%g,%g,%g\n",
				  cc.paint.values[0], cc.paint.values[1],
				  cc.paint.values[2], cc.paint.values[3]);
			if_debug1('B', "[B]cc[3]=%g\n",
				  cc.paint.values[3]);
			break;
		case 3:			/* rgb */
			next.v[1] = psrc[1];
			next.v[2] = psrc[2];
			psrc += 3;
			if ( color_frac_eq(next, run) ) goto inc;
			if ( device_color )
			{	(*map_rgb)(next.v[0], next.v[1],
					   next.v[2], pdevc_next, pgs);
				goto f;
			}
			decode_frac(next.v[0], cc, 0);
			decode_frac(next.v[1], cc, 1);
			decode_frac(next.v[2], cc, 2);
			if_debug3('B', "[B]cc[0..2]=%g,%g,%g\n",
				  cc.paint.values[0], cc.paint.values[1],
				  cc.paint.values[2]);
			break;
		case 1:			/* gray */
			psrc++;
			if ( next.v[0] == run.v[0] ) goto inc;
			if ( device_color )
			{	(*map_rgb)(next.v[0], next.v[0],
					   next.v[0], pdevc_next, pgs);
				goto f;
			}
			decode_frac(next.v[0], cc, 0);
			if_debug1('B', "[B]cc[0]=%g\n",
				  cc.paint.values[0]);
			break;
		}
		(*remap_color)(&cc, pcs, pdevc_next, pgs);
f:		if_debug7('B', "[B]0x%x,0x%x,0x%x,0x%x -> %ld,%ld,0x%lx\n",
			next.v[0], next.v[1], next.v[2], next.v[3],
			pdevc_next->colors.binary.color[0],
			pdevc_next->colors.binary.color[1],
			(ulong)pdevc_next->type);
		/* Even though the supplied colors don't match, */
		/* the device colors might. */
		if ( !dev_color_eq(devc1, devc2) ||
		     psrc > bufend	/* force end of last run */
		   )
		{	/* Fill the region between */
			/* xrun/irun and xl */
			gx_device_color _ss *sptemp;
			int code;
			if ( posture != image_portrait )
			{	/* Parallelogram */
				code = gx_fill_pgram_fixed(xrun, yrun,
					xl - xrun, ytf - yrun, dyx, dyy,
					pdevc, pgs);
				xrun = xl;
				yrun = ytf;
			}
				else
			{	/* Rectangle */
				int xi = irun;
				int wi = (irun = fixed2int_var_rounded(xl)) - xi;
				if ( wi < 0 ) xi += wi, wi = -wi;
				code = gx_fill_rectangle(xi, yt, wi, iht,
							 pdevc, pgs);
			}
			if ( code < 0 )
				return code;
			sptemp = spdevc;
			spdevc = spdevc_next;
			spdevc_next = sptemp;
		}
		run = next;
inc:		xl += dxx;
		ytf += dxy;		/* harmless if no skew */
	}
	return 1;
}

/* ------ Rendering for interpolated images ------ */

int
image_render_interpolate(gs_image_enum *penum, byte *buffer, uint w, int h)
{	uint row_size =
	  penum->scale_state.src_width * penum->scale_state.values_per_pixel *
	    penum->scale_state.sizeofPixelIn;
	byte *row =
	  (byte *)penum->scale_state.src + penum->y * row_size;
	const gs_state *pgs = penum->pgs;
	const gs_color_space *pcs = pgs->color_space;
	int c = penum->scale_state.values_per_pixel;

	/* Convert the unpacked data to concrete values in */
	/* the source buffer. */
	if ( penum->scale_state.sizeofPixelIn == 1 )
	  {	/* Easy case: 8-bit device color values. */
		memcpy(row, buffer, row_size);
	  }
	else
	  {	/* Messy case: concretize each sample. */
		int bps = penum->bps;
		int dc = penum->spp;
		byte *pdata = buffer;
		frac *psrc = (frac *)row;
		gs_client_color cc;
		int i;
		for ( i = 0; i < penum->scale_state.src_width;
		      i++, psrc += c
		    )
		  {	int j;
			for ( j = 0; j < dc; j++ )
			{	if ( bps <= 8 )
				{	decode_sample(*pdata, cc, j);
					pdata++;
				}
				else		/* bps == 12 */
				{	decode_frac(*(frac *)pdata, cc, j);
					pdata += sizeof(frac);
				}
			}
			(*pcs->type->concretize_color)(&cc, pcs, psrc, pgs);
		  }
	  }

	/* Check whether we've got all the source data. */
	if ( penum->y != penum->height - 1 )
		return 1;

	/* We do.  Scale and render. */
	gs_image_scale(penum->scale_state.dst, penum->scale_state.src,
		       &penum->scale_state);

	/* By construction, the pixels are 1-for-1 with the device, */
	/* but the Y coordinate might be inverted. */
	/* The loop below was written for simplicity, not speed! */
	{	const frac *pdst = penum->scale_state.dst;
		gx_device_color devc;
		int x, y;
		int xo = fixed2int_rounded(penum->mtx);
		int yo = fixed2int_rounded(penum->mty);
		int dy;
		const gs_color_space *pconcs = cs_concrete_space(pcs, pgs);

		if ( penum->fyy > 0 ) dy = 1;
		else dy = -1, yo--;
		for ( y = 0; y < penum->scale_state.dst_height; y++ )
		{	for ( x = 0; x < penum->scale_state.dst_width;
			      x++, pdst += c
			    )
			{	(*pconcs->type->remap_concrete_color)
					(pdst, &devc, pgs);
				gx_fill_rectangle(xo + x, yo + y * dy,
						  1, 1, &devc, pgs);
			}
		}
	}

	return 1;
}
