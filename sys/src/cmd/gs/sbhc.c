/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* sbhc.c */
/* Bounded Huffman code filters */
#include "memory_.h"
#include "stdio_.h"
#include "gdebug.h"
#include "strimpl.h"
#include "sbhc.h"
#include "shcgen.h"

/* ------ BoundedHuffmanEncode ------ */

private_st_BHCE_state();

#define ss ((stream_BHCE_state *)st)

/* Initialize BoundedHuffmanEncode filter. */
private int
s_BHCE_init(register stream_state *st)
{	uint count = ss->encode.count = ss->definition.num_values;
	hce_code *encode = ss->encode.codes =
	  (hce_code *)gs_alloc_byte_array(st->memory, count,
					  sizeof(hce_code), "BHCE encode");
	if ( encode == 0 )
	  return ERRC;			/****** WRONG ******/
	hc_make_encoding(encode, &ss->definition);
	s_bhce_init_inline(ss);
	return 0;
}

/* Release the filter. */
private void
s_BHCE_release(stream_state *st)
{	gs_free_object(st->memory, ss->encode.codes, "BHCE encode");
}

/* Process a buffer. */
private int
s_BHCE_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	const byte *p = pr->ptr;
	const byte *rlimit = pr->limit;
	byte *q = pw->ptr;
	byte *wlimit = pw->limit - (hc_bits_size >> 3);
	const hce_code *encode = ss->encode.codes;
	uint num_values = ss->definition.num_values;
	uint zero_runs = ss->EncodeZeroRuns;
	uint zero_max = num_values - zero_runs + (ss->EndOfData ? 0 : 1);
	uint zero_value = (zero_max > 1 ? 0 : 0x100);
	int zeros = ss->zeros;
	int status = 0;

	while ( p < rlimit && q < wlimit )
	  {	uint value = *++p;
		const hce_code *cp;
		if ( value >= num_values )
		  {	status = ERRC;
			break;
		  }
		if ( value == zero_value )
		{	/* Accumulate a run of zeros. */
			++zeros;
			if ( zeros != zero_max )
			  continue;
			/* We've scanned the longest run we can encode. */
			cp = &encode[zeros - 2 + zero_runs];
			zeros = 0;
			hc_put_code((stream_hc_state *)ss, q, cp);
			continue;
		}
		/* Check whether we need to put out a zero run. */
		if ( zeros > 0 )
		{	--p;
			cp = (zeros == 1 ? &encode[0] :
			      &encode[zeros - 2 + zero_runs]);
			zeros = 0;
			hc_put_code((stream_hc_state *)ss, q, cp);
			continue;
		}
		cp = &encode[value];
		hc_put_code((stream_hc_state *)ss, q, cp);
	  }
	if ( q >= wlimit )
	  status = 1;
	wlimit = pw->limit;
	if ( last && status == 0 )
	  {	if ( zeros > 0 )
		  {	/* Put out a final run of zeros. */
			const hce_code *cp = (zeros == 1 ? &encode[0] :
					      &encode[zeros - 2 + zero_runs]);
			if ( !hce_bits_available(cp->code_length) )
			  status = 1;
			else
			  {	hc_put_code((stream_hc_state *)ss, q, cp);
				zeros = 0;
			  }
		  }
		if ( ss->EndOfData )
		  {	/* Put out the EOD code if we have room. */
			const hce_code *cp = &encode[num_values - 1];
			if ( !hce_bits_available(cp->code_length) )
			  status = 1;
			else
			  hc_put_code((stream_hc_state *)ss, q, cp);
		  }
		else
		  {	if ( q >= wlimit )
			  status = 1;
		  }
		if ( !status )
		  q = hc_put_last_bits((stream_hc_state *)ss, q);
	  }
	pr->ptr = p;
	pw->ptr = q;
	ss->zeros = zeros;
	return (p == rlimit ? 0 : 1);
}

#undef ss

