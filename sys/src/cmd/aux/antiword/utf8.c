/*
 * utf8.c
 * Copyright (C) 2001-2003 A.J. van Os; Released under GPL
 *
 *====================================================================
 * This part of the software is based on:
 * An implementation of wcwidth() as defined in
 * "The Single UNIX Specification, Version 2, The Open Group, 1997"
 * <http://www.UNIX-systems.org/online.html>
 * Markus Kuhn -- 2001-01-12 -- public domain
 *====================================================================
 * The credit should go to him, but all the bugs are mine.
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

struct interval {
	USHORT	first;
	USHORT	last;
};
/* Sorted list of non-overlapping intervals of non-spacing characters */
static const struct interval combining[] = {
	{ 0x0300, 0x034E }, { 0x0360, 0x0362 }, { 0x0483, 0x0486 },
	{ 0x0488, 0x0489 }, { 0x0591, 0x05A1 }, { 0x05A3, 0x05B9 },
	{ 0x05BB, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
	{ 0x05C4, 0x05C4 }, { 0x064B, 0x0655 }, { 0x0670, 0x0670 },
	{ 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
	{ 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
	{ 0x07A6, 0x07B0 }, { 0x0901, 0x0902 }, { 0x093C, 0x093C },
	{ 0x0941, 0x0948 }, { 0x094D, 0x094D }, { 0x0951, 0x0954 },
	{ 0x0962, 0x0963 }, { 0x0981, 0x0981 }, { 0x09BC, 0x09BC },
	{ 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD }, { 0x09E2, 0x09E3 },
	{ 0x0A02, 0x0A02 }, { 0x0A3C, 0x0A3C }, { 0x0A41, 0x0A42 },
	{ 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D }, { 0x0A70, 0x0A71 },
	{ 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC }, { 0x0AC1, 0x0AC5 },
	{ 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD }, { 0x0B01, 0x0B01 },
	{ 0x0B3C, 0x0B3C }, { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 },
	{ 0x0B4D, 0x0B4D }, { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 },
	{ 0x0BC0, 0x0BC0 }, { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 },
	{ 0x0C46, 0x0C48 }, { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 },
	{ 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
	{ 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D }, { 0x0DCA, 0x0DCA },
	{ 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 }, { 0x0E31, 0x0E31 },
	{ 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E }, { 0x0EB1, 0x0EB1 },
	{ 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC }, { 0x0EC8, 0x0ECD },
	{ 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 }, { 0x0F37, 0x0F37 },
	{ 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E }, { 0x0F80, 0x0F84 },
	{ 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 }, { 0x0F99, 0x0FBC },
	{ 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 }, { 0x1032, 0x1032 },
	{ 0x1036, 0x1037 }, { 0x1039, 0x1039 }, { 0x1058, 0x1059 },
	{ 0x1160, 0x11FF }, { 0x17B7, 0x17BD }, { 0x17C6, 0x17C6 },
	{ 0x17C9, 0x17D3 }, { 0x180B, 0x180E }, { 0x18A9, 0x18A9 },
	{ 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x206A, 0x206F },
	{ 0x20D0, 0x20E3 }, { 0x302A, 0x302F }, { 0x3099, 0x309A },
	{ 0xFB1E, 0xFB1E }, { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF },
	{ 0xFFF9, 0xFFFB }
};

/* Auxiliary function for binary search in interval table */
static BOOL
bIsZeroWidthChar(ULONG ucs)
{
	int low = 0;
	int high = elementsof(combining) - 1;
	int mid;

	if (ucs < (ULONG)combining[low].first ||
	    ucs > (ULONG)combining[high].last) {
		return FALSE;
	}

	while (high >= low) {
		mid = (low + high) / 2;
		if (ucs > (ULONG)combining[mid].last) {
			low = mid + 1;
		} else if (ucs < (ULONG)combining[mid].first) {
			high = mid - 1;
		} else {
			return TRUE;
		}
	}
	return FALSE;
} /* end of bIsZeroWidthChar */

/* The following functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      FullWidth (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that all characters are encoded
 * in ISO 10646.
 *
 * This function is not named wcwidth() to prevent name clashes
 */
static int
iWcWidth(ULONG ucs)
{
	/* Test for 8-bit control characters */
	if (ucs == 0) {
		return 0;
	}
	if (ucs < 0x20 || (ucs >= 0x7f && ucs < 0xa0)) {
		NO_DBG_HEX(ucs);
		return -1;
	}

	/* Binary search in table of non-spacing characters */
	if (bIsZeroWidthChar(ucs)) {
		return 0;
	}

	/* Ucs is not a combining or C0/C1 control character */

	return 1 +
	(ucs >= 0x1100 &&
	 (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
	  (ucs >= 0x2e80 && ucs <= 0xa4cf && (ucs & ~0x0011) != 0x300a &&
	   ucs != 0x303f) ||                  /* CJK ... Yi */
	  (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
	  (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
	  (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
	  (ucs >= 0xff00 && ucs <= 0xff5f) || /* Fullwidth Forms */
	  (ucs >= 0xffe0 && ucs <= 0xffe6) ||
	  (ucs >= 0x20000 && ucs <= 0x2ffff)));
} /* end of iWcWidth */

/*
 * utf8_to_ucs - convert from UTF-8 to UCS
 *
 * Returns the UCS character,
 * Fills in the number of bytes in the UTF-8 character
 */
static ULONG
utf8_to_ucs(const char *p, int *utflen)
{
	ULONG	wc;
	int	j, charlen;

	fail(p == NULL || utflen == NULL);

	wc = (ULONG)(UCHAR)p[0];

	if (wc < 0x80) {
		*utflen = 1;
		return wc;
	}

	if (wc < 0xe0){
		charlen = 2;
		wc &= 0x1f;
	} else if (wc < 0xf0){
		charlen = 3;
		wc &= 0x0f;
	} else if (wc < 0xf8){
		charlen = 4;
		wc &= 0x07;
	} else if (wc < 0xfc){
		charlen = 5;
		wc &= 0x03;
	} else {
		charlen = 6;
		wc &= 0x01;
	}
	for (j = 1; j < charlen; j++) {
		wc <<= 6;
		wc |= (UCHAR)p[j] & 0x3f;
	}
	*utflen = charlen;
	return wc;
} /* end of utf8_to_ucs */

/*
 * utf8_strwidth - compute the string width of an UTF-8 string
 *
 * Returns the string width in columns
 */
long
utf8_strwidth(const char *p, size_t numchars)
{
	const char	*maxp;
	ULONG	ucs;
	long	width, totwidth;
	int	utflen;

	fail(p == NULL);

	totwidth = 0;
	maxp = p + numchars;

	while (*p != '\0' && p < maxp) {
		ucs = utf8_to_ucs(p, &utflen);
		width = iWcWidth(ucs);
		if (width > 0) {
			totwidth += width;
		}
		p += utflen;
	}
	NO_DBG_DEC(totwidth);
	return totwidth;
} /* end of utf8_strwidth */

/*
 * utf8_chrlength - get the number of bytes in an UTF-8 character
 *
 * Returns the number of bytes
 */
int
utf8_chrlength(const char *p)
{
	int	utflen;

	fail(p == NULL);

	utflen = -1;		/* Just to make sure */
	(void)utf8_to_ucs(p, &utflen);
	NO_DBG_DEC(utflen);
	return utflen;
} /* end of utf8_chrlength */

/*
 * Original version:
 * Copyright (C) 1999  Bruno Haible
 */
BOOL
is_locale_utf8(void)
{
	const char	*locale, *cp, *encoding;

	/*
	 * Determine the current locale the same way as setlocale() does,
	 * according to POSIX.
	 */
	locale = getenv("LC_ALL");
	if (locale == NULL || locale[0] == '\0') {
		locale = getenv("LC_CTYPE");
		if (locale == NULL || locale[0] == '\0') {
			locale = getenv("LANG");
		}
	}

	if (locale == NULL || locale[0] == '\0') {
		return FALSE;
	}

	/* The most general syntax of a locale (not all optional parts
	 * recognized by all systems) is
	 * language[_territory][.codeset][@modifier][+special][,[sponsor][_revision]]
	 * To retrieve the codeset, search the first dot. Stop searching when
	 * a '@' or '+' or ',' is encountered.
	 */
	for (cp = locale;
	     *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ',';
	     cp++) {
		if (*cp != '.') {
			continue;
		}
		encoding = cp + 1;
		for (cp = encoding;
		     *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ',';
		     cp++)
			;	/* EMPTY */
		/*
		 * The encoding is now contained in the part from encoding to
		 * cp. Check it for "UTF-8", which is the only official IANA
		 * name of UTF-8. Also check for the lowercase-no-dashes
		 * version, which is what some SystemV systems use.
		 */
		if ((cp - encoding == 5 && STRNEQ(encoding, "UTF-8", 5)) ||
		    (cp - encoding == 4 && STRNEQ(encoding, "utf8", 4))) {
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
} /* end of is_locale_utf8 */
