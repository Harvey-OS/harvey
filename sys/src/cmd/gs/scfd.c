/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* scfd.c */
/* CCITTFax decoding filter */
#include "stdio_.h"	/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "strimpl.h"
#include "scf.h"
#include "scfx.h"

private_st_CFD_state();

#define ss ((stream_CFD_state *)st)

/* Initialize CCITTFaxDecode filter */
private int
s_CFD_init(stream_state *st)
{	int raster = ss->raster = (ss->Columns + 7) >> 3;
	byte white = (ss->BlackIs1 ? 0 : 0xff);
	s_hcd_init_inline(ss);
	ss->lprev = 0;			/* in case we bail out */
	ss->lbuf = gs_alloc_bytes(st->memory, raster + 1, "CFD lbuf");
	if ( ss->lbuf == 0 )
		return ERRC;		/****** WRONG ******/
	if ( ss->K != 0 )
	{	ss->lprev = gs_alloc_bytes(st->memory, raster + 1, "CFD lprev");
		if ( ss->lprev == 0 )
			return ERRC;		/****** WRONG ******/
		/* Clear the initial reference line for 2-D encoding. */
		memset(ss->lbuf, white, raster);
	}
	ss->k_left = min(ss->K, 0);
	ss->cbit = 0;
	ss->uncomp_run = 0;
	ss->rows_left = (ss->Rows <= 0 ? -1 : ss->Rows + 1);
	ss->rpos = ss->wpos = raster - 1;
	ss->eol_count = 0;
	ss->invert = white;
	return 0;
}

/* Release the filter. */
private void
s_CFD_release(stream_state *st)
{	gs_free_object(st->memory, ss->lprev, "CFD lprev(close)");
	gs_free_object(st->memory, ss->lbuf, "CFD lbuf(close)");
}

/* Declare the variables that hold the state. */
#define cfd_declare_state\
	hcd_declare_state;\
	register byte *q;\
	int qbit
/* Load the state from the stream. */
#define cfd_load_state()\
	hcd_load_state(),\
	q = ss->lbuf + ss->wpos, qbit = ss->cbit
/* Store the state back in the stream. */
#define cfd_store_state()\
	hcd_store_state(),\
	ss->wpos = q - ss->lbuf, ss->cbit = qbit

/* Macros to get blocks of bits from the input stream. */
/* Invariants: 0 <= bits_left <= bits_size; */
/* bits [bits_left-1..0] contain valid data. */

#define avail_bits(n) hcd_bits_available(n)
#define ensure_bits(n, outl) hcd_ensure_bits(n, outl)
#define peek_bits(n) hcd_peek_bits(n)
#define peek_var_bits(n) hcd_peek_var_bits(n)
#define skip_bits(n) hcd_skip_bits(n)

/* Get a run from the stream. */
#define get_run(decode, initial_bits, runlen, str, outl)\
{	const cfd_node *np;\
	int clen;\
	ensure_bits(initial_bits, outl);\
	np = &decode[peek_bits(initial_bits)];\
	if ( (clen = np->code_length) > initial_bits )\
	{	do_debug(uint init_bits = peek_bits(initial_bits));\
		if ( !avail_bits(clen) ) goto outl;\
		clen -= initial_bits;\
		skip_bits(initial_bits);\
		ensure_bits(clen, outl);		/* can't goto outl */\
		np = &decode[np->run_length + peek_var_bits(clen)];\
		if_debug4('W', "%s clen=%d xcode=0x%x rlen=%d\n", str,\
			  initial_bits + np->code_length,\
			  (init_bits << np->code_length) +\
				  peek_var_bits(np->code_length),\
			  np->run_length);\
		skip_bits(np->code_length);\
	}\
	else\
	{	if_debug4('W', "%s clen=%d code=0x%x rlen=%d\n", str,\
			  clen, peek_var_bits(clen), np->run_length);\
		skip_bits(clen);\
	}\
	runlen = np->run_length;\
}

