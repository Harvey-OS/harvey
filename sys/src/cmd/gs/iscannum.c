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

/* iscannum.c */
/* Number scanner for Ghostscript interpreter */
#include "ghost.h"
#include "errors.h"
#include "scommon.h"
#include "iscannum.h"			/* defines interface */
#include "scanchar.h"
#include "store.h"

#define is_digit(d, c)\
  ((d = decoder[c]) < 10)

#define scan_sign(sign, ptr)\
  switch ( *ptr ) {\
    case '-': sign = -1; ptr++; break;\
    case '+': sign = 1; ptr++; break;\
    default: sign = 0;\
  }

/* Scan a number for cvi or cvr. */
/* The first argument is a t_string.  This is just like scan_number, */
/* but allows leading or trailing whitespace. */
int
scan_number_only(const ref *psref, ref *pnref)
{	const byte *str = psref->value.const_bytes;
	const byte *end = str + r_size(psref);
	int sign, code;
	const byte *newstr;
	if ( !r_has_attr(psref, a_read) )
		return_error(e_invalidaccess);
	while ( str < end && scan_char_decoder[*str] == ctype_space )
		str++;
	while ( str < end && scan_char_decoder[end[-1]] == ctype_space )
		end--;
	if ( str == end )
		return_error(e_syntaxerror);
	scan_sign(sign, str);
	code = scan_number(str, end, sign, pnref, &newstr);
	if ( code <= 0 )
		return code;
	return_error(e_syntaxerror);
}

/* Note that the number scanning procedures use a byte ** and a byte * */
/* rather than a stream.  (It makes quite a difference in performance.) */
#define ngetc(cvar, sp, exit)\
  if ( sp >= end ) { exit; } else cvar = *sp++

