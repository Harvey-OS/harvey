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

/* $Id: oparc.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Arc operator declarations */

#ifndef oparc_INCLUDED
#  define oparc_INCLUDED

/*
 * These declarations are in a separate from, rather than in opextern.h,
 * because these operators are not included in PDF-only configurations.
 */

int zarc(i_ctx_t *);
int zarcn(i_ctx_t *);
int zarct(i_ctx_t *);

#endif /* oparc_INCLUDED */
