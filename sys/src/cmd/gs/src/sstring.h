/* Copyright (C) 1996, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sstring.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* are referenced, but some compilers always require strimpl.h. */

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
  ((ss)->odd = -1, 0)
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
/* We define the initialization procedure here, so that the scanner */
/* can avoid a procedure call. */
#define s_PSSD_init_inline(ss)\
  ((ss)->depth = 0)
extern const stream_template s_PSSD_template;

/* PSStringEncode */
/* (no state) */
extern const stream_template s_PSSE_template;

#endif /* sstring_INCLUDED */
