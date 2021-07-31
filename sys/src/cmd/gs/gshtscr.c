/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gshtscr.c */
/* Screen (Type 1) halftone processing for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gzstate.h"
#include "gxdevice.h"			/* for gzht.h */
#include "gzht.h"

/* Import GCD from gsmisc.c */
extern int igcd(P2(int, int));

/* Structure descriptors */
private_st_gs_screen_enum();

/* GC procedures */
#define eptr ((gs_screen_enum *)vptr)

private ENUM_PTRS_BEGIN(screen_enum_enum_ptrs) {
	if ( index < 2 + st_ht_order_max_ptrs )
	  { gs_ptr_type_t ret = (*st_ht_order.enum_ptrs)(&eptr->order, sizeof(eptr->order), index-2, pep);
	    if ( ret == 0 )	/* don't stop early */
	      ret = ptr_struct_type, *pep = 0;
	    return ret;
	  }
	return (*st_halftone.enum_ptrs)(&eptr->halftone, sizeof(eptr->halftone), index-(2+st_ht_order_max_ptrs), pep);
	}
	ENUM_PTR(0, gs_screen_enum, pgs);
ENUM_PTRS_END

private RELOC_PTRS_BEGIN(screen_enum_reloc_ptrs) {
	RELOC_PTR(gs_screen_enum, pgs);
	(*st_halftone.reloc_ptrs)(&eptr->halftone, sizeof(gs_halftone), gcst);
	(*st_ht_order.reloc_ptrs)(&eptr->order, sizeof(gx_ht_order), gcst);
} RELOC_PTRS_END

#undef eptr

/* Define the default value of AccurateScreens that affects */
/* setscreen and setcolorscreen. */
private bool screen_accurate_screens = false;

/* Default AccurateScreens control */
void
gs_setaccuratescreens(bool accurate)
{	screen_accurate_screens = accurate;
}
bool
gs_currentaccuratescreens(void)
{	return screen_accurate_screens;
}

/*
 * We construct a halftone tile as a "super-cell" consisting of
 * multiple copies of a basic cell.  We characterize the basic cell
 * by two integers U and V, at least one of which is non-zero,
 * which define the vertices of the basic cell at device space coordinates
 * (0,0), (U,V), (U-V,U+V), and (-V,U).  If the coefficients of the
 * default device transformation matrix are xx, xy, yx, and yy, then
 * U and V are related to the frequency F and the angle A by:
 *	P = 72 / F;
 *	U = P * (xx * cos(A) + yx * sin(A));
 *	V = P * (xy * cos(A) + yy * sin(A)).
 * If we then define:
 *	D = gcd(abs(U), abs(V));
 *	W = (U * U + V * V) / D;
 * then the super-cell consists of W basic cells occupying W x W pixels.
 * The trick in all this is to find values of F and A that aren't too far
 * from the requested ones, and that yield a manageably small value for W.
 *
 * Note that the super-cell only has to be so large because we want to
 * use it directly to tile the plane.  In fact, it is composed of W / D
 * horizontal strips of size W x D, shifted horizontally with respect
 * to each other by S * D pixels, where S = (K * U / D) mod (W / D) for
 * the smallest non-negative integer K such that K * V / D = 1 mod (W / D).
 * The routines here only generate a single strip of samples, and let
 * gx_ht_construct_spot_order construct the rest.  We could save a lot of
 * space by storing only one strip, and letting the tile_rectangle routines
 * do the shifting at rendering time; however, we will leave this
 * added complexity for the future.
 */

/* Forward references */
private void pick_cell_size(P5(gs_screen_halftone *ph,
  const gs_matrix *pmat, long max_size,
  gs_int_point *pcell, gs_int_point *ptile));

/* Allocate a screen enumerator. */
gs_screen_enum *
gs_screen_enum_alloc(gs_memory_t *mem, client_name_t cname)
{	return gs_alloc_struct(mem, gs_screen_enum, &st_gs_screen_enum, cname);
}

