/* Copyright (C) 2001 Artifex Software, Inc.  All rights reserved.
  
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

/*$Id: sarc4.h,v 1.2 2001/09/02 07:09:13 giles Exp $ */
/* Definitions for Arcfour cipher and filter */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sarc4_INCLUDED
#  define sarc4_INCLUDED

#include "scommon.h"

/* Arcfour is a symmetric cipher whose state is maintained
 * in two indices into an accompanying 8x8 S box. this will
 * typically be allocated on the stack, and so has no memory
 * management associated.
 */
typedef struct stream_arcfour_state_s
{
    stream_state_common;	/* a define from scommon.h */
    unsigned int x, y;
    unsigned char S[256];
}
stream_arcfour_state;

int s_arcfour_set_key(stream_arcfour_state * state, const unsigned char *key,
		      int keylength);

#define private_st_arcfour_state()	/* used in sarc4.c */\
  gs_private_st_simple(st_arcfour_state, stream_arcfour_state,\
    "Arcfour filter state")
extern const stream_template s_arcfour_template;

#endif /* sarc4_INCLUDED */
