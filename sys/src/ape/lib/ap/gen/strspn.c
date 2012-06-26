#include <string.h>

#define	N	256

size_t
strspn(const char *s, const char *b)
{
	char map[N], *os;

	memset(map, 0, N);
	while(*b)
		map[*(unsigned char *)b++] = 1;
	os = s;
	while(map[*(unsigned char *)s++])
		;
	return s - os - 1;
}