/* Skip data bits for a white run. */
/* rlen is either less than 64, or a multiple of 64. */
#define skip_data(rlen, makeup_label)\
	if ( (qbit -= rlen) < 0 )\
	{	q -= qbit >> 3, qbit &= 7;\
		if ( rlen >= 64 ) goto makeup_label;\
	}

/* Invert data bits for a black run. */
/* If rlen >= 64, execute makeup_action: this is to handle */
/* makeup codes efficiently, since these are always a multiple of 64. */
#define invert_data(rlen, black_byte, makeup_action, d)\
	if ( rlen > qbit )\
	{	*q++ ^= (1 << qbit) - 1;\
		rlen -= qbit;\
		switch ( rlen >> 3 )\
		{\
		default:	/* original rlen >= 64 */\
d:			memset(q, black_byte, rlen >> 3);\
			q += rlen >> 3;\
			rlen &= 7;\
			if ( !rlen ) qbit = 0, q--;\
			else qbit = 8 - rlen, *q ^= 0xff << qbit;\
			makeup_action;\
			break;\
		case 7:		/* original rlen possibly >= 64 */\
			if ( rlen + qbit >= 64 ) goto d;\
			*q++ = black_byte;\
		case 6: *q++ = black_byte;\
		case 5: *q++ = black_byte;\
		case 4: *q++ = black_byte;\
		case 3: *q++ = black_byte;\
		case 2: *q++ = black_byte;\
		case 1: *q = black_byte;\
			rlen &= 7;\
			if ( !rlen ) { qbit = 0; break; }\
			q++;\
		case 0:			/* know rlen != 0 */\
			qbit = 8 - rlen;\
			*q ^= 0xff << qbit;\
		}\
	}\
	else\
		qbit -= rlen,\
		*q ^= ((1 << rlen) - 1) << qbit

/* Buffer refill for CCITTFaxDecode filter */
private int cf_decode_eol(P2(stream_CFD_state *, stream_cursor_read *));
private int cf_decode_1d(P2(stream_CFD_state *, stream_cursor_read *));
private int cf_decode_2d(P2(stream_CFD_state *, stream_cursor_read *));
private int cf_decode_uncompressed(P2(stream_CFD_state *, stream_cursor_read *));
private int
s_CFD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	int wstop = ss->raster - 1;
	int eol_count = ss->eol_count;
	int k_left = ss->k_left;
	int rows_left = ss->rows_left;
	int status = 0;
