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

/* gsiscale.c */
/* Image scaling with filtering */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsiscale.h"

/*
 *	Image rescaling code, based on public domain code from
 *	Graphics Gems III (pp. 414-424), Academic Press, 1992.
 */

/* Mitchell filter definition */
#define Mitchell_support 2.0
#define B (1.0 / 3.0)
#define C (1.0 / 3.0)
private double
Mitchell_filter(double t)
{	double t2 = t * t;

	if ( t < 0 )
	  t = -t;

	if ( t < 1 )
	  return
	    ((12 - 9 * B - 6 * C) * (t * t2) +
	     (-18 + 12 * B + 6 * C) * t2 +
	     (6 - 2 * B)) / 6;
	else if ( t < 2 )
	  return
	    ((-1 * B - 6 * C) * (t * t2) +
	     (6 * B + 30 * C) * t2 +
	     (-12 * B - 48 * C) * t +
	     (8 * B + 24 * C)) / 6;
	else
	  return 0;
}

#define filter_support Mitchell_support
#define filter_proc Mitchell_filter
#define fproc(t) filter_proc(t)
#define fwidth filter_support

/*
 * The environment provides the following definitions:
 *	typedef PixelTmp
 *	double fproc(double t)
 *	double fwidth
 *	PixelTmp {min,max,unit}PixelTmp
 */
#define CLAMP(v, mn, mx)\
  (v < mn ? mn : v > mx ? mx : v)

/* ---------------- Auxiliary procedures ---------------- */

/* Pre-calculate filter contributions for a row or a column. */
private int
contrib_pixels(double scale)
{	return (int)(fwidth / min(scale, 1.0) * 2 + 1);
}
private CLIST *
calculate_contrib(CLIST *contrib, CONTRIB *items, double scale,
  int size, int limit, int modulus, int stride,
  double rescale_factor)
{
	double width, fscale;
	bool squeeze;
	int npixels;
	int i, j;

	if ( scale < 1.0 )
	  {	width = fwidth / scale;
		fscale = 1.0 / scale;
		squeeze = true;
	  }
	else
	  {	width = fwidth;
		fscale = 1.0;
		squeeze = false;
	  }
	npixels = (int)(width * 2 + 1);

	for ( i = 0; i < size; ++i )
	  {	double center = i / scale;
		int left = (int)ceil(center - width);
		int right = (int)floor(center + width);
		int max_n = -1;
		contrib[i].n = 0;
		contrib[i].p = items + i * npixels;
		if ( squeeze )
		  {	for ( j = left; j <= right; ++j )
			  {	double weight =
				  fproc((center - j) / fscale) / fscale;
				int n =
				  (j < 0 ? -j :
				   j >= limit ? (limit - j) + limit - 1 :
				   j);
				int k = contrib[i].n++;
				if ( n > max_n )
				  max_n = n;
				contrib[i].p[k].pixel = (n % modulus) * stride;
				contrib[i].p[k].weight =
				  weight * rescale_factor;
			  }
		  }
		else
		  {	for ( j = left; j <= right; ++j )
			  {	double weight = fproc(center - j);
				int n =
				  (j < 0 ? -j :
				   j >= limit ? (limit - j) + limit - 1 :
				   j);
				int k = contrib[i].n++;
				if ( n > max_n )
				  max_n = n;
				contrib[i].p[k].pixel = (n % modulus) * stride;
				contrib[i].p[k].weight =
				  weight * rescale_factor;
			  }
		  }
		contrib[i].max_index = max_n;
	  }
	return contrib;
}


/* Apply filter to zoom horizontally from src to tmp. */
private void
zoom_x(PixelTmp *tmp, const void /*PixelIn*/ *src, int sizeofPixelIn,
  int tmp_width, int src_width, int values_per_pixel, const CLIST *contrib)
{	int c, i, j;
	for ( c = 0; c < values_per_pixel; ++c )
	  {
#define zoom_x_loop(PixelIn)\
		const PixelIn *raster = &((PixelIn *)src)[c];\
		for ( i = 0; i < tmp_width; ++i )\
		  {	double weight = 0.0;\
			for ( j = 0; j < contrib[i].n; ++j )\
				weight +=\
				  raster[contrib[i].p[j].pixel]\
				    * contrib[i].p[j].weight;\
			tmp[i * values_per_pixel + c] =\
			  (PixelTmp)CLAMP(weight, minPixelTmp, maxPixelTmp);\
		  }

		if ( sizeofPixelIn == 1 )
		  {	zoom_x_loop(byte)
		  }
		else			/* sizeofPixelIn == 2 */
		  {	zoom_x_loop(bits16)
		  }
	  }
}

/* Apply filter to zoom vertically from tmp to dst. */
/* This is simpler because we can treat all columns identically */
/* without regard to the number of samples per pixel. */
private void
zoom_y(void /*PixelOut*/ *dst, int sizeofPixelOut, uint maxPixelOut,
  const PixelTmp *tmp, int dst_width, int tmp_width,
  int values_per_pixel, const CLIST *contrib)
{	int kn = dst_width * values_per_pixel;
	int cn = contrib->n;
	const CONTRIB *cp = contrib->p;
	int kc;
	double max_weight = (double)maxPixelOut;

#define zoom_y_loop(PixelOut)\
	for ( kc = 0; kc < kn; ++kc )\
	  {	const PixelTmp *raster = &tmp[kc];\
		double weight = 0.0;\
		int j;\
		for ( j = 0; j < cn; ++j )\
		  weight += raster[cp[j].pixel] * cp[j].weight;\
		((PixelOut *)dst)[kc] =\
		  (PixelOut)CLAMP(weight, 0, max_weight);\
	  }

	if ( sizeofPixelOut == 1 )
	  {	zoom_y_loop(byte)
	  }
	else			/* sizeofPixelOut == 2 */
	  {	zoom_y_loop(bits16)
	  }
}

