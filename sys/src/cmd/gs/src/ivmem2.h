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

/* $Id: ivmem2.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* VM control user parameter procedures */

#ifndef ivmem2_INCLUDED
#  define ivmem2_INCLUDED

/* Exported by zvmem2.c for zusparam.c */
int set_vm_reclaim(i_ctx_t *, long);
int set_vm_threshold(i_ctx_t *, long);

#endif /* ivmem2_INCLUDED */
