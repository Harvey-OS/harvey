/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ifilter.h,v 1.6 2002/06/16 04:47:10 lpd Exp $ */
/* Interpreter filter support */
/* Requires oper.h, stream.h, strimpl.h */

#ifndef ifilter_INCLUDED
#  define ifilter_INCLUDED

#include "istream.h"
#include "ivmspace.h"

/*
 * Define the utility procedures for creating filters.
 * Note that a filter will be allocated in global VM iff the source/target
 * and all relevant parameters (if any) are in global VM.
 */
int filter_read(
	/* Operator arguments that were passed to zfxxx operator */
		i_ctx_t *i_ctx_p,
	/* # of parameters to pop off o-stack, */
	/* not counting the source/target and also not counting any */
	/* top dictionary operand (both of which will always be popped) */
		int npop,
	/* Template for stream */
		const stream_template * template,
	/* Initialized s_xxx_state, 0 if no separate state */
		stream_state * st,
	/* Max of space attributes of all parameters referenced by */
	/* the state, 0 if no such parameters */
		uint space
		);
int filter_write(i_ctx_t *i_ctx_p, int npop,
		 const stream_template * template,
		 stream_state * st, uint space);

/*
 * Define a simplified interface for streams with no parameters (except
 * an optional dictionary) or state.
 */
int filter_read_simple(i_ctx_t *i_ctx_p,
		       const stream_template * template);
int filter_write_simple(i_ctx_t *i_ctx_p,
			const stream_template * template);

/* Mark a filter stream as temporary. */
/* See stream.h for the meaning of is_temp. */
void filter_mark_temp(const ref * fop, int is_temp);

/* Mark the source or target of a filter as temporary, and propagate */
/* close_strm from the temporary stream to the filter. */
void filter_mark_strm_temp(const ref * fop, int is_temp);

/* Define a standard report_error procedure for filters, */
/* that records the error message in $error.errorinfo. */
stream_proc_report_error(filter_report_error);

/*
 * Define the state of a procedure-based stream.
 * Note that procedure-based streams are defined at the Ghostscript
 * interpreter level, unlike all other stream types which depend only
 * on the stream package and the memory manager.
 */
typedef struct stream_proc_state_s {
    stream_state_common;
    bool eof;
    uint index;			/* current index within data */
    ref proc;
    ref data;
} stream_proc_state;

#define private_st_stream_proc_state() /* in zfproc.c */\
  gs_private_st_complex_only(st_sproc_state, stream_proc_state,\
    "procedure stream state", sproc_clear_marks, sproc_enum_ptrs, sproc_reloc_ptrs, 0)

/* Test whether a stream is procedure-based. */
bool s_is_proc(const stream *s);

#endif /* ifilter_INCLUDED */
