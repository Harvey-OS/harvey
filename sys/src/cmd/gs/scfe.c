/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* scfe.c */
/* CCITTFax encoding filter */
#include "stdio_.h"	/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "strimpl.h"
#include "scf.h"
#include "scfx.h"

/* ------ Macros and support routines ------ */

/* Put a run onto the output stream. */
/* Free variables: q, wlimit, status. */

#define cf_ensure_put_runs(n, color, out)\
	if ( wlimit - q < (n) * cfe_max_code_bytes )	/* worst case */\
	{	ss->run_color = color;\
		status = 1;\
		goto out;\
	}
#define cf_put_run(ss, lenv, tt, mut)\
{	const cfe_run *rp;\
	if ( lenv >= 64 )\
	{	while ( lenv >= 2560 + 64 )\
		  {	rp = &mut[40];\
			hc_put_code(ss, q, rp);\
			lenv -= 2560;\
		  }\
		rp = &mut[lenv >> 6];\
		hc_put_code(ss, q, rp);\
		lenv &= 63;\
	}\
	rp = &tt[lenv];\
	hc_put_code(ss, q, rp);\
}

#define cf_put_white_run(ss, lenv)\
  cf_put_run(ss, lenv, cf_white_termination, cf_white_make_up)

#define cf_put_black_run(ss, lenv)\
  cf_put_run(ss, lenv, cf_black_termination, cf_black_make_up)

/* ------ Stream procedures ------ */

private_st_CFE_state();
	  
#define ss ((stream_CFE_state *)st)

/*
 * For the 2-D encoding modes, we leave the previous complete scan line
 * at the beginning of the buffer, and start the new data after it.
 */

/* Initialize CCITTFaxEncode filter */
private int
s_CFE_init(register stream_state *st)
{	int columns = ss->Columns;
	int raster = ss->raster = (columns + 7) >> 3;
	s_hce_init_inline(ss);
	ss->count = raster << 3;	/* starting a scan line */
	ss->lbuf = ss->lprev = 0;
	if ( columns > cfe_max_width )
	  return ERRC;			/****** WRONG ******/
	if ( ss->K != 0 )
	{	ss->lbuf = gs_alloc_bytes(st->memory, raster + 1, "CFE lbuf");
		ss->lprev = gs_alloc_bytes(st->memory, raster + 1, "CFE lprev");
		if ( ss->lbuf == 0 || ss->lprev == 0 )
			return ERRC;		/****** WRONG ******/
		/* Clear the initial reference line for 2-D encoding. */
		/* Make sure it is terminated properly. */
		memset(ss->lprev, (ss->BlackIs1 ? 0 : 0xff), raster);
		if ( columns & 7 )
		  ss->lprev[raster - 1] ^= 0x80 >> (columns & 7);
		else
		  ss->lprev[raster] = ~ss->lprev[0];
		ss->copy_count = raster;
	}
	else
		ss->copy_count = 0;
	ss->new_line = true;
	ss->k_left = (ss->K > 0 ? 1 : ss->K);
	return 0;
}

/* Release the filter. */
private void
s_CFE_release(stream_state *st)
{	gs_free_object(st->memory, ss->lprev, "CFE lprev(close)");
	gs_free_object(st->memory, ss->lbuf, "CFE lbuf(close)");
}

/* Flush the buffer */
private int cf_encode_1d(P4(stream_CFE_state *, stream_cursor_read *,
  stream_cursor_write *, uint));
private int cf_encode_2d(P5(stream_CFE_state *, const byte *,
  stream_cursor_write *, uint, const byte *));
