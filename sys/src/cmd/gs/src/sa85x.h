/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: sa85x.h,v 1.3 2000/09/19 19:00:47 lpd Exp $ */
/* ASCII85 filter interface */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sa85x_INCLUDED
#  define sa85x_INCLUDED

#include "sa85d.h"

/* ASCII85Encode */
typedef struct stream_A85E_state_s {
    stream_state_common;
    /* The following change dynamically. */
    int count;			/* # of digits since last EOL */
    int last_char;		/* last character written */
} stream_A85E_state;

#define private_st_A85E_state()	/* in sfilter2.c */\
  gs_private_st_simple(st_A85E_state, stream_A85E_state,\
    "ASCII85Encode state")
#define s_A85E_init_inline(ss)\
  ((ss)->count = 0, (ss)->last_char = '\n')
extern const stream_template s_A85E_template;

#endif /* sa85x_INCLUDED */
