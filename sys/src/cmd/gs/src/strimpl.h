/* Copyright (C) 1993, 1995, 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: strimpl.h,v 1.6 2002/06/16 05:00:54 lpd Exp $ */
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
 *      EOFC - an end-of-data pattern was detected in the input,
 *        or no more input can be processed for some other reason (e.g.,
 *        the stream was told only to read a certain amount of data).
 *      ERRC - a syntactic error was detected in the input.
 *      0 - more input data is needed.
 *      1 - more output space is needed.
 * If the procedure returns EOFC, it can assume it will never be called
 * again for that stream.
 *
 * If the procedure is called with last = 1, this is an indication that
 * no more input will ever be supplied (after the input in the current
 * buffer defined by *pr); the procedure should produce as much output
 * as possible, including an end-of-data marker if applicable.  In this
 * case:
 *      - If the procedure returns 1, it may be called again (also with
 *        last = 1).
 *      - If the procedure returns any other value other than 1, the
 *        procedure will never be called again for that stream.
 *      - If the procedure returns 0, this is taken as equivalent to
 *        returning EOFC.
 *      - If the procedure returns EOFC (or 0), the stream's end_status
 *        is set to EOFC, meaning no more writing is allowed.
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
 *
 * It appears that the Adobe specifications, read correctly, require that when
 * the process procedure of a decoding filter has filled up the output
 * buffer, it must still peek ahead in the input to determine whether or not
 * the next thing in the input stream is EOD.  If the next thing is an EOD (or
 * end-of-data, indicated by running out of input data with last = true), the
 * process procedure must return EOFC; if the next thing is definitely not
 * an EOD, the process procedure must return 1 (output full) (without, of
 * course, consuming the non-EOD datum); if the procedure cannot determine
 * whether or not the next thing is an EOD, it must return 0 (need more input).
 * Decoding filters that don't have EOD (for example, NullDecode) can use
 * a simpler algorithm: if the output buffer is full, then if there is more
 * input, return 1, otherwise return 0 (which is taken as EOFC if last
 * is true).  All this may seem a little awkward, but it is needed in order
 * to have consistent behavior regardless of where buffer boundaries fall --
 * in particular, if a buffer boundary falls just before an EOD.  It is
 * actually quite easy to implement if the main loop of the process
 * procedure tests for running out of input rather than for filling the
 * output: with this structure, exhausting the input always returns 0,
 * and discovering that the output buffer is full when attempting to store
 * more output always returns 1.
 *
 * Even this algorithm for handling end-of-buffer is not sufficient if an
 * EOD falls just after a buffer boundary, but the generic stream code
 * handles this case: the process procedures need only do what was just
 * described.
 */

/*
 * The set_defaults procedure in the template has a dual purpose: it sets
 * default values for all parameters that the client can set before calling
 * the init procedure, and it also must initialize all pointers in the
 * stream state to a value that will be valid for the garbage collector
 * (normally 0).  The latter implies that:
 *
 *	Any stream whose state includes additional pointers (beyond those
 *	in stream_state_common) must have a set_defaults procedure.
 */

/*
 * Note that all decoding filters that require an explicit EOD in the
 * source data must have an init procedure that sets min_left = 1.
 * This effectively provides a 1-byte lookahead in the source data,
 * which is required so that the stream can close itself "after reading
 * the last byte of data" (per Adobe specification), as noted above.
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

    /* Define an optional parameter defaulting and pointer initialization */
    /* procedure. */
    stream_proc_set_defaults((*set_defaults));

    /* Define an optional reinitialization procedure. */
    stream_proc_reinit((*reinit));

};

/* Utility procedures */
int stream_move(stream_cursor_read *, stream_cursor_write *);	/* in stream.c */

/* Hex decoding utility procedure */
typedef enum {
    hex_ignore_garbage = 0,
    hex_ignore_whitespace = 1,
    hex_ignore_leading_whitespace = 2
} hex_syntax;
int s_hex_process(stream_cursor_read *, stream_cursor_write *, int *, hex_syntax);	/* in sstring.c */

#endif /* strimpl_INCLUDED */