/* Set up for halftone sampling. */
int
gs_screen_init(gs_screen_enum *penum, gs_state *pgs,
  gs_screen_halftone *phsp)
{	return gs_screen_init_accurate(penum, pgs, phsp,
				       screen_accurate_screens);
}
int
gs_screen_init_accurate(gs_screen_enum *penum, gs_state *pgs,
  gs_screen_halftone *phsp, bool accurate)
{	int code = gs_screen_order_init(&penum->order, pgs, phsp, accurate);
	if ( code < 0 )
		return code;
	return gs_screen_enum_init(penum, &penum->order, pgs, phsp);
}

/* Allocate and initialize a spot screen. */
/* This is the first half of gs_screen_init_accurate. */
int
gs_screen_order_init(gx_ht_order *porder, const gs_state *pgs,
  gs_screen_halftone *phsp, bool accurate)
{	gs_matrix imat;
	gs_int_point cell, tile;
	int code;
	int d;

	if ( phsp->frequency < 0.1 )
		return_error(gs_error_rangecheck);
	gs_deviceinitialmatrix(gs_currentdevice(pgs), &imat);
	pick_cell_size(phsp, &imat, (accurate ? max_ushort : 512),
		       &cell, &tile);
#define u cell.x
#define v cell.y
#define w tile.x
	d = igcd(u, v);
	code = gx_ht_alloc_order(porder, tile.x, tile.y,
				 w * d, pgs->memory);
	if ( code < 0 )
		return code;
	{	/* Unfortunately, we don't know any closed formula for */
		/* computing the shift, so we do it by enumeration. */
		uint k;
		uint wd = w / d;
		uint vk = (v < 0 ? v + w : v) / d;
		for ( k = 0; k < wd; k++ )
		{	if ( (k * vk) % wd == 1 )
				break;
		}
		porder->shift = ((((u < 0 ? u + w : u) / d) * k) % wd) * d;
		if_debug2('h', "[h]strip=%d shift=%d\n", d, porder->shift);
	}
#undef u
#undef v
#undef w
	return 0;
}

/* Prepare to sample a spot screen. */
/* This is the second half of gs_screen_init_accurate. */
int
gs_screen_enum_init(gs_screen_enum *penum, const gx_ht_order *porder,
  gs_state *pgs, gs_screen_halftone *phsp)
{	uint w = porder->width;
	uint d = porder->num_levels / w;

	penum->pgs = pgs;		/* ensure clean for GC */
	penum->order = *porder;
	penum->halftone.type = ht_type_screen;
	penum->halftone.params.screen = *phsp;
	penum->x = penum->y = 0;
	penum->strip = d;
	/* The transformation matrix must include normalization to the */
	/* interval (-1..1), and rotation by the negative of the angle. */
	/****** WRONG IF NON-SQUARE PIXELS ******/
	{	float xscale = 2.0 / sqrt((double)w * d);
		float yscale = xscale;
		gs_make_rotation(-phsp->actual_angle, &penum->mat);
		penum->mat.xx *= xscale, penum->mat.xy *= xscale;
		penum->mat.yx *= yscale, penum->mat.yy *= yscale;
		penum->mat.tx = -1.0;
		penum->mat.ty = -1.0;
		if_debug6('h', "[h]Screen: %dx%d [%f %f %f %f]\n",
			  porder->width, porder->height,
			  penum->mat.xx, penum->mat.xy,
			  penum->mat.yx, penum->mat.yy);
	}
	return 0;
}

