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

/* sfilter1.c */
/* Filters included in Level 1 systems: NullEncode/Decode, PFBDecode, */
/*   SubFileDecode, RunLengthEncode/Decode. */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "strimpl.h"
#include "sfilter.h"
#include "srlx.h"

/* ------ NullEncode/Decode ------ */

/* Process a buffer */
private int
s_Null_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	return stream_move(pr, pw);
}

/* Stream template */
const stream_template s_Null_template =
{	&st_stream_state, NULL, s_Null_process, 1, 1
};

/* ------ PFBDecode ------ */

private_st_PFBD_state();

#define ss ((stream_PFBD_state *)st)

/* Initialize the state */
private int
s_PFBD_init(stream_state *st)
{	ss->record_type = -1;
	return 0;
}

/* Process a buffer */
private int
s_PFBD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	int rcount, wcount;
	int c;
	int status = 0;
top:	rcount = pr->limit - p;
	wcount = pw->limit - q;
	switch ( ss->record_type )
	{
	case -1:			/* new record */
		if ( rcount < 2 )
			goto out;
		if ( p[1] != 0x80 )
			goto err;
		c = p[2];
		switch ( c )
		{
		case 1: case 2:
			break;
		case 3:
			status = EOFC;
			p += 2;
			goto out;
		default:
			p += 2;
			goto err;
		}
		if ( rcount < 6 )
			goto out;
		ss->record_type = c;
		ss->record_left = p[3] + ((uint)p[4] << 8) +
					((ulong)p[5] << 16) +
					((ulong)p[6] << 24);
		p += 6;
		goto top;
	case 1:				/* text data */
		/* Translate \r to \n. */
	{	int count = (wcount < rcount ? (status = 1, wcount) : rcount);
		if ( count > ss->record_left )
			count = ss->record_left,
			status = 0;
		ss->record_left -= count;
		for ( ; count != 0; count-- )
		{	c = *++p;
			*++q = (c == '\r' ? '\n' : c);
		}
	}	break;
	case 2:				/* binary data */
		if ( ss->binary_to_hex )
		{	/* Translate binary to hex. */
			int count;
			register const char _ds *hex_digits =
							"0123456789abcdef";
			wcount >>= 1;		/* 2 chars per input byte */
			count = (wcount < rcount ? (status = 1, wcount) : rcount);
			if ( count > ss->record_left )
				count = ss->record_left,
				status = 0;
			ss->record_left -= count;
			for ( ; count != 0; count-- )
			{	c = *++p;
				q[1] = hex_digits[c >> 4];
				q[2] = hex_digits[c & 0xf];
				q += 2;
			}
		}
		else
		{	/* Just read binary data. */
			int count = (wcount < rcount ? (status = 1, wcount) : rcount);
			if ( count > ss->record_left )
				count = ss->record_left,
				status = 0;
			ss->record_left -= count;
			memcpy(q + 1, p + 1, count);
			p += count;
			q += count;
		}
		break;
	   }
	if ( ss->record_left == 0 )
	{	ss->record_type = -1;
		goto top;
	}
out:	pr->ptr = p;
	pw->ptr = q;
	return status;
err:	pr->ptr = p;
	pw->ptr = q;
	return ERRC;
}

#undef ss

/* Stream template */
const stream_template s_PFBD_template =
{	&st_PFBD_state, s_PFBD_init, s_PFBD_process, 6, 2
};

/* ------ RunLengthEncode ------ */

private_st_RLE_state();

#define ss ((stream_RLE_state *)st)

/* Initialize */
private int
s_RLE_init(stream_state *st)
{	return s_RLE_init_inline(ss);
}

/* Process a buffer */
private int
s_RLE_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	int status = 0;
	ulong rleft = ss->record_left;
	while ( p < rlimit )
	{	const byte *beg = p;
		const byte *p1;
		uint count = rlimit - p;
		byte next;
		if ( count > rleft )
			count = rleft;
		if ( count > 127 )
			count = 127;
		p1 = p + count;
		if ( count > 2 && (next = p[1]) == p[2] && next == p[3] )
		{	if ( wlimit - q < 2 )
			{	status = 1;
				break;
			}
			/* Recognize leading repeated byte */
			p1--;
			do { p++; }
			while ( p < p1 && p[2] == next );
			p++;
			*++q = (byte)(257 - (p - beg));
			*++q = next;
		}
		else
		{	p1 -= 2;
			while ( p < p1 && (p[2] != p[1] || p[3] != p[1]) )
				p++;
			if ( p >= p1 )
				p = p1 + 2;
			count = p - beg;
			if ( wlimit - q < count + 1 )
			{	p = beg;
				status = 1;
				break;
			}
			*++q = count - 1;
			memcpy(q + 1, beg + 1, count);
			q += count;
		}
		rleft -= p - beg;
		if ( rleft == 0 )
			rleft = ss->record_size;
	}
	if ( last && status == 0 )
	  {	if ( q < wlimit )
		  *++q = 128;
		else
		  status = 1;
	  }
	pr->ptr = p;
	pw->ptr = q;
	ss->record_left = rleft;
	return status;
}

