/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: icremap.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Interpreter color remapping structure */

#ifndef icremap_INCLUDED
#  define icremap_INCLUDED

#include "gsccolor.h"

/*
 * Define the structure used to communicate information back to the
 * interpreter for color remapping.  Pattern remapping doesn't use the
 * tint values, DeviceN remapping does.
 */
#ifndef int_remap_color_info_DEFINED
#  define int_remap_color_info_DEFINED
typedef struct int_remap_color_info_s int_remap_color_info_t;
#endif
struct int_remap_color_info_s {
    op_proc_t proc;		/* remapping procedure */
    float tint[GS_CLIENT_COLOR_MAX_COMPONENTS];
};

#define private_st_int_remap_color_info() /* in zgstate.c */\
  gs_private_st_simple(st_int_remap_color_info, int_remap_color_info_t,\
    "int_remap_color_info_t")

#endif /* icremap_INCLUDED */
