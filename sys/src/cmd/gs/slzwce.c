/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
  This file may be distributed with Aladdin Ghostscript, under the terms of
  the Aladdin Ghostscript Free Public License, and/or with GNU Ghostscript,
  under the terms of the Ghostscript General Public License.
  
  This file is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless
  he or she says so in writing.  */

/* slzwce.c */
/* Simple encoder compatible with LZW decoding filter */
#include "stdio_.h"	/* includes std.h */
#include "gdebug.h"
#include "strimpl.h"
#include "slzwx.h"

/*
 * This filter does not use the LZW compression algorithms,
 * but its output is compatible with LZW decoders.
 * Its output is approximately 9/8 times the size of its input.
 */

/* Define the special codes, relative to 1 << InitialCodeLength. */
#define code_reset 0
#define code_eod 1
#define code_0 2			/* first assignable code */

/* Internal routine to put a code into the output buffer. */
/* Let S = ss->code_size. */
/* Relevant invariants: 9 <= S <= 15, 0 <= code < 1 << S; */
/* 1 <= ss->bits_left <= 8; only the rightmost (8 - ss->bits_left) */
/* bits of ss->bits contain valid data. */
private byte *
lzw_put_code(register stream_LZW_state *ss, byte *q, uint code)
{	uint size = ss->code_size;
	byte cb = (ss->bits << ss->bits_left) +
		(code >> (size - ss->bits_left));
	if_debug2('W', "[w]writing 0x%x,%d\n", code, ss->code_size);
	*++q = cb;
	if ( (ss->bits_left += 8 - size) <= 0 )
	{	*++q = code >> -ss->bits_left;
		ss->bits_left += 8;
	}
	ss->bits = code;
	return q;
}

#define ss ((stream_LZW_state *)st)

/* Initialize LZW-compatible encoding filter. */
private int
s_LZWE_init(stream_state *st)
{	ss->InitialCodeLength = 8;
	ss->code_size = ss->InitialCodeLength + 1;
	ss->bits_left = 8;
	ss->table.encode = 0;
	ss->next_code = (1 << ss->InitialCodeLength) - 1;
	return 0;
}

/* Process a buffer */
private int
s_LZWE_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	const byte *rlimit = pr->limit;
	register byte *q = pw->ptr;
	byte *wlimit = pw->limit;
	int status = 0;
	int signal = 1 < (ss->code_size - 1);
	uint limit_code = (1 << ss->code_size) - 1;
	uint next_code = ss->next_code;
	while ( p < rlimit )
	  {	if ( next_code == limit_code )
		  {	/* Emit a reset code. */
			if ( wlimit - q < 2 )
			  {	status = 1;
				break;
			  }
			q = lzw_put_code(ss, q, signal + code_reset);
			next_code = code_0;
		  }
		if ( wlimit - q < 2 )
		  {	status = 1;
			break;
		  }
		q = lzw_put_code(ss, q, *++p);
		next_code++;
	  }
	if ( last && status == 0 )
	  {	if ( wlimit - q < 2 )
			status = 1;
		else
		  {	q = lzw_put_code(ss, q, signal + code_eod);
			if ( ss->bits_left < 8 )
			  *++q = ss->bits << ss->bits_left;  /* final byte */
		  }
	  }
out:	ss->next_code = next_code;
	pr->ptr = p;
	pw->ptr = q;
	return status;
}

#undef ss

/* Stream template */
const stream_template s_LZWE_template =
{	&st_LZW_state, s_LZWE_init, s_LZWE_process, 1, 2
};
