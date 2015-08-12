/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include <stdarg.h>
#include <string.h>
#include "utf.h"
#include "utfdef.h"

enum {
	Bit1 = 7,
	Bitx = 6,
	Bit2 = 5,
	Bit3 = 4,
	Bit4 = 3,
	Bit5 = 2,

	T1 = ((1 << (Bit1 + 1)) - 1) ^ 0xFF, /* 0000 0000 */
	Tx = ((1 << (Bitx + 1)) - 1) ^ 0xFF, /* 1000 0000 */
	T2 = ((1 << (Bit2 + 1)) - 1) ^ 0xFF, /* 1100 0000 */
	T3 = ((1 << (Bit3 + 1)) - 1) ^ 0xFF, /* 1110 0000 */
	T4 = ((1 << (Bit4 + 1)) - 1) ^ 0xFF, /* 1111 0000 */
	T5 = ((1 << (Bit5 + 1)) - 1) ^ 0xFF, /* 1111 1000 */

	Rune1 = (1 << (Bit1 + 0 * Bitx)) - 1, /* 0000 0000 0000 0000 0111 1111 */
	Rune2 = (1 << (Bit2 + 1 * Bitx)) - 1, /* 0000 0000 0000 0111 1111 1111 */
	Rune3 = (1 << (Bit3 + 2 * Bitx)) - 1, /* 0000 0000 1111 1111 1111 1111 */
	Rune4 = (1 << (Bit4 + 3 * Bitx)) - 1, /* 0001 1111 1111 1111 1111 1111 */

	Maskx = (1 << Bitx) - 1, /* 0011 1111 */
	Testx = Maskx ^ 0xFF,    /* 1100 0000 */

	SurrogateMin = 0xD800,
	SurrogateMax = 0xDFFF,

	Bad = Runeerror,
};

int
chartorune(Rune *rune, char *str)
{
	int c, c1, c2, c3;
	int32_t l;

	/*
	 * one character sequence
	 *	00000-0007F => T1
	 */
	c = *(uint8_t *)str;
	if(c < Tx) {
		*rune = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	00080-007FF => T2 Tx
	 */
	c1 = *(uint8_t *)(str + 1) ^ Tx;
	if(c1 & Testx)
		goto bad;
	if(c < T3) {
		if(c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if(l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	/*
	 * three character sequence
	 *	00800-0FFFF => T3 Tx Tx
	 */
	c2 = *(uint8_t *)(str + 2) ^ Tx;

	if(c2 & Testx)
		goto bad;
	if(c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(l <= Rune2)
			goto bad;
		if(SurrogateMin <= l && l <= SurrogateMax)
			goto bad;
		*rune = l;
		return 3;
	}

	/*
	 * four character sequence
	 *	10000-10FFFF => T4 Tx Tx Tx
	 */
	if(UTFmax >= 4) {
		c3 = *(uint8_t *)(str + 3) ^ Tx;
		if(c3 & Testx)
			goto bad;
		if(c < T5) {
			l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
			if(l <= Rune3)
				goto bad;
			if(l > Runemax)
				goto bad;
			*rune = l;
			return 4;
		}
	}

/*
	 * bad decoding
	 */
bad:
	*rune = Bad;
	return 1;
}

int
runetochar(char *str, Rune *rune)
{
	int32_t c;

	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	c = *rune;
	if(c <= Rune1) {
		str[0] = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if(c <= Rune2) {
		str[0] = T2 | (c >> 1 * Bitx);
		str[1] = Tx | (c & Maskx);
		return 2;
	}
	/*
	 * If the Rune is out of range or a surrogate half, convert it to the error rune.
	 * Do this test here because the error rune encodes to three bytes.
	 * Doing it earlier would duplicate work, since an out of range
	 * Rune wouldn't have fit in one or two bytes.
	 */
	if(c > Runemax)
		c = Runeerror;
	if(SurrogateMin <= c && c <= SurrogateMax)
		c = Runeerror;

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	if(c <= Rune3) {
		str[0] = T3 | (c >> 2 * Bitx);
		str[1] = Tx | ((c >> 1 * Bitx) & Maskx);
		str[2] = Tx | (c & Maskx);
		return 3;
	}

	/*
	 * four character sequence (21-bit value)
	 *     10000-1FFFFF => T4 Tx Tx Tx
	 */
	str[0] = T4 | (c >> 3 * Bitx);
	str[1] = Tx | ((c >> 2 * Bitx) & Maskx);
	str[2] = Tx | ((c >> 1 * Bitx) & Maskx);
	str[3] = Tx | (c & Maskx);
	return 4;
}

int
runelen(int32_t c)
{
	Rune rune;
	char str[10];

	rune = c;
	return runetochar(str, &rune);
}

int
runenlen(Rune *r, int nrune)
{
	int nb, c;

	nb = 0;
	while(nrune--) {
		c = *r++;
		if(c <= Rune1)
			nb++;
		else if(c <= Rune2)
			nb += 2;
		else if(c <= Rune3)
			nb += 3;
		else
			nb += 4;
	}
	return nb;
}

int
fullrune(char *str, int n)
{
	int c;
	if(n <= 0)
		return 0;
	c = *(uint8_t *)str;
	if(c < Tx)
		return 1;
	if(c < T3)
		return n >= 2;
	if(c < T4)
		return n >= 3;
	return n >= 4;
}
