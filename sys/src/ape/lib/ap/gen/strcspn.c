#include <string.h>

#define	N	256

size_t
strcspn(const char *s, const char *b)
{
	char map[N], *os;

	memset(map, 0, N);
	for(;;) {
		map[*(unsigned char*)b] = 1;
		if(*b++ == 0)
			break;
	}
	os = s;
	while(map[*(unsigned char*)s++] == 0)
		;
	return s - os - 1;
}
