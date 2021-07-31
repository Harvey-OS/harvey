/* Copyright (C) 1992, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxfmap.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Fraction map representation for Ghostscript */

#ifndef gxfmap_INCLUDED
#  define gxfmap_INCLUDED

#include "gsrefct.h"
#include "gsstype.h"
#include "gxfrac.h"
#include "gxtmap.h"

/*
 * Define a cached map from fracs to fracs.  Level 1 uses this only
 * for the transfer function; level 2 also uses it for black generation
 * and undercolor removal.  Note that reference counting macros must
 * be used to allocate, free, and assign references to gx_transfer_maps.
 *
 * NOTE: proc and closure are redundant.  Eventually closure will replace
 * proc.  For now, things are in an uneasy intermediate state where
 * proc = 0 means use closure.
 */
/* log2... must not be greater than frac_bits, and must be least 8. */
#define log2_transfer_map_size 8
#define transfer_map_size (1 << log2_transfer_map_size)
							/*typedef struct gx_transfer_map_s gx_transfer_map; *//* in gxtmap.h */
struct gx_transfer_map_s {
    rc_header rc;
    gs_mapping_proc proc;	/* typedef is in gxtmap.h */
    gs_mapping_closure_t closure;	/* SEE ABOVE */
    /* The id changes whenever the map or function changes. */
    gs_id id;
    frac values[transfer_map_size];
};

extern_st(st_transfer_map);
#define public_st_transfer_map() /* in gscolor.c */\
  gs_public_st_composite(st_transfer_map, gx_transfer_map, "gx_transfer_map",\
    transfer_map_enum_ptrs, transfer_map_reloc_ptrs)

/*
 * Map a color fraction through a transfer map.  If the map is small,
 * we interpolate; if it is large, we don't, and save a lot of time.
 */
#define FRAC_MAP_INTERPOLATE (log2_transfer_map_size <= 8)
#if FRAC_MAP_INTERPOLATE

frac gx_color_frac_map(P2(frac, const frac *));		/* in gxcmap.c */

#  define gx_map_color_frac(pgs,cf,m)\
     (pgs->m->proc == gs_identity_transfer ? cf :\
      gx_color_frac_map(cf, &pgs->m->values[0]))

#else /* !FRAC_MAP_INTERPOLATE */

/* Do the lookup in-line. */
#  define gx_map_color_frac(pgs,cf,m)\
     (pgs->m->values[frac2bits(cf, log2_transfer_map_size)])

#endif /* (!)FRAC_MAP_INTERPOLATE */

/* Map a color fraction expressed as a byte through a transfer map. */
/* (We don't use this anywhere right now.) */
/****************
#if log2_transfer_map_size <= 8
#  define byte_to_tmx(b) ((b) >> (8 - log2_transfer_map_size))
#else
#  define byte_to_tmx(b)\
	(((b) << (log2_transfer_map_size - 8)) +\
	 ((b) >> (16 - log2_transfer_map_size)))
#endif
#define gx_map_color_frac_byte(pgs,b,m)\
 (pgs->m->values[byte_to_tmx(b)])
 ****************/

/* Map a floating point value through a transfer map. */
#define gx_map_color_float(pmap,v)\
  ((pmap)->values[(int)((v) * transfer_map_size + 0.5)] / frac_1_float)

/* Define a mapping procedure that just looks up the value in the cache. */
/* (It is equivalent to gx_map_color_float with the arguments swapped.) */
float gs_mapped_transfer(P2(floatp, const gx_transfer_map *));

/* Define an identity mapping procedure. */
float gs_identity_transfer(P2(floatp, const gx_transfer_map *));

#endif /* gxfmap_INCLUDED */
