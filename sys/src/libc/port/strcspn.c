#include <u.h>
#include <libc.h>

#define	N	256

long
strcspn(char *s, char *b)
{
	char map[N], *os;

	memset(map, 0, N);
	for(;;) {
		map[*(uchar*)b] = 1;
		if(*b++ == 0)
			break;
	}
	os = s;
	while(map[*(uchar*)s++] == 0)
		;
	return s - os - 1;
}
