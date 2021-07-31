/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* srlx.h */
/* Definitions for RLE/RLD streams */
/* Requires scommon.h; strimpl.h if any templates are referenced */

/* RunLengthEncode */
typedef struct stream_RLE_state_s {
	stream_state_common;
		/* The following parameters are set by the client. */
	ulong record_size;
		/* The following change dynamically. */
	ulong record_left;		/* bytes left in current record */
} stream_RLE_state;
#define private_st_RLE_state()	/* in sfilter2.c */\
  gs_private_st_simple(st_RLE_state, stream_RLE_state, "RunLengthEncode state")
/* We define the initialization procedure here, so that clients */
/* can avoid a procedure call. */
#define s_RLE_init_inline(ss)\
  ((ss)->record_left = ((ss)->record_size == 0 ?\
			((ss)->record_size = max_uint) :\
			((ss)->record_size)), 0)
extern const stream_template s_RLE_template;

/* RunLengthDecode */
/* (no state) */
typedef stream_state stream_RLD_state;
/* We define the initialization procedure here, so that clients */
/* can avoid a procedure call. */
#define s_RLD_init_inline(ss) DO_NOTHING
extern const stream_template s_RLD_template;