top:	/* Check for a completed scan line. */
	if ( ss->wpos == wstop && ss->cbit <= (-ss->Columns & 7) )
	{	/* Check for completed data to be copied to the client. */
		/* (We could avoid the extra copy step for 1-D, but */
		/* it's simpler not to, and it doesn't cost much.) */
		if ( ss->rpos < ss->wpos )
		{	stream_cursor_read cr;
			cr.ptr = ss->lbuf + ss->rpos;
			cr.limit = ss->lbuf + ss->wpos;
			status = stream_move(&cr, pw);
			ss->rpos = cr.ptr - ss->lbuf;
			if ( status )
				goto out;
		}
		if ( rows_left > 0 && --rows_left == 0 )
		{	status = EOFC;
			goto out;
		}
		if ( ss->K != 0 )
		{	byte *lprev = ss->lprev;
			ss->lprev = ss->lbuf;
			ss->lbuf = lprev;
		}
		ss->rpos = ss->wpos = -1;
		ss->eol_count = eol_count = 0;
		ss->cbit = 0;
		ss->invert = (ss->BlackIs1 ? 0 : 0xff);
		memset(ss->lbuf, ss->invert, wstop + 1);
		ss->run_color = 0;
		if ( ss->EncodedByteAlign )
			ss->bits_left &= ~7;
	}
	/* If we're between scan lines, scan for EOLs. */
	if ( ss->wpos < 0 )
	{	while ( (status = cf_decode_eol(ss, pr)) > 0 )
		{	if_debug0('w', "[w]EOL\n");
			/* If we are in a Group 3 mixed regime, */
			/* check the next bit for 1- vs. 2-D. */
			if ( ss->K > 0 )
			{	hcd_declare_state;
				hcd_load_state();
				ensure_bits(1, out);	/* can't fail */
				k_left = (peek_bits(1) ? 0 : 1);
				skip_bits(1);
				hcd_store_state();
			}
			++eol_count;
			if ( ss->EndOfBlock )
			{	/* Check for end-of-data sequence. */
				if (eol_count == (ss->K < 0 ? 2 : 6))
				{	status = EOFC;
					goto out;
				}
			}
			else
				break;		/* >1 EOL is an error */
		}
		if ( status == 0 )	/* input empty while scanning EOLs */
			goto out;
		switch ( eol_count )
		{
		case 0:
			if ( ss->EndOfLine )
			{	/* EOL is required, but none is present. */
				status = ERRC;
				goto out;
			}
		case 1:
			break;
		default:
			status = ERRC;
			goto out;
		}
	}
	/* Now decode actual data. */
	if ( k_left < 0 )
	{	if_debug0('w', "[w2]new row\n");
		status = cf_decode_2d(ss, pr);
	}
	else if ( k_left == 0 )
	{	if_debug0('w', "[w1]new row\n");
		status = cf_decode_1d(ss, pr);
		k_left = ss->K;
	}
	else
	{	if_debug1('w', "[w1]new 2-D row, %d left\n", k_left);
		status = cf_decode_2d(ss, pr);
		k_left--;
	}
	if_debug3('w', "[w]CFD status = %d, wpos = %d, cbit = %d\n",
		  status, ss->wpos, ss->cbit);
	if ( status == 1 )
		goto top;
out:	ss->k_left = k_left;
	ss->rows_left = rows_left;
	ss->eol_count = eol_count;
	return status;
}

#undef ss

/* Decode a leading EOL, if any. */
/* If an EOL is present, skip over it and return 1; */
/* if no EOL is present, return -1; if more input is needed, return 0. */
private int
cf_decode_eol(stream_CFD_state *ss, stream_cursor_read *pr)
{	hcd_declare_state;
	int zeros;
	int look_ahead;
	hcd_load_state();
	for ( zeros = 0; zeros < run_eol_code_length - 1; zeros++ )
	{	ensure_bits(1, out);
		if ( peek_bits(1) )
			return -1;
		skip_bits(1);
	}
	/* We definitely have an EOL.  Skip further zero bits. */
	look_ahead = (ss->K > 0 ? 2 : 1);
	for ( ; ; )
	{	ensure_bits(look_ahead, back);
		if ( peek_bits(1) )
			break;
		skip_bits(1);
	}
	skip_bits(1);
	hcd_store_state();
	return 1;
back:	/* We ran out of data while skipping zeros. */
	/* We know we are at a byte boundary, and have just skipped */
	/* at least run_eol_code_length - 1 zeros. */
	bits = 0;
	bits_left = run_eol_code_length - 1;
	hcd_store_state();
out:	return 0;
}