/* ---------------- Public interface ---------------- */

/* Initialize the scaler state, allocating all necessary working storage. */
int
gs_image_scale_init(gs_image_scale_state *pss, bool alloc_dst, bool alloc_src)
{	gs_memory_t *mem = pss->memory;

	pss->xscale = (double)pss->dst_width / (double)pss->src_width;
	pss->yscale = (double)pss->dst_height / (double)pss->src_height;

	/* create intermediate image to hold horizontal zoom */
#define tmp_width dst_width
#define tmp_height src_height
	pss->tmp = (PixelTmp *)gs_alloc_byte_array(mem,
					pss->tmp_width * pss->tmp_height *
					  pss->values_per_pixel,
					sizeof(PixelTmp), "image_scale tmp");
	pss->contrib = (CLIST *)gs_alloc_byte_array(mem,
					max(pss->dst_width, pss->dst_height),
					sizeof(CLIST), "image_scale contrib");
	pss->items = (CONTRIB *)gs_alloc_byte_array(mem,
				max(contrib_pixels(pss->xscale) *
				    pss->dst_width,
				    contrib_pixels(pss->yscale) *
				    pss->dst_height),
				sizeof(CONTRIB), "image_scale contrib[*]");
	if ( alloc_dst )
	  pss->dst = gs_alloc_byte_array(mem,
				pss->dst_width * pss->dst_height *
				  pss->values_per_pixel,
				pss->sizeofPixelOut, "image_scale dst");
	else
	  pss->dst = 0;
	if ( alloc_src )
	  pss->src = gs_alloc_byte_array(mem,
				pss->src_width * pss->src_height *
				  pss->values_per_pixel,
				pss->sizeofPixelIn, "image_scale src");
	else
	  pss->src = 0;
	if ( pss->tmp == 0 || pss->contrib == 0 || pss->items == 0 ||
	     (alloc_dst && pss->dst == 0) || (alloc_src && pss->src == 0)
	   )
	  {	gs_image_scale_cleanup(pss);
		return_error(gs_error_VMerror);
	  }
	return 0;
}

/* Scale the image. */
int
gs_image_scale(void /*PixelOut*/ *dst, const void /*PixelIn*/ *src,
  gs_image_scale_state *pss)
{
	int dst_width = pss->dst_width, dst_height = pss->dst_height;
	int src_width = pss->src_width, src_height = pss->src_height;
	int values_per_pixel = pss->values_per_pixel;
	double xscale = pss->xscale, yscale = pss->yscale;
	PixelTmp *tmp = pss->tmp;
	int i, k;
	CLIST *contrib_x = 0;
	CLIST *contrib_y = 0;

	/* use preallocated src and dst if requested */
	if ( dst == 0 )
	  dst = pss->dst;
	if ( src == 0 )
	  src = pss->src;

	/* pre-calculate filter contributions for a row and for a column */
	contrib_x = calculate_contrib(pss->contrib, pss->items, xscale,
				      dst_width, src_width,
				      src_width, values_per_pixel,
				      (double)unitPixelTmp / pss->maxPixelIn);

	/* apply filter to zoom horizontally from src to tmp */
	for ( k = 0; k < tmp_height; ++k )
	  zoom_x(tmp + k * tmp_width * values_per_pixel,
		 (byte *)src + k * src_width * values_per_pixel *
		   pss->sizeofPixelIn, pss->sizeofPixelIn,
		 tmp_width, src_width, values_per_pixel, contrib_x);

	contrib_y = calculate_contrib(pss->contrib, pss->items, yscale,
				      dst_height, src_height,
				      tmp_height, dst_width * values_per_pixel,
				      (double)pss->maxPixelOut / unitPixelTmp);

	/* apply filter to zoom vertically from tmp to dst */
	for ( i = 0; i < dst_height; ++i )
	  zoom_y((byte *)dst + i * dst_width * values_per_pixel *
		   pss->sizeofPixelOut, pss->sizeofPixelOut,
		   pss->maxPixelOut, tmp,
		 dst_width, tmp_width, values_per_pixel, contrib_y + i);

	return 0;
}

/* Clean up by freeing the scaler state. */
void
gs_image_scale_cleanup(gs_image_scale_state *pss)
{	gs_memory_t *mem = pss->memory;
	gs_free_object(mem, (void *)pss->src, "image_scale src"); /* no longer const */
	pss->src = 0;
	gs_free_object(mem, pss->dst, "image_scale dst");
	pss->dst = 0;
	gs_free_object(mem, pss->items, "image_scale contrib[*]");
	pss->items = 0;
	gs_free_object(mem, pss->contrib, "image_scale contrib");
	pss->contrib = 0;
	gs_free_object(mem, pss->tmp, "image_scale tmp");
	pss->tmp = 0;
}
