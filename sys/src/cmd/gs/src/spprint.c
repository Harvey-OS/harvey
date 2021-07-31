/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: spprint.c,v 1.2 2000/09/19 19:00:51 lpd Exp $ */
/* Print values in ASCII form on a stream */
#include "math_.h"		/* for fabs */
#include "stdio_.h"		/* for stream.h */
#include "string_.h"		/* for strchr */
#include "stream.h"
#include "spprint.h"

/* ------ Output ------ */

/* Put a byte array on a stream. */
int
pwrite(stream * s, const void *ptr, uint count)
{
    uint used;

    sputs(s, (const byte *)ptr, count, &used);
    return (int)used;
}

/* Put a string on a stream. */
int
pputs(stream * s, const char *str)
{
    uint len = strlen(str);
    uint used;
    int status = sputs(s, (const byte *)str, len, &used);

    return (status >= 0 && used == len ? 0 : EOF);
}

/* Print a format string up to the first variable substitution. */
/* Return a pointer to the %, or to the terminating 0 if no % found. */
private const char *
pprintf_scan(stream * s, const char *format)
{
    const char *fp = format;

    for (; *fp != 0; ++fp) {
	if (*fp == '%') {
	    if (fp[1] != '%')
		break;
	    ++fp;
	}
	sputc(s, *fp);
    }
    return fp;
}

/* Print (an) int value(s) using a format. */
const char *
pprintd1(stream * s, const char *format, int v)
{
    const char *fp = pprintf_scan(s, format);
    char str[25];

#ifdef DEBUG
    if (*fp == 0 || fp[1] != 'd')	/* shouldn't happen! */
	lprintf1("Bad format in pprintd1: %s\n", format);
#endif
    sprintf(str, "%d", v);
    pputs(s, str);
    return pprintf_scan(s, fp + 2);
}
const char *
pprintd2(stream * s, const char *format, int v1, int v2)
{
    return pprintd1(s, pprintd1(s, format, v1), v2);
}
const char *
pprintd3(stream * s, const char *format, int v1, int v2, int v3)
{
    return pprintd2(s, pprintd1(s, format, v1), v2, v3);
}
const char *
pprintd4(stream * s, const char *format, int v1, int v2, int v3, int v4)
{
    return pprintd2(s, pprintd2(s, format, v1, v2), v3, v4);
}

/* Print (a) floating point number(s) using a format. */
/* See gdevpdfx.h for why this is needed. */
const char *
pprintg1(stream * s, const char *format, floatp v)
{
    const char *fp = pprintf_scan(s, format);
    char str[50];

#ifdef DEBUG
    if (*fp == 0 || fp[1] != 'g')	/* shouldn't happen! */
	lprintf1("Bad format in pprintg: %s\n", format);
#endif
    sprintf(str, "%g", v);
    if (strchr(str, 'e')) {
	/* Bad news.  Try again using f-format. */
	sprintf(str, (fabs(v) > 1 ? "%1.1f" : "%1.8f"), v);
    }
    pputs(s, str);
    return pprintf_scan(s, fp + 2);
}
const char *
pprintg2(stream * s, const char *format, floatp v1, floatp v2)
{
    return pprintg1(s, pprintg1(s, format, v1), v2);
}
const char *
pprintg3(stream * s, const char *format, floatp v1, floatp v2, floatp v3)
{
    return pprintg2(s, pprintg1(s, format, v1), v2, v3);
}
const char *
pprintg4(stream * s, const char *format, floatp v1, floatp v2, floatp v3,
	 floatp v4)
{
    return pprintg2(s, pprintg2(s, format, v1, v2), v3, v4);
}
const char *
pprintg6(stream * s, const char *format, floatp v1, floatp v2, floatp v3,
	 floatp v4, floatp v5, floatp v6)
{
    return pprintg3(s, pprintg3(s, format, v1, v2, v3), v4, v5, v6);
}

/* Print a long value using a format. */
const char *
pprintld1(stream * s, const char *format, long v)
{
    const char *fp = pprintf_scan(s, format);
    char str[25];

#ifdef DEBUG
    if (*fp == 0 || fp[1] != 'l' || fp[2] != 'd')	/* shouldn't happen! */
	lprintf1("Bad format in pprintld: %s\n", format);
#endif
    sprintf(str, "%ld", v);
    pputs(s, str);
    return pprintf_scan(s, fp + 3);
}
const char *
pprintld2(stream * s, const char *format, long v1, long v2)
{
    return pprintld1(s, pprintld1(s, format, v1), v2);
}
const char *
pprintld3(stream * s, const char *format, long v1, long v2, long v3)
{
    return pprintld2(s, pprintld1(s, format, v1), v2, v3);
}

/* Print (a) string(s) using a format. */
const char *
pprints1(stream * s, const char *format, const char *str)
{
    const char *fp = pprintf_scan(s, format);

#ifdef DEBUG
    if (*fp == 0 || fp[1] != 's')	/* shouldn't happen! */
	lprintf1("Bad format in pprints: %s\n", format);
#endif
    pputs(s, str);
    return pprintf_scan(s, fp + 2);
}
const char *
pprints2(stream * s, const char *format, const char *str1, const char *str2)
{
    return pprints1(s, pprints1(s, format, str1), str2);
}
const char *
pprints3(stream * s, const char *format, const char *str1, const char *str2,
	 const char *str3)
{
    return pprints2(s, pprints1(s, format, str1), str2, str3);
}
