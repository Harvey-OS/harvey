#include <u.h>
#include <libc.h>

#define	N	256

long
strspn(char *s, char *b)
{
	char map[N], *os;

	memset(map, 0, N);
	while(*b)
		map[*(uchar *)b++] = 1;
	os = s;
	while(map[*(uchar *)s++])
		;
	return s - os - 1;
}
