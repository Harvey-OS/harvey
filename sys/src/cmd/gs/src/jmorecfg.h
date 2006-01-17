/* Copyright (C) 1994, 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsjmorec.h,v 1.5 2002/02/21 22:24:52 giles Exp $ */
/* "Wrapper" for Independent JPEG Group code jmorecfg.h */

#ifndef gsjmorec_INCLUDED
#  define gsjmorec_INCLUDED

#include "jmcorig.h"

/* Remove unwanted / unneeded features. */
#undef DCT_IFAST_SUPPORTED
#if FPU_TYPE <= 0
#  undef DCT_FLOAT_SUPPORTED
#endif
#undef C_MULTISCAN_FILES_SUPPORTED
#undef C_PROGRESSIVE_SUPPORTED
#undef ENTROPY_OPT_SUPPORTED
#undef INPUT_SMOOTHING_SUPPORTED


/* Progressive JPEG is required for PDF 1.3.
 * Don't undefine D_MULTISCAN_FILES_SUPPORTED and D_PROGRESSIVE_SUPPORTED
 */

#undef BLOCK_SMOOTHING_SUPPORTED
#undef IDCT_SCALING_SUPPORTED
#undef UPSAMPLE_SCALING_SUPPORTED
#undef UPSAMPLE_MERGING_SUPPORTED
#undef QUANT_1PASS_SUPPORTED
#undef QUANT_2PASS_SUPPORTED
/*
 * Read "JPEG" files with up to 64 blocks/MCU for Adobe compatibility.
 * Note that this #define will have no effect in pre-v6 IJG versions.
 */
#define D_MAX_BLOCKS_IN_MCU   64

#endif /* gsjmorec_INCLUDED */
