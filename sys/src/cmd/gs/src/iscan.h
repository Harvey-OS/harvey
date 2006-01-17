/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iscan.h,v 1.9 2004/03/19 08:30:16 ray Exp $ */
/* Token scanner state and interface */
/* Requires gsstruct.h, ostack.h, stream.h */

#ifndef iscan_INCLUDED
#  define iscan_INCLUDED

#include "sa85x.h"
#include "sstring.h"

/*
 * Define the state of the scanner.  Before calling scan_token initially,
 * the caller must initialize the state by calling scanner_state_init.
 * Most of the state is only used if scanning is suspended because of
 * an interrupt or a callout.
 *
 * We expose the entire state definition to the caller so that
 * the state can normally be allocated on the stack.
 */
#ifndef scanner_state_DEFINED
#  define scanner_state_DEFINED
typedef struct scanner_state_s scanner_state;
#endif

/*
 * Define a structure for dynamically growable strings.
 * If is_dynamic is true, base/next/limit point to a string on the heap;
 * if is_dynamic is false, base/next/limit point either to the local buffer
 * or (only while control is inside scan_token) into the source stream buffer.
 */
#define max_comment_line 255	/* max size of an externally processable comment */
#define max_dsc_line max_comment_line	/* backward compatibility */
#define da_buf_size (max_comment_line + 2)
typedef struct dynamic_area_s {
    byte *base;
    byte *next;
    byte *limit;
    bool is_dynamic;
    byte buf[da_buf_size];	/* initial buffer */
    gs_memory_t *memory;
} dynamic_area;

#define da_size(pda) ((uint)((pda)->limit - (pda)->base))
typedef dynamic_area *da_ptr;

/* Define state specific to binary tokens and binary object sequences. */
typedef struct scan_binary_state_s {
    int num_format;
    int (*cont)(i_ctx_t *, stream *, ref *, scanner_state *);
    ref bin_array;
    uint index;
    uint max_array_index;	/* largest legal index in objects */
    uint min_string_index;	/* smallest legal index in strings */
    uint top_size;
    uint size;
} scan_binary_state;

/* Define the scanner state. */
struct scanner_state_s {
    uint s_pstack;		/* stack depth when starting current */
				/* procedure, after pushing old pstack */
    uint s_pdepth;		/* pstack for very first { encountered, */
				/* for error recovery */
    int s_options;
    enum {
	scanning_none,
	scanning_binary,
	scanning_comment,
	scanning_name,
	scanning_string
    } s_scan_type;
    dynamic_area s_da;
    union sss_ {		/* scanning sub-state */
	scan_binary_state binary;	/* binary */
	struct sns_ {		/* name */
	    int s_name_type;	/* number of /'s preceding a name */
	    bool s_try_number;	/* true if should try scanning name */
	    /* as number */
	} s_name;
	stream_state st;	/* string */
	stream_A85D_state a85d;	/* string */
	stream_AXD_state axd;	/* string */
	stream_PSSD_state pssd;	/* string */
    } s_ss;
};

/* The type descriptor is public only for checking. */
extern_st(st_scanner_state);
#define public_st_scanner_state()	/* in iscan.c */\
  gs_public_st_complex_only(st_scanner_state, scanner_state, "scanner state",\
    scanner_clear_marks, scanner_enum_ptrs, scanner_reloc_ptrs, 0)

/* Initialize a scanner with a given set of options. */
#define SCAN_FROM_STRING 1	/* true if string is source of data */
				/* (for Level 1 `\' handling) */
#define SCAN_CHECK_ONLY 2	/* true if just checking for syntax errors */
				/* and complete statements (no value) */
#define SCAN_PROCESS_COMMENTS 4	/* return scan_Comment for comments */
				/* (all comments or only non-DSC) */
#define SCAN_PROCESS_DSC_COMMENTS 8  /* return scan_DSC_Comment */
#define SCAN_PDF_RULES 16	/* PDF scanning rules (for names) */
#define SCAN_PDF_INV_NUM 32	/* Adobe ignores invalid numbers */
				/* This is for compatibility with Adobe */
				/* Acrobat Reader			*/
void scanner_state_init_options(scanner_state *sstate, int options);
#define scanner_state_init_check(pstate, from_string, check_only)\
  scanner_state_init_options(pstate,\
			     (from_string ? SCAN_FROM_STRING : 0) |\
			     (check_only ? SCAN_CHECK_ONLY : 0))
#define scanner_state_init(pstate, from_string)\
  scanner_state_init_check(pstate, from_string, false)

/*
 * Read a token from a stream.  As usual, 0 is a normal return,
 * <0 is an error.  There are also some special return codes:
 */
#define scan_BOS 1		/* binary object sequence */
#define scan_EOF 2		/* end of stream */
#define scan_Refill 3		/* get more input data, then call again */
#define scan_Comment 4		/* comment, non-DSC if processing DSC */
#define scan_DSC_Comment 5	/* DSC comment */
int scan_token(i_ctx_t *i_ctx_p, stream * s, ref * pref,
	       scanner_state * pstate);

/*
 * Read a token from a string.  Return like scan_token, but also
 * update the string to move past the token (if no error).
 */
int scan_string_token_options(i_ctx_t *i_ctx_p, ref * pstr, ref * pref,
			      int options);
#define scan_string_token(i_ctx_p, pstr, pref)\
  scan_string_token_options(i_ctx_p, pstr, pref, 0)

/*
 * Handle a scan_Refill return from scan_token.
 * This may return o_push_estack, 0 (meaning just call scan_token again),
 * or an error code.
 */
int scan_handle_refill(i_ctx_t *i_ctx_p, const ref * fop,
		       scanner_state * pstate, bool save, bool push_file,
		       op_proc_t cont);

/*
 * Define the procedure "hook" for parsing DSC comments.  If not NULL,
 * this procedure is called for every DSC comment seen by the scanner.
 */
extern int (*scan_dsc_proc) (const byte *, uint);

/*
 * Define the procedure "hook" for parsing general comments.  If not NULL,
 * this procedure is called for every comment seen by the scanner.
 * If both scan_dsc_proc and scan_comment_proc are set,
 * scan_comment_proc is called only for non-DSC comments.
 */
extern int (*scan_comment_proc) (const byte *, uint);

#endif /* iscan_INCLUDED */
