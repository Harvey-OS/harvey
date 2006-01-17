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

/* $Id: sbwbs.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Definitions for BWBlockSort (Burroughs-Wheeler) filters */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sbwbs_INCLUDED
#  define sbwbs_INCLUDED

/* Common framework for streams that buffer a block for processing */
#define stream_buffered_state_common\
	stream_state_common;\
		/* The client may set the following before initialization, */\
		/* or the stream may set it later. */\
	int BlockSize;\
		/* The init procedure sets the following, */\
		/* if BlockSize has been set. */\
	byte *buffer;		/* [BlockSize] */\
		/* The following are updated dynamically. */\
	bool filling;		/* true if filling buffer, */\
				/* false if emptying */\
	int bsize;		/* size of current block (<= BlockSize) */\
	int bpos		/* current index within buffer */
typedef struct stream_buffered_state_s {
    stream_buffered_state_common;
} stream_buffered_state;

#define private_st_buffered_state()	/* in sbwbs.c */\
  gs_private_st_ptrs1(st_buffered_state, stream_buffered_state,\
    "stream_buffered state", sbuf_enum_ptrs, sbuf_reloc_ptrs, buffer)

/* BWBlockSortEncode/Decode */
typedef struct of_ {
    uint v[256];
} offsets_full;
typedef struct stream_BWBS_state_s {
    stream_buffered_state_common;
    /* The init procedure sets the following. */
    void *offsets;		/* permutation indices when writing, */
    /* multi-level indices when reading */
    /* The following are updated dynamically. */
    int N;			/* actual length of block */
    /* The following are only used when decoding. */
    int I;			/* index of unrotated string */
    int i;			/* next index in encoded string */
} stream_BWBS_state;
typedef stream_BWBS_state stream_BWBSE_state;
typedef stream_BWBS_state stream_BWBSD_state;

#define private_st_BWBS_state()	/* in sbwbs.c */\
  gs_private_st_suffix_add1(st_BWBS_state, stream_BWBS_state,\
    "BWBlockSortEncode/Decode state", bwbs_enum_ptrs, bwbs_reloc_ptrs,\
    st_buffered_state, offsets)
extern const stream_template s_BWBSE_template;
extern const stream_template s_BWBSD_template;

#endif /* sbwbs_INCLUDED */
