/* Copyright (C) 1994, 1995 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsjmorec.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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

/****** Comment out the next two lines to add progressive decoding. ******/
#undef D_MULTISCAN_FILES_SUPPORTED
#undef D_PROGRESSIVE_SUPPORTED

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
