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

/* gsht.c */
/* setscreen operator for Ghostscript library */
#include "memory_.h"
#include <stdlib.h>		/* for qsort */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gzstate.h"
#include "gxdevice.h"			/* for gzht.h */
#include "gzht.h"

/* Import GCD from gsmisc.c */
extern int igcd(P2(int, int));

/* Forward declarations */
int gx_ht_process_screen(P4(gs_screen_enum *, gs_state *,
  gs_screen_halftone *, bool));		/* exported for gsht1.c */
void gx_set_effective_transfer(P1(gs_state *));

/* Structure types */
public_st_ht_order();
public_st_halftone();
public_st_device_halftone();

/* GC procedures */

#define hptr ((gs_halftone *)vptr)

private ENUM_PTRS_BEGIN(halftone_enum_ptrs) return 0;
	case 0:
		switch ( hptr->type )
		{
		case ht_type_threshold:
		  *pep = (void *)hptr->params.threshold.thresholds; break; /* discard const */
		case ht_type_multiple:
		  *pep = hptr->params.multiple.components; break;
		default:
		  return 0;
		}
		break;
ENUM_PTRS_END

private RELOC_PTRS_BEGIN(halftone_reloc_ptrs) {
	switch ( hptr->type )
	  {
	  case ht_type_threshold:
	    RELOC_PTR(gs_halftone, params.threshold.thresholds);
	    break;
	  case ht_type_multiple:
	    RELOC_PTR(gs_halftone, params.multiple.components);
	    break;
	  case ht_type_none:
	  case ht_type_screen:
	  case ht_type_colorscreen:
	  case ht_type_spot:
	    break;
	  }
} RELOC_PTRS_END

#undef hptr

/* setscreen */
int
gs_setscreen(gs_state *pgs, gs_screen_halftone *phsp)
{	gs_screen_enum senum;
	int code = gx_ht_process_screen(&senum, pgs, phsp,
					gs_currentaccuratescreens());
	if ( code < 0 )
		return code;
	return gs_screen_install(&senum);
}

/* currentscreen */
int
gs_currentscreen(const gs_state *pgs, gs_screen_halftone *phsp)
{	switch ( pgs->halftone->type )
	{
	case ht_type_screen:
		*phsp = pgs->halftone->params.screen;
		return 0;
	case ht_type_colorscreen:
		*phsp = pgs->halftone->params.colorscreen.screens.colored.gray;
		return 0;
	default:
		return_error(gs_error_undefined);
	}
}

/* .currentscreenlevels */
int
gs_currentscreenlevels(const gs_state *pgs)
{	return pgs->dev_ht->order.num_levels;
}

/* sethalftonephase */
int
gs_sethalftonephase(gs_state *pgs, int x, int y)
{	pgs->ht_phase.x = x;
	pgs->ht_phase.y = y;
	gx_ht_set_phase(pgs);
	return 0;
}

/* currenthalftonephase */
int
gs_currenthalftonephase(const gs_state *pgs, gs_int_point *pphase)
{	*pphase = pgs->ht_phase;
	return 0;
}

/* currenthalftone */
int
gs_currenthalftone(gs_state *pgs, gs_halftone *pht)
{	*pht = *pgs->halftone;
	return 0;
}

/* ------ Internal routines ------ */

/* Process one screen plane. */
int
gx_ht_process_screen(gs_screen_enum *penum, gs_state *pgs,
  gs_screen_halftone *phsp, bool accurate)
{	gs_point pt;
	int code = gs_screen_init_accurate(penum, pgs, phsp, accurate);
	if ( code < 0 ) return code;
	while ( (code = gs_screen_currentpoint(penum, &pt)) == 0 )
		if ( (code = gs_screen_next(penum, (*phsp->spot_function)(pt.x, pt.y))) < 0 )
			return code;
	return 0;
}

/* Compute the negated halftone phase mod the tile size. */
/* This is the displacement of the tile relative to the device coordinates. */
private void near
order_set_phase(register gx_ht_order *porder, gs_state *pgs)
{	if ( porder->width == 0 )
		porder->phase.x = 0;
	else
	{	if ( (porder->phase.x = -pgs->ht_phase.x % porder->width) < 0 )
			porder->phase.x += porder->width;
	}
	if ( porder->height == 0 )
		porder->phase.y = 0;
	else
	{	if ( (porder->phase.y = -pgs->ht_phase.y % porder->height) < 0 )
			porder->phase.y += porder->height;
	}
}
void
gx_ht_set_phase(gs_state *pgs)
{	gx_device_halftone *pdht = pgs->dev_ht;
	order_set_phase(&pdht->order, pgs);
	if ( pdht->components != 0 )
	{	uint i;
		for ( i = 0; i < pdht->num_comp; i++ )
			order_set_phase(&pdht->components[i].corder, pgs);
	}
}

