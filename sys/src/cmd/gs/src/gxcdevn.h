/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxcdevn.h,v 1.6 2002/08/22 07:12:29 henrys Exp $ */
/* Internal definitions for DeviceN color spaces */

#ifndef gxcdevn_INCLUDED
#  define gxcdevn_INCLUDED

#include "gsrefct.h"
#include "gxcindex.h"

/* Cache for DeviceN color.  Note that currently this is a 1-entry cache. */
#ifndef gs_device_n_map_DEFINED
#  define gs_device_n_map_DEFINED
typedef struct gs_device_n_map_s gs_device_n_map;
#endif
struct gs_device_n_map_s {
    rc_header rc;
    int (*tint_transform)(const float *in, float *out,
                          const gs_imager_state *pis, void *data);
    void *tint_transform_data;
    bool cache_valid;
    float tint[GS_CLIENT_COLOR_MAX_COMPONENTS];
    frac conc[GX_DEVICE_COLOR_MAX_COMPONENTS];
};
#define private_st_device_n_map() /* in gscdevn.c */\
  gs_private_st_ptrs1(st_device_n_map, gs_device_n_map, "gs_device_n_map",\
    device_n_map_enum_ptrs, device_n_map_reloc_ptrs, tint_transform_data)

/* Allocate and initialize a DeviceN map. */
int alloc_device_n_map(gs_device_n_map ** ppmap, gs_memory_t * mem,
                       client_name_t cname);

/* Check if we are using the alternate color space */
bool using_alt_color_space(const gs_state * pgs);

#endif /* gxcdevn_INCLUDED */