private int
s_CFE_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	stream_cursor_read lr;
	stream_cursor_read *plr = pr;
	int raster = ss->raster;
	int initial_count = raster << 3;
	int last_count = -ss->Columns & 7;
	byte last_mask = 1 << (-ss->Columns & 7);
	int status = 0;
	while ( pr->ptr < rlimit )
	{	int count = ss->count;
		int rcount = rlimit - pr->ptr;
		int row_left = (count + 7) >> 3;
		int end_count;
		byte *end;
		byte save_end, end_byte;
		byte end_mask;
		if_debug5('w', "[w]CFE: count = %d, pr = 0x%lx, 0x%lx, pw=0x%lx, 0x%lx\n",
			  count, (ulong)pr->ptr, (ulong)rlimit,
			  (ulong)pw->ptr, (ulong)wlimit);
		/* Check whether we are still accumulating a scan line */
		/* for 2-D encoding. */
		if ( ss->copy_count != 0 )
		{	int ccount = min(rcount, ss->copy_count);
			memcpy(ss->lbuf + raster - ss->copy_count,
			       pr->ptr + 1, ccount);
			pr->ptr += ccount;
			if ( (ss->copy_count -= ccount) != 0 )
				goto out;
		}
		if ( ss->K != 0 )
		{	/* Use lbuf as the source of data. */
			lr.limit = ss->lbuf + raster - 1;
			lr.ptr = lr.limit - row_left;
			plr = &lr;
			rcount = row_left;
		}
		if ( ss->new_line )
		{	/* Start a new scan line. */
			byte *q = pw->ptr;
			if ( wlimit - q < 4 + cfe_max_code_bytes * 2 )	/* byte align, aligned eol, run_horizontal + 2 runs */
			{	status = 1;
				break;
			}
			if ( ss->EndOfLine )
			{	const cfe_run *rp =
					(ss->k_left <= 0 ? &cf_run_eol :
					 ss->k_left > 1 ? &cf2_run_eol_2d :
					 &cf2_run_eol_1d);
				cfe_run run;
				if ( ss->EncodedByteAlign )
				  {	run = *rp;
					/* Pad the run on the left */
					/* so it winds up byte-aligned. */
					run.code_length +=
					 (ss->bits_left - run_eol_code_length) & 7;
					if ( run.code_length > 16 ) /* <= 23 */
					  ss->bits_left -= run.code_length & 7,
					  run.code_length = 16;
					rp = &run;
				  }
				hc_put_code(ss, q, rp);
				pw->ptr = q;
			}
			else if ( ss->EncodedByteAlign )
				ss->bits_left &= ~7;
			ss->run_count = initial_count;
			ss->run_color = 0;
			ss->new_line = false;
		}
		/* Ensure that the scan line ends with a polarity change. */
		/* This may involve saving and restoring one byte beyond */
		/* the scan line. */
		if ( rcount < row_left )	/* partial scan line */
			end = (byte *)plr->limit,
			end_mask = 1,
			end_count = (row_left - rcount) << 3;
		else
			end = (byte *)plr->ptr + row_left,
			end_mask = last_mask,
			end_count = last_count;
		if ( end_mask == 1 )		/* set following byte */
			save_end = *++end,
			end_byte = *end = (end[-1] & 1) - 1;
		else if ( (save_end = *end) & end_mask ) /* clear lower bits */
			end_byte = *end &= -end_mask;
		else				/* set lower bits */
			end_byte = *end |= end_mask - 1;
		if ( ss->K > 0 )
		{	/* Group 3, mixed encoding */
			if ( --(ss->k_left) )	/* Use 2-D encoding */
			{	status = cf_encode_2d(ss, ss->lbuf, pw, end_count, ss->lprev);
				if ( status )
				  {	/* We didn't finish encoding */
					/* the line, so back out. */
					ss->k_left++;
				  }
			}
			else	/* Use 1-D encoding */
			{	status = cf_encode_1d(ss, plr, pw, end_count);
				if ( status )
				  {	/* Didn't finish encoding the line, */
					/* back out. */
					ss->k_left++;
				  }
				else
				  ss->k_left = ss->K;
			}
		}
		else	/* Uniform encoding */
		{	status = (ss->K == 0 ?
				cf_encode_1d(ss, pr, pw, end_count) :
				cf_encode_2d(ss, ss->lbuf, pw, end_count, ss->lprev));
		}
		*end = save_end;
		if ( status )
			break;
		if ( ss->count == last_count )
		{	/* Finished a scan line, start a new one. */
			ss->count = initial_count;
			ss->new_line = true;
			if ( ss->K != 0 )
			{	byte *temp = ss->lbuf;
				ss->copy_count = raster;
				ss->lbuf = ss->lprev;
				ss->lprev = temp;
				*end = end_byte;	/* re-terminate */
			}
		}
	}
	/* Check for end of data. */
	if ( last && status == 0 )
	{	const cfe_run *rp =
			(ss->K > 0 ? &cf2_run_eol_1d : &cf_run_eol);
		int i = (!ss->EndOfBlock ? 0 : ss->K < 0 ? 2 : 6);
		uint bits_to_write = hc_bits_size - ss->bits_left +
			i * rp->code_length;
		byte *q = pw->ptr;
		if ( wlimit - q < (bits_to_write + 7) >> 3 )
		{	status = 1;
			goto out;
		}
		if ( ss->EncodedByteAlign )
			ss->bits_left &= ~7;
		while ( --i >= 0 )
			hc_put_code(ss, q, rp);
		/* Force out the last byte or bytes. */
		pw->ptr = q = hc_put_last_bits((stream_hc_state *)ss, q);
	}
out:	if_debug8('w', "[w]CFE exit %d: count = %d, run_count = %d, run_color = %d,\n     pr = 0x%lx, 0x%lx; pw = 0x%lx, 0x%lx\n",
		  status, ss->count, ss->run_count, ss->run_color,
		  (ulong)pr->ptr, (ulong)rlimit,
		  (ulong)pw->ptr, (ulong)wlimit);
#ifdef DEBUG
	if ( pr->ptr > rlimit || pw->ptr > wlimit )
	{	lprintf("Pointer overrun!\n");
		status = ERRC;
	}
#endif
	return status;
}

#undef ss