/* Decode a 1-D scan line. */
private int
cf_decode_1d(stream_CFD_state *ss, stream_cursor_read *pr)
{	cfd_declare_state;
	byte black_byte = (ss->BlackIs1 ? 0xff : 0);
	int end_bit = -ss->Columns & 7;
	byte *stop = ss->lbuf - 1 + ss->raster;
	int status;
	int bcnt;
	cfd_load_state();
	if ( ss->run_color )
		goto db;
	else
		goto dw;
#define q_at_stop() (q >= stop && (qbit <= end_bit || q > stop))
top:	if ( q_at_stop() )
		goto done;
dw:	/* Decode a white run. */
	get_run(cf_white_decode, cfd_white_initial_bits, bcnt, "[w1]white", out0);
	if ( bcnt < 0 )			/* exceptional situation */
	{	switch ( bcnt )
		{
		case run_uncompressed:	/* Uncompressed data. */
			cfd_store_state();
			bcnt = cf_decode_uncompressed(ss, pr);
			if ( bcnt < 0 )
				return bcnt;
			cfd_load_state();
			if ( bcnt ) goto db;
			else goto dw;
		/*case run_error:*/
		/*case run_zeros:*/	/* Premature end-of-line. */
		default:
			status = ERRC;
			goto out;
		}
	}
	skip_data(bcnt, dw);
	if ( q_at_stop() )
		goto done;
db:	/* Decode a black run. */
	get_run(cf_black_decode, cfd_black_initial_bits, bcnt, "[w1]black", out1);
	if ( bcnt < 0 )
	{	/* All exceptional codes are invalid here. */
		/****** WRONG, uncompressed IS ALLOWED ******/
		status = ERRC;
		goto out;
	}
	/* Invert bits designated by black run. */
	invert_data(bcnt, black_byte, goto db, idb);
	goto top;
done:	if ( q > stop || qbit < end_bit )
		status = ERRC;
	else
		status = 1;
out:	cfd_store_state();
	return status;
out0:	ss->run_color = 0;
	status = 0;
	goto out;
out1:	ss->run_color = 1;
	status = 0;
	goto out;
}