/* Allocate and initialize the contents of a halftone order. */
int
gx_ht_alloc_order(register gx_ht_order *porder, uint width, uint height,
  uint num_levels, gs_memory_t *mem)
{	uint size = width * height;
	gx_ht_order order;
	order = *porder;
	order.width = width;
	order.height = height;
	order.raster = bitmap_raster(width);
	order.num_levels = num_levels;
	order.num_bits = size;
	order.levels =
	  (uint *)gs_alloc_byte_array(mem, num_levels, sizeof(uint),
				      "ht order(levels)");
	order.bits =
	  (gx_ht_bit *)gs_alloc_byte_array(mem, size, sizeof(gx_ht_bit),
					   "ht order(bits)");
	if ( order.levels == 0 || order.bits == 0 )
	{	gs_free_object(mem, order.bits, "ht order(bits)");
		gs_free_object(mem, order.levels, "ht order(levels)");
		return_error(gs_error_VMerror);
	}
	order.cache = 0;
	order.transfer = 0;
	*porder = order;
	return 0;
}

/* Compare keys ("masks", actually sample values) for qsort. */
private int
compare_samples(const void *p1, const void *p2)
{	ht_sample_t m1 = ((const gx_ht_bit *)p1)->mask;
	ht_sample_t m2 = ((const gx_ht_bit *)p2)->mask;
	return (m1 < m2 ? -1 : m1 > m2 ? 1 : 0);
}
/* Sort the halftone order by sample value. */
void
gx_sort_ht_order(gx_ht_bit *recs, uint N)
{	int i;
	/* Tag each sample with its index, for sorting. */
	for ( i = 0; i < N; i++ )
	  recs[i].offset = i;
	qsort((void *)recs, N, sizeof(*recs), compare_samples);
#ifdef DEBUG
if ( gs_debug_c('h') )
	{	uint i;
		dprintf("[h]Sorted samples:\n");
		for ( i = 0; i < N; i++ )
			dprintf3("%5u: %5u: %u\n",
				 i, recs[i].offset, recs[i].mask);
	}
#endif
}

/* Construct the halftone order from a sampled spot function. */
/* Only width x strip samples have been filled in; */
/* we must replicate the resulting sorted order vertically, */
/* shifting it by shift each time. */
void
gx_ht_construct_spot_order(gx_ht_order *porder)
{	uint *levels = porder->levels;
	uint num_levels = porder->num_levels;	/* = width x strip */
	uint width = porder->width;
	uint height = porder->height;
	uint strip = num_levels / width;
	uint shift = porder->shift;
	uint copies = height / strip;
	gx_ht_bit *bits = porder->bits;
	gx_ht_bit *bp = bits + porder->num_bits - 1;
	uint i;

	gx_sort_ht_order(bits, num_levels);
	if_debug5('h', "[h]spot order: num_levels=%u w=%u h=%u strip=%u shift=%u\n",
		  num_levels, width, height, strip, shift);
	/* Replicate the sorted order vertically. */
	for ( i = num_levels; i > 0; )
	{	uint offset = bits[--i].offset;
		uint x = offset % height;
		uint hy = offset - x;
		uint k;
		levels[i] = i * copies;
		for ( k = 0; k < copies;
		      k++, bp--, hy += num_levels, x = (x + shift) % width
		    )
			bp->offset = hy + x;
	}
	gx_ht_construct_bits(porder);
}

/* Construct offset/masks from the whitening order. */
/* porder->bits[i].offset contains the index of the bit position */
/* that is i'th in the whitening order. */
void
gx_ht_construct_bits(gx_ht_order *porder)
{	uint width = porder->width;
	uint size = porder->num_bits;
	gx_ht_bit *bits = porder->bits;
	uint i;
	gx_ht_bit *phb;
	byte *pb;
	uint padding = porder->raster * 8 - width;

	for ( i = 0, phb = bits; i < size; i++, phb++ )
	{	int pix = phb->offset;
		ht_mask_t mask;
		pix += pix / width * padding;
		phb->offset = (pix >> 3) & -sizeof(mask);
		mask = (ht_mask_t)1 << (~pix & (ht_mask_bits - 1));
		/* Replicate the mask bits. */
		pix = ht_mask_bits - width;
		while ( (pix -= width) >= 0 )
			mask |= mask >> width;
		/* Store the mask, reversing bytes if necessary. */
		phb->mask = 0;
		for ( pb = (byte *)&phb->mask + (sizeof(mask) - 1);
		      mask != 0;
		      mask >>= 8, pb--
		    )
			*pb = (byte)mask;
	}
#ifdef DEBUG
if ( gs_debug_c('h') )
	   {	dprintf1("[h]Halftone order bits 0x%lx:\n", (ulong)bits);
		for ( i = 0, phb = bits; i < size; i++, phb++ )
			dprintf3("%4d: %u:0x%lx\n", i, phb->offset,
				 (ulong)phb->mask);
	   }
#endif
}