/* Encode a 1-D scan line. */
private int
cf_encode_1d(stream_CFE_state *ss, stream_cursor_read *pr,
  stream_cursor_write *pw, uint end_count)
{	register const byte *p = pr->ptr + 1;
	byte *q = pw->ptr;
	byte *wlimit = pw->limit;
	byte invert = (ss->BlackIs1 ? 0 : 0xff);
	register uint count = ss->count;
	/* Invariant: data = p[-1] ^ invert. */
	register uint data = *p++ ^ invert;
	int run = ss->run_count;
	int status = 0;
	if ( ss->run_color )
	{	cf_ensure_put_runs(1, 1, out);
		goto sb;
	}
sw:	/* Parse a white run. */
	cf_ensure_put_runs(2, 0, out);
	skip_white_pixels(data, p, count, invert);
	if ( count == end_count && count >= 8 )
	{	/* We only have a partial scan line. */
		ss->run_color = 0;
		goto out;
	}
	run -= count;
	cf_put_white_run(ss, run);
	run = count;
	if ( count == end_count )
	{	ss->run_color = 1;
		goto out;
	}
sb:	/* Parse a black run. */
	skip_black_pixels(data, p, count, invert);
	if ( count == end_count && count >= 8 )
	{	/* We only have a partial scan line. */
		ss->run_color = 1;
		goto out;
	}
	run -= count;
	cf_put_black_run(ss, run);
	run = count;
	if ( count > end_count )
		goto sw;
	ss->run_color = 0;
out:	pr->ptr = p - 2;
	pw->ptr = q;
	ss->count = count;
	ss->run_count = run;
	return status;
}

/* Encode a 2-D scan line. */
/* We know we have a full scan line of input, */
/* but we may run out of space to store the output. */
private int
cf_encode_2d(stream_CFE_state *ss, const byte *lbuf,
  stream_cursor_write *pw, uint end_count, const byte *lprev)
{	byte invert_white = (ss->BlackIs1 ? 0 : 0xff);
	byte invert = (ss->run_color ? ~invert_white : invert_white);
	register uint count = ss->count;
	const byte *p = lbuf + ss->raster - ((count + 7) >> 3);
	byte *q = pw->ptr;
	byte *wlimit = pw->limit;
	register uint data = *p++ ^ invert;
	int status = 0;
	while ( count != end_count )
	{	/* If invert == invert_white, white and black have their */
		/* correct meanings; if invert == ~invert_white, */
		/* black and white are interchanged. */
		uint a0 = count;
		uint a1, b1;
		int diff;
		uint prev_count = count;
		static const byte count_bit[8] =
			{ 0x80, 1, 2, 4, 8, 0x10, 0x20, 0x40 };
		const byte *prev_p = p - lbuf + lprev;
		byte prev_data = prev_p[-1] ^ invert;
		/* Make sure we have room for a run_horizontal plus */
		/* two data runs. */
		cf_ensure_put_runs(3, invert != invert_white, out);
		/* Find the a1 and b1 transitions. */
		skip_white_pixels(data, p, count, invert);
		a1 = count;
		if ( (prev_data & count_bit[prev_count & 7]) )
		{	/* Look for changing white first. */
			skip_black_pixels(prev_data, prev_p, prev_count, invert);
		}
		if ( prev_count != end_count )
		{	skip_white_pixels(prev_data, prev_p, prev_count, invert);
		}
		b1 = prev_count;
		/* In all the comparisons below, remember that count */
		/* runs downward, not upward, so the comparisons are */
		/* reversed. */
		if ( b1 >= a1 + 2 )
		{	/* Could be a pass mode.  Find b2. */
			if ( prev_count != end_count )
			{	skip_black_pixels(prev_data, prev_p,
						 prev_count, invert);
			}
			if ( prev_count > a1 )
			{	/* Use pass mode. */
				hc_put_code(ss, q, &cf2_run_pass);
				count = prev_count;
				p = prev_p - lprev + lbuf;
				data = p[-1] ^ invert;
				continue;
			}
		}
		/* Check for vertical coding. */
		diff = a1 - b1;		/* i.e., logical b1 - a1 */
		if ( diff <= 3 && diff >= -3 )
		{	/* Use vertical coding. */
			hc_put_code(ss, q, &cf2_run_vertical[diff + 3]);
			invert = ~invert;	/* a1 polarity changes */
			data ^= 0xff;
			continue;
		}
		/* No luck, use horizontal coding. */
		hc_put_code(ss, q, &cf2_run_horizontal);
		if ( count != end_count )
		{	skip_black_pixels(data, p, count, invert);	/* find a2 */
		}
		a0 -= a1;
		a1 -= count;
		if ( invert == invert_white )
		{	cf_put_white_run(ss, a0);
			cf_put_black_run(ss, a1);
		}
		else
		{	cf_put_black_run(ss, a0);
			cf_put_white_run(ss, a1);
		}
	}
out:	pw->ptr = q;
	ss->count = count;
	return status;
}

/* Stream template */
const stream_template s_CFE_template =
{	&st_CFE_state, s_CFE_init, s_CFE_process,
	2, 15, /* 31 left-over bits + 7 bits of padding + 6 13-bit EOLs */
	s_CFE_release
};