/*
 * The following routine looks for "good" values of U and V
 * in a simple-minded way, according to the following algorithm:
 * We compute trial values u and v from the original values of F and A.
 * Normally these will not be integers.  We then examine the 4 pairs of
 * integers obtained by rounding each of u and v independently up or down,
 * and pick the pair U, V that yields the smallest value of W. If no pair
 * yields an acceptably small W, we divide both u and v by 2 and try again.
 * Then we run the equations backward to obtain the actual F and A.
 * This is fairly easy given that we require either xx = yy = 0 or
 * xy = yx = 0.  In the former case, we have
 *	U = (72 / F * xx) * cos(A);
 *	V = (72 / F * yy) * sin(A);
 * from which immediately
 *	A = arctan((V / yy) / (U / xx)),
 * or equivalently
 *	A = arctan((V * xx) / (U * yy)).
 * We can then obtain F as
 *	F = (72 * xx / U) * cos(A),
 * or equivalently
 *	F = (72 * yy / V) * sin(A).
 * For landscape devices, we replace xx by yx, yy by xy, and interchange
 * sin and cos, resulting in
 *	A = arctan((U * xy) / (V * yx))
 * and
 *	F = (72 * yx / U) * sin(A)
 * or
 *	F = (72 * xy / V) * cos(A).
 */
/* ph->frequency and ph->angle are input parameters; */
/* the routine sets ph->actual_frequency and ph->actual_angle. */
private void
pick_cell_size(gs_screen_halftone *ph, const gs_matrix *pmat, long max_size,
  gs_int_point *pcell, gs_int_point *ptile)
{	bool landscape = (pmat->xy != 0.0 || pmat->yx != 0.0);
	/* Account for a possibly reflected coordinate system. */
	/* See gxstroke.c for the algorithm. */
	bool reflected =
		(landscape ? pmat->xy * pmat->yx > pmat->xx * pmat->yy :
		 (pmat->xx < 0) != (pmat->yy < 0));
	int reflection = (reflected ? -1 : 1);
	int rotation =
		(landscape ? (pmat->yx < 0 ? 90 : -90) :
			     pmat->xx < 0 ? 180 : 0);
	double a = ph->angle;
	gs_matrix rmat;
	gs_point t;
	int u0, v0, ut, vt, u, v, w;
	long w_size;
	double rf, ra;

	/*
	 * We need to find a vector in device space whose length is
	 * 1 inch / ph->frequency and whose angle is ph->angle.
	 * Because device pixels may not be square, we can't simply
	 * map the length to device space and then rotate it;
	 * instead, since we know that user space is uniform in X and Y,
	 * we calculate the correct angle in user space before rotation.
	 */

	a = a * reflection + rotation;

	/* Compute trial values of u and v. */

	gs_make_rotation(a, &rmat);
	gs_distance_transform(72.0 / ph->frequency, 0.0, &rmat, &t);
	gs_distance_transform(t.x, t.y, pmat, &t);
	if_debug9('h', "[h]Requested: f=%g a=%g mat=[%g %g %g %g] max_size=%ld =>\n     u=%g v=%g\n",
		  ph->frequency, ph->angle,
		  pmat->xx, pmat->xy, pmat->yx, pmat->yy, max_size,
		  t.x, t.y);

	/* Choose the "best" U and V. */

	u0 = (int)floor(t.x + 0.0001);
	v0 = (int)floor(t.y + 0.0001);
	/* Force a halfway reasonable cell size. */
	if ( u0 == 0 && v0 == 0 )
		u0 = v0 = 1;
	while ( any_abs(u0) + any_abs(v0) < 4 )
		u0 <<= 1, v0 <<= 1;
	u = u0, v = v0;			/* only to pacify gcc */
try_size:
	w_size = max_size;
	for ( ut = u0 + 1; ut >= u0; ut-- )
	  for ( vt = v0 + 1; vt >= v0; vt-- )
	{	int d = igcd(ut, vt);
		long utd = ut / d * (long)ut, vtd = vt / d * (long)vt;
		long wt = any_abs(utd) + any_abs(vtd);
		long wt_size;
		if_debug5('h', "[h]trying ut=%d, vt=%d => d=%d, wt=%ld, wt_size=%ld\n",
			  ut, vt, d, wt, bitmap_raster(wt) * wt);
		if ( wt < max_short &&
		     (wt_size = bitmap_raster(wt) * wt) < w_size
		   )
		  u = ut, v = vt, w = wt, w_size = wt_size;
	}
	if ( w_size == max_size )
	{	/* We couldn't find an acceptable U and V. */
		/* Shrink the cell and try again. */
		u0 >>= 1;
		v0 >>= 1;
		goto try_size;
	}

	/* Compute the actual values of F and A. */

	if ( landscape )
		ra = atan2(u * pmat->xy, v * pmat->yx),
		rf = 72.0 * (u == 0 ? pmat->xy / v * cos(ra) : pmat->yx / u * sin(ra));
	else
		ra = atan2(v * pmat->xx, u * pmat->yy),
		rf = 72.0 * (u == 0 ? pmat->yy / v * sin(ra) : pmat->xx / u * cos(ra));

	/* Deliver the results. */
	/****** WRONG IF NON-SQUARE PIXELS ******/

	pcell->x = u, pcell->y = v;
	ptile->x = w, ptile->y = w;
	ph->actual_frequency = fabs(rf);
	{	/* Normalize the angle to the requested quadrant. */
		float aa = (ra * radians_to_degrees - rotation) * reflection;
		aa -= floor(aa / 180.0) * 180.0;
		aa += floor(ph->angle / 180.0) * 180.0;
		ph->actual_angle = aa;
	}
	if_debug5('h', "[h]Chosen: f=%g a=%g u=%d v=%d w=%d\n",
		  ph->actual_frequency, ph->actual_angle, u, v, (int)w);
}