/* Install a new halftone in the graphics state. */
int
gx_ht_install(gs_state *pgs, const gs_halftone *pht,
  const gx_device_halftone *pdht)
{	gx_device_halftone *pgdht = pgs->dev_ht;
	if ( (ulong)pdht->order.raster * pdht->order.height >
	       pgs->ht_cache->bits_size
	   )
		return_error(gs_error_limitcheck);
	*pgs->halftone = *pht;
	*pgdht = *pdht;
	gx_ht_set_phase(pgs);
	/* Clear the cache, to avoid confusion in case the address of */
	/* a new order vector matches that of a (deallocated) old one. */
	gx_ht_clear_cache(pgs->ht_cache);
	/* Set the color_indices according to the device color_info. */
	/* Also compute the LCM of the primary color cell sizes. */
	if ( pdht->components != 0 )
	{	static const gs_ht_separation_name dcnames[5][4] =
		{	{ gs_ht_separation_Default },	/* not used */
			{ gs_ht_separation_Default, gs_ht_separation_Default,
			  gs_ht_separation_Default, gs_ht_separation_Gray
			},
			{ gs_ht_separation_Default },	/* not used */
			{ gs_ht_separation_Red, gs_ht_separation_Green,
			  gs_ht_separation_Blue, gs_ht_separation_Default
			},
			{ gs_ht_separation_Cyan, gs_ht_separation_Magenta,
			  gs_ht_separation_Yellow, gs_ht_separation_Black
			}
		};
		static const gs_ht_separation_name cscnames[4] =
			{ gs_ht_separation_Red, gs_ht_separation_Green,
			  gs_ht_separation_Blue, gs_ht_separation_Default
			};
		int num_comps = gs_currentdevice_inline(pgs)->color_info.
		  num_components;
		const gs_ht_separation_name _ds *cnames = dcnames[num_comps];
		int lcm_width = 1, lcm_height = 1;
		uint i;

		/* Halftones set by setcolorscreen are supposed to work */
		/* for both RGB and CMYK, so we need a special check here. */
		if ( num_comps == 4 && pht->type == ht_type_colorscreen )
			cnames = cscnames;
		if_debug4('h', "[h]dcnames=%lu,%lu,%lu,%lu\n",
			  (ulong)cnames[0], (ulong)cnames[1],
			  (ulong)cnames[2], (ulong)cnames[3]);
		memset(pgdht->color_indices, 0, sizeof(pdht->color_indices));
		for ( i = 0; i < pdht->num_comp; i++ )
		{	const gx_ht_order_component *pcomp =
			  &pdht->components[i];
			int j;
			if_debug2('h', "[h]cname[%d]=%lu\n",
				  i, (ulong)pcomp->cname);
			for ( j = 0; j < 4; j++ )
			{	if ( pcomp->cname == cnames[j] )
				{	uint cw = pcomp->corder.width;
					uint ch = pcomp->corder.height;
					int dw = lcm_width /
					  igcd(lcm_width, cw % lcm_width);
					int dh = lcm_height /
					  igcd(lcm_height, ch % lcm_height);
					if_debug2('h',
						  "[h]color_indices[%d]=%d\n",
						  j, i);
					pgdht->color_indices[j] = i;
					lcm_width =
					  (cw > max_int / dw ? max_int :
					   cw * dw);
					lcm_height =
					  (ch > max_int / dh ? max_int :
					   ch * dh);
				}
			}
		}
		pgdht->lcm_width = lcm_width;
		pgdht->lcm_height = lcm_height;
	}
	else
	  {	/* Only one component. */
		pgdht->lcm_width = pgdht->order.width;
		pgdht->lcm_height = pgdht->order.height;
	  }
	if_debug2('h', "[h]LCM=(%d,%d)\n",
		  pgdht->lcm_width, pgdht->lcm_height);
	gx_set_effective_transfer(pgs);
	gx_unset_dev_color(pgs);
	return 0;
}

/* Reestablish the effective transfer functions, taking into account */
/* any overrides from halftone dictionaries. */
void
gx_set_effective_transfer(gs_state *pgs)
{	const gx_device_halftone *pdht = pgs->dev_ht;
	pgs->effective_transfer = pgs->set_transfer;		/* default */
	if ( pdht->components == 0 )
	{	/* Check for transfer function override in single halftone */
		gx_transfer_map *pmap = pdht->order.transfer;
		if ( pmap != 0 )
		  pgs->effective_transfer.indexed[0] =
		    pgs->effective_transfer.indexed[1] =
		    pgs->effective_transfer.indexed[2] =
		    pgs->effective_transfer.indexed[3] = pmap;
	}
	else
	{	/* Check in all 4 standard separations */
		int i;
		for ( i = 0; i < 4; ++i )
		{	gx_transfer_map *pmap =
			  pdht->components[pdht->color_indices[i]].corder.
			    transfer;
			if ( pmap != 0 )
			  pgs->effective_transfer.indexed[i] = pmap;
		}
	}
}
