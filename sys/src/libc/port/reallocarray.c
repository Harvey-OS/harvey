/*	$OpenBSD: reallocarray.c,v 1.3 2015/09/13 08:31:47 guenther Exp $	*/
/*
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <u.h>
#include <libc.h>

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define SIZE_MAX ~(size_t)0
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
	/* If both size or nmemb are 0, and realloc is called with
	 * 0, it's equivalent to freeing the memory. realloc is even
	 * allowed to return nil. Hence this function needs to distinguish
	 * between 'pointer to 0 length array' and 'nil pointer'. In the
	 * event that nmemb * size is zero: free optr, and malloc(0),
	 * i.e. return a pointer to a zero-length array.
	 * It is entirely possible that it will return optr, since the allocators
	 * tend to like to optimize that case, but the bookkeeping
	 * will be correct: the allocator will know that optr points to a
	 * 0-length allocation.
	 */
	if (nmemb == 0 || size == 0) {
		free(optr);
		return malloc(0);
	}

	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb > 0 && SIZE_MAX / nmemb < size) {
		return nil;
	}
	return realloc(optr, size * nmemb);
}
