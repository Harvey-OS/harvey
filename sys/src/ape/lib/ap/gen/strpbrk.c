#include <string.h>
#define	N	256

char*
strpbrk(const char *s, const char *b)
{
	char map[N];

	memset(map, 0, N);
	for(;;) {
		map[*b] = 1;
		if(*b++ == 0)
			break;
	}
	while(map[*s++] == 0)
		;
	if(*--s)
		return s;
	return 0;
}
