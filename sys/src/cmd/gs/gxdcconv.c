/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxdcconv.c */
/* Conversion between device color spaces for Ghostscript */
#include "gx.h"
#include "gxdcconv.h"			/* interface */
#include "gxdcolor.h"			/* for gxcmap.h */
#include "gxdevice.h"			/* ditto */
#include "gxcmap.h"
#include "gxfarith.h"
#include "gxlum.h"
#include "gzstate.h"

/* ------ Color space conversion ------ */

/* Only 4 of the 6 conversions are implemented here; */
/* the other 2 (Gray to RGB/CMYK) are trivial. */
/* The CMYK to RGB algorithms specified by Adobe are, e.g., */
/*	R = 1.0 - min(1.0, C + K)	*/
/* but we get much better results with */
/*	R = (1.0 - C) * (1.0 - K)	*/

/* Convert RGB to Gray. */
frac
color_rgb_to_gray(frac r, frac g, frac b, const gs_state *pgs)
{	return (r * (unsigned long)lum_red_weight +
		g * (unsigned long)lum_green_weight +
		b * (unsigned long)lum_blue_weight +
		(lum_all_weights / 2))
	    / lum_all_weights;
}

/* Convert RGB to CMYK. */
/* Note that this involves black generation and undercolor removal. */
void
color_rgb_to_cmyk(frac r, frac g, frac b, const gs_state *pgs,
  frac cmyk[4])
{	frac c = frac_1 - r, m = frac_1 - g, y = frac_1 - b;
	frac k = (c < m ? min(c, y) : min(m, y));
	/* The default UCR and BG functions are pretty arbitrary.... */
	frac bg =
		(pgs->black_generation == NULL ? frac_0 :
		 gx_map_color_frac(pgs, k, black_generation));
	signed_frac ucr =
		(pgs->undercolor_removal == NULL ? frac_0 :
		 gx_map_color_frac(pgs, k, undercolor_removal));
	/* Adobe specifies, e.g., */
	/*	C = max(0.0, min(1.0, 1 - R - UCR)) */
	/* but in order to match our improved CMYK->RGB mapping, we use */
	/*	C = max(0.0, min(1.0, 1 - R / (1 - UCR)) */
	if ( ucr == frac_1 )
		cmyk[0] = cmyk[1] = cmyk[2] = 0;
	else
	{	float denom = frac2float(frac_1 - ucr);	/* unscaled */
		float v;
		v = (float)frac_1 - r / denom;	/* unscaled */
		cmyk[0] =
		  (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac)v);
		v = (float)frac_1 - g / denom;	/* unscaled */
		cmyk[1] =
		  (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac)v);
		v = (float)frac_1 - b / denom;	/* unscaled */
		cmyk[2] =
		  (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac)v);
	}
	cmyk[3] = bg;
	if_debug7('c', "[c]RGB 0x%x,0x%x,0x%x -> CMYK 0x%x,0x%x,0x%x,0x%x\n",
		  r, g, b, cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
}

/* Convert CMYK to Gray. */
frac
color_cmyk_to_gray(frac c, frac m, frac y, frac k, const gs_state *pgs)
{	frac not_gray = color_rgb_to_gray(c, m, y, pgs);
	return (not_gray > frac_1 - k ?		/* gray + k > 1.0 */
		frac_0 : frac_1 - (not_gray + k));
}

/* Convert CMYK to RGB. */
void
color_cmyk_to_rgb(frac c, frac m, frac y, frac k, const gs_state *pgs,
  frac rgb[3])
{	switch ( k )
	{
	case frac_0:
		rgb[0] = frac_1 - c;
		rgb[1] = frac_1 - m;
		rgb[2] = frac_1 - y;
		break;
	case frac_1:
		rgb[0] = rgb[1] = rgb[2] = frac_0;
		break;
	default:
	{	ulong not_k = frac_1 - k;
		/* Compute not_k * (frac_1 - v) / frac_1 efficiently. */
		ulong prod;
#define deduct_black(v)\
  (prod = (frac_1 - (v)) * not_k, frac_1_quo(prod))
		rgb[0] = deduct_black(c);
		rgb[1] = deduct_black(m);
		rgb[2] = deduct_black(y);
	}
	}
	if_debug7('c', "[c]CMYK 0x%x,0x%x,0x%x,0x%x -> RGB 0x%x,0x%x,0x%x\n",
		  c, m, y, k, rgb[0], rgb[1], rgb[2]);
}
