/* Copyright (C) 2002 artofcode LLC.  All rights reserved.
  
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

/* $Id: gxdhtserial.h,v 1.2 2002/08/26 23:00:07 dan Exp $ */
/* Interface to [de-]serialization for (traditional) halftones */

#ifndef gxdhtserial_INCLUDED
#  define gxdhtserial_INCLUDED

#ifndef gs_memory_DEFINED
#  define gs_memory_DEFINED
typedef struct gs_memory_s  gs_memory_t;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s  gx_device;
#endif

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s    gs_imager_state;
#endif


/*
 * Serialize a halftone.
 *
 * Returns:
 *
 *    0, with *psize set the the amount of space required, if successful
 *
 *    gs_error_rangecheck, with *psize set to the size required, if the
 *        original *psize was not large enough
 *
 *    some other error code, with *psize unchange, in the event of an
 *        error other than lack of space
 */
extern  int     gx_ht_write( const gx_device_halftone * pdht,
                             const gx_device *          dev,
                             byte *                     data,
                             uint *                     psize );

/*
 * Reconstruct a halftone from its serial representation, and install it
 * as the current halftone. The buffer provided is expected to be large
 * enough to hold the entire halftone.
 *
 * The reading and installation phases are combined in this routine so as
 * to avoid unnecessarily allocating a device halftone and its component
 * array, just to release them immediately after installation is complete.
 * There is also not much reason to reconstuct a halftone except to make
 * it the current halftone.
 *
 * Returns the number of bytes read, or < 0 in the event of an error.
 */
extern  int     gx_ht_read_and_install( gs_imager_state *   pis,
                                        const gx_device *   dev,
                                        const byte *        data,
                                        uint                size,
                                        gs_memory_t *       mem );

#endif  /* gxdhtserail_INCLUDED */
