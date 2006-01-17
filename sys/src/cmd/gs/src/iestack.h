/* Copyright (C) 1989, 1992, 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iestack.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Generic execution stack API */

#ifndef iestack_INCLUDED
#  define iestack_INCLUDED

#include "iesdata.h"
#include "istack.h"

/* Define pointers into the execution stack. */
typedef s_ptr es_ptr;
typedef const_s_ptr const_es_ptr;

/* Manage the current_file cache. */
#define estack_clear_cache(pes) ((pes)->current_file = 0)
#define estack_set_cache(pes,pref) ((pes)->current_file = (pref))
#define estack_check_cache(pes)\
  BEGIN\
    if (r_has_type_attrs((pes)->stack.p, t_file, a_executable))\
      estack_set_cache(pes, (pes)->stack.p);\
  END

#endif /* iestack_INCLUDED */
