/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscsepnm.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Standard color space separation names */

#ifndef gscsepnm_INCLUDED
#  define gscsepnm_INCLUDED

/*
 * Define enumeration indices for the standard separation names, and the
 * corresponding name strings. These are only used internally: in all
 * externally accessible APIs, separations are defined either by a string
 * name or by an opaque identifier.
 *
 * NB: the enumeration and the list of strings must be synchronized.  */
typedef enum {
    gs_ht_separation_Default,	/* must be first */
    gs_ht_separation_Gray,
    gs_ht_separation_Red,
    gs_ht_separation_Green,
    gs_ht_separation_Blue,
    gs_ht_separation_Cyan,
    gs_ht_separation_Magenta,
    gs_ht_separation_Yellow,
    gs_ht_separation_Black
} gs_ht_separation_name;

#define gs_ht_separation_name_strings            \
    "Default", "Gray", "Red", "Green", "Blue",   \
    "Cyan", "Magenta", "Yellow", "Black"

#endif /* gscsepnm_INCLUDED */
