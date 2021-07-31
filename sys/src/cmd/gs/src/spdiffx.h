/* Copyright (C) 1994, 1995, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: spdiffx.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Definitions for PixelDifference filters */
/* Requires strimpl.h */

#ifndef spdiffx_INCLUDED
#  define spdiffx_INCLUDED

/*
 * Define the maximum value for Colors.  This must be at least 4, but can
 * be arbitrarily large: the only cost is a larger stream state structure.
 */
#define s_PDiff_max_Colors 16

/* PixelDifferenceDecode / PixelDifferenceEncode */
typedef struct stream_PDiff_state_s {
    stream_state_common;
    /* The client sets the following before initialization. */
    int Colors;			/* # of colors, 1..s_PDiff_max_Colors */
    int BitsPerComponent;	/* 1, 2, 4, 8 */
    int Columns;
    /* The init procedure computes the following. */
    uint row_count;		/* # of bytes per row */
    byte end_mask;		/* mask for left-over bits in last byte */
    int case_index;		/* switch index for case dispatch */
    /* The following are updated dynamically. */
    uint row_left;		/* # of bytes left in row */
    byte prev[s_PDiff_max_Colors];	/* previous sample */
} stream_PDiff_state;

#define private_st_PDiff_state()	/* in spdiff.c */\
  gs_private_st_simple(st_PDiff_state, stream_PDiff_state,\
    "PixelDifferenceEncode/Decode state")
#define s_PDiff_set_defaults_inline(ss)\
  ((ss)->Colors = 1, (ss)->BitsPerComponent = 8, (ss)->Columns = 1)
extern const stream_template s_PDiffD_template;
extern const stream_template s_PDiffE_template;

#endif /* spdiffx_INCLUDED */
