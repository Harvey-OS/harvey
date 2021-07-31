/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscompt.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Abstract types for compositing objects */

#ifndef gscompt_INCLUDED
#  define gscompt_INCLUDED

/*
 * Compositing is the next-to-last step in the rendering pipeline.
 * It occurs after color correction but before halftoning (if needed).
 *
 * gs_composite_t is the abstract superclass for compositing functions such
 * as RasterOp functions or alpha-based compositing.  Concrete subclasses
 * must provide a default implementation (presumably based on
 * get_bits_rectangle and copy_color) for devices that provide no optimized
 * implementation of their own.
 *
 * A client that wants to produce composited output asks the target device
 * to create an appropriate compositing device based on the target device
 * and the gs_composite_t (and possibly other elements of the imager state).
 * If the target device doesn't have its own implementation for the
 * requested function, format, and state, it passes the buck to the
 * gs_composite_t, which may make further tests for special cases before
 * creating and returning a compositing device that uses the default
 * implementation.
 */
typedef struct gs_composite_s gs_composite_t;

/*
 * To enable fast cache lookup and equality testing, compositing functions,
 * like halftones, black generation functions, etc., carry a unique ID (time
 * stamp).
 */
gs_id gs_composite_id(P1(const gs_composite_t * pcte));

#endif /* gscompt_INCLUDED */
