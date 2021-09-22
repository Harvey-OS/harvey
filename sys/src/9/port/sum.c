#include <u.h>

ulong
checksum(void *vbuf, uint len)
{
	ulong sum;
	uchar *buf;

	buf = vbuf;
	sum = 0;
#ifdef SUM_WORDS
	ulong l;

	/* sum bytes to next word boundary (if any) */
	if ((uintptr)buf & (sizeof l - 1)) {
		int n;

		n = sizeof l - ((uintptr)buf & (sizeof l - 1));
		if (n > len)
			n = len;
		len -= n;
		while (n-- > 0)
			sum += *buf++;
	}

	/* sum words */
	for (; len >= sizeof l; len -= sizeof l) {
		l = *(ulong *)buf;
		sum += (uchar)l + (uchar)(l>>8) +
			(uchar)(l>>16) + (uchar)(l>>24);
		buf += sizeof l;
	}

	/* sum straggling bytes */
#endif
	while (len-- > 0)
		sum += *buf++;
	return sum;
}