/* Decode a 2-D scan line. */
private int
cf_decode_2d(stream_CFD_state *ss, stream_cursor_read *pr)
{	cfd_declare_state;
	byte invert_white = (ss->BlackIs1 ? 0 : 0xff);
	byte black_byte = ~invert_white;
	byte invert = ss->invert;
	int end_count = -ss->Columns & 7;
	uint raster = ss->raster;
	byte *q0 = ss->lbuf;
	byte *prev_q01 = ss->lprev + 1;
	byte *endptr = q0 - 1 + raster;
	int init_count = raster << 3;
	register int count;
	int rlen;
	int status;
	cfd_load_state();
	count = ((endptr - q) << 3) + qbit;
	endptr[1] = 0xa0;	/* a byte with some 0s and some 1s, */
				/* to ensure run scan will stop */
	if_debug1('W', "[w2]raster=%d\n", raster);
	switch ( ss->run_color )
	{
	case -1: ss->run_color = 0; goto hbw;
	case 1: ss->run_color = 0; goto hwb;
	/*case 0:*/
	}
top:	if ( count <= end_count )
	{	status = (count < end_count ? ERRC : 1);
		goto out;
	}
	/* If invert == invert_white, white and black have their */
	/* correct meanings; if invert == ~invert_white, */
	/* black and white are interchanged. */
	if_debug1('W', "[w2]%4d:\n", count);
#ifdef DEBUG
	/* Check the invariant between q, qbit, and count. */
	{	int pcount = (endptr - q) * 8 + qbit;
		if ( pcount != count )
			dprintf2("[w2]Error: count=%d pcount=%d\n",
				 count, pcount);
	}
#endif
	/* We could just use get_run here, but we can do better: */
	ensure_bits(3, out0);
	switch( peek_bits(3) )
	{
	default/*4..7*/:			/* vertical(0) */
		skip_bits(1);
		rlen = countof(cf2_run_vertical) / 2;
		break;
	case 2:					/* vertical(+1) */
		skip_bits(3);
		rlen = countof(cf2_run_vertical) / 2 + 1;
		break;
	case 3:					/* vertical(-1) */
		skip_bits(3);
		rlen = countof(cf2_run_vertical) / 2 - 1;
		break;
	case 1:					/* horizontal */
		skip_bits(3);
		if ( invert == invert_white )
		{	/* White, then black. */
hww:			get_run(cf_white_decode, cfd_white_initial_bits,
				rlen, " white", hback);
			if ( (count -= rlen) < end_count )
			{	status = ERRC;
				goto out;
			}
			skip_data(rlen, hww);
			goto hwb;
		}
		else
		{	/* Black, then white. */
hbb:			get_run(cf_black_decode, cfd_black_initial_bits,
				rlen, " black", hback);
			if ( (count -= rlen) < end_count )
			{	status = ERRC;
				goto out;
			}
			invert_data(rlen, black_byte, goto hbb, ihbb);
			goto hbw;
		}
	case 0:				/* everything else */
		get_run(cf_2d_decode, cfd_2d_initial_bits, rlen,
			"[w2]", out0);
		/* rlen may be run2_pass, run_uncompressed, or */
		/* 0..countof(cf2_run_vertical)-1. */
		if ( rlen < 0 )
		  switch ( rlen )
		{
		case run2_pass:
			break;
		case run_uncompressed:
		{	int which;
			cfd_store_state();
			which = cf_decode_uncompressed(ss, pr);
			if ( which < 0 )
			{	status = which;
				goto out;
			}
			cfd_load_state();
			/****** ADJUST count ******/
			invert = (which ? ~invert_white : invert_white);
		}	goto top;
		default:	/* run_error, run_zeros */
			status = ERRC;
			goto out;
		}
	}
	/* Interpreting the run requires scanning the */
	/* previous ('reference') line. */
	{	int prev_count = count;
		byte prev_data;
		int dlen;
		static const byte count_bit[8] =
			{ 0x80, 1, 2, 4, 8, 0x10, 0x20, 0x40 };
		byte *prev_q = prev_q01 + (q - q0);
		if ( !(count & 7) )
			prev_q++;	/* because of skip macros */
		prev_data = prev_q[-1] ^ invert;
		/* Find the b1 transition. */
		if ( (prev_data & count_bit[prev_count & 7]) &&
		     (prev_count < init_count || invert != invert_white )
		   )
		{	/* Look for changing white first. */
			if_debug1('W', " data=0x%x", prev_data);
			skip_black_pixels(prev_data, prev_q,
					  prev_count, invert);
			if ( prev_count < end_count ) /* overshot */
				prev_count = end_count;
			if_debug1('W', " b1 other=%d", prev_count);
		}
		if ( prev_count != end_count )
		{	if_debug1('W', " data=0x%x", prev_data);
			skip_white_pixels(prev_data, prev_q,
					  prev_count, invert);
			if ( prev_count < end_count ) /* overshot */
				prev_count = end_count;
			if_debug1('W', " b1 same=%d", prev_count);
		}
		/* b1 = prev_count; */
		if ( rlen == run2_pass )
		{	/* Pass mode.  Find b2. */
			if ( prev_count != end_count )
			{	if_debug1('W', " data=0x%x", prev_data);
				skip_black_pixels(prev_data, prev_q,
						  prev_count, invert);
				if ( prev_count < end_count ) /* overshot */
					prev_count = end_count;
			}
			/* b2 = prev_count; */
			if_debug2('W', " b2=%d, pass %d\n",
				  prev_count, count - prev_count);
		}
		else
		{	/* Vertical coding. */
			/* Remember that count counts *down*. */
			prev_count += rlen - 3;	/* a1 */
			if_debug2('W', " vertical %d -> %d\n",
				  3 - rlen, prev_count);
		}
		/* Now either invert or skip from count */
		/* to prev_count, and reset count. */
		if ( invert == invert_white )
		{	/* Skip data bits. */
			q = endptr - (prev_count >> 3);
			qbit = prev_count & 7;
		}
		else
		{	/* Invert data bits. */
			dlen = count - prev_count;
			invert_data(dlen, black_byte, DO_NOTHING, idd);
		}
		count = prev_count;
		if ( rlen >= 0 )	/* vertical mode */
			invert = ~invert;	/* polarity changes */
	}
	goto top;
out:	cfd_store_state();
	ss->invert = invert;
	return status;
		/* Handle input exhausted after recognizing a horizontal */
		/* mode, but before parsing the black or white run. */
		/* The mode flag is 3 bits; the longest black/white */
		/* run code is 13 bits, so bits_left <= 12, and we can */
		/* back up by just incrementing bits_left. */
		/* To make this work even with makeup codes, */
		/* we force the previous 3 bits to 001 (horizontal mode). */
hback:	bits = (bits & ((1 << bits_left) - 1)) + (1 << bits_left);
	bits_left += 3;
out0:	status = 0;
	goto out;
		/* Handle input exhausted when parsing the second run */
		/* of a horizontal sequence.  We have to resume parsing */
		/* at hwb or hbw depending on the run color. */
outbw:	ss->run_color = -1;
	goto out0;
outwb:	ss->run_color = 1;
	goto out0;
		/* Handle the second half of a white-black horizontal code. */
		/* We put this here so we can branch to it when resuming. */
hwb:	get_run(cf_black_decode, cfd_black_initial_bits, rlen,
		" black", outwb);
	if ( (count -= rlen) < end_count )
	{	status = ERRC;
		goto out;
	}
	invert_data(rlen, black_byte, goto hwb, ihwb);
	goto top;
		/* Handle the second half of a black-white horizontal code. */
		/* We put this here so we can branch to it when resuming. */
hbw:	get_run(cf_white_decode, cfd_white_initial_bits, rlen,
		" white", outbw);
	if ( (count -= rlen) < end_count )
	{	status = ERRC;
		goto out;
	}
	skip_data(rlen, hbw);
	goto top;
}

