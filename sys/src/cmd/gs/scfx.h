/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* scfx.h */
/* CCITTFax filter state definition */
/* Requires strimpl.h */
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
	int DamagedRowsBeforeError;\
	/*bool FirstBitLowOrder;*/	/* in stream_hc_state_common */\
		/* The init procedure sets the following. */\
	uint raster;\
	byte *lbuf;		/* current scan line buffer */\
				/* (only if decoding or 2-D encoding) */\
	byte *lprev;		/* previous scan line buffer (only if 2-D) */\
		/* The following are updated dynamically. */\
	int k_left;		/* number of next rows to encode in 2-D */\
				/* (only if K > 0) */\
	int run_color		/* 0 if processing white run, 1 if black */
typedef struct stream_CF_state_s {
	stream_CF_state_common;
} stream_CF_state;

/* CCITTFaxEncode */
typedef struct stream_CFE_state_s {
	stream_CF_state_common;
	int count;		/* # of source bits left to scan, */
				/* padded to a byte boundary */
	int run_count;		/* count at start of run begin scanned */
	int copy_count;		/* # of bytes to copy into lbuf */
	bool new_line;		/* false if processing a line, */
				/* true if need to start new line */
} stream_CFE_state;
#define private_st_CFE_state()	/* in scfe.c */\
  gs_private_st_ptrs2(st_CFE_state, stream_CFE_state, "CCITTFaxEncode state",\
    cfe_enum_ptrs, cfe_reloc_ptrs, lbuf, lprev)
extern const stream_template s_CFE_template;

/* CCITTFaxDecode */
typedef struct stream_CFD_state_s {
	stream_CF_state_common;
	int cbit;		/* bits left to fill in current decoded */
				/* byte at lbuf[wpos] (0..7) */
	int rows_left;		/* number of rows left */
	int rpos;		/* rptr for copying lbuf to client */
	int wpos;		/* rlimit/wptr for filling lbuf or */
				/* copying to client */
	int eol_count;		/* number of EOLs seen so far */
	byte invert;		/* current value of 'white' */
				/* for 2-D decoding */
	/* The following are not used yet. */
	int uncomp_run;		/* non-0 iff we are in an uncompressed */
				/* run straddling a scan line (-1 if white, */
				/* 1 if black) */
	int uncomp_left;	/* # of bits left in the run */
	int uncomp_exit;	/* non-0 iff this is an exit run */
				/* (-1 if next run white, 1 if black) */
} stream_CFD_state;
#define private_st_CFD_state()	/* in scfd.c */\
  gs_private_st_ptrs2(st_CFD_state, stream_CFD_state, "CCITTFaxDecode state",\
    cfd_enum_ptrs, cfd_reloc_ptrs, lbuf, lprev)
extern const stream_template s_CFD_template;
