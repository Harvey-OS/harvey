/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sfilter.h,v 1.8 2002/02/21 22:24:54 giles Exp $ */
/* Definitions for simple Ghostscript streams */
/* Requires scommon.h; should require strimpl.h only if any templates */
/* are referenced, but some compilers always require strimpl.h. */

#ifndef sfilter_INCLUDED
#  define sfilter_INCLUDED

#include "gstypes.h"		/* for gs_[const_]string */

/*
 * Define the processing states of the simplest Ghostscript streams.
 * We use abbreviations for the stream names so as not to exceed the
 * 31-character limit that some compilers put on identifiers.
 *
 * The processing state of a stream has three logical sections:
 * parameters set by the client before the stream is opened,
 * values computed from the parameters at initialization time,
 * and values that change dynamically.  Unless otherwise indicated,
 * all structure members change dynamically.
 */

/* eexecEncode */
typedef struct stream_exE_state_s {
    stream_state_common;
    /* The following parameters are set by the client. */
    ushort cstate;		/* encryption state */
} stream_exE_state;

#define private_st_exE_state()	/* in sfilter1.c */\
  gs_private_st_simple(st_exE_state, stream_exE_state, "eexecEncode state")
extern const stream_template s_exE_template;

/* eexecDecode */
typedef struct stream_PFBD_state_s stream_PFBD_state;
typedef struct stream_exD_state_s {
    stream_state_common;
    /* The following parameters are set by the client. */
    ushort cstate;		/* encryption state */
    int binary;			/* 1=binary, 0=hex, -1=don't know yet */
    int lenIV;			/* # of initial decoded bytes to skip */
    stream_PFBD_state *pfb_state;	/* state of underlying */
				/* PFBDecode stream, if any */
    /* The following change dynamically. */
    int odd;			/* odd digit */
    long record_left;		/* data left in binary record in .PFB file, */
				/* max_long if not reading a .PFB file */
    long hex_left;		/* # of encoded chars to process as hex */
				/* if binary == 0 */
    int skip;			/* # of decoded bytes to skip */
} stream_exD_state;

#define private_st_exD_state()	/* in seexec.c */\
  gs_private_st_ptrs1(st_exD_state, stream_exD_state, "eexecDecode state",\
    exd_enum_ptrs, exd_reloc_ptrs, pfb_state)
extern const stream_template s_exD_template;

/* PFBDecode */
/* The typedef for the state appears under eexecDecode above. */
/*typedef */ struct stream_PFBD_state_s {
    stream_state_common;
    /* The following parameters are set by the client. */
    int binary_to_hex;
    /* The following change dynamically. */
    int record_type;
    ulong record_left;		/* bytes left in current record */
} /*stream_PFBD_state */ ;

#define private_st_PFBD_state()	/* in sfilter1.c */\
  gs_private_st_simple(st_PFBD_state, stream_PFBD_state, "PFBDecode state")
extern const stream_template s_PFBD_template;

/* SubFileDecode */
typedef struct stream_SFD_state_s {
    stream_state_common;
    /* The following parameters are set by the client. */
    long count;			/* # of chars or EODs to scan over */
    gs_const_string eod;
    long skip_count;		/* # of initial chars or records to skip */
    /* The following change dynamically. */
    uint match;			/* # of matched chars not copied to output */
    uint copy_count;		/* # of matched characters left to copy */
    uint copy_ptr;		/* index of next character to copy */
} stream_SFD_state;
#define private_st_SFD_state()	/* in sfilter1.c */\
  gs_private_st_const_strings1(st_SFD_state, stream_SFD_state,\
    "SubFileDecode state", sfd_enum_ptrs, sfd_reloc_ptrs, eod)
extern const stream_template s_SFD_template;

#endif /* sfilter_INCLUDED */
