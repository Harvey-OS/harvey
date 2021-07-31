/* Copyright (C) 1990, 1991, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* ibnum.c */
/* Level 2 encoded number reading utilities for Ghostscript */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "stream.h"
#include "ibnum.h"

/* Define the number of bytes for a given format of encoded number. */
const byte enc_num_bytes[] = { enc_num_bytes_values };

/* ------ Encoded number reading ------ */

/* Set up to read from an encoded number array/string. */
/* Return <0 for error, or a num_format. */
int
sread_num_array(stream *s, ref *op)
{	int num_format;
	switch ( r_type(op) )
	   {
	case t_string:
	   {	/* Check that this is a legitimate encoded number array. */
		byte *bp = op->value.bytes;
		short count;
		int nshift;
		if ( r_size(op) < 4 || bp[0] != bt_num_array_value ||
		    !num_is_valid(bp[1])
		    )
			return_error(e_typecheck);
		sread_string(s, bp + 4, r_size(op) - 4);
		num_format = bp[1];
		count = sdecodeshort(bp + 2, num_format);
		nshift = ((bp[1] & 0x70) == 0x20 ? 1 : 2);
		if ( count != (r_size(op) - 4) >> nshift )
			return_error(e_typecheck);
	   }	break;
	case t_array:
		sread_string(s, (byte *)op->value.refs, r_size(op) * sizeof(ref));
		num_format = num_array;
		break;
	default:
		return_error(e_typecheck);
	   }
	return num_format;
}

/* Get the number of elements in an encoded number stream. */
uint
scount_num_stream(stream *s, int format)
{	long avlong;
	savailable(s, &avlong);
	return (uint)(avlong / encoded_number_bytes(format));
}

/* Read an encoded number from a stream according to the given num_format. */
/* Put the value in np->value.{intval,realval}. */
/* Return t_int if integer, t_real if real, t_null if end of stream, */
/* or an error if the format is invalid. */
/* Note that the stream is always reading a string or an array, */
/* and we pre-checked the size, so sgets cannot return an error. */
int
sget_encoded_number(stream *s, int format, ref *np)
{	byte nstr[sizeof(ref)];		/* worst case */
	uint len;
	sgets(s, nstr, encoded_number_bytes(format), &len);
	if ( len == 0 )
	  return t_null;
	else
	  return sdecode_number(nstr, format, np);
}

/* Internal routine to decode a number in a given format. */
/* Same returns as sget_encoded_number. */
static const double binary_scale[32] = {
#define expn2(n) (0.5 / (1L << (n-1)))
	1.0, expn2(1), expn2(2), expn2(3),
	expn2(4), expn2(5), expn2(6), expn2(7),
	expn2(8), expn2(9), expn2(10), expn2(11),
	expn2(12), expn2(13), expn2(14), expn2(15),
	expn2(16), expn2(17), expn2(18), expn2(19),
	expn2(20), expn2(21), expn2(22), expn2(23),
	expn2(24), expn2(25), expn2(26), expn2(27),
	expn2(28), expn2(29), expn2(30), expn2(31)
#undef expn2
};
int
sdecode_number(const byte *str, int format, ref *np)
{	switch ( format & 0x170 )
	   {
	case num_int32: case num_int32 + 16:
		if ( (format & 31) == 0 )
		  {	np->value.intval = sdecodelong(str, format);
			return t_integer;
		  }
		else
		  {	np->value.realval =
			  (double)sdecodelong(str, format) *
			    binary_scale[format & 31];
			return t_real;
		  }
	case num_int16:
		if ( (format & 15) == 0 )
		  {	np->value.intval = sdecodeshort(str, format);
			return t_integer;
		  }
		else
		  {	np->value.realval =
			  sdecodeshort(str, format) *
			    binary_scale[format & 15];
			return t_real;
		  }
	case num_float:
		np->value.realval = sdecodefloat(str, format);
		return t_real;
	case num_array:
		memcpy(np, str, sizeof(ref));
		switch ( r_type(np) )
		   {
		case t_integer:
			return t_integer;
		case t_real:
			return t_real;
		default:
			return_error(e_typecheck);
		   }
	default:
		return_error(e_syntaxerror);	/* invalid num_format?? */
	   }
}

/* ------ Decode number ------ */

/* Decode encoded numbers from a string according to format. */

/* Decode a (16-bit, signed) short. */
short
sdecodeshort(register const byte *p, int format)
{	int a = p[0], b = p[1];
	short v = (num_is_lsb(format) ? (b << 8) + a : (a << 8) + b);
#if arch_sizeof_short == 2
	return v;
#else
	/* Sign-extend if sizeof(short) > 2. */
	return (v & 0x7fff) - (v & 0x8000);
#endif
}

/* Decode a (32-bit, signed) long. */
long
sdecodelong(register const byte *p, int format)
{	int a = p[0], b = p[1], c = p[2], d = p[3];
	long v = (num_is_lsb(format) ?
		  ((long)d << 24) + ((long)c << 16) + (b << 8) + a :
		  ((long)a << 24) + ((long)b << 16) + (c << 8) + d);
#if arch_sizeof_long == 4
	return v;
#else
	/* Sign-extend if sizeof(long) > 4. */
	return (v & 0x7fffffffL) - (v & 0x80000000L);
#endif
}

/* Decode a float.  We don't handle non-IEEE native representations yet. */
float
sdecodefloat(register const byte *p, int format)
{	float fnum;
	if ( format != num_float_native )
	{	bits32 lnum = (bits32)sdecodelong(p, format);
		/* We know IEEE floats take 32 bits. */
#if !arch_floats_are_IEEE
		/* Convert IEEE float to native float. */
		int sign_expt = lnum >> 32;
		int expt = sign_expt & 0xff;
		long mant = lnum & 0x7fffff;
		if ( expt == 0 && mant == 0 )
		  fnum = 0;
		else
		  {	mant += 0x800000;
			fnum = (float)ldexp((float)mant, expt - 127 - 24);
		  }
		if ( sign_expt >= 0x100 )
		  fnum = -fnum;
#else
		fnum = *(float *)&lnum;
#endif
	}
	else
		memcpy(&fnum, p, sizeof(float));
	return fnum;
}
