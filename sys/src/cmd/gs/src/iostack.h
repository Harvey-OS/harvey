/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iostack.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Generic operand stack API */

#ifndef iostack_INCLUDED
#  define iostack_INCLUDED

#include "iosdata.h"
#include "istack.h"

/* Define pointers into the operand stack. */
typedef s_ptr os_ptr;
typedef const_s_ptr const_os_ptr;

#endif /* iostack_INCLUDED */
