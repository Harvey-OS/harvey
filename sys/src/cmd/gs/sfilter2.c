/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* sfilter2.c */
/* Simple Level 2 filters */
#include "stdio_.h"	/* includes std.h */
#include "memory_.h"
#include "strimpl.h"
#include "sfilter.h"
#include "scanchar.h"

/* ------ ASCII85Encode ------ */

/* Process a buffer */
private int
s_A85E_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	int status = 0;
	int count;
	for ( ; (count = rlimit - p) >= 4; p += 4 )
	{	ulong word =
			((ulong)(((uint)p[1] << 8) + p[2]) << 16) +
			(((uint)p[3] << 8) + p[4]);
		if ( word == 0 )
		{	if ( wlimit - q < 2 )
			{	status = 1;
				break;
			}
			*++q = 'z';
		}
		else
		{	ulong v = word / (85L*85*85*85);  /* actually only a byte */
			ushort w1;
			if ( wlimit - q < 6 )
			{	status = 1;
				break;
			}
			q[1] = (byte)v + '!';
			word -= v * (85L*85*85*85);
			v = word / (85L*85*85);
			q[2] = (byte)v + '!';
			word -= v * (85L*85*85);
			v = word / (85*85);
			q[3] = (byte)v + '!';
			w1 = (ushort)(word - v * (85L*85));
			q[4] = (byte)(w1 / 85) + '!';
			q[5] = (byte)(w1 % 85) + '!';
			q += 5;
		}
		if ( !(count & 60) )
			*++q = '\n';
	}
	/* Check for final partial word. */
	if ( last && status == 0 && count < 4 )
	{	if ( wlimit - q < (count == 0 ? 2 : count + 3) )
			status = 1;
		else
		{	ulong word = 0;
			ulong divisor = 85L*85*85*85;
			switch ( count )
			{
			case 3:
				word += (uint)p[3] << 8;
			case 2:
				word += (ulong)p[2] << 16;
			case 1:
				word += (ulong)p[1] << 24;
				p += count;
				while ( count-- >= 0 )
				{	ulong v = word / divisor;  /* actually only a byte */
					*++q = (byte)v + '!';
					word -= v * divisor;
					divisor /= 85;
				}
			/*case 0:*/
			}
			*++q = '~';
			*++q = '>';
		}
	}
	pr->ptr = p;
	pw->ptr = q;
	return status;
}

/* Stream template */
const stream_template s_A85E_template =
{	&st_stream_state, NULL, s_A85E_process, 4, 6
};

/* ------ ASCII85Decode ------ */

private_st_A85D_state();

#define ss ((stream_A85D_state *)st)

/* Initialize the state */
private int
s_A85D_init(stream_state *st)
{	return s_A85D_init_inline(ss);
}

/* Process a buffer */
private int a85d_finish(P3(int, ulong, stream_cursor_write *));
private int
s_A85D_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	register const byte *p = pr->ptr;
	register byte *q = pw->ptr;
	const byte *rlimit = pr->limit;
	byte *wlimit = pw->limit;
	int ccount = ss->odd;
	ulong word = ss->word;
	int status = 0;
	while ( p < rlimit )
	{	int ch = *++p;
		uint ccode = ch - '!';
		if ( ccode < 85 )		/* catches ch < '!' as well */
		{	if ( wlimit - q < 4 )
			{	p--;
				status = 1;
				break;
			}
			word = word * 85 + ccode;
			if ( ++ccount == 5 )
			{	q[1] = word >> 24;
				q[2] = (byte)(word >> 16);
				q[3] = (byte)((uint)word >> 8);
				q[4] = (byte)word;
				q += 4;
				word = 0;
				ccount = 0;
			}
		}
		else if ( ch == 'z' && ccount == 0 )
		{	if ( wlimit - q < 4 )
			{	p--;
				status = 1;
				break;
			}
			q[1] = q[2] = q[3] = q[4] = 0,
			q += 4;
		}
		else if ( scan_char_decoder[ch] == ctype_space )
			;
		else if ( ch == '~' )
		{	/* Handle odd bytes. */
			if ( p == rlimit )
			{	if ( last )
				  status = ERRC;
				else
				  p--;
				break;
			}
			if ( (int)(wlimit - q) < ccount - 1 )
			{	status = 1;
				p--;
				break;
			}
			if ( *++p != '>' )
			{	status = ERRC;
				break;
			}
			pw->ptr = q;
			status = a85d_finish(ccount, word, pw);
			q = pw->ptr;
			break;
		}
		else			/* syntax error or exception */
		{	status = ERRC;
			break;
		}
	}
	pw->ptr = q;
	if ( status == 0 && last )
	{	if ( (int)(wlimit - q) < ccount - 1 )
		  status = 1;
		else
		  status = a85d_finish(ccount, word, pw);
	}
	pr->ptr = p;
	ss->odd = ccount;
	ss->word = word;
	return status;
}
/* Handle the end of input data. */
private int
a85d_finish(int ccount, ulong word, stream_cursor_write *pw)
{	/* Assume there is enough room in the output buffer! */
	byte *q = pw->ptr;
	int status = EOFC;
	switch ( ccount )
	{
	case 0:
		break;
	case 1:			/* syntax error */
		status = ERRC;
		break;
	case 2:			/* 1 odd byte */
		word = word * (85L*85*85) + 0xffffffL;
		goto o1;
	case 3:			/* 2 odd bytes */
		word = word * (85L*85) + 0xffffL;
		goto o2;
	case 4:			/* 3 odd bytes */
		word = word * 85 + 0xffL;
		q[3] = (byte)(word >> 8);
o2:		q[2] = (byte)(word >> 16);
o1:		q[1] = (byte)(word >> 24);
		q += ccount - 1;
		pw->ptr = q;
	}
	return status;
}

#undef ss

/* Stream template */
const stream_template s_A85D_template =
{	&st_A85D_state, s_A85D_init, s_A85D_process, 2, 4
};
