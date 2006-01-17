/* Copyright (C) 1994, 1999 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: icsmap.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Interface to shared routines for loading the cached color space maps. */

#ifndef icsmap_INCLUDED
#  define icsmap_INCLUDED

/*
 * Set up to load a cached map for an Indexed or substituted Separation
 * color space.  The implementation is in zcsindex.c.  When the map1
 * procedure is called, the following structure is on the e_stack:
 */
#define num_csme 5
#  define csme_num_components (-4)	/* t_integer */
#  define csme_map (-3)		/* t_struct (bytes) */
#  define csme_proc (-2)	/* -procedure- */
#  define csme_hival (-1)	/* t_integer */
#  define csme_index 0		/* t_integer */
/*
 * Note that the underlying color space parameter is a direct space, not a
 * base space, since the underlying space of an Indexed color space may be
 * a Separation or DeviceN space.
 */
int zcs_begin_map(i_ctx_t *i_ctx_p, gs_indexed_map ** pmap,
		  const ref * pproc, int num_entries,
		  const gs_direct_color_space * base_space,
		  op_proc_t map1);

#endif /* icsmap_INCLUDED */