#if 1	/****************/
private int
cf_decode_uncompressed(stream_CFD_state *ss, stream_cursor_read *pr)
{	return ERRC;
}
#else	/****************/

/* Decode uncompressed data. */
/* (Not tested: no sample data available!) */
/****** DOESN'T CHECK FOR OVERFLOWING SCAN LINE ******/
private int
cf_decode_uncompressed(stream *s)
{	cfd_declare_state;
	const cfd_node *np;
	int clen, rlen;
	cfd_load_state();
	while ( 1 )
	{	ensure_bits(cfd_uncompressed_initial_bits, NOOUT);
		np = &cf_uncompressed_decode[peek_bits(cfd_uncompressed_initial_bits)];
		clen = np->code_length;
		rlen = np->run_length;
		if ( clen > cfd_uncompressed_initial_bits )
		{	/* Must be an exit code. */
			break;
		}
		if ( rlen == cfd_uncompressed_initial_bits )
		{	/* Longest representable white run */
			if_debug1('W', "[wu]%d\n", rlen);
			if ( (qbit -= cfd_uncompressed_initial_bits) < 0 )
				qbit += 8, q++;
		}
		else
		{	if_debug1('W', "[wu]%d+1\n", rlen);
			if ( qbit -= rlen < 0 )
				qbit += 8, q++;
			*q ^= 1 << qbit;
		}
		skip_bits(clen);
	}
	clen -= cfd_uncompressed_initial_bits;
	skip_bits(cfd_uncompressed_initial_bits);
	ensure_bits(clen, NOOUT);
	np = &cf_uncompressed_decode[rlen + peek_var_bits(clen)];
	rlen = np->run_length;
	skip_bits(np->code_length);
	if_debug1('w', "[wu]exit %d\n", rlen);
	if ( rlen >= 0 )
	{	/* Valid exit code, rlen = 2 * run length + next polarity */
		if ( (qbit -= rlen >> 1) < 0 )
			qbit += 8, q++;
		rlen &= 1;
	}
out:		/******* WRONG ******/
	cfd_store_state();
	return rlen;
}

#endif	/****************/

/* Stream template */
const stream_template s_CFD_template =
{	&st_CFD_state, s_CFD_init, s_CFD_process, 1, 1, s_CFD_release
};
