/* Copyright (C) 1989, 1995, 1998, 2001 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gstypes.h,v 1.3 2001/08/25 06:46:21 lpd Exp $ */
/* Miscellaneous common types for Ghostscript library */

#ifndef gstypes_INCLUDED
#  define gstypes_INCLUDED

/*
 * Define a type used internally for unique IDs of various kinds
 * (primarily, but not exclusively, character and halftone bitmaps).
 * These IDs bear no relation to any other ID space; we generate them all
 * ourselves.
 */
typedef ulong gs_id;

#define gs_no_id 0L

/*
 * Define a sensible representation of a string, as opposed to
 * the C char * type (which can't store arbitrary data, represent
 * substrings, or perform concatenation without destroying aliases).
 *
 * If a byte * pointer P is the result of allocating a string of size N,
 * then any substring of [P .. P+N) is a valid gs_string, i.e., any
 * gs_string S is valid (until the string is deallocated) if it has P <=
 * S.data and S.data + S.size <= P + N.
 */
#define GS_STRING_COMMON\
    byte *data;\
    uint size
typedef struct gs_string_s {
    GS_STRING_COMMON;
} gs_string;
#define GS_CONST_STRING_COMMON\
    const byte *data;\
    uint size
typedef struct gs_const_string_s {
    GS_CONST_STRING_COMMON;
} gs_const_string;

/*
 * Since strings are allocated differently from ordinary objects, define a
 * structure that can reference either a string or a byte object.  If bytes
 * == 0, data and size are the same as for a gs_string.  If bytes != 0, data
 * and size point within the object addressed by bytes (i.e., the bytes
 * member plays the role of P in the consistency condition given for
 * gs_string above).  Thus in either case, code can process the string using
 * only data and size: bytes is only relevant for garbage collection.
 *
 * Note: for garbage collection purposes, the string_common members must
 * come first.
 */
typedef struct gs_bytestring_s {
    GS_STRING_COMMON;
    byte *bytes;		/* see above */
} gs_bytestring;
typedef struct gs_const_bytestring_s {
    GS_CONST_STRING_COMMON;
    const byte *bytes;		/* see above */
} gs_const_bytestring;

#define gs_bytestring_from_string(pbs, dat, siz)\
  ((pbs)->data = (dat), (pbs)->size = (siz), (pbs)->bytes = 0)
#define gs_bytestring_from_bytes(pbs, byts, offset, siz)\
  ((pbs)->data = ((pbs)->bytes = (byts)) + (offset), (pbs)->size = (siz))

/*
 * Define types for Cartesian points.
 */
typedef struct gs_point_s {
    double x, y;
} gs_point;
typedef struct gs_int_point_s {
    int x, y;
} gs_int_point;

/*
 * Define a scale for oversampling.  Clients don't actually use this,
 * but this seemed like the handiest place for it.
 */
typedef struct gs_log2_scale_point_s {
    int x, y;
} gs_log2_scale_point;

/*
 * Define types for rectangles in the Cartesian plane.
 * Note that rectangles are half-open, i.e.: their width is
 * q.x-p.x and their height is q.y-p.y; they include the points
 * (x,y) such that p.x<=x<q.x and p.y<=y<q.y.
 */
typedef struct gs_rect_s {
    gs_point p, q;		/* origin point, corner point */
} gs_rect;
typedef struct gs_int_rect_s {
    gs_int_point p, q;
} gs_int_rect;

#endif /* gstypes_INCLUDED */