/* Stream template */
const stream_template s_BHCE_template =
{	&st_BHCE_state, s_BHCE_init, s_BHCE_process,
	1, hc_bits_size >> 3, s_BHCE_release
};

/* ------ BoundedHuffmanDecode ------ */

private_st_BHCD_state();

#define ss ((stream_BHCD_state *)st)

#define hcd_initial_bits 7		/* arbitrary, >= 1 and <= 8 */

/* Initialize BoundedHuffmanDecode filter. */
private int
s_BHCD_init(register stream_state *st)
{	uint initial_bits = ss->decode.initial_bits =
	  min(hcd_initial_bits, ss->definition.num_counts);
	uint dsize = hc_sizeof_decoding(&ss->definition, initial_bits);
	hcd_code *decode = ss->decode.codes =
	  (hcd_code *)gs_alloc_byte_array(st->memory, dsize,
					  sizeof(hcd_code), "BHCD decode");
	if ( decode == 0 )
	  return ERRC;			/****** WRONG ******/
	hc_make_decoding(decode, &ss->definition, initial_bits);
	ss->decode.count = ss->definition.num_values;
	s_bhcd_init_inline(ss);
	return 0;
}

/* Release the filter. */
private void
s_BHCD_release(stream_state *st)
{	gs_free_object(st->memory, ss->decode.codes, "BHCD decode");
}

/* Process a buffer. */
private int
s_BHCD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	bhcd_declare_state;
	byte *q = pw->ptr;
	byte *wlimit = pw->limit;
	const hcd_code *decode = ss->decode.codes;
	uint initial_bits = ss->decode.initial_bits;
	uint zero_runs = ss->EncodeZeroRuns;
	int status = 0;
	int eod = (ss->EndOfData ? ss->definition.num_values - 1 : -1);

	bhcd_load_state();
z:	for ( ; zeros > 0; --zeros )
	{	if ( q >= wlimit )
		{	status = 1;
			goto out;
		}
		*++q = 0;
	}
	for ( ; ; )
	  {	const hcd_code *cp;
		int clen;
		hcd_ensure_bits(initial_bits, x1);
		cp = &decode[hcd_peek_var_bits(initial_bits)];
w1:		if ( q >= wlimit )
		  {	status = 1;
			break;
		  }
		if ( (clen = cp->code_length) > initial_bits )
		  {	if ( !hcd_bits_available(clen) )
			  {	/* We don't have enough bits for */
				/* all possible codes that begin this way, */
				/* but we might have enough for */
				/* the next code. */
				/****** NOT IMPLEMENTED YET ******/
			    break;
			  }
			clen -= initial_bits;
			hcd_skip_bits(initial_bits);
			hcd_ensure_bits(clen, out);	/* can't exit */
			cp = &decode[cp->value + hcd_peek_var_bits(clen)];
			hcd_skip_bits(cp->code_length);
		  }
		else
		  {	hcd_skip_bits(clen);
		  }
		if ( cp->value >= zero_runs )
		{	if ( cp->value == eod )
			  {	status = EOFC;
				p -= bits_left >> 3;	/* unread bytes beyond EOD */
				goto out;
			  }
			/* This code represents a run of zeros, */
			/* not a single output value. */
			zeros = cp->value - zero_runs + 2;
			goto z;
		}
		*++q = cp->value;
		continue;
			/* We don't have enough bits for all possible */
			/* codes, but we might have enough for */
			/* the next code. */
x1:		cp = &decode[(bits & ((1 << bits_left) - 1)) <<
			     (initial_bits - bits_left)];
		if ( (clen = cp->code_length) <= bits_left )
		  goto w1;
		break;
	  }
out:	bhcd_store_state();
	pw->ptr = q;
	return status;
}

#undef ss

/* Stream template */
const stream_template s_BHCD_template =
{	&st_BHCD_state, s_BHCD_init, s_BHCD_process, 1, 1, s_BHCD_release
};
