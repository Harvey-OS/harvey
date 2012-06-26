#include <string.h>

size_t
strxfrm(char *s1, const char *s2, size_t n)
{
	/*
	 * BUG: supposed to transform s2 to a canonical form
	 * so that strcmp can be used instead of strcoll, but
	 * our strcoll just uses strcmp.
	 */

	size_t xn = strlen(s2);
	if(n > xn)
		n = xn;
	memcpy(s1, s2, n);
	return xn;
}
