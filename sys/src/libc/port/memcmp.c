#include	<u.h>
#include	<libc.h>

int
memcmp(void *a1, void *a2, ulong n)
{
	uchar *s1, *s2;
	uint c1, c2;

	s1 = a1;
	s2 = a2;
	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		n--;
	}
	return 0;
}
