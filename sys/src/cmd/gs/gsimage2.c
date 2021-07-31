/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsimage2.c */
/* Level 2 and color image procedures for Ghostscript library */
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

/* ------ Unpacking procedures ------ */

void
image_unpack_1_spread(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register int spread = penum->spread;
	register byte *bufp = bptr + (inpos << 3) * spread;
	int left = dsize;
	register const byte *map = &pmap->table.lookup8[0];
	while ( left-- )
	   {	register uint b = *data++;
		*bufp = map[b >> 7]; bufp += spread;
		*bufp = map[(b >> 6) & 1]; bufp += spread;
		*bufp = map[(b >> 5) & 1]; bufp += spread;
		*bufp = map[(b >> 4) & 1]; bufp += spread;
		*bufp = map[(b >> 3) & 1]; bufp += spread;
		*bufp = map[(b >> 2) & 1]; bufp += spread;
		*bufp = map[(b >> 1) & 1]; bufp += spread;
		*bufp = map[b & 1]; bufp += spread;
	   }
}

void
image_unpack_2_spread(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register int spread = penum->spread;
	register byte *bufp = bptr + (inpos << 2) * spread;
	int left = dsize;
	register const byte *map = &pmap->table.lookup8[0];
	while ( left-- )
	   {	register unsigned b = *data++;
		*bufp = map[b >> 6]; bufp += spread;
		*bufp = map[(b >> 4) & 3]; bufp += spread;
		*bufp = map[(b >> 2) & 3]; bufp += spread;
		*bufp = map[b & 3]; bufp += spread;
	   }
}

void
image_unpack_8_spread(const gs_image_enum *penum, const sample_map *pmap,
  byte *bptr, register const byte *data, uint dsize, uint inpos)
{	register int spread = penum->spread;
	register byte *bufp = bptr + inpos * spread;
	register int left = dsize;
	register const byte *map = &pmap->table.lookup8[0];
	while ( left-- )
	   {	*bufp = map[*data++]; bufp += spread;
	   }
}

/* ------ Rendering procedures ------ */

/* Render a color image with 8 or fewer bits per sample. */
typedef union {
	byte v[4];
	bits32 all;		/* for fast comparison & clearing */
} color_samples;
int
image_render_color(gs_image_enum *penum, byte *buffer, uint w, int h)
{	gs_state *pgs = penum->pgs;
	fixed	dxx = penum->fxx, dxy = penum->fxy,
		dyx = penum->fyx, dyy = penum->fyy;
	image_posture posture = penum->posture;
	fixed xt = penum->xcur;
	fixed ytf = penum->ycur;
	int vci, vdi;
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
	const byte *psrc = buffer;
	fixed xrun = xt;		/* x at start of run */
	fixed yrun = ytf;		/* y ditto */
	int irun;			/* int x/rrun */
	color_samples run;		/* run value */
	color_samples next;		/* next sample value */
	bool small = (any_abs(dxx) < fixed_1 && any_abs(dxy) < fixed_1);
	byte *bufend = buffer + w;

	switch ( posture )
	  {
	  case image_portrait:
	    vci = penum->yci, vdi = penum->hci;
	    irun = fixed2int_var_rounded(xrun);
	    break;
	  case image_landscape:
	    vci = penum->xci, vdi = penum->wci;
	    irun = fixed2int_var_rounded(yrun);
	    break;
	  case image_skewed:
	    ;
	  }

	bufend[0] = ~bufend[-spp];	/* force end of run */
	if_debug5('b', "[b]y=%d w=%d xt=%f yt=%f yb=%f\n",
		  penum->y, w,
		  fixed2float(xt), fixed2float(ytf), fixed2float(ytf + dyy));
	run.all = 0;
	next.all = 0;
	cc.paint.values[0] = cc.paint.values[1] =
	  cc.paint.values[2] = cc.paint.values[3] = 0;
	cc.pattern = 0;
	(*remap_color)(&cc, pcs, pdevc, pgs);
	run.v[0] = ~psrc[0];		/* force remap */
	while ( psrc <= bufend )	/* 1 extra iteration */
				/* to handle final run */
	{	fixed xn = xl + dxx, yn = ytf + dxy;
#define includes_pixel_center(a, b)\
  (fixed_floor(a < b ? (a - (fixed_half + fixed_epsilon)) ^ (b - fixed_half) :\
	       (b - (fixed_half + fixed_epsilon)) ^ (a - fixed_half)) != 0)
#define paint_no_pixels()\
  (small && !includes_pixel_center(xl, xn) && !includes_pixel_center(ytf, yn) && psrc <= bufend)
		next.v[0] = psrc[0];
		next.v[1] = psrc[1];
		next.v[2] = psrc[2];
		if ( spp == 4 )		/* cmyk */
		{	next.v[3] = psrc[3];
			psrc += 4;
			if ( next.all == run.all || paint_no_pixels() )
			  goto inc;
			if ( device_color )
			{	(*map_cmyk)(byte2frac(next.v[0]),
					byte2frac(next.v[1]),
					byte2frac(next.v[2]),
					byte2frac(next.v[3]),
					pdevc_next, pgs);
				goto f;
			}
			else
			{	decode_sample(next.v[3], cc, 3);
				if_debug1('B', "[B]cc[3]=%g\n",
					  cc.paint.values[3]);
			}
		}
		else			/* rgb */
		{	psrc += 3;
			if ( next.all == run.all || paint_no_pixels() )
			  goto inc;
			if ( device_color )
			{	(*map_rgb)(byte2frac(next.v[0]),
					byte2frac(next.v[1]),
					byte2frac(next.v[2]),
					pdevc_next, pgs);
				goto f;
			}
		}
		decode_sample(next.v[0], cc, 0);
		decode_sample(next.v[1], cc, 1);
		decode_sample(next.v[2], cc, 2);
		if_debug3('B', "[B]cc[0..2]=%g,%g,%g\n",
			  cc.paint.values[0], cc.paint.values[1],
			  cc.paint.values[2]);
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
			switch ( posture )
			  {
			  case image_portrait:
			    {	/* Rectangle */
				int xi = irun;
				int wi =
				  (irun = fixed2int_var_rounded(xl)) - xi;
				if ( wi < 0 )
				  xi += wi, wi = -wi;
				code = gx_fill_rectangle(xi, vci, wi, vdi,
							 pdevc, pgs);
			    }	break;
			  case image_landscape:
			    {	/* 90 degree rotated rectangle */
				int yi = irun;
				int hi =
				  (irun = fixed2int_var_rounded(ytf)) - yi;
				if ( hi < 0 )
				  yi += hi, hi = -hi;
				code = gx_fill_rectangle(vci, yi, vdi, hi,
							 pdevc, pgs);
			    }	break;
			  default:
			    {	/* Parallelogram */
				code = gx_fill_pgram_fixed(xrun, yrun,
					xl - xrun, ytf - yrun, dyx, dyy,
					pdevc, pgs);
				xrun = xl;
				yrun = ytf;
			    }
			  }
			if ( code < 0 )
				return code;
			sptemp = spdevc;
			spdevc = spdevc_next;
			spdevc_next = sptemp;
		}
		run.all = next.all;
inc:		xl = xn;
		ytf = yn;		/* harmless if no skew */
	}
	return 1;
}