#undef ss

/* Stream template */
const stream_template s_RLE_template =
{	&st_RLE_state, s_RLE_init, s_RLE_process, 128, 129
};

/* ------ RunLengthDecode ------ */

/* Refill the buffer */
private int
s_RLD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	int status = 0;
	while ( p < rlimit )
	{	int b = *++p;
		if ( b < 128 )
		{	if ( b >= rlimit - p )
			{	p--;
				break;
			}
			else if ( b >= wlimit - q )
			{	p--;
				status = 1;
				break;
			}
			memcpy(q + 1, p + 1, ++b);
			p += b;
			q += b;
		}
		else if ( b == 128 )	/* end of data */
		{	status = EOFC;
			break;
		}
		else if ( p == rlimit )
		{	p--;
			break;
		}
		else if ( (b = 257 - b) > wlimit - q )
		{	p--;
			status = 1;
			break;		/* won't fit */
		}
		else
		{	memset(q + 1, *++p, b);
			q += b;
		}
	}
	pr->ptr = p;
	pw->ptr = q;
	return status;
}

/* Stream template */
const stream_template s_RLD_template =
{	&st_stream_state, NULL, s_RLD_process, 129, 128
};

/* ------ SubFileDecode ------ */

private_st_SFD_state();
/* GC procedures */
private ENUM_PTRS_BEGIN(sfd_enum_ptrs) return 0;
	ENUM_CONST_STRING_PTR(0, stream_SFD_state, eod);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(sfd_reloc_ptrs) ;
	RELOC_CONST_STRING_PTR(stream_SFD_state, eod);
RELOC_PTRS_END

#define ss ((stream_SFD_state *)st)

/* Initialize the stream */
private int
s_SFD_init(stream_state *st)
{	ss->match = 0;
	ss->copy_count = 0;
	return 0;
}

/* Refill the buffer */
private int
s_SFD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	int status = 0;
	if ( ss->eod.size == 0 )
	{	/* Just read, with no EOD pattern. */
		int rcount = rlimit - p;
		int wcount = wlimit - q;
		int count = min(rcount, wcount);
		if ( ss->count == 0 )		/* no EOD limit */
			return stream_move(pr, pw);
		else if ( ss->count > count )	/* not EOD yet */
		{	ss->count -= count;
			return stream_move(pr, pw);
		}
		else
		{	/* We're going to reach EOD. */
			count = ss->count;
			memcpy(q + 1, p + 1, count);
			pr->ptr = p + count;
			pw->ptr = q + count;
			return EOFC;
		}
	}
	else
	{	/* Read looking for an EOD pattern. */
		const byte *pattern = ss->eod.data;
		uint match = ss->match;
cp:		/* Check whether we're still copying a partial match. */
		if ( ss->copy_count )
		{	int count = min(wlimit - q, ss->count);
			memcpy(q + 1, ss->eod.data + ss->copy_ptr, count);
			ss->copy_count -= count;
			ss->copy_ptr += count;
			q += count;
			if ( ss->copy_count != 0 )	/* hit wlimit */
			{	status = 1;
				goto xit;
			}
			else if ( ss->count < 0 )
			{	status = EOFC;
				goto xit;
			}
		}
		while ( p < rlimit )
		{	int c = *++p;
			int end;
mat:			if ( c == pattern[match] )
			{	if ( ++match == ss->eod.size )
				{	switch ( ss->count )
					{
					case 0:
						status = EOFC;
						goto xit;
					case 1:
						ss->count = -1;
						break;
					default:
						ss->count--;
					}
					ss->copy_ptr = 0;
					ss->copy_count = match;
					match = 0;
					goto cp;
				}
				continue;
			}
			/* No match here, back up to find the longest one. */
			/* This may be quadratic in string_size, but */
			/* we don't expect this to be a real problem. */
			end = match;
			while ( match > 0 )
			{	match--;
				if ( !memcmp(pattern, pattern + end - match,
					     match)
				   )
					goto mat;
			}
			if ( q == wlimit )
			{	p--;
				status = 1;
				break;
			}
			*++q = c;
		}
xit:		pr->ptr = p;
		pw->ptr = q;
		ss->match = match;
	}
	return status;
}

#undef ss

/* Stream template */
const stream_template s_SFD_template =
{	&st_SFD_state, s_SFD_init, s_SFD_process, 1, 1
};
