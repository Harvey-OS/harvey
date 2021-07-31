#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_error.c,v 1.13 92/02/10 19:06:34 sam Exp $";
#endif

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 */
#include <stdio.h>
#include "tiffioP.h"
#include "prototypes.h"

static void
DECLARE3(defaultHandler, char*, module, char*, fmt, va_list, ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}

static TIFFErrorHandler _errorHandler = defaultHandler;

TIFFErrorHandler
DECLARE1(TIFFSetErrorHandler, TIFFErrorHandler, handler)
{
	TIFFErrorHandler prev = _errorHandler;
	_errorHandler = handler;
	return (prev);
}

void
#if USE_PROTOTYPES
TIFFError(char *module, char *fmt, ...)
#else
/*VARARGS2*/
TIFFError(module, fmt, va_alist)
	char *module;
	char *fmt;
	va_dcl
#endif
{
	if (_errorHandler) {
		va_list ap;
		VA_START(ap, fmt);
		(*_errorHandler)(module, fmt, ap);
		va_end(ap);
	}
}
