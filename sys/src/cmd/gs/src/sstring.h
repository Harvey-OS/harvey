/* Copyright (C) 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sstring.h,v 1.7 2005/04/25 12:28:49 igor Exp $ */
/* String and hexstring streams (filters) */

#ifndef sstring_INCLUDED
#  define sstring_INCLUDED

/* ASCIIHexEncode */
typedef struct stream_AXE_state_s {
    stream_state_common;
    /* The following are set by the client. */
    bool EndOfData;		/* if true, write > at EOD (default) */
    /* The following change dynamically. */
    int count;			/* # of digits since last EOL */
} stream_AXE_state;

#define private_st_AXE_state()	/* in sstring.c */\
  gs_private_st_simple(st_AXE_state, stream_AXE_state,\
    "ASCIIHexEncode state")
#define s_AXE_init_inline(ss)\
  ((ss)->EndOfData = true, (ss)->count = 0)
extern const stream_template s_AXE_template;

/* ASCIIHexDecode */
typedef struct stream_AXD_state_s {
    stream_state_common;
    int odd;			/* odd digit */
} stream_AXD_state;

#define private_st_AXD_state()	/* in sstring.c */\
  gs_private_st_simple(st_AXD_state, stream_AXD_state,\
    "ASCIIHexDecode state")
#define s_AXD_init_inline(ss)\
  ((ss)->min_left = 1, (ss)->odd = -1, 0)
extern const stream_template s_AXD_template;

/* PSStringDecode */
typedef struct stream_PSSD_state_s {
    stream_state_common;
    /* The following are set by the client. */
    bool from_string;		/* true if using Level 1 \ convention */
    /* The following change dynamically. */
    int depth;
} stream_PSSD_state;

#define private_st_PSSD_state()	/* in sstring.c */\
  gs_private_st_simple(st_PSSD_state, stream_PSSD_state,\
    "PSStringDecode state")

/* Initialize the state */
int s_PSSD_init(stream_state * st);

/* A special initialization procedure for the scanner */
/* can avoid a procedure call. */
/* Note : it doesn't initialize ss->from_string. */
#define s_PSSD_partially_init_inline(ss)\
  ((ss)->depth = 0)
extern const stream_template s_PSSD_template;

/* PSStringEncode */
/* (no state) */
extern const stream_template s_PSSE_template;

#endif /* sstring_INCLUDED */
