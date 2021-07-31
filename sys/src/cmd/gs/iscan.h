/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* iscan.h */
/* Interface to Ghostscript scanner */
/* Requires gsstruct.h, ostack.h, stream.h, sfilter.h */

/*
 * Define the state of the scanner.  Before calling scan_token initially,
 * the caller must initialize the state by calling scanner_state_init.
 * Most of the state is only used if scanning is suspended because of
 * an interrupt or a callout.
 *
 * We expose the entire state definition to the caller so that
 * the state can normally be allocated on the stack.
 */
typedef struct scanner_state_s scanner_state;

/*
 * Define a structure for dynamically growable strings.
 * If is_dynamic is true, base/next/limit point to a string on the heap;
 * if is_dynamic is false, base/next/limit point either to the local buffer
 * or (only while control is inside scan_token) into the source stream buffer.
 */
#define max_dsc_line 255	/* max size of a DSC comment */
#define da_buf_size (max_dsc_line + 2)
typedef struct dynamic_area_s {
	gs_string str;			/* for GC */
	byte *base;
	byte *next;
	byte *limit;
	bool is_dynamic;
	byte buf[da_buf_size];		/* initial buffer */
	gs_memory_t *memory;
} dynamic_area;
#define da_size(pda) ((uint)((pda)->limit - (pda)->base))
typedef dynamic_area _ss *da_ptr;

/* Define state specific to binary tokens and binary object sequences. */
typedef struct scan_binary_state_s {
	int num_format;
	int (*cont)(P3(stream *, ref *, scanner_state *));
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
	bool s_from_string;	/* true if string is source of data */
				/* (for Level 1 `\' handling) */
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
		struct sns_ {			/* name */
		  int s_name_type;	/* number of /'s preceding a name */
		  bool s_try_number;	/* true if should try scanning name */
					/* as number */
		} s_name;
		stream_state st;		/* string */
		stream_A85D_state a85d;		/* string */
		stream_AXD_state axd;		/* string */
		stream_PSSD_state pssd;		/* string */
	} s_ss;
};
/* The type descriptor is public only for checking. */
extern_st(st_scanner_state);
#define public_st_scanner_state()	/* in iscan.c */\
  gs_public_st_complex_only(st_scanner_state, scanner_state, "scanner state",\
    scanner_clear_marks, scanner_enum_ptrs, scanner_reloc_ptrs, 0)
#define scanner_state_init(pstate, from_string)\
  ((pstate)->s_scan_type = scanning_none,\
   (pstate)->s_pstack = 0,\
   (pstate)->s_from_string = from_string)

/* Read a token from a stream. */
/* As usual, 0 is a normal return, <0 is an error. */
/* There are also three special return codes: */
#define scan_BOS 1		/* binary object sequence */
#define scan_EOF 2		/* end of stream */
#define scan_Refill 3		/* get more input data, then call again */
int scan_token(P3(stream *s, ref *pref, scanner_state *pstate));

/* Handle a scan_Refill return from scan_token. */
/* This may return o_push_estack, 0 (meaning just call scan_token again), */
/* or an error code. */
int scan_handle_refill(P4(const ref *fop, scanner_state *pstate, bool save,
  int (*cont)(P1(os_ptr))));

/* Define the procedure "hook" for parsing DSC comments.  If not NULL, */
/* this procedure is called for every DSC comment seen by the scanner. */
extern int (*scan_dsc_proc)(P2(const byte *, uint));
