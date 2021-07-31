#include "u.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "libc.h"

/*
 * Portable version for systems from the ancien regime
 */

void*
memmove(void *a1, void *a2, Ulong n)
{
	char *s1, *s2;

	if(a1 > a2)
		goto back;
	s1 = a1;
	s2 = a2;
	while(n > 0) {
		*s1++ = *s2++;
		n--;
	}
	return a1;

back:
	s1 = (char*)a1 + n;
	s2 = (char*)a2 + n;
	while(n > 0) {
		*--s1 = *--s2;
		n--;
	}
	return a1;
}
