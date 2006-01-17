/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: slzwe.c,v 1.4 2005/04/04 23:00:24 igor Exp $ */
/* LZW encoding filter */
#include "stdio_.h"	/* includes std.h */
#include "gdebug.h"
#include "strimpl.h"
#include "slzwx.h"

/********************************************************/
/* LZW routines are based on:				*/
/* Dr. Dobbs Journal --- Oct. 1989. 			*/
/* Article on LZW Data Compression by Mark R. Nelson 	*/
/********************************************************/

/* Define the special codes */
#define code_reset 256
#define code_eod 257
#define code_0 258			/* first assignable code */

/* Import the stream closing procedure */
extern stream_proc_release(s_LZW_release);

typedef struct lzw_encode_s {
	byte datum;			/* last byte of this code */
	ushort prefix;			/* code for prefix of this code */
} lzw_encode;

#define encode_max 4095		/* max # of codes, must be */
				/* > code_0 and <= 4095 */
#define hash_size (encode_max + encode_max / 4)

struct lzw_encode_table_s {
	lzw_encode encode[encode_max];
	ushort hashed[hash_size];
};
gs_private_st_simple(st_lzwe_table, lzw_encode_table, "lzw_encode_table");

/* Hashing function */
#define encode_hash(code, chr)\
  ((uint)((code) * 59 + (chr) * ((hash_size / 256) | 1)) % hash_size)

/* Internal routine to put a code into the output buffer. */
/* Let S = ss->code_size, M = ss->next_code, N = 1 << M. */
/* Relevant invariants: 9 <= S <= 12; N / 2 <= M < N; 0 <= code < N; */
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

/* Internal routine to reset the encoding table */
private void
lzw_reset_encode(stream_LZW_state *ss)
{	register int c;
	lzw_encode_table *table = ss->table.encode;
	ss->next_code = code_0;
	ss->code_size = 9;
	ss->prev_code = code_eod;
	for ( c = 0; c < hash_size; c++ )
		table->hashed[c] = code_eod;
	for ( c = 0; c < 256; c++ )
	{	lzw_encode *ec = &table->encode[c];
		register ushort *tc = &table->hashed[encode_hash(code_eod, c)];
		while ( *tc != code_eod )
		  if ( ++tc == &table->hashed[hash_size] )
		    tc = &table->hashed[0];
		*tc = c;
		ec->datum = c, ec->prefix = code_eod;
	}
	table->encode[code_eod].prefix = code_reset;	/* guarantee no match */
}

#define ss ((stream_LZW_state *)st)

/* Initialize LZWEncode filter */
private int
s_LZWE_init(stream_state *st)
{	ss->bits_left = 8;
	ss->table.encode = gs_alloc_struct(st->memory,
			lzw_encode_table, &st_lzwe_table, "LZWEncode init");
	if ( ss->table.encode == 0 )
		return ERRC;		/****** WRONG ******/
	ss->first = true;
	lzw_reset_encode(ss);
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
	int code = ss->prev_code;
	lzw_encode_table *table = ss->table.encode;
	ushort *table_end = &table->hashed[hash_size];
	int status = 0;
	int limit_code;
#define set_limit_code()\
  limit_code = (1 << ss->code_size) - ss->EarlyChange;\
  if ( limit_code > encode_max ) limit_code = encode_max
	set_limit_code();
	if ( ss->first )
	{	/* Emit the initial reset code. */
		if ( wlimit - q < 2 )
			return 1;
		q = lzw_put_code(ss, q, code_reset);
		ss->first = false;
	}
	while ( p < rlimit )
	{	byte c = p[1];
		ushort *tp;
		for ( tp = &table->hashed[encode_hash(code, c)]; ; )
		{	lzw_encode *ep = &table->encode[*tp];
			if ( ep->prefix == code && ep->datum == c )
			{	code = *tp;
				p++;
				break;
			}
			else if ( *tp != code_eod )
			{	if ( ++tp == table_end )
				  tp = &table->hashed[0]; /* wrap around */
			}
			else
			{	/* end of recognized sequence */
				if ( wlimit - q <= 4 )
				{	status = 1;
					goto out;
				}
				q = lzw_put_code(ss, q, code);
				if ( ss->next_code == limit_code )
				{	/* Reached power of 2 or limit. */
					/* Determine which. */
					if ( ss->next_code == encode_max )
					{	q = lzw_put_code(ss, q, code_reset);
						lzw_reset_encode(ss);
						set_limit_code();
						goto cx;
					}
					ss->code_size++;
					set_limit_code();
				}
				if_debug3('W', "[W]encoding 0x%x=0x%x+%c\n",
					  ss->next_code, code, c);
				*tp = ss->next_code++;
				ep = &table->encode[*tp];
				ep->datum = c;
				ep->prefix = code;
cx:				code = code_eod;
				break;
			}
		}
	}
	if ( last && status == 0 )
	{	if ( wlimit - q < 4 )
			status = 1;
		else
		  {	if ( code != code_eod )
			  {	q = lzw_put_code(ss, q, code);	/* put out final code */
			  }
			q = lzw_put_code(ss, q, code_eod);
			if ( ss->bits_left < 8 )
			  *++q = ss->bits << ss->bits_left;  /* final byte */
		  }
	}
out:	ss->prev_code = code;
	pr->ptr = p;
	pw->ptr = q;
	return status;
}

#undef ss

/* Stream template */
const stream_template s_LZWE_template =
{	&st_LZW_state, s_LZWE_init, s_LZWE_process, 1, 4, s_LZW_release,
	s_LZW_set_defaults
};
