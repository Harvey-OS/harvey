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

/*$Id: sbwbs.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
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
