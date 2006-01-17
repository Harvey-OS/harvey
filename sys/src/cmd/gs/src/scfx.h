/* Copyright (C) 1993, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: scfx.h,v 1.6 2005/06/03 20:03:54 ray Exp $ */
/* CCITTFax filter state definition */
/* Requires strimpl.h */

#ifndef scfx_INCLUDED
#  define scfx_INCLUDED

#include "shc.h"

/* Common state */
#define stream_CF_state_common\
	stream_hc_state_common;\
		/* The client sets the following before initialization. */\
	bool Uncompressed;\
	int K;\
	bool EndOfLine;\
	bool EncodedByteAlign;\
	int Columns;\
	int Rows;\
	bool EndOfBlock;\
	bool BlackIs1;\
	int DamagedRowsBeforeError;	/* (Decode only) */\
	/*bool FirstBitLowOrder;*/	/* in stream_hc_state_common */\
	int DecodedByteAlign;\
		/* The init procedure sets the following. */\
	uint raster;\
	byte *lbuf;		/* current scan line buffer */\
				/* (only if decoding or 2-D encoding) */\
	byte *lprev;		/* previous scan line buffer (only if 2-D) */\
		/* The following are updated dynamically. */\
	int k_left		/* number of next rows to encode in 2-D */\
				/* (only if K > 0) */
typedef struct stream_CF_state_s {
    stream_CF_state_common;
} stream_CF_state;

/* Define common default parameter setting. */
#define s_CF_set_defaults_inline(ss)\
  ((ss)->Uncompressed = false,\
   (ss)->K = 0,\
   (ss)->EndOfLine = false,\
   (ss)->EncodedByteAlign = false,\
   (ss)->Columns = 1728,\
   (ss)->Rows = 0,\
   (ss)->EndOfBlock = true,\
   (ss)->BlackIs1 = false,\
		/* Added by Adobe since the Red Book */\
   (ss)->DamagedRowsBeforeError = 0, /* always set, for s_CF_get_params */\
   (ss)->FirstBitLowOrder = false,\
		/* Added by us */\
   (ss)->DecodedByteAlign = 1,\
	/* Clear pointers */\
   (ss)->lbuf = 0, (ss)->lprev = 0)

/* CCITTFaxEncode */
typedef struct stream_CFE_state_s {
    stream_CF_state_common;
    /* The init procedure sets the following. */
    int max_code_bytes;		/* max # of bytes for an encoded line */
    byte *lcode;		/* buffer for encoded output line */
    /* The following change dynamically. */
    int read_count;		/* # of bytes to copy into lbuf */
    int write_count;		/* # of bytes to copy out of lcode */
    int code_bytes;		/* # of occupied bytes in lcode */
} stream_CFE_state;

#define private_st_CFE_state()	/* in scfe.c */\
  gs_private_st_ptrs3(st_CFE_state, stream_CFE_state, "CCITTFaxEncode state",\
    cfe_enum_ptrs, cfe_reloc_ptrs, lbuf, lprev, lcode)
#define s_CFE_set_defaults_inline(ss)\
  (s_CF_set_defaults_inline(ss), (ss)->lcode = 0)
extern const stream_template s_CFE_template;

/* CCITTFaxDecode */
typedef struct stream_CFD_state_s {
    stream_CF_state_common;
    int cbit;			/* bits left to fill in current decoded */
    /* byte at lbuf[wpos] (0..7) */
    int rows_left;		/* number of rows left */
    int row;			/* current row, first is 0 */
    int rpos;			/* rptr for copying lbuf to client */
    int wpos;			/* rlimit/wptr for filling lbuf or */
    /* copying to client */
    int eol_count;		/* number of EOLs seen so far */
    byte invert;		/* current value of 'white' */
    /* for 2-D decoding */
    int run_color;		/* -1 if processing white run, */
    /* 0 if between runs but white is next, */
    /* 1 if between runs and black is next, */
    /* 2 if processing black run */
    int damaged_rows;		/* # of consecutive damaged rows preceding */
    /* the current row */
    bool skipping_damage;	/* true if skipping a damaged row looking */
    /* for EOL */
    /* The following are not used yet. */
    int uncomp_run;		/* non-0 iff we are in an uncompressed */
    /* run straddling a scan line (-1 if white, */
    /* 1 if black) */
    int uncomp_left;		/* # of bits left in the run */
    int uncomp_exit;		/* non-0 iff this is an exit run */
    /* (-1 if next run white, 1 if black) */
} stream_CFD_state;

#define private_st_CFD_state()	/* in scfd.c */\
  gs_private_st_ptrs2(st_CFD_state, stream_CFD_state, "CCITTFaxDecode state",\
    cfd_enum_ptrs, cfd_reloc_ptrs, lbuf, lprev)
#define s_CFD_set_defaults_inline(ss)\
  s_CF_set_defaults_inline(ss)
extern const stream_template s_CFD_template;

#endif /* scfx_INCLUDED */
