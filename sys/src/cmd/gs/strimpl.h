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

/* strimpl.h */
/* Definitions for stream implementors */
/* Requires stdio.h */

#ifndef strimpl_INCLUDED
#  define strimpl_INCLUDED

#include "scommon.h"
#include "gstypes.h"		/* for gsstruct.h */
#include "gsstruct.h"

/*
 * The 'process' procedure does the real work of the stream.
 * It must process as much input information (from pr->ptr + 1 through
 * pr->limit) as it can, subject to space available for output
 * (pw->ptr + 1 through pw->limit), updating pr->ptr and pw->ptr.
 *
 * The procedure return value must be one of:
 *	EOFC - an end-of-data pattern was detected in the input.
 *	ERRC - a syntactic error was detected in the input.
 *	0 - more input data is needed.
 *	1 - more output space is needed.
 * If the procedure returns EOFC, it can assume it will never be called
 * again for that stream.
 *
 * If the procedure is called with last = 1, this is an indication that
 * no more input will ever be supplied (after the input in the current
 * buffer defined by *pr); the procedure should produce as much output
 * as possible, including an end-of-data marker if applicable.
 * If the procedure is called with last = 1 and returns 1, it may be
 * called again (with last = 1); if it is called with last = 1 and returns
 * any other value, it will never be called again for that stream.
 * If the procedure is called with last = 1 and returns 0, this is taken
 * as equivalent to returning EOFC.
 *
 * Note that these specifications do not distinguish input from output
 * streams.  This is deliberate: The processing procedures should work
 * regardless of which way they are oriented in a stream pipeline.
 * (The PostScript language does take a position as whether any given
 * filter may be used for input or output, but this occurs at a higher level.)
 *
 * The value returned by the process procedure of a stream whose data source
 * or sink is external (i.e., not another stream) is interpreted slightly
 * differently.  For an external data source, a return value of 0 means
 * "no more input data are available now, but more might become available
 * later."  For an external data sink, a return value of 1 means "there is
 * no more room for output data now, but there might be room later."
 */

/*
 * Define a template for creating a stream.
 *
 * The meaning of min_in_size and min_out_size is the following:
 * If the amount of input information is at least min_in_size,
 * and the available output space is at least min_out_size,
 * the process procedure guarantees that it will make some progress.
 * (It may make progress even if this condition is not met, but this is
 * not guaranteed.)
 */
struct stream_template_s {

		/* Define the structure type for the stream state. */
	gs_memory_type_ptr_t stype;

		/* Define an optional initialization procedure. */
	stream_proc_init((*init));

		/* Define the processing procedure. */
		/* (The init procedure can reset other procs if it wants.) */
	stream_proc_process((*process));

		/* Define the minimum buffer sizes. */
	uint min_in_size;		/* minimum size for process input */
	uint min_out_size;		/* minimum size for process output */

		/* Define an optional releasing procedure. */
	stream_proc_release((*release));

};

/* Utility procedures */
int stream_move(P2(stream_cursor_read *, stream_cursor_write *)); /* in stream.c */
/* Hex decoding utility procedure */
typedef enum {
	hex_ignore_garbage = 0,
	hex_ignore_whitespace = 1,
	hex_ignore_leading_whitespace = 2
} hex_syntax;
int s_hex_process(P4(stream_cursor_read *, stream_cursor_write *, int *, hex_syntax)); /* in sstring.c */

#endif					/* strimpl_INCLUDED */