/* Scan a number.  If the number consumes the entire string, return 0; */
/* if not, set *psp to the first character beyond the number and return 1. */
int
scan_number(register const byte *sp, const byte *end, int sign,
  ref *pref, const byte **psp)
{	/* Powers of 10 up to 6 can be represented accurately as */
	/* a single-precision float. */
#define num_powers_10 6
	static const float powers_10[num_powers_10+1] =
	{	1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6	};
	static const double neg_powers_10[num_powers_10+1] =
	{	1e0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6	};
	int ival;
	long lval;
	double dval;
	int exp10;
	int code = 0;
	register int c, d;
	register const byte _ds *decoder = scan_char_decoder;
	ngetc(c, sp, return_error(e_syntaxerror));
#define would_overflow(val, d, maxv)\
  (val >= maxv / 10 && (val > maxv / 10 || d > (int)(maxv % 10)))
	if ( !is_digit(d, c) )
	{	if ( c != '.' )
			return_error(e_syntaxerror);
		/* Might be a number starting with '.'. */
		ngetc(c, sp, return_error(e_syntaxerror));
		if ( !is_digit(d, c) )
			return_error(e_syntaxerror);
		ival = 0;
		goto i2r;
	}

	/* Accumulate an integer in ival. */
	/* Do up to 4 digits without a loop, */
	/* since we know this can't overflow and since */
	/* most numbers have 4 (integer) digits or fewer. */
	ival = d;
	if ( end - sp >= 3 )		/* just check once */
	{	if ( !is_digit(d, (c = *sp)) )
		{	sp++;
			goto ind;
		}
		ival = ival * 10 + d;
		if ( !is_digit(d, (c = sp[1])) )
		{	sp += 2;
			goto ind;
		}
		ival = ival * 10 + d;
		sp += 3;
		if ( !is_digit(d, (c = sp[-1])) )
			goto ind;
		ival = ival * 10 + d;
	}
	for ( ; ; ival = ival * 10 + d )
	{	ngetc(c, sp, goto iret);
		if ( !is_digit(d, c) )
			break;
		if ( would_overflow(ival, d, max_int) )
			goto i2l;
	}
ind:	switch ( c )
	{
	case '.':
		ngetc(c, sp, c = EOFC);
		goto i2r;
	default:
		*psp = sp;
		code = 1;
		break;
	case 'e': case 'E':
		if ( sign < 0 )
			ival = -ival;
		dval = ival;
		exp10 = 0;
		goto fe;
	case '#':
	{	ulong uval = 0, lmax;
#define radix (uint)ival
		if ( sign || radix < min_radix || radix > max_radix )
			return_error(e_syntaxerror);
		/* Avoid multiplies for power-of-2 radix. */
		if ( !(radix & (radix - 1)) )
		{	int shift;
			switch ( radix )
			{
#define set_shift(n)\
  shift = n; lmax = max_ulong >> n
			case 2: set_shift(1); break;
			case 4: set_shift(2); break;
			case 8: set_shift(3); break;
			case 16: set_shift(4); break;
			case 32: set_shift(5); break;
#undef set_shift
			default:		/* can't happen */
			  return_error(e_rangecheck);
			}
			for ( ; ; uval = (uval << shift) + d )
			{	ngetc(c, sp, break);
				d = decoder[c];
				if ( d >= radix )
				{	*psp = sp;
					code = 1;
					break;
				}
				if ( uval > lmax )
					return_error(e_limitcheck);
			}
		}
		else
		{	int lrem = max_ulong % radix;
			lmax = max_ulong / radix;
			for ( ; ; uval = uval * radix + d )
			{	ngetc(c, sp, break);
				d = decoder[c];
				if ( d >= radix )
				{	*psp = sp;
					code = 1;
					break;
				}
				if ( uval >= lmax &&
				     (uval > lmax || d > lrem)
				   )
					return_error(e_limitcheck);
			}
		}
#undef radix
		make_int_new(pref, uval);
		return code;
	}
	}
iret:	make_int_new(pref, (sign < 0 ? -ival : ival));
	return code;

	/* Accumulate a long in lval. */
i2l:	for ( lval = ival; ; )
	{	lval = lval * 10 + d;
		ngetc(c, sp, goto lret);
		if ( !is_digit(d, c) )
			break;
		if ( would_overflow(lval, d, max_long) )
		{	/* Make a special check for entering the smallest */
			/* (most negative) integer. */
			if ( lval == max_long / 10 &&
			     d == (int)(max_long % 10) + 1 && sign < 0
			   )
			{	ngetc(c, sp, c = EOFC);
				dval = -(double)min_long;
				if ( c == 'e' || c == 'E' || c == '.' )
				{	exp10 = 0;
					goto fs;
				}
				else if ( !is_digit(d, c) )
				{	lval = min_long;
					break;
				}
			}
			else
				dval = lval;
			goto l2d;
		}
	}
	switch ( c )
	{
	case '.':
		ngetc(c, sp, c = EOFC);
		exp10 = 0;
		goto l2r;
	default:
		*psp = sp;
		code = 1;
		break;
	case 'e': case 'E':
		exp10 = 0;
		goto le;
	case '#':
		return_error(e_syntaxerror);
	}
lret:	make_int_new(pref, (sign < 0 ? -lval : lval));
	return code;

	/* Accumulate a double in dval. */
l2d:	exp10 = 0;
	for ( ; ; )
	{	dval = dval * 10 + d;
		ngetc(c, sp, c = EOFC);
		if ( !is_digit(d, c) )
			goto fs;
	}

	/* We saw a '.' while accumulating an integer in ival. */
i2r:	exp10 = 0;
	while ( is_digit(d, c) )
	{	if ( would_overflow(ival, d, max_int) )
		{	lval = ival;
			goto l2r;
		}
		ival = ival * 10 + d;
		exp10--;
		ngetc(c, sp, c = EOFC);
	}
	if ( sign < 0 )
		ival = -ival;
	/* Take a shortcut for the common case */
	if ( !(c == 'e' || c == 'E' || exp10 < -num_powers_10) )
	{	/* Check for trailing garbage */
		if ( c != EOFC )
			*psp = sp, code = 1;
		make_real_new(pref, ival * neg_powers_10[-exp10]);
		return code;
	}
	dval = ival;
	goto fe;

	/* We saw a '.' while accumulating a long in lval. */
l2r:	while ( is_digit(d, c) )
	{	if ( would_overflow(lval, d, max_long) )
		{	dval = lval;
			goto fd;
		}
		lval = lval * 10 + d;
		exp10--;
		ngetc(c, sp, c = EOFC);
	}
le:	if ( sign < 0 )
		lval = -lval;
	dval = lval;
	goto fe;

	/* Now we are accumulating a double in dval. */
fd:	while ( is_digit(d, c) )
	{	dval = dval * 10 + d;
		exp10--;
		ngetc(c, sp, c = EOFC);
	}
fs:	if ( sign < 0 )
		dval = -dval;
fe:	/* Now dval contains the value, negated if necessary. */
	switch ( c )
	{
	case 'e': case 'E':
	{	/* Check for a following exponent. */
		int esign = 0;
		int iexp;
		ngetc(c, sp, return_error(e_syntaxerror));
		switch ( c )
		{
		case '-':
			esign = 1;
		case '+':
			ngetc(c, sp, return_error(e_syntaxerror));
		}
		/* Scan the exponent.  We limit it arbitrarily to 999. */
		if ( !is_digit(d, c) )
			return_error(e_syntaxerror);
		iexp = d;
		for ( ; ; iexp = iexp * 10 + d )
		{	ngetc(c, sp, break);
			if ( !is_digit(d, c) )
			{	*psp = sp;
				code = 1;
				break;
			}
			if ( iexp > 99 )
				return_error(e_limitcheck);
		}
		if ( esign )
			exp10 -= iexp;
		else
			exp10 += iexp;
		break;
	}
	default:
		*psp = sp;
		code = 1;
	case EOFC:
		;
	}
	/* Compute dval * 10^exp10. */
	if ( exp10 > 0 )
	{	while ( exp10 > num_powers_10 )
			dval *= powers_10[num_powers_10],
			exp10 -= num_powers_10;
		if ( exp10 > 0 )
			dval *= powers_10[exp10];
	}
	else if ( exp10 < 0 )
	{	while ( exp10 < -num_powers_10 )
			dval /= powers_10[num_powers_10],
			exp10 += num_powers_10;
		if ( exp10 < 0 )
			dval /= powers_10[-exp10];
	}
	make_real_new(pref, dval);
	return code;
}
