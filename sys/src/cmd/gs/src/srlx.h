/* Copyright (C) 1994, 1995, 1996, 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: srlx.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Definitions for RunLength filters */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef srlx_INCLUDED
#  define srlx_INCLUDED

/* Common state */
#define stream_RL_state_common\
	stream_state_common;\
	bool EndOfData		/* true if 128 = EOD */

/* RunLengthEncode */
typedef struct stream_RLE_state_s {
    stream_RL_state_common;
    /* The following parameters are set by the client. */
    ulong record_size;
    /* The following change dynamically. */
    ulong record_left;		/* bytes left in current record */
    int copy_left;		/* # of bytes waiting to be copied */
} stream_RLE_state;

#define private_st_RLE_state()	/* in srle.c */\
  gs_private_st_simple(st_RLE_state, stream_RLE_state, "RunLengthEncode state")
/* We define the initialization procedure here, so that clients */
/* can avoid a procedure call. */
#define s_RLE_set_defaults_inline(ss)\
  ((ss)->EndOfData = true, (ss)->record_size = 0)
#define s_RLE_init_inline(ss)\
  ((ss)->record_left =\
   ((ss)->record_size == 0 ? ((ss)->record_size = max_uint) :\
    (ss)->record_size),\
   (ss)->copy_left = 0)
extern const stream_template s_RLE_template;

/* RunLengthDecode */
typedef struct stream_RLD_state_s {
    stream_RL_state_common;
    /* The following change dynamically. */
    int copy_left;		/* # of output bytes waiting to be produced */
    int copy_data;		/* -1 if copying, repeated byte if repeating */
} stream_RLD_state;

#define private_st_RLD_state()	/* in srld.c */\
  gs_private_st_simple(st_RLD_state, stream_RLD_state, "RunLengthDecode state")
/* We define the initialization procedure here, so that clients */
/* can avoid a procedure call. */
#define s_RLD_set_defaults_inline(ss)\
  ((ss)->EndOfData = true)
#define s_RLD_init_inline(ss)\
  ((ss)->copy_left = 0)
extern const stream_template s_RLD_template;

#endif /* srlx_INCLUDED */