/* Report current point for sampling */
int
gs_screen_currentpoint(gs_screen_enum *penum, gs_point *ppt)
{	gs_point pt;
	int code;
	if ( penum->y >= penum->strip )		/* all done */
	{	gx_ht_construct_spot_order(&penum->order);
		return 1;
	}
	/* We displace the sampled coordinates very slightly */
	/* in order to reduce the likely number of points */
	/* for which the spot function returns the same value. */
	if ( (code = gs_point_transform(penum->x + 0.501, penum->y + 0.498, &penum->mat, &pt)) < 0 )
		return code;
	if ( pt.x < -1.0 )
		pt.x += ((int)(-ceil(pt.x)) + 1) & ~1;
	else if ( pt.x >= 1.0 )
		pt.x -= ((int)pt.x + 1) & ~1;
	if ( pt.y < -1.0 )
		pt.y += ((int)(-ceil(pt.y)) + 1) & ~1;
	else if ( pt.y >= 1.0 )
		pt.y -= ((int)pt.y + 1) & ~1;
	*ppt = pt;
	return 0;
}

/* Record next halftone sample */
int
gs_screen_next(gs_screen_enum *penum, floatp value)
{	ht_sample_t sample;
	int width = penum->order.width;
	if ( value < -1.0 || value > 1.0 )
		return_error(gs_error_rangecheck);
	/* The following statement was split into two */
	/* to work around a bug in the Siemens C compiler. */
	sample = (ht_sample_t)(value * max_ht_sample);
	sample += max_ht_sample;	/* convert from signed to biased */
#ifdef DEBUG
if ( gs_debug_c('h') )
   {	gs_point pt;
	gs_screen_currentpoint(penum, &pt);
	dprintf6("[h]sample x=%d y=%d (%f,%f): %f -> %u\n",
		 penum->x, penum->y, pt.x, pt.y, value, sample);
   }
#endif
	penum->order.bits[penum->y * width + penum->x].mask = sample;
	if ( ++(penum->x) >= width )
		penum->x = 0, ++(penum->y);
	return 0;
}

/* Install a fully constructed screen in the gstate. */
int
gs_screen_install(gs_screen_enum *penum)
{	gx_device_halftone dev_ht;
	dev_ht.order = penum->order;
	dev_ht.components = 0;
	return gx_ht_install(penum->pgs, &penum->halftone, &dev_ht);
}
