/*
 * strlcpy() - Copy as much of the source string to the
 * destination as possible, ensuring we NUL-terminate the
 * result.  Return the length of src.
 *
 * Dan Cross <net!gajendra!cross>
 */

#include <u.h>
#include <libc.h>

size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t len, srclen;

	/*
	 * Get the length of the source first, test for the
	 * pathological case, then copy as much as we can.
	 */
	srclen = strlen(src);
	if (size-- == 0)
		return(srclen);
	len = (size < srclen) ? size : srclen;
	memmove(dst, src, len);
	dst[len] = '\0';

	return(srclen);
}
