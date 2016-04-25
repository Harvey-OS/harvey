/*
 * strlcat() - Concatenate as much data as we can onto a
 * fixed-sized string, ensuring the result is NUL-terminated.
 * Return the total number of bytes required to concatenate
 * all of the src string onto dst.
 *
 * Dan Cross <net!gajendra!cross>
 */

#include <u.h>
#include <libc.h>

size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t len, n;

	/*
	 * This mimics the OpenBSD semantics most closely.
	 *
	 * We would like to use strlen() here, but the idea is to catch
	 * cases where dst isn't a C-style string, and this function is
	 * (presumably) called in error.
	 */
	for (n = size, len = 0; n > 0 && dst[len] != '\0'; n--)
		len++;
	return(strlcpy(dst + len, src, n) + len);
}
