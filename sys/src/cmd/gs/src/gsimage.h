/* Copyright (C) 1992, 1995, 1996, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsimage.h,v 1.9 2005/06/21 16:50:51 igor Exp $ */
/* Generic image rendering interface */
/* Requires gsstate.h */

#ifndef gsimage_INCLUDED
#  define gsimage_INCLUDED

#include "gsiparam.h"

/*
  The API defined in this file and implemented in gsimage.c provides a
  layer of buffering between clients and the underlying image processing
  interface defined in gxdevcli.h (begin_[typed_]image device procedure) and
  gxiparam.h (image processing procedures).

  Two of the underlying image processing procedures defined in gxiparam.h
  define and process the actual data:

  - The underlying planes_wanted image processing procedure indicates which
    data planes are needed (not necessarily all of them, in the case of
    images with differently scaled planes), and may also change the widths
    and/or depths of the planes.  It may return different results at
    different times, if the widths, depths, or planes wanted changes in the
    course of the image.

  - The underlying plane_data procedure actually processes the image.  Each
    call of plane_data requires an integral number of scan lines for each
    wanted plane.  If the widths, depths, or planes wanted vary from call to
    call, plane_data may choose to accept fewer scan lines than provided.
    If this happens, it is the client's responsibility to call planes_wanted
    to find out which planes are now wanted, and then call plane_data again
    with data for (only) the wanted planes.

  Conceptually, the gs_image_next_planes procedure defined here provides the
  same function as the plane_data procedure, except that:

  - The data need not consist of complete scan lines, or be aligned in any
    way;

  - If a single call passes multiple scan lines for a single plane, each
    scan line is only padded to a byte boundary, not to an alignment
    boundary;

  - Different amounts of data (including none) may be passed for each plane,
    independent of which planes need data or the amount of data that makes
    up a complete scan line for a plane;

  - The amount actually used is returned as a count of bytes used
    (separately for each plane) rather than a count of scan lines.

  There is one added complication.  To avoid allocating large amounts of
  storage, gs_image_next_planes may choose to copy only part of the data,
  retaining the rest of it by reference.  Clients must be informed about
  this, since if the data is in a stream buffer, the data may move.  To
  accommodate this possibility, on subsequent calls, any data passed by the
  client for a plane with retained data *replaces* the retained data rather
  than (as one might expect) appending to it; if the client passes no data
  for that plane, the retained data stays retained if needed.
  gs_image_next_planes returns information about retained data on each call,
  so the client need not keep track of it.

  The gs_image_planes_wanted procedure is analogous to planes_wanted.  It
  identifies a plane as wanted if both of the following are true:

  - The underlying planes_wanted procedure says the plane is wanted.

  - Less than a full scan line of data is already buffered for that plane
    (including retained data if any).

  This is not sufficient information for the PostScript interpreter for the
  case where the data sources are procedures, which must be called in a
  cyclic order even if they return less than a full scan line.  For this
  case, the interpreter must keep track of a plane index itself, cycling
  through the planes that gs_image_planes_wanted says are wanted (which may
  vary from cycle to cycle).

  There is an older, simpler procedure gs_image_next that simply cycles
  through the planes in order.  It does not offer the option of replacing
  retained data, of passing data for more than one plane at a time, or of
  passing data for planes in an arbitrary order.  Consequently, it is only
  usable with image types where all planes are always wanted.  gs_image_next
  should also only be used when all planes have the same width and depth and
  the same amount of data is passed for each plane in a given cycle of
  calls.  This is not currently checked; however, gs_image_next will give an
  error if an attempt is made to pass data for a plane that has any retained
  data.
*/

/*
 * The image painting interface uses an enumeration style:
 * the client initializes an enumerator, then supplies data incrementally.
 */

/*
 * Create an image enumerator given image parameters and a graphics state.
 * This calls the device's begin_typed_image procedure with appropriate
 * parameters.  Note that this is an enumerator that requires entire
 * rows of data, not the buffered enumerator used by the procedures below:
 * for this reason, we may move the prototype elsewhere in the future.
 */
#ifndef gx_image_enum_common_t_DEFINED
#  define gx_image_enum_common_t_DEFINED
typedef struct gx_image_enum_common_s gx_image_enum_common_t;
#endif

int gs_image_begin_typed(const gs_image_common_t * pic, gs_state * pgs,
			 bool uses_color, gx_image_enum_common_t ** ppie);

typedef struct gs_image_enum_s gs_image_enum;
gs_image_enum *gs_image_enum_alloc(gs_memory_t *, client_name_t);

/*
 * gs_image_init returns 1 for an empty image, 0 normally, <0 on error.
 * Note that gs_image_init serves for both image and imagemask,
 * depending on the value of ImageMask in the image structure.
 */
#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/* Initialize an enumerator for an ImageType 1 image. */
int gs_image_init(gs_image_enum * penum, const gs_image_t * pim,
		  bool MultipleDataSources, gs_state * pgs);

/* Initialize an enumerator for a general image. 
   penum->memory must be initialized in advance.
*/
int gs_image_enum_init(gs_image_enum * penum,
		       gx_image_enum_common_t * pie,
		       const gs_data_image_t * pim, gs_state *pgs);

/*
 * Return the number of bytes of data per row
 * (per plane, if there are multiple planes).
 */
uint gs_image_bytes_per_plane_row(const gs_image_enum * penum, int plane);

#define gs_image_bytes_per_row(penum)\
  gs_image_bytes_per_plane_row(penum, 0)

/*
 * Return a byte vector indicating which planes (still) need data for the
 * current row.  See above for details.
 */
const byte *gs_image_planes_wanted(gs_image_enum *penum);

/*
 * Pass multiple or selected planes of data for an image.  See above for
 * details.
 *
 *   plane_data[]  is an array of size num_planes of gs_const_string type
 *                 which contains the pointer and the length for each.
 *   used[]        is also of size num_planes and will be set to the number of
 *                 bytes consumed for each plane.
 *
 * The amount of data available for a plane (i.e., the size of a
 * plane_data[] element) can be 0 in order to provide data for a single
 * plane or only some of the planes.  Note that if data is retained,
 * it is not "consumed": e.g., if all of the data for a given plane is
 * retained, used[] for that plane will be set to 0.
 *
 * Returns 1 if end of image, < 0 error code, otherwise 0.  In any case,
 * stores pointers to the retained strings into plane_data[].  Note that
 * used[] and plane_data[] are set even in the error or end-of-image case.
 */
int gs_image_next_planes(gs_image_enum *penum, gs_const_string *plane_data,
			 uint *used);

/* Pass the next plane of data for an image.  See above for details. */
int gs_image_next(gs_image_enum * penum, const byte * dbytes,
		  uint dsize, uint * pused);

/* Clean up after processing an image. */
int gs_image_cleanup(gs_image_enum * penum);

/* Clean up after processing an image and free the enumerator. */
int gs_image_cleanup_and_free_enum(gs_image_enum * penum);

#endif /* gsimage_INCLUDED */
